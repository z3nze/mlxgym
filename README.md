# mlxgym

A local Apple-silicon GPU practice gym for writing native Metal compute kernels.

## Requirements

- An Apple-silicon Mac with Xcode.
- Xcode's optional Metal Toolchain component.
- CMake 3.24 or newer and standard Make. Ninja is not required.

Install the Metal component in Xcode > Settings > Components, or run:

```sh
xcodebuild -downloadComponent metalToolchain
```

Check the complete environment with `./scripts/check-environment.sh`.

## Local workflow

```sh
./scripts/test-task.sh vector_add
./scripts/benchmark-task.sh vector_add
./scripts/profile-task.sh vector_add
./scripts/test-all.sh
```

Tests cover deterministic randomized inputs, hand-picked numerical cases,
SIMD/workgroup boundaries, odd shapes, output sentinels, and buffer guard zones.
Benchmarks refuse to run until correctness passes, warm up the GPU, and report
median and p95 GPU execution time from Metal command-buffer timestamps.

<!-- MLXGYM_BENCHMARKS_START -->
## M4 MacBook Air records

Best results from the representative workload for each task. Running
`./scripts/benchmark-task.sh <task>` replaces a record when its median GPU time improves.

| Task | Workload | Best median | p95 | Throughput | Recorded | Source |
|---|---:|---:|---:|---:|---:|---:|
| [Value clipping](problems/value_clipping/README.md) | 16777216 elements | 1.935 ms | 2.368 ms | 69.37 GB/s | 2026-09-09 | `b23db4305f0e+dirty` |
| [Vector add](problems/vector_add/README.md) | 16777216 elements | 2.779 ms | 4.387 ms | 72.45 GB/s | 2026-09-09 | `b23db4305f0e+dirty` |
<!-- MLXGYM_BENCHMARKS_END -->

`profile-task.sh` writes an Xcode `.gputrace` under `build/profiles/` using
Metal's programmatic capture API.

## Exercise files

- `kernel.metal`: the GPU algorithm you implement.
- `solution.mm`: default dispatch plumbing. Change this when a multipass
  implementation needs additional Metal entry points or intermediate buffers.
- `README.md`: the contract, correctness coverage, and benchmark workload.

Shared Metal allocation, compilation, testing, and benchmarking infrastructure
lives under `framework/`; it does not contain solutions to the exercises.

See `docs/doom-emacs.md` for Metal-aware syntax, diagnostics, and completion.
