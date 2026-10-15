#!/usr/bin/env python3
"""Read-only queries over the generated ConquestResearchArchive catalog."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sqlite3


def connect(path: Path) -> sqlite3.Connection:
    db = sqlite3.connect(f"file:{path.absolute()}?mode=ro", uri=True)
    db.row_factory = sqlite3.Row
    db.execute("PRAGMA query_only=ON")
    return db


def rows(db: sqlite3.Connection, sql: str, values=()) -> list[dict]:
    return [dict(row) for row in db.execute(sql, values)]


def canonical_path(db: sqlite3.Connection, supplied: str) -> str:
    value = supplied.replace("\\", "/").lstrip("./")
    row = db.execute("SELECT path FROM files WHERE path=? COLLATE NOCASE", (value,)).fetchone()
    if row:
        return row[0]
    row = db.execute("SELECT path FROM files WHERE path=? COLLATE NOCASE",
                     ("11_Extracted_Views/" + value,)).fetchone()
    if not row:
        raise SystemExit(f"File not found: {supplied}")
    return row[0]


def lineage(db: sqlite3.Connection, path: str) -> dict:
    record = rows(db, "SELECT * FROM files WHERE path=?", (path,))[0]
    chain = json.loads(record["lineage_json"])
    record["lineage"] = [
        {"source": item,
         "sha256": (db.execute("SELECT sha256 FROM files WHERE path=?", (item,)).fetchone() or
                    db.execute("SELECT sha256 FROM artifacts WHERE path=?", (item,)).fetchone() or
                    [None])[0]}
        for item in chain
    ]
    return record


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--db", type=Path,
                    default=Path(r"D:\Dev\ConquestResearchArchive\00_Index\archive-catalog.sqlite"))
    sub = ap.add_subparsers(dest="command", required=True)
    for command, arguments in {
        "find": ["basename"], "path": ["pattern"], "hash": ["sha256"],
        "lineage": ["relative_path"], "duplicates": ["relative_path"],
        "package": ["source_artifact"],
        "compare": ["source_artifact_a", "source_artifact_b"],
    }.items():
        parser = sub.add_parser(command)
        for argument in arguments:
            parser.add_argument(argument)
    args = ap.parse_args()
    with connect(args.db) as db:
        if args.command == "find":
            output = rows(db, "SELECT * FROM files WHERE basename=? COLLATE NOCASE ORDER BY path",
                          (args.basename,))
        elif args.command == "path":
            pattern = args.pattern.replace("*", "%").replace("?", "_")
            if not any(c in pattern for c in "%_"):
                pattern = "%" + pattern + "%"
            output = rows(db, "SELECT * FROM files WHERE path LIKE ? ORDER BY path", (pattern,))
        elif args.command == "hash":
            output = rows(db, "SELECT * FROM files WHERE sha256=? ORDER BY path",
                          (args.sha256.upper(),))
        elif args.command == "lineage":
            output = lineage(db, canonical_path(db, args.relative_path))
        elif args.command == "duplicates":
            path = canonical_path(db, args.relative_path)
            row = db.execute("SELECT duplicate_group FROM files WHERE path=?", (path,)).fetchone()
            output = rows(db, "SELECT * FROM duplicate_groups WHERE group_id=?", (row[0],)) if row[0] else []
        elif args.command == "package":
            output = rows(db, "SELECT * FROM files WHERE root_artifact=? COLLATE NOCASE ORDER BY path",
                          (args.source_artifact.replace("\\", "/"),))
        else:
            a = args.source_artifact_a.replace("\\", "/")
            b = args.source_artifact_b.replace("\\", "/")
            file_a = rows(db, "SELECT path,sha256,size FROM files WHERE root_artifact=? COLLATE NOCASE", (a,))
            file_b = rows(db, "SELECT path,sha256,size FROM files WHERE root_artifact=? COLLATE NOCASE", (b,))
            hashes_a = {(x["sha256"], x["size"]) for x in file_a}
            hashes_b = {(x["sha256"], x["size"]) for x in file_b}
            output = {"a": a, "b": b, "a_file_count": len(file_a), "b_file_count": len(file_b),
                      "shared_content_groups": len(hashes_a & hashes_b),
                      "a_only_content_groups": len(hashes_a - hashes_b),
                      "b_only_content_groups": len(hashes_b - hashes_a)}
    print(json.dumps(output, ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == "__main__":
    main()
