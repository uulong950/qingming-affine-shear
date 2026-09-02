# 8. Limitations and Research Boundary

The value of an exact theorem depends on keeping its assumptions explicit.

## Affine address regions

The GF(2) matrix model is exact when the relevant coordinate-to-bank map is affine over the analyzed bit region.

If a word address is

\[
a(r,c)=A_rr+A_cc+a_0
\]

in the binary-linear sense and bank extraction is a linear bit projection \(E\), then

\[
B_r=EA_r,\qquad B_c=EA_c.
\]

## Integer carries

Ordinary integer address arithmetic can introduce carries, so expressions such as

```text
base + r*stride + c
```

are not automatically globally linear over `GF(2)`.

Possible extensions are:

- partition the address domain into affine regions and treat them as a multi-access family;
- use exact bit-vector reasoning for the non-affine part.

## Non-power-of-two modulo

A transform involving

```cpp
x % L
```

with non-power-of-two \(L\) should not be silently represented as a GF(2) matrix.

It requires a separate exact finite-domain or bit-vector model.

## Broadcast

Lane multiplicity is not always bank conflict. Multiple lanes requesting the same word may be broadcast.

A future structural model should include a word/tag map \(T\) and analyze

\[
G=
\begin{bmatrix}
M\\
T
\end{bmatrix}.
\]

The current release intentionally does not claim broadcast-aware arbitration.

## Wave phases and bank ports

Real LDS instructions can split a wave into instruction-specific phases. Port count, access width, vectorization, scheduler behavior, and overlap affect cycle cost.

Therefore:

\[
2^{\operatorname{nullity}M}
\]

is a structural multiplicity, not a universal latency multiplier.

## Exact circuit cost versus ISA cost

`exact_circuit` proves minimum gate count only in its declared 5-bit language.

It does not prove minimum:

- AMD ISA instruction count;
- NVIDIA SASS instruction count;
- cycles;
- dependency stalls;
- VGPR pressure;
- occupancy impact.

Those require a compiler/ISA validation layer.

## Universal fixed-swizzle impossibility

No fixed XOR-linear shear can make every affine coupling conflict-free.

In the canonical identity-row case,

\[
M=A+P.
\]

For any fixed \(P\), choose

\[
A=P.
\]

Then

\[
M=0.
\]

Therefore a "universal fixed conflict-free swizzle" is impossible in this model.

The correct general object is a **synthesizer conditioned on the access family and hardware projection**.

## Uniform-corpus invariance

Over the complete matrix universe

\[
A\in\operatorname{Mat}_{5\times5}(\mathbb F_2),
\]

the map

\[
A\mapsto A+P
\]

is a bijection for every fixed \(P\).

Therefore every \(P\) has the same rank histogram under a uniform distribution over all matrices.

A meaningful "robustness" preference between feasible shears requires a non-uniform workload/access prior or an independent implementation-cost objective.

This is why the retained `--robustness-ck` synthetic-corpus command is marked exploratory rather than normative.

## Novelty boundary

The repository does not claim invention of:

- XOR swizzles;
- GF(2) linear algebra;
- rank metric;
- matrix-rank optimization in general.

The intended research contribution is the integrated systems formulation:

```text
GPU affine access
→ hardware bank projection
→ exact rank law
→ constructive / exact shared-shear synthesis
→ explicit infeasibility certificate
→ exact small circuit cost
→ hardware case study
```

A formal novelty claim should still be supported by a dedicated related-work review before publication.
