#include "./structual.h"
#include <stdexcept>
#include "../util/util.h"
using namespace std;

namespace ccpy::serialize {

struct StructualSerializer::Impl {
  IBaseSerializer &is;
  bool pretty;

  DECL_OP_PUT

  template<typename T>
  void put(const T &x) {
    this->is.put(x);
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

StructualSerializer::StructualSerializer(
  IBaseSerializer &is,
  bool pretty
) : pimpl(Impl { is, pretty }) {
  if(pretty)
    throw invalid_argument { "TODO: Not implemented" };
}

StructualSerializer::~StructualSerializer() noexcept {}

void StructualSerializer::put(const Structual &x) {
  this->pimpl->put(x);
}

} // namespace ccpy::serialize
