# 1. Algebraic Model

## 1.1 Field and dimensions

The formal core works over

\[
\mathbb F_2.
\]

The implementation currently fixes

\[
V=\mathbb F_2^5.
\]

A five-bit vector is encoded as an integer in `[0,31]`. A `5 x 5` binary matrix is stored by columns.

Affine constants matter for concrete addresses, but they do not affect the rank of the linear lane-to-bank map.

## Definition 1 — Affine access and hardware bank projection

For access pattern \(i\), parameterize lanes by

\[
t\in V.
\]

Logical coordinates are

\[
r_i(t)=R_it+r_{0,i},
\qquad
c_i(t)=C_it+c_{0,i}.
\]

The bank-label function is affine on the analyzed region:

\[
h(r,c)=B_rr+B_cc+b_0.
\]

The linear maps are

\[
R_i,C_i,B_r,B_c\in\mathbb F_2^{5\times5}.
\]

The constants \(r_{0,i},c_{0,i},b_0\) translate labels but do not alter rank or fiber multiplicity.

## Definition 2 — Affine shear and effective bank operator

For

\[
P\in\mathbb F_2^{5\times5},
\]

define

\[
S_P(r,c)=(r,c+Pr).
\]

Its block matrix is

\[
S_P=
\begin{bmatrix}
I&0\\
P&I
\end{bmatrix}.
\]

Because the field has characteristic two,

\[
S_P^2=I.
\]

Therefore every full coordinate shear \(S_P\) is invertible, even when \(P\) itself is singular.

After the shear, pattern \(i\) has linear bank operator

\[
\boxed{
M_i(P)
=
B_rR_i+B_cC_i+B_cPR_i.
}
\]

Define

\[
A_i=B_rR_i+B_cC_i.
\]

Then

\[
M_i(P)=A_i+B_cPR_i.
\]

## Why "affine shear"

The block transform is always an invertible unipotent shear. It should not be called symplectic without specifying a preserved non-degenerate alternating form and proving preservation.

For the canonical symplectic form on \(V\oplus V\), the lower shear

\[
\begin{bmatrix}
I&0\\
P&I
\end{bmatrix}
\]

is symplectic only under an additional symmetry condition on \(P\). The current CK matrix is not assumed to satisfy that condition.

The open-source theory therefore uses the stronger accurate term **affine shear** rather than attaching a symplectic claim that is not needed by the bank-conflict theorem.
