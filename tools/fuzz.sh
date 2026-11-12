#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Fuzzes every reader of outside data with libFuzzer (clang, address and undefined sanitizers), as a release asks
# (docs/engineering.md, "At a release"): the settings file, a palette file and a color, pasted HTML, a typed number, and a
# picture opened from bytes. Each runs at nice 19, all at once, for SECONDS (default 2400, 40 minutes).
#
#   tools/fuzz.sh [SECONDS]
#
# Work in .scratch/fuzz (git-ignored): the binaries, a corpus per harness seeded from tests/data (and kept between runs, so a later
# run starts from what an earlier one found), crashes in crashes-<name>/, a log per harness. Exit status 1 if any harness crashed.
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
SECS=${1:-2400}
OUT=$ROOT/.scratch/fuzz
mkdir -p "$OUT"
CORE=$(ls "$ROOT"/src/core/*.c)
GLIB="$(pkg-config --cflags --libs gio-2.0 gdk-pixbuf-2.0)"
for name in settings palette html number open; do
  extra=""
  [ "$name" = open ] && extra="$ROOT/src/io/picture-file.c $GLIB"
  # shellcheck disable=SC2086
  clang -g -O1 -fsanitize=fuzzer,address,undefined -I"$ROOT/src/core" -I"$ROOT/src/io" \
    "$ROOT/tests/fuzz/fuzz-$name.c" $CORE $extra -lm -o "$OUT/fuzz-$name" || { echo "build of fuzz-$name failed"; exit 2; }
  mkdir -p "$OUT/corpus-$name" "$OUT/crashes-$name"
done
# Seeds: the test data, by kind
cp -n "$ROOT"/tests/data/*.{png,jpg,jpeg,bmp,gif,tif,tiff,webp,ico} "$OUT/corpus-open/" 2>/dev/null
find "$ROOT/tests/data" -name '*.ini' -o -name '*.conf' | while read -r f; do cp -n "$f" "$OUT/corpus-settings/"; cp -n "$f" "$OUT/corpus-palette/"; done
find "$ROOT/tests/data" -name '*.htm*' -exec cp -n {} "$OUT/corpus-html/" \;
pids=()
for name in settings palette html number open; do
  ( cd "$OUT" && ASAN_OPTIONS=detect_leaks=0 nice -n 19 "./fuzz-$name" -max_total_time="$SECS" -max_len=65536 -timeout=30 \
      -rss_limit_mb=4096 -artifact_prefix="crashes-$name/" "corpus-$name" > "fuzz-$name.log" 2>&1 ) &
  pids+=($!)
done
wait "${pids[@]}"
status=0
for name in settings palette html number open; do
  n=$(find "$OUT/crashes-$name" -type f | wc -l)
  echo "fuzz-$name: $(grep -oE '#[0-9]+.*cov: [0-9]+' "$OUT/fuzz-$name.log" | tail -1), crashes $n"
  [ "$n" -eq 0 ] || status=1
done
exit $status
