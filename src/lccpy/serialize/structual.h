#ifndef __CCPY_SERIALIZE_STRUCTUAL__
#define __CCPY_SERIALIZE_STRUCTUAL__

#include <vector>
#include "./serializer.h"
#include "../util/adt.h"
#include "../util/types.h"

namespace ccpy::serialize {

struct StructValue;
struct StructStr;
struct StructParen;
struct StructBracket;
struct StructBrace;

using Structual = tagged_union<
  StructValue,
  StructStr,
  StructParen,
  StructBracket,
  StructBrace
>;

struct StructValue {
  Str value;
};

struct StructStr {
  Str str;
};

struct StructParen {
  optional<Str> name;
  std::vector<Structual> inner;
};

struct StructBracket {
  optional<Str> name;
  std::vector<Structual> inner;
};

struct StructBrace {
  optional<Str> name;
  std::vector<Structual> inner;
};

class StructualSerializer: public ISerializer<Structual, Str> {
public:
  StructualSerializer() {}
  virtual ~StructualSerializer() noexcept {}

  virtual Str operator()(const Structual &) const;
};

} // namespace ccpy::serialize

#endif // __CCPY_SERIALIZE_STRUCTUAL__
