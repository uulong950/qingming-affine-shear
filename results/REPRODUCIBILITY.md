# Reproducibility

The repository is source-only. No prebuilt binary is required.

## Toolchain

Recommended:

```text
C++17
GCC 10+ or Clang 12+
CMake 3.16+
```

No ROCm, HIP, CUDA, Python, or external C++ library is required for the algebraic artifact.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Fast tests

```bash
./build/exact_synth --self-test
./build/exact_circuit --self-test
```

Expected invariant highlights:

```text
Grassmann counts:
1,31,155,155,31,1

n=2 hardware rank-ceiling:
failures = 0

exact circuit states through four gates:
1,38,915,17942,331291
```

## CK case study

```bash
./build/exact_synth --case-study
```

Expected:

```text
diagonal original:      rank=0, banks=1,  max_mult=32
diagonal proposed:      rank=5, banks=32, max_mult=1
anti-diagonal original: rank=0, banks=1,  max_mult=32
anti-diagonal proposed: rank=5, banks=32, max_mult=1
stride-2 original:      rank=5, banks=32, max_mult=1
stride-2 proposed:      rank=5, banks=32, max_mult=1
```

## Complete Q-space enumeration

```bash
./build/exact_synth --count-ck
```

Expected:

```text
Q-space candidates checked: 33554432
total simultaneous rank-optimal Q: 2887680
```

Runtime is machine-dependent. The discrete result should be invariant.

## MUS extraction

```bash
./build/exact_synth --mus-demo
```

Expected:

```text
Inclusion-minimal infeasible core size: 6
```

Each one-pattern deletion should report `SAT` with a witness.

## Exact circuit demo

```bash
./build/exact_circuit --demo
```

Expected:

```text
R4 exact minimum gate count: 1
P_star exact minimum gate count: 4
I^L1^R4 exact minimum gate count: 4
robust-v2 target not found through 4 gates
certified lower bound >= 5
```

## One-command validation

```bash
./scripts/validate.sh
```

The script builds a clean release configuration and runs the principal checks.

## Reference output

See [`REFERENCE_RESULTS.txt`](REFERENCE_RESULTS.txt).

Timing values in that file are examples only. Compare discrete counts, matrices, SAT/UNSAT status, ranks, and witness codes rather than wall-clock time.
