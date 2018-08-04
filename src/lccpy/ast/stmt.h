#ifndef __CCPY_AST_STMT__
#define __CCPY_AST_STMT__

#include <vector>
#include "./expr.h"
#include "../util/adt.h"

namespace ccpy::ast {

struct StmtPass {};

struct StmtExpr {
  Expr expr;
};

using Stmt = tagged_union<
  StmtPass,
  StmtExpr
>;

} // namespace ccpy::ast

#endif // __CCPY_AST_STMT__
