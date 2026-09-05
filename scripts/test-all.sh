#!/usr/bin/env bash
set -euo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cmake --preset debug -S "$root"
cmake --build --preset debug --parallel
ctest --preset debug
