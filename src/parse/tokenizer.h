#ifndef __CCPY_PARSE_TOKENIZER__
#define __CCPY_PARSE_TOKENIZER__

#include <cstdlib>
#include <vector>
#include "../ast/token.h"
#include "../util/stream.h"
#include "../util/util.h"

namespace ccpy::parse {

class Tokenizer: public IBufSource<ast::Token> {
public:
  explicit Tokenizer(IBufSource<char> &);
  virtual ~Tokenizer();

  virtual ast::Token get();
  virtual const ast::Token &peek();
  virtual void putback(ast::Token &&);

private:
  struct Impl;
  owned<Impl> pimpl;
};

} // namespace ccpy::parse

#endif // __CCPY_PARSE_TOKENIZER__