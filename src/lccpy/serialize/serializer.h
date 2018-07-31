#ifndef __CCPY_SERIALIZE_SERIALIZER__
#define __CCPY_SERIALIZE_SERIALIZER__

#include "../util/stream.h"
#include "../util/util.h"

namespace ccpy::serialize {

/// Serializer transforming T to low-level type U.
template<typename T, typename U>
class ISerializer {
public:
  virtual ~ISerializer() noexcept {}

  /// Should be pure
  virtual U operator()(const T &) const = 0;
};

} // namespace ccpy::serialize

#endif // __CCPY_SERIALIZE_SERIALIZER__
