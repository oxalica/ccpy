#include "./stream.h"
#include <cstdlib>
#include <iostream>
#include "./util.h"
using namespace std;

namespace ccpy {

namespace {

void trans_exception(istream &s) {
  if(s.eof())
    throw StreamEndException {};
  s.exceptions(ios_base::failbit);
}

} // namespace anonymouse

struct WrapIStream::Impl {
  istream &s;
  char buf;

  char get() {
    char c;
    this->s.get(c);
    return c;
  }

  const char &peek() {
    int c = this->s.peek();
    trans_exception(this->s);
    return this->buf = static_cast<char>(c);
  }

  void putback(char c) {
    this->s.putback(c);
    trans_exception(this->s);
  }
};

WrapIStream::WrapIStream(istream &s)
  : pimpl(Impl { s, '\0' }) {}

WrapIStream::~WrapIStream() noexcept {}

char WrapIStream::get() {
  return this->pimpl->get();
}

const char &WrapIStream::peek() {
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

const char *StreamEndException::what() const noexcept {
  return "Stream End";
}

StreamFailException::StreamFailException(const char *_reason) noexcept
  : reason(_reason) {}

const char *StreamFailException::what() const noexcept {
  return this->reason;
}

} // namespace ccpy
