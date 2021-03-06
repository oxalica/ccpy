#include "./name_scope.h"
#include <algorithm>
#include <vector>
#include <map>
#include "../util/adt.h"
using namespace std;

namespace ccpy::hir {

struct NameScope::Impl {
  optional<shared_ptr<NameScope>> parent;
  // Generated codes of `StmtClass` depends on the order of local names.
  // We need to use ordered map to make HIR generation deterministic.
  map<Str, NameKind> names;
  vector<Str> captured;
  vector<bool> local_alloc;

  Impl(): parent({}) {}
  Impl(const shared_ptr<NameScope> &_parent): parent(move(_parent)) {}

  size_t new_capture(const Str &name) {
    this->captured.push_back(name);
    return this->captured.size() - 1;
  }

  void mark_nonlocal(const Str &name) {
    auto it = this->names.find(name);
    if(it == this->names.end()) {
      it = this->names
        .insert({ name, NameCapture { this->new_capture(name) } })
        .first;

      auto par = this->parent ? (*this->parent)->get(name) : NameGlobal {};
      auto is_global = match<bool>(par
      , [&](const NameGlobal &) { return true; }
      , [&](const auto &) { return false; }
      );
      if(is_global)
        throw NameResolveException { "Cannot find nonlocal in ancestor scopes" };
    }
    match(it->second
    , [](const NameCapture &) {}
    , [](const NameGlobal &) {
      throw NameResolveException { "Declare global name as nonlocal" };
    }
    , [](const NameLocal &) {
      throw NameResolveException { "Use name before nonlocal" };
    }
    );
  }

  void mark_global(const Str &name) {
    auto it = this->names.insert({ name, NameGlobal {} }).first;
    match(it->second
    , [](const NameGlobal &) {}
    , [](const NameCapture &) {
      throw NameResolveException { "Declare nonlocal name as global" };
    }
    , [](const NameLocal &) {
      throw NameResolveException { "Use name before global" };
    }
    );
  }

  void mark_local(const Str &name) {
    if(this->names.find(name) == this->names.end())
      this->names.insert({ name, NameLocal { this->new_local() } });
  }

  NameKind get(const Str &name) {
    auto it = this->names.find(name);
    if(it == this->names.end()) { // Implicit capture
      auto par = this->parent ? (*this->parent)->get(name) : NameGlobal {};
      auto ret = match<NameKind>(par
      , [&](const NameGlobal &) { return NameGlobal {}; }
      , [&](const auto &) { return NameCapture { this->new_capture(name) }; }
      );
      this->names.insert({ name, ret });
      return ret;
    } else
      return it->second;
  }

  size_t new_local() {
    auto it = find(begin(this->local_alloc), end(this->local_alloc), false);
    if(it == this->local_alloc.end()) {
      this->local_alloc.push_back(true);
      return this->local_alloc.size() - 1;
    } else {
      *it = true;
      return it - this->local_alloc.begin();
    }
  }

  void del_local(size_t id) {
    this->local_alloc[id] = false;
  }

  auto get_locals() {
    vector<pair<Str, size_t>> ret {};
    for(auto &kv: this->names)
      match(kv.second
      , [&](const NameLocal &kind) { ret.emplace_back(kv.first, kind.id); }
      , [&](const auto &) {}
      );
    return ret;
  }
};

NameScope::NameScope()
  : pimpl(Impl {}) {}

NameScope::NameScope(const shared_ptr<NameScope> &parent)
  : pimpl(Impl { parent }) {}

NameScope::NameScope(NameScope &&ri) noexcept
  : pimpl(move(ri.pimpl)) {}

NameScope::~NameScope() noexcept {}

void NameScope::mark_nonlocal(const Str &name) {
  pimpl->mark_nonlocal(name);
}

void NameScope::mark_global(const Str &name) {
  pimpl->mark_global(name);
}

void NameScope::mark_local(const Str &name) {
  pimpl->mark_local(name);
}

NameKind NameScope::get(const Str &name) {
  return pimpl->get(name);
}

size_t NameScope::new_local() {
  return pimpl->new_local();
}

void NameScope::del_local(size_t id) {
  pimpl->del_local(id);
}

const vector<Str> &NameScope::get_captured() const {
  return pimpl->captured;
}

vector<Str> &&NameScope::get_captured() {
  return move(pimpl->captured);
}

size_t NameScope::get_local_size() const {
  return pimpl->local_alloc.size();
}

vector<pair<Str, size_t>> NameScope::get_locals() const {
  return pimpl->get_locals();
}

} // namespace ccpy::hir
