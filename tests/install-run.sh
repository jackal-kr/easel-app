#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# AS-8 (REQ-0024-09): installed under a prefix, every language's catalog and Help page, the entry, the metadata and the icons are
# where the installed program looks. $1 the source folder, $2 the build folder.
set -e
root=$(mktemp -d)
trap 'rm -rf "$root"' EXIT
DESTDIR="$root" meson install -C "$2" --no-rebuild --quiet
prefix=$(meson introspect "$2" --buildoptions | python3 -c "import json,sys; print([o['value'] for o in json.load(sys.stdin) if o['name']=='prefix'][0])")
share="$root$prefix/share"
for code in cs de es fr hu it pl pt ro ru uk; do
  test -f "$share/locale/$code/LC_MESSAGES/easel.mo" || { echo "no catalog for $code"; exit 1; }
  test -f "$share/easel/help/$code/index.html" || { echo "no Help page for $code"; exit 1; }
done
test -f "$share/easel/help/index.html"
test -x "$root$prefix/bin/easel"
test -f "$share/applications/io.github.jackal_kr.Easel.desktop"
test -f "$share/metainfo/io.github.jackal_kr.Easel.metainfo.xml"
test -f "$share/icons/hicolor/scalable/apps/io.github.jackal_kr.Easel.svg"
test -f "$share/icons/hicolor/symbolic/apps/io.github.jackal_kr.Easel-symbolic.svg"
if find "$root" -path '*xl*' -name 'easel.mo' | grep -q .; then echo "the long test language was installed"; exit 1; fi
