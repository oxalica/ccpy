#include "./hir_runner.h"
#include <algorithm>
#include <memory>
#include <vector>
#include "./intrinsic.h"
#include "./object.h"
using namespace std;
using namespace ccpy::hir;

namespace ccpy::runtime {

namespace {

ObjectRef new_obj(PrimitiveObj &&o) {
  return make_shared<Object>(Object { move(o), {} });
}

} // namespace anonymous

struct HIRRunner::Impl {
  const Module &mod;
  vector<shared_ptr<vector<Frame>>> frames_stack;
  IntrinsicMod intrinsic;

  Impl(const Module &_mod, std::istream &in, std::ostream &out)
    : mod(_mod)
    , intrinsic({ in, out }) {
    if(mod.closures.empty())
      throw HIRRuntimeException { "Module should have an entry" };
  }

  vector<Frame> &frames() {
    return *this->frames_stack.back();
  }

  Frame &context() {
    return this->frames().back();
  }

  const ObjectPlace local(LocalIdx id) {
    if(id >= 0) {
      auto &locals = *this->context().locals;
      if(size_t(id) >= locals.size())
        throw HIRRuntimeException { "Local access out of range" };
      return locals[id];
    } else {
      id = ~id;
      auto &captured = *this->context().captured;
      if(size_t(id) >= captured.size())
        throw HIRRuntimeException { "Captured access out of range" };
      return captured[id];
    }
  }

  const HIR &hir(size_t ip) {
    auto &hirs = this->mod.closures[this->context().closure_id].hirs; // Checked
    if(ip >= hirs.size())
      throw HIRRuntimeException { "Invalid ip ptr" };
    return hirs[ip];
  }

  auto make_obj_pool(size_t local_size) {
    ObjectPool new_locals { local_size };
    for(auto &c: new_locals)
      c = make_shared<ObjectRef>(new_obj(ObjNull {}));
    return make_shared<ObjectPool>(move(new_locals));
  }

  void push_except_frame(LocalIdx dest, size_t target) {
    auto frame = this->context(); // Copy
    frame.ip = target - 1;
    frame.is_except = true;
    frame.dest = dest;
    this->frames().insert(this->frames().end() - 1, move(frame));
    // `context` is not changed
  }

  void push_return_frame(
    LocalIdx dest,
    size_t closure_id,
    size_t local_size,
    shared_ptr<ObjectPool> captured,
    ObjectRef &&args,
    const ObjectRef &defaults
  ) {
    this->context().dest = dest;
    this->context().is_except = false;
    this->frames().push_back(Frame {
      closure_id,
      size_t(-1), // `ip` will be increased after this instruction
      this->make_obj_pool(local_size),
      move(captured),
      move(args),
      defaults,

      false, 0, // Unused
    });
  }

  optional<Frame &> pop_frame() {
    auto &frames = this->frames();
    if(frames.empty())
      throw HIRRuntimeException { "Impossible: pop frame when empty" };
    frames.pop_back();
    if(!frames.empty())
      return frames.back();
    if(this->frames_stack.empty())
      throw HIRRuntimeException { "Impossible: backtrace gen frame when empty" };
    this->frames_stack.pop_back();
    return this->context();
  }

  void new_frames() {
    this->frames_stack.push_back(make_shared<vector<Frame>>());
  }

  auto pop_gen_frames() {
    if(this->frames_stack.empty())
      throw HIRRuntimeException { "Impossible: pop gen frame when empty" };
    auto frames = move(this->frames_stack.back());
    this->frames_stack.pop_back();
    return frames;
  }

  void run() {
    this->new_frames();
    this->frames().push_back(Frame {
      0, // `closure_id` checked in constructor
      0, // `ip` at beginning
      this->make_obj_pool(this->mod.closures[0].local_size),
      this->make_obj_pool(0),
      new_obj(ObjTuple { {} }),
      new_obj(ObjTuple { {} }),

      false, 0, // Unused
    });
    for(; !this->is_ended(); ++this->context().ip)
      match(this->hir(this->context().ip), [&](const auto &hir) {
        this->exec(hir);
      });
  }

  bool is_ended() {
    auto &context = this->context();
    return context.closure_id == 0 && // Running root
      this->mod.closures[0].hirs.size() == context.ip; // At the end
  }

  void exec(const HIRMov &hir) {
    *this->local(hir.dest) = *this->local(hir.source);
  }

  void exec(const HIRImm &hir) {
    *this->local(hir.dest) = match<ObjectRef>(hir.imm
    , [](const ImmInteger &imm) { return new_obj(ObjInt { imm.value }); }
    , [&](const ImmBool &imm) {
      return imm.value ? this->intrinsic.true_ : this->intrinsic.false_;
    }
    , [](const ImmStr &imm) { return new_obj(ObjStr { imm.value }); }
    , [&](const ImmEllipse &) { return this->intrinsic.ellipse; }
    , [&](const ImmNone &) { return this->intrinsic.none; }
    );
  }

  void exec(const HIRClosure &hir) {
    if(hir.closure_id >= this->mod.closures.size())
      throw HIRRuntimeException { "Closure id out of range" };
    vector<ObjectPlace> captured;
    for(LocalIdx id: hir.captured)
      captured.push_back(this->local(id));
    auto defaults = *this->local(hir.defaults);
    *this->local(hir.dest) = new_obj(ObjClosure {
      hir.closure_id,
      make_shared<ObjectPool>(move(captured)),
      defaults,
    });
  }

  void exec(const HIRIntrinsicCall &hir) {
    switch(static_cast<Intrinsic>(hir.intrinsic_id)) {
      case Intrinsic::v_args0:
        *this->local(hir.dest) = this->context().args;
        break;

      case Intrinsic::v_defaults0:
        *this->local(hir.dest) = this->context().defaults;
        break;

      case Intrinsic::v_call2: {
        if(hir.args.size() != 2)
          throw HIRRuntimeException { "v_call2 should have 2 args" };

        auto &fn = match<ObjClosure &>((*this->local(hir.args[0]))->primitive
        , [&](ObjClosure &fn) -> ObjClosure & { return fn; }
        , [](auto &) -> ObjClosure & {
          throw HIRRuntimeException { "v_call_ `fn` requires a closure" };
        }
        );
        auto args = *this->local(hir.args[1]);
        match(args->primitive
        , [](const ObjTuple &) {}
        , [](const auto &) {
          throw HIRRuntimeException { "v_call_ `args` requires a tuple" };
        }
        );
        this->exec_call(hir.dest, fn, args);
        break;
      }

      case Intrinsic::v_gen_next1: {
        if(hir.args.size() != 1)
          throw HIRRuntimeException { "v_gen_next1 should have 1 args" };
        auto &gen = match<ObjGenerator &>((*this->local(hir.args[0]))->primitive
        , [&](ObjGenerator &fn) -> ObjGenerator & { return fn; }
        , [&](auto &) -> ObjGenerator & {
          throw HIRRuntimeException { "v_gen_next1 `gen` requires a generator" };
        }
        );
        if(gen.frames->empty())
          throw HIRRuntimeException { "v_gen_next1 on a stopped generator" };
        this->context().is_except = false;
        this->context().dest = hir.dest;
        this->frames_stack.push_back(gen.frames);
        break;
      }

      default: {
        if(hir.intrinsic_id >= INTRINSIC_COUNT)
          throw HIRRuntimeException { "Invalid intrinsic id" };

        ObjectTuple args;
        for(auto idx: hir.args)
          args.push_back(*this->local(idx));
        auto tup_args = new_obj(ObjTuple { move(args) });

        auto fn = IntrinsicMethodMap[hir.intrinsic_id];
        *this->local(hir.dest) = (this->intrinsic.*fn)(tup_args);
      }
    }
  }

  void exec_call(size_t dest, ObjClosure &fn, ObjectRef args) {
    auto &closure = this->mod.closures[fn.closure_id]; // Checked
    if(!closure.is_generator) {
      this->push_return_frame(
        dest,
        fn.closure_id,
        closure.local_size,
        fn.captured,
        move(args),
        fn.defaults
      );
    } else {
      this->new_frames();
      this->frames().push_back(Frame {
        fn.closure_id,
        size_t(-1), // Will be increased next time running it
        this->make_obj_pool(closure.local_size),
        fn.captured,
        move(args),
        fn.defaults,

        false, 0, // Unused
      });
      auto frames = this->pop_gen_frames();
      *this->local(dest) = new_obj(ObjGenerator { move(frames) });
    }
  }

  void exec(const HIRJF &hir) {
    if(this->local(hir.cond)->get() == this->intrinsic.false_.get())
      this->context().ip = hir.target - 1;
  }

  void exec(const HIRReturn &hir) {
    auto value = *this->local(hir.value);
    while(auto context = this->pop_frame())
      if(!context->is_except) { // Is `return` frame
        *this->local(context->dest) = value;
        return;
      }
    throw HIRRuntimeException { "Return outside function" };
  }

  void exec(const HIRRaise &hir) {
    auto value = *this->local(hir.value);
    while(auto context = this->pop_frame())
      if(context->is_except) { // Is `except` frame
        *this->local(context->dest) = value;
        return;
      }
    throw HIRRuntimeException { "Unhandled exception" };
  }

  void exec(const HIRYield &hir) {
    auto value = *this->local(hir.value);
    this->pop_gen_frames();
    *this->local(this->context().dest) = value;
  }

  void exec(const HIRPushExcept &hir) {
    this->push_except_frame(hir.dest, hir.target);
  }

  void exec(const HIRPopExcept &) {
    auto &frames = this->frames();
    if(frames.size() < 2)
      throw HIRRuntimeException { "No frame for PopExcept" };
    if(!frames[frames.size() - 2].is_except)
      throw HIRRuntimeException { "Invalid PopExcept" };
    frames.erase(frames.end() - 2);
  }
};

HIRRunner::HIRRunner(const Module &mod, istream &in, ostream &out)
  : pimpl(Impl { mod, in, out }) {}

HIRRunner::~HIRRunner() noexcept {}

void HIRRunner::run() {
  pimpl->run();
}

} // namespace ccpy::runtime
