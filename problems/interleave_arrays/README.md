# Array interleaving

Contract: `[A0,B0,A1,B1,…]`.

Implement the GPU algorithm in `kernel.metal`. `solution.mm` contains only the
default dispatch; edit it if an optimized solution needs multiple Metal kernel
passes. No CUDA implementation is included in this repository.

## Correctness coverage

Exact ordering, distinct patterns, guard regions, and boundary lengths. Every GPU buffer has guard regions, output starts with a sentinel, and
randomized patterns use a fixed seed.

## Benchmarks

1M and 16M input pairs; effective GB/s. A benchmark runs only after the complete correctness suite passes.
