#ifndef __CCPY_AST_STMT__
#define __CCPY_AST_STMT__

#include <vector>
#include "./expr.h"
#include "../util/adt.h"
#include "../util/macro.h"

namespace ccpy::ast {

#define STMT_LIST(F) \
  F(StmtPass, {}) \
  F(StmtExpr, { Expr expr; })\

DECL_TAGGED_UNION(Stmt, STMT_LIST)

} // namespace ccpy::ast

#endif // __CCPY_AST_STMT__
