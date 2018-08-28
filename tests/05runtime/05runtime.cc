#include <iostream>
#include <iomanip>
#include <lccpy/hir/gen.h>
#include <lccpy/parse/tokenizer.h>
#include <lccpy/parse/parser.h>
#include <lccpy/runtime/hir_runner.h>
#include <lccpy/runtime/intrinsic.h>
#include <lccpy/serialize/hir.h>
#include <lccpy/util/stream_wrap.h>
using namespace ccpy;
using namespace ccpy::hir;
using namespace ccpy::parse;
using namespace ccpy::serialize;
using namespace ccpy::runtime;
using namespace std;

int main() {
  auto iconsole = WrapIStream { cin };

  vector<ast::Stmt> stmts;

  try {
    auto tok = Tokenizer { iconsole };
    auto buf_tok = Buffered { tok };
    auto parser = Parser { buf_tok };
    while(auto stmt = parser.get())
      stmts.push_back(move(*stmt));

  } catch(StreamFailException e) {
    cout << "Parse fail: " << e.what() << "\n";
  } catch(StreamFatalException e) {
    cout << "Parse fatal: " << e.what() << "\n";
  }

  Module mod;

  try {
    mod = HIRGen {}(stmts);
    cout << HIRSerializer {}(mod);
  } catch(HIRGenException e) {
    cout << "Gen fail: " << e.what() << "\n";
  }

  try {
    HIRRunner runner { cin, cout };
    auto id = runner.load(move(mod));
    runner.run(id);
  } catch(HIRRuntimeException e) {
    cout << "HIR runtime error: " << e.what() << "\n";
  } catch(IntrinsicException e) {
    cout << "Intrinsic error: " << e.what() << "\n";
  }

  cout << "Done\n";
  return 0;
}


