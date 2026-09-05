# Valid 2D cross-correlation

Contract: `Stride one, no padding`.

Implement the GPU algorithm in `kernel.metal`. `solution.mm` contains only the
default dispatch; edit it if an optimized solution needs multiple Metal kernel
passes. No CUDA implementation is included in this repository.

## Correctness coverage

1×1/full-image kernels, rectangular kernels, and odd output dimensions. Every GPU buffer has guard regions, output starts with a sentinel, and
randomized patterns use a fixed seed.

## Benchmarks

512² and 1024² inputs with 3×3, 5×5, and 7×7 kernels; GFLOP/s. A benchmark runs only after the complete correctness suite passes.
