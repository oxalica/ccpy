#ifndef __CCPY_UTIL_ADT__
#define __CCPY_UTIL_ADT__

#include <optional>
#include <type_traits>
#include <utility>
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
      }, std::forward<U>(u));
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
R match(tagged_union<Ts...> &u, Fs ...arms) {
  return _impl::match_impl<R, Ts &...>(u, arms...);
}

template<typename R = void, typename ...Ts, typename ...Fs>
R match(tagged_union<Ts...> &&u, Fs ...arms) {
  return _impl::match_impl<R, Ts &&...>(std::move(u), arms...);
}

template<typename T>
class optional {
public:
  optional() noexcept: value() {}
  optional(std::nullopt_t) noexcept: optional() {}
  template<typename U = T>
  optional(U &&_value): value(std::forward<U>(_value)) {}

  optional(const optional &ri): value(ri.value) {}
  optional(optional &ri): value(ri.value) {} // F**king compiler
  optional(optional &&ri) noexcept: value(std::move(ri.value)) {}

  ~optional() noexcept {}

  optional &operator=(const optional &) = default;
  optional &operator=(optional &&) noexcept = default;

  explicit operator bool() const noexcept { return this->value.has_value(); }
  const T *operator->() const { return &*this->value; }
  T *operator->() { return &*this->value; }
  const T &operator*() const & { return *this->value; }
  T &operator*() & { return *this->value; }
  T &&operator*() && { return std::move(*this->value); }

  template<typename U>
  T value_or(U &&x) const & { return this->value.value_or(std::forward<U>(x)); }
  template<typename U>
  T value_or(U &&x) && { return this->value.value_or(std::forward<U>(x)); }

  template<typename U>
  friend bool operator==(const optional<U> &, const optional<U> &);

private:
  std::optional<T> value;
};

template<typename T>
class optional<T &> {
public:
  optional() noexcept: value(nullptr) {}
  optional(std::nullopt_t) noexcept: optional() {}
  optional(T &value): value(&value) {}

  optional(const optional &ri): value(ri.value) {}
  optional(optional &&ri) noexcept: value(ri.value) {}

  ~optional() noexcept {}

  optional &operator=(const optional &) = default;
  optional &operator=(optional &&) noexcept = default;

  explicit operator bool() const noexcept { return this->value != nullptr; }
  const T *operator->() const { return this->value; }
  T *operator->() { return this->value; }
  const T &operator*() const & { return *this->value; }
  T &operator*() & { return *this->value; }

  template<typename U>
  T value_or(U &&x) const & {
    return this->value ? *this->value : std::forward<U>(x);
  }

  template<typename U>
  friend bool operator==(const optional<U> &, const optional<U> &);

private:
  T *value;
};

template<typename T>
bool operator==(const optional<T> &a, const optional<T> &b) {
  return a.value == b.value;
}

template<typename T>
bool operator==(const optional<T> &a, const std::remove_reference_t<T> &b) {
  return a && *a == b;
}

template<typename T>
bool operator==(const T &a, const optional<T> &b) {
  return b == a;
}

template<typename T>
bool operator!=(const optional<T> &a, const optional<T> &b) {
  return !(a == b);
}

template<typename T>
bool operator!=(const optional<T> &a, const std::remove_reference_t<T> &b) {
  return !(a == b);
}

template<typename T>
bool operator!=(const T &a, const optional<T> &b) {
  return !(a == b);
}

} // namespace ccpy

#endif // __CCPY_UTIL_ADT__
