#!/usr/bin/env bash
set -euo pipefail

task="${1:?usage: $0 <task>}"
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
[[ -d "$root/problems/$task" ]] || { echo "Unknown task: $task" >&2; exit 2; }
cmake --preset release -S "$root"
cmake --build --preset release --target "$task" --parallel
"$root/build/release/problems/$task/$task" --benchmark
