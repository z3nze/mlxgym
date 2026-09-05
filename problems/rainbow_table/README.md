# Repeated 32-bit FNV-1a hashing

Contract: `Apply FNV-1a R times to every integer`.

Implement the GPU algorithm in `kernel.metal`. `solution.mm` contains only the
default dispatch; edit it if an optimized solution needs multiple Metal kernel
passes. No CUDA implementation is included in this repository.

## Correctness coverage

Zero/one/many rounds, signed integer extremes, and boundary lengths. Every GPU buffer has guard regions, output starts with a sentinel, and
randomized patterns use a fixed seed.

## Benchmarks

1M values with R=1, 100, and 1000; hashes/s. A benchmark runs only after the complete correctness suite passes.
