#!/usr/bin/env bash
set -euo pipefail

[[ "$(uname -m)" == "arm64" ]] || { echo "mlxgym requires Apple silicon." >&2; exit 1; }
command -v cmake >/dev/null || { echo "cmake is missing." >&2; exit 1; }
command -v make >/dev/null || { echo "make is missing." >&2; exit 1; }
toolchain="$(xcodebuild -showComponent MetalToolchain -json | plutil -extract toolchainIdentifier raw -o - -)"
TOOLCHAINS="$toolchain" xcrun --find metal >/dev/null
TOOLCHAINS="$toolchain" xcrun --find metallib >/dev/null
TOOLCHAINS="$toolchain" xcrun metal --version
echo "mlxgym environment: OK"
