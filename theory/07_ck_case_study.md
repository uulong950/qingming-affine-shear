# 7. CK / gfx1100 End-to-End Case Study

This is the first concrete end-to-end application of the theory.

## Hardware/model boundary

AMD RDNA3 LDS is physically organized more richly than a single abstract 32-bank array. The case study therefore analyzes an **effective 32-bank side** for the declared DWORD tile/address region.

It does not claim to be a complete cycle-level model of every DS instruction or every wave phase.

## Byte-address derivation

Consider a row-major `32 x 32` tile of DWORDs.

After the coordinate transform, let the lower coordinate be

\[
(r,c').
\]

Use relative byte address

\[
\boxed{
a(r,c')=4(32r+c').
}
\]

Within the analyzed 32-bank side, take the low five DWORD-address bits:

\[
\beta(r,c')
=
\left(\frac{a(r,c')}{4}\right)\bmod32.
\]

Then

\[
\beta(r,c')
=
(32r+c')\bmod32
=
c'.
\]

A fixed tile-base bank offset would only translate bank labels and does not change collision multiplicity.

Thus:

\[
\boxed{
B_r=0,\qquad B_c=I.
}
\]

The CPU case-study code independently enumerates lane byte addresses and checks that the observed bank histogram agrees with the rank prediction.

## Original CK-style transform

The original low-five-bit transform is

\[
c'=c+r,
\]

so

\[
P_{\rm CK}=I.
\]

### Diagonal

\[
r=t,\qquad c=t.
\]

Therefore

\[
R=I,\qquad C=I
\]

and

\[
M=I+I=0.
\]

Predicted:

```text
rank = 0
reachable banks = 1
structural multiplicity = 32
```

### Anti-diagonal

On five bits,

\[
31-r=31\oplus r.
\]

The affine constant changes labels but the linear part remains

\[
R=I,\qquad C=I.
\]

Therefore the rank result is the same as diagonal.

### Stride-2

Let \(L_1\) be the five-bit zero-fill left shift:

\[
c=2t\bmod32=L_1t.
\]

Then

\[
M=L_1+I
\]

has rank 5.

Thus the original transform is already conflict-free for this stride-2 pattern in the declared model.

## Proposed patch matrix

The tested mix is:

```cpp
(r ^ (r >> 2) ^ (r << 3)) & 0x1F
```

Its matrix is

\[
P_\*=
\begin{bmatrix}
1&0&1&0&0\\
0&1&0&1&0\\
0&0&1&0&1\\
1&0&0&1&0\\
0&1&0&0&1
\end{bmatrix}.
\]

The exact rank is

\[
\boxed{
\operatorname{rank}P_\*=4.
}
\]

So \(P_\*\) itself is not a permutation of all 32 row labels.

The relevant full coordinate transform is nevertheless invertible because

\[
S_{P_\*}^2=I.
\]

For identity-coupled accesses,

\[
I+P_\*
=
\begin{bmatrix}
0&0&1&0&0\\
0&0&0&1&0\\
0&0&0&0&1\\
1&0&0&0&0\\
0&1&0&0&0
\end{bmatrix},
\]

which is a five-bit cyclic bit permutation and has rank 5.

## Reproduced result

```text
diagonal:
  original rank 0, banks 1,  multiplicity 32
  proposed rank 5, banks 32, multiplicity 1

anti-diagonal:
  original rank 0, banks 1,  multiplicity 32
  proposed rank 5, banks 32, multiplicity 1

stride-2:
  original rank 5, banks 32, multiplicity 1
  proposed rank 5, banks 32, multiplicity 1
```

The byte-address enumerator and the GF(2) rank prediction agree exactly.

## Non-uniqueness

The three CK constraints do not uniquely select \(P_\*\).

The complete `2^25` search finds

\[
\boxed{
2,887,680
}
\]

simultaneous rank-optimal matrices.

Therefore the correct statement is:

> \(P_\*\) is a certified feasible member of a large simultaneous rank-optimal solution set for the three modeled accesses.

Choosing one member of that set for production requires an additional criterion such as real access distribution, implementation cost, architecture-specific lowering, or measured kernel behavior.

## Reproduction

```bash
./exact_synth --case-study
./exact_synth --count-ck
```

or after CMake build:

```bash
./build/exact_synth --case-study
./build/exact_synth --count-ck
```
