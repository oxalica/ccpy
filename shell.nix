with import <nixpkgs> {};
mkShell {
  buildInputs = [ cmake glibc.static ];
}
