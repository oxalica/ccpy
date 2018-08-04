#include "./structual.h"
#include <sstream>
#include "../util/adt.h"
#include "../util/stream.h"
#include "../util/stream_wrap.h"
using namespace std;

namespace ccpy::serialize {

namespace {

struct Impl {
  ISink<Str> &o;

  DECL_OP_PUT

  template<typename T>
  void put(const T &x) {
    this->o.put(x);
  }

  void put(const Structual &x) {
    match(x,
      [&](const StructValue &x) {
        *this << x.value;
      },
      [&](const StructParen &x) {
        if(x.name)
          *this << *x.name;
        *this << "(";
        this->put_list(false, x.inner);
        *this << ")";
      },
      [&](const StructBracket &x) {
        if(x.name)
          *this << *x.name;
        *this << "[";
        this->put_list(false, x.inner);
        *this << "]";
      },
      [&](const StructBrace &x) {
        if(x.name)
          *this << *x.name << " ";
        *this << "{";
        this->put_list(true, x.inner);
        *this << "}";
      }
    );
  }

  void put_list(bool space, const vector<Structual> &v) {
    if(v.empty()) return;
    if(space) *this << " ";
    bool fst = true;
    for(auto &c: v) {
      if(fst) fst = false;
      else *this << ", ";
      *this << c;
    }
    if(space) *this << " ";
  }
};

} // namespace anonymouse

Str StructualSerializer::operator()(const Structual &x) const {
  ostringstream ss {};
  WrapOStream sink { ss };
  Impl { sink }.put(x);
  return ss.str();
}

} // namespace ccpy::serialize
