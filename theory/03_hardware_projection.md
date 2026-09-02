# 3. Hardware Projection and Exact Rank Ceiling

The canonical formula \(C+PR\) is only a special case. Real hardware may derive banks from both logical coordinates.

## Hardware selector

Let

\[
H=[B_r\ B_c].
\]

Let the joint access map be

\[
J_i=
\begin{bmatrix}
R_i\\
C_i
\end{bmatrix}.
\]

Then after the affine shear,

\[
M_i(P)=HS_PJ_i.
\]

## Lemma 1 — Hardware-nullspace intersection identity

Define

\[
L_i=\operatorname{Im}J_i
\]

and

\[
N_P=\ker(HS_P).
\]

Then

\[
\boxed{
\dim\ker M_i(P)
=
\dim\ker J_i
+
\dim(L_i\cap N_P).
}
\]

### Proof

Because

\[
\ker M_i(P)=J_i^{-1}(N_P),
\]

the standard dimension formula for the preimage of a subspace gives

\[
\dim J_i^{-1}(N_P)
=
\dim\ker J_i
+
\dim(\operatorname{Im}J_i\cap N_P).
\]

## Canonical graph special case

For

\[
H=[0\ I],
\]

the post-shear hardware nullspace is the graph

\[
\Gamma_P=\{(r,Pr):r\in V\}.
\]

Thus

\[
\dim\ker(C_i+PR_i)
=
\dim\ker J_i
+
\dim(L_i\cap\Gamma_P).
\]

This separates:

- intrinsic information loss already present in the joint access map;
- avoidable loss caused by how the selected shear intersects the access geometry.

## Theorem 2 — Exact hardware rank ceiling

For one pattern,

\[
M(P)=A+B_cPR,
\qquad
A=B_rR+B_cC.
\]

Then

\[
\boxed{
\max_P\operatorname{rank}M(P)
=
\min\left\{
\operatorname{rank}[B_rR\ \ B_c],
\operatorname{rank}
\begin{bmatrix}
R\\
B_cC
\end{bmatrix}
\right\}.
}
\]

Call this value

\[
\rho^\*.
\]

### Interpretation

The first term is an output-capacity bound:

\[
\operatorname{rank}[B_rR\ \ B_c].
\]

The second term is a visible-input-information bound:

\[
\operatorname{rank}
\begin{bmatrix}
R\\
B_cC
\end{bmatrix}.
\]

The exact best achievable rank is the minimum of the two.

### Verification

The program exhaustively checks the theorem at `n=2`:

```text
hardware/access quadruples: 65,536
P evaluations:            1,048,576
theorem failures:                 0
```

The exhaustive checker is independent of the closed-form formula.

## Theorem 3 — Projection quotient and active-subspace reduction

Bank behavior depends on \(P\) only through

\[
\boxed{
Q=B_cP.
}
\]

Let

\[
S=\operatorname{Im}B_c.
\]

Every achievable \(Q\) is a linear map

\[
Q:V\to S.
\]

For a family of accesses define

\[
U=\sum_i\operatorname{Im}R_i.
\]

Only the restriction

\[
Q|_U:U\to S
\]

can affect the family.

Therefore the intrinsic synthesis variable lives in

\[
\boxed{
\operatorname{Hom}(U,S).
}
\]

If

\[
u=\dim U,\qquad s=\dim S,
\]

the search space contains exactly

\[
\boxed{
2^{us}
}
\]

linear maps.

The current CK case has \(u=s=5\), so the exact space is

\[
2^{25}=33,554,432.
\]

## Gauge freedom

Two implementation matrices \(P_1,P_2\) are bank-equivalent for the modeled access family when

\[
B_c(P_1-P_2)R_i=0
\]

for every pattern.

Thus algebraic synthesis should solve for the quotient variable \(Q\) first. A later implementation layer can use the remaining gauge freedom to minimize actual hardware cost.
