#ifndef __CCPY_UTIL_MEMORY__
#define __CCPY_UTIL_MEMORY__

#include <memory>
#include <utility>

namespace ccpy {

template<typename T>
class owned {
public:
  using value_type = T;

  template<typename ...Args>
  explicit owned(Args &&...args)
    : ptr(std::make_unique<T>(std::forward<Args>(args)...)) {}
  owned(owned &&) noexcept = default;
  ~owned() noexcept = default;

  const owned &operator=(owned &&rhs) noexcept { // Must manually defined
    ptr = std::move(rhs.ptr);                    // because it may be recursive
    return *this;
  }
  T &operator*() const noexcept { return *ptr.get(); }
  T *operator->() const noexcept { return ptr.get(); }

private:
  std::unique_ptr<T> ptr;
};

template<typename T>
auto to_owned(T &&value) {
  using BareT = std::remove_cv_t<std::remove_reference_t<T>>;
  return owned<BareT> { std::forward<T>(value) };
}

} // namespace ccpy

#endif // __CCPY_UTIL_MEMORY__
