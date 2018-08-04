#include "./number.h"
#include <string>
using std::to_string;

namespace ccpy::serialize {

Str IntegerSerializer::operator()(const Integer &num) const {
  return to_string(num);
}

Str DecimalSerializer::operator()(const Decimal &num) const {
  return to_string(num);
}

} // namespace ccpy::serialize
