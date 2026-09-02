# 2. Single-Access Rank Theory

This document explains why matrix rank exactly captures structural bank multiplicity in the affine model.

## Theorem 1 — Exact linear collision law

Let

\[
M:V\to V
\]

be the effective lane-to-bank linear map and let

\[
\rho=\operatorname{rank}M.
\]

Then the number of reachable linear bank labels is exactly

\[
\boxed{
|\operatorname{Im}M|=2^\rho.
}
\]

Every reachable bank label has exactly

\[
\boxed{
2^{5-\rho}
}
\]

lane preimages.

Equivalently,

\[
\boxed{
K(M)=2^{\operatorname{nullity}M}.
}
\]

### Proof

Rank-nullity gives

\[
\dim\ker M=5-\rho.
\]

A \(d\)-dimensional binary vector space has \(2^d\) elements, so

\[
|\ker M|=2^{5-\rho}.
\]

Each non-empty fiber of a linear map is a coset of the kernel, hence all fibers have this size. The image has

\[
2^5 / 2^{5-\rho}=2^\rho
\]

elements.

## Interpretation table

| rank | reachable bank labels | structural multiplicity |
|---:|---:|---:|
| 5 | 32 | 1 |
| 4 | 16 | 2 |
| 3 | 8 | 4 |
| 2 | 4 | 8 |
| 1 | 2 | 16 |
| 0 | 1 | 32 |

This is a structural statement. It is not a claim that a rank-0 access is exactly 32 times slower than a rank-5 access.

## Canonical single-access form

If the analyzed hardware projection is

\[
B_r=0,\qquad B_c=I,
\]

then

\[
M(P)=C+PR.
\]

If \(R\) is invertible, choose any target

\[
Q\in GL(5,2)
\]

and set

\[
\boxed{
P=(C+Q)R^{-1}.
}
\]

Then

\[
C+PR=Q
\]

and the bank map has full rank.

This is a constructive existence result: the conflict-free feasibility subproblem does not require empirical swizzle search when the affine model applies and \(R\) is invertible.

## Singular-\(R\) canonical theorem

For the canonical model

\[
M(P)=C+PR,
\]

the best possible rank satisfies

\[
\boxed{
\max_P\operatorname{rank}(C+PR)
=
\operatorname{rank}
\begin{bmatrix}
R\\
C
\end{bmatrix}.
}
\]

Equivalently, if

\[
D=\ker R\cap\ker C,
\]

then

\[
\boxed{
\max_P\operatorname{rank}(C+PR)=5-\dim D.
}
\]

The only unavoidable lane directions are those invisible to both \(R\) and \(C\).

The implementation separately exhaustively verified the corresponding more general hardware theorem for every `n=2` hardware/access configuration.

## Lemma 2 — Grassmann rank criterion

Let the required rank be \(\rho\), and define

\[
k=5-\rho+1.
\]

Then

\[
\boxed{
\operatorname{rank}M\ge\rho
}
\]

if and only if no \(k\)-dimensional subspace of \(V\) is contained in \(\ker M\).

### Proof

\[
\operatorname{rank}M\ge\rho
\iff
\dim\ker M\le 5-\rho.
\]

A kernel has dimension at least \(k\) exactly when it contains a \(k\)-dimensional subspace.

## Exhaustive Grassmann verifier

The implementation constructs every subspace of \(\mathbb F_2^5\).

Counts by dimension are:

```text
dim 0:   1
dim 1:  31
dim 2: 155
dim 3: 155
dim 4:  31
dim 5:   1
total:  374
```

`exact_synth --self-test` checks Gaussian elimination against this independent subspace criterion.
