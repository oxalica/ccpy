#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <lccpy/ast/early_fold.h>
#include <lccpy/hir/gen.h>
#include <lccpy/parse/tokenizer.h>
#include <lccpy/parse/parser.h>
#include <lccpy/runtime/hir_runner.h>
#include <lccpy/runtime/intrinsic.h>
#include <lccpy/serialize/hir.h>
#include <lccpy/util/adt.h>
#include <lccpy/util/stream_wrap.h>
using namespace std;
using namespace ccpy;
using ccpy::hir::Module;

const char *DEFAULT_STD_FILE = "std/__builtins__.py";

class InterpretException: public exception {
public:
  InterpretException(string _reason): reason(move(_reason)) {}
  virtual ~InterpretException() noexcept {}

  virtual const char *what() const noexcept { return this->reason.c_str(); }

private:
  string reason;
};

Module compile(std::istream &is) {
  using namespace ccpy::parse;
  using namespace ccpy::hir;

  auto source = WrapIStream { is };
  vector<ast::Stmt> stmts;

  try {
    auto tok = Tokenizer { source };
    auto buf_tok = Buffered { tok };
    auto parser = Parser { buf_tok };
    while(auto stmt = parser.get())
      stmts.push_back(ast::EarlyFold {}(move(*stmt)));
    return HIRGen {}(stmts);

  } catch(const StreamFailException &e) {
    throw InterpretException { string("Parse fail: ") + e.what() };

  } catch(const StreamFatalException &e) {
    throw InterpretException { string("Parse fatal: ") + e.what() };

  } catch(const HIRGenException &e) {
    throw InterpretException { string("Generate HIR fail: ") + e.what() };
  }
}

void run_sources(
  const vector<string> &source_paths,
  std::istream &is,
  std::ostream &os
) {
  using namespace runtime;

  HIRRunner runner { is, os };
  vector<size_t> idxs;

  for(auto &path: source_paths)
    try {
      ifstream fin(path);
      if(!fin)
        throw InterpretException { "Cannot open file" };
      auto mod = compile(fin);
      fin.close();
      auto idx = runner.load(move(mod));
      idxs.push_back(idx);
    } catch(const InterpretException &e) {
      throw InterpretException { path + ": " + e.what() };
    }

  try {
    for(auto idx: idxs)
      runner.run(idx);
  } catch(const HIRRuntimeException &e) {
    throw InterpretException { string("HIR runtime error: ") + e.what() };
  } catch(const IntrinsicException &e) {
    throw InterpretException { string("Intrinsic error: ") + e.what() };
  }
}

int main(int argc, char *argv[]) {
  if(argc <= 1) {
    cerr << "Usage: ccpy [--no-std] <SOURCE_FILE> ...\n"
         << "\n"
         << "  <STD_PATH> is default to be `std/__builtins__.py`.\n"
         << "  All <SOURCE_FILE>s will be compiled first and run one by one.\n"
         << endl;
    return EXIT_FAILURE;
  }

  vector<string> sources;
  int i = 1;
  if(argv[i] == string { "--no-std" })
    ++i;
  else if(string(argv[i]).substr(0, 1) == "-") {
    cerr << "Unknow flag `" << argv[i] << "`\n"
         << "Run without arguments to see usage.\n"
         << endl;
    return EXIT_FAILURE;
  } else
    sources.emplace_back(DEFAULT_STD_FILE);
  for(; i < argc; ++i)
    sources.emplace_back(argv[i]);

  if(sources.empty()) {
    cerr << "No source file.\n"
         << endl;
    return EXIT_FAILURE;
  }

  try {
    run_sources(sources, cin, cout);
    return EXIT_SUCCESS;
  } catch(const InterpretException &e) {
    cerr << "[ERR] " << e.what() << "\n"
         << endl;
    return EXIT_FAILURE;
  }
}
