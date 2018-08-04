#ifndef __CCPY_SERIALIZE_INTEGER__
#define __CCPY_SERIALIZE_INTEGER__

#include "./serializer.h"
#include "../util/types.h"

namespace ccpy::serialize {

class IntegerSerializer: public ISerializer<Integer, Str> {
public:
  IntegerSerializer() {}
  virtual ~IntegerSerializer() noexcept {}

  Str operator()(const Integer &) const;
};

class DecimalSerializer: public ISerializer<Decimal, Str> {
public:
  DecimalSerializer() {}
  virtual ~DecimalSerializer() noexcept {}

  Str operator()(const Decimal &) const;
};

} // namespace serialize

#endif // __CCPY_SERIALIZE_INTEGER__
