#ifndef __CCPY_AST_EXPR__
#define __CCPY_AST_EXPR__

#include <utility>
#include <vector>
#include "../util/adt.h"
#include "../util/macro.h"
#include "../util/memory.h"
#include "../util/types.h"

namespace ccpy::ast {

#define LITERAL_LIST(F) \
  F(LitInteger, { Integer value; }) \
  F(LitBool, { bool value; }) \
  F(LitStr, { Str value; }) \
  F(LitEllipse, {}) \
  F(LitNone, {}) \

DECL_TAGGED_UNION(Literal, LITERAL_LIST)

#define UNARY_OP_LIST(F) \
  F(Pos, "pos") F(Neg, "neg") F(Inv, "inv") \
  F(LogNot, "not") \

DECL_REFL_ENUM(UnaryOp, UNARY_OP_LIST)

#define BINARY_OP_LIST(F) \
  F(Add, "add") F(Sub, "sub") F(Mul, "mul") \
  F(FloorDiv, "floordiv") F(Mod, "mod") \
  F(LogAnd, "and") F(LogOr, "or") \

DECL_REFL_ENUM(BinaryOp, BINARY_OP_LIST)

#define RELATION_OP_LIST(F) \
  F(Lt, "lt") F(Gt, "gt") \
  F(Le, "le") F(Ge, "ge") \
  F(Eq, "eq") F(Ne, "ne") \
  F(Is, "is") F(Ns, "isnot")

DECL_REFL_ENUM(RelationOp, RELATION_OP_LIST)

#define EXPR_LIST(F) \
  F(ExprName, { Str name; }) \
  F(ExprLiteral, { Literal lit; }) \
  F(ExprMember, { owned<Expr> obj; Str member; }) \
  F(ExprCall, { owned<Expr> func; std::vector<Expr> args; }) \
  F(ExprIndex, { owned<Expr> obj; owned<Expr> idx; }) \
  F(ExprTuple, { std::vector<Expr> elems; }) \
  F(ExprDict, { std::vector<std::pair<Expr, Expr>> kvs; }) \
  F(ExprUnary, { UnaryOp op; owned<Expr> expr; }) \
  F(ExprBinary, { BinaryOp op; owned<Expr> lexpr, rexpr; }) \
  F(ExprCond, { owned<Expr> cond, then_expr, else_expr; }) \
  F(ExprRelation, { std::vector<Expr> exprs; std::vector<RelationOp> ops; }) \

DECL_TAGGED_UNION(Expr, EXPR_LIST)

} // namespace ccpy::ast

#endif // __CCPY_AST_EXPR__
