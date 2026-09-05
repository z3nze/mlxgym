# mlxgym

A local Apple-silicon GPU practice gym for writing native Metal compute kernels.
It mirrors the 22 mathematical problems in `l33tgpu`, but contains no CUDA
kernel implementations and requires no remote machine.

Every `problems/<task>/kernel.metal` is an intentionally non-solving stub. The
corresponding executable compiles and runs, but its correctness tests fail until
you implement the Metal algorithm.

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
