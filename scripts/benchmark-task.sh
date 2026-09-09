#!/usr/bin/env bash
set -euo pipefail

task="${1:?usage: $0 <task>}"
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
[[ -d "$root/problems/$task" ]] || { echo "Unknown task: $task" >&2; exit 2; }
cmake --preset release -S "$root"
cmake --build --preset release --target "$task" --parallel
record_output="$(mktemp -t mlxgym-benchmark.XXXXXX)"
trap 'rm -f "$record_output"' EXIT
MLXGYM_BENCHMARK_RECORD_FILE="$record_output" \
  "$root/build/release/problems/$task/$task" --benchmark
"$root/scripts/update-benchmark-records.sh" "$record_output"
