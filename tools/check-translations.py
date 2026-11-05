#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Every translation of Easel, checked (REQ-0023-07, -10): msgfmt's own checks, every message there and none empty or uncertain, each language's plural rule, and its
review mark. check-translations.py --all PO_DIR, or --one FILE LANG POT for a file alone, or --refuse DIR for the bad examples, each of which must fail."""
import re
import subprocess
import sys
from pathlib import Path

LANGUAGES = ["cs", "de", "es", "fr", "hu", "it", "pl", "pt", "ro", "ru", "uk"]
REVIEWED_BY_JOD = {"pl", "ru", "uk"}
TWO = "nplurals=2; plural=(n != 1);"
TWO_FR = "nplurals=2; plural=(n > 1);"
RULES = {
    "de": TWO, "es": TWO, "hu": TWO, "it": TWO,
    "fr": TWO_FR, "pt": TWO_FR,
    "cs": "nplurals=3; plural=(n==1 ? 0 : n>=2 && n<=4 ? 1 : 2);",
    "pl": "nplurals=3; plural=(n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);",
    "ro": "nplurals=3; plural=(n==1 ? 0 : n==0 || (n%100>0 && n%100<20) ? 1 : 2);",
    "ru": "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);",
    "uk": "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);",
}


def unquote(line):
    return line[1:-1].replace('\\"', '"').replace("\\n", "\n").replace("\\\\", "\\")


def read_po(path):
    """A list of entries: flags, msgid, msgid_plural, msgstr forms."""
    entries, entry, field = [], None, None
    for raw in Path(path).read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#~"):
            continue
        if line.startswith("#,"):
            if entry is None or "msgid" in entry:
                entry = {"flags": "", "strs": {}}
                entries.append(entry)
            entry["flags"] += line
            continue
        if line.startswith("#"):
            continue
        if line.startswith("msgid_plural "):
            field = "plural"
            entry["plural"] = unquote(line[13:])
        elif line.startswith("msgid "):
            if entry is None or "msgid" in entry:
                entry = {"flags": "", "strs": {}}
                entries.append(entry)
            field = "msgid"
            entry["msgid"] = unquote(line[6:])
        elif line.startswith("msgstr"):
            match = re.match(r"msgstr(?:\[(\d+)\])? (.*)", line)
            field = int(match.group(1) or 0)
            entry["strs"][field] = unquote(match.group(2))
        elif isinstance(field, int):
            entry["strs"][field] += unquote(line)
        else:
            entry[field] += unquote(line)
    return entries


def problems(po, lang, pot):
    found = []
    if lang not in RULES:
        return [f"{lang}: no rule"]
    run = subprocess.run(["msgfmt", "--check", "--check-format", "-o", "/dev/null", str(po)], capture_output=True, text=True)
    if run.returncode != 0:
        found.append(f"{po}: msgfmt: {run.stderr.strip()[:200]}")
    entries = read_po(po)
    header = next((e for e in entries if e.get("msgid") == ""), None)
    head = header["strs"].get(0, "") if header else ""
    rule = re.search(r"Plural-Forms: (.*)", head)
    if rule is None or rule.group(1).strip() != RULES[lang]:
        found.append(f"{po}: its plural rule is not {lang}'s")
    forms = int(re.search(r"nplurals=(\d)", RULES[lang]).group(1))
    if not re.search(r"X-Easel-Reviewed: (no|yes \d{4}-\d{2}-\d{2})", head):
        found.append(f"{po}: no review mark")
    elif "X-Easel-Reviewed: yes" in head and lang not in REVIEWED_BY_JOD:
        found.append(f"{po}: only ru, uk and pl are reviewed")
    have = {e["msgid"]: e for e in entries if e.get("msgid")}
    for wanted in read_po(pot):
        msgid = wanted.get("msgid")
        if not msgid:
            continue
        entry = have.get(msgid)
        if entry is None:
            found.append(f"{po}: missing: {msgid!r}")
        elif "fuzzy" in entry["flags"]:
            found.append(f"{po}: uncertain: {msgid!r}")
        elif any(not text for text in entry["strs"].values()):
            found.append(f"{po}: empty: {msgid!r}")
        elif "plural" in entry and len(entry["strs"]) != forms:
            found.append(f"{po}: {msgid!r} has {len(entry['strs'])} forms, not {forms}")
    return found


def main():
    if sys.argv[1] == "--one":
        found = problems(Path(sys.argv[2]), sys.argv[3], sys.argv[4])
    elif sys.argv[1] == "--refuse":
        folder = Path(sys.argv[2])
        found = problems(folder / "good-ru.po", "ru", folder / "test.pot")
        for bad in sorted(folder.glob("bad-*-ru.po")):
            if not problems(bad, "ru", folder / "test.pot"):
                found.append(f"{bad}: not refused")
    else:
        folder = Path(sys.argv[2])
        found = []
        listed = (folder / "LINGUAS").read_text().split()
        for lang in LANGUAGES:
            if lang not in listed:
                found.append(f"{lang}: not in LINGUAS")
            elif not (folder / f"{lang}.po").exists():
                found.append(f"{lang}: no translation")
            else:
                found += problems(folder / f"{lang}.po", lang, folder / "easel.pot")
    for line in found:
        print(line)
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
