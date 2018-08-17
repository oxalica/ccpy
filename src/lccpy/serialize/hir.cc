#include "./hir.h"
#include "./number.h"
using namespace std;
using namespace ccpy::hir;

namespace ccpy::serialize {

namespace {

Str escape(const Str &s) {
  Str ret = "\"";
  for(auto c: s)
    switch(c) {
      case '"':  ret += "\\\""; break;
      case '\\': ret += "\\\\"; break;
      default:   ret += c;
    }
  return ret + "\"";
}

Str trans(const Integer &x) {
  return IntegerSerializer {}(x);
}

Str trans(size_t x) {
  return trans(Integer(x));
}

Str trans_offset(ptrdiff_t x) {
  Str s = x >= 0 ? "+" : "";
  s += trans(static_cast<size_t>(x));
  return s;
}

Str trans(const Decimal &x) {
  return DecimalSerializer {}(x);
}

Str trans(const Immediate &imm) {
  return match<Str>(imm
  , [](const ImmInteger &imm) { return trans(imm.value); }
  , [](const ImmDecimal &imm) { return trans(imm.value); }
  , [](const ImmStr &imm) { return escape(imm.value); }
  , [](const ImmIntrinsic &imm) { return "Intrinsic " + escape(imm.name); }
  );
}

Str trans(const HIR &hir) {
  return match<Str>(hir
  , [](const HIRImm &hir) {
    return "%" + trans(hir.dest) + " = " + trans(hir.imm);
  }
  , [](const HIRTuple &hir) {
    Str ret = "%" + trans(hir.dest) + " = Tuple#" + trans(hir.elems.size());
    ret += "[";
    bool fst = true;
    for(auto c: hir.elems) {
      if(fst)
        fst = false;
      else
        ret += ", ";
      ret += "%" + trans(c);
    }
    ret += "]";
    return ret;
  }
  , [](const HIRClosure &hir) {
    return "%" + trans(hir.dest) + " = Closure #" + trans(hir.closure_id);
  }
  , [](const HIRCall &hir) {
    return "%" + trans(hir.dest) + " = Call %" + trans(hir.fnargs);
  }
  , [](const HIRCondJmp &hir) {
    return "JmpIf %" + trans(hir.cond)
      + " <" + trans_offset(hir.offset) + ">";
  }
  , [](const HIRReturn &hir) {
    return "Return %" + trans(hir.source);
  }
  , [](const HIRRaise &hir) {
    return "Raise %" + trans(hir.source);
  }
  , [](const HIRPushExcept &hir) {
    return "PushExcept %" + trans(hir.dest)
      + " <" + trans_offset(hir.offset) + ">";
  }
  , [](const HIRPopExcept &) {
    return "PopExcept";
  }
  );
}

} // namespace anonymous

Str HIRSerializer::operator()(const HIR &hir) const {
  return trans(hir);
}

Str HIRSerializer::operator()(const Closure &closure) const {
  Str ret;
  for(auto &c: closure.hirs) {
    ret += trans(c);
    ret += "\n";
  }
  return ret;
}

Str HIRSerializer::operator()(const Module &mod) const {
  Str ret;
  auto len = mod.closures.size();
  for(size_t i = 0; i < len; ++i) {
    auto &c = mod.closures[i];
    if(i == 0)
      ret += "Start";
    else
      ret += "Closure #" + trans(i);
    ret += "(Local size = " + trans(c.local_size) + "):\n";
    ret += (*this)(c);
    ret += "\n\n";
  }
  return ret;
}

} // namespace ccpy::serialize
