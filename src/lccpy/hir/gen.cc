#include "./gen.h"
#include <memory>
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
  vector<shared_ptr<NameScope>> scope_stack;

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
    return *this->scope_stack.back();
  }

  Closure &closure() {
    return this->mod.closures[this->mod_stack.back()];
  }

  size_t push_scope(bool global = false) {
    auto closure_id = this->mod.closures.size();
    this->mod_stack.push_back(this->mod.closures.size());
    this->mod.closures.push_back(Closure { 0, {}, false });
    auto new_ns = global
      ? make_shared<NameScope>() // Global
      : make_shared<NameScope>(this->scope_stack.back());
    this->scope_stack.push_back(move(new_ns));
    return closure_id;
  }

  vector<Str> pop_scope() {
    this->closure().local_size = this->scope().get_local_size();
    auto captured = this->scope().get_captured();
    this->mod_stack.pop_back();
    this->scope_stack.pop_back();
    return captured;
  }

  Local get_locals() {
    auto dict = this->eval_intrinsic_call(Intrinsic::dict_new0, SEQ0());
    auto kvs = this->scope().get_locals();
    for(auto &kv: kvs) {
      this->eval_intrinsic_call(Intrinsic::dict_set3, SEQ3(
        Local { dict.id },
        this->eval(ImmStr { kv.first }),
        Local { LocalIdx(kv.second) }
      ));
    }
    return dict;
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

  void store(const Local &dest, Local &&value) {
    *this << HIRMov { dest, value };
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
      this->eval(ImmNone {})
    ));
    return this->eval_intrinsic_call(Intrinsic::v_call2, SEQ2(
      move(fn),
      this->eval_tuple(move(args))
    ));
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
    auto c = this->resolve_name(name);
    if(c)
      return move(*c);
    return this->eval_builtin_call("global_get", SEQ1(
      this->eval(ImmStr { name })
    ));
  }

  optional<Local> resolve_name(const Str &name) {
    optional<Local> ret;
    match(this->scope().get(name)
    , [&](const NameLocal &kind) {
      ret = Local { LocalIdx(kind.id) };
    }
    , [&](const NameGlobal &) { }
    , [&](const NameCapture &kind) {
      ret = Local { ~LocalIdx(kind.id) };
    }
    );
    return ret;
  }

  void set_jmp_pos(size_t idx, size_t target) {
    match(this->closure().hirs[idx]
    , [&](HIRJF &hir) { hir.target = target; }
    , [&](auto &) { throw HIRGenException { "JF locate failed" }; }
    );
  }

  template<typename T, typename F>
  Local eval_cond(Local &&cond, T f_true, F f_false) {
    auto ret = this->new_local();
    this->run_cond(
      move(cond),
      [&]() { this->store(Local { ret.id }, f_true()); },
      [&]() { this->store(Local { ret.id }, f_false()); }
    );
    return move(ret);
  }

  template<typename T, typename F>
  void run_cond(Local &&cond, T f_true, F f_false) {
    auto begin_jmp = this->cur_pos();
    *this << HIRJF { move(cond), 0 }; // Placeholder
    f_true();
    auto const_false = this->eval(ImmBool { false });
    auto mid_jmp = this->cur_pos();
    *this << HIRJF { move(const_false), 0 }; // Placeholder
    f_false();
    auto end_pos = this->cur_pos();

    this->set_jmp_pos(begin_jmp, mid_jmp + 1);
    this->set_jmp_pos(mid_jmp, end_pos);
  }

  void run_stmts(const vector<Stmt> &stmts) {
    for(auto &c: stmts)
      this->run_stmt(c);
  }

  void run_stmt(const Stmt &stmt) { // Prevent recur
    match(stmt, [&](const auto &stmt) {
      this->run(stmt);
    });
  }

  void run(const StmtPass &) {}

  void run(const StmtGlobal &) {} // Preprocessed

  void run(const StmtNonlocal &) {} // Preprocessed

  void run(const StmtExpr &stmt) {
    this->eval(stmt.expr);
  }

  void name_store(const Str &name, Local &&value) {
    match(this->scope().get(name)
    , [&](const NameGlobal &) {
      this->eval_intrinsic_call(Intrinsic::dict_set3, SEQ3(
        this->eval_intrinsic_call(Intrinsic::get_global0, SEQ0()),
        this->eval(ImmStr { name }),
        move(value)
      ));
    }
    , [&](const NameLocal &kind) {
      this->store(Local { LocalIdx(kind.id) }, move(value));
    }
    , [&](const NameCapture &kind) {
      this->store(Local { -1 - LocalIdx(kind.id) }, move(value));
    }
    );
  }

  void pat_store(const Pat &pat, Local &&value) {
    match(pat
    , [&](const PatName &pat) {
      this->name_store(pat.name, move(value));
    }
    , [&](const PatTuple &pat) {
      size_t idx = 0;
      for(auto &p: pat.pats) {
        auto c = this->eval_builtin_call("index", SEQ2(
          move(value),
          this->eval(ImmInteger { Integer(idx++) })
        ));
        this->pat_store(p, move(c));
      }
    }
    , [&](const PatAttr &pat) {
      this->eval_builtin_call("setattr", SEQ3(
        this->eval(pat.expr),
        this->eval(ImmStr { pat.name }),
        move(value)
      ));
    }
    , [&](const PatIndex &pat) {
      this->eval_builtin_call("setitem", SEQ3(
        this->eval(pat.expr),
        this->eval(pat.idx),
        move(value)
      ));
    }
    );
  }

  void pat_del(const Pat &pat) {
    match(pat
    , [&](const PatName &pat) {
      auto c = this->resolve_name(pat.name);
      if(c) // Local
        this->eval_intrinsic_call(Intrinsic::v_del1, SEQ1(
          move(*c)
        ));
      else // Global
        this->eval_intrinsic_call(Intrinsic::dict_del2, SEQ2(
          this->eval_intrinsic_call(Intrinsic::get_global0, SEQ0()),
          this->eval(ImmStr { pat.name })
        ));
    }
    , [&](const PatTuple &pat) {
      for(auto &p: pat.pats)
        this->pat_del(p);
    }
    , [&](const PatAttr &pat) {
      this->eval_builtin_call("delattr", SEQ2(
        this->eval(pat.expr),
        this->eval(ImmStr { pat.name })
      ));
    }
    , [&](const PatIndex &pat) {
      this->eval_builtin_call("delitem", SEQ2(
        this->eval(pat.expr),
        this->eval(pat.idx)
      ));
    }
    );
  }

  void run(const StmtAssign &stmt) {
    auto value = this->eval(stmt.expr);
    for(auto &pat: stmt.pats)
      this->pat_store(pat, Local { value.id }); // Borrow
  }

  void run(const StmtReturn &stmt) {
    auto value = stmt.value
      ? this->eval(*stmt.value)
      : this->eval(ImmNone {});
    *this << HIRReturn { value };
  }

  void run(const StmtRaise &stmt) {
    *this << HIRRaise { this->eval(stmt.value) };
  }

  void run(const StmtYield &stmt) {
    this->closure().is_generator = true;
    auto value = stmt.value
      ? this->eval(*stmt.value)
      : this->eval(ImmNone {});
    *this << HIRYield { move(value) };
  }

  void run(const StmtDel &stmt) {
    this->pat_del(stmt.pat);
  }

  void run(const StmtClass &stmt) {
    auto base = this->eval(stmt.base);

    size_t closure_id = this->push_scope();
    for(auto &s: stmt.body)
      this->local_preresolve(s);

    this->run_stmts(stmt.body);
    *this << HIRReturn { this->get_locals() };

    auto name_captured = this->pop_scope();
    auto idx_captured = this->capture_vars(name_captured);
    auto fn = this->new_local();
    *this << HIRClosure {
      fn,
      closure_id,
      move(idx_captured),
      {},
    };

    auto type = this->eval_intrinsic_call(Intrinsic::dict_get3, SEQ3(
      this->eval_intrinsic_call(Intrinsic::get_global0, SEQ0()),
      this->eval(ImmStr { "type" }),
      this->eval(ImmNone {})
    ));
    auto cls = this->eval_intrinsic_call(Intrinsic::obj_new3, SEQ3(
      move(base),
      move(type),
      this->eval_intrinsic_call(Intrinsic::v_call2, SEQ2(
        move(fn),
        this->eval_tuple(SEQ0())
      ))
    ));
    this->name_store(stmt.name, move(cls));
  }

  void run(const StmtDef &stmt) {
    size_t closure_id = this->push_scope();
    this->local_preresolve(stmt);

    this->load_args(stmt.args, stmt.rest_args);
    this->run_stmts(stmt.body);
    this->run(StmtReturn { {} }); // Return None at the end

    auto name_captured = this->pop_scope();

    vector<Local> defaults; // Eval defaults first
    for(auto &arg: stmt.args)
      if(arg.default_)
        defaults.push_back(this->eval(*arg.default_));
    auto tup_defaults = this->eval_tuple(move(defaults));

    auto idx_captured = this->capture_vars(name_captured);

    *this << HIRClosure {
      tup_defaults, // Reuse
      closure_id,
      move(idx_captured),
      tup_defaults,
    };

    this->pat_store(PatName { stmt.name }, move(tup_defaults));
  }

  vector<LocalIdx> capture_vars(const vector<Str> &name_captured) {
    vector<Local> local_captured;
    vector<LocalIdx> idx_captured;
    for(auto &name: name_captured) {
      auto var = this->eval_name(name);
      idx_captured.push_back(var);
      local_captured.push_back(move(var)); // Keep when collecting
    }
    return idx_captured;
  }

  void run(const StmtIf &stmt) {
    // Hack for avoiding calling `__builtin__not`
    auto is_wrapped = match<bool>(stmt.cond
    , [&](const ExprCall &expr) {
      return match<bool>(*expr.func
      , [&](const ExprName &func) { return func.name == "__intrinsic__not1"; }
      , [&](const auto &) { return false; }
      );
    }
    , [&](const auto &) { return false; }
    );

    auto cond = this->eval(stmt.cond);
    if(is_wrapped)
      this->run_cond(
        move(cond),
        [&]() { this->run_stmts(stmt.thens); }, // truth
        [&]() { this->run_stmts(stmt.elses); } // falsy
      );
    else
      this->run_cond(
        this->eval_not(move(cond)),
        [&]() { this->run_stmts(stmt.elses); }, // falsy
        [&]() { this->run_stmts(stmt.thens); } // truth
      );
  }

  void load_args(const vector<FuncArg> &args, const optional<Str> &rest_args) {
    if(args.empty()) {
      if(rest_args) {
        auto args = this->eval_intrinsic_call(Intrinsic::v_args0, SEQ0());
        this->name_store(*rest_args, move(args));
      }
      return;
    }

    auto real_args = this->eval_intrinsic_call(Intrinsic::v_args0, SEQ0());
    auto real_len = this->eval_intrinsic_call(Intrinsic::tuple_len1, SEQ1(
      Local { real_args.id } // Borrow
    ));

    auto f_get_arg = [&](size_t idx) {
      return this->eval_intrinsic_call(Intrinsic::tuple_idx2, SEQ2(
        Local { real_args.id }, // Borrow
        this->eval(ImmInteger { Integer(idx) })
      ));
    };
    size_t idx = 0, idx_default = 0;
    for(auto &arg: args) {
      if(!arg.default_)
        this->name_store(arg.name, f_get_arg(idx));
      else {
        auto has_arg = this->eval_intrinsic_call(Intrinsic::int_lt2, SEQ2(
          this->eval(ImmInteger { Integer(idx) }),
          Local { real_len.id } // Borrow
        ));
        auto val = this->eval_cond(move(has_arg), [&]() {
          return f_get_arg(idx);
        }, [&]() { // Load default
          return this->eval_intrinsic_call(Intrinsic::tuple_idx2, SEQ2(
            this->eval_intrinsic_call(Intrinsic::v_defaults0, SEQ0()),
            this->eval(ImmInteger { Integer(idx_default++) })
          ));
        });
        this->name_store(arg.name, move(val));
      }
      ++idx;
    }

    if(rest_args) {
      auto rest = this->eval_intrinsic_call(Intrinsic::tuple_slice4, SEQ4(
        move(real_args),
        this->eval(ImmInteger { Integer(args.size()) }),
        this->eval(ImmNone {}),
        this->eval(ImmNone {})
      ));
      this->name_store(*rest_args, move(rest));
    }
  }

  void local_preresolve(const StmtDef &stmt) {
    for(auto &arg: stmt.args)
      this->scope().mark_local(arg.name);
    if(stmt.rest_args)
      this->scope().mark_local(*stmt.rest_args);
    for(auto &stmt: stmt.body)
      this->local_preresolve(stmt);
  }

  void local_preresolve(const Stmt &stmt) {
    match(stmt
    , [&](const StmtGlobal &stmt) {
      for(auto &name: stmt.names)
        this->scope().mark_global(name);
    }
    , [&](const StmtNonlocal &stmt) {
      for(auto &name: stmt.names)
        this->scope().mark_nonlocal(name);
    }
    , [&](const StmtAssign &stmt) {
      for(auto &p: stmt.pats)
        this->local_preresolve(p);
    }
    , [&](const StmtDel &stmt) {
      this->local_preresolve(stmt.pat);
    }
    , [&](const StmtDef &stmt) {
      this->scope().mark_local(stmt.name);
    }
    , [&](const auto &) {}
    );
  }

  void local_preresolve(const Pat &pat) {
    match(pat
    , [&](const PatName &pat) {
      this->scope().mark_local(pat.name);
    }
    , [&](const PatTuple &pat) {
      for(auto &p: pat.pats)
        this->local_preresolve(p);
    }
    , [&](const auto &) {}
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

  Local eval(const ExprIndex &expr) {
    auto obj = this->eval(*expr.obj);
    auto idx = this->eval(*expr.idx);
    return this->eval_builtin_call("index", SEQ2(
      move(obj),
      move(idx)
    ));
  }

  Local eval(const ExprTuple &expr) {
    vector<Local> elems;
    for(auto &elem: expr.elems)
      elems.push_back(this->eval(elem));
    return this->eval_tuple(move(elems));
  }

  Local eval(const ExprDict &expr) {
    auto ret = this->eval_intrinsic_call(Intrinsic::dict_new0, SEQ0());
    for(auto &kv: expr.kvs)
      this->eval_intrinsic_call(Intrinsic::dict_set3, SEQ3(
        Local { ret.id },
        this->eval(kv.first),
        this->eval(kv.second)
      ));
    return ret;
  }

  Local eval(const ExprUnary &expr) {
    Str op_str { UnaryOpMap[static_cast<size_t>(expr.op)] };
    return this->eval_builtin_call(move(op_str), SEQ1(
      this->eval(*expr.expr)
    ));
  }

  Local eval(const ExprBinary &expr) {
    auto lvalue = this->eval(*expr.lexpr);
    if(expr.op == BinaryOp::LogAnd)
      return this->eval_cond(
        this->eval_not(Local { lvalue.id }), // Borrow
        [&]() { return move(lvalue); }, // falsy
        [&]() { return this->eval(*expr.rexpr); } // truth
      );
    else if(expr.op == BinaryOp::LogOr)
      return this->eval_cond(
        this->eval_not(Local { lvalue.id }), // Borrow
        [&]() { return this->eval(*expr.rexpr); }, // falsy
        [&]() { return move(lvalue); } // truth
      );
    else {
      Str op_str { BinaryOpMap[static_cast<size_t>(expr.op)] };
      return this->eval_builtin_call(move(op_str), SEQ2(
        move(lvalue),
        this->eval(*expr.rexpr)
      ));
    }
  }

  Local eval(const ExprCond &expr) {
    return this->eval_cond(
      this->eval_not(this->eval(*expr.cond)),
      [&]() { return this->eval(*expr.else_expr); }, // falsy
      [&]() { return this->eval(*expr.then_expr); } // truth
    );
  }

  Local eval(const ExprRelation &expr) {
    auto ret = this->eval(ImmBool { true });
    vector<size_t> jmps;

    auto last = this->eval(expr.exprs[0]);
    size_t idx = 0;
    for(auto op: expr.ops) {
      auto nxt = this->eval(expr.exprs[++idx]);
      this->store(
        ret,
        this->eval_relation(op, move(last), Local { nxt.id })
      );
      last = move(nxt);
      if(idx < expr.exprs.size() - 1) {
        jmps.push_back(this->cur_pos());
        *this << HIRJF { ret, 0 }; // Placeholder
      }
    }

    auto end_pos = this->cur_pos();
    for(auto idx: jmps)
      this->set_jmp_pos(idx, end_pos);

    return ret;
  }

  Local eval_relation(RelationOp op, Local &&l, Local &&r) {
    auto lr = SEQ2(move(l), move(r));
    if(op == RelationOp::Is)
      return this->eval_intrinsic_call(Intrinsic::is2, move(lr));
    if(op == RelationOp::Ns)
      return this->eval_intrinsic_call(Intrinsic::not1, SEQ1(
        this->eval_intrinsic_call(Intrinsic::is2, move(lr))
      ));
    auto op_str = RelationOpMap[static_cast<size_t>(op)];
    return this->eval_builtin_call(op_str, move(lr));
  }

  Local eval_not(Local &&x) {
    return this->eval_builtin_call("not", SEQ1(move(x)));
  }
};

unordered_map<Str, Intrinsic> Impl::IntrinsicReverseMap;

} // namespace anonymous

Module HIRGen::operator()(const vector<Stmt> &stmts) const {
  try {
    Impl t {};
    t.run_stmts(stmts);
    auto captured = t.pop_scope();
    if(!captured.empty())
      throw HIRGenException { "Impossible: Global captures" };
    if(t.mod.closures[0].is_generator)
      throw HIRGenException { "Cannot `yield` in global space" };
    return move(t.mod);
  } catch(NameResolveException e) {
    throw HIRGenException { e.what() }; // Trans & rethrow
  }
}

} // namespace ccpy::hir
