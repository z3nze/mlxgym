# Packed RGB to grayscale

Contract: `0.299R + 0.587G + 0.114B`.

Implement the GPU algorithm in `kernel.metal`. `solution.mm` contains only the
default dispatch; edit it if an optimized solution needs multiple Metal kernel
passes. No CUDA implementation is included in this repository.

## Correctness coverage

Pure primary colors, black/white, patterns, and odd dimensions. Every GPU buffer has guard regions, output starts with a sentinel, and
randomized patterns use a fixed seed.

## Benchmarks

1080p, 4K, and odd-width images; pixels/s. A benchmark runs only after the complete correctness suite passes.
