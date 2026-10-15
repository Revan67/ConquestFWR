#!/usr/bin/env python3
"""Read-only queries over the generated ConquestResearchArchive catalog."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sqlite3


CATALOG_SHA256 = "68737EC67D8E3E99FF45400236EB7C954B45E5B345C1C5A91B7A70C15BF80174"


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def bounded_search(db: sqlite3.Connection, args: argparse.Namespace) -> dict:
    if not 1 <= args.limit <= 100:
        raise SystemExit("--limit must be between 1 and 100")
    if args.offset < 0:
        raise SystemExit("--offset must be nonnegative")
    actual_sha = file_sha256(args.catalog_jsonl)
    if actual_sha != args.expect_catalog_sha256.upper():
        raise SystemExit(f"Catalog identity mismatch: {actual_sha}")
    run_row = db.execute("SELECT run_json FROM runs ORDER BY id DESC LIMIT 1").fetchone()
    if not run_row:
        raise SystemExit("Catalog SQLite has no provenance run")
    catalog_run = json.loads(run_row[0])
    if (catalog_run.get("jsonl_sha256") != actual_sha or
            catalog_run.get("completion_status") != "COMPLETE"):
        raise SystemExit("Catalog SQLite and canonical JSONL identity/status disagree")
    terms = args.term or []
    if not terms:
        raise SystemExit("At least one --term is required")
    field = "basename" if args.scope == "basename" else "path"
    predicate = " OR ".join(
        f"lower({field}) = ?" if args.exact else f"instr(lower({field}), ?) > 0"
        for _ in terms
    )
    values = [term.lower() for term in terms]
    where = f"({predicate})"
    if args.extension:
        where += " AND lower(extension) IN (" + ",".join("?" for _ in args.extension) + ")"
        values.extend(ext.lower() if ext.startswith(".") else "." + ext.lower()
                      for ext in args.extension)
    total = db.execute(f"SELECT count(*) FROM files WHERE {where}", values).fetchone()[0]
    roots = db.execute(f"SELECT count(DISTINCT root_artifact) FROM files WHERE {where}", values).fetchone()[0]
    groups = db.execute(
        f"SELECT count(DISTINCT sha256 || ':' || size) FROM files WHERE {where}", values
    ).fetchone()[0]
    select_sql = (
        "SELECT path,basename,extension,sha256,size,root_artifact,status,"
        "duplicate_group,destination,immediate_source,lineage_json FROM files WHERE "
        + where + " ORDER BY path"
    )
    if args.dedupe_content:
        matched_rows = rows(db, select_sql, values)
        content_groups = {}
        for record in matched_rows:
            key = (record["sha256"], record["size"])
            if key not in content_groups:
                content_groups[key] = {"representative": record, "member_count": 0,
                                       "root_artifacts": set()}
            content_groups[key]["member_count"] += 1
            content_groups[key]["root_artifacts"].add(record["root_artifact"])
        representatives = []
        for group in content_groups.values():
            record = group["representative"]
            record["matching_duplicate_count"] = group["member_count"]
            record["matching_root_artifacts"] = sorted(group["root_artifacts"])
            representatives.append(record)
        result_rows = representatives[args.offset:args.offset + args.limit]
    else:
        result_rows = rows(db, select_sql + " LIMIT ? OFFSET ?",
                           (*values, args.limit, args.offset))
    for record in result_rows:
        record["extraction_lineage"] = json.loads(record.pop("lineage_json"))
    return {
        "schema_version": 1,
        "tool": "ArchiveQuery.py",
        "tool_version": "1.1.1",
        "tool_sha256": file_sha256(Path(__file__)),
        "catalog_jsonl_sha256": actual_sha,
        "catalog_sqlite_sha256": file_sha256(args.db),
        "completion_status": "COMPLETE",
        "query": {"terms": terms, "scope": args.scope, "exact": args.exact,
                  "extensions": args.extension or [], "limit": args.limit,
                  "offset": args.offset, "dedupe_content": args.dedupe_content},
        "total_hits": total,
        "returned_hits": len(result_rows),
        "distinct_root_artifacts": roots,
        "distinct_content_hashes": groups,
        "rows": result_rows,
    }


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
    ap.add_argument("--catalog-jsonl", type=Path,
                    default=Path(r"D:\Dev\ConquestResearchArchive\00_Index\archive-catalog.jsonl"))
    ap.add_argument("--expect-catalog-sha256", default=CATALOG_SHA256)
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
    search = sub.add_parser("search", help="Bounded, provenance-pinned metadata search")
    search.add_argument("--term", action="append", required=True)
    search.add_argument("--scope", choices=("basename", "path"), default="basename")
    search.add_argument("--exact", action="store_true")
    search.add_argument("--extension", action="append")
    search.add_argument("--limit", type=int, default=25)
    search.add_argument("--offset", type=int, default=0)
    search.add_argument("--dedupe-content", action="store_true",
                        help="Return one representative per SHA-256/size while retaining group counts")
    search.add_argument("--output", type=Path,
                        help="Create a new machine-readable result; never overwrite an earlier run")
    args = ap.parse_args()
    with connect(args.db) as db:
        if args.command == "search":
            output = bounded_search(db, args)
        elif args.command == "find":
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
    serialized = json.dumps(output, ensure_ascii=False, sort_keys=True, indent=2) + "\n"
    if args.command == "search" and args.output:
        with args.output.open("x", encoding="utf-8", newline="\n") as stream:
            stream.write(serialized)
    else:
        print(serialized, end="")


if __name__ == "__main__":
    main()
