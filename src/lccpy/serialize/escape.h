#ifndef __CCPY_SERIALIZE_ESCAPE__
#define __CCPY_SERIALIZE_ESCAPE__

#include "../util/trans.h"
#include "../util/types.h"

namespace ccpy::serialize {

class StringEscape: ITrans<Str, Str> {
public:
  StringEscape() {}
  virtual ~StringEscape() noexcept {}

  Str operator()(const Str &) const;
};

} // namespace ccpy::serialize

#endif // __CCPY_SERIALIZE_ESCAPE__
