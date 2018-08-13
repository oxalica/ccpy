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
  , [](const TokKeyword &tok) {
    switch(tok.keyword) {
      case Keyword::True: case Keyword::False: case Keyword::None:
        return true;
      default:
        return false;
    }
  }
  , [](const TokSymbol &tok) {
    switch(tok.symbol) {
      case Symbol::DotDotDot: // `...`
      case Symbol::Not: case Symbol::Add: case Symbol::Sub: // Unary op
      case Symbol::LParen: // Group or tuple
        return true;
      default:
        return false;
    }
  }
  , [](auto &) { return false; }
  );
}

bool is_symbol(OptTokRef tok, Symbol sym) {
  if(!tok)
    return false;
  return match<bool>(*tok,
    [=](const TokSymbol &tok) { return tok.symbol == sym; },
    [](auto &) { return false; }
  );
}

} // namespace anonymouse

struct Parser::Impl {
  IBufSource<Token> &is;

  template<typename ...Fs>
  bool try_eat(bool eof_ok, Fs ...fs) {
    auto tok = this->is.peek();
    if(tok ? match<bool>(*tok, fs...) : eof_ok) {
      this->is.get();
      return true;
    }
    return false;
  }

  template<typename ...Fs>
  void expect(const char msg[], bool eof_ok, Fs ...fs) {
    if(!this->try_eat(eof_ok, fs...))
      throw StreamFailException { msg };
  }

  bool try_eat_symbol(Symbol sym) {
    return this->try_eat(false
    , [=](const TokSymbol &tok) { return tok.symbol == sym; }
    , [](auto &) { return false; }
    );
  }

  void expect_newline() {
    this->expect("Expecting newline", true
    , [](const TokNewline &) { return true; }
    , [](const TokDedent &) { return true; } // Dedent before EOF
    , [](auto &) { return false; }
    );
  }

  void expect_symbol(const char msg[], Symbol sym) {
    return this->expect(msg, false
    , [=](const TokSymbol &tok) { return tok.symbol == sym; }
    , [](auto &) { return false; }
    );
  }

  optional<Stmt> get_stmt() {
    auto nxt = this->is.peek();
    if(!nxt)
      return {};
    if(is_expr_begin(this->is.peek())) {
      auto ret = this->get_expr_list();
      this->expect_newline();
      auto expr = ret.first.size() == 1 && !ret.second
        ? move(ret.first.front())
        : ExprTuple { move(ret.first) };
      return StmtExpr { move(expr) };
    }
    return match<Stmt>(*nxt
    , [&](const TokKeyword &tok) {
      switch(tok.keyword) {
        case Keyword::Pass:
          this->is.get();
          this->expect_newline();
          return StmtPass {};
        default:
          throw StreamFailException { "Unexpected keyword for stmt" };
      }
    }
    , [&](auto &&) -> Stmt {
      throw StreamFailException { "Unexpected token for stmt" };
    }
    );
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
    return this->get_expr_val();
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
    else if(is_symbol(*tok, Symbol::Not))
      op = UnaryOp::Not;
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
        case Symbol::LParen: {
          auto ret = this->get_expr_list();
          this->expect_symbol("Expecting `)`", Symbol::RParen);
          return ret.first.size() == 1 && !ret.second
            ? move(ret.first.front())
            : ExprTuple { move(ret.first) };
        }
        default:
          throw StreamFailException { "Unexpected symbol for atom" };
      }
    }
    , [](TokName &&tok) {
      return ExprName { move(tok.name) };
    }
    , [](TokInteger &&tok) {
      return ExprLiteral { LitInteger { move(tok.integer) } };
    }
    , [](auto &&) -> Expr { // never
      throw StreamFailException { "Unexpected token for atom" };
    }
    );
  }
};

Parser::Parser(IBufSource<Token> &is)
  : pimpl(Impl { is }) {}

Parser::~Parser() noexcept {}

optional<Stmt> Parser::get() {
  return this->pimpl->get_stmt();
}

} // namespace ccpy::parse
