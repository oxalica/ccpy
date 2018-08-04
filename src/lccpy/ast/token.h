#ifndef __CCPY_AST_TOKEN__
#define __CCPY_AST_TOKEN__

#include "../util/adt.h"
#include "../util/macro.h"
#include "../util/types.h"

namespace ccpy::ast {

struct Span {
  StrPos begin, end;
};

#define KEYWORD_LIST(F) \
  F(Del, "del") \
  F(Pass, "pass") \
  F(Break, "break") \
  F(Continue, "continue") \
  F(Return, "return") \
  F(From, "from") F(Import, "import") F(As, "as") \
  F(If, "if") F(Elif, "elif") F(Else, "else") \
  F(For, "for") \
  F(In, "in") \
  F(Def, "def")

DECL_REFL_ENUM(Keyword, KEYWORD_LIST);

#define SYMBOL_LIST(F) \
  F(Comma, ",") F(Colon, ":") F(SemiColon, ";") \
  F(Dot, ".") F(DotDotDot, "...") \
  F(LParen, "(") F(RParen, ")") \
  F(Not, "~") \
  F(Add, "+") F(Sub, "-") F(Mul, "*") F(Div, "/") F(Mod, "%")

DECL_REFL_ENUM(Symbol, SYMBOL_LIST);

struct TokKeyword { Keyword keyword; };
struct TokSymbol  { Symbol symbol; };
struct TokName    { Str name; };
struct TokInteger { Integer integer; };
struct TokIndent  {};
struct TokDedent  {};

using Token = tagged_union<
  TokKeyword,
  TokSymbol,
  TokName,
  TokInteger,
  TokIndent,
  TokDedent
>;

} // namespace ccpy::ast

#endif // __CCPY_AST_TOKEN__
