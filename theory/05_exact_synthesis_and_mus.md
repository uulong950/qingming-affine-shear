# 5. Exact Synthesis and Infeasibility Certificates

## Definition 3 — Simultaneous rank-optimal synthesis

For pattern \(i\), compute

\[
A_i=B_rR_i+B_cC_i
\]

and its exact hardware ceiling

\[
\rho_i^\*.
\]

The simultaneous synthesis problem is:

\[
\boxed{
\text{find }Q\in\operatorname{Hom}(U,S)
\text{ such that }
\operatorname{rank}(A_i+QR_i)=\rho_i^\*
\quad\forall i.
}
\]

For the current `n=5`, full-domain CK case,

\[
|\operatorname{Hom}(V,V)|=2^{25}.
\]

The executable can enumerate the complete space.

## Exact enumeration

`exact_synth --solve-ck` stops at the first simultaneous rank-optimal witness.

`exact_synth --count-ck` exhausts all

\[
33,554,432
\]

candidates.

For diagonal + anti-diagonal + stride-2, the reproduced solution count is

\[
\boxed{
2,887,680.
}
\]

This is a useful negative result as well as a positive one: the three access constraints certify the current patch as rank-optimal for those patterns, but they do not uniquely derive that specific matrix.

## Grassmann verification

The fast solver uses Gaussian rank. A returned witness is independently checked by the Grassmann-subspace criterion described in [`02_single_access.md`](02_single_access.md).

This separates candidate search from witness verification.

## Inclusion-minimal infeasible subset

For each pattern define its optimal solution set

\[
\mathcal S_i^\*
=
\{Q:\operatorname{rank}(A_i+QR_i)=\rho_i^\*\}.
\]

A pattern index set \(I\) is an inclusion-minimal infeasible subset if

\[
\bigcap_{i\in I}\mathcal S_i^\*=\varnothing
\]

but for every \(j\in I\),

\[
\bigcap_{i\in I\setminus\{j\}}\mathcal S_i^\*\neq\varnothing.
\]

The current implementation uses exact deletion-based shrinking:

1. prove the full family infeasible;
2. remove one pattern;
3. keep it removed only if infeasibility remains;
4. continue until every remaining pattern is necessary;
5. for every remaining member, solve the family with that member removed and emit a SAT witness.

This proves **inclusion minimality**. It does not claim globally minimum cardinality.

## Reproduced six-pattern MUS

The `n=5` construction

\[
A_0=0,\qquad A_j=e_0f_j
\]

for \(j=0,\dots,4\) produces a six-pattern core.

The executable reproduces:

```text
MUS core size: 6
remove any one member: SAT
```

and emits a concrete witness `Q` for every deletion.

## SAT encoding path

The current release enumerates `Q` directly because `2^25` is small.

For larger spaces, the same rank constraints admit an exact Boolean encoding.

To require

\[
\operatorname{rank}M\ge\rho,
\]

let

\[
k=5-\rho+1.
\]

For every \(k\)-dimensional subspace \(D\), require that \(D\) is not contained in the kernel. If \(u_1,\dots,u_k\) is a basis of \(D\),

\[
(Mu_1\ne0)\vee\cdots\vee(Mu_k\ne0).
\]

Each output bit is an affine XOR expression in the bits of \(Q\), so this can be Tseitin-encoded to CNF or handled by an XOR-aware SAT solver.

The exact finite problem therefore has a clean path beyond brute-force enumeration without changing its semantics.
