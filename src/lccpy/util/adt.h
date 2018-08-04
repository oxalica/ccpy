#ifndef __CCPY_UTIL_ADT__
#define __CCPY_UTIL_ADT__

#include <optional>
#include <type_traits>
#include <variant>

namespace ccpy {

namespace _impl {

template<typename T>
constexpr T quiet_declval() { return *static_cast<T *>(nullptr); }

template<>
constexpr void quiet_declval<void>() {}

template<typename R, typename ...Ts, typename U, typename ...Fs>
R match_impl(U &&u, Fs ...arms) {
  struct overloaded: public Fs... {
    using Fs::operator()...;
  };
  using CheckArgs = std::conjunction<std::is_invocable<overloaded, Ts>...>;
  static_assert(CheckArgs::value, "Missing matching cases");
  if constexpr(!CheckArgs::value)
    return quiet_declval<R>(); // suppress more errors
  else {
    using CheckRet = std::conjunction<
      std::is_convertible<std::invoke_result_t<overloaded, Ts>, R>...
    >;
    static_assert(CheckRet::value, "Return type not compatible");
    if constexpr(!CheckRet::value)
      return quiet_declval<R>(); // suppress more errors
    else {
      overloaded dispatcher { arms... };
      return std::visit([&](auto &&var) -> R {
        return dispatcher(std::forward<decltype(var)>(var));
      }, u);
    }
  }
}

} // namespace _impl

template<typename ...Ts>
using tagged_union = std::variant<Ts...>;

template<typename R = void, typename ...Ts, typename ...Fs>
R match(const tagged_union<Ts...> &u, Fs ...arms) {
  return _impl::match_impl<R, const Ts &...>(u, arms...);
}

template<typename R = void, typename ...Ts, typename ...Fs>
R match(tagged_union<Ts...> &&u, Fs ...arms) {
  return _impl::match_impl<R, Ts &&...>(std::move(u), arms...);
}

template<typename T>
using optional = std::optional<T>;

} // namespace ccpy

#endif // __CCPY_UTIL_ADT__
