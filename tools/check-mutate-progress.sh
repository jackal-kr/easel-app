#!/usr/bin/env bash
# Checks the progress reporting of tools/mutate.sh on the three mutants of REQ-0011-04 (a ui test: run it through
# tools/isolated.sh, as the mutation runs are):
#
#   tools/isolated.sh tools/check-mutate-progress.sh
#
# Progress goes to stderr and to a file, never to stdout; stdout keeps the lines and the summary line it always had. The
# heartbeat is every 2 seconds here (EASEL_MUTATE_HEARTBEAT) so that a mutant of about 6 seconds shows at least one.
set -u
cd "$(dirname "$0")/.."
OUT=$(mktemp)
ERR=$(mktemp)
trap 'rm -f "$OUT" "$ERR"' EXIT
failures=0
check () {                      # check DESCRIPTION CONDITION-EXIT-STATUS
  if [ "$2" -eq 0 ]; then echo "  ok   $1"; else echo "  FAIL $1"; failures=$((failures + 1)); fi
}

EASEL_MUTATE_HEARTBEAT=2 tools/mutate.sh REQ-0011-04 > "$OUT" 2> "$ERR"
echo "the mutation run: exit status $?"

PROGRESS_FILE=$(sed -n 's/^progress file: //p' "$ERR" | head -1)
[ -n "$PROGRESS_FILE" ]; check "stderr names the progress file at the start" $?
[ -n "$PROGRESS_FILE" ] && [ -s "$PROGRESS_FILE" ]; check "the progress file exists and is not empty" $?
for n in 1 2 3; do
  grep -qE "^\[$n of 3\] [0-9]+:[0-9]{2} elapsed" "$ERR"; check "stderr has a line for mutant $n of 3 with the elapsed time" $?
done
grep -qE "^\[[0-9] of 3\] [0-9]+:[0-9]{2} elapsed, about [0-9]+:[0-9]{2} left" "$ERR"; check "stderr has an estimate of the time left" $?
grep -qE "still running" "$ERR"; check "stderr has a heartbeat line while a mutant runs" $?
# The estimate does not grow while one mutant runs: within the lines of mutant n, the time left never goes up.
awk '/^\[[0-9]+ of [0-9]+\] / && /about [0-9]+:[0-9][0-9] left/ {
       match($0, /^\[[0-9]+/); n = substr($0, RSTART + 1, RLENGTH - 1)
       match($0, /about [0-9]+:[0-9][0-9]/); split(substr($0, RSTART + 6, RLENGTH - 6), t, ":"); left = t[1] * 60 + t[2]
       if (n in last && left > last[n]) bad = 1
       last[n] = left }
     END { exit bad }' "$ERR"; check "the estimate of the time left does not grow while one mutant runs" $?
[ -n "$PROGRESS_FILE" ] && tail -1 "$PROGRESS_FILE" | grep -qE "^\[3 of 3\]"; check "the last line of the progress file is the third mutant's" $?
if [ -n "$PROGRESS_FILE" ]; then
  diff <(grep -E '^\[' "$ERR") <(grep -E '^\[' "$PROGRESS_FILE") >/dev/null; check "the progress file holds the progress lines that stderr has" $?
fi
! grep -qE '^\[|progress|still running' "$OUT"; check "stdout has no progress line" $?
[ "$(grep -cE '^(killed|SURVIVED|crashed|INCONCLUSIVE|ERROR) ' "$OUT")" -eq 3 ]; check "stdout has the three result lines as before" $?
SUMMARY=$(grep -E '^[0-9]+ mutants: ' "$OUT")
[ "$(printf '%s\n' "$SUMMARY" | wc -l)" -eq 1 ] && printf '%s\n' "$SUMMARY" |
  grep -qE '^3 mutants: 3 killed, 0 killed by crash \(test gap\), 0 inconclusive \(no snapshot\), 0 survived, 0 errors in [0-9]+ s$'
check "the summary line is the one it always was, only its seconds differ" $?

if [ "$failures" -eq 0 ]; then echo "progress check: all ok"; else echo "progress check: $failures failed"; exit 1; fi
