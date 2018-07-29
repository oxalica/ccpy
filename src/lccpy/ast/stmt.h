#ifndef __CCPY_AST_STMT__
#define __CCPY_AST_STMT__

#include <variant>
#include <vector>
#include "./expr.h"

namespace ccpy::ast {

struct StmtPass {};

struct StmtExpr {
  Expr expr;
};

using Stmt = std::variant<
  StmtPass,
  StmtExpr
>;

} // namespace ccpy::ast

#endif // __CCPY_AST_STMT__
