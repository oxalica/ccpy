#ifndef __CCPY_UTIL_STREAM__
#define __CCPY_UTIL_STREAM__

#include <exception>
#include <type_traits>
#include <vector>
#include "./adt.h"
#include "./macro.h"
#include "./types.h"

namespace ccpy {

template<typename T>
class ISource {
public:
  using value_type = T;

  virtual ~ISource() noexcept {}
  /// `std::nullopt` for the end of stream.
  virtual optional<T> get() = 0;
};

template<typename T>
class IBufSource: public ISource<T> {
public:
  /// `std::nullopt` for the end of stream.
  virtual optional<const T &> peek() = 0;
  virtual void putback(T &&) = 0;
  virtual void putback(const T &x) { this->putback(T(x)); }
};

template<typename T>
class ISink {
public:
  virtual ~ISink() noexcept {}
  virtual void put(const T &) = 0;
};

class StreamFailException: public std::exception {
public:
  StreamFailException(const char *) noexcept;

  virtual const char *what() const noexcept;

private:
  const char *reason;
};

} // namespace ccpy

#endif // __CCPY_UTIL_STREAM__
