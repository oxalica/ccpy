#ifndef __CCPY_HIR_GEN__
#define __CCPY_HIR_GEN__

#include <exception>
#include <vector>
#include "./hir.h"
#include "../ast/stmt.h"
#include "../util/adt.h"
#include "../util/macro.h"
#include "../util/trans.h"

namespace ccpy::hir {

class HIRGen
  : public ITrans<std::vector<ast::Stmt>, Module> {
public:
  HIRGen() {}
  virtual ~HIRGen() noexcept {}

  Module operator()(const std::vector<ast::Stmt> &) const;
};

class HIRGenException: public std::exception {
public:
  HIRGenException(const char *_reason) noexcept: reason(_reason) {}
  virtual ~HIRGenException() noexcept {}

  const char *what() noexcept { return this->reason; }

private:
  const char *reason;
};

} // namespace ccpy::hir

#endif // __CCPY_HIR_GEN__
