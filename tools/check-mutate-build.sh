#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Checks that tools/mutate.sh builds only the tests the mutant names, and what those tests need, not the whole project:
#
#   tools/check-mutate-build.sh
#
# One core mutant, whose test needs no display.
set -u
cd "$(dirname "$0")/.."
DIR=$(mktemp -d)
trap 'rm -rf "$DIR"' EXIT
failures=0
check () {                      # check DESCRIPTION CONDITION-EXIT-STATUS
  if [ "$2" -eq 0 ]; then echo "  ok   $1"; else echo "  FAIL $1"; failures=$((failures + 1)); fi
}
cat > "$DIR/list.txt" <<'LIST'
=== REQ-0004-22 redo of a resize forgets the fill color
file: src/core/history.c
test: core-resize
--- find
        return easel_image_resize (img, s->new_w, s->new_h, s->fill) == EASEL_RESIZE_OK;
--- replace
        return easel_image_resize (img, s->new_w, s->new_h, 0) == EASEL_RESIZE_OK;
LIST
EASEL_MUTATE_LAST_KILLS=$DIR/kills.tsv MUTANTS=$DIR/list.txt tools/mutate.sh > "$DIR/out" 2> "$DIR/err"
[ $? -eq 0 ]; check "the run passes" $?
grep -q '^killed .*redo of a resize forgets' "$DIR/out"; check "the mutant is killed" $?
BUILD=$(ls -d .scratch/mutate/build .scratch/mutate/w*/build 2>/dev/null | head -1)
[ -x "$BUILD/tests/test-resize" ]; check "its test is built" $?
[ ! -e "$BUILD/tests/test-window" ]; check "the ui test program is not built" $?
[ ! -e "$BUILD/tests/perf-0001" ]; check "no benchmark is built" $?
[ ! -e "$BUILD/tests/probe-edge-drawing" ]; check "no probe is built" $?
[ ! -e "$BUILD/src/easel" ]; check "the app is not built" $?
cat >> "$DIR/list.txt" <<'LIST'

=== REQ-0011-04 a change outside src/core, which --core leaves alone
file: src/ui/canvas.c
test: ui-0011
--- find
      self->pend_pts[4] = x;
      self->pend_pts[5] = y;
--- replace
      self->pend_pts[4] = x; /* not run */
      self->pend_pts[5] = y;
LIST
EASEL_MUTATE_LAST_KILLS=$DIR/kills.tsv MUTANTS=$DIR/list.txt tools/mutate.sh --core > "$DIR/core.out" 2> "$DIR/core.err"
[ $? -eq 0 ]; check "--core: the run passes" $?
grep -qE '^1 mutants: 1 killed, ' "$DIR/core.out"; check "--core: only the mutant under src/core runs" $?
if [ "$failures" -eq 0 ]; then echo "build check: all ok"; else echo "build check: $failures failed"; exit 1; fi
