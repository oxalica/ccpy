#include "./runtime.h"
#include "../util/adt.h"
#include "./escape.h"
#include "./number.h"
using namespace std;
using namespace ccpy::runtime;

namespace ccpy::serialize {

namespace {

struct Impl {
  static Str trans(const ObjectRef &obj) {
    if(obj->attrs.empty())
      return trans(obj->primitive, size_t(obj.get()));
    else
      return trans(obj->primitive, size_t(obj.get())) + trans(obj->attrs);
  }

  static Str trans(const PrimitiveObj &obj, size_t ptr) {
    return match<Str>(obj
    , [](const ObjBool &obj) {
      return obj.value ? "True" : "False";
    }
    , [](const ObjInt &obj) {
      return IntegerSerializer {}(obj.value);
    }
    , [](const ObjStr &obj) {
      return StringEscape {}(obj.value);
    }
    , [](const ObjTuple &obj) {
      Str ret = "(";
      bool fst = true;
      for(auto &c: obj.elems) {
        if(fst)
          fst = false;
        else
          ret += ", ";
        ret += trans(c);
      }
      if(obj.elems.size() == 1)
        ret += ",";
      return ret + ")";
    }
    , [](const ObjDict &obj) {
      return trans(obj.value);
    }
    , [&](const ObjClosure &obj) {
      auto ser = IntegerSerializer {};
      return "<Closure#" + ser(obj.closure_id) +
        " captured " + ser(obj.captured->size()) +
        " @" + ser(Integer(ptr)) + ">";
    }
    , [&](const ObjObject &) {
      return "<Object @" + IntegerSerializer {}(Integer(ptr)) + ">";
    }
    , [&](const ObjNull &) {
      return "<Null>";
    }
    );
  }

  static Str trans(const Dict &dict) {
    Str ret = "{";
    bool fst = true;
    for(auto &c: dict) {
      if(fst)
        fst = false;
      else
        ret += ", ";
      ret += StringEscape {}(c.first) + ": " + trans(c.second);
    }
    return ret + "}";
  }
};

} // namespace anonymous

Str ObjectSerializer::operator()(const ObjectRef &obj) const {
  return Impl::trans(obj);
}

} // namespace ccpy::serialize
