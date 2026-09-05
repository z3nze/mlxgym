#!/usr/bin/env bash
set -euo pipefail

task="${1:?usage: $0 <task>}"
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
[[ -d "$root/problems/$task" ]] || { echo "Unknown task: $task" >&2; exit 2; }
cmake --preset release -S "$root"
cmake --build --preset release --target "$task" --parallel
mkdir -p "$root/build/profiles"
trace="$root/build/profiles/${task}-$(date +%Y%m%d-%H%M%S).gputrace"
MTL_CAPTURE_ENABLED=1 "$root/build/release/problems/$task/$task" --capture "$trace" || true
[[ -e "$trace" ]] && echo "GPU trace: $trace"
