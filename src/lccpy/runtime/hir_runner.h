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
  HIRRunner(const hir::Module &, std::istream &, std::ostream &);
  ~HIRRunner() noexcept;

  void run();

private:
  struct Impl;
  owned<Impl> pimpl;
};

class HIRRuntimeException: public std::exception {
public:
  HIRRuntimeException(const char *_reason) noexcept: reason(_reason) {}
  virtual ~HIRRuntimeException() noexcept {}

  const char *what() noexcept { return this->reason; }

private:
  const char *reason;
};

} // namespace ccpy::runtime

#endif // __CCPY_RUNTIME_HIR_RUNNER__
