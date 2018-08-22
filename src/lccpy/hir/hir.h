#ifndef __CCPY_HIR_HIR__
#define __CCPY_HIR_HIR__

#include <cstddef>
#include <vector>
#include "../util/adt.h"
#include "../util/macro.h"
#include "../util/types.h"

namespace ccpy::hir {

#define HIR_IMMEDIATE_LIST(F) \
  F(ImmInteger, { Integer value; }) \
  F(ImmBool, { bool value; }) \
  F(ImmStr, { Str value; }) \
  F(ImmEllipse, {}) \
  F(ImmNone, {}) \

DECL_TAGGED_UNION(Immediate, HIR_IMMEDIATE_LIST)

using LocalIdx = std::ptrdiff_t;

#define HIR_LIST(F) \
  F(HIRMov, { LocalIdx dest, source; }) \
  F(HIRImm, { LocalIdx dest; Immediate imm; }) \
  F(HIRClosure, { \
    LocalIdx dest; \
    std::size_t closure_id; \
    std::vector<LocalIdx> captured; \
  }) \
  F(HIRIntrinsicCall, { \
    LocalIdx dest; \
    std::size_t intrinsic_id; \
    std::vector<LocalIdx> args; \
  }) \
  F(HIRJF, { LocalIdx cond; std::size_t target; }) /* Jump if False */ \
  F(HIRReturn, { LocalIdx value; }) \
  F(HIRRaise, { LocalIdx value; }) \
  F(HIRPushExcept, { LocalIdx dest; std::size_t target; }) \
  F(HIRPopExcept, {}) \

DECL_TAGGED_UNION(HIR, HIR_LIST)

struct Closure {
  std::size_t local_size;
  std::vector<HIR> hirs;
};

struct Module {
  // The first closure is always the entry point.
  std::vector<Closure> closures;
};

} // namespace ccpy::hir

#endif // __CCPY_HIR_HIR__
