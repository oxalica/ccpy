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

template<typename T>
static Structual trans_opt(const optional<T> &x) {
  if(x)
    return StructParen { "Some", { trans(*x) } };
  return StructValue { "None" };
}

static auto trans_kvs(const vector<pair<Expr, Expr>> &kvs) {
  vector<Structual> ret {};
  for(auto &kv: kvs)
    ret.push_back(StructParen { {}, { trans(kv.first), trans(kv.second) } });
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
  , [](const LitStr &lit) {
    return StructParen { "LitStr", {
      StructStr { lit.value }
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

static Structual trans(RelationOp op) {
  return StructStr { RelationOpMap[static_cast<size_t>(op)] };
}

static Structual trans(const Str &s) {
  return StructStr { s };
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
  , [](const ExprIndex &expr) {
    return StructParen { "ExprIndex", {
      trans(*expr.obj),
      trans(*expr.idx),
    } };
  }
  , [](const ExprTuple &expr) {
    return StructParen { "ExprTuple", trans_all(expr.elems) };
  }
  , [](const ExprDict &expr) {
    return StructBrace { "ExprDict", trans_kvs(expr.kvs) };
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
  }, [](const ExprCond &expr) {
    return StructParen { "ExprCond", {
      trans(*expr.cond),
      trans(*expr.then_expr),
      trans(*expr.else_expr),
    } };
  }, [](const ExprRelation &expr) {
    vector<Structual> v;
    size_t idx = 0;
    for(auto &e: expr.exprs) {
      v.push_back(trans(e));
      if(idx < expr.ops.size())
        v.push_back(trans(expr.ops[idx++]));
    }
    return StructBracket { "ExprRelation", move(v) };
  }
  );
}

static Structual trans(const vector<Stmt> &stmt) {
  return StructBracket { {}, trans_all(stmt) };
}

static Structual trans(const Stmt &stmt) {
  return match<Structual>(stmt
  , [](const StmtPass &) {
    return StructValue { "StmtPass" };
  }
  , [](const StmtExpr &stmt) {
    return StructParen { "StmtExpr", { trans(stmt.expr) } };
  }
  , [](const StmtGlobal &stmt) {
    return StructParen { "StmtGlobal", trans_all(stmt.names) };
  }
  , [](const StmtNonlocal &stmt) {
    return StructParen { "StmtNonlocal", trans_all(stmt.names) };
  }
  , [](const StmtAssign &stmt) {
    return StructParen { "StmtAssign", {
      StructBracket { {}, trans_all(stmt.pats) },
      trans(stmt.expr),
    } };
  }
  , [](const StmtReturn &stmt) {
    return StructParen { "StmtReturn", { trans_opt(stmt.value) } };
  }
  , [](const StmtRaise &stmt) {
    return StructParen { "StmtRaise", { trans(stmt.value) } };
  }
  , [](const StmtYield &stmt) {
    return StructParen { "StmtYield", { trans_opt(stmt.value) } };
  }
  , [](const StmtDel &stmt) {
    return StructParen { "StmtDel", { trans(stmt.pat) } };
  }
  , [](const StmtDef &stmt) {
    return StructParen { "StmtDef", {
      StructStr { stmt.name },
      StructBracket { {}, trans_all(stmt.args) },
      trans_opt(stmt.rest_args),
      trans(stmt.body),
    } };
  }
  , [](const StmtIf &stmt) {
    return StructParen { "StmtIf", {
      trans(stmt.cond),
      trans(stmt.thens),
      trans(stmt.elses),
    } };
  }
  , [](const StmtWhile &stmt) {
    return StructBrace { "StmtWhile", {
      trans(stmt.cond),
      trans(stmt.stmts),
    } };
  }
  , [](const StmtFor &stmt) {
    return StructBrace { "StmtFor", {
      trans(stmt.pat),
      trans(stmt.iterable),
      trans(stmt.stmts),
    } };
  }
  , [](const StmtClass &stmt) {
    return StructBrace { "StmtClass", {
      trans(stmt.name),
      trans(stmt.base),
      trans(stmt.body),
    } };
  }
  , [](const StmtTry &stmt) {
    return StructBrace { "StmtTry", {
      trans(stmt.stmts),
      trans(stmt.bind),
      trans(stmt.except),
    } };
  }
  );
}

static Structual trans(const FuncArg &arg) {
  return StructParen { {}, {
    trans(arg.name),
    trans_opt(arg.default_),
  } };
}

static Structual trans(const Pat &pat) {
  return match<Structual>(pat
  , [](const PatName &pat) {
    return StructParen { "PatName", { trans(pat.name) } };
  }
  , [](const PatTuple &pat) {
    return StructBracket { "PatTuple", { trans_all(pat.pats) } };
  }
  , [](const PatAttr &pat) {
    return StructParen { "PatAttr", {
      trans(pat.expr),
      trans(pat.name),
    } };
  }
  , [](const PatIndex &pat) {
    return StructParen { "PatIndex", {
      trans(pat.expr),
      trans(pat.idx),
    } };
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
