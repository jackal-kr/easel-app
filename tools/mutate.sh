#!/usr/bin/env bash
# Mutation tool: puts one deliberate bug at a time into a scratch copy of the sources, builds, runs the tests that must
# catch it, and says whether the bug was KILLED (a test failed) or SURVIVED (all passed: a hole in the tests).
#
#   tools/mutate.sh                 every mutant of tests/mutants.txt
#   tools/mutate.sh REQ-0005-03     only the mutants of that requirement (several IDs may be given)
#   tools/mutate.sh --lint          only checks that the find text of every mutant still occurs once in the working tree
#                                   (seconds, builds nothing); a stale mutant is listed and the exit status is 1
#   MUTANTS=other.txt tools/mutate.sh   another mutant list
#
# The working tree is never touched: everything happens under .scratch/mutate (git-ignored), which is deleted and made
# again at each start, so an interrupted run leaves nothing to clean up. Exit status: 0 if every mutant was killed by an
# assertion. A mutant whose test died without an assertion of ours (a FATAL or CRITICAL message, "Aborted", a signal) is
# "crashed": killed by crash, a test gap, counted apart and failing the run (docs/engineering.md, "Mutation tool"). A run that
# ended because the window gave no snapshot (the line "EASEL-INCONCLUSIVE: no snapshot", exit status 75) is no verdict: that
# mutant is run once more, and a second one is "INCONCLUSIVE", counted apart and failing the run; it is never counted as killed.
# Format of the list and how to add a mutant: docs/engineering.md, "Mutation tool".
set -u
cd "$(dirname "$0")/.."
ROOT=$PWD
MUTANTS=${MUTANTS:-tests/mutants.txt}
LINT=0
if [ "${1:-}" = "--lint" ]; then LINT=1; shift; fi
SCRATCH=$ROOT/.scratch/mutate
[ "$LINT" -eq 1 ] && SCRATCH=$ROOT/.scratch/mutate-lint     # a lint never disturbs a mutation run
TREE=$SCRATCH/tree
BUILD=$SCRATCH/build
WANTED=("$@")

[ -f "$MUTANTS" ] || { echo "no mutant list: $MUTANTS" >&2; exit 2; }
command -v python3 >/dev/null || { echo "python3 is needed to read the mutant list" >&2; exit 2; }

# The list, split into one numbered record per mutant: SCRATCH/m/N.find, N.replace and a line "N<TAB>REQ<TAB>file<TAB>tests<TAB>name"
# in SCRATCH/index.tsv (only the wanted requirements).
rm -rf "$SCRATCH"
mkdir -p "$SCRATCH/m" "$TREE"
python3 - "$MUTANTS" "$SCRATCH" "${WANTED[@]}" <<'PY' || exit 2
import re, sys
path, scratch, wanted = sys.argv[1], sys.argv[2], set(sys.argv[3:])
blocks, cur, mode = [], None, None
cut = []                          # blocks with text after a blank line that ended their find or replace
for raw in open(path, encoding="utf-8").read().split("\n"):
    if mode is None and cur is not None and cur.get("ended") and raw != "" and not raw.startswith(("===", "--- ", "# ")) and raw != "#":
        if cur not in cut:
            cut.append(cur)
    if mode is None and raw.startswith("#"):
        continue
    m = re.match(r"=== (REQ-\d{4}-\d{2}) (.+)$", raw)
    if m:
        cur = {"req": m.group(1), "name": m.group(2), "file": None, "test": None, "find": [], "replace": []}
        blocks.append(cur); mode = "head"; continue
    if cur is None:
        continue
    if raw == "--- find":
        mode = "find"; cur["ended"] = False; continue
    if raw == "--- replace":
        mode = "replace"; cur["ended"] = False; continue
    if mode == "head":
        k = re.match(r"(file|test): (.+)$", raw)
        if k:
            cur[k.group(1)] = k.group(2).strip()
    elif mode in ("find", "replace"):
        if raw == "":            # a blank line ends the block: the text of a mutant has none
            cur["ended"] = True
            mode = None
        else:
            cur[mode].append(raw)
if cut:
    for b in cut:
        print("blank line  %s  %s  (inside a find or replace text; it ends the entry and the rest is dropped)" % (b["req"], b["name"]))
    sys.exit("%d mutants with a blank line inside" % len(cut))
rows = []
for n, b in enumerate(blocks, 1):
    for key in ("file", "test"):
        if not b[key]:
            sys.exit("mutant %d (%s %s): no %s:" % (n, b["req"], b["name"], key))
    if not b["find"]:
        sys.exit("mutant %d (%s %s): empty find text" % (n, b["req"], b["name"]))
    if wanted and b["req"] not in wanted:
        continue
    open("%s/m/%d.find" % (scratch, n), "w", encoding="utf-8").write("\n".join(b["find"]))
    open("%s/m/%d.replace" % (scratch, n), "w", encoding="utf-8").write("\n".join(b["replace"]))
    rows.append("%d\t%s\t%s\t%s\t%s" % (n, b["req"], b["file"], b["test"], b["name"]))
open(scratch + "/index.tsv", "w", encoding="utf-8").write("\n".join(rows) + ("\n" if rows else ""))
if wanted and not rows:
    sys.exit("no mutant for: " + " ".join(sorted(wanted)))
PY

if [ "$LINT" -eq 1 ]; then
  stale=0
  while IFS=$'\t' read -r n req file tests name; do
    count=$(python3 - "$ROOT/$file" "$SCRATCH/m/$n.find" <<'PY'
import sys
try:
    print(open(sys.argv[1], encoding="utf-8").read().count(open(sys.argv[2], encoding="utf-8").read()))
except OSError:
    print(-1)
PY
)
    if [ "$count" != 1 ]; then
      echo "stale   $req  $name  ($file: the find text occurs $count times)"
      stale=$((stale + 1))
    fi
  done < "$SCRATCH/index.tsv"
  echo "lint: $(wc -l < "$SCRATCH/index.tsv") mutants, $stale stale"
  [ "$stale" -eq 0 ]
  exit
fi

# A mutant tested by a ui test needs a display without another Easel or test window (docs/engineering.md, Testing): such a
# window changes the layout of the test windows, and every ui test would "kill" its mutant for that reason.
# Not in tools/isolated.sh's private session (EASEL_ISOLATED=1): no other window can be in that display.
if cut -f4 "$SCRATCH/index.tsv" | grep -q 'ui-' && [ "${EASEL_ISOLATED:-}" != 1 ] && pgrep -x 'easel|test-window' >/dev/null; then
  echo "close every Easel and test-window window first (pid: $(pgrep -x 'easel|test-window' | tr '\n' ' ')), or run through tools/isolated.sh" >&2
  exit 2
fi

# One copy of the sources and one build directory, reused by every mutant: only the mutated file is built again.
rsync -a --exclude /build --exclude /builddir --exclude /.git --exclude /.scratch --exclude /.cache --exclude /docs \
      --exclude /.claude --exclude /.vscode ./ "$TREE/" || exit 2
echo "building the scratch copy ..."
meson setup "$BUILD" "$TREE" >"$SCRATCH/setup.log" 2>&1 || { cat "$SCRATCH/setup.log"; exit 2; }
ninja -C "$BUILD" >"$SCRATCH/build.log" 2>&1 || { tail -20 "$SCRATCH/build.log"; echo "the unmodified sources do not build" >&2; exit 2; }

# The tests of a mutant, by their meson names, one meson call for the lot (without a rebuild: ninja has built them).
run_tests () {
  local t
  for t in $1; do                       # one after the other: the first test that fails is the kill, the rest are spared
    if ! meson test -C "$BUILD" --no-rebuild --print-errorlogs "$t" >"$SCRATCH/test.log" 2>&1; then
      FAILED_TEST=$t
      return 1
    fi
  done
}

# Which case of the failing test died, and what it said. A GTest case prints "not ok /path - message". A core test prints
# "file:line: CHECK failed: ..." and the case is the function around that line. A crash prints neither, so the case is the
# one after the last "ok" line.
why_failed () {
  local line loc func last
  line=$(grep -m1 -E '^not ok ' "$SCRATCH/test.log")
  if [ -z "$line" ]; then
    line=$(grep -m1 -E 'CHECK failed' "$SCRATCH/test.log")
    if [ -n "$line" ]; then
      loc=${line%%: CHECK failed*}                      # ../tree/tests/core/test-resize.c:135
      func=$(awk -v n="${loc##*:}" 'NR <= n && /^[a-z_0-9]+ \(void\)$/ { f = $1 } END { print f }' "$TREE/${loc%:*}" 2>/dev/null)
      [ -n "$func" ] && line="case $func: $line"
    fi
  fi
  if [ -z "$line" ]; then
    line=$(grep -m1 -E 'killed by|TIMEOUT|Segmentation|Abort|failed|ERROR' "$SCRATCH/test.log")
    last=$(grep -E '^ok [0-9]+ ' "$SCRATCH/test.log" | tail -1)
    [ -n "$last" ] && line="the case after \"$last\" died: $line"
  fi
  [ -n "$line" ] || line="(no message; see $SCRATCH/test.log)"
  printf '%s' "$line" | cut -c1-230
}

# True if the test that failed did so by an assertion of ours: a core CHECK, or a GTest assertion in a file under tests/
# ("ERROR:../tests/ui/test-window.c:123:case: assertion failed ..." or "... should be TRUE"). Anything else that went with a
# FATAL or CRITICAL message, "Aborted" or a signal is a crash, which says nothing about the rule the mutant breaks.
killed_by_assertion () {
  grep -qE 'CHECK failed|ERROR:[^ ]*tests/[^ :]+:[0-9]+:' "$SCRATCH/test.log" && return 0
  grep -qE 'FATAL|CRITICAL|Aborted|Segmentation|killed by signal|SIG[A-Z]+' "$SCRATCH/test.log" && return 1
  return 0
}

# True if the run ended because the window gave no snapshot to render (render_pixels in tests/ui/test-window.c prints this line
# and exits with status 75). That says nothing about the mutant: it is not a kill, the mutant is run once more, and a second one
# is counted apart and fails the run.
inconclusive () {
  grep -q 'EASEL-INCONCLUSIVE: no snapshot' "$SCRATCH/test.log"
}

# Runs the tests of a mutant and sets KIND to survived, killed, crashed or inconclusive.
judge () {
  if run_tests "$tests"; then KIND=survived
  elif inconclusive; then KIND=inconclusive
  elif killed_by_assertion; then KIND=killed
  else KIND=crashed
  fi
}

killed=0; crashed=0; survived=0; errors=0; inconclusives=0
FAILED_TEST=
reason=
total_start=$(date +%s)

# Progress goes to stderr and to a file, never to stdout, so the result lines and the summary line of stdout stay as they were:
# the number of the mutant of the whole, the time elapsed, an estimate of what is left, and a line every EASEL_MUTATE_HEARTBEAT
# seconds (30) while one mutant is running.
PROGRESS=$SCRATCH/progress
HEARTBEAT=${EASEL_MUTATE_HEARTBEAT:-30}
HEARTBEAT_PID=
FINISHED=0                    # seconds from the start to the end of the last finished mutant
TOTAL=$(wc -l < "$SCRATCH/index.tsv")
position=0
: > "$PROGRESS"
echo "progress file: $PROGRESS" >&2
trap '[ -n "$HEARTBEAT_PID" ] && kill "$HEARTBEAT_PID" 2>/dev/null' EXIT

fmt_time () { printf '%d:%02d' $(( $1 / 60 )) $(( $1 % 60 )); }
progress () { printf '%s\n' "$1" >&2; printf '%s\n' "$1" >> "$PROGRESS"; }
# status_line DONE WHAT: "[n of N] m:ss elapsed, about m:ss left: WHAT"; DONE mutants have finished. What is left is the
# average of the finished ones times the ones to come, less what the running one has taken, so it only goes down while one runs.
status_line () {
  local elapsed=$(( $(date +%s) - total_start )) eta left
  if [ "$1" -gt 0 ]; then
    left=$(( FINISHED * (TOTAL - $1) / $1 - (elapsed - FINISHED) ))
    [ "$left" -lt 0 ] && left=0
    eta="about $(fmt_time "$left") left"
  else
    eta="the estimate comes after the first mutant"
  fi
  printf '[%d of %d] %s elapsed, %s: %s' "$position" "$TOTAL" "$(fmt_time "$elapsed")" "$eta" "$2"
}

while IFS=$'\t' read -r n req file tests name; do
  start=$(date +%s)
  position=$((position + 1))
  progress "$(status_line $((position - 1)) "mutant $position, $req $name")"
  ( while true; do
      for _ in $(seq "$HEARTBEAT"); do sleep 1; done
      progress "$(status_line $((position - 1)) "still running $req $name, $(( $(date +%s) - start )) s")"
    done ) &
  HEARTBEAT_PID=$!
  target=$TREE/$file
  cp "$target" "$SCRATCH/original"
  verdict=$(python3 - "$target" "$SCRATCH/m/$n.find" "$SCRATCH/m/$n.replace" <<'PY'
import sys
target, fpath, rpath = sys.argv[1:4]
text = open(target, encoding="utf-8").read()
find = open(fpath, encoding="utf-8").read()
count = text.count(find)
if count != 1:
    print("ERROR the text to find occurs %d times, not once" % count)
    sys.exit(0)
open(target, "w", encoding="utf-8").write(text.replace(find, open(rpath, encoding="utf-8").read(), 1))
print("OK")
PY
)
  if [ "$verdict" != "OK" ]; then
    result="ERROR   ($verdict)"; errors=$((errors + 1))
  elif ! ninja -C "$BUILD" >"$SCRATCH/build.log" 2>&1; then
    result="ERROR   (the mutant does not build)"; errors=$((errors + 1))
  else
    note=
    judge
    if [ "$KIND" = inconclusive ]; then
      note="the first run was inconclusive (no snapshot) and was run again; "
      judge
    fi
    case "$KIND" in
      survived) result="SURVIVED"; survived=$((survived + 1)) ;;
      killed) result="killed"; killed=$((killed + 1)); reason="${note}by $FAILED_TEST: $(why_failed)" ;;
      crashed)
        result="crashed"; crashed=$((crashed + 1)); reason="${note}killed by crash (test gap), by $FAILED_TEST: $(why_failed)"
        cp "$SCRATCH/test.log" "$SCRATCH/crash-$n.log"; reason="$reason (log: .scratch/mutate/crash-$n.log)" ;;
      inconclusive)
        result="INCONCLUSIVE"; inconclusives=$((inconclusives + 1)); reason="no snapshot twice, so no verdict on this mutant"
        cp "$SCRATCH/test.log" "$SCRATCH/inconclusive-$n.log"; reason="$reason (log: .scratch/mutate/inconclusive-$n.log)" ;;
    esac
  fi
  kill "$HEARTBEAT_PID" 2>/dev/null; wait "$HEARTBEAT_PID" 2>/dev/null; HEARTBEAT_PID=
  cp "$SCRATCH/original" "$target"          # the original text back, before the next mutant
  printf '%-9s %s  %s  (%d s)\n' "$result" "$req" "$name" $(( $(date +%s) - start ))
  FINISHED=$(( $(date +%s) - total_start ))
  progress "$(status_line "$position" "done, $result")"
  case "$result" in killed|crashed|INCONCLUSIVE) printf '          %s\n' "$reason" ;; esac
done < "$SCRATCH/index.tsv"

echo "$((killed + crashed + survived + errors + inconclusives)) mutants: $killed killed, $crashed killed by crash (test gap), $inconclusives inconclusive (no snapshot), $survived survived, $errors errors in $(( $(date +%s) - total_start )) s"
[ "$survived" -eq 0 ] && [ "$errors" -eq 0 ] && [ "$crashed" -eq 0 ] && [ "$inconclusives" -eq 0 ]
