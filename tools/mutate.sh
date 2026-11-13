#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Mutation tool: puts one deliberate bug at a time into a scratch copy of the sources, builds, runs the tests that must
# catch it, and says whether the bug was KILLED (a test failed) or SURVIVED (all passed: a hole in the tests).
#
#   tools/mutate.sh                 every mutant of tests/mutants.txt
#   tools/mutate.sh REQ-0005-03     only the mutants of that requirement (several IDs may be given)
#   tools/mutate.sh --core          only the mutants of files under src/core: the one run at the end of a spec (docs/engineering.md)
#   tools/mutate.sh --lint          only checks that the find text of every mutant still occurs once in the working tree
#                                   (seconds, builds nothing); a stale mutant is listed and the exit status is 1
#   MUTANTS=other.txt tools/mutate.sh   another mutant list
#   EASEL_MUTATE_WORKERS=2 tools/mutate.sh   fewer workers than the 8 at a time (each has its own sources, build folder and session)
#
# It starts its own private sessions (tools/isolated.sh), one per worker. The working tree is never touched: everything happens under
# .scratch/mutate (git-ignored), which is deleted and made again at each start, so an interrupted run leaves nothing to clean up. Exit status: 0 if every mutant was killed by an
# assertion. A mutant whose test died without an assertion of ours (a FATAL or CRITICAL message, "Aborted", a signal) is
# "crashed": killed by crash, a test gap, counted apart and failing the run (docs/engineering.md, "Mutation tool"). A run that
# ended because the window gave no snapshot (the line "EASEL-INCONCLUSIVE: no snapshot", exit status 75) is no verdict: that
# mutant is run once more, and a second one is "INCONCLUSIVE", counted apart and failing the run; it is never counted as killed.
# A test that meson reports skipped (exit 77, no display) is the same: no verdict, never "survived".
# A kill is two failures of the same case: one that does not repeat is "UNREPEATED", counted apart and failing the run. The case that
# last killed a mutant (kept in .scratch/mutate-last-kills.tsv) runs first, alone; when it passes, the mutant's tests run in full.
# Each of those cases first runs alone on the clean sources: one that fails there is named and not used. A core or io test that a mutant
# names runs on the clean sources too: one that fails there would "kill" every mutant, so the run stops with status 2 and no verdicts.
# Format of the list and how to add a mutant: docs/engineering.md, "Mutation tool".
set -u
cd "$(dirname "$0")/.."
ROOT=$PWD
MUTANTS=${MUTANTS:-tests/mutants.txt}
LINT=0
WORKER=
if [ "${1:-}" = "--lint" ]; then LINT=1; shift; fi
CORE_ONLY=0
if [ "${1:-}" = "--core" ]; then CORE_ONLY=1; shift; fi
if [ "${1:-}" = "--worker" ]; then WORKER=$2; shift 2; fi    # only as started by this script, inside its session
SCRATCH=$ROOT/.scratch/mutate
[ "$LINT" -eq 1 ] && SCRATCH=$ROOT/.scratch/mutate-lint     # a lint never disturbs a mutation run
WANTED=("$@")
LAST_KILLS=${EASEL_MUTATE_LAST_KILLS:-$ROOT/.scratch/mutate-last-kills.tsv}

[ -f "$MUTANTS" ] || { echo "no mutant list: $MUTANTS" >&2; exit 2; }
command -v python3 >/dev/null || { echo "python3 is needed to read the mutant list" >&2; exit 2; }

if [ -z "$WORKER" ]; then
# The list, split into one numbered record per mutant: SCRATCH/m/N.find, N.replace and a line "N<TAB>REQ<TAB>file<TAB>tests<TAB>name"
# in SCRATCH/index.tsv (only the wanted requirements).
rm -rf "$SCRATCH"
mkdir -p "$SCRATCH/m"
python3 - "$MUTANTS" "$SCRATCH" "$CORE_ONLY" "${WANTED[@]}" <<'PY' || exit 2
import re, sys
path, scratch, core_only, wanted = sys.argv[1], sys.argv[2], sys.argv[3] == "1", set(sys.argv[4:])
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
        cur = {"req": m.group(1), "name": m.group(2), "file": None, "test": None, "accepted": None, "find": [], "replace": []}
        blocks.append(cur); mode = "head"; continue
    if cur is None:
        continue
    if raw == "--- find":
        mode = "find"; cur["ended"] = False; continue
    if raw == "--- replace":
        mode = "replace"; cur["ended"] = False; continue
    if mode == "head":
        k = re.match(r"(file|test|accepted): (.+)$", raw)
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
    if core_only and not b["file"].startswith("src/core/"):
        continue
    open("%s/m/%d.find" % (scratch, n), "w", encoding="utf-8").write("\n".join(b["find"]))
    open("%s/m/%d.replace" % (scratch, n), "w", encoding="utf-8").write("\n".join(b["replace"]))
    if b["accepted"]:
        open("%s/m/%d.accepted" % (scratch, n), "w", encoding="utf-8").write(b["accepted"])
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
fi

TRACE=$SCRATCH/trace

# ---- a worker: its own sources, build folder and session (it runs inside tools/isolated.sh); it takes mutants from the queue ----

# How each test is started, from meson: W/tests/NAME.env (its environment), NAME.exe (its program) and NAME.targets (what ninja builds
# for it). A single case is started directly, not through `meson test --test-args`, which would add a second filter and run the case
# twice (docs/engineering.md, Testing).
read_tests () {
  mkdir -p "$TESTS"
  meson introspect --tests --targets "$BUILD" | python3 -c '
import json, os, shlex, sys
out, build = sys.argv[1:3]
info = json.load(sys.stdin)
files = {t["id"]: t["filename"] for t in info["targets"]}
for t in info["tests"]:
    with open("%s/%s.env" % (out, t["name"]), "w") as f:
        for k, v in t["env"].items():
            f.write("export %s=%s\n" % (k, shlex.quote(v)))
    open("%s/%s.exe" % (out, t["name"]), "w").write(t["cmd"][0])
    paths = [t["cmd"][0]] + [p for d in t["depends"] for p in files.get(d, [])]
    wanted = sorted({os.path.relpath(p, build) for p in paths if os.path.abspath(p).startswith(build + "/")})
    open("%s/%s.targets" % (out, t["name"]), "w").write("\n".join(wanted) + "\n")
' "$TESTS" "$BUILD"
}

# What the tests named need, and only that: a mutant then relinks the program it is tested by, not every benchmark and probe.
targets_of () {
  local t
  for t in "$@"; do [ -f "$TESTS/$t.targets" ] && cat "$TESTS/$t.targets"; done | sort -u
}

# One case alone, with the environment of its test and in the folder meson would use.
run_case () {
  ( cd "$BUILD" && . "$TESTS/$1.env" && MALLOC_PERTURB_=$((RANDOM % 255 + 1)) timeout 180 "$(cat "$TESTS/$1.exe")" -p "$2" ) >"$LOG" 2>&1
}

# The tests of a mutant, by their meson names, one meson call for the lot (without a rebuild: ninja has built them). 1 when one
# failed; 2 when one was skipped (exit 77, meson's "SKIP": no display, say), which meson counts as a pass but is no verdict.
run_tests () {
  local t
  for t in $1; do                       # one after the other: the first test that fails is the kill, the rest are spared
    if ! meson test -C "$BUILD" --no-rebuild --print-errorlogs "$t" >"$LOG" 2>&1; then
      FAILED_TEST=$t
      printf '%s %s: full %s failed\n' "$req" "$name" "$t" >> "$TRACE"
      return 1
    fi
    if grep -qE '^Skipped: +[1-9]' "$LOG"; then
      FAILED_TEST=$t
      printf '%s %s: full %s skipped\n' "$req" "$name" "$t" >> "$TRACE"
      return 2
    fi
    printf '%s %s: full %s passed\n' "$req" "$name" "$t" >> "$TRACE"
  done
}

# Which case of the failing test died, and what it said. A GTest case prints "not ok /path - message". A core test prints
# "file:line: CHECK failed: ..." and the case is the function around that line. A crash prints neither, so the case is the
# one after the last "ok" line.
why_failed () {
  local line loc func last
  line=$(grep -m1 -E '^not ok ' "$LOG")
  if [ -z "$line" ]; then
    line=$(grep -m1 -E 'CHECK failed' "$LOG")
    if [ -n "$line" ]; then
      loc=${line%%: CHECK failed*}                      # ../tree/tests/core/test-resize.c:135
      func=$(awk -v n="${loc##*:}" 'NR <= n && /^[a-z_0-9]+ \(void\)$/ { f = $1 } END { print f }' "$TREE/${loc%:*}" 2>/dev/null)
      [ -n "$func" ] && line="case $func: $line"
    fi
  fi
  if [ -z "$line" ]; then
    line=$(grep -m1 -E 'killed by|TIMEOUT|Segmentation|Abort|failed|ERROR' "$LOG")
    last=$(grep -E '^ok [0-9]+ ' "$LOG" | tail -1)
    [ -n "$last" ] && line="the case after \"$last\" died: $line"
  fi
  [ -n "$line" ] || line="(no message; see the worker's test.log)"
  printf '%s' "$line" | cut -c1-230
}

# True if the test that failed did so by an assertion of ours: a core CHECK, or a GTest assertion in a file under tests/
# ("ERROR:../tests/ui/test-window.c:123:case: assertion failed ..." or "... should be TRUE"). Anything else that went with a
# FATAL or CRITICAL message, "Aborted" or a signal is a crash, which says nothing about the rule the mutant breaks.
killed_by_assertion () {
  grep -qE 'CHECK failed|ERROR:[^ ]*tests/[^ :]+:[0-9]+:' "$LOG" && return 0
  grep -qE 'FATAL|CRITICAL|Aborted|Segmentation|killed by signal|SIG[A-Z]+' "$LOG" && return 1
  return 0
}

# True if the run ended because the window gave no snapshot to render (render_pixels in tests/ui/test-window.c prints this line
# and exits with status 75). That says nothing about the mutant: it is not a kill, the mutant is run once more, and a second one
# is counted apart and fails the run.
inconclusive () {
  grep -q 'EASEL-INCONCLUSIVE: no snapshot' "$LOG"
}

# A run whose test was skipped is no verdict either (a mutant counted as survived in 1 s while the session gave no display: 0017).

# What identifies a failure, so that two runs can be compared: the case, or the line of a core CHECK; a message can differ in its values.
failure_of () {
  local line
  line=$(grep -m1 -E '^not ok ' "$LOG")
  if [ -n "$line" ]; then printf '%s' "$line" | sed -E 's/^not ok ([0-9]+ )?([^ ]+).*/\2/'; return; fi
  line=$(grep -m1 -E 'CHECK failed' "$LOG")
  if [ -n "$line" ]; then printf '%s' "${line%%: CHECK failed*}"; return; fi
  why_failed
}

# The kept case of this mutant, twice if it fails; true when it failed twice alike, with KIND and FAILED_TEST set.
judge_first_case () {
  local test case first

  IFS=$'\t' read -r _ test case < <(grep -m1 -P "^$n\t" "$SCRATCH/first.tsv")
  [ -n "${case:-}" ] && [ -f "$TESTS/$test.exe" ] || return 1
  grep -qFx "$test"$'\t'"$case" "$SCRATCH/clean-ok" || return 1
  if run_case "$test" "$case"; then
    printf '%s %s: first-case %s passed\n' "$req" "$name" "$case" >> "$TRACE"
    return 1
  fi
  printf '%s %s: first-case %s failed\n' "$req" "$name" "$case" >> "$TRACE"
  inconclusive && return 1
  first=$(failure_of)
  if run_case "$test" "$case"; then
    printf '%s %s: first-case %s passed\n' "$req" "$name" "$case" >> "$TRACE"
    return 1
  fi
  printf '%s %s: first-case %s failed\n' "$req" "$name" "$case" >> "$TRACE"
  inconclusive && return 1
  [ "$(failure_of)" = "$first" ] || return 1
  FAILED_TEST=$test
  if killed_by_assertion; then KIND=killed; else KIND=crashed; fi
}

# Runs the tests of a mutant and sets KIND to survived, killed, crashed, inconclusive or unrepeated.
judge () {
  local rc first

  judge_first_case && return
  run_tests "$tests"
  rc=$?
  if [ "$rc" -eq 0 ]; then KIND=survived
  elif [ "$rc" -eq 2 ] || inconclusive; then KIND=inconclusive
  elif ! killed_by_assertion; then KIND=crashed
  else
    first=$(failure_of)
    run_tests "$FAILED_TEST"
    rc=$?
    if [ "$rc" -eq 2 ] || { [ "$rc" -ne 0 ] && inconclusive; }; then KIND=inconclusive
    elif [ "$rc" -ne 0 ] && [ "$(failure_of)" = "$first" ]; then KIND=killed
    else KIND=unrepeated
    fi
  fi
}

# One mutant: its result goes to SCRATCH/results/N in one move, first line the verdict, second the reason; a kill by a named case to
# SCRATCH/kills/N.
run_mutant () {
  local start=$(date +%s) target=$TREE/$file verdict result reason= note=

  date +%s > "$SCRATCH/started/$n"
  cp "$target" "$W/original"
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
    result="ERROR   ($verdict)"
  elif ! ninja -C "$BUILD" $(targets_of $tests $(grep -m1 -P "^$n\t" "$SCRATCH/first.tsv" | cut -f2)) >"$W/build.log" 2>&1; then
    result="ERROR   (the mutant does not build)"
  else
    judge
    if [ "$KIND" = inconclusive ]; then
      note="the first run was inconclusive (no snapshot or a skipped test) and was run again; "
      judge
    fi
    case "$KIND" in
      survived)
        result="SURVIVED"
        # A survivor the owner has accepted (`accepted:` in the list, with why): reported every run, fails nothing.
        if [ -f "$SCRATCH/m/$n.accepted" ]; then result="ACCEPTED"; reason="survives, accepted: $(cat "$SCRATCH/m/$n.accepted")"; fi ;;
      killed)
        result="killed"; reason="${note}by $FAILED_TEST: $(why_failed)"
        case "$(failure_of)" in
          /*) printf '%s %s\t%s\t%s\n' "$req" "$name" "$FAILED_TEST" "$(failure_of)" > "$SCRATCH/kills/$n" ;;
        esac ;;
      crashed)
        result="crashed"; reason="${note}killed by crash (test gap), by $FAILED_TEST: $(why_failed)"
        cp "$LOG" "$SCRATCH/crash-$n.log"; reason="$reason (log: .scratch/mutate/crash-$n.log)" ;;
      inconclusive)
        result="INCONCLUSIVE"; reason="no snapshot or a skipped test twice, so no verdict on this mutant"
        cp "$LOG" "$SCRATCH/inconclusive-$n.log"; reason="$reason (log: .scratch/mutate/inconclusive-$n.log)" ;;
      unrepeated)
        result="UNREPEATED"; reason="$FAILED_TEST failed once and not the same way again, so no verdict on this mutant"
        cp "$LOG" "$SCRATCH/unrepeated-$n.log"; reason="$reason (log: .scratch/mutate/unrepeated-$n.log)" ;;
    esac
  fi
  cp "$W/original" "$target"          # the original text back, before the next mutant
  { printf '%-9s %s  %s  (%d s)\n' "$result" "$req" "$name" $(( $(date +%s) - start ))
    [ -n "$reason" ] && printf '          %s\n' "$reason"; } > "$W/result"
  mv "$W/result" "$SCRATCH/results/$n"
}

run_worker () {
  local k=0 line total

  W=$SCRATCH/w$WORKER; TREE=$W/tree; BUILD=$W/build; TESTS=$W/tests; LOG=$W/test.log
  mkdir -p "$TREE"
  echo $$ > "$W/pid"
  printf '%s %s %s\n' "${WAYLAND_DISPLAY:-}" "${DBUS_SESSION_BUS_ADDRESS:-}" "${HOME:-}" > "$W/session"
  # One copy of the sources and one build directory per worker, reused by its mutants: only the mutated file is built again.
  rsync -a --exclude /build --exclude /.git --exclude /.scratch --exclude /.cache --exclude /docs \
        --exclude /.claude --exclude /.vscode ./ "$TREE/" || { echo "the sources could not be copied" > "$W/failed"; exit 2; }
  # The one file of docs that a test reads: the normative corner table of 0043, which core-shapes checks the code against.
  mkdir -p "$TREE/docs/reference/proto-0043" && cp docs/reference/proto-0043/corners.md "$TREE/docs/reference/proto-0043/" \
    || { echo "the sources could not be copied" > "$W/failed"; exit 2; }
  if ! meson setup "$BUILD" "$TREE" >"$W/setup.log" 2>&1 || ! read_tests; then
    tail -20 "$W/setup.log" > "$W/failed"; exit 2
  fi
  if ! ninja -C "$BUILD" $(targets_of $(cut -f4 "$SCRATCH/queue.tsv" | tr ' ' '\n' | sort -u) $(cut -f2 "$SCRATCH/first.tsv" | sort -u)) >"$W/build.log" 2>&1; then
    { tail -20 "$W/build.log"; echo "the unmodified sources do not build"; } > "$W/failed"; exit 2
  fi
  touch "$W/ready"

  # Each kept case alone on the clean sources, shared out like the mutants; no mutant starts before they have run. A case with no
  # result (its worker died, or it ran out the wait) is not used. A line with no case is a whole core or io test.
  total=$(wc -l < "$SCRATCH/cases.tsv")
  while IFS=$'\t' read -r test case; do
    k=$((k + 1))
    mkdir "$SCRATCH/claim/case-$k" 2>/dev/null || continue
    if [ ! -f "$TESTS/$test.exe" ]; then
      :
    elif [ -z "$case" ]; then
      meson test -C "$BUILD" --no-rebuild "$test" >"$LOG" 2>&1 || printf '%s\n' "$test" >> "$SCRATCH/clean-test-failed"
    elif run_case "$test" "$case"; then
      printf 'clean-case %s %s passed\n' "$test" "$case" >> "$TRACE"
      printf '%s\t%s\n' "$test" "$case" >> "$SCRATCH/clean-ok"
    else
      printf 'clean-case %s %s failed\n' "$test" "$case" >> "$TRACE"
      printf 'a case fails alone on the clean sources: %s %s\n' "$test" "$case" >> "$SCRATCH/clean-failed"
    fi
    touch "$SCRATCH/case-done/$k"
  done < "$SCRATCH/cases.tsv"
  for _ in $(seq 1 1200); do
    [ "$(ls "$SCRATCH/case-done" | wc -l)" -ge "$total" ] && break
    sleep 0.5
  done
  [ -s "$SCRATCH/clean-test-failed" ] && exit 0

  while IFS=$'\t' read -r n req file tests name; do
    mkdir "$SCRATCH/claim/$n" 2>/dev/null || continue
    run_mutant
  done < "$SCRATCH/queue.tsv"
}

if [ -n "$WORKER" ]; then
  run_worker
  exit 0
fi

# ---- the parent: the queue, the workers, what is printed ----

WORKERS=${EASEL_MUTATE_WORKERS:-8}
killed=0; crashed=0; survived=0; errors=0; inconclusives=0; long_only=0; unrepeated=0; accepted=0
total_start=$(date +%s)
: > "$TRACE"; : > "$SCRATCH/clean-ok"; : > "$SCRATCH/clean-failed"
mkdir -p "$SCRATCH/claim" "$SCRATCH/case-done" "$SCRATCH/started" "$SCRATCH/results" "$SCRATCH/kills"

# A mutant that only a long suite can kill (every test it names ends in -long, such as ui-0014-long) is not run here: the long
# suites belong to the gate and to /sdd-check (tests/meson.build). It is listed and counted apart, and fails nothing.
# The queue holds the others, one file's together: a worker that stays in a file builds it once for each mutant, and twice when it changes files.
python3 - "$SCRATCH/index.tsv" "$LAST_KILLS" "$SCRATCH" <<'PY'
import os, sys
index, path, scratch = sys.argv[1:4]
rows = [line.rstrip("\n").split("\t") for line in open(index, encoding="utf-8")]
queue = [r for r in rows if not all(t.endswith("-long") for t in r[3].split())]
kept = {}
if os.path.exists(path):
    for line in open(path, encoding="utf-8"):
        key, test, case = line.rstrip("\n").split("\t")
        kept[key] = (test, case)
first = [(r[0],) + kept[r[1] + " " + r[4]] for r in queue if r[1] + " " + r[4] in kept]
with open(scratch + "/queue.tsv", "w", encoding="utf-8") as f:
    for r in sorted(queue, key=lambda r: r[2]):
        f.write("\t".join(r) + "\n")
with open(scratch + "/long.tsv", "w", encoding="utf-8") as f:
    for r in rows:
        if r not in queue:
            f.write("\t".join(r) + "\n")
with open(scratch + "/first.tsv", "w", encoding="utf-8") as f:
    for row in first:
        f.write("\t".join(row) + "\n")
with open(scratch + "/cases.tsv", "w", encoding="utf-8") as f:
    for row in sorted({row[1:] for row in first}):
        f.write("\t".join(row) + "\n")
    for test in sorted({t for r in queue for t in r[3].split() if not t.startswith("ui-")}):
        f.write(test + "\t\n")
PY
while IFS=$'\t' read -r n req file tests name; do
  long_only=$((long_only + 1))
  printf '%-9s %s  %s  (needs %s, not run)\n' "LONG-ONLY" "$req" "$name" "$tests"
done < "$SCRATCH/long.tsv"

# Progress goes to stderr and to a file, never to stdout, so the result lines and the summary line of stdout stay as they were:
# the number of the mutant of the whole, the time elapsed, an estimate of what is left, and a line every EASEL_MUTATE_HEARTBEAT
# seconds (30) while a mutant is running.
PROGRESS=$SCRATCH/progress
HEARTBEAT=${EASEL_MUTATE_HEARTBEAT:-30}
FINISHED=0                    # seconds from the start to the end of the last finished mutant
TOTAL=$(wc -l < "$SCRATCH/queue.tsv")
position=0
finished=0
: > "$PROGRESS"
echo "progress file: $PROGRESS" >&2

fmt_time () { printf '%d:%02d' $(( $1 / 60 )) $(( $1 % 60 )); }
progress () { printf '%s\n' "$1" >&2; printf '%s\n' "$1" >> "$PROGRESS"; }
# status_line DONE WHAT: "[n of N] m:ss elapsed, about m:ss left: WHAT"; n mutants have started and DONE have finished. What is left is
# the average of the finished ones times the ones to come, less what has passed since the last one ended.
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

# The workers: each in a session of its own (tools/isolated.sh); without a headless GNOME Shell, one in this display.
pids=()
stop_workers () { [ "${#pids[@]}" -gt 0 ] && kill "${pids[@]}" 2>/dev/null; wait 2>/dev/null; }
trap 'stop_workers; exit 130' INT
trap 'stop_workers; exit 143' TERM
trap 'stop_workers; exit 129' HUP
[ "$TOTAL" -lt "$WORKERS" ] && WORKERS=$TOTAL
if command -v gnome-shell >/dev/null && command -v dbus-run-session >/dev/null; then
  for i in $(seq 1 "$WORKERS"); do
    mkdir -p "$SCRATCH/w$i"
    env -u EASEL_ISOLATED -u EASEL_ISOLATED_SCALE EASEL_ISOLATED_LOG="$SCRATCH/w$i/shell.log" \
        "$ROOT/tools/isolated.sh" "$ROOT/tools/mutate.sh" --worker "$i" >"$SCRATCH/w$i/session.log" 2>&1 &
    pids+=($!)
  done
else
  # A mutant tested by a ui test needs a display without another Easel or test window (docs/engineering.md, Testing): such a
  # window changes the layout of the test windows, and every ui test would "kill" its mutant for that reason.
  if cut -f4 "$SCRATCH/queue.tsv" | grep -q 'ui-' && [ "${EASEL_ISOLATED:-}" != 1 ] && pgrep -x 'easel|test-window' >/dev/null; then
    echo "close every Easel and test-window window first (pid: $(pgrep -x 'easel|test-window' | tr '\n' ' '))" >&2
    exit 2
  fi
  WORKERS=1
  mkdir -p "$SCRATCH/w1"
  "$ROOT/tools/mutate.sh" --worker 1 >"$SCRATCH/w1/session.log" 2>&1 &
  pids+=($!)
fi

declare -A ABOUT STARTED SHOWN
while IFS=$'\t' read -r n req file tests name; do ABOUT[$n]="$req $name"; done < "$SCRATCH/queue.tsv"

# What has started and finished since the last look, in that order.
take_news () {
  local path n
  for path in "$SCRATCH"/started/*; do
    [ -e "$path" ] || continue
    n=${path##*/}
    [ -n "${STARTED[$n]+x}" ] && continue
    STARTED[$n]=$(cat "$path")
    position=$((position + 1))
    progress "$(status_line "$finished" "mutant $position, ${ABOUT[$n]}")"
  done
  for path in "$SCRATCH"/results/*; do
    [ -e "$path" ] || continue
    n=${path##*/}
    [ -n "${SHOWN[$n]+x}" ] && continue
    SHOWN[$n]=1
    count_result "$(head -c 12 "$path")"
    cat "$path"
    finished=$((finished + 1))
    FINISHED=$(( $(date +%s) - total_start ))
    progress "$(status_line "$finished" "done, $(head -1 "$path" | cut -d' ' -f1)")"
  done
}
count_result () {
  case "$1" in
    killed*) killed=$((killed + 1)) ;;
    SURVIVED*) survived=$((survived + 1)) ;;
    ACCEPTED*) accepted=$((accepted + 1)) ;;
    crashed*) crashed=$((crashed + 1)) ;;
    INCONCLUSIVE*) inconclusives=$((inconclusives + 1)) ;;
    UNREPEATED*) unrepeated=$((unrepeated + 1)) ;;
    *) errors=$((errors + 1)) ;;
  esac
}

last_beat=$(date +%s)
while :; do
  take_news
  [ "$finished" -ge "$TOTAL" ] && break
  alive=0
  for pid in "${pids[@]}"; do kill -0 "$pid" 2>/dev/null && alive=1; done
  [ "$alive" -eq 0 ] && break
  if [ $(( $(date +%s) - last_beat )) -ge "$HEARTBEAT" ]; then
    last_beat=$(date +%s)
    for n in $(printf '%s\n' "${!STARTED[@]}" | sort -n); do
      if [ -z "${SHOWN[$n]+x}" ]; then
        progress "$(status_line "$finished" "still running ${ABOUT[$n]}, $(( $(date +%s) - STARTED[$n] )) s")"
        break
      fi
    done
  fi
  sleep 0.5
done
wait "${pids[@]}" 2>/dev/null
take_news

# A core or io test that fails with no mutant says nothing about the mutants.
if [ -s "$SCRATCH/clean-test-failed" ]; then
  sort -u "$SCRATCH/clean-test-failed" | sed 's/^/a test fails on the clean sources: /'
  echo "no verdicts: fix that test first" >&2
  exit 2
fi
# No worker came up: nothing has run, and the reason is theirs.
if [ "$finished" -eq 0 ] && [ "$TOTAL" -gt 0 ] && ! ls "$SCRATCH"/w*/ready >/dev/null 2>&1; then
  cat "$SCRATCH"/w*/failed 2>/dev/null
  [ -n "$(cat "$SCRATCH"/w*/failed 2>/dev/null)" ] || echo "no worker started; see $SCRATCH/w1/session.log" >&2
  exit 2
fi
# A mutant no worker finished (its worker died) is an error, never a verdict.
while IFS=$'\t' read -r n req file tests name; do
  if [ -z "${SHOWN[$n]+x}" ]; then
    errors=$((errors + 1))
    printf '%-9s %s  %s  (0 s)\n' "ERROR   (the worker stopped before its verdict)" "$req" "$name"
  fi
done < "$SCRATCH/queue.tsv"
cat "$SCRATCH/clean-failed"

# The kills of this run replace what was kept of its mutants; a mutant not run here keeps its line.
python3 - "$SCRATCH/index.tsv" "$SCRATCH/kills" "$LAST_KILLS" <<'PY'
import os, sys
index, kills, path = sys.argv[1:4]
run = set()
for line in open(index, encoding="utf-8"):
    _n, req, _file, _tests, name = line.rstrip("\n").split("\t")
    run.add(req + " " + name)
kept = []
if os.path.exists(path):
    kept = [l for l in open(path, encoding="utf-8") if l.split("\t")[0] not in run]
for name in sorted(os.listdir(kills), key=int):
    kept += list(open(os.path.join(kills, name), encoding="utf-8"))
open(path + ".new", "w", encoding="utf-8").writelines(kept)
os.replace(path + ".new", path)
PY

extra=
[ "$unrepeated" -gt 0 ] && extra=", $unrepeated not repeated"
[ "$accepted" -gt 0 ] && extra="$extra, $accepted accepted"
[ "$long_only" -gt 0 ] && extra="$extra, $long_only long-only (not run)"
echo "$((killed + crashed + survived + errors + inconclusives + unrepeated + accepted + long_only)) mutants: $killed killed, $crashed killed by crash (test gap), $inconclusives inconclusive (no snapshot or skipped), $survived survived, $errors errors$extra in $(( $(date +%s) - total_start )) s"
[ "$survived" -eq 0 ] && [ "$errors" -eq 0 ] && [ "$crashed" -eq 0 ] && [ "$inconclusives" -eq 0 ] && [ "$unrepeated" -eq 0 ]
