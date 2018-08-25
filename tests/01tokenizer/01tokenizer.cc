#include <iostream>
#include <fstream>
#include <lccpy/util/stream_wrap.h>
#include <lccpy/parse/tokenizer.h>
#include <lccpy/serialize/token.h>
using namespace ccpy;
using namespace ccpy::parse;
using namespace ccpy::serialize;
using namespace std;

int main() {
  auto iconsole = WrapIStream { cin };
  auto tok = Tokenizer { iconsole };
  auto oconsole = WrapOStream { cout };
  auto structual_ser = StructualSerializer {};
  auto token_ser = TokenTreeSerializer {};

  try {
    while(auto c = tok.get())
      oconsole << structual_ser(token_ser(*c)) << "\n";
    oconsole << "End\n";
  } catch(StreamFailException e) {
    oconsole << "Fail: " << e.what() << "\n";
  } catch(StreamFatalException e) {
    oconsole << "Fatal: " << e.what() << "\n";
  }
  return 0;
}
