# Contributing

Contributions are welcome when they preserve the repository's exactness boundary.

Good contribution areas include:

- proof corrections or clearer derivations;
- independent GF(2) verifiers;
- additional exact affine hardware projections;
- real access extractors that emit \(R,C,B_r,B_c\);
- exact SAT/SMT encodings;
- circuit-search improvements that preserve the declared semantics;
- compiler/ISA validation as a separate measurement layer.

Please keep three layers distinct in code and documentation:

1. algebraic bank-conflict structure;
2. abstract implementation cost;
3. measured hardware performance.

A performance claim should state its benchmark boundary. A theorem claim should state its algebraic assumptions. A new hardware model should identify its address/bank mapping source and scope.
