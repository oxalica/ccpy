#include "./token.h"
#include <optional>
#include <utility>
#include "./structual.h"
#include "../util/util.h"
using namespace std;
using namespace ccpy::ast;

namespace ccpy::serialize {

struct TokenTreeSerializer::Impl {
  StructualSerializer &down;

  DECL_OP_PUT

  void put(const Structual &x) {
    this->down << x;
  }

  void put_parened(Str name, Str value) {
    this->down << Structual { StructParen {
      optional<Str> { name },
      vector<Structual> { StructValue { "\"" + value + "\"" } }
    } };
  }

  void put(const Token &tok) {
    match(tok,
      [&](const TokKeyword &tok) {
        auto kw = KeywordMap[static_cast<size_t>(tok.keyword)];
        this->put_parened("Keyword", kw);
      },
      [&](const TokSymbol &tok) {
        auto sym = SymbolMap[static_cast<size_t>(tok.symbol)];
        this->put_parened("Symbol", sym);
      },
      [&](const TokName &tok) {
        this->put_parened("Name", tok.name);
      },
      [&](const TokInteger &tok) {
        this->put_parened("Integer", to_str(tok.integer));
      },
      [&](const TokIndent &) {
        *this << Structual { StructValue { "Indent" } };
      },
      [&](const TokDedent &) {
        *this << Structual { StructValue { "Dedent" } };
      }
    );
  }
};

TokenTreeSerializer::TokenTreeSerializer(StructualSerializer &down)
  : pimpl(Impl { down }) {}

TokenTreeSerializer::~TokenTreeSerializer() noexcept {}

void TokenTreeSerializer::put(const Token &tok) {
  this->pimpl->put(tok);
}


} // namespace ccpy::serialize
