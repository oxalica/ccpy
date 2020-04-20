#ifndef __CCPY_RUNTIME_HIR_RUNNER__
#define __CCPY_RUNTIME_HIR_RUNNER__

#include <iostream>
#include <exception>
#include "./object.h"
#include "../hir/hir.h"
#include "../util/memory.h"
#include "../util/stream.h"

namespace ccpy::runtime {

class HIRRunner {
public:
  HIRRunner(std::istream &, std::ostream &);
  ~HIRRunner() noexcept;

  size_t load(hir::Module &&);
  void run(size_t mod_id);

private:
  struct Impl;
  owned<Impl> pimpl;
};

class HIRRuntimeException: public std::exception {
public:
  HIRRuntimeException(const char *_reason) noexcept: reason(_reason) {}
  virtual ~HIRRuntimeException() noexcept {}

  const char *what() const noexcept { return this->reason; }

private:
  const char *reason;
};

} // namespace ccpy::runtime

#endif // __CCPY_RUNTIME_HIR_RUNNER__
