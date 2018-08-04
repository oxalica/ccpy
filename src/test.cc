#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

const char TEST_LIST_PATH[] = "test_list.txt";
const char TMP_PATH[] = "tmp.out";

struct Test {
  string name;
  vector<string> tests;
};

string get_filename(const string &path) {
  auto sep = path.find_last_of("/\\");
  assert(sep != string::npos);
  return path.substr(sep + 1);
}

string get_bare_filename(const string &path) {
  auto name = get_filename(path);
  return name.substr(0, name.rfind('.'));
}

vector<Test> load_tests() {
  ifstream fin(TEST_LIST_PATH);
  vector<Test> ret {};
  string buf;
  while(getline(fin, buf), !buf.empty()) {
    Test cur;
    cur.name = move(buf);
    while(getline(fin, buf), !buf.empty())
      cur.tests.push_back(buf);
    ret.push_back(cur);
  }
  return ret;
}

bool file_eq(const string &path1, const string &path2) {
  ifstream f1 { path1 }, f2 { path2 };
  while(f1 && f2)
    if(f1.get() != f2.get())
      return false;
  return f1.eof() && f2.eof();
}

void file_copy(const string &source, const string &dest) {
  ifstream fin { source };
  ofstream fout { dest };
  char c;
  while(fin.get(c))
    fout.put(c);
}

bool run_one(const string &name, const string &in_path, const string &std_path) {
  string cmd {
#ifdef _WIN32
    "test_" + name
#else
    "./test_" + name
#endif
    + " <" + in_path
    + " >" + TMP_PATH
  };
  int ret = system(cmd.c_str());
  if(ret != 0) {
    cerr << "Exit with [" << ret << "]" << flush;
    return false;
  }
  if(!file_eq(TMP_PATH, std_path)) {
    cerr << "Different" << flush;
    file_copy(TMP_PATH, std_path);
    return false;
  }
  return true;
}

bool run(const vector<Test> &tests) {
  cout << "Total " << tests.size() << " tests." << endl;
  size_t cnt = 0;
  for(auto &test: tests) {
    cout << "[" << ++cnt << "/" << tests.size() <<  "]"
         << " Testing " << test.name
         << " ( " << test.tests.size() << " samples )" << endl;
    for(auto &path: test.tests) {
      string opath { path };
      opath.erase(end(opath) - 3, end(opath)); // Remove `.in`
      opath += ".out";

      cout << "  " << get_bare_filename(path) << ": " << flush;
      if(run_one(test.name, path, opath))
        cout << " OK" << endl;
      else {
        cout << " FAILED" << endl;
        return false;
      }
    }
  }
  cout << "ALL TEST OK" << endl;
  return true;
}

int main(int argc, char *argv[]) {
  auto all_tests = load_tests();
  vector<Test> run_tests {};

  if(argc == 1) // No args
    run_tests = all_tests;
  else {
    for(int i = 1; i < argc; ++i) { // Skip the first
      string name { argv[i] };
      auto it = find_if(begin(all_tests), end(all_tests), [&](auto &c) {
        return c.name == name;
      });
      if(it == all_tests.end()) { // Not found
        cerr << "Test '" << name << "' not found." << endl;
        return 1;
      }
      run_tests.push_back(move(*it));
      all_tests.erase(it);
    }
  }

  return run(run_tests)
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
