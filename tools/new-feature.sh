#!/usr/bin/env bash
# Usage: tools/new-feature.sh <NNNN> <slug>   e.g. tools/new-feature.sh 0001 walking-skeleton
# Creates docs/specs/NNNN-slug/{spec,plan,tasks}.md from docs/specs/_templates.
set -euo pipefail
cd "$(dirname "$0")/.."
n="${1:?usage: tools/new-feature.sh <NNNN> <slug>}"
slug="${2:?usage: tools/new-feature.sh <NNNN> <slug>}"
[[ "$n" =~ ^[0-9]{4}$ ]] || { echo "number must be 4 digits"; exit 1; }
dir="docs/specs/$n-$slug"
[ -e "$dir" ] && { echo "exists: $dir"; exit 1; }
mkdir -p "$dir"
for f in spec plan tasks; do
  sed "s/NNNN/$n/g" "docs/specs/_templates/$f.md" > "$dir/$f.md"
done
echo "created $dir"
