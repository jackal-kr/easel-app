#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The long test language (REQ-0023-11): each message as long as the longer of its German and Russian forms, in a catalog
that is never offered to users. po-long.py PO_DIR OUT_DIR STAMP writes OUT_DIR/xl/LC_MESSAGES/easel.mo, then the stamp
the build follows."""
import importlib.util
import subprocess
import sys
from pathlib import Path

spec = importlib.util.spec_from_file_location("ct", Path(__file__).with_name("check-translations.py"))
ct = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ct)


def quote(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def main():
    po_dir, out = Path(sys.argv[1]), Path(sys.argv[2])
    sources = [{e["msgid"]: e for e in ct.read_po(po_dir / (code + ".po")) if e.get("msgid")} for code in ("de", "ru")]
    lines = ['msgid ""', 'msgstr ""', '"Language: xl\\n"', '"MIME-Version: 1.0\\n"', '"Content-Type: text/plain; charset=UTF-8\\n"',
             '"Content-Transfer-Encoding: 8bit\\n"', '"Plural-Forms: nplurals=2; plural=(n != 1);\\n"', ""]
    for entry in ct.read_po(po_dir / "easel.pot"):
        if not entry.get("msgid"):
            continue
        forms = [s for source in sources for s in source[entry["msgid"]]["strs"].values()]
        longest = max(forms, key=len)
        if entry["flags"]:
            lines.append(entry["flags"])
        lines.append("msgid " + quote(entry["msgid"]))
        if "plural" in entry:
            lines += ["msgid_plural " + quote(entry["plural"]), "msgstr[0] " + quote(longest), "msgstr[1] " + quote(longest), ""]
        else:
            lines += ["msgstr " + quote(longest), ""]
    folder = out / "xl" / "LC_MESSAGES"
    folder.mkdir(parents=True, exist_ok=True)
    po = out / "xl.po"
    po.write_text("\n".join(lines), encoding="utf-8")
    subprocess.run(["msgfmt", "--check-format", "-o", str(folder / "easel.mo"), str(po)], check=True)
    Path(sys.argv[3]).touch()


main()
