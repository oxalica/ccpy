#ifndef __CCPY_SERIALIZE_TOKEN__
#define __CCPY_SERIALIZE_TOKEN__

#include "./structual.h"
#include "../ast/token.h"
#include "../util/util.h"

namespace ccpy::serialize {

class TokenTreeSerializer: public ISerializer<ast::Token> {
public:
  TokenTreeSerializer(StructualSerializer &);
  virtual ~TokenTreeSerializer() noexcept;

  virtual void put(const ast::Token &);

private:
  struct Impl;
  owned<Impl> pimpl;
};

} // namespace ccpy

#endif // __CCPY_SERIALIZE_TOKEN__
