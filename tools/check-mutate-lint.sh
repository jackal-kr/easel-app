#!/usr/bin/env bash
# Checks that `tools/mutate.sh --lint` refuses a blank line inside the find or the replace text of a mutant (a blank line
# ends the entry, so the rest would be dropped and the mutant would test unchanged code). Needs no display:
#
#   tools/check-mutate-lint.sh
set -u
cd "$(dirname "$0")/.."
DIR=$(mktemp -d)
trap 'rm -rf "$DIR"' EXIT
failures=0
check () {                      # check DESCRIPTION CONDITION-EXIT-STATUS
  if [ "$2" -eq 0 ]; then echo "  ok   $1"; else echo "  FAIL $1"; failures=$((failures + 1)); fi
}
# A list of one mutant over a real line of the tree; $1 is the text between the find line and the replace line, $2 after it.
list () {
  printf '# a comment\n=== REQ-0001-01 sample\nfile: src/core/selection.c\ntest: core-selection\n--- find\n%s\n--- replace\n%s\n\n=== REQ-0001-02 next\nfile: src/core/selection.c\ntest: core-selection\n--- find\n  f->stamps = p;\n--- replace\n  f->stamps = 0;\n' "$1" "$2"
}
run () {                        # run NAME FIND REPLACE: lint output in $DIR/NAME.out, exit status returned
  list "$2" "$3" > "$DIR/$1.txt"
  MUTANTS="$DIR/$1.txt" tools/mutate.sh --lint > "$DIR/$1.out" 2>&1
}
ONE='  f->hole_color = hole_color;'

run clean "$ONE" '  f->hole_color = 0;'
st=$?
[ "$st" -eq 0 ]; check "a clean list: the lint passes" $?
grep -q "blank line" "$DIR/clean.out"; [ $? -ne 0 ]; check "a list without a blank line is not refused for one" $?

run blank_find "$ONE

$ONE" '  return;'
st=$?
[ "$st" -ne 0 ]; check "a blank line inside a find: the lint fails" $?
grep -q "blank line.*REQ-0001-01" "$DIR/blank_find.out"; check "a blank line inside a find: the mutant is named" $?

run blank_replace "$ONE" '  a;

  b;'
st=$?
[ "$st" -ne 0 ]; check "a blank line inside a replace: the lint fails" $?
grep -q "blank line.*REQ-0001-01" "$DIR/blank_replace.out"; check "a blank line inside a replace: the mutant is named" $?

tools/mutate.sh --lint > "$DIR/real.out" 2>&1
grep -q "blank line" "$DIR/real.out"; [ $? -ne 0 ]; check "the real list has no blank line inside a mutant" $?

echo "$failures failure(s)"
[ "$failures" -eq 0 ]
