# Element-wise vector addition

Contract: `a[i] + b[i]`.

Implement the GPU algorithm in `kernel.metal`. `solution.mm` contains only the
default dispatch; edit it if an optimized solution needs multiple Metal kernel
passes. No CUDA implementation is included in this repository.

## Correctness coverage

Boundary lengths, mixed signs, deterministic random values, and guarded output. Every GPU buffer has guard regions, output starts with a sentinel, and
randomized patterns use a fixed seed.

## Benchmarks

4K, 1M, and 16M elements; effective GB/s. A benchmark runs only after the complete correctness suite passes.
