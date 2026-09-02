#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

echo "== CTest =="
ctest --test-dir build --output-on-failure

echo "== Full CK Q-space =="
./build/exact_synth --count-ck

echo "== MUS =="
./build/exact_synth --mus-demo

echo "== Exact circuit demo =="
./build/exact_circuit --demo
