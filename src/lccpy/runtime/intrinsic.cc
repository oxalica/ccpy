#include "./intrinsic.h"
#include <algorithm>
#include <sstream>
#include "../util/adt.h"
#include "../serialize/number.h"
using namespace std;

namespace ccpy::runtime {

namespace {

using Obj = ObjectRef;
using CObj = const Obj &;

Obj new_obj(PrimitiveObj &&o) {
  return make_shared<Object>(Object { move(o), {} });
}

template<typename T>
T &expect(CObj x, const char *reason) {
  return match<T &>(x->primitive
  , [](T &x) -> T & { return x; }
  , [=](auto &) -> T & { throw IntrinsicException { reason }; }
  );
}

} // namespace anonymous

#define TO_STR(X) #X
#define INTRINSIC_NAME_STR(NARG, NAME) "__intrinsic__" TO_STR(NAME ## NARG),
#define INTRINSIC_METHOD(NARG, NAME) IntrinsicMod::NAME ## NARG,

const char *IntrinsicNameMap[INTRINSIC_COUNT] = {
  INTRINSIC_LIST(INTRINSIC_NAME_STR)
};

IntrinsicFunc const IntrinsicMod::*IntrinsicMethodMap[INTRINSIC_COUNT] = {
  INTRINSIC_LIST(INTRINSIC_METHOD)
};

IntrinsicMod::IntrinsicMod(std::istream &_in, std::ostream &_out)
  : in(_in)
  , out(_out)
  , global(new_obj(ObjDict { {} }))
  , true_(new_obj(ObjBool { true }))
  , false_(new_obj(ObjBool { false }))
  , none(new_obj(ObjObject {}))
  , ellipse(new_obj(ObjObject {}))
  {}

#define SIG(NAME) ObjectRef IntrinsicMod::NAME(const ObjectRef &_args)
#define ARGS(NAME, N) \
  auto &args = expect<ObjTuple>(_args, "Invalid args for " #NAME).elems; \
  if(args.size() != N) \
    throw IntrinsicException { "Invalid args length for" #NAME };

SIG(v_call_) {
  (void)_args; // Suppress warning
  throw IntrinsicException { "Call to virtual intrinsic v_call_" };
}

SIG(v_args0) { ARGS(v_args0, 0)
  throw IntrinsicException { "Call to virtual intrinsic v_args0" };
}

SIG(v_del1) { ARGS(v_del1, 1)
  throw IntrinsicException { "Call to virtual intrinsic v_del1" };
}

SIG(get_global0) { ARGS(get_global0, 0)
  return this->global;
}

SIG(is2) { ARGS(is2, 2)
  return args[0].get() == args[1].get() ? this->true_ : this->false_;
}

SIG(id1) { ARGS(id1, 1)
  return new_obj(ObjInt { Integer(args[0].get()) });
}

SIG(getattr3) { ARGS(getattr3, 3)
  auto &name = expect<ObjStr>(args[1], "Wrong name type for getattr3").value;
  auto it = args[0]->attrs.find(name);
  return it == args[0]->attrs.end() ? args[2] : it->second;
}

SIG(setattr3) { ARGS(setattr3, 3)
  auto &name = expect<ObjStr>(args[1], "Wrong name type for setattr3").value;
  args[0]->attrs.insert_or_assign(name, args[2]);
  return this->none;
}

SIG(delattr2) { ARGS(delattr2, 2)
  auto &name = expect<ObjStr>(args[1], "Wrong name type for delattr3").value;
  args[0]->attrs.erase(name);
  return this->none;
}

SIG(tuple_make_) {
  return _args;
}

SIG(tuple_len1) { ARGS(tuple_len1, 1)
  auto &tup =
    expect<ObjTuple>(args[0], "Wrong tuple type for tuple_len1").elems;
  return new_obj(ObjInt { Integer(tup.size()) });
}

SIG(tuple_idx2) { ARGS(tuple_idx2, 2)
  auto &tup =
    expect<ObjTuple>(args[0], "Wrong tuple type for tuple_idx2").elems;
  auto &idx = expect<ObjInt>(args[1], "Wrong idx type for tuple_idx2").value;
  if(idx >= Integer(tup.size()))
    throw IntrinsicException { "tuple_idx2 out of range" };
  return tup[size_t(idx)];
}

SIG(tuple_concat2) { ARGS(tuple_concat2, 2)
  auto &a =
    expect<ObjTuple>(args[0], "Wrong first type for tuple_concat2").elems;
  auto &b =
    expect<ObjTuple>(args[1], "Wrong second type for tuple_concat2").elems;
  vector<Obj> v;
  v.insert(end(v), begin(a), end(a));
  v.insert(end(v), begin(b), end(b));
  return new_obj(ObjTuple { move(v) });
}

SIG(tuple_splice4) { ARGS(tuple_splice4, 4)
  auto &tup =
    expect<ObjTuple>(args[0], "Wrong tuple type for tuple_splice4").elems;
  auto &l =
    expect<ObjInt>(args[1], "Wrong l type for tuple_splice4").value;
  auto &r =
    expect<ObjInt>(args[2], "Wrong r type for tuple_splice4").value;
  auto &new_tup =
    expect<ObjTuple>(args[3], "Wrong new tuple type for tuple_splice4").elems;
  if(!(0 <= l && l <= r && r <= Integer(tup.size())))
    throw IntrinsicException { "Wrong range for tuple_splice4" };
  size_t _l = l, _r = r;
  auto ndel = _r - _l;
  auto nadd = new_tup.size();
  auto nlast = tup.size() - _r;
  if(nadd > ndel) { // grow
    tup.resize(tup.size() - ndel + nadd);
    for(size_t i = 0; i < nlast; ++i)
      tup[tup.size() - i - 1] = move(tup[_r + nlast - i - 1]);
  } else if(nadd < ndel) { // Shrink
    for(size_t i = 0; i < nlast; ++i)
      tup[l + nadd + i] = move(tup[_r + i]);
    tup.resize(tup.size() - ndel + nadd);
  }
  copy_n(begin(new_tup), nadd, tup.begin() + _l);
  return args[0];
}

SIG(tuple_slice4) { ARGS(tuple_slice4, 4)
  auto &tup =
    expect<ObjTuple>(args[0], "Wrong string type for tuple_slice4").elems;
  auto &l = expect<ObjInt>(args[1], "Wrong l type for tuple_slice4").value;
  auto &r = expect<ObjInt>(args[2], "Wrong r type for tuple_slice4").value;
  auto &step = expect<ObjInt>(args[3], "Wrong step type for tuple_slice4").value;
  if(step == 0)
    throw IntrinsicException { "tuple_slice4 got zero step" };
  ObjectTuple ret;
  auto len = Integer(tup.size());
  if(step > 0)
    for(auto i = max(l, Integer { 0 }); i < len && i < r; i += step)
      ret.push_back(tup[i]);
  else
    for(auto i = min(l, len - 1); i >= 0 && i > r; i += step)
      ret.push_back(tup[i]);
  return new_obj(ObjTuple { move(tup) });
}

#define IMPL_INT_OP(NAME, OP, TEST) \
  SIG(NAME) { ARGS(NAME, 2) \
    auto &a = expect<ObjInt>(args[0], "Wrong first type for " #NAME).value; \
    auto &b = expect<ObjInt>(args[1], "Wrong second type for " #NAME).value; \
    TEST \
    return new_obj(ObjInt { a OP b }); \
  }

IMPL_INT_OP(int_add2, +, )
IMPL_INT_OP(int_sub2, -, )
IMPL_INT_OP(int_mul2, *, )
IMPL_INT_OP(int_div2, /, {
  if(b == 0)
    throw IntrinsicException { "int_div2 divide by zero" };
})
IMPL_INT_OP(int_mod2, %, {
  if(b == 0)
    throw IntrinsicException { "int_mod2 divide by zero" };
})

SIG(int_lt2) { ARGS(int_lt2, 2)
  auto &a = expect<ObjInt>(args[0], "Wrong first type for int_lt2").value;
  auto &b = expect<ObjInt>(args[1], "Wrong second type for int_lt2").value;
  return a < b ? this->true_ : this->false_;
}

SIG(int_eq2) { ARGS(int_eq2, 2)
  auto &a = expect<ObjInt>(args[0], "Wrong first type for int_eq2").value;
  auto &b = expect<ObjInt>(args[1], "Wrong second type for int_eq2").value;
  return a == b ? this->true_ : this->false_;
}

SIG(int_to_str1) { ARGS(int_to_str1, 1)
  auto &a = expect<ObjInt>(args[0], "Wrong int type for int_to_str1").value;
  auto s = serialize::IntegerSerializer {}(a);
  return new_obj(ObjStr { move(s) });
}

SIG(str_slice4) { ARGS(str_slice4, 4)
  auto &s = expect<ObjStr>(args[0], "Wrong string type for str_slice4").value;
  auto &l = expect<ObjInt>(args[1], "Wrong l type for str_slice4").value;
  auto &r = expect<ObjInt>(args[2], "Wrong r type for str_slice4").value;
  auto &step = expect<ObjInt>(args[3], "Wrong step type for str_slice4").value;
  if(step == 0)
    throw IntrinsicException { "str_slice4 got zero step" };
  Str ret;
  auto len = Integer(s.length());
  if(step > 0)
    for(auto i = max(l, Integer { 0 }); i < len && i < r; i += step)
      ret.push_back(s[i]);
  else
    for(auto i = min(l, len - 1); i >= 0 && i > r; i += step)
      ret.push_back(s[i]);
  return new_obj(ObjStr { move(s) });
}

SIG(str_to_ord1) { ARGS(str_to_ord1, 1)
  auto &s = expect<ObjStr>(args[0], "Wrong string type for str_to_ord1").value;
  if(s.length() != 1)
    throw IntrinsicException { "str_to_ord1 requires string of length 1" };
  return new_obj(ObjInt { Integer(size_t(s[0])) });
}

SIG(str_to_int1) { ARGS(str_to_int1, 1)
  auto &s = expect<ObjStr>(args[0], "Wrong string type for str_to_ord1").value;
  istringstream ss { s };
  Integer n;
  ss >> n;
  return new_obj(ObjInt { move(n) });
}

SIG(str_concat2) { ARGS(str_concat2, 2)
  auto &a = expect<ObjStr>(args[0], "Wrong first type for str_concat2").value;
  auto &b = expect<ObjStr>(args[1], "Wrong second type for str_concat2").value;
  return new_obj(ObjStr { a + b });
}

SIG(dict_get3) { ARGS(dict_get3, 3)
  auto &dict = expect<ObjDict>(args[0], "Wrong dict type for dict_get3").value;
  auto &key = expect<ObjStr>(args[1], "Wrong key type for dict_get3").value;
  auto it = dict.find(key);
  return it == dict.end() ? args[2] : it->second;
}

SIG(dict_set3) { ARGS(dict_set3, 3)
  auto &dict = expect<ObjDict>(args[0], "Wrong dict type for dict_get3").value;
  auto &key = expect<ObjStr>(args[1], "Wrong key type for dict_get3").value;
  dict.insert_or_assign(key, args[2]);
  return this->none;
}

SIG(dict_del2) { ARGS(dict_del2, 3)
  auto &dict = expect<ObjDict>(args[0], "Wrong dict type for dict_del2").value;
  auto &key = expect<ObjStr>(args[1], "Wrong key type for dict_del2").value;
  dict.erase(key);
  return this->none;
}

SIG(dict_to_tuple1) { ARGS(dict_to_tuple1, 1)
  auto &dict = expect<ObjDict>(args[0], "Wrong dict type for dict_get3").value;
  vector<Obj> v;
  for(auto &c: dict) {
    auto name = new_obj(ObjStr { c.first });
    v.push_back(new_obj(ObjTuple { vector<Obj> { name, c.second } }));
  }
  return new_obj(ObjTuple { move(v) });
}

SIG(print1) { ARGS(print1, 1)
  auto &str = expect<ObjStr>(args[0], "Wrong string type for print1").value;
  if(!(this->out << str))
    throw IntrinsicException { "Stream failed in print1" };
  return this->none;
}

SIG(flush0) { ARGS(flush0, 0)
  if(!this->out.flush())
    throw IntrinsicException { "Stream failed in flush0" };
  return this->none;
}

SIG(input0) { ARGS(input0, 0)
  Str str;
  if(!getline(this->in, str))
    throw IntrinsicException { "Stream failed in input0" };
  return new_obj(ObjStr { str });
}

} // namespace ccpy::runtime
