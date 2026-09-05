# Row-major matrix transpose

Contract: `output[col,row] = input[row,col]`.

Implement the GPU algorithm in `kernel.metal`. `solution.mm` contains only the
default dispatch; edit it if an optimized solution needs multiple Metal kernel
passes. No CUDA implementation is included in this repository.

## Correctness coverage

Skinny, wide, square, prime, and tile-boundary dimensions. Every GPU buffer has guard regions, output starts with a sentinel, and
randomized patterns use a fixed seed.

## Benchmarks

512², 2048², 4096×1024, and odd rectangles; GB/s. A benchmark runs only after the complete correctness suite passes.
