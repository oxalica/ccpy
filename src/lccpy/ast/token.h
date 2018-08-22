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
  F(Pass, "pass") \
  F(Del, "del") \
  F(Def, "def") F(Return, "return") F(Yield, "yield") \
  F(Try, "try") F(Except, "except") F(Raise, "raise") \
  F(Global, "global") F(Nonlocal, "nonlocal") \
  F(If, "if") F(Elif, "elif") F(Else, "else") \
  F(While, "while") F(For, "for") \
  F(Break, "break") F(Continue, "continue") \
  F(In, "in") \
  F(True, "True") F(False, "False") F(None, "None") \

DECL_REFL_ENUM(Keyword, KEYWORD_LIST)

#define SYMBOL_LIST(F) \
  F(Comma, ",") F(Colon, ":") F(SemiColon, ";") \
  F(Dot, ".") F(DotDotDot, "...") \
  F(LParen, "(") F(RParen, ")") \
  F(LBracket, "[") F(RBracket, "]") \
  F(Eq, "=") \
  F(Inv, "~") \
  F(Add, "+") F(Sub, "-") F(Mul, "*") F(Div, "/") F(Mod, "%")

DECL_REFL_ENUM(Symbol, SYMBOL_LIST)

#define TOKEN_LIST(F) \
  F(TokKeyword, { Keyword keyword; }) \
  F(TokSymbol, { Symbol symbol; }) \
  F(TokName, { Str name; }) \
  F(TokString, { Str str; }) \
  F(TokInteger, { Integer integer; }) \
  F(TokIndent, {}) \
  F(TokDedent, {}) \
  F(TokNewline, {}) \

DECL_TAGGED_UNION(Token, TOKEN_LIST)

} // namespace ccpy::ast

#endif // __CCPY_AST_TOKEN__
