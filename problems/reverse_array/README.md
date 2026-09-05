# In-place array reversal

Contract: `Swap element i with N-1-i`.

Implement the GPU algorithm in `kernel.metal`. `solution.mm` contains only the
default dispatch; edit it if an optimized solution needs multiple Metal kernel
passes. No CUDA implementation is included in this repository.

## Correctness coverage

Odd/even sizes, exact bit preservation, and workgroup boundaries. Every GPU buffer has guard regions, output starts with a sentinel, and
randomized patterns use a fixed seed.

## Benchmarks

1M and 16M elements; effective GB/s. A benchmark runs only after the complete correctness suite passes.
