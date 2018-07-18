@echo off
mkdir release 2>nul
cd release
cmake -DCMAKE_BUILD_TYPE=Release .. -G Ninja && ninja
