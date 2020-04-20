#include <fstream>
#include <iostream>
#include <iomanip>
#include <lccpy/hir/gen.h>
#include <lccpy/parse/tokenizer.h>
#include <lccpy/parse/parser.h>
#include <lccpy/runtime/hir_runner.h>
#include <lccpy/runtime/intrinsic.h>
#include <lccpy/serialize/hir.h>
#include <lccpy/util/adt.h>
#include <lccpy/util/stream_wrap.h>
using namespace ccpy;
using namespace ccpy::hir;
using namespace ccpy::parse;
using namespace ccpy::serialize;
using namespace ccpy::runtime;
using namespace std;

std::optional<Module> compile(std::istream &is) {
  auto source = WrapIStream { is };
  vector<ast::Stmt> stmts;

  try {
    auto tok = Tokenizer { source };
    auto buf_tok = Buffered { tok };
    auto parser = Parser { buf_tok };
    while(auto stmt = parser.get())
      stmts.push_back(move(*stmt));

    return HIRGen {}(stmts);
  } catch(const StreamFailException &e) {
    cout << "Parse fail: " << e.what() << "\n";
  } catch(const StreamFatalException &e) {
    cout << "Parse fatal: " << e.what() << "\n";
  } catch(const HIRGenException &e) {
    cout << "Gen fail: " << e.what() << "\n";
  }
  return {};
}

int main() {
  ifstream fstd { "std/__builtins__.py" };
  if(!fstd) {
    cout << "Cannot open std file\n";
    return 0;
  }

  auto mod_std = compile(fstd);
  if(!mod_std)
    cout << "Std module compile failed\n";

  auto mod_main = compile(cin);
  if(!mod_main)
    cout << "Main module compile failed\n";
  if(!mod_std || !mod_main)
    return 0;

  try {
    HIRRunner runner { cin, cout };
    auto id_std = runner.load(move(*mod_std));
    auto id_main = runner.load(move(*mod_main));
    runner.run(id_std);
    cout << "Std done\n\n";
    runner.run(id_main);
    cout << "Done\n";
  } catch(const HIRRuntimeException &e) {
    cout << "HIR runtime error: " << e.what() << "\n";
  } catch(const IntrinsicException &e) {
    cout << "Intrinsic error: " << e.what() << "\n";
  }
  return 0;
}


