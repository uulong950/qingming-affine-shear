# Qingming Affine Shear

**Rank-theoretic shared-memory bank mapping, exact multi-access swizzle synthesis, and exact 5-bit linear-circuit cost over GF(2).**

This repository is a compact research artifact for turning a class of GPU LDS/shared-memory bank-conflict problems into an exact finite algebra problem. It is intentionally narrower than a full GPU performance model and broader than a single hand-written XOR swizzle.

The core pipeline is:

```text
affine access extraction
        ↓
hardware bank projection
        ↓
per-pattern exact rank ceiling
        ↓
shared-Q exact synthesis
        ↓
SAT / MUS-style infeasibility diagnosis
        ↓
exact 5-bit linear-circuit gate minimization
        ↓
compiler / ISA / hardware validation
```

## Positioning

This project is **not**:

- a claim that XOR swizzling itself is new;
- a universal fixed swizzle for every access pattern;
- a cycle-accurate LDS contention simulator;
- a proof that one current CK swizzle is globally optimal for all kernels;
- a replacement for final hardware measurement.

This project **is**:

- an exact GF(2) model for affine access families;
- a closed-form characterization of each pattern's best achievable bank rank under a hardware projection;
- an exact finite-space simultaneous synthesizer for `n=5`;
- an independent Grassmann-subspace rank verifier;
- an inclusion-minimal infeasible-subset extractor;
- an exact minimum-gate synthesizer for a declared 5-bit `{SHL, SHR, AND, XOR}` circuit model;
- an end-to-end CK/gfx1100 case study deriving the effective bank projection from LDS byte addresses.

The intended audience is GPU kernel/compiler engineers and researchers working on LDS/shared-memory layout transforms, swizzles, tensor descriptors, address generation, and low-level scheduling.

## Current frozen scope

The open-source baseline intentionally freezes the first complete proof-and-verification chain at:

- field: `GF(2)`;
- coordinate width: `n = 5`;
- effective bank label width: 5 bits;
- exact `Q`-space enumeration: `2^25 = 33,554,432`;
- exact Grassmann rank verification over all 374 subspaces of `GF(2)^5`;
- exact inclusion-minimal MUS extraction by deletion;
- exact linear-circuit BFS through the declared gate model;
- CK diagonal / anti-diagonal / stride-2 case study.

Future work such as real CK access extraction, ISA-aware lowering cost, workload-weighted synthesis, broadcast-aware arbitration, and non-affine bit-vector analysis should be developed as separate follow-on layers rather than silently folded into the current theorem boundary.

---

## Repository map

```text
.
├── README.md
├── LICENSE
├── CONTRIBUTING.md
├── CMakeLists.txt
├── src/
│   ├── exact_synth.cpp
│   └── exact_circuit.cpp
├── theory/
│   ├── README.md
│   ├── 01_model.md
│   ├── 02_single_access.md
│   ├── 03_hardware_projection.md
│   ├── 04_multi_access.md
│   ├── 05_exact_synthesis_and_mus.md
│   ├── 06_exact_linear_circuit.md
│   ├── 07_ck_case_study.md
│   └── 08_limitations.md
├── results/
│   ├── REFERENCE_RESULTS.txt
│   └── REPRODUCIBILITY.md
├── scripts/
│   ├── build.sh
│   └── validate.sh
└── .github/workflows/
    └── ci.yml
```

The theory index is [`theory/README.md`](theory/README.md).

---

# Quick start

## Option A — direct GCC build

```bash
g++ -O3 -std=c++17 -march=native src/exact_synth.cpp -o exact_synth
g++ -O3 -std=c++17 -march=native src/exact_circuit.cpp -o exact_circuit
```

## Option B — direct Clang build

```bash
clang++ -O3 -std=c++17 -march=native src/exact_synth.cpp -o exact_synth
clang++ -O3 -std=c++17 -march=native src/exact_circuit.cpp -o exact_circuit
```

## Option C — CMake

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Run CTest:

```bash
ctest --test-dir build --output-on-failure
```

## Option D — provided scripts

```bash
./scripts/build.sh
./scripts/validate.sh
```

---

# Core execution commands

If you used the direct compiler commands above:

```bash
./exact_synth --self-test
./exact_synth --case-study
./exact_synth --solve-ck
./exact_synth --count-ck
./exact_synth --mus-demo

./exact_circuit --self-test
./exact_circuit --demo
```

If you used CMake:

```bash
./build/exact_synth --self-test
./build/exact_synth --case-study
./build/exact_synth --solve-ck
./build/exact_synth --count-ck
./build/exact_synth --mus-demo

./build/exact_circuit --self-test
./build/exact_circuit --demo
```

The exploratory v2 structured-corpus analysis is still retained for reproducibility:

```bash
./exact_synth --robustness-ck
```

It is **non-normative**: the synthetic corpus is not treated as a real CK workload prior.

---

# Reproduced core results

The independent local reference run is recorded in [`results/REFERENCE_RESULTS.txt`](results/REFERENCE_RESULTS.txt).

Key invariants are:

```text
Grassmann GF(2)^5 counts:
1, 31, 155, 155, 31, 1

Gaussian vs Grassmann:
PASS

rank(P_star):
4

rank(I + P_star):
5

n=2 hardware rank-ceiling exhaustive verifier:
65,536 hardware/access configurations
1,048,576 P evaluations
0 failures

complete CK Q-space:
33,554,432 candidates
2,887,680 simultaneous rank-optimal solutions

six-pattern MUS:
core size = 6
removing any one member => SAT witness

exact circuit state counts:
gates 0:       1
gates 1:      38
gates 2:     915
gates 3:  17,942
gates 4: 331,291

exact circuit costs:
cost(R4) = 1
cost(P_star) = 4
cost(I ^ L1 ^ R4) = 4
```

For the v2 strongest structured-corpus witness, exhaustive circuit search through four gates does not find a realization, certifying:

\[
\mathrm{cost}\ge 5
\]

under the declared circuit model.

---

# Formal theory at a glance

For access pattern \(i\),

\[
r_i(t)=R_it+r_{0,i},\qquad c_i(t)=C_it+c_{0,i},
\]

with hardware bank projection

\[
h(r,c)=B_rr+B_cc+b_0.
\]

Apply the affine shear

\[
S_P(r,c)=(r,c+Pr).
\]

The effective bank operator is

\[
\boxed{
M_i(P)=B_rR_i+B_cC_i+B_cPR_i.
}
\]

Its rank exactly determines the number of reachable linear bank labels:

\[
|\operatorname{Im}M_i(P)|=2^{\operatorname{rank}M_i(P)}.
\]

The exact per-pattern rank ceiling is:

\[
\boxed{
\rho_i^\*
=
\min\left\{
\operatorname{rank}[B_rR_i\ \ B_c],
\operatorname{rank}
\begin{bmatrix}
R_i\\
B_cC_i
\end{bmatrix}
\right\}.
}
\]

Bank behavior depends on \(P\) only through

\[
Q=B_cP.
\]

For a family of accesses, if

\[
U=\sum_i\operatorname{Im}R_i,\qquad S=\operatorname{Im}B_c,
\]

then the intrinsic synthesis variable is

\[
Q|_U\in\operatorname{Hom}(U,S).
\]

The exact simultaneous synthesis problem is therefore:

\[
\boxed{
\text{find }Q
\text{ such that }
\operatorname{rank}(A_i+QR_i)=\rho_i^\*
\quad\forall i,
}
\]

where

\[
A_i=B_rR_i+B_cC_i.
\]

The complete derivation, proof sketches, verification strategy, and scope boundaries are split into the linked theory notes below.

---

# Theory documents

| Document | Purpose |
|---|---|
| [`theory/01_model.md`](theory/01_model.md) | affine access, bank projection, affine shear, effective bank operator |
| [`theory/02_single_access.md`](theory/02_single_access.md) | exact collision law, canonical single-access theorem, Grassmann verifier |
| [`theory/03_hardware_projection.md`](theory/03_hardware_projection.md) | hardware-nullspace formulation, exact hardware rank ceiling, quotient reduction |
| [`theory/04_multi_access.md`](theory/04_multi_access.md) | simultaneous feasibility, graph/nullspace intersection, higher-order incompatibility |
| [`theory/05_exact_synthesis_and_mus.md`](theory/05_exact_synthesis_and_mus.md) | exact `Q` enumeration, certificates, MUS extraction, compromise objectives |
| [`theory/06_exact_linear_circuit.md`](theory/06_exact_linear_circuit.md) | exact circuit state space, BFS optimality, cost semantics |
| [`theory/07_ck_case_study.md`](theory/07_ck_case_study.md) | gfx1100/RDNA3 CK diagonal, anti-diagonal, stride-2 end-to-end case |
| [`theory/08_limitations.md`](theory/08_limitations.md) | non-affine addressing, broadcast, cycle-model boundary, research claims |

---

# CK case-study result

For the declared row-major `32 x 32` DWORD tile model,

\[
a(r,c')=4(32r+c').
\]

Within the analyzed effective 32-bank side,

\[
bank_{32}=(a/4)\bmod32=c',
\]

so

\[
B_r=0,\qquad B_c=I.
\]

The original CK-style transform uses \(P=I\). For diagonal and anti-diagonal accesses, the linear effective map is

\[
I+I=0,
\]

giving rank 0 in the model.

The proposed patch uses

```cpp
(r ^ (r >> 2) ^ (r << 3)) & 0x1F
```

with matrix \(P_\*\) satisfying

\[
\operatorname{rank}P_\*=4,
\qquad
\operatorname{rank}(I+P_\*)=5.
\]

The full coordinate shear remains invertible even though \(P_\*\) itself is singular.

The CPU byte-address enumerator and GF(2) rank model agree exactly for the three included patterns.

See [`theory/07_ck_case_study.md`](theory/07_ck_case_study.md).

---

# Exact linear-circuit result

`exact_circuit` solves the minimum-gate problem exactly under this finite model:

```text
SHL_k(v) = ((v << k) & 0x1F), k=1..4      cost 1
SHR_k(v) = v >> k,              k=1..4      cost 1
AND_m(v) = v & m,               m=1..30     cost 1
XOR(a,b) = a ^ b                              cost 1
input x                                         free
```

A search state is the canonical set of all distinct linear values currently available. Breadth-first search is therefore exact by gate count.

For the current CK map:

```cpp
v1 = (x << 3) & 0x1F;
v2 = x >> 2;
v3 = v1 ^ v2;
v4 = v3 ^ x;
```

the exact minimum is:

\[
\boxed{\mathrm{cost}(P_\*)=4}.
\]

This is an exact abstract gate count, **not** a claim of four GPU cycles.

See [`theory/06_exact_linear_circuit.md`](theory/06_exact_linear_circuit.md).

---

# Reproducibility

The full validation sequence is documented in [`results/REPRODUCIBILITY.md`](results/REPRODUCIBILITY.md).

Fast core validation:

```bash
./scripts/validate.sh
```

The script builds from source, runs both self-tests, the CK case study, full `2^25` Q-space count, MUS extraction, and exact circuit demo.

---

# References and upstream context

The first end-to-end application is CK-Tile / ROCm LDS coordinate transformation.

Relevant public material:

- ROCm Composable Kernel documentation: LDS bank conflicts and CK-Tile hardware concepts.
- AMD RDNA3 Shader Instruction Set Architecture: Data Share / LDS organization.
- ROCm pull request `ROCm/rocm-libraries#11594`.
- ROCm issue `ROCm/rocm-libraries#11597`.

These references motivate and validate the first hardware case study. The theory itself is intentionally presented independently of one framework.

---

# License

Apache-2.0. See [`LICENSE`](LICENSE).
