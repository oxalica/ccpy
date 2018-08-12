#ifndef __CCPY_SERIALIZE_STRUCTUAL__
#define __CCPY_SERIALIZE_STRUCTUAL__

#include <vector>
#include "../util/adt.h"
#include "../util/macro.h"
#include "../util/trans.h"
#include "../util/types.h"

namespace ccpy::serialize {

#define STRUCTUAL_LIST(F) \
  F(StructValue, { Str value; }) \
  F(StructStr, { Str str; }) \
  F(StructParen, { optional<Str> name; std::vector<Structual> inner; }) \
  F(StructBracket, { optional<Str> name; std::vector<Structual> inner; }) \
  F(StructBrace, { optional<Str> name; std::vector<Structual> inner; }) \

DECL_TAGGED_UNION(Structual, STRUCTUAL_LIST)

class StructualSerializer: public ITrans<Structual, Str> {
public:
  StructualSerializer() {}
  virtual ~StructualSerializer() noexcept {}

  virtual Str operator()(const Structual &) const;
};

} // namespace ccpy::serialize

#endif // __CCPY_SERIALIZE_STRUCTUAL__
