#ifndef __CCPY_HIR_HIR__
#define __CCPY_HIR_HIR__

#include <cstddef>
#include <vector>
#include "../util/adt.h"
#include "../util/macro.h"
#include "../util/memory.h"
#include "../util/types.h"

namespace ccpy::hir {

#define HIR_IMMEDIATE_LIST(F) \
  F(ImmInteger, { Integer value; }) \
  F(ImmDecimal, { Decimal value; }) \
  F(ImmStr, { Str value; }) \
  F(ImmIntrinsic, { Str name; }) \

DECL_TAGGED_UNION(Immediate, HIR_IMMEDIATE_LIST)

#define HIR_LIST(F) \
  F(HIRImm, { std::size_t dest; Immediate imm; }) \
  F(HIRTuple, { std::size_t dest; std::vector<std::size_t> elems; }) \
  F(HIRClosure, { std::size_t dest, closure_id; }) \
  F(HIRCall, { std::size_t dest, fnargs; }) \
  F(HIRCondJmp, { std::size_t cond; std::ptrdiff_t offset; }) \
  F(HIRReturn, { std::size_t source; }) \
  F(HIRRaise, { std::size_t source; }) \
  F(HIRPushExcept, { std::size_t dest, offset; }) \
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
