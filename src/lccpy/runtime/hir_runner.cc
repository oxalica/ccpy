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
  struct Context {
    size_t closure_id;
    size_t ip;

    shared_ptr<ObjectPool> locals, captured;
    ObjectRef args;
  };

  struct FrameInfo {
    bool is_except;
    LocalIdx dest;
  };

  struct Frame {
    FrameInfo info;
    Context context;
  };

  const Module &mod;
  vector<Frame> frames;
  Context context;
  IntrinsicMod intrinsic;

  Impl(const Module &_mod, std::istream &in, std::ostream &out)
    : mod(_mod)
    , intrinsic({ in, out }) {
    if(mod.closures.empty())
      throw HIRRuntimeException { "Module should have an entry" };
  }

  const ObjectPlace local(LocalIdx id) {
    if(id >= 0) {
      auto &locals = *this->context.locals;
      if(size_t(id) >= locals.size())
        throw HIRRuntimeException { "Local access out of range" };
      return locals[id];
    } else {
      id = -id;
      auto &captured = *this->context.captured;
      if(size_t(id) >= captured.size())
        throw HIRRuntimeException { "Captured access out of range" };
      return captured[id];
    }
  }

  const HIR &hir(size_t ip) {
    auto &hirs = this->mod.closures[this->context.closure_id].hirs; // Checked
    if(ip >= hirs.size())
      throw HIRRuntimeException { "Invalid target ptr" };
    return hirs[ip];
  }

  auto make_obj_pool(size_t local_size) {
    ObjectPool new_locals { local_size };
    for(auto &c: new_locals)
      c = make_shared<ObjectRef>(new_obj(ObjNull {}));
    return make_shared<ObjectPool>(move(new_locals));
  }

  void push_except_frame(LocalIdx dest, size_t target) {
    this->frames.push_back(Frame {
      FrameInfo { true, dest },
      this->context,
    });
    this->frames.back().context.ip = target - 1;
    // `this->context` is not changed
  }

  void push_return_frame(
    LocalIdx dest,
    size_t closure_id,
    size_t local_size,
    shared_ptr<ObjectPool> captured,
    ObjectRef &&args
  ) {
    this->frames.push_back(Frame {
      FrameInfo { false, dest },
      move(this->context),
    });
    this->context = Context {
      closure_id,
      size_t(-1), // `ip` will be increased after this instruction

      this->make_obj_pool(local_size),
      captured,
      move(args),
    };
  }

  optional<FrameInfo> pop_frame() {
    if(this->frames.empty())
      return {};
    auto &frame = this->frames.back();
    auto ret = frame.info;
    this->context = move(frame.context);
    this->frames.pop_back();
    return ret;
  }

  void run() {
    this->context = Context {
      0, // `closure_id` checked in constructor
      0, // `ip` at beginning

      this->make_obj_pool(this->mod.closures[0].local_size),
      this->make_obj_pool(0),
      new_obj(ObjTuple { {} }),
    };
    for(; !this->is_ended(); ++this->context.ip)
      match(this->hir(this->context.ip), [&](const auto &hir) {
        this->exec(hir);
      });
  }

  bool is_ended() {
    return this->frames.empty() && // Root level
      this->context.closure_id == 0 && // Running root
      this->mod.closures[0].hirs.size() == this->context.ip; // At the end
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
    *this->local(hir.dest) = new_obj(ObjClosure {
      hir.closure_id,
      make_shared<ObjectPool>(move(captured)),
    });
  }

  void exec(const HIRIntrinsicCall &hir) {
    switch(static_cast<Intrinsic>(hir.intrinsic_id)) {
      case Intrinsic::v_args0:
        *this->local(hir.dest) = this->context.args;
        break;

      case Intrinsic::v_call_: {
        if(hir.args.empty())
          throw HIRRuntimeException { "v_call_ have no `func` argument" };

        auto fn = *this->local(hir.args[0]);
        match(fn->primitive
        , [&](const ObjClosure &fn) {
          ObjectTuple args;
          for(size_t i = 1; i < hir.args.size(); ++i)
            args.push_back(*this->local(hir.args[i]));

          this->push_return_frame(
            hir.dest,
            fn.closure_id,
            this->mod.closures[fn.closure_id].local_size, // Checked
            fn.captured,
            new_obj(ObjTuple { move(args) })
          );
        }
        , [](const auto &) {
          throw HIRRuntimeException { "v_call_ requires a function" };
        }
        );
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

  void exec(const HIRJF &hir) {
    if(this->local(hir.target)->get() == this->intrinsic.false_.get())
      this->context.ip = hir.target - 1;
  }

  void exec(const HIRReturn &hir) {
    auto value = *this->local(hir.value);
    while(auto info = this->pop_frame())
      if(!info->is_except) { // Is `return` frame
        *this->local(info->dest) = value;
        return;
      }
    throw HIRRuntimeException { "Return outside function" };
  }

  void exec(const HIRRaise &hir) {
    auto value = *this->local(hir.value);
    while(auto info = this->pop_frame())
      if(info->is_except) { // Is `except` frame
        *this->local(info->dest) = value;
        return;
      }
    throw HIRRuntimeException { "Unhandled exception" };
  }

  void exec(const HIRPushExcept &hir) {
    this->push_except_frame(hir.dest, hir.target);
  }

  void exec(const HIRPopExcept &) {
    auto info = this->pop_frame();
    if(!info || !info->is_except)
      throw HIRRuntimeException { "Invalid PopExcept" };
  }
};

HIRRunner::HIRRunner(const Module &mod, istream &in, ostream &out)
  : pimpl(Impl { mod, in, out }) {}

HIRRunner::~HIRRunner() noexcept {}

void HIRRunner::run() {
  pimpl->run();
}

} // namespace ccpy::runtime
