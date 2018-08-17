#ifndef __CCPY_SERIALIZE_HIR__
#define __CCPY_SERIALIZE_HIR__

#include "../hir/hir.h"
#include "../util/trans.h"
#include "../util/types.h"

namespace ccpy::serialize {

class HIRSerializer
  : public ITrans<hir::HIR, Str>
  , public ITrans<hir::Closure, Str>
  , public ITrans<hir::Module, Str> {
public:
  HIRSerializer() {}
  virtual ~HIRSerializer() noexcept {}

  virtual Str operator()(const hir::HIR &) const;
  virtual Str operator()(const hir::Closure &) const;
  virtual Str operator()(const hir::Module &) const;
};

} // namespace ccpy::serialize

#endif // __CCPY_SERIALIZE_HIR__
