# Theory Index

The theory is split into small documents so that the algebraic claims, implementation claims, and hardware assumptions can be reviewed independently.

## Reading order

1. [`01_model.md`](01_model.md) — notation and the affine-shear operator.
2. [`02_single_access.md`](02_single_access.md) — rank means exact bank-image cardinality.
3. [`03_hardware_projection.md`](03_hardware_projection.md) — real hardware bank projection and the exact rank ceiling.
4. [`04_multi_access.md`](04_multi_access.md) — one shared shear across multiple patterns.
5. [`05_exact_synthesis_and_mus.md`](05_exact_synthesis_and_mus.md) — finite exact synthesis and infeasibility certificates.
6. [`06_exact_linear_circuit.md`](06_exact_linear_circuit.md) — exact implementation-cost synthesis in the declared 5-bit gate model.
7. [`07_ck_case_study.md`](07_ck_case_study.md) — the first end-to-end CK/gfx1100 case study.
8. [`08_limitations.md`](08_limitations.md) — what the current proof chain does not claim.

## Formal core

The current release centers on eight named formal objects/results.

1. **Definition 1 — Affine access and hardware bank projection**
2. **Definition 2 — Affine shear and effective bank operator**
3. **Theorem 1 — Exact linear collision law**
4. **Lemma 1 — Hardware-nullspace intersection identity**
5. **Theorem 2 — Exact hardware rank ceiling**
6. **Theorem 3 — Projection quotient and active-subspace reduction**
7. **Lemma 2 — Grassmann rank criterion**
8. **Definition 3 — Simultaneous rank-optimal synthesis and MUS certificate**

Several corollaries and constructions are documented around those eight items, but the open-source baseline is deliberately organized around this compact chain.
