# MIP Feature Computation Code

Dependencies:

- BLAS/LAPACK
- CoinOR development libraries
- igraph C library

Building and testing:

- Build: `make`
- Quick test: `make test`

Using the code:

- Build steps create a binary bin/evaluate
- Expects MIPs in MPS format
- Compute features of one instance using: `bin/evaluate model.mps`
