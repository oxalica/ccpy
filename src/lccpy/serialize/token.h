#ifndef __CCPY_SERIALIZE_TOKEN__
#define __CCPY_SERIALIZE_TOKEN__

#include "./structual.h"
#include "../ast/token.h"

namespace ccpy::serialize {

class TokenTreeSerializer: public ITrans<ast::Token, Structual> {
public:
  TokenTreeSerializer() {}
  virtual ~TokenTreeSerializer() noexcept {}

  virtual Structual operator()(const ast::Token &) const;
};

} // namespace ccpy

#endif // __CCPY_SERIALIZE_TOKEN__
