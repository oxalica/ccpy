#ifndef __CCPY_SERIALIZE_INTEGER__
#define __CCPY_SERIALIZE_INTEGER__

#include "../util/trans.h"
#include "../util/types.h"

namespace ccpy::serialize {

class IntegerSerializer: public ITrans<Integer, Str> {
public:
  IntegerSerializer() {}
  virtual ~IntegerSerializer() noexcept {}

  Str operator()(const Integer &) const;
};

class DecimalSerializer: public ITrans<Decimal, Str> {
public:
  DecimalSerializer() {}
  virtual ~DecimalSerializer() noexcept {}

  Str operator()(const Decimal &) const;
};

} // namespace serialize

#endif // __CCPY_SERIALIZE_INTEGER__
