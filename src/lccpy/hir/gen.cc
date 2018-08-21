#include "./gen.h"
#include <unordered_map>
#include <utility>
#include <vector>
#include "./name_scope.h"
#include "../ast/expr.h"
#include "../ast/stmt.h"
#include "../runtime/intrinsic.h"
#include "../util/adt.h"
#include "../util/macro.h"
using namespace std;
using namespace ccpy::ast;
using namespace ccpy::runtime;

#define SEQ0() SeqVec {} \
  .finish()
#define SEQ1(A) SeqVec {} \
  .then([&]() { return A; }) \
  .finish()
#define SEQ2(A, B) SeqVec {} \
  .then([&]() { return A; }) \
  .then([&]() { return B; }) \
  .finish()
#define SEQ3(A, B, C) SeqVec {} \
  .then([&]() { return A; }) \
  .then([&]() { return B; }) \
  .then([&]() { return C; }) \
  .finish()
#define SEQ4(A, B, C, D) SeqVec {} \
  .then([&]() { return A; }) \
  .then([&]() { return B; }) \
  .then([&]() { return C; }) \
  .then([&]() { return D; }) \
  .finish()

namespace ccpy::hir {

namespace {

struct Local {
  optional<NameScope &> scope;
  LocalIdx id;

  explicit Local(NameScope &_scope): scope(_scope), id(_scope.new_local()) {}
  explicit Local(LocalIdx _id): scope({}), id(_id) {}
  Local(const Local &) = delete;
  Local(Local &&ri): scope(ri.scope), id(ri.id) { ri.scope = {}; }
  ~Local() {
    if(this->scope)
      this->scope->del_local(this->id);
  }

  Local &operator=(const Local &) = delete;
  Local &operator=(Local &&ri) {
    this->scope = move(ri.scope);
    this->id = ri.id;
    return *this;
  }

  operator LocalIdx() const { return this->id; }
};

struct SeqVec {
  vector<Local> v;

  SeqVec() {}
  SeqVec(SeqVec &&) = default;

  template<typename F>
  SeqVec &then(F f) {
    this->v.push_back(f());
    return *this;
  }

  vector<Local> finish() {
    return move(this->v);
  }
};

struct Impl {
  Module mod;
  vector<size_t> mod_stack;
  vector<NameScope> scope_stack;

  static unordered_map<Str, Intrinsic> IntrinsicReverseMap;

  Impl() {
    if(IntrinsicReverseMap.empty()) {
      size_t idx = 0;
      for(auto &c: IntrinsicNameMap)
        IntrinsicReverseMap[c] = static_cast<Intrinsic>(idx++);
    }

    this->push_scope(true);
  }

  NameScope &scope() {
    return this->scope_stack.back();
  }

  Closure &closure() {
    return this->mod.closures[this->mod_stack.back()];
  }

  void push_scope(bool global = false) {
    this->mod_stack.push_back(this->mod.closures.size());
    this->mod.closures.push_back(Closure { 0, {} });
    if(global)
      this->scope_stack.push_back(NameScope { }); // Global space
    else
      this->scope_stack.push_back(NameScope { this->scope_stack.back() });
  }

  vector<Str> pop_scope() {
    this->closure().local_size = this->scope().get_local_size();
    auto captured = this->scope().get_captured();
    this->mod_stack.pop_back();
    this->scope_stack.pop_back();
    return captured;
  }

  Local new_local() {
    return Local { this->scope() };
  }

  size_t cur_pos() {
    return this->closure().hirs.size();
  }

  DECL_OP_PUT

  void put(HIR &&hir) {
    this->closure().hirs.push_back(std::move(hir));
  }

  Local eval(Immediate &&imm) {
    auto x = this->new_local();
    *this << HIRImm { x, move(imm) };
    return x;
  }

  Local eval_intrinsic_call(Intrinsic id, vector<Local> &&args) {
    vector<LocalIdx> idxs;
    for(auto &idx: args)
      idxs.push_back(idx);
    args.clear(); // Release

    auto x = this->new_local();
    *this << HIRIntrinsicCall { x, static_cast<size_t>(id), move(idxs) };
    return x;
  }

  Local eval_builtin_call(Str &&name, vector<Local> &&args) {
    auto fn = this->eval_intrinsic_call(Intrinsic::dict_get3, SEQ3(
      this->eval_intrinsic_call(Intrinsic::get_global0, SEQ0()),
      this->eval(ImmStr { "__builtin__" + name }),
      this->new_local()
    ));
    args.insert(args.cbegin(), move(fn));
    return this->eval_intrinsic_call(Intrinsic::v_call_, move(args));
  }

  Local eval_tuple(vector<Local> &&elems) {
    return this->eval_intrinsic_call(Intrinsic::tuple_make_, move(elems));
  }

  Local eval_normal_call(Local &&fn, Local &&args) {
    return this->eval_builtin_call("call", SEQ2(
      move(fn),
      move(args)
    ));
  }

  Local eval_push_except(size_t label) {
    auto x = this->new_local();
    *this << HIRPushExcept { x, label };
    return x;
  }

  Local eval_name(const Str &name) {
    return match<Local>(this->scope().get(name)
    , [&](const NameLocal &kind) {
      return Local { LocalIdx(kind.id) };
    }
    , [&](const NameGlobal &) {
      return this->eval_builtin_call("global_get", SEQ1(
        this->eval(ImmStr { name })
      ));
    }
    , [&](const NameCapture &kind) {
      return Local { -1 - LocalIdx(kind.id) };
    }
    );
  }

  void put(const vector<Stmt> &stmts) {
    for(auto &c: stmts)
      *this << c;
  }

  void put(const Stmt &stmt) {
    match(stmt
    , [](const StmtPass &) {}
    , [&](const StmtExpr &stmt) { this->eval(stmt.expr); }
    );
  }

  Local eval(const Expr &expr) {
    return match<Local>(expr, [&](const auto &expr) -> Local {
      return this->eval(expr);
    });
  }

  Local eval(const ExprName &expr) {
    return this->eval_name(expr.name);
  }

  Local eval(const ExprLiteral &expr) {
    return match<Local>(expr.lit
    , [&](const LitInteger &lit) {
      return this->eval(ImmInteger { lit.value });
    }
    , [&](const LitBool &lit) {
      return this->eval(ImmBool { lit.value });
    }
    , [&](const LitStr &lit) {
      return this->eval(ImmStr { lit.value });
    }
    , [&](const LitEllipse &) {
      return this->eval(ImmEllipse {});
    }
    , [&](const LitNone &) {
      return this->eval(ImmNone {});
    }
    );
  }

  Local eval(const ExprMember &expr) {
    return this->eval_builtin_call("getattr", SEQ2(
      this->eval(*expr.obj),
      this->eval(ImmStr { expr.member })
    ));
  }

  Local eval(const ExprCall &expr) {
    // Hack intrinsic functions
    optional<Intrinsic> intrinsic_id = {};
    match(*expr.func
    , [&](const ExprName &func) {
      auto it = IntrinsicReverseMap.find(func.name);
      if(it != IntrinsicReverseMap.end())
        intrinsic_id = it->second;
    }
    , [](const auto &) {}
    );

    if(intrinsic_id) {
      vector<Local> args;
      for(auto &arg: expr.args)
        args.push_back(this->eval(arg));
      return this->eval_intrinsic_call(*intrinsic_id, move(args));
    } else {
      auto func = this->eval(*expr.func);
      vector<Local> args;
      for(auto &arg: expr.args)
        args.push_back(this->eval(arg));
      auto tup_args = this->eval_tuple(move(args));
      return this->eval_normal_call(move(func), move(tup_args));
    }
  }

  Local eval(const ExprTuple &expr) {
    vector<Local> elems;
    for(auto &elem: expr.elems)
      elems.push_back(this->eval(elem));
    return this->eval_tuple(move(elems));
  }

  Local eval(const ExprUnary &expr) {
    Str op_str { UnaryOpMap[static_cast<size_t>(expr.op)] };
    return this->eval_builtin_call(move(op_str), SEQ1(
      this->eval(*expr.expr)
    ));
  }

  Local eval(const ExprBinary &expr) {
    Str op_str { BinaryOpMap[static_cast<size_t>(expr.op)] };
    return this->eval_builtin_call(move(op_str), SEQ2(
      this->eval(*expr.lexpr),
      this->eval(*expr.rexpr)
    ));
  }
};

unordered_map<Str, Intrinsic> Impl::IntrinsicReverseMap;

} // namespace anonymous

Module HIRGen::operator()(const vector<Stmt> &stmts) const {
  try {
    Impl t {};
    t << stmts;
    auto captured = t.pop_scope();
    if(!captured.empty())
      throw HIRGenException { "Impossible: Global captures" };
    return move(t.mod);
  } catch(NameResolveException e) {
    throw HIRGenException { e.what() }; // Trans & rethrow
  }
}

} // namespace ccpy::hir
