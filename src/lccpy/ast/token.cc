#include "./token.h"
#include "../util/macro.h"

namespace ccpy::ast {

IMPL_REFL_ENUM(Keyword, KEYWORD_LIST);
IMPL_REFL_ENUM(Symbol, SYMBOL_LIST);

} // namespace ccpy::ast
