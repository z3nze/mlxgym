# Softmax attention

Contract: `output = softmax(Q × Kᵀ / sqrt(d)) × V`, with softmax applied row-wise.

Inputs are row-major: `Q` has shape M×d, while `K` and `V` have shape N×d. The
output has shape M×d. All dimensions are positive. Implement the GPU algorithm
in `kernel.metal`; `solution.mm` may be changed when an optimized implementation
needs multiple Metal entry points or an intermediate M×N score buffer.

## Correctness coverage

Singleton dimensions, rectangular and odd shapes, SIMD/workgroup boundaries,
large logits requiring stable softmax, input immutability, output sentinels, and
guard regions around every GPU buffer. The reference uses double-precision
accumulation and max-subtracted softmax.

## Benchmarks

M=N=128 with d=64, M=N=512 with d=64, and M=N=1024 with d=128; effective
matrix-multiplication GFLOP/s. A benchmark runs only after the complete
correctness suite passes.
