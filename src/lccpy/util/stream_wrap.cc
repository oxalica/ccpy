#include "./stream_wrap.h"
#include <cstdlib>
#include <iostream>
#include "./stream.h"
#include "../util/types.h"
using namespace std;

namespace ccpy {

struct WrapIStream::Impl {
  istream &s;
  char buf;

  void trans_exception() {
    this->s.exceptions(ios_base::badbit);
  }

  optional<char> get() {
    char c;
    if(this->s.get(c))
      return c;
    this->trans_exception();
    return {};
  }

  optional<const char &> peek() {
    int c = this->s.peek();
    this->trans_exception();
    if(c == istream::traits_type::eof())
      return {};
    this->buf = static_cast<char>(c);
    return this->buf;
  }

  void putback(char c) {
    this->s.putback(c);
    if(this->s.eof())
      this->s.clear();
    this->trans_exception();
  }
};

WrapIStream::WrapIStream(istream &s)
  : pimpl(Impl { s, {} }) {}

WrapIStream::~WrapIStream() noexcept {}

optional<char> WrapIStream::get() {
  return this->pimpl->get();
}

optional<const char &> WrapIStream::peek() {
  return this->pimpl->peek();
}

void WrapIStream::putback(char &&c) {
  return this->pimpl->putback(c);
}

struct WrapOStream::Impl {
  ostream &s;
};

WrapOStream::WrapOStream(ostream &s)
  : pimpl(Impl { s }) {}

WrapOStream::~WrapOStream() noexcept {}

void WrapOStream::put(const Str &c) {
  this->pimpl->s.write(c.data(), c.length());
}

StreamFailException::StreamFailException(const char *_reason) noexcept
  : reason(_reason) {}

const char *StreamFailException::what() const noexcept {
  return this->reason;
}

} // namespace ccpy
