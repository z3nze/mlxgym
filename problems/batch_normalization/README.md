# Per-channel batch normalization

Contract: `Population mean/variance over row-major N×C, then gamma and beta`.

Implement the GPU algorithm in `kernel.metal`. `solution.mm` contains only the
default dispatch; edit it if an optimized solution needs multiple Metal kernel
passes. No CUDA implementation is included in this repository.

## Correctness coverage

N=1, C=1, constant channels, varied gamma/beta, and non-workgroup multiples. Every GPU buffer has guard regions, output starts with a sentinel, and
randomized patterns use a fixed seed.

## Benchmarks

1024×256, 4096×768, and 8192×1024; elements/s. A benchmark runs only after the complete correctness suite passes.
