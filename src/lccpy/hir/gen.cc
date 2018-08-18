#include "./gen.h"
#include <utility>
#include <vector>
#include "./name_scope.h"
#include "../ast/expr.h"
#include "../ast/stmt.h"
#include "../util/macro.h"
using namespace std;
using namespace ccpy::ast;

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
  size_t id;

  explicit Local(NameScope &_scope): scope(_scope), id(_scope.new_local()) {}
  explicit Local(size_t _id): scope({}), id(_id) {}
  Local(const Local &) = delete;
  Local(Local &&ri): scope(ri.scope), id(ri.id) { ri.scope = {}; }
  ~Local() {
    if(this->scope)
      this->scope->del_local(this->id);
  }

  operator size_t() const { return this->id; }
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

  Impl() {
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

  Local eval_imm(Immediate &&imm) {
    auto x = this->new_local();
    *this << HIRImm { x, move(imm) };
    return x;
  }

  Local eval_tuple(vector<Local> &&elems) {
    vector<size_t> args;
    for(auto &c: elems)
      args.push_back(c);
    elems.clear();

    auto x = this->new_local();
    *this << HIRTuple { x, args };
    return x;
  }

  Local eval_call(Local &&fnargs) {
    *this << HIRCall { fnargs, fnargs }; // Reuse
    return move(fnargs);
  }

  Local eval_call(vector<Local> &&fnargs) {
    return this->eval_call(this->eval_tuple(move(fnargs)));
  }

  Local eval_push_except(size_t label) {
    auto x = this->new_local();
    *this << HIRPushExcept { x, label };
    return x;
  }

  Local eval_name(const Str &name) {
    // Hack for intrinsic names
    Str intrinsic { "__intrinsic__" };
    if(name.substr(0, intrinsic.length()) == intrinsic)
      return this->eval_imm(ImmIntrinsic { name.substr(intrinsic.length()) });

    return match<Local>(this->scope().get(name)
    , [&](const NameLocal &kind) {
      return Local { kind.id };
    }
    , [&](const NameGlobal &) {
      return this->eval_call(SEQ3(
        this->eval_imm(ImmIntrinsic { "dict_get2" }),
        this->eval_imm(ImmIntrinsic { "globals" }),
        this->eval_imm(ImmStr { name })
      ));
    }
    , [&](const NameCapture &kind) {
      return this->eval_call(SEQ3(
        this->eval_imm(ImmIntrinsic { "tuple_idx2" }),
        this->eval_imm(ImmIntrinsic { "captures" }),
        this->eval_imm(ImmInteger { Integer(kind.id) })
      ));
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
    , [&](const StmtExpr &stmt) { this->eval_expr(stmt.expr); }
    );
  }

  Local eval_expr(const Expr &expr) {
    return match<Local>(expr
    , [&](const ExprName &expr) { return this->eval_name(expr.name); }
    , [&](const ExprLiteral &expr) {
      return match<Local>(expr.lit
      , [&](const LitInteger &lit) {
        return this->eval_imm(ImmInteger { lit.value });
      }
      , [&](const LitBool &lit) {
        return this->eval_imm(ImmIntrinsic { lit.value ? "true" : "false" });
      }
      , [&](const LitEllipse &) {
        return this->eval_imm(ImmIntrinsic { "ellipse" });
      }
      , [&](const LitNone &) {
        return this->eval_imm(ImmIntrinsic { "none" });
      }
      );
    }
    , [&](const ExprMember &expr) {
      return this->eval_call(SEQ3(
        this->eval_imm(ImmIntrinsic { "getattr2" }),
        this->eval_expr(*expr.obj),
        this->eval_imm(ImmStr { expr.member })
      ));
    }
    , [&](const ExprCall &expr) {
      vector<Local> fnargs;
      fnargs.push_back(this->eval_expr(*expr.func));
      for(auto &arg: expr.args)
        fnargs.push_back(this->eval_expr(arg));
      return this->eval_call(move(fnargs));
    }
    , [&](const ExprTuple &expr) {
      vector<Local> elems;
      for(auto &elem: expr.elems)
        elems.push_back(this->eval_expr(elem));
      return this->eval_tuple(move(elems));
    }
    , [&](const ExprUnary &expr) {
      Str op_str { UnaryOpMap[static_cast<size_t>(expr.op)] };
      return this->eval_call(SEQ2(
        this->eval_name(op_str + "1"),
        this->eval_expr(*expr.expr)
      ));
    }
    , [&](const ExprBinary &expr) {
      Str op_str { BinaryOpMap[static_cast<size_t>(expr.op)] };
      return this->eval_call(SEQ3(
        this->eval_name(op_str + "2"),
        this->eval_expr(*expr.lexpr),
        this->eval_expr(*expr.rexpr)
      ));
    }
    );
  }
};

} // namespace anonymous

Module HIRGen::operator()(const vector<Stmt> &stmts) const {
  try {
    Impl t {};
    t << stmts;
    auto captured = t.pop_scope();
    // if(!captured.empty())
    //   throw HIRGenException { "Impossible: Global captures" };
    return move(t.mod);
  } catch(NameResolveException e) {
    throw HIRGenException { e.what() }; // Trans & rethrow
  }
}

} // namespace ccpy::hir
