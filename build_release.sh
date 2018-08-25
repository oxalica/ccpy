#!/bin/sh
mkdir release 2>/dev/null
cd release
cmake -DCMAKE_BUILD_TYPE=Release .. && make all
