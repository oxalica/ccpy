#ifndef __CCPY_PARSE_PARSER__
#define __CCPY_PARSE_PARSER__

#include <optional>
#include "../ast/token.h"
#include "../ast/stmt.h"
#include "../util/stream.h"
#include "../util/util.h"

namespace ccpy::parse {

class Parser: public ISource<ast::Stmt> {
public:
  explicit Parser(IBufSource<ast::Token> &);
  virtual ~Parser() noexcept;

  virtual std::optional<ast::Stmt> get();

private:
  struct Impl;
  owned<Impl> pimpl;
};

} // namespace ccpy::parse

#endif // __CCPY_PARSE_PARSER__
