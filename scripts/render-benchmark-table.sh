#!/usr/bin/env bash
set -euo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
records="$root/benchmarks/apple-m4-macbook-air.tsv"
readme="$root/README.md"
section="$(mktemp -t mlxgym-readme-section.XXXXXX)"
rendered="$(mktemp -t mlxgym-readme.XXXXXX)"
trap 'rm -f "$section" "$rendered"' EXIT

{
  echo '<!-- MLXGYM_BENCHMARKS_START -->'
  echo '## M4 MacBook Air records'
  echo
  echo 'Best results from the representative workload for each task. Running'
  echo '`./scripts/benchmark-task.sh <task>` replaces a record when its median GPU time improves.'
  echo
  echo '| Task | Workload | Best median | p95 | Throughput | Recorded | Source |'
  echo '|---|---:|---:|---:|---:|---:|---:|'
  awk -F '\t' '
    function title(value) {
      gsub(/_/, " ", value)
      return toupper(substr(value, 1, 1)) substr(value, 2)
    }
    function duration(us) {
      return us >= 1000 ? sprintf("%.3f ms", us / 1000) : sprintf("%.3f µs", us)
    }
    function throughput(task, gb, gflop, munit) {
      if (gflop + 0 > 0)
        return sprintf("%.2f GFLOP/s", gflop)
      if (task == "rainbow_table")
        return sprintf("%.2f Mhash/s", munit)
      if (task == "rgb_to_grayscale")
        return sprintf("%.2f Mpixel/s", munit)
      if (task == "batch_normalization" || task == "softmax" ||
          task == "sigmoid_activation" || task == "sigmoid_linear_unit" ||
          task == "swish_gated_linear_unit" ||
          task == "gaussian_error_gated_linear_unit")
        return sprintf("%.2f Melem/s", munit)
      return sprintf("%.2f GB/s", gb)
    }
    NR > 1 && $3 == 1 {
      printf "| [%s](problems/%s/README.md) | %s | %s | %s | %s | %s | `%s` |\n", \
        title($1), $1, $2, duration($4), duration($5), throughput($1, $6, $7, $8), \
        substr($12, 1, 10), $13
      found = 1
    }
    END {
      if (!found)
        print "| _No benchmark records yet_ | — | — | — | — | — | — |"
    }
  ' "$records"
  echo '<!-- MLXGYM_BENCHMARKS_END -->'
} > "$section"

awk '
  FNR == NR { replacement[++replacement_count] = $0; next }
  $0 == "<!-- MLXGYM_BENCHMARKS_START -->" {
    for (i = 1; i <= replacement_count; ++i)
      print replacement[i]
    replacing = 1
    found = 1
    next
  }
  replacing {
    if ($0 == "<!-- MLXGYM_BENCHMARKS_END -->")
      replacing = 0
    next
  }
  { print }
  END {
    if (!found) {
      print ""
      for (i = 1; i <= replacement_count; ++i)
        print replacement[i]
    }
  }
' "$section" "$readme" > "$rendered"

mv "$rendered" "$readme"
