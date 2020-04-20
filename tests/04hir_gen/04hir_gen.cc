#include <iostream>
#include <fstream>
#include <lccpy/hir/gen.h>
#include <lccpy/parse/tokenizer.h>
#include <lccpy/parse/parser.h>
#include <lccpy/serialize/hir.h>
#include <lccpy/util/stream_wrap.h>
using namespace ccpy;
using namespace ccpy::hir;
using namespace ccpy::parse;
using namespace ccpy::serialize;
using namespace std;

int main() {
  auto iconsole = WrapIStream { cin };
  auto oconsole = WrapOStream { cout };

  vector<ast::Stmt> stmts;

  try {
    auto tok = Tokenizer { iconsole };
    auto buf_tok = Buffered { tok };
    auto parser = Parser { buf_tok };
    while(auto stmt = parser.get())
      stmts.push_back(move(*stmt));

  } catch(const StreamFailException &e) {
    oconsole << "Parse fail: " << e.what() << "\n";
  } catch(const StreamFatalException &e) {
    oconsole << "Parse fatal: " << e.what() << "\n";
  }

  try {
    auto mod = HIRGen {}(stmts);
    oconsole << HIRSerializer {}(mod);
  } catch(const HIRGenException &e) {
    oconsole << "Gen fail: " << e.what() << "\n";
  }
  return 0;
}

