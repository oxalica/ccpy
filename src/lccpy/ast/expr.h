#ifndef __CCPY_AST_EXPR__
#define __CCPY_AST_EXPR__

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

DECL_REFL_ENUM(UnaryOp, UNARY_OP_LIST)

#define BINARY_OP_LIST(F) \
  F(Add, "add") F(Sub, "sub") F(Mul, "mul") F(Div, "div") F(Mod, "mod") \

DECL_REFL_ENUM(BinaryOp, BINARY_OP_LIST)

#define EXPR_LIST(F) \
  F(ExprName, { Str name; }) \
  F(ExprLiteral, { Literal lit; }) \
  F(ExprMember, { owned<Expr> obj; Str member; }) \
  F(ExprCall, { owned<Expr> func; std::vector<Expr> args; }) \
  F(ExprTuple, { std::vector<Expr> elems; }) \
  F(ExprUnary, { UnaryOp op; owned<Expr> expr; }) \
  F(ExprBinary, { BinaryOp op; owned<Expr> lexpr, rexpr; }) \

DECL_TAGGED_UNION(Expr, EXPR_LIST)

} // namespace ccpy::ast

#endif // __CCPY_AST_EXPR__
