@echo off
mkdir debug 2>nul
cd debug
cmake -DCMAKE_BUILD_TYPE=Debug .. -G Ninja && ninja || exit
cd ..
"./debug/test_runner" debug debug/tmp.out
