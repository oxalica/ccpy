#include <iostream>
#include <fstream>
#include <lccpy/ast/early_fold.h>
#include <lccpy/parse/tokenizer.h>
#include <lccpy/parse/parser.h>
#include <lccpy/serialize/ast.h>
#include <lccpy/serialize/structual.h>
#include <lccpy/util/stream_wrap.h>
using namespace ccpy;
using namespace ccpy::ast;
using namespace ccpy::parse;
using namespace ccpy::serialize;
using namespace std;

int main() {
  auto iconsole = WrapIStream { cin };
  auto tok = Tokenizer { iconsole };
  auto buf_tok = Buffered { tok };
  auto parser = Parser { buf_tok };
  auto oconsole = WrapOStream { cout };

  auto ser_ast = [](auto &x) {
    return StructualSerializer {}(ASTSerializer{} (x));
  };

  try {
    while(auto c = parser.get()) {
      oconsole << ser_ast(*c) << "\n";
      auto ast_folded = EarlyFold {}(move(*c));
      oconsole << ser_ast(ast_folded) << "\n\n";
    }
    oconsole << "End\n";
  } catch(StreamFailException e) {
    oconsole << "Fail: " << e.what() << "\n";
  } catch(StreamFatalException e) {
    oconsole << "Fatal: " << e.what() << "\n";
  } catch(EarlyFoldException e) {
    oconsole << "Fold error: " << e.what() << "\n";
  }
  return 0;
}
