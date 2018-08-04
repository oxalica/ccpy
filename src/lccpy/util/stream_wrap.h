#ifndef __CCPY_UTIL_STREAM_WRAP__
#define __CCPY_UTIL_STREAM_WRAP__

#include <algorithm>
#include <iostream>
#include <vector>
#include "./stream.h"
#include "../util/adt.h"
#include "../util/memory.h"

namespace ccpy {

/// Wrap `std::istream` to `IBufSource<char>`
class WrapIStream: public IBufSource<char> {
public:
  explicit WrapIStream(std::istream &);
  virtual ~WrapIStream() noexcept;

  virtual optional<char> get();
  DECL_OP_GET
  virtual const optional<char> &peek();
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

using StreamFatalException = std::ios_base::failure;

template<typename T>
class WrapIData: public IBufSource<T> {
public:
  explicit WrapIData(const std::vector<T> &data) {
    this->tail = data;
    std::reverse(std::begin(this->tail), std::end(this->tail));
  }
  virtual ~WrapIData() noexcept {}

  virtual optional<T> get() {
    this->peek();
    auto x = move(this->head);
    this->head = {};
    return move(x);
  }
  DECL_OP_GET
  virtual const optional<T> &peek() {
    if(!this->head && !this->tail.empty()) {
      this->head = move(this->tail.back());
      this->tail.pop_back();
    }
    return this->head;
  }
  virtual void putback(T &&x) {
    this->tail.push_back(move(x));
  }

private:
  optional<T> head;
  std::vector<T> tail;
};

}

#endif // __CCPY_UTIL_STREAM_WRAP__
