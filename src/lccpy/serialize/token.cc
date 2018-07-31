#include "./token.h"
#include <optional>
#include <utility>
#include "./structual.h"
#include "../util/util.h"
using namespace std;
using namespace ccpy::ast;

namespace ccpy::serialize {

namespace {

Structual parened(Str name, Str value) {
  return Structual { StructParen {
    optional<Str> { move(name) },
    vector<Structual> { StructValue { "\"" + move(value) + "\"" } }
  } };
}

Structual trans(const Token &tok) {
  return match(tok,
    [&](const TokKeyword &tok) {
      auto kw = KeywordMap[static_cast<size_t>(tok.keyword)];
      return parened("Keyword", kw);
    },
    [&](const TokSymbol &tok) {
      auto sym = SymbolMap[static_cast<size_t>(tok.symbol)];
      return parened("Symbol", sym);
    },
    [&](const TokName &tok) {
      return parened("Name", tok.name);
    },
    [&](const TokInteger &tok) {
      return parened("Integer", to_str(tok.integer));
    },
    [&](const TokIndent &) {
      return Structual { StructValue { "Indent" } };
    },
    [&](const TokDedent &) {
      return Structual { StructValue { "Dedent" } };
    }
  );
}

} // namespace anonymous


Structual TokenTreeSerializer::operator()(const Token &tok) const {
  return trans(tok);
}


} // namespace ccpy::serialize
