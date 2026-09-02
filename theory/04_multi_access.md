# 4. Multi-Access Theory

A production layout is often shared by several access phases. Individual feasibility therefore does not imply family feasibility.

## Canonical graph-intersection identity

For canonical bank selection

\[
B_r=0,\qquad B_c=I,
\]

pattern \(i\) has

\[
M_i(P)=C_i+PR_i.
\]

Define

\[
D_i=\ker R_i\cap\ker C_i,
\qquad
d_i=\dim D_i,
\]

and

\[
L_i=
\operatorname{Im}
\begin{bmatrix}
R_i\\
C_i
\end{bmatrix}.
\]

Then

\[
\boxed{
\dim\ker(C_i+PR_i)
=
d_i+\dim(L_i\cap\Gamma_P).
}
\]

Therefore

\[
\boxed{
\operatorname{rank}(C_i+PR_i)
=
5-d_i-\dim(L_i\cap\Gamma_P).
}
\]

A pattern reaches its individual optimum exactly when

\[
L_i\cap\Gamma_P=\{0\}.
\]

A family is simultaneously rank-optimal exactly when this holds for every pattern.

The condition is **individual intersection avoidance**. Replacing the union of access subspaces with their total span would impose an unnecessarily stronger constraint.

## Invertible-\(R\) normalization

If every \(R_i\) is invertible, define

\[
A_i=C_iR_i^{-1}.
\]

Then

\[
C_i+PR_i=(A_i+P)R_i
\]

and hence

\[
\operatorname{rank}(C_i+PR_i)
=
\operatorname{rank}(A_i+P).
\]

Over `GF(2)`, rank distance is

\[
d_R(X,Y)=\operatorname{rank}(X-Y)
=\operatorname{rank}(X+Y).
\]

Thus the multi-access problem becomes a rank-metric common-antipode problem:

\[
\rho_i(P)=d_R(P,A_i).
\]

The rank metric itself is established mathematics. The GPU-specific contribution is the mapping from access/layout synthesis to this finite rank problem.

## Individual feasibility does not imply simultaneous feasibility

There exist access families where every pattern individually admits a full-rank shear but no single \(P\) satisfies all of them.

For every dimension \(n\), choose nonzero \(u\) and coordinate functionals \(f_1,\dots,f_n\). Define

\[
A_0=0,
\qquad
A_j=uf_j.
\]

Suppose one \(P\) makes every \(P+A_j\) invertible.

Because \(A_0=0\), \(P\) must be invertible. Let

\[
x=P^{-1}u.
\]

Some \(f_j(x)=1\). Then

\[
(P+uf_j)x=u+u=0,
\]

a contradiction.

For `n=5`, this gives the six-pattern MUS used by the executable.

## Pairwise compatibility is insufficient

Multi-access feasibility is a higher-order constraint problem. In small examples, every pair can be jointly feasible while the whole family is infeasible.

Therefore phase grouping is not generally reducible to ordinary graph coloring. The natural combinatorial object is a hypergraph or general constraint system.

## If no common optimum exists

Let each pattern's exact ceiling be \(\rho_i^\*\). Define avoidable rank loss

\[
e_i(Q)=\rho_i^\*-\operatorname{rank}M_i(Q).
\]

A clean algebraic compromise objective is

\[
\boxed{
e^\*=\min_Q\max_i e_i(Q).
}
\]

A complete system may instead retain the Pareto frontier of

\[
(e_1,\dots,e_m)
\]

before applying hardware-specific weights.

Rank loss or collision multiplicity should not be silently converted into an exact cycle model.
