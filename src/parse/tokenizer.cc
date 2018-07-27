#include "./tokenizer.h"
#include <algorithm>
#include <cctype>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../ast/token.h"
#include "../util/util.h"
#include "../util/stream.h"
using namespace ccpy::ast;
using namespace std;

namespace ccpy::parse {

namespace {

static bool is_digit(char c) {
  return isdigit(static_cast<unsigned char>(c));
}

static bool is_name_begin(char c) {
  return c == '_' || isalpha(static_cast<unsigned char>(c));
}

static bool is_name_continue(char c) {
  return c == '_' || isalnum(static_cast<unsigned char>(c));
}

} // anonymous namespace

struct Tokenizer::Impl {
  static size_t symbol_max_len;
  static unordered_map<Str, Keyword> keyword_rev_map;
  static unordered_map<Str, Symbol> symbol_rev_map;
  static unordered_set<char> symbol_chars;

  IBufSource<char> &is;
  std::deque<ast::Token> buf;
  std::vector<std::size_t> indents;
  bool newline;
  bool end;

  static void init_static() {
    if(symbol_max_len != 0)
      return; // Already init

    size_t idx = 0;
    for(auto pname: KeywordMap)
      keyword_rev_map.emplace(Str(pname), static_cast<Keyword>(idx++));

    idx = 0;
    size_t mxlen = 0;
    for(auto pname: SymbolMap) {
      Str name { pname };
      mxlen = max(mxlen, name.length());
      for(auto c: name)
        symbol_chars.insert(c);
      symbol_rev_map.emplace(move(name), static_cast<Symbol>(idx++));
    }
    symbol_max_len = mxlen;
  }

  Token get() {
    this->peek();
    auto ret { move(this->buf.front()) };
    this->buf.pop_front();
    return ret;
  }

  const Token &peek() {
    while(this->buf.empty())
      this->product();
    return this->buf.front();
  }

  void putback(Token &&tok) {
    this->buf.push_front(move(tok));
  }

  void product() {
    if(this->end)
      throw StreamEndException {};

    try {
      auto cur_indent = this->eat_indent();
      if(this->is.peek() == '\n') {
        this->is.get();
        return; // Skip empty line
      }
      while(this->indents.back() > cur_indent) {
        this->indents.pop_back();
        this->buf.push_back(Token { TokDedent {} });
      }
      if(cur_indent > this->indents.back()) {
        this->indents.push_back(cur_indent);
        this->buf.push_back(Token { TokIndent {} });
      }
      while(!this->eat_space())
        this->buf.push_back(this->eat_token());
    } catch(StreamEndException) {
      this->end = true;
      this->clear_indents();
    }
  }

  bool eat_space() { // Return whether it eat `\n`
    for(;;)
      switch(this->is.peek()) {
        case '#':
          while(this->is.get() != '\n')
            ;
          return true;
        case '\n':
          this->is.get();
          return true;
        case ' ':
          this->is.get();
          continue;
        default:
          return false;
      }
  }

  size_t eat_indent() {
    size_t cur_indent = 0;
    while(this->is.peek() == ' ') {
      this->is.get();
      ++cur_indent;
    }
    return cur_indent;
  }

  void clear_indents() {
    while(this->indents.back() != 0) {
      this->buf.push_back(Token { TokDedent {} });
      this->indents.pop_back();
    }
  }

  Token eat_token() {
    char c = this->is.get();
    if(is_digit(c)) {
      // Integer
      Integer num { int(c - '0') };
      while(isdigit(this->is.peek()))
        num = num * 10 + int(this->is.get() - '0');
      return Token { TokInteger { num } };

    } else if(is_name_begin(c)) {
      // Name
      std::string s { c };
      while(is_name_continue(c = this->is.peek()))
        s.push_back(this->is.get());
      return this->postprocess_name(move(s));

    } else if(symbol_chars.find(c) != symbol_chars.end()) {
      // Symbol
      size_t len;
      string eaten { c };
      for(len = 1; len <= symbol_max_len; ++len) {
        try {
          if(symbol_chars.find(this->is.peek()) != symbol_chars.end())
            eaten.push_back(this->is.get());
          else
            break;
        } catch(StreamEndException) {
          break;
        }
      }
      // Backtrace to find the longest match
      while(len > 0 && symbol_rev_map.find(eaten) == symbol_rev_map.end()) {
        this->is.putback(eaten.back());
        eaten.pop_back();
        --len;
      }
      if(len == 0)
        throw StreamFailException { "Invalid symbol" };
      return Token { TokSymbol { symbol_rev_map.find(eaten)->second } };

    } else {
      throw StreamFailException { "Invalid token" };
    }
  }

  Token postprocess_name(Str s) {
    auto it = keyword_rev_map.find(s);
    if(it != keyword_rev_map.end())
      return Token { TokKeyword { it->second } };
    return Token { TokName { move(s) } };
  }
};

size_t Tokenizer::Impl::symbol_max_len = 0;
unordered_map<Str, Keyword> Tokenizer::Impl::keyword_rev_map;
unordered_map<Str, Symbol> Tokenizer::Impl::symbol_rev_map;
unordered_set<char> Tokenizer::Impl::symbol_chars;

Tokenizer::Tokenizer(IBufSource<char> &_is)
  : pimpl(Impl { _is, {}, { 0 }, false, false }) {
  Impl::init_static();
}

Tokenizer::~Tokenizer() {}

Token Tokenizer::get() {
  return this->pimpl->get();
}

const Token &Tokenizer::peek() {
  return this->pimpl->peek();
}

void Tokenizer::putback(Token &&tok) {
  this->pimpl->putback(move(tok));
}

} // namespace ccpy::parse

