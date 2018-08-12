#ifndef __CCPY_SERIALIZE_AST__
#define __CCPY_SERIALIZE_AST__

#include "./structual.h"
#include "../ast/stmt.h"
#include "../ast/expr.h"
#include "../util/trans.h"

namespace ccpy::serialize {

class ASTSerializer
  : public ITrans<ast::Stmt, Structual>
  , public ITrans<ast::Expr, Structual>
  , public ITrans<ast::Literal, Structual> {
public:
  ASTSerializer() {}
  virtual ~ASTSerializer() noexcept {}

  virtual Structual operator()(const ast::Stmt &) const;
  virtual Structual operator()(const ast::Expr &) const;
  virtual Structual operator()(const ast::Literal &) const;
};

} // namespace ccpy::serialize

#endif // __CCPY_SERIALIZE_AST__
