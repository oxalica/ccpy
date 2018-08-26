#ifndef __CCPY_RUNTIME_INTRINSIC__
#define __CCPY_RUNTIME_INTRINSIC__

#include <iostream>
#include "./object.h"
#include "../util/memory.h"
#include "../util/stream.h"

namespace ccpy::runtime {

#define INTRINSIC_LIST(F) \
  F(2, v_call) \
  F(0, v_args) \
  F(0, v_defaults) \
  F(1, v_del) \
  F(0, get_global) \
  F(2, is) \
  F(1, id) \
  F(1, not) \
  F(1, repr) \
  F(3, getattr) \
  F(3, setattr) \
  F(2, delattr) \
  F(2, obj_new) \
  F(1, obj_get_base) \
  F(1, obj_get_type) \
  F(_, tuple_make) \
  F(1, tuple_len) \
  F(2, tuple_idx) \
  F(2, tuple_concat) \
  F(4, tuple_splice) \
  F(4, tuple_slice) \
  F(2, int_add) \
  F(2, int_sub) \
  F(2, int_mul) \
  F(2, int_div) \
  F(2, int_mod) \
  F(2, int_lt) \
  F(2, int_eq) \
  F(1, int_to_str) \
  F(4, str_slice) \
  F(1, str_to_ord) \
  F(1, str_to_int) \
  F(2, str_concat) \
  F(0, dict_new) \
  F(3, dict_get) \
  F(3, dict_set) \
  F(2, dict_del) \
  F(1, dict_to_tuple) \
  F(1, print) \
  F(0, flush) \
  F(0, input) \

#define INC(...) + 1
#define INTRINSIC_COUNT (0 INTRINSIC_LIST(INC))

enum class Intrinsic {
#define INTRINSIC_VAR(NARG, NAME) NAME ## NARG,
  INTRINSIC_LIST(INTRINSIC_VAR)
};

using IntrinsicFunc = ObjectRef (const ObjectRef &);

struct IntrinsicMod { // Public struct
  std::istream &in;
  std::ostream &out;

  ObjectRef global;
  ObjectRef true_, false_, none, ellipse;

  IntrinsicMod(std::istream &, std::ostream &);

#define DECL_INTRINSIC_METHOD(NARG, NAME) IntrinsicFunc NAME ## NARG;

  INTRINSIC_LIST(DECL_INTRINSIC_METHOD)
};

extern const char *IntrinsicNameMap[INTRINSIC_COUNT];
extern IntrinsicFunc const IntrinsicMod::*IntrinsicMethodMap[INTRINSIC_COUNT];

class IntrinsicException: public std::exception {
public:
  IntrinsicException(const char *_reason) noexcept: reason(_reason) {}
  virtual ~IntrinsicException() noexcept {}

  const char *what() noexcept { return this->reason; }

private:
  const char *reason;
};

} // namespace ccpy::runtime

#endif // __CCPY_RUNTIME_INTRINSIC__
