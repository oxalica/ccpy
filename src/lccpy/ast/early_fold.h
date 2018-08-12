#ifndef __CCPY_AST_EARLY_FOLD__
#define __CCPY_AST_EARLY_FOLD__

#include <exception>
#include "./stmt.h"
#include "../util/trans.h"

namespace ccpy::ast {

class EarlyFold: public IHardTrans<ast::Stmt, ast::Stmt> {
public:
  EarlyFold() {}
  virtual ~EarlyFold() noexcept {}

  ast::Stmt operator()(ast::Stmt &&) const;
};

class EarlyFoldException: public std::exception {
public:
  EarlyFoldException(const char *_reason): reason(_reason) {}
  virtual ~EarlyFoldException() noexcept {}

  virtual const char *what() const noexcept { return this->reason; }

private:
  const char *reason;
};

} // namespace ccpy::ast

#endif // __CCPY_AST_EARLY_FOLD__
