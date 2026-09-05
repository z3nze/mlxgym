# Square matrix addition

Contract: `C = A + B`.

Implement the GPU algorithm in `kernel.metal`. `solution.mm` contains only the
default dispatch; edit it if an optimized solution needs multiple Metal kernel
passes. No CUDA implementation is included in this repository.

## Correctness coverage

Mixed magnitudes, cancellation, and dimensions around common tiles. Every GPU buffer has guard regions, output starts with a sentinel, and
randomized patterns use a fixed seed.

## Benchmarks

N=512, 2048, and 4096; effective GB/s. A benchmark runs only after the complete correctness suite passes.
