#include "./early_fold.h"
#include <utility>
#include "../util/adt.h"
using namespace std;
using namespace ccpy::ast;

namespace ccpy::ast {

struct Impl {

static bool is_lit(const Expr &e) {
  return match<bool>(e
  , [](const ExprLiteral &) { return true; }
  , [](const auto &) { return false; }
  );
}

static void fold(Stmt &stmt) {
  match(stmt
  , [](StmtPass &) {}
  , [](StmtExpr &stmt) { fold(stmt.expr); }
  );
}

static void fold(vector<Expr> &exprs) {
  for(auto &e: exprs)
    fold(e);
}

static Literal fold_unary_lit(UnaryOp op, Literal &&x) {
  return match<Literal>(move(x)
  , [&](LitInteger &&x) {
    switch(op) {
      case UnaryOp::Pos: break;
      case UnaryOp::Neg: x.value = -move(x.value); break;
      case UnaryOp::Inv: x.value = ~move(x.value); break;
      default: throw EarlyFoldException { "Invalid unary op for LitInteger" };
    }
    return move(x);
  }
  , [&](LitBool &&x) {
    switch(op) {
      case UnaryOp::Inv: return LitBool { !x.value };
      default: throw EarlyFoldException { "Invalid unary op for LitBool" };
    }
  }
  , [&](LitStr &&) -> Literal {
    throw EarlyFoldException { "Invalid unary op for LitStr" };
  }
  , [&](LitNone &&) -> Literal {
    throw EarlyFoldException { "Invalid unary op for LitNone" };
  }
  , [&](LitEllipse &&) -> Literal {
    throw EarlyFoldException { "Invalid unary op for LitEllipse" };
  }
  );
}

static Literal fold_binary_lit(BinaryOp op, Literal &&a, Literal &&b) {
  return match<Literal>(move(a)
  , [&](LitInteger &&a) {
    return match<Literal>(move(b)
    , [&](LitInteger &&b) { // int .. int
      switch(op) {
        case BinaryOp::Add: a.value += b.value; break;
        case BinaryOp::Sub: a.value -= b.value; break;
        case BinaryOp::Mul: a.value *= b.value; break;
        case BinaryOp::Div: a.value /= b.value; break;
        case BinaryOp::Mod: a.value %= b.value; break;
        default:
          throw EarlyFoldException { "Invalid binary op for LitInteger" };
      }
      return a;
    }
    , [](auto &&) -> Literal {
      throw EarlyFoldException { "Invalid binary op for LitInteger" };
    }
    );
  }
  , [&](LitBool &&) {
    return match<Literal>(move(b)
    , [&](LitBool &&) -> Literal { // bool .. bool
      // TODO: Bool ops are not supported now
      throw EarlyFoldException { "Invalid binary op for LitBool" };
    }
    , [](auto &&) -> Literal {
      throw EarlyFoldException { "Invalid binary op for LitBool" };
    }
    );
  }
  , [&](LitStr &&a) {
    return match<Literal>(move(b)
    , [&](LitStr &&b) -> Literal { // str .. str
      a.value += b.value;
      return a;
    }
    , [](auto &&) -> Literal {
      throw EarlyFoldException { "Invalid binary op for LitStr" };
    }
    );
  }
  , [&](LitNone &&) -> Literal {
    throw EarlyFoldException { "Invalid binary op for LitNone" };
  }
  , [&](LitEllipse &&) -> Literal {
    throw EarlyFoldException { "Invalid binary op for LitEllipse" };
  }
  );
}

static void fold(Expr &expr) {
  optional<Expr> new_expr {};
  match(expr
  // No wildcard here, or may forget updating codes here
  , [](ExprName &) {}
  , [](ExprLiteral &) {}
  , [](ExprMember &expr) { fold(*expr.obj); }
  , [](ExprCall &expr) { fold(*expr.func); fold(expr.args); }
  , [](ExprTuple &expr) { fold(expr.elems); }
  , [&](ExprUnary &expr) {
    fold(*expr.expr);
    if(is_lit(*expr.expr))
      new_expr = ExprLiteral { fold_unary_lit(
        expr.op,
        get<ExprLiteral>(move(*expr.expr)).lit
      ) };
  }
  , [&](ExprBinary &expr) {
    fold(*expr.lexpr);
    fold(*expr.rexpr);
    if(is_lit(*expr.lexpr) && is_lit(*expr.rexpr))
      new_expr = ExprLiteral { fold_binary_lit(
        expr.op,
        get<ExprLiteral>(move(*expr.lexpr)).lit,
        get<ExprLiteral>(move(*expr.rexpr)).lit
      ) };
  }
  );
  if(new_expr)
    expr = move(*new_expr);
}

};

Stmt EarlyFold::operator()(Stmt &&stmt) const {
  Impl::fold(stmt);
  return move(stmt);
}

} // namespace ccpy::ast
