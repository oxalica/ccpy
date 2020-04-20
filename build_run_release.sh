#!/usr/bin/env bash
set -e

mkdir -p release
cd release
cmake -DCMAKE_BUILD_TYPE=Release .. && make all -j4
cd ..
./release/test_runner ./release /tmp/tmp.out
