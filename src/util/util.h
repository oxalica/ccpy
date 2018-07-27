#ifndef __CCPY_UTIL_UTIL__
#define __CCPY_UTIL_UTIL__

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>

namespace ccpy {

#define DECL_REFL_ENUM(NAME, LIST) \
  enum class NAME { \
    LIST(DECL_REFL_ENUM__VAR) \
  }; \
  extern const char *(NAME ## Map)[0 LIST(DECL_REFL_ENUM__CNT)] // No semicolon
#define DECL_REFL_ENUM__VAR(VAR, STR) VAR,
#define DECL_REFL_ENUM__CNT(VAR, STR) + 1
#define IMPL_REFL_ENUM(NAME, LIST) \
  const char *(NAME ## Map)[0 LIST(DECL_REFL_ENUM__CNT)] = { \
    LIST(IMPL_REFL_ENUM__STR) \
  } // No semicolon
#define IMPL_REFL_ENUM__STR(VAR, STR) STR,

// Shortcut for declare `operator<<` for `put` and `operator>>` for `get`.
#define DECL_OP_GET \
  template<typename _OP_GET> \
  auto &operator>>(const _OP_GET &x) { x = this->get(); return *this; }
#define DECL_OP_PUT \
  template<typename _OP_PUT> \
  auto &operator<<(_OP_PUT x) { this->put(x); return *this; }

using SourcePos = const char *;
using Str = std::string;

using Integer = std::int64_t;
using Decimal = double;

template<typename T>
class owned {
public:
  using value_type = T;

  template<typename ...Args>
  explicit owned(Args &&...args)
    : ptr(std::make_unique<T>(std::forward<Args>(args)...)) {}
  owned(const owned &) = delete;
  owned(owned &&) noexcept = default;
  ~owned() noexcept = default;

  const owned &operator=(const owned &) = delete;
  T &operator*() const noexcept { return *ptr.get(); }
  T *operator->() const noexcept { return ptr.get(); }

private:
  std::unique_ptr<T> ptr;
};

template<typename Variant, typename ...Fs>
void match(Variant &&v, Fs ...fs) {
  struct overloaded: public Fs... {
    using Fs::operator()...;
  };

  std::visit(overloaded { fs... }, v);
}

Str to_str(const Integer &x);

} // namespace ccpy

#endif // __CCPY_UTIL_UTIL__
