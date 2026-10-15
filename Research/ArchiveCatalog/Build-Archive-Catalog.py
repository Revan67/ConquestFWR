#!/usr/bin/env python3
"""Offline, read-only-input catalog of ConquestResearchArchive extracted views.

The JSONL file is the canonical result. The SQLite database is a query derivative.
This tool never opens a preserved executable as a program or extracts an archive.
"""

from __future__ import annotations

import argparse
import collections
import csv
import hashlib
import json
import os
from pathlib import Path
import sqlite3
import subprocess
import sys
import tempfile
from datetime import datetime, timezone


VERSION = "1.0.0"
VIEW = "11_Extracted_Views"
INPUTS = (
    "manifest.json", "extraction-summary.json", "extraction-report.json",
    "extraction-smart-install-maker-refinery.csv", "hashes.sha256",
)


def norm(value: object) -> str:
    if value is None:
        return ""
    result = str(value).replace("\\", "/")
    while result.startswith("./"):
        result = result[2:]
    return result


def key(value: object) -> str:
    return norm(value).casefold()


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(4 * 1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def stable_json(data: object) -> str:
    return json.dumps(data, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def atomic_bytes(path: Path, data: bytes) -> None:
    if path.is_symlink():
        raise RuntimeError(f"Refusing symlink output: {path}")
    fd, tmp = tempfile.mkstemp(prefix=".catalog-", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
        os.replace(tmp, path)
    finally:
        if os.path.exists(tmp):
            os.unlink(tmp)


def beneath(child: str, parent: str) -> bool:
    return key(child).startswith(key(parent).rstrip("/") + "/")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--root", type=Path, default=Path(r"D:\Dev\ConquestResearchArchive"))
    ap.add_argument("--verify-all", action="store_true", required=True,
                    help="hash every regular derived file from bytes (formal run)")
    ap.add_argument("--output-dir", type=Path, help="default: ROOT/00_Index")
    args = ap.parse_args()
    # pathlib.resolve() opens a Windows final-path handle and can be denied on
    # a read-only preservation root even when enumeration/read access works.
    root = args.root.absolute()
    view = root / VIEW
    index = root / "00_Index"
    out = (args.output_dir or index).absolute()
    if not view.is_dir() or not index.is_dir():
        raise RuntimeError("Archive view or index directory is absent")

    input_hashes = {name: digest(index / name) for name in INPUTS}
    manifest = json.loads((index / "manifest.json").read_text(encoding="utf-8-sig"))
    summary = json.loads((index / "extraction-summary.json").read_text(encoding="utf-8-sig"))
    json.loads((index / "extraction-report.json").read_text(encoding="utf-8-sig"))
    with (index / "extraction-smart-install-maker-refinery.csv").open(
            newline="", encoding="utf-8-sig") as stream:
        refinery = list(csv.DictReader(stream))

    anomalies: list[dict[str, str]] = []

    def anomaly(kind: str, path: str, detail: str) -> None:
        anomalies.append({"kind": kind, "path": path, "detail": detail})

    artifacts = {}
    for entry in manifest.get("files", []):
        p = norm(entry.get("path"))
        if p:
            if key(p) in artifacts and artifacts[key(p)] != entry:
                anomaly("manifest_duplicate_path", p, "manifest records disagree")
            artifacts[key(p)] = entry
    sha_list = {}
    for line in (index / "hashes.sha256").read_text(encoding="utf-8-sig").splitlines():
        parts = line.split(None, 1)
        if len(parts) == 2 and len(parts[0]) == 64:
            sha_list[key(parts[1].lstrip("* "))] = parts[0].upper()

    files = {}
    file_paths = {}
    initial_stat = {}
    unreadable = []
    bytes_hashed = 0
    for directory, dirs, names in os.walk(view, followlinks=False):
        dirs[:] = sorted((d for d in dirs if not (Path(directory) / d).is_symlink()),
                         key=lambda s: (s.casefold(), s))
        for name in sorted(names, key=lambda s: (s.casefold(), s)):
            path = Path(directory) / name
            if path.is_symlink() or not path.is_file():
                continue
            relative = path.relative_to(root).as_posix()
            k = key(relative)
            if k in files:
                anomaly("casefold_collision", relative, files[k]["relative_path"])
                continue
            before_stat = path.stat()
            size = before_stat.st_size
            try:
                file_hash = digest(path)
                bytes_hashed += size
            except OSError as exc:
                file_hash = ""
                unreadable.append(relative)
                anomaly("unreadable_derived_file", relative, str(exc))
            files[k] = {"relative_path": relative, "basename": name,
                        "extension": path.suffix.lower(), "size": size,
                        "sha256": file_hash}
            file_paths[k] = path
            after_stat = path.stat()
            initial_stat[k] = (size, after_stat.st_mtime_ns)
            if (before_stat.st_size, before_stat.st_mtime_ns) != initial_stat[k]:
                anomaly("changed_during_hash", relative, "size or modification time changed")

    # The on-disk metadata is the primary destination/source evidence. Reports
    # cross-check it, but do not manufacture a destination when none exists.
    containers = {}
    for file_record in files.values():
        if file_record["basename"].casefold() != "_extraction.json":
            continue
        meta_path = file_record["relative_path"]
        destination = norm(str(Path(meta_path).parent))
        try:
            meta = json.loads(file_paths[key(meta_path)].read_text(encoding="utf-8-sig"))
        except (UnicodeError, ValueError) as exc:
            anomaly("invalid_extraction_metadata", meta_path, str(exc))
            continue
        source = norm(meta.get("source"))
        declared_dest = norm(meta.get("destination"))
        issues = []
        if not source:
            issues.append("missing source path")
        if declared_dest and key(declared_dest) != key(destination):
            issues.append(f"destination differs: {declared_dest}")
        record = {"destination": destination, "source": source,
                  "source_sha256": str(meta.get("source_sha256") or "").upper(),
                  "source_size": meta.get("source_size"),
                  "method": meta.get("extraction_tool") or meta.get("tool") or
                            meta.get("extraction_kind") or "",
                  "metadata_path": meta_path, "issues": issues}
        if key(destination) in containers:
            issues.append("duplicate extraction metadata")
        containers[key(destination)] = record

    report_rows = [("summary", e) for e in summary if e.get("destination")]
    report_rows += [("refinery", e) for e in refinery if e.get("destination")]
    for origin, entry in report_rows:
        destination = norm(entry["destination"])
        source = norm(entry.get("source"))
        found = containers.get(key(destination))
        if not found:
            anomaly("report_without_metadata", destination, origin)
            continue
        if key(source) != key(found["source"]):
            found["issues"].append(f"{origin} source differs: {source}")
        report_hash = str(entry.get("source_sha256") or "").upper()
        if report_hash and found["source_sha256"] and report_hash != found["source_sha256"]:
            found["issues"].append(
                f"{origin} source hash differs: report={report_hash} metadata={found['source_sha256']}")

    # Every parent edge is resolved by an exact metadata source, never by a
    # filename or a guessed companion package.
    ordered_containers = sorted(containers.values(),
                                key=lambda c: (-len(c["destination"]), key(c["destination"])))
    root_cache = {}

    def observed_source(source: str):
        k = key(source)
        if not source or source.startswith("/") or ":" in source or ".." in source.split("/"):
            anomaly("unsafe_source_path", source, "source is not an archive-relative path")
            return "", None, False
        if k in files:
            return files[k]["sha256"], files[k]["size"], True
        candidate = root / source
        if source and not candidate.is_symlink() and candidate.is_file():
            if k not in root_cache:
                try:
                    root_cache[k] = (digest(candidate), candidate.stat().st_size)
                except OSError as exc:
                    anomaly("unreadable_source", source, str(exc))
                    return "", candidate.stat().st_size, False
            return (*root_cache[k], True)
        return "", None, False

    def enclosing(path: str):
        return next((c for c in ordered_containers if beneath(path, c["destination"])), None)

    resolved = {}

    def resolve(container: dict, visiting: frozenset[str] = frozenset()):
        destination = container["destination"]
        k = key(destination)
        if k in resolved:
            return resolved[k]
        issues = list(container["issues"])
        if k in visiting:
            issues.append("container cycle")
            return {"status": "CONFLICT", "root": "", "root_hash": "",
                    "lineage": [], "depth": 0, "issues": issues}
        source = container["source"]
        actual_hash, actual_size, exists = observed_source(source)
        stated_hash = container["source_sha256"]
        if stated_hash and actual_hash and stated_hash != actual_hash:
            issues.append(f"source hash differs: stated={stated_hash} actual={actual_hash}")
        stated_size = container["source_size"]
        if stated_size is not None and actual_size is not None and int(stated_size) != actual_size:
            issues.append(f"source size differs: stated={stated_size} actual={actual_size}")
        if not exists:
            anomaly("missing_source", destination, source)
        parent = enclosing(source) if source else None
        if parent and key(parent["destination"]) == k:
            issues.append("source lies inside its own destination")
            parent = None
        if parent:
            previous = resolve(parent, visiting | {k})
            root_path, root_hash = previous["root"], previous["root_hash"]
            lineage = [*previous["lineage"], source]
            depth = previous["depth"] + 1
            parent_status = previous["status"]
        else:
            artifact = artifacts.get(key(source))
            root_path = norm(artifact.get("path")) if artifact else ""
            root_hash = str(artifact.get("sha256") or "").upper() if artifact else ""
            lineage = [source] if source else []
            depth = 1 if source else 0
            parent_status = "VERIFIED" if artifact else "UNKNOWN"
            if source and not artifact:
                anomaly("source_not_in_preserved_manifest", destination, source)
            if artifact and root_hash and actual_hash and root_hash != actual_hash:
                issues.append(f"manifest hash differs: stated={root_hash} actual={actual_hash}")
            if artifact and sha_list.get(key(source)) and sha_list[key(source)] != root_hash:
                issues.append("hashes.sha256 differs from manifest")
        if issues:
            status = "CONFLICT"
        elif parent_status == "CONFLICT":
            status = "CONFLICT"
        elif not source or not root_path:
            status = "UNKNOWN" if not source else "PARTIAL"
        elif not exists or not actual_hash or not stated_hash or not root_hash or parent_status != "VERIFIED":
            status = "PARTIAL"
        else:
            status = "VERIFIED"
        result = {"status": status, "root": root_path, "root_hash": root_hash,
                  "lineage": lineage, "depth": depth, "issues": issues,
                  "source_actual_sha256": actual_hash}
        resolved[k] = result
        for issue in issues:
            anomaly("container_conflict", destination, issue)
        return result

    for container in ordered_containers:
        resolve(container)

    duplicates = collections.defaultdict(list)
    for record in files.values():
        if record["sha256"]:
            duplicates[(record["sha256"], record["size"])].append(record["relative_path"])
    group_ids = {pair: f"{pair[0]}:{pair[1]}" for pair, members in duplicates.items()
                 if len(members) > 1}

    records = []
    for record in files.values():
        path = record["relative_path"]
        container = enclosing(path)
        is_meta = record["basename"].casefold() == "_extraction.json"
        if container:
            chain = resolved[key(container["destination"])]
            status = chain["status"]
            if is_meta and status == "VERIFIED":
                # This catalog/extraction sidecar was not payload from source.
                status = "PARTIAL"
            record.update({
                "immediate_extraction_destination": container["destination"],
                "immediate_source_container": container["source"],
                "immediate_source_sha256": chain["source_actual_sha256"] or
                                          container["source_sha256"],
                "root_source_artifact": chain["root"],
                "root_source_sha256": chain["root_hash"],
                "extraction_depth": chain["depth"],
                "extraction_method": container["method"],
                "provenance_status": status,
                "container_lineage": chain["lineage"],
                "provenance_issues": [*chain["issues"],
                    *(["extraction metadata is a generated sidecar, not source payload"] if is_meta else [])],
            })
        else:
            anomaly("file_without_extraction_metadata", path, "no enclosing destination")
            record.update({"immediate_extraction_destination": "",
                           "immediate_source_container": "", "immediate_source_sha256": "",
                           "root_source_artifact": "", "root_source_sha256": "",
                           "extraction_depth": 0, "extraction_method": "",
                           "provenance_status": "UNKNOWN", "container_lineage": [],
                           "provenance_issues": ["no enclosing extraction metadata"]})
        if not record["sha256"]:
            status = "UNKNOWN" if record["provenance_status"] != "CONFLICT" else "CONFLICT"
            record["provenance_status"] = status
            record["provenance_issues"].append("byte hash unavailable")
        pair = (record["sha256"], record["size"])
        record["duplicate_group"] = group_ids.get(pair, "")
        records.append(record)
    records.sort(key=lambda r: (key(r["relative_path"]), r["relative_path"]))
    current_stat = {}
    for directory, dirs, names in os.walk(view, followlinks=False):
        dirs[:] = [d for d in dirs if not (Path(directory) / d).is_symlink()]
        for name in names:
            path = Path(directory) / name
            if path.is_symlink() or not path.is_file():
                continue
            stat = path.stat()
            current_stat[key(path.relative_to(root).as_posix())] = (stat.st_size, stat.st_mtime_ns)
    if current_stat != initial_stat:
        anomaly("input_tree_changed", VIEW,
                f"initial={len(initial_stat)} final={len(current_stat)} changed="
                f"{sum(initial_stat.get(k) != current_stat.get(k) for k in initial_stat.keys() | current_stat.keys())}")
    if {name: digest(index / name) for name in INPUTS} != input_hashes:
        anomaly("input_metadata_changed", "00_Index", "index evidence changed during run")
    anomalies.sort(key=lambda a: (key(a["path"]), a["kind"], a["detail"]))

    jsonl = b"".join((stable_json(r) + "\n").encode("utf-8") for r in records)
    jsonl_path = out / "archive-catalog.jsonl"
    atomic_bytes(jsonl_path, jsonl)
    csv_path = out / "archive-catalog-anomalies.csv"
    with tempfile.NamedTemporaryFile("w", newline="", encoding="utf-8", dir=out,
                                     prefix=".catalog-", delete=False) as tmp:
        csv_tmp = Path(tmp.name)
        writer = csv.DictWriter(tmp, fieldnames=["kind", "path", "detail"], lineterminator="\n")
        writer.writeheader()
        writer.writerows(anomalies)
    os.replace(csv_tmp, csv_path)
    jsonl_hash, anomaly_hash = digest(jsonl_path), digest(csv_path)
    counts = dict(sorted(collections.Counter(r["provenance_status"] for r in records).items()))
    git_head = "UNKNOWN"
    try:
        git_head = subprocess.check_output(
            ["git", "-C", str(Path(__file__).resolve().parents[2]), "rev-parse", "HEAD"],
            text=True, stderr=subprocess.DEVNULL).strip()
    except (OSError, subprocess.CalledProcessError):
        pass
    run = {"tool": "Build-Archive-Catalog.py", "tool_version": VERSION,
           "tool_git_commit": git_head, "tool_sha256": digest(Path(__file__)),
           "schema_version": 1,
           "configuration": {"verify_all": True, "view": VIEW, "hash_algorithm": "SHA-256"},
           "archive_root": str(root), "input_sha256": input_hashes,
           "extraction_metadata_count": len(containers),
           "extraction_metadata_sha256": digest_list(
               [(c["metadata_path"], files[key(c["metadata_path"])]["sha256"])
                for c in containers.values()]),
           "files_scanned": len(records), "bytes_cataloged": sum(r["size"] for r in records),
           "files_hashed": len(records) - len(unreadable), "bytes_hashed": bytes_hashed,
           "status_counts": counts, "jsonl_sha256": jsonl_hash,
           "anomaly_report_sha256": anomaly_hash, "anomaly_count": len(anomalies),
           "duplicate_group_count": len(group_ids),
           "complete_root_chains": sum(r["provenance_status"] == "VERIFIED" for r in records),
           "completed_utc": datetime.now(timezone.utc).isoformat(),
           "completion_status": ("INCOMPLETE_INPUT_CHANGED" if any(
               a["kind"] in ("input_tree_changed", "input_metadata_changed", "changed_during_hash")
               for a in anomalies) else
               "INCOMPLETE_UNREADABLE_FILES" if unreadable else "COMPLETE")}
    atomic_bytes(out / "archive-catalog-summary.json", (stable_json(run) + "\n").encode("utf-8"))
    write_sqlite(out / "archive-catalog.sqlite", manifest.get("files", []),
                 ordered_containers, resolved, records, duplicates, group_ids, run)
    print(stable_json({"files": len(records), "bytes": run["bytes_cataloged"],
                       "statuses": counts, "containers": len(containers),
                       "duplicate_groups": len(group_ids), "jsonl_sha256": jsonl_hash,
                       "anomalies": len(anomalies), "unreadable": len(unreadable),
                       "completion_status": run["completion_status"]}))
    return 0 if run["completion_status"] == "COMPLETE" else 2


def digest_list(items: list[tuple[str, str]]) -> str:
    data = "".join(f"{p}\t{h}\n" for p, h in sorted(items, key=lambda x: key(x[0])))
    return hashlib.sha256(data.encode("utf-8")).hexdigest().upper()


def write_sqlite(path, artifact_rows, containers, resolved, records, duplicates,
                 group_ids, run):
    fd, tmp_name = tempfile.mkstemp(prefix=".catalog-", suffix=".sqlite", dir=path.parent)
    os.close(fd)
    tmp = Path(tmp_name)
    try:
        db = sqlite3.connect(tmp)
        db.executescript("""
            PRAGMA journal_mode=DELETE;
            CREATE TABLE artifacts(path TEXT PRIMARY KEY, sha256 TEXT, size INTEGER, category TEXT);
            CREATE TABLE containers(destination TEXT PRIMARY KEY, source TEXT, source_sha256 TEXT,
                method TEXT, metadata_path TEXT, root_artifact TEXT, status TEXT, depth INTEGER);
            CREATE TABLE files(path TEXT PRIMARY KEY, basename TEXT, extension TEXT, size INTEGER,
                sha256 TEXT, destination TEXT, immediate_source TEXT, root_artifact TEXT,
                status TEXT, duplicate_group TEXT, lineage_json TEXT);
            CREATE TABLE provenance_edges(child_path TEXT, parent_path TEXT, relationship TEXT,
                PRIMARY KEY(child_path,parent_path,relationship));
            CREATE TABLE duplicate_groups(group_id TEXT PRIMARY KEY, sha256 TEXT, size INTEGER,
                member_count INTEGER, member_paths_json TEXT, root_artifacts_json TEXT);
            CREATE TABLE runs(id INTEGER PRIMARY KEY, run_json TEXT NOT NULL);
            CREATE INDEX files_basename ON files(basename COLLATE NOCASE);
            CREATE INDEX files_extension ON files(extension);
            CREATE INDEX files_sha256 ON files(sha256);
            CREATE INDEX files_root ON files(root_artifact);
            CREATE INDEX files_container ON files(immediate_source);
            CREATE INDEX files_status ON files(status);
            CREATE VIRTUAL TABLE files_fts USING fts5(path, basename, root_artifact,
                container_lineage, content='');
        """)
        db.executemany("INSERT INTO artifacts VALUES(?,?,?,?)", (
            (norm(e.get("path")), str(e.get("sha256") or "").upper(),
             e.get("size"), e.get("category")) for e in artifact_rows if e.get("path")))
        db.executemany("INSERT INTO containers VALUES(?,?,?,?,?,?,?,?)", (
            (c["destination"], c["source"], c["source_sha256"], c["method"],
             c["metadata_path"], resolved[key(c["destination"])]["root"],
             resolved[key(c["destination"])]["status"], resolved[key(c["destination"])]["depth"])
            for c in containers))
        for c in containers:
            db.execute("INSERT OR IGNORE INTO provenance_edges VALUES(?,?,?)",
                       (c["destination"], c["source"], "extracted_from"))
        for r in records:
            db.execute("INSERT INTO files VALUES(?,?,?,?,?,?,?,?,?,?,?)", (
                r["relative_path"], r["basename"], r["extension"], r["size"], r["sha256"],
                r["immediate_extraction_destination"], r["immediate_source_container"],
                r["root_source_artifact"], r["provenance_status"], r["duplicate_group"],
                stable_json(r["container_lineage"])))
            db.execute("INSERT INTO files_fts(rowid,path,basename,root_artifact,container_lineage) "
                       "VALUES((SELECT rowid FROM files WHERE path=?),?,?,?,?)", (
                r["relative_path"], r["relative_path"], r["basename"],
                r["root_source_artifact"], " ".join(r["container_lineage"])))
            if r["immediate_extraction_destination"]:
                db.execute("INSERT OR IGNORE INTO provenance_edges VALUES(?,?,?)",
                           (r["relative_path"], r["immediate_extraction_destination"],
                            "within_destination"))
        roots = {r["relative_path"]: r["root_source_artifact"] for r in records}
        db.executemany("INSERT INTO duplicate_groups VALUES(?,?,?,?,?,?)", (
            (group_ids[pair], pair[0], pair[1], len(members), stable_json(sorted(members)),
             stable_json(sorted({roots[m] for m in members if roots[m]})))
            for pair, members in duplicates.items() if pair in group_ids))
        db.execute("INSERT INTO runs(run_json) VALUES(?)", (stable_json(run),))
        db.commit()
        db.close()
        if path.is_symlink():
            raise RuntimeError(f"Refusing symlink output: {path}")
        os.replace(tmp, path)
    finally:
        if tmp.exists():
            tmp.unlink()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError, sqlite3.Error) as exc:
        print(f"archive catalog failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
