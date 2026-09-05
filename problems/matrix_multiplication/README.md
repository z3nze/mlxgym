# Row-major matrix multiplication

Contract: `C[M,K] = A[M,N] × B[N,K]`.

Implement the GPU algorithm in `kernel.metal`. `solution.mm` contains only the
default dispatch; edit it if an optimized solution needs multiple Metal kernel
passes. No CUDA implementation is included in this repository.

## Correctness coverage

Degenerate, rectangular, prime, and tile-boundary shapes with mixed-sign inputs. Every GPU buffer has guard regions, output starts with a sentinel, and
randomized patterns use a fixed seed.

## Benchmarks

256³, 512³, 1024³, and rectangular shapes; GFLOP/s. A benchmark runs only after the complete correctness suite passes.
