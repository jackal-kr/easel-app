#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Checks how tools/mutate.sh uses the case that last killed a mutant (ui tests, in the sessions the tool starts):
#
#   tools/check-mutate-kills.sh
#
# The case runs first, alone; a kill is two failures of the same case; a case that passes sends the mutant to its full tests; a case
# that fails alone on the clean sources is not used; a kill that does not repeat is counted apart and fails the run. Each run's kills
# are kept for the next one.
set -u
cd "$(dirname "$0")/.."
DIR=$(mktemp -d)
trap 'rm -rf "$DIR"' EXIT
failures=0
check () {                      # check DESCRIPTION CONDITION-EXIT-STATUS
  if [ "$2" -eq 0 ]; then echo "  ok   $1"; else echo "  FAIL $1"; failures=$((failures + 1)); fi
}
TRACE=.scratch/mutate/trace
KILLER=/0011/as2_curve_stages_and_preview
OTHER=/0011/as5_polygon_clicks
export EASEL_MUTATE_LAST_KILLS=$DIR/kills.tsv

# A real mutant, a change no test can see, and a failure that happens once only (its marker file is there at the second run).
KILLED='=== REQ-0011-04 the second bend moves both control points
file: src/ui/canvas.c
test: ui-0011
--- find
      self->pend_pts[4] = x;
      self->pend_pts[5] = y;
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
--- replace
      self->pend_pts[2] = x;
      self->pend_pts[3] = y;
      self->pend_pts[4] = x;
      self->pend_pts[5] = y;
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
'
SEEN='=== REQ-0011-04 a comment no test can see
file: src/ui/canvas.c
test: ui-0011
--- find
      self->pend_pts[4] = x;
      self->pend_pts[5] = y;
--- replace
      self->pend_pts[4] = x; /* no test can see this */
      self->pend_pts[5] = y;
'
ACCEPTED='=== REQ-0011-04 an accepted change no test can see
file: src/ui/canvas.c
test: ui-0011
accepted: this check, to see how it is counted
--- find
      self->pend_pts[4] = x;
      self->pend_pts[5] = y;
--- replace
      self->pend_pts[4] = x; /* an accepted change */
      self->pend_pts[5] = y;
'
ONCE="=== REQ-0002-17 a failure that happens once only
file: tests/core/test-lattice.c
test: core-lattice
--- find
main (void)
{
--- replace
main (void)
{
  if (fopen (\"$DIR/marker\", \"r\") == NULL) { fclose (fopen (\"$DIR/marker\", \"w\")); printf (\"tests/core/test-lattice.c:1: CHECK failed: once\\n\"); return 1; }
"
printf '%s\n%s\n%s\n' "$KILLED" "$SEEN" "$ONCE" > "$DIR/list.txt"
printf '%s\n' "$KILLED" > "$DIR/one.txt"
map () {                        # map CASE-OF-THE-REAL-MUTANT [CASE-OF-THE-COMMENT]
  printf 'REQ-0011-04 the second bend moves both control points\tui-0011\t%s\n' "$1" > "$EASEL_MUTATE_LAST_KILLS"
  [ $# -gt 1 ] && printf 'REQ-0011-04 a comment no test can see\tui-0011\t%s\n' "$2" >> "$EASEL_MUTATE_LAST_KILLS"
}

echo "an accepted survivor, and an accepted mutant that a test kills"
printf '%s\n' "$ACCEPTED" > "$DIR/accepted.txt"
MUTANTS=$DIR/accepted.txt tools/mutate.sh > "$DIR/accepted.out" 2> "$DIR/accepted.err"; st=$?
[ "$st" -eq 0 ]; check "an accepted survivor does not fail the run" $?
grep -q '^ACCEPTED .*an accepted change no test can see' "$DIR/accepted.out"; check "it is reported as accepted" $?
grep -q 'this check, to see how it is counted' "$DIR/accepted.out"; check "with the reason" $?
grep -qE '^1 mutants: 0 killed, 0 killed by crash \(test gap\), 0 inconclusive \(no snapshot or skipped\), 0 survived, 0 errors, 1 accepted in [0-9]+ s$' "$DIR/accepted.out"
check "the summary line counts it apart" $?
printf '%s\n' "$KILLED" | sed 's/^test: ui-0011$/test: ui-0011\naccepted: this check/' > "$DIR/accepted-killed.txt"
MUTANTS=$DIR/accepted-killed.txt tools/mutate.sh > "$DIR/accepted-killed.out" 2> "$DIR/accepted-killed.err"; st=$?
[ "$st" -eq 0 ]; check "an accepted mutant that is killed passes" $?
grep -q '^killed .*the second bend moves both' "$DIR/accepted-killed.out"; check "it is reported as killed: the mark can go" $?

echo "the first run: the right case, a case that cannot see the change, a kill that does not repeat"
map "$KILLER" "$OTHER"
MUTANTS=$DIR/list.txt tools/mutate.sh > "$DIR/first.out" 2> "$DIR/first.err"; st=$?
[ "$st" -ne 0 ]; check "the run fails: a survivor and a kill that did not repeat" $?
grep -q '^killed .*the second bend moves both' "$DIR/first.out"; check "the real mutant is killed" $?
[ "$(grep -c "the second bend moves both.*first-case $KILLER failed" "$TRACE")" -eq 2 ]; check "its case failed twice" $?
! grep -q 'the second bend moves both.*full ' "$TRACE"; check "its full test did not run" $?
grep -q "^clean-case ui-0011 $KILLER passed" "$TRACE"; check "the case ran on the clean sources first" $?
grep -q "a comment no test can see.*first-case $OTHER passed" "$TRACE"; check "the case that cannot see the change passed" $?
grep -q 'a comment no test can see.*full ui-0011 passed' "$TRACE"; check "and the full test ran after it" $?
grep -q '^SURVIVED .*a comment no test can see' "$DIR/first.out"; check "that change survives" $?
grep -q '^UNREPEATED .*a failure that happens once' "$DIR/first.out"; check "the failure that happened once is no kill" $?
grep -qE '^3 mutants: 1 killed, 0 killed by crash \(test gap\), 0 inconclusive \(no snapshot or skipped\), 1 survived, 0 errors, 1 not repeated in [0-9]+ s$' "$DIR/first.out"
check "the summary line counts it apart" $?
grep -q "^REQ-0011-04 the second bend moves both control points	ui-0011	$KILLER\$" "$EASEL_MUTATE_LAST_KILLS"; check "the kill is kept for the next run" $?
! grep -q 'a comment no test can see' "$EASEL_MUTATE_LAST_KILLS"; check "the survivor's old case is forgotten" $?

echo "the second run: the kept case cannot see the change"
map "$OTHER"
MUTANTS=$DIR/one.txt tools/mutate.sh > "$DIR/second.out" 2> "$DIR/second.err"; st=$?
[ "$st" -eq 0 ]; check "the run passes" $?
grep -q 'the second bend moves both.*full ui-0011 failed' "$TRACE"; check "the full test killed it" $?
[ "$(grep -c 'the second bend moves both.*full ui-0011 failed' "$TRACE")" -eq 2 ]; check "and the same case failed twice" $?
grep -q "^REQ-0011-04 the second bend moves both control points	ui-0011	$KILLER\$" "$EASEL_MUTATE_LAST_KILLS"; check "the case that killed it now is kept" $?

echo "the third run: the kept case fails alone on the clean sources (no snapshot in the first process)"
map "$KILLER"
EASEL_TEST_NO_SNAPSHOT_ONCE=$DIR/snapshot-once MUTANTS=$DIR/one.txt tools/mutate.sh > "$DIR/third.out" 2> "$DIR/third.err"; st=$?
[ "$st" -eq 0 ]; check "the run passes" $?
grep -q "^a case fails alone on the clean sources: ui-0011 $KILLER" "$DIR/third.out"; check "the case is named" $?
! grep -q 'the second bend moves both.*first-case' "$TRACE"; check "it is not used" $?
grep -q 'the second bend moves both.*full ui-0011 failed' "$TRACE"; check "the full test killed the mutant" $?

if [ "$failures" -eq 0 ]; then echo "last-kill check: all ok"; else echo "last-kill check: $failures failed"; exit 1; fi
