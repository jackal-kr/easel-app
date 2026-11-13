#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Checks the parallel workers of tools/mutate.sh on the mutants of REQ-0011-04 (ui tests; each worker has its own session):
#
#   tools/check-mutate-workers.sh
#
# Each worker has its own copy of the sources, build folder and session; the working tree is not touched; every mutant gets one verdict;
# a worker that dies does not lose its mutant: it is an error and fails the run; an interrupted run stops its sessions.
set -u
cd "$(dirname "$0")/.."
DIR=$(mktemp -d)
trap 'rm -rf "$DIR"' EXIT
failures=0
check () {                      # check DESCRIPTION CONDITION-EXIT-STATUS
  if [ "$2" -eq 0 ]; then echo "  ok   $1"; else echo "  FAIL $1"; failures=$((failures + 1)); fi
}
export EASEL_MUTATE_LAST_KILLS=$DIR/kills.tsv EASEL_MUTATE_WORKERS=2
BEFORE=$(git status --porcelain | sha1sum; git diff | sha1sum)
ID=REQ-0011-04
WANT=$(python3 -c 'import re,sys; print(sum(1 for l in open("tests/mutants.txt", encoding="utf-8") if l.startswith("=== '$ID' ")))')

echo "two workers, every mutant of $ID ($WANT)"
tools/mutate.sh "$ID" > "$DIR/run.out" 2> "$DIR/run.err"; st=$?
[ "$st" -eq 0 ]; check "the run passes" $?
[ "$(grep -c '^killed ' "$DIR/run.out")" -eq "$WANT" ]; check "every mutant has its verdict" $?
grep -qE "^$WANT mutants: $WANT killed, 0 killed by crash \(test gap\), 0 inconclusive \(no snapshot or skipped\), 0 survived, 0 errors in [0-9]+ s\$" "$DIR/run.out"
check "the summary line is the one it always was" $?
[ -d .scratch/mutate/w1/tree ] && [ -d .scratch/mutate/w2/tree ] && [ -d .scratch/mutate/w1/build ] && [ -d .scratch/mutate/w2/build ]
check "each worker has its own sources and build folder" $?
[ -s .scratch/mutate/w1/session ] && [ -s .scratch/mutate/w2/session ] && ! cmp -s .scratch/mutate/w1/session .scratch/mutate/w2/session
check "each worker has its own display, bus and home" $?
[ "$(ls .scratch/mutate/results | wc -l)" -eq "$WANT" ]; check "one result per mutant" $?
[ "$(git status --porcelain | sha1sum; git diff | sha1sum)" = "$BEFORE" ]; check "the working tree is not touched" $?

echo "a worker dies: its mutant is an error, the other worker finishes the rest"
tools/mutate.sh "$ID" > "$DIR/die.out" 2> "$DIR/die.err" &
RUN=$!
sleep 2                         # the new run has emptied the folder of the old one
for _ in $(seq 1 900); do [ -e .scratch/mutate/w1/original ] && break; sleep 0.1; done     # that file: it has begun a mutant
kill -KILL "$(cat .scratch/mutate/w1/pid 2>/dev/null)" 2>/dev/null
wait "$RUN"; st=$?
[ "$st" -ne 0 ]; check "the run fails" $?
grep -q '^ERROR .*the worker stopped' "$DIR/die.out"; check "the lost mutant is reported as an error" $?
grep -qE "^$WANT mutants: " "$DIR/die.out"; check "every mutant is counted" $?

echo "an interrupted run"
tools/mutate.sh "$ID" > "$DIR/int.out" 2> "$DIR/int.err" &
RUN=$!
sleep 2
for _ in $(seq 1 120); do [ -e .scratch/mutate/w2/ready ] && break; sleep 0.5; done
kill -TERM "$RUN"; wait "$RUN"; st=$?
[ "$st" -eq 143 ]; check "the status is 143" $?
sleep 9
[ "$(ls -d /tmp/easel-isolated-home-* 2>/dev/null | wc -l)" -eq 0 ]; check "no private home is left" $?
[ "$(ps -eo args | grep -F -- 'mutate.sh --worker' | grep -vc grep)" -eq 0 ]; check "no worker is left" $?

if [ "$failures" -eq 0 ]; then echo "workers check: all ok"; else echo "workers check: $failures failed"; exit 1; fi
