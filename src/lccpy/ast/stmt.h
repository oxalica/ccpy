#ifndef __CCPY_AST_STMT__
#define __CCPY_AST_STMT__

#include <vector>
#include "./expr.h"
#include "../util/adt.h"
#include "../util/macro.h"

namespace ccpy::ast {

#define PAT_LIST(F) \
  F(PatName, { Str name; }) \
  F(PatTuple, { std::vector<Pat> pats; }) \
  F(PatAttr, { Expr expr; Str name; }) \
  F(PatIndex, { Expr expr, idx; }) \

DECL_TAGGED_UNION(Pat, PAT_LIST)

struct FuncArg {
  Str name;
  optional<Expr> default_;
};

#define STMT_LIST(F) \
  F(StmtPass, {}) \
  F(StmtGlobal, { std::vector<Str> names; }) \
  F(StmtNonlocal, { std::vector<Str> names; }) \
  F(StmtExpr, { Expr expr; }) \
  F(StmtAssign, { std::vector<Pat> pats; Expr expr; }) \
  F(StmtReturn, { optional<Expr> value; }) \
  F(StmtRaise, { Expr value; }) \
  F(StmtDel, { Pat pat; }) \
  F(StmtDef, { \
    Str name; \
    std::vector<FuncArg> args; \
    optional<Str> rest_args; \
    std::vector<Stmt> body; \
  }) \
  F(StmtIf, { Expr cond; std::vector<Stmt> thens, elses; }) \
  F(StmtClass, { \
    Str name; \
    Expr base; \
    std::vector<Stmt> body; \
  }) \

DECL_TAGGED_UNION(Stmt, STMT_LIST)

} // namespace ccpy::ast

#endif // __CCPY_AST_STMT__
