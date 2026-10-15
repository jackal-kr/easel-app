#!/usr/bin/env bash
# Spec lint. Every REQ needs a scenario (AS) that mentions it; every AS needs "Verified by:";
# an accepted/implemented spec may have no open "- [NEEDS CLARIFICATION" lines.
# Draft specs only get warnings; the exit code is 1 only for accepted/implemented specs.
cd "$(dirname "$0")/.."
rc=0
for s in docs/specs/[0-9][0-9][0-9][0-9]-*/spec.md; do
  [ -f "$s" ] || continue
  st=$(grep -m1 '^Status:' "$s" | sed 's/Status: *//' | cut -d' ' -f1)
  problems=()
  for r in $(grep -oE '^- REQ-[0-9]{4}-[0-9]{2}' "$s" | sed 's/- //'); do
    grep -E '^- AS-[0-9]+' "$s" | grep -q "$r" || problems+=("$r has no acceptance scenario")
  done
  if grep -E '^- AS-[0-9]+' "$s" | grep -vq 'Verified by:'; then
    problems+=("a scenario has no 'Verified by:'")
  fi
  if [ "$st" != draft ] && grep -q '^- \[NEEDS CLARIFICATION' "$s"; then
    problems+=("open NEEDS CLARIFICATION in a $st spec")
  fi
  for p in "${problems[@]}"; do
    echo "$s: $p"
    [ "$st" = draft ] || rc=1
  done
done
[ $rc -eq 0 ] && echo "spec lint: ok (draft specs may still show warnings above)"
exit $rc
