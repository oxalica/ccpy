#include "./expr.h"
#include "../util/util.h"

namespace ccpy::ast {

IMPL_REFL_ENUM(UnaryOp, UNARY_OP_LIST);
IMPL_REFL_ENUM(BinaryOp, BINARY_OP_LIST);

} // namespace ccpy::ast
