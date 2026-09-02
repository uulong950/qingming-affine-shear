# 6. Exact 5-bit Linear-Circuit Cost

The algebraic solver can leave millions of valid shears. A second question is implementation cost.

The current release solves one precise abstract cost model exactly.

## Gate model

Every intermediate is a 5-bit GF(2)-linear transform of the original input \(x\).

Allowed gates are:

\[
\operatorname{SHL}_k(v)=((v\ll k)\ \&\ 0x1F),
\quad
k=1,\dots,4,
\]

\[
\operatorname{SHR}_k(v)=v\gg k,
\quad
k=1,\dots,4,
\]

\[
\operatorname{AND}_m(v)=v\ \&\ m,
\quad
m=1,\dots,30,
\]

and

\[
\operatorname{XOR}(a,b)=a\oplus b.
\]

Every gate has unit cost. Input `x` is free.

The search excludes operations that produce zero or a value already available because such a gate cannot improve a minimum-gate realization of a nonzero target under this model.

## Canonical search state

A state is the sorted set of all distinct linear values currently available.

Two different programs that reach the same set can be merged because all legal future operations depend only on the values available, not on how they were produced.

Breadth-first search therefore explores straight-line circuits in nondecreasing gate count.

## Exactness statement

If a target first appears at depth \(d\),

\[
\boxed{
\mathrm{cost}(T)=d
}
\]

under the declared gate model.

If every state through depth \(d\) is exhausted without the target,

\[
\boxed{
\mathrm{cost}(T)\ge d+1.
}
\]

This is an exact finite proof, not a heuristic synthesis result.

## State-space invariant

The reproduced canonical state counts are:

```text
gates 0:       1
gates 1:      38
gates 2:     915
gates 3:  17,942
gates 4: 331,291
```

These counts are checked by `exact_circuit --self-test`.

## Current CK matrix

The patch matrix is

\[
P_\*=I+L_3+R_2.
\]

An exact witness is:

```cpp
v1 = (x << 3) & 0x1F;
v2 = x >> 2;
v3 = v1 ^ v2;
v4 = v3 ^ x;
```

The search proves no circuit with fewer than four gates exists, so:

\[
\boxed{
\mathrm{cost}(P_\*)=4.
}
\]

The witness has dependency depth 3.

## Negative control

For

\[
R_4:x\mapsto x\gg4,
\]

the exact minimum is

\[
\mathrm{cost}(R_4)=1.
\]

For the v2 alternative

\[
I+L_1+R_4,
\]

the exact minimum is also

\[
4.
\]

For the v2 strongest structured-corpus matrix, exhaustive search through four gates does not find a realization, certifying

\[
\mathrm{cost}\ge5.
\]

## What this cost is not

This is not exact ISA latency.

A compiler may alter instruction selection, fold masks, change bit width, exploit address-generation hardware, or schedule independent operations differently.

The exact circuit solver answers a narrower question:

> what is the minimum number of gates in this explicitly declared finite 5-bit linear-circuit language?

That exact answer can then be compared with generated ISA in a later hardware layer.
