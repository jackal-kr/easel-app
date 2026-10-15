#!/usr/bin/env bash
# One line per feature: status, number of requirements, scenarios, tasks done/total.
cd "$(dirname "$0")/.."
printf '%-30s %-12s %4s %4s %9s\n' FEATURE STATUS REQ AS TASKS
for d in docs/specs/[0-9][0-9][0-9][0-9]-*/; do
  d=${d%/}; s=$d/spec.md
  [ -f "$s" ] || continue
  st=$(grep -m1 '^Status:' "$s" | sed 's/Status: *//' | cut -d' ' -f1)
  req=$(grep -cE '^- REQ-[0-9]{4}-[0-9]{2}' "$s" || true)
  as=$(grep -cE '^- AS-[0-9]+' "$s" || true)
  t=$d/tasks.md
  done_=$(grep -cE '^- \[x\] T[0-9]+' "$t" 2>/dev/null || true)
  all=$(grep -cE '^- \[[ x]\] T[0-9]+' "$t" 2>/dev/null || true)
  printf '%-30s %-12s %4s %4s %9s\n' "$(basename "$d")" "${st:-?}" "${req:-0}" "${as:-0}" "${done_:-0}/${all:-0}"
done
