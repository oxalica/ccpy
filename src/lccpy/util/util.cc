#include "./util.h"
#include <string>
#include <algorithm>

namespace ccpy {

Str to_str(const Integer &x) { // `x` always >= 0
  std::string s {};
  auto t = x;
  do
    s.push_back('0' + static_cast<int>(t % 10));
  while((t /= 10) != 0);
  std::reverse(std::begin(s), std::end(s));
  return s;
}

} // namespace ccpy
