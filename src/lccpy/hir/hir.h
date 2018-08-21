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

#define HIR_LIST(F) \
  F(HIRImm, { std::size_t dest; Immediate imm; }) \
  F(HIRClosure, { std::size_t dest, closure_id, captured; }) \
  F(HIRTuple, { std::size_t dest; std::vector<std::size_t> elems; }) \
  F(HIRIntrinsicCall, { std::size_t dest, intrinsic_id, args; }) \
  F(HIRJF, { std::size_t cond; std::size_t target; }) /* Jump if False */ \
  F(HIRReturn, { std::size_t value; }) \
  F(HIRRaise, { std::size_t value; }) \
  F(HIRPushExcept, { std::size_t dest, target; }) \
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
