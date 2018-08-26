#ifndef __CCPY_HIR_NAME_SCOPE__
#define __CCPY_HIR_NAME_SCOPE__

#include <cstddef>
#include <exception>
#include <memory>
#include <utility>
#include <vector>
#include "../util/adt.h"
#include "../util/macro.h"
#include "../util/memory.h"
#include "../util/types.h"

namespace ccpy::hir {

#define NAME_KIND_LIST(F) \
  F(NameLocal, { std::size_t id; }) \
  F(NameCapture, { std::size_t id; }) \
  F(NameGlobal, {}) \

DECL_TAGGED_UNION(NameKind, NAME_KIND_LIST)

class NameScope {
public:
  NameScope();
  explicit NameScope(const std::shared_ptr<NameScope> &parent);
  ~NameScope() noexcept;
  NameScope(NameScope &&) noexcept;

  // Used in preresolving
  void mark_nonlocal(const Str &);
  void mark_global(const Str &);
  void mark_local(const Str &);

  // Used in code generating.
  NameKind get(const Str &name);

  std::size_t new_local();
  void del_local(std::size_t);

  std::vector<std::pair<Str, std::size_t>> get_locals() const;

  // Used after code (inside the scope) generating.
  const std::vector<Str> &get_captured() const;
  std::vector<Str> &&get_captured();
  std::size_t get_local_size() const; // Get the maximum size of locals.

private:
  struct Impl;
  owned<Impl> pimpl;
};

class NameResolveException: public std::exception {
public:
  NameResolveException(const char *_reason) noexcept: reason(_reason) {}
  virtual ~NameResolveException() noexcept {}

  const char *what() noexcept { return this->reason; }

private:
  const char *reason;
};

} // namespace ccpy::hir

#endif // __CCPY_HIR_NAME_SCOPE__
