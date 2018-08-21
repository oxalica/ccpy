#ifndef __CCPY_SERIALIZE_RUNTIME__
#define __CCPY_SERIALIZE_RUNTIME__

#include "../util/trans.h"
#include "../util/types.h"
#include "../runtime/object.h"

namespace ccpy::serialize {

class ObjectSerializer: public ITrans<runtime::ObjectRef, Str> {
public:
  ObjectSerializer() {}
  virtual ~ObjectSerializer() noexcept {}

  Str operator()(const runtime::ObjectRef &) const;
};

} // namespace ccpy::serialize

#endif // __CCPY_SERIALIZE_RUNTIME__
