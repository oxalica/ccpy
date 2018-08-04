#include <iostream>
#include <initializer_list>
#include <lccpy/ast/stmt.h>
#include <lccpy/ast/expr.h>
#include <lccpy/serialize/ast.h>
#include <lccpy/serialize/structual.h>
#include <lccpy/util/memory.h>
#include <lccpy/util/stream_wrap.h>
using namespace ccpy;
using namespace ccpy::ast;
using namespace ccpy::serialize;
using namespace std;

// Junk `std::initializer_list` does not support `move`.
template<typename T, typename ...Ts>
vector<T> move_list(T &&x, Ts &&...xs) {
  vector<T> ret {};
  ret.push_back(std::forward<T>(x));
  (ret.push_back(std::forward<Ts>(xs)), ...);
  return ret;
}

int main() {
  auto ser = ASTSerializer {};
  vector<Structual> v = {
    ser(Literal { LitEllipse {} }),
    ser(Literal { LitBool { true } }),
    ser(Literal { LitBool { false } }),
    ser(Literal { LitInteger { 123 } }),

    ser(ExprUnary {
      UnaryOp::Neg,
      ExprName { "a" }
    }),

    ser(ExprTuple { move_list(
      Expr { ExprLiteral { Literal { LitEllipse {} } } },
      ExprCall { ExprName { "func" }, move_list(
        Expr { ExprBinary {
          BinaryOp::Add,
          ExprLiteral { LitBool { true } },
          ExprUnary {
            UnaryOp::Not,
            ExprLiteral { LitInteger { 1 } },
          },
        } },
        ExprMember { ExprName { "a" }, "b" }
       ) }
    ) }),
  };

  auto oconsole = WrapOStream { cout };
  auto structual_ser = StructualSerializer {};
  for(auto &c: v)
    oconsole << structual_ser(c) << "\n";

  return 0;
}
