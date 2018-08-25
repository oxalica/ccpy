#include "./hir.h"
#include "./escape.h"
#include "./number.h"
#include "../runtime/intrinsic.h"
using namespace std;
using namespace ccpy::hir;
using namespace ccpy::runtime;

namespace ccpy::serialize {

namespace {

Str trans(const Integer &x) { // The same as LocalIdx
  return IntegerSerializer {}(x);
}

Str trans(const Immediate &imm) {
  return match<Str>(imm
  , [](const ImmInteger &imm) { return trans(imm.value); }
  , [](const ImmBool &imm) { return imm.value ? "True" : "False"; }
  , [](const ImmStr &imm) { return StringEscape {}(imm.value); }
  , [](const ImmEllipse &) { return "Ellipse"; }
  , [](const ImmNone &) { return "None"; }
  );
}

Str trans(const vector<LocalIdx> &idxs) {
  Str ret;
  bool fst = true;
  for(auto idx: idxs) {
    if(fst)
      fst = false;
    else
      ret += ", ";
    ret += "%" + trans(idx);
  }
  return ret;
}

Str trans(const HIR &hir) {
  return match<Str>(hir
  , [](const HIRMov &hir) {
    return "%" + trans(hir.dest) + " = %" + trans(hir.source);
  }
  , [](const HIRImm &hir) {
    return "%" + trans(hir.dest) + " = " + trans(hir.imm);
  }
  , [](const HIRClosure &hir) {
    return "%" + trans(hir.dest) + " = Closure #" + trans(hir.closure_id)
      + "(" + trans(hir.captured) + ")"
      + " defaults = %" + trans(hir.defaults);
  }
  , [](const HIRIntrinsicCall &hir) {
    auto id = static_cast<size_t>(hir.intrinsic_id);
    return "%" + trans(hir.dest) + " = " +
      (id < INTRINSIC_COUNT ? IntrinsicNameMap[id] : "?") +
      "(" + trans(hir.args) + ")";
  }
  , [](const HIRJF &hir) {
    return "JmpIfFalse %" + trans(hir.cond)
      + " <" + trans(hir.target) + ">";
  }
  , [](const HIRReturn &hir) {
    return "Return %" + trans(hir.value);
  }
  , [](const HIRRaise &hir) {
    return "Raise %" + trans(hir.value);
  }
  , [](const HIRPushExcept &hir) {
    return "PushExcept %" + trans(hir.dest)
      + " <" + trans(hir.target) + ">";
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
  size_t idx = 0;
  for(auto &c: closure.hirs) {
    ret += IntegerSerializer {}(Integer(idx++)) + ": ";
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
