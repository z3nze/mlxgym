# Sigmoid activation

Contract: `1 / (1 + exp(-x))`.

Implement the GPU algorithm in `kernel.metal`. `solution.mm` contains only the
default dispatch; edit it if an optimized solution needs multiple Metal kernel
passes. No CUDA implementation is included in this repository.

## Correctness coverage

Near-zero and ±100 stability cases plus boundary lengths. Every GPU buffer has guard regions, output starts with a sentinel, and
randomized patterns use a fixed seed.

## Benchmarks

1M and 16M elements; elements/s. A benchmark runs only after the complete correctness suite passes.
