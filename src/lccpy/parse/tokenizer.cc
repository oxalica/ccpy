#include "./tokenizer.h"
#include <algorithm>
#include <cctype>
#include <deque>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../ast/token.h"
#include "../util/stream.h"
using namespace ccpy::ast;
using namespace std;

namespace ccpy::parse {

namespace {

using OptCharRef = optional<const char &>;

size_t SYMBOL_MAX_LEN = 0;
unordered_map<Str, Keyword> KEYWORD_REV_MAP;
unordered_map<Str, Symbol> SYMBOL_REV_MAP;
bool SYMBOL_CHARS[static_cast<size_t>(numeric_limits<unsigned char>::max()) + 1];

void init_tables() {
  if(SYMBOL_MAX_LEN != 0)
    return; // Already initialized

  size_t idx = 0;
  for(auto pname: KeywordMap)
    KEYWORD_REV_MAP.emplace(Str(pname), static_cast<Keyword>(idx++));

  idx = 0;
  size_t mxlen = 0;
  for(auto pname: SymbolMap) {
    Str name { pname };
    mxlen = max(mxlen, name.length());
    for(auto c: name)
      SYMBOL_CHARS[static_cast<unsigned char>(c)] = true;
    SYMBOL_REV_MAP.emplace(move(name), static_cast<Symbol>(idx++));
  }
  SYMBOL_MAX_LEN = mxlen;
}

bool is_digit(OptCharRef c) {
  return c ? isdigit(static_cast<unsigned char>(*c)) : false;
}

bool is_name_begin(OptCharRef c) {
  return c ? *c == '_' || isalpha(static_cast<unsigned char>(*c)) : false;
}

bool is_name_continue(OptCharRef c) {
  return c ? *c == '_' || isalnum(static_cast<unsigned char>(*c)) : false;
}

bool is_symbol_char(OptCharRef c) {
  return c ? SYMBOL_CHARS[static_cast<unsigned char>(*c)] : false;
}

optional<Keyword> to_keyword(const Str &s) {
  auto it = KEYWORD_REV_MAP.find(s);
  if(it == KEYWORD_REV_MAP.end())
    return {};
  return it->second;
}

optional<Symbol> to_symbol(const Str &s) {
  auto it = SYMBOL_REV_MAP.find(s);
  if(it == SYMBOL_REV_MAP.end())
    return {};
  return it->second;
}

} // anonymous namespace

struct Tokenizer::Impl {

  IBufSource<char> &is;
  std::vector<std::size_t> indents; // Always has at least one element: 0
  bool last_newline;
  size_t cur_indent;

  bool expect_eat(char c) {
    if(this->is.peek() != c)
      return false;
    this->is.get();
    return true;
  }

  optional<Token> get() {
    if(!this->last_newline && this->expect_eat('\n')) {
      this->last_newline = true;
      return TokNewline {};
    }
    if(this->last_newline || !this->is.peek()) { // or EOF
      this->last_newline = false;
      do {
        this->cur_indent = this->eat_indent();
        this->eat_space_comment(); // Remove tailing spaces and comments
      } while(this->expect_eat('\n')); // Then empty lines are skipped
    }
    if(this->cur_indent < this->indents.back()) {
      this->indents.pop_back();
      return TokDedent {};
    }
    // `Indent` should go after `Dedent`. See also `tests/tokenizer/indent0`
    if(this->cur_indent > this->indents.back()) {
      this->indents.push_back(this->cur_indent);
      return TokIndent {};
    }
    auto tok = this->eat_token();
    this->eat_space_comment();
    return tok;
  }

  void eat_space_comment() {
    while(this->expect_eat(' '))
      ;
    if(this->is.peek() == '#')
      while(this->is.peek().value_or('\n') != '\n') // Treat EOF as `\n`
        this->is.get();
  }

  // If already met EOF, it returns 0.
  size_t eat_indent() {
    size_t count = 0;
    while(this->expect_eat(' '))
      ++count;
    return count;
  }

  optional<Token> eat_token() {
    auto c = this->is.peek();
    if(!c)
      return {}; // Stream end
    else if(is_digit(c))
      return this->eat_num();
    else if(is_name_begin(c))
      return this->eat_name_or_kw();
    else if(is_symbol_char(c))
      return this->eat_symbol();
    else
      throw StreamFailException { "Invalid token beginning" };
  }

  Token eat_num() {
    Integer num { 0 };
    while(is_digit(this->is.peek()))
      num = num * 10 + int(*this->is.get() - '0');
    return TokInteger { num };
  }

  Token eat_name_or_kw() {
    std::string s { *this->is.get() };
    while(is_name_continue(this->is.peek()))
      s.push_back(*this->is.get());
    if(auto kw = to_keyword(s))
      return TokKeyword { *kw };
    return TokName { move(s) };
  }

  Token eat_symbol() {
    string eaten {};
    size_t len;
    for(len = 0; len < SYMBOL_MAX_LEN; ++len) {
      auto c = this->is.peek();
      if(!is_symbol_char(c))
        break;
      eaten.push_back(*this->is.get());
    }
    // Backtrace to find the longest match
    optional<Symbol> sym;
    while(len > 0 && !(sym = to_symbol(eaten))) {
      this->is.putback(eaten.back()); // If not found, putback and loop
      eaten.pop_back();
      --len;
    }
    if(len == 0)
      throw StreamFailException { "Invalid symbol" };
    return TokSymbol { *sym };
  }
};

Tokenizer::Tokenizer(IBufSource<char> &_is)
  : pimpl(Impl { _is, { 1, 0 }, true, 0 }) { // indents: [0]
  init_tables();
}

Tokenizer::~Tokenizer() noexcept {}

optional<Token> Tokenizer::get() {
  return this->pimpl->get();
}

} // namespace ccpy::parse

