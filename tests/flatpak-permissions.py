#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""AS-9 (REQ-0024-10): the manifest asks for a display and its acceleration, and nothing else: no home folder, no network."""
import re
import sys

text = open(sys.argv[1], encoding="utf-8").read()
block = re.search(r"^finish-args:\n((?:\s+- .*\n)+)", text, re.M)
if block is None:
    sys.exit("no finish-args")
asked = set(re.findall(r"-\s*(--\S+)", block.group(1)))
want = {"--share=ipc", "--socket=fallback-x11", "--socket=wayland", "--device=dri"}
if asked != want:
    sys.exit(f"asked {sorted(asked)}, want {sorted(want)}")
