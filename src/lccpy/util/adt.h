#ifndef __CCPY_UTIL_ADT__
#define __CCPY_UTIL_ADT__

#include <optional>
#include <type_traits>
#include <variant>

namespace ccpy {

template<typename ...Ts>
using tagged_union = std::variant<Ts...>;

template<typename Variant, typename ...Fs>
auto match(Variant &&v, Fs ...fs) -> decltype(auto) {
  struct overloaded: public Fs... {
    using Fs::operator()...;
  };

  return std::visit(overloaded { fs... }, std::forward<Variant>(v));
}

template<typename T>
using optional = std::optional<T>;

} // namespace ccpy

#endif // __CCPY_UTIL_ADT__
