#include <iostream>
#include <lccpy/util/stream_wrap.h>
#include <lccpy/serialize/structual.h>
using namespace ccpy;
using namespace ccpy::serialize;
using namespace std;

int main() {
  vector<Structual> v = {
    StructValue { "Value" },
    StructParen { "Fn0", {} },
    StructParen { "Fn1", { StructValue { "e0" } } },
    StructParen { "Fn2", { StructValue { "e0" }, StructValue { "e1" } } },
    StructParen { {}, {
      StructStr { "" },
      StructStr { "string! (escape" },
      StructStr { "\"" },
      StructStr { "\\\"\\" },
    } },
    StructBracket { "Array0", {} },
    StructBracket { "Array1", { StructValue { "e0" } } },
    StructBracket { "Array2", { StructValue { "e0" }, StructValue { "e1" } } },
    StructBrace { "Obj0", {} },
    StructBrace { "Obj1", { StructValue { "e0" } } },
    StructBrace { "Obj2", { StructValue { "e0" }, StructValue { "e1" } } },
    StructBrace {
      "BigStruct",
      {
        StructValue { "value" },
        StructParen {
          "f",
          {
            StructValue { "hello" },
            StructBracket { {}, {
              StructValue { "1" },
              StructValue { "2" },
            } },
            StructBrace { {}, {} }
          }
        },
        StructParen { {}, {
          StructValue { "a" },
          StructValue { "b" },
        } }
      }
    }
  };

  auto oconsole = WrapOStream { cout };
  auto structual_ser = StructualSerializer {};
  for(auto &c: v)
    oconsole << structual_ser(c) << "\n";

  return 0;
}
