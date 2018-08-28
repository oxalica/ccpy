#include "./parser.h"
#include <utility>
#include "../ast/expr.h"
#include "../util/adt.h"
using namespace std;
using namespace ccpy::ast;

namespace ccpy::parse {

namespace {

using OptTokRef = optional<const Token &>;

bool is_expr_begin(OptTokRef tok) {
  if(!tok)
    return false;
  return match<bool>(*tok
  , [](const TokName &) { return true; }
  , [](const TokInteger &) { return true; }
  , [](const TokString &) { return true; }
  , [](const TokKeyword &tok) {
    switch(tok.keyword) {
      case Keyword::True: case Keyword::False: case Keyword::None: // Literal
      case Keyword::Not: // Unary op
        return true;
      default:
        return false;
    }
  }
  , [](const TokSymbol &tok) {
    switch(tok.symbol) {
      case Symbol::DotDotDot: // `...`
      case Symbol::Inv: case Symbol::Add: case Symbol::Sub: // Unary op
      case Symbol::LParen: // Group or tuple
      case Symbol::LBrace: // Dict literal
        return true;
      default:
        return false;
    }
  }
  , [](const auto &) { return false; }
  );
}

bool is_symbol(OptTokRef tok, Symbol sym) {
  if(!tok)
    return false;
  return match<bool>(*tok,
    [=](const TokSymbol &tok) { return tok.symbol == sym; },
    [](const auto &) { return false; }
  );
}

bool is_keyword(OptTokRef tok, Keyword kw) {
  if(!tok)
    return false;
  return match<bool>(*tok,
    [=](const TokKeyword &tok) { return tok.keyword == kw; },
    [](const auto &) { return false; }
  );
}

bool is_param_begin(OptTokRef tok) {
  if(!tok)
    return false;
  return match<bool>(*tok
  , [](const TokName &) { return true; }
  , [](const TokSymbol &tok) { return tok.symbol == Symbol::Mul; }
  , [](const auto &) { return false; }
  );
}

Pat expr_to_pat(Expr &&expr) {
  return match<Pat>(move(expr)
  , [](ExprName &&expr) { return PatName { move(expr.name) }; }
  , [](ExprMember &&expr) {
    return PatAttr { move(*expr.obj), move(expr.member) };
  }
  , [](ExprIndex &&expr) {
    return PatIndex { move(*expr.obj), move(*expr.idx) };
  }
  , [](ExprTuple &&expr) {
    vector<Pat> pats;
    for(auto &e: expr.elems)
      pats.push_back(expr_to_pat(move(e)));
    return PatTuple { move(pats) };
  }
  , [](auto &&) -> Pat {
    throw StreamFailException { "Invalid pattern expression" };
  }
  );
}

} // namespace anonymouse

struct Parser::Impl {
  IBufSource<Token> &is;

  template<typename ...Fs>
  bool try_eat(bool eof_ok, Fs ...fs) {
    auto tok = this->is.peek();
    if(tok
        ? match<bool>(*tok, fs..., [](const auto &) { return false; })
        : eof_ok) {
      this->is.get();
      return true;
    }
    return false;
  }

  template<typename ...Fs>
  void expect(const char msg[], bool eof_ok, Fs ...fs) {
    auto tok = this->is.get();
    if(tok ? match<bool>(move(*tok), fs...) : eof_ok)
      return; // Ok
    throw StreamFailException { msg };
  }

  bool try_eat_symbol(Symbol sym) {
    return this->try_eat(
      false,
      [=](const TokSymbol &tok) { return tok.symbol == sym; }
    );
  }

  bool try_eat_keyword(Keyword kw) {
    return this->try_eat(
      false,
      [=](const TokKeyword &tok) { return tok.keyword == kw; }
    );
  }

  void expect_symbol(const char msg[], Symbol sym) {
    this->expect(msg, false
    , [=](TokSymbol &&tok) { return tok.symbol == sym; }
    , [](auto &&) { return false; }
    );
  }

  void expect_keyword(const char msg[], Keyword kw) {
    this->expect(msg, false
    , [=](TokKeyword &&tok) { return tok.keyword == kw; }
    , [](auto &&) { return false; }
    );
  }

  Str expect_name(const char msg[]) {
    Str ret;
    this->expect(msg, false
    , [&](TokName &&tok) { ret = move(tok.name); return true; }
    , [](auto &&) { return false; }
    );
    return ret;
  }

  void expect_newline() {
    this->expect(
      "Expect newline",
      false,
      [](TokNewline &&) { return true; },
      [](auto &&) { return false; }
    );
  }

  vector<Stmt> get_stmt_or_suite() {
    auto nxt = this->is.peek();
    if(!nxt)
      throw StreamFailException { "Expect stmt or suite, found EOF" };
    bool is_suite = this->try_eat(
      false,
      [](const TokNewline &) { return true; }
    );

    vector<Stmt> ret;
    if(!is_suite) {
      ret.push_back(this->get_stmt());
      return ret;
    }

    this->expect("Expect indent", false,
      [](TokIndent &&) { return true; },
      [](auto &&) { return false; }
    );
    do
      ret.push_back(this->get_stmt());
    while(!this->try_eat(true, [](const TokDedent &) { return true; }));
    return ret;
  }

  optional<Stmt> try_get_stmt() {
    if(!this->is.peek())
      return {};
    return this->get_stmt();
  }

  Stmt get_stmt() {
    auto nxt = this->is.peek();
    if(!nxt)
      throw StreamFailException { "Expect stmt, found EOF" };

    if(is_expr_begin(nxt))
      return this->get_stmt_ahead_expr();

    return match<Stmt>(*nxt
    , [&](const TokKeyword &tok) {
      return this->get_stmt_ahead_kw(tok.keyword);
    }
    , [&](const auto &) -> Stmt {
      throw StreamFailException { "Unexpected token for stmt" };
    }
    );
  }

  Stmt get_stmt_ahead_expr() {
    vector<Pat> pats;
    auto expr = *this->get_expr_list_maybe_tuple(); // Checked
    while(this->try_eat_symbol(Symbol::Assign)) {
      pats.push_back(expr_to_pat(move(expr)));
      auto c = this->get_expr_list_maybe_tuple();
      if(!c)
        throw StreamFailException { "Empty expr" };
      expr = move(*c);
    }
    this->expect_newline();
    if(pats.empty())
      return StmtExpr { move(expr) };
    return StmtAssign { move(pats), move(expr) };
  }

  Stmt get_stmt_ahead_kw(Keyword kw) {
    switch(kw) {
      case Keyword::Pass:
        this->is.get();
        this->expect_newline();
        return StmtPass {};

      case Keyword::Del: {
        this->is.get();
        auto pat = this->get_pat();
        this->expect_newline();
        return StmtDel { move(pat) };
      }

      case Keyword::Nonlocal: {
        this->is.get();
        auto names = this->get_name_list();
        this->expect_newline();
        return StmtNonlocal { move(names) };
      }

      case Keyword::Global: {
        this->is.get();
        auto names = this->get_name_list();
        this->expect_newline();
        return StmtGlobal { move(names) };
      }

      case Keyword::Raise: {
        this->is.get();
        auto c = this->get_expr_list_maybe_tuple();
        if(!c)
          throw StreamFailException { "Raise without value is unsupported" };
        this->expect_newline();
        return StmtRaise { move(*c) };
      }

      case Keyword::Return: {
        this->is.get();
        auto c = this->get_expr_list_maybe_tuple();
        this->expect_newline();
        return StmtReturn { move(c) };
      }

      case Keyword::Yield: {
        this->is.get();
        auto c = this->get_expr_list_maybe_tuple();
        this->expect_newline();
        return StmtYield { move(c) };
      }

      case Keyword::Def:
        return this->get_stmt_ahead_def(); // Already eat newline

      case Keyword::If:
        this->is.get();
        return this->get_stmt_after_if(); // Already eat newline

      case Keyword::Class:
        this->is.get();
        return this->get_stmt_after_class(); // Already eat newline

      case Keyword::Try:
        return this->get_stmt_ahead_try(); // Newline eaten

      default:
        throw StreamFailException { "Unexpected keyword for stmt" };
    }
  }

  Stmt get_stmt_ahead_try() {
    this->is.get();
    this->expect_symbol("Expect `:` after try", Symbol::Colon);
    auto stmts = this->get_stmt_or_suite();

    this->expect_keyword("Expect `except`", Keyword::Except);
    this->expect(
      "Now support catching `Exception` only",
      false,
      [](TokName &&tok) { return tok.name == "Exception"; },
      [](auto &&) { return false; }
    );
    this->expect_keyword("Expect `as`", Keyword::As);
    auto name = this->expect_name("Expect name for binding exception");
    this->expect_symbol("Expect `:` after except signature", Symbol::Colon);
    auto excepts = this->get_stmt_or_suite();

    return StmtTry {
      move(stmts),
      move(name),
      move(excepts),
    };
  }

  Stmt get_stmt_after_class() {
    auto name = this->expect_name("Expect class name");
    this->expect_symbol("Expect `(` after class name", Symbol::LParen);
    auto base = this->get_expr();
    this->expect_symbol("Expect `)` after class base", Symbol::RParen);
    this->expect_symbol("Expect `:` after class signature", Symbol::Colon);
    auto body = this->get_stmt_or_suite();
    return StmtClass { move(name), move(base), move(body) };
  }

  Stmt get_stmt_after_if() {
    auto cond = this->get_expr();
    this->expect_symbol("Expect `:` after if condition", Symbol::Colon);
    auto thens = this->get_stmt_or_suite();
    vector<Stmt> elses {};
    if(this->try_eat_keyword(Keyword::Elif)) {
      elses.push_back(this->get_stmt_after_if());
    } else if(this->try_eat_keyword(Keyword::Else)) {
      this->expect_symbol("Expect `:` after `else`", Symbol::Colon);
      elses = this->get_stmt_or_suite();
    }
    return StmtIf { move(cond), move(thens), move(elses) };
  }

  Stmt get_stmt_ahead_def() {
    this->is.get(); // `def`
    Str name = this->expect_name("Expect name after `def`");
    this->expect_symbol("Expect `(` after function name", Symbol::LParen);
    auto args = this->get_func_params();
    this->expect_symbol("Expect `)` after function params", Symbol::RParen);
    this->expect_symbol("Expect `:` after function signature", Symbol::Colon);
    auto body = this->get_stmt_or_suite();

    return StmtDef {
      move(name),
      move(args.first),
      move(args.second),
      move(body),
    };
  }

  // Return (parameters, optional rest parameter).
  pair<vector<FuncArg>, optional<Str>> get_func_params() {
    vector<FuncArg> args;
    optional<Str> rest_arg;
    bool has_default = false;

    while(is_param_begin(this->is.peek())) {
      match(*this->is.get() // Checked
      , [&](TokName &&tok) {
        optional<Expr> default_;
        if(this->try_eat_symbol(Symbol::Assign)) {
          default_ = this->get_expr();
          has_default = true;
        } else if(has_default) // Previous param has default, but this not.
          throw StreamFailException
            { "Params after one having default should all have default" };
        else if(rest_arg)
          throw StreamFailException { "Rest param should be the last param" };
        args.push_back(FuncArg { move(tok.name), move(default_) });
      }
      , [&](TokSymbol &&tok) {
        if(tok.symbol == Symbol::Mul) {
          if(rest_arg) // Always have it
            throw StreamFailException { "At most one rest param is allowed" };
          rest_arg = this->expect_name("Expect name for rest param");
        } else
          throw StreamFailException
            { "Impossible: unexpected symbol for param" };
      }
      , [](auto &&) {
        throw StreamFailException { "Impossible: unexpected token for param" };
      }
      );
      if(!this->try_eat_symbol(Symbol::Comma))
        break;
    }
    return { move(args), move(rest_arg) };
  }

  // Eat `name (, name)*`. Note that tailing comma is unexpected.
  vector<Str> get_name_list() {
    vector<Str> ret;
    ret.push_back(this->expect_name("Expect name in name list"));
    while(this->try_eat_symbol(Symbol::Comma))
      ret.push_back(this->expect_name("Expect name in name list"));
    return ret;
  }

  Pat get_pat() {
    auto c = this->get_expr_list_maybe_tuple();
    if(!c)
      throw StreamFailException { "Empty pat" };
    return expr_to_pat(move(*c));
  }

  // Parse `e` as expr, `(e,)+ [e]` as tuple.
  // Return None if it eat nothing.
  optional<Expr> get_expr_list_maybe_tuple() {
    auto ret = this->get_expr_list();
    if(ret.first.empty())
      return {};
    return ret.first.size() == 1 && !ret.second // Length == 1, no tailing comma
      ? move(ret.first.front())
      : ExprTuple { move(ret.first) };
  }

  /// Return (expressions, whether_has_tailing_comma)
  pair<vector<Expr>, bool> get_expr_list() {
    vector<Expr> v;
    while(is_expr_begin(this->is.peek())) {
      v.push_back(this->get_expr());
      if(!this->try_eat_symbol(Symbol::Comma))
        return make_pair(move(v), false);
    }
    return make_pair(move(v), true);
  }

  Expr get_expr() {
    return this->get_expr_cond();
  }

  Expr get_expr_cond() {
    Expr c = this->get_expr_logic1();
    if(!is_keyword(this->is.peek(), Keyword::If))
      return move(c);
    this->is.get();

    auto cond = this->get_expr_logic1(); // No recur in `cond`
    this->expect_keyword("Expect `else` in condition expr", Keyword::Else);
    auto else_expr = this->get_expr_cond(); // Recur in `else_expr`
    return ExprCond { move(cond), move(c), move(else_expr) };
  }

  Expr get_expr_logic1() {
    Expr c = this->get_expr_logic2();
    if(!is_keyword(this->is.peek(), Keyword::Or))
      return move(c);
    this->is.get();

    return ExprBinary {
      BinaryOp::LogOr,
      move(c),
      this->get_expr_logic1(), // Recur
    };
  }

  Expr get_expr_logic2() {
    Expr ret = this->get_expr_logic3();
    if(!is_keyword(this->is.peek(), Keyword::And))
      return ret;
    this->is.get();

    return ExprBinary {
      BinaryOp::LogAnd,
      move(ret),
      this->get_expr_logic2(), // Recur
    };
  }

  Expr get_expr_logic3() {
    if(!is_keyword(this->is.peek(), Keyword::Not))
      return this->get_expr_relation();
    this->is.get();

    return ExprUnary { UnaryOp::LogNot, this->get_expr_logic3() }; // Recur
  }

  Expr get_expr_relation() {
    vector<Expr> exprs;
    vector<RelationOp> ops;
    exprs.push_back(this->get_expr_val());

    while(auto op = this->try_trans_relation_op()) {
      exprs.push_back(this->get_expr_val());
      ops.push_back(*op);
    }

    if(exprs.size() == 1)
      return move(exprs.front());
    return ExprRelation { move(exprs), move(ops) };
  }

  optional<RelationOp> try_trans_relation_op() {
    auto tok = this->is.peek();
    if(!tok)
      return {};
    optional<RelationOp> op;
    match(*tok
    , [&](const TokSymbol &tok) {
      switch(tok.symbol) {
        case Symbol::Lt: op = RelationOp::Lt; break;
        case Symbol::Gt: op = RelationOp::Gt; break;
        case Symbol::Le: op = RelationOp::Le; break;
        case Symbol::Ge: op = RelationOp::Ge; break;
        case Symbol::Eq: op = RelationOp::Eq; break;
        case Symbol::Ne: op = RelationOp::Ne; break;
        default: ;
      }
      if(op)
        this->is.get();
    }
    , [&](const TokKeyword &tok) {
      if(tok.keyword == Keyword::Is) {
        op = RelationOp::Is;
        this->is.get();
        if(this->try_eat_keyword(Keyword::Not))
          op = RelationOp::Ns;
      }
    }
    , [&](const auto &) {}
    );
    return op;
  }



  Expr get_expr_val() {
    Expr ret = this->get_expr_term();
    for(;;) {
      auto tok = this->is.peek();
      BinaryOp op;
      if(is_symbol(tok, Symbol::Add))
        op = BinaryOp::Add;
      else if(is_symbol(tok, Symbol::Sub))
        op = BinaryOp::Sub;
      else
        break;

      this->is.get();
      ret = ExprBinary { op, move(ret), this->get_expr_term() };
    }
    return ret;
  }

  Expr get_expr_term() {
    Expr ret = this->get_expr_factor();
    for(;;) {
      auto tok = this->is.peek();
      BinaryOp op;
      if(is_symbol(tok, Symbol::Mul))
        op = BinaryOp::Mul;
      else if(is_symbol(tok, Symbol::Div))
        op = BinaryOp::Div;
      else if(is_symbol(tok, Symbol::Mod))
        op = BinaryOp::Mod;
      else
        break;

      this->is.get();
      ret = ExprBinary { op, move(ret), this->get_expr_factor() };
    }
    return ret;
  }

  Expr get_expr_factor() {
    auto tok = this->is.peek();
    if(!tok)
      throw StreamFailException { "Expect expr atom, found EOF" };

    UnaryOp op;
    if(is_symbol(*tok, Symbol::Add))
      op = UnaryOp::Pos;
    else if(is_symbol(*tok, Symbol::Sub))
      op = UnaryOp::Neg;
    else if(is_symbol(*tok, Symbol::Inv))
      op = UnaryOp::Inv;
    else
      return this->get_expr_atom();

    this->is.get();
    return ExprUnary { op, this->get_expr_factor() }; // Recur
  }

  Expr get_expr_atom() {
    Expr ret = this->get_atom();
    for(;;) {
      auto tok = this->is.peek();
      if(is_symbol(tok, Symbol::LParen)) {
        // Function call
        this->is.get();
        ret = ExprCall { move(ret), this->get_expr_list().first };
        this->expect_symbol("Expecting `)`", Symbol::RParen);
      } else if(is_symbol(tok, Symbol::LBracket)) {
        // Index
        this->is.get();
        auto c = this->get_expr_list_maybe_tuple();
        if(!c)
          throw StreamFailException { "Empty index" };
        ret = ExprIndex { move(ret), move(*c) };
        this->expect_symbol("Expecting `]`", Symbol::RBracket);
      } else if(is_symbol(tok, Symbol::Dot)) {
        // Member access
        this->is.get();
        auto member = this->is.get();
        if(!member)
          throw StreamFailException { "Expect member name, found EOF" };
        match(move(*member)
        , [&](TokName &&tok) {
          ret = ExprMember { move(ret), move(tok.name) };
        }
        , [](auto &&) {
          throw StreamFailException { "Expect member name" };
        }
        );
      } else
        break;
    }
    return ret;
  }

  Expr get_atom() {
    return match<Expr>(*this->is.get() // Must not EOF. Checked as expr begin
    , [&](TokKeyword &&tok) {
      switch(tok.keyword) {
        case Keyword::True:  return ExprLiteral { LitBool { true } };
        case Keyword::False: return ExprLiteral { LitBool { false } };
        case Keyword::None:  return ExprLiteral { LitNone {} };
        default:
          throw StreamFailException { "Unexpected keyword for atom" };
      }
    }
    , [&](TokSymbol &&tok) -> Expr {
      switch(tok.symbol) {
        case Symbol::DotDotDot:
          return ExprLiteral { LitEllipse {} };

        case Symbol::LParen: { // Group of tuple literal
          auto ret = this->get_expr_list();
          this->expect_symbol("Expecting `)`", Symbol::RParen);
          return ret.first.size() == 1 && !ret.second
            ? move(ret.first.front())
            : ExprTuple { move(ret.first) };
        }

        case Symbol::LBrace: { // Dict literal
          auto ret = this->get_dict_kvs();
          this->expect_symbol("Expecting `}`", Symbol::RBrace);
          return ExprDict { move(ret) };
        }

        default:
          throw StreamFailException { "Unexpected symbol for atom" };
      }
    }
    , [](TokName &&tok) {
      return ExprName { move(tok.name) };
    }
    , [](TokString &&tok) {
      return ExprLiteral { LitStr { move(tok.str) } };
    }
    , [](TokInteger &&tok) {
      return ExprLiteral { LitInteger { move(tok.integer) } };
    }
    , [](auto &&) -> Expr { // never
      throw StreamFailException { "Unexpected token for atom" };
    }
    );
  }

  vector<pair<Expr, Expr>> get_dict_kvs() {
    vector<pair<Expr, Expr>> kvs;
    while(is_expr_begin(this->is.peek())) {
      auto k = this->get_expr();
      this->expect_symbol("Expect `:` between key and value", Symbol::Colon);
      auto v = this->get_expr();
      kvs.emplace_back(move(k), move(v));
      if(!this->try_eat_symbol(Symbol::Comma))
        break;
    }
    return kvs;
  }
};

Parser::Parser(IBufSource<Token> &is)
  : pimpl(Impl { is }) {}

Parser::~Parser() noexcept {}

optional<Stmt> Parser::get() {
  return this->pimpl->try_get_stmt();
}

} // namespace ccpy::parse
