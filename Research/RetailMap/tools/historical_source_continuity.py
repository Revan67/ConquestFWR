#!/usr/bin/env python3
"""Bounded six-file, read-only historical-mirror/current source delta index.

Reports hashes and line-delta locations, not copied historical source. Changed
files remain UNKNOWN until a human classifies the relevant hunks.
"""
from __future__ import annotations

import argparse
import difflib
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[3]
PRIVATE = ROOT / "Evidence/RetailMap/local"
MIRROR = Path("D:/Dev/ConquestResearchArchive/01_Source/Trass3r-ConquestFrontierWars.git")
PAIRS = [
    ("Trim.cpp", "src/app/rrc/Trim.cpp", "Code/App/Src/Trim.cpp"),
    ("Menu_Toolbar.cpp", "src/app/rrc/Menu_Toolbar.cpp", "Code/App/Src/Menu_Toolbar.cpp"),
    ("Menu_AdmiralBar.cpp", "src/app/rrc/Menu_AdmiralBar.cpp", "Code/App/Src/Menu_AdmiralBar.cpp"),
    ("IAdmiral.h", "src/app/rrc/IAdmiral.h", "Code/App/Src/IAdmiral.h"),
    ("FlagShip.cpp", "src/app/rrc/FlagShip.cpp", "Code/App/Src/FlagShip.cpp"),
    ("DBaseData.h", "src/app/dinclude/DBaseData.h", "Code/App/DInclude/DBaseData.h"),
]
FEATURES = {
    "class_or_struct": re.compile(rb"\b(?:class|struct)\s+[A-Za-z_][A-Za-z_0-9]*"),
    "callback": re.compile(rb"callback|notify|event|dispatch", re.I),
    "resource_or_control": re.compile(rb"resource|control|button|toolbar|menu", re.I),
    "vtable_or_virtual": re.compile(rb"vtable|virtual", re.I),
    "include": re.compile(rb"^\s*#\s*include", re.I),
}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def git(*args: str) -> bytes:
    return subprocess.check_output(["git", "--git-dir=" + str(MIRROR), *args])


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() or PRIVATE.resolve() not in output.parents:
        parser.error("output must be a new private evidence file")
    mirror_head = git("rev-parse", "HEAD").decode().strip()
    rows = []
    for name, historical, current in PAIRS:
        old = git("show", f"{mirror_head}:{historical}")
        new = (ROOT / current).read_bytes()
        # Windows checkout bytes can be CRLF while both paths have the same
        # canonical Git blob. Compare line content, retain raw byte hashes.
        old_lines, new_lines = old.replace(b"\r\n", b"\n").splitlines(keepends=True), new.replace(b"\r\n", b"\n").splitlines(keepends=True)
        changes = []
        for op, i, j, k, l in difflib.SequenceMatcher(None, old_lines, new_lines, autojunk=False).get_opcodes():
            if op == "equal":
                continue
            changed = b"".join(old_lines[i:j] + new_lines[k:l])
            changes.append({"kind": op, "historical_lines_1based": [i + 1, j],
                            "current_lines_1based": [k + 1, l],
                            "feature_terms": [label for label, pattern in FEATURES.items() if pattern.search(changed)]})
        historical_blob = git("rev-parse", f"{mirror_head}:{historical}").decode().strip()
        current_blob = subprocess.check_output(["git", "hash-object", str(ROOT / current)], cwd=ROOT).decode().strip()
        rows.append({"name": name, "historical_path": historical, "current_path": current,
                     "historical_sha256": sha(old), "current_sha256": sha(new),
                     "historical_git_blob": historical_blob, "current_git_blob": current_blob,
                     "classification": "UNCHANGED" if historical_blob == current_blob else "UNKNOWN",
                     "raw_checkout_bytes_equal": old == new,
                     "normalized_line_content_equal": old_lines == new_lines,
                     "historical_lines": len(old_lines), "current_lines": len(new_lines),
                     "line_change_groups": changes})
    result = {"schema": "cfw.retailmap.historical-source-continuity.v1",
              "status": "COMPLETE_HASH_DELTA_ONLY_NO_RETAIL_AUTHORITY",
              "tool_sha256": sha(Path(__file__).read_bytes()), "python_version": sys.version.split()[0],
              "project_head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
              "mirror_head": mirror_head, "mirror_git_directory": str(MIRROR),
              "rows": rows,
              "limitations": ["Mirror identity is historical source, not shipped retail machine behavior.",
                              "Changed-hunk semantic classification requires review.",
                              "Line diff normalizes CRLF only; other whitespace can appear as changes."]}
    with output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(result, stream, sort_keys=True, separators=(",", ":"))
        stream.write("\n")
    print(json.dumps({"output": str(output), "sha256": sha(output.read_bytes()),
                      "unchanged": sum(x["classification"] == "UNCHANGED" for x in rows),
                      "changed": sum(x["classification"] != "UNCHANGED" for x in rows)}, sort_keys=True))


if __name__ == "__main__":
    main()
