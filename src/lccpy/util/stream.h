#ifndef __CCPY_UTIL_STREAM__
#define __CCPY_UTIL_STREAM__

#include <exception>
#include <iostream>
#include <optional>
#include <type_traits>
#include <vector>
#include "./util.h"

namespace ccpy {

template<typename T>
class ISource {
public:
  using value_type = T;

  virtual ~ISource() noexcept {}
  /// `std::nullopt` for the end of stream.
  virtual std::optional<T> get() = 0;
};

template<typename T>
class IBufSource: public ISource<T> {
public:
  /// `std::nullopt` for the end of stream.
  virtual const std::optional<T> &peek() = 0;
  virtual void putback(T &&) = 0;
  virtual void putback(const T &x) { this->putback(T(x)); }
};

template<typename T>
class ISink {
public:
  virtual ~ISink() noexcept {}
  virtual void put(const T &) = 0;
};

/// Wrap an existing `ISource` with buffer.
template<
  typename S,
  typename T = typename S::value_type,
  typename = std::enable_if_t<std::is_base_of_v<ISource<T>, S>>>
class Buffered: public IBufSource<typename S::value_type> {
public:
  using value_type = T;

public:
  Buffered(const S &_upstream): upstream(_upstream), buf() {}
  virtual ~Buffered() noexcept {}

  virtual std::optional<T> get() {
    if(this->buf.empty())
      return this->upstream->get();
    else {
      auto x { std::move(this->buf.back()) };
      this->buf.pop_back();
      return x;
    }
  }

  virtual std::optional<const T &> peek() {
    if(this->buf.empty()) {
      if(auto x = this->upstream.get())
        this->buf.push_back(move(x));
      else
        return {};
    }
    return this->buf.back();
  }

  virtual void putback(T &&x) {
    this->buf.push_back(std::move(x));
  }

private:
  S &upstream;
  std::vector<value_type> buf;
};

/// Wrap `std::istream` to `IBufSource<char>`
class WrapIStream: public IBufSource<char> {
public:
  explicit WrapIStream(std::istream &);
  virtual ~WrapIStream() noexcept;

  virtual std::optional<char> get();
  DECL_OP_GET
  virtual const std::optional<char> &peek();
  virtual void putback(char &&);

private:
  struct Impl;
  owned<Impl> pimpl;
};

/// Wrap `std::ostream` to `ISink<Str>`
class WrapOStream: public ISink<Str> {
public:
  explicit WrapOStream(std::ostream &);
  virtual ~WrapOStream() noexcept;

  virtual void put(const Str &);
  DECL_OP_PUT

private:
  struct Impl;
  owned<Impl> pimpl;
};

class StreamFailException: public std::exception {
public:
  StreamFailException(const char *) noexcept;

  virtual const char *what() const noexcept;

private:
  const char *reason;
};

using StreamFatalException = std::ios_base::failure;

} // namespace ccpy

#endif // __CCPY_UTIL_STREAM__