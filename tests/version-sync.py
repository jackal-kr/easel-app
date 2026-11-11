#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The version is one in meson.build, the deb's changelog and the metainfo's newest release (a ~ for the deb's and AppStream's
pre-release, a - for Meson's): the 0.1.0-beta.1 bump left the deb at 0.1.0~alpha.1, which tools/package.sh would have built and
then failed to find (2026-11-11). Usage: version-sync.py PROJECT_ROOT"""
import re, sys

root = sys.argv[1]
meson = re.search(r"^  version: '([^']+)',$", open(f"{root}/meson.build").read(), re.M).group(1)
want = meson.replace('-', '~', 1)
deb = re.match(r"easel \(([^)]+)\)", open(f"{root}/debian/changelog").read()).group(1)
meta = re.search(r'<release version="([^"]+)"', open(f"{root}/data/io.github.jackal_kr.Easel.metainfo.xml.in").read()).group(1)
bad = [f"{name} has {got}, meson.build {meson} (want {want})" for name, got in (("debian/changelog", deb), ("metainfo", meta)) if got != want]
for b in bad:
    print(b)
sys.exit(1 if bad else 0)
