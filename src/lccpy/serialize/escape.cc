#include "./escape.h"

namespace ccpy::serialize {

Str StringEscape::operator()(const Str &s) const {
  Str ret = "\"";
  for(auto c: s)
    switch(c) {
      case '"':  ret += "\\\""; break;
      case '\\': ret += "\\\\"; break;
      default:   ret += c;
    }
  return ret + "\"";
}

} // namespace ccpy::serialize
