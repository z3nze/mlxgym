# In-place RGBA color inversion

Contract: `Invert RGB bytes and preserve alpha`.

Implement the GPU algorithm in `kernel.metal`. `solution.mm` contains only the
default dispatch; edit it if an optimized solution needs multiple Metal kernel
passes. No CUDA implementation is included in this repository.

## Correctness coverage

Pure and patterned colors, exact alpha preservation, odd image dimensions. Every GPU buffer has guard regions, output starts with a sentinel, and
randomized patterns use a fixed seed.

## Benchmarks

1080p, 4K, and odd-width images; pixels/s and GB/s. A benchmark runs only after the complete correctness suite passes.
