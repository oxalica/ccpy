#ifndef __CCPY_SERIALIZE_SERIALIZER__
#define __CCPY_SERIALIZE_SERIALIZER__

#include "../util/stream.h"
#include "../util/util.h"

namespace ccpy::serialize {

template<typename T>
using ISerializer = ISink<T>;

using IBaseSerializer = ISerializer<Str>;

} // namespace ccpy::print

#endif // __CCPY_SERIALIZE_SERIALIZER__
