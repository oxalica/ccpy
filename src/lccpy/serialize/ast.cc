#include "./ast.h"
#include <vector>
#include "./number.h"
#include "../util/adt.h"
using namespace std;
using namespace ccpy::ast;

namespace ccpy::serialize {

namespace {

struct Impl {

template<typename T>
static auto trans_all(const vector<T> &v) {
  vector<Structual> ret {};
  for(auto &c: v)
    ret.push_back(trans(c));
  return ret;
}

static Structual trans(const Literal &lit) {
  return match<Structual>(lit
  , [](const LitInteger &lit) {
    return StructParen { "LitInteger", {
      StructValue { IntegerSerializer {}(lit.value) },
    } };
  }
  , [](const LitBool &lit) {
    return StructParen { "LitBool", {
      StructValue { lit.value ? "true" : "false" },
    } };
  }
  , [](const LitNone &) {
    return StructValue { "LitNone" };
  }
  , [](const LitEllipse &) {
    return StructValue { "LitEllipse" };
  }
  );
}

static Structual trans(UnaryOp op) {
  return StructStr { UnaryOpMap[static_cast<size_t>(op)] };
}

static Structual trans(BinaryOp op) {
  return StructStr { BinaryOpMap[static_cast<size_t>(op)] };
}

static Structual trans(const Expr &expr) {
  return match<Structual>(expr
  , [](const ExprName &expr) {
    return StructParen { "ExprName", { StructStr { expr.name } } };
  }
  , [](const ExprLiteral &expr) {
    return StructParen { "ExprLiteral", { trans(expr.lit) } };
  }
  , [](const ExprMember &expr) {
    return StructParen { "ExprMember", {
      trans(*expr.obj),
      StructValue { expr.member },
    } };
  }
  , [](const ExprCall &expr) {
    return StructParen { "ExprCall", {
      trans(*expr.func),
      StructBracket { {}, trans_all(expr.args) },
    } };
  }
  , [] (const ExprTuple &expr) {
    return StructParen { "ExprTuple", trans_all(expr.elems) };
  }
  , [](const ExprUnary &expr) {
    return StructParen { "ExprUnary", {
      trans(expr.op),
      trans(*expr.expr),
    } };
  }, [](const ExprBinary &expr) {
    return StructParen { "ExprBinary", {
      trans(expr.op),
      trans(*expr.lexpr),
      trans(*expr.rexpr),
    } };
  }
  );
}

static Structual trans(const Stmt &stmt) {
  return match<Structual>(stmt
  , [](const StmtPass &) {
    return StructValue { "StmtPass" };
  }
  , [](const StmtExpr &stmt) {
    return StructParen { "StmtExpr", { trans(stmt.expr) } };
  }
  );
}

};

} // namespace anonymous

Structual ASTSerializer::operator()(const Stmt &stmt) const {
  return Impl::trans(stmt);
}

Structual ASTSerializer::operator()(const Expr &expr) const {
  return Impl::trans(expr);
}

Structual ASTSerializer::operator()(const Literal &lit) const {
  return Impl::trans(lit);
}

} // namespace ccpy::serialize
