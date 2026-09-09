#!/usr/bin/env bash
set -euo pipefail

input="${1:?usage: $0 <benchmark-record-file>}"
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
records="$root/benchmarks/apple-m4-macbook-air.tsv"

[[ -f "$input" ]] || { echo "Benchmark record file is missing: $input" >&2; exit 2; }

device="$(awk -F '\t' '$1 == "MLXGYM_DEVICE_V1" { print $2; exit }' "$input")"
hardware="$(system_profiler SPHardwareDataType 2>/dev/null || true)"
model_name="$(awk -F ': ' '/^[[:space:]]*Model Name:/ { print $2; exit }' <<<"$hardware")"
model_id="$(awk -F ': ' '/^[[:space:]]*Model Identifier:/ { print $2; exit }' <<<"$hardware")"
chip="$(awk -F ': ' '/^[[:space:]]*Chip:/ { print $2; exit }' <<<"$hardware")"

if [[ "$device" != "Apple M4" || "$chip" != "Apple M4" || "$model_name" != "MacBook Air" ]]; then
  echo "Benchmark finished, but records were not updated: expected an M4 MacBook Air" >&2
  echo "Detected model='${model_name:-unknown}', chip='${chip:-unknown}', Metal device='${device:-unknown}'" >&2
  exit 0
fi

model_id="${model_id:-unknown}"
macos_version="$(sw_vers -productVersion 2>/dev/null || echo unknown)"
os_build="$(sw_vers -buildVersion 2>/dev/null || echo unknown)"
recorded_at="$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
commit="$(git -C "$root" rev-parse --short=12 HEAD 2>/dev/null || echo unknown)"
updated=0
seen=0

while IFS=$'\t' read -r marker task workload primary median_us p95_us gb_per_s gflop_per_s munit_per_s; do
  [[ "$marker" == "MLXGYM_BENCH_V1" ]] || continue
  seen=$((seen + 1))

  source="$commit"
  if [[ -n "$(git -C "$root" status --porcelain --untracked-files=no -- \
      "problems/$task/kernel.metal" "problems/$task/solution.mm")" ]]; then
    source="${source}+dirty"
  fi

  old_median="$(awk -F '\t' -v task="$task" -v workload="$workload" \
    'NR > 1 && $1 == task && $2 == workload { print $4; exit }' "$records")"
  if [[ -n "$old_median" ]] && ! awk -v candidate="$median_us" -v best="$old_median" \
      'BEGIN { exit !(candidate + 0 < best + 0) }'; then
    continue
  fi

  temporary="$(mktemp -t mlxgym-records.XXXXXX)"
  awk -F '\t' -v task="$task" -v workload="$workload" \
    'NR == 1 || !($1 == task && $2 == workload)' "$records" > "$temporary"
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$task" "$workload" "$primary" "$median_us" "$p95_us" "$gb_per_s" \
    "$gflop_per_s" "$munit_per_s" "$model_id" "$macos_version" "$os_build" \
    "$recorded_at" "$source" >> "$temporary"
  mv "$temporary" "$records"
  updated=$((updated + 1))
  if [[ -n "$old_median" ]]; then
    printf 'New record: %s / %s (%s us, previously %s us)\n' \
      "$task" "$workload" "$median_us" "$old_median"
  else
    printf 'First record: %s / %s (%s us)\n' "$task" "$workload" "$median_us"
  fi
done < "$input"

if (( seen == 0 )); then
  echo "Benchmark produced no machine-readable results" >&2
  exit 1
fi

if (( updated > 0 )); then
  temporary="$(mktemp -t mlxgym-records.XXXXXX)"
  sed -n '1p' "$records" > "$temporary"
  sed -n '2,$p' "$records" | LC_ALL=C sort -t $'\t' -k1,1 -k2,2 >> "$temporary"
  mv "$temporary" "$records"
else
  echo "No records improved."
fi

"$root/scripts/render-benchmark-table.sh"
