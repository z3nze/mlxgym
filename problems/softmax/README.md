# Numerically stable full-vector softmax

Contract: `exp(x-max(x)) / Σ exp(x-max(x))`.

Implement the GPU algorithm in `kernel.metal`. `solution.mm` contains only the
default dispatch; edit it if an optimized solution needs multiple Metal kernel
passes. No CUDA implementation is included in this repository.

## Correctness coverage

Equal logits, dominant values, large shifts, nonnegative outputs, and sum-to-one. Every GPU buffer has guard regions, output starts with a sentinel, and
randomized patterns use a fixed seed.

## Benchmarks

1K, 1M, and 16M elements; elements/s. A benchmark runs only after the complete correctness suite passes.
