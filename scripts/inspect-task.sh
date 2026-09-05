#!/usr/bin/env bash
set -euo pipefail

task="${1:?usage: $0 <task>}"
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
"$root/scripts/test-task.sh" "$task"
