#!/usr/bin/env bash
# Usage: tools/spec.sh new <NNNN> <slug> | status | lint [--names <NNNN>]
# Specs are single files: docs/specs/NNNN-slug.md (template: docs/specs/_template.md).
cd "$(dirname "$0")/.."
shopt -s nullglob
specs=(docs/specs/[0-9][0-9][0-9][0-9]-*.md)

status_of() { grep -m1 '^Status:' "$1" | sed 's/Status: *//' | cut -d' ' -f1; }

cmd_new() {
  local n="${1:-}" slug="${2:-}"
  [[ "$n" =~ ^[0-9]{4}$ && -n "$slug" ]] || { echo "usage: tools/spec.sh new <NNNN> <slug>"; exit 1; }
  local f="docs/specs/$n-$slug.md"
  [ -e "$f" ] && { echo "exists: $f"; exit 1; }
  sed "s/NNNN/$n/g" docs/specs/_template.md > "$f"
  echo "created $f"
}

cmd_status() {
  [ ${#specs[@]} -eq 0 ] && { echo "no specs yet"; return; }
  printf '%-34s %-12s %4s %4s %9s\n' SPEC STATUS REQ AS TASKS
  local s
  for s in "${specs[@]}"; do
    printf '%-34s %-12s %4s %4s %9s\n' "$(basename "$s" .md)" "$(status_of "$s")" \
      "$(grep -cE '^- REQ-[0-9]{4}-[0-9]{2}' "$s")" "$(grep -cE '^- AS-[0-9]+' "$s")" \
      "$(grep -cE '^- \[x\] T[0-9]+' "$s")/$(grep -cE '^- \[[ x]\] T[0-9]+' "$s")"
  done
}

# Every REQ needs a scenario that mentions it; every scenario needs "Verified by:";
# an accepted/implemented spec may have no open "- [NEEDS CLARIFICATION" lines; an accepted one needs a "Reviewed:" line.
# Draft specs only get warnings; the exit code is 1 only for accepted/implemented specs.
cmd_lint() {
  local rc=0 s st r p
  for s in "${specs[@]}"; do
    st=$(status_of "$s")
    local problems=()
    for r in $(grep -oE '^- REQ-[0-9]{4}-[0-9]{2}' "$s" | sed 's/- //'); do
      grep -E '^- AS-[0-9]+' "$s" | grep -q "$r" || problems+=("$r has no scenario")
    done
    grep -E '^- AS-[0-9]+' "$s" | grep -vq 'Verified by:' && problems+=("a scenario has no 'Verified by:'")
    if [ "$st" != draft ] && grep -q '^- \[NEEDS CLARIFICATION' "$s"; then
      problems+=("open NEEDS CLARIFICATION in a $st spec")
    fi
    # An accepted spec names the review it had (sdd-spec step 5); implemented specs predate the rule and are not asked.
    if [ "$st" = accepted ] && ! grep -qE '^Reviewed: ' "$s"; then
      problems+=("accepted without a 'Reviewed:' line")
    fi
    for p in "${problems[@]}"; do
      echo "$s: $p"
      [ "$st" = draft ] || rc=1
    done
  done
  [ $rc -eq 0 ] && echo "spec lint: ok (draft specs may still show warnings above)"
  return $rc
}

# Every test named in backticks in the scenario-to-test map of one spec must be registered in tests/ (a core test as
# `RUN (name)`, a ui or io test as `"/NNNN/name"` or, in a long suite, `"/long/NNNN/name"`): the names drift when a test is split or
# moved. The map ends at the next heading of any level (the Tasks follow it, and their Result lines name helpers too). Run by /sdd-check; the
# plain lint does not do it, because a Plan names its tests before they are written.
cmd_names() {
  local n="${1:-}" f t rc=0
  [[ "$n" =~ ^[0-9]{4}$ ]] || { echo "usage: tools/spec.sh lint --names <NNNN>"; return 1; }
  f=$(ls docs/specs/"$n"-*.md 2>/dev/null | head -1)
  [ -n "$f" ] || { echo "no spec $n"; return 1; }
  for t in $(sed -n '/^### Scenario-to-test map/,/^##/p' "$f" | grep -oE '`[a-z][a-z0-9]*_[a-z0-9_]*`' | tr -d '`' | sort -u); do
    if ! grep -rqE "(RUN \\(|\"/(long/)?[0-9]{4}/)$t\\b" tests/; then
      echo "$f: the scenario map names a test that is not registered: $t"
      rc=1
    fi
  done
  [ $rc -eq 0 ] && echo "spec names: ok ($f)"
  return $rc
}

case "${1:-}" in
  new) shift; cmd_new "$@" ;;
  status) cmd_status ;;
  lint) if [ "${2:-}" = --names ]; then cmd_names "${3:-}"; else cmd_lint; fi ;;
  *) echo "usage: tools/spec.sh new <NNNN> <slug> | status | lint [--names <NNNN>]"; exit 1 ;;
esac
