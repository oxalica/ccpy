#ifndef __CCPY_AST_EXPR__
#define __CCPY_AST_EXPR__

#include <variant>
#include <vector>
#include "../util/util.h"

namespace ccpy::ast {

struct LitInteger {
  Integer value;
};

struct LitBool {
  bool value;
};

struct LitEllipse {}; // `...`

using Literal = std::variant<
  LitInteger,
  LitBool,
  LitEllipse
>;

#define UNARY_OP_LIST(F) \
  F(Pos, "+") F(Neg, "-") F(Not, "~")
DECL_REFL_ENUM(UnaryOp, UNARY_OP_LIST);

#define BINARY_OP_LIST(F) \
  F(Add, "+") F(Sub, "-") F(Mul, "*") F(Div, "/") F(Mod, "%")
DECL_REFL_ENUM(BinaryOp, BINARY_OP_LIST);

struct ExprName;
struct ExprLiteral;
struct ExprMember;
struct ExprCall;
struct ExprTuple;
struct ExprUnary;
struct ExprBinary;

using Expr = std::variant<
  ExprName,
  ExprLiteral,
  ExprMember,
  ExprCall,
  ExprTuple,
  ExprUnary,
  ExprBinary
>;

struct ExprName {
  Str name;
};

struct ExprLiteral {
  Literal lit;
};

struct ExprMember {
  owned<Expr> obj;
  Str member;
};

struct ExprCall {
  owned<Expr> func;
  std::vector<Expr> args;
};

struct ExprTuple {
  std::vector<Expr> elems;
};

struct ExprUnary {
  UnaryOp op;
  owned<Expr> expr;
};

struct ExprBinary {
  BinaryOp op;
  owned<Expr> lexpr, rexpr;
};

} // namespace ccpy::ast

#endif // __CCPY_AST_EXPR__
