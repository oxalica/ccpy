# CCPY
CCPY is a simple C++ implemented interpreter of a subset of Python 3.

Currently it works in these steps:
1. Tokenize and parse source codes into AST (abstract syntax tree).
   - Indentation translation is done in tokenizer
   - Tokenizer and parser are both pull-driven streams.
2. Apply AST-based trivial constant folding.
3. Lower AST into a register-based dynamic-typed HIR (high level intermediate representation).
   - Name resolution and variable capturing are done in lowering.
   - Local variables are stored in pre-estimated function frames with name erased.
4. Execute HIR in a simple runtime.
   - Still dynamic typed.
   - Code interact with the world through special intrinsic functions.

You can look into `grammar.bnf` or examples in `tests/06std/*.in` for *what syntax/statements/expressions are implemented yet*.

`std/__builtins__.py` contains partially implemented `__builtins__`, and necessary helper functions
which will be invoked by some special syntax/statements/expressions to simplify HIR generator.
