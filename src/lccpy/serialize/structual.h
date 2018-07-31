#ifndef __CCPY_SERIALIZE_STRUCTUAL__
#define __CCPY_SERIALIZE_STRUCTUAL__

#include <optional>
#include <variant>
#include <vector>
#include "./serializer.h"

namespace ccpy::serialize {

struct StructValue;
struct StructParen;
struct StructBracket;
struct StructBrace;

using Structual = std::variant<
  StructValue,
  StructParen,
  StructBracket,
  StructBrace
>;

struct StructValue {
  Str value;
};

struct StructParen {
  std::optional<Str> name;
  std::vector<Structual> inner;
};

struct StructBracket {
  std::optional<Str> name;
  std::vector<Structual> inner;
};

struct StructBrace {
  std::optional<Str> name;
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
