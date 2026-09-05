# Benchmarking and profiling

Correctness is always the first gate. `benchmark-task.sh` runs the full test
suite before recording timings. Allocation, pipeline creation, and CPU reference
work are outside the reported GPU interval.

Each case performs five warmups and thirty measured command buffers, then reports
the median and p95 GPU time. Depending on the task it also reports effective
bandwidth, GFLOP/s, or processed units per second. These figures are intended for
comparing your own revisions on the same machine, not for comparing unlike Macs.

Use `profile-task.sh <task>` to save a timestamped `.gputrace` under
`build/profiles/`. Open the trace in Xcode's Metal debugger to inspect occupancy,
memory access, shader source, and individual dispatches. Because capture adds
overhead, do not use captured times as benchmark results.
