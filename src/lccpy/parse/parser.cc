#include "./parser.h"
#include <optional>
#include <utility>
using namespace std;
using namespace ccpy::ast;

namespace ccpy::parse {

namespace {

using OptTok = optional<Token>;

bool is_expr_begin(const OptTok &tok) {
  if(!tok)
    return false;
  return match(*tok, [](const auto &tok) -> bool {
    using T = decay_t<decltype(tok)>;
    if constexpr(is_same_v<T, TokSymbol>)
      switch(tok.symbol) {
        case Symbol::DotDotDot: // `...`
        case Symbol::Not: case Symbol::Add: case Symbol::Sub: // Unary op
          return true;
        default:
          return false;
      }
    else if constexpr(is_same_v<T, TokName> || is_same_v<T, TokInteger>)
      return true;
    else
      return false;
  });
}

bool is_symbol(const OptTok &tok, Symbol sym) {
  if(!tok)
    return false;
  return match(*tok,
    [=](const TokSymbol &tok) { return tok.symbol == sym; },
    [](auto &) { return false; }
  );
}

} // namespace anonymouse

struct Parser::Impl {
  IBufSource<Token> &is;

  optional<Stmt> get_stmt() {
    auto &nxt = this->is.peek();
    if(!nxt)
      return {};
    return match(*nxt, [&](const auto &tok) -> Stmt {
      using T = decay_t<decltype(tok)>;
      if constexpr(is_same_v<T, TokKeyword>)
        switch(tok.keyword) {
          case Keyword::Pass:
            return StmtPass {};
          default:
            throw StreamFailException { "Unexcepted keyword" };
        }
      else {
        auto ret = this->get_expr_list();
        auto expr = ret.first.size() == 1 && !ret.second
          ? move(ret.first.front())
          : ExprTuple { move(ret.first) };
        return StmtExpr { move(expr) };
      }
    });
  }

  /// Return (expressions, whether_has_tailing_comma)
  pair<vector<Expr>, bool> get_expr_list() {
    vector<Expr> v;
    while(is_expr_begin(this->is.peek())) {
      v.push_back(this->get_expr());
      if(!this->expect_comma())
        return make_pair(move(v), false);
    }
    return make_pair(move(v), true);
  }

  bool expect_comma() {
    if(auto &tok = this->is.peek())
      if(is_symbol(tok, Symbol::Comma)) {
        this->is.get();
        return true;
      }
    return false;
  }

  Expr get_expr() {
    return this->get_expr_val();
  }

  Expr get_expr_val() {
    Expr ret = this->get_expr_term();
    for(;;) {
      auto &tok = this->is.peek();
      optional<BinaryOp> op;
      if(is_symbol(tok, Symbol::Add))
        op = BinaryOp::Add;
      else if(is_symbol(tok, Symbol::Sub))
        op = BinaryOp::Sub;
      else
        break;

      this->is.get();
      auto lexpr = make_owned(move(ret));
      auto rexpr = make_owned(this->get_expr_term());
      ret = Expr { ExprBinary { *op, move(lexpr), move(rexpr) } };
    }
    return ret;
  }

  Expr get_expr_term() {
    Expr ret = this->get_expr_factor();
    for(;;) {
      auto &tok = this->is.peek();
      optional<BinaryOp> op;
      if(is_symbol(tok, Symbol::Mul))
        op = BinaryOp::Mul;
      else if(is_symbol(tok, Symbol::Div))
        op = BinaryOp::Div;
      else if(is_symbol(tok, Symbol::Mod))
        op = BinaryOp::Mod;
      else
        break;

      this->is.get();
      auto lexpr = make_owned(move(ret));
      auto rexpr = make_owned(this->get_expr_term());
      ret = Expr { ExprBinary { *op, move(lexpr), move(rexpr) } };
    }
    return ret;
  }

  Expr get_expr_factor() {
    auto &tok = this->is.peek();
    if(!tok)
      throw StreamFailException { "Expect expr atom, found EOF" };

    optional<UnaryOp> op;
    if(is_symbol(*tok, Symbol::Add))
      op = UnaryOp::Pos;
    else if(is_symbol(*tok, Symbol::Sub))
      op = UnaryOp::Neg;
    else if(is_symbol(*tok, Symbol::Not))
      op = UnaryOp::Not;

    if(!op)
      return this->get_expr_atom();

    this->is.get();
    auto expr = make_owned(this->get_expr_factor()); // Recur
    return ExprUnary { *op, move(expr) };
  }

  Expr get_expr_atom() {
    Expr ret = this->get_atom();
    for(;;) {
      auto &tok = this->is.peek();
      if(is_symbol(tok, Symbol::LParen)) {
        // Function call
        this->is.get();
        auto args = this->get_expr_list().first;
        auto fn = make_owned(move(ret));
        ret = Expr { ExprCall { move(fn), move(args) } };
      } else if(is_symbol(tok, Symbol::Dot)) {
        // Member access
        this->is.get();
        auto member = this->is.get();
        if(!member)
          throw StreamFailException { "Expect member name, found EOF" };
        match(move(*member), [&](auto &&tok) {
          using T = decay_t<decltype(tok)>;
          if constexpr(is_same_v<T, TokName>) {
            auto obj = make_owned(move(ret));
            ret = Expr { ExprMember { move(obj), move(tok.name) } };
          } else
            throw StreamFailException { "Expect member name" };
        });
      } else
        break;
    }
    return ret;
  }

  Expr get_atom() {
    return match(move(*this->is.get()), [&](auto &&tok) -> Expr {
      using T = decay_t<decltype(tok)>;
      if constexpr(is_same_v<T, TokSymbol>)
        switch(tok.symbol) {
          case Symbol::DotDotDot:
            return ExprLiteral { LitEllipse {} };
          case Symbol::LParen: {
            auto ret = this->get_expr_list();
            return ret.first.size() == 1 && !ret.second
              ? move(ret.first.front())
              : ExprTuple { move(ret.first) };
          }
          default:
            throw StreamFailException { "Unexpected symbol for atom" };
        }
      else if constexpr(is_same_v<T, TokName>)
        return ExprName { move(tok.name) };
      else if constexpr(is_same_v<T, TokInteger>)
        return ExprLiteral { LitInteger { move(tok.integer) } };
      else
        throw StreamFailException { "Unexpected token for atom" };
    });
  }
};

Parser::Parser(IBufSource<Token> &is)
  : pimpl(Impl { is }) {}

Parser::~Parser() noexcept {}

optional<Stmt> Parser::get() {
  return this->pimpl->get_stmt();
}

} // namespace ccpy::parse
