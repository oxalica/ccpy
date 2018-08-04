#include <iostream>
#include <fstream>
#include <lccpy/parse/tokenizer.h>
#include <lccpy/parse/parser.h>
#include <lccpy/serialize/ast.h>
#include <lccpy/serialize/structual.h>
#include <lccpy/util/stream_wrap.h>
using namespace ccpy;
using namespace ccpy::parse;
using namespace ccpy::serialize;
using namespace std;

int main() {
  auto iconsole = WrapIStream { cin };
  auto tok = Tokenizer { iconsole };
  auto buf_tok = Buffered { tok };
  auto parser = Parser { buf_tok };
  auto oconsole = WrapOStream { cout };
  auto structual_ser = StructualSerializer {};
  auto ast_ser = ASTSerializer {};

  try {
    while(auto c = parser.get())
      oconsole << structual_ser(ast_ser(*c)) << "\n";
    oconsole << "End\n";
  } catch(StreamFailException e) {
    oconsole << "Fail: " << e.what() << "\n";
  } catch(StreamFatalException e) {
    oconsole << "Fatal: " << e.what() << "\n";
  }
  return 0;
}
