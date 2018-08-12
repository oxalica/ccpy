#ifndef __CCPY_UTIL_TRANSFORMER__
#define __CCPY_UTIL_TRANSFORMER__

namespace ccpy {

/// Transformer transforming T to U (non-destructive).
template<typename T, typename U>
class ITrans {
public:
  virtual ~ITrans() noexcept {}

  /// Should be pure
  virtual U operator()(const T &) const = 0;
};

/// Transformer transforming T to U (destructive).
template<typename T, typename U>
class IHardTrans {
public:
  virtual ~IHardTrans() noexcept {}

  /// Should be pure
  virtual U operator()(T &&) const = 0;
};

} // namespace ccpy

#endif // __CCPY_UTIL_TRANSFORMER__
