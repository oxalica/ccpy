#include <iostream>
#include <lccpy/util/stream_wrap.h>
#include <lccpy/util/types.h>
#include <lccpy/serialize/number.h>
using namespace ccpy;
using namespace ccpy::serialize;
using namespace std;

int main() {
  vector<Integer> ints = {
    0,
    123,
    -123,
  };
  vector<Decimal> decs = {
    +0.0,
    -0.0,
    123.5,
    -0.75,
  };

  auto oconsole = WrapOStream { cout };
  auto int_ser = IntegerSerializer {};
  for(auto &c: ints)
    oconsole << int_ser(c) << "\n";

  auto dec_ser = DecimalSerializer {};
  for(auto &c: decs)
    oconsole << dec_ser(c) << "\n";
  return 0;
}

