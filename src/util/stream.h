#ifndef __CCPY_UTIL_STREAM__
#define __CCPY_UTIL_STREAM__

#include <exception>
#include <iostream>
#include <type_traits>
#include <vector>
#include "./util.h"

namespace ccpy {

template<typename T>
class ISource {
public:
  using value_type = T;

  virtual ~ISource() noexcept {}
  virtual T get() = 0;
};

template<typename T>
class IBufSource: public ISource<T> {
public:
  virtual const T &peek() = 0;
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

  virtual T get() {
    if(this->buf.empty())
      return this->upstream->get();
    else {
      auto x { std::move(this->buf.back()) };
      this->buf.pop_back();
      return x;
    }
  }

  virtual const T &peek() {
    if(this->buf.empty())
      this->buf.push_back(this->upstream.get());
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

  virtual char get();
  DECL_OP_GET
  virtual const char &peek();
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

class StreamEndException: public std::exception {
public:
  virtual const char *what() const noexcept;
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
