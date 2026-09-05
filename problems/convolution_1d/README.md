# Valid 1D cross-correlation

Contract: `output[i] = Σ input[i+j]·kernel[j]`.

Implement the GPU algorithm in `kernel.metal`. `solution.mm` contains only the
default dispatch; edit it if an optimized solution needs multiple Metal kernel
passes. No CUDA implementation is included in this repository.

## Correctness coverage

Kernel sizes 1 and input-size, even/odd kernels, and non-workgroup multiples. Every GPU buffer has guard regions, output starts with a sentinel, and
randomized patterns use a fixed seed.

## Benchmarks

1M inputs with K=3, 7, 31, and 127; GFLOP/s. A benchmark runs only after the complete correctness suite passes.
