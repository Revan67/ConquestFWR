#!/usr/bin/env python3
"""Import one independently analyzed retail module into a private map database.

The input is metadata, not disassembly or decompiled code. The module identity is
its complete SHA-256 plus image-relative RVA; existing v1 runs are never edited.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import sqlite3

import pefile


CANONICAL_TRIM = "AC13625FEF6DA8D553196C894351E93F842BD8FB5344B97FEBA1FD6835E830F0"


def digest(path: Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            hasher.update(block)
    return hasher.hexdigest().upper()


def rows(path: Path):
    with path.open(encoding="utf-8") as stream:
        for number, line in enumerate(stream, 1):
            try:
                yield json.loads(line)
            except json.JSONDecodeError as error:
                raise ValueError(f"Invalid JSON {path}:{number}") from error


def address(value: str) -> int:
    return int(str(value).removeprefix("0x"), 16)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--db", required=True, type=Path)
    parser.add_argument("--metadata", required=True, type=Path)
    parser.add_argument("--module", required=True, type=Path)
    args = parser.parse_args()
    if not args.db.is_file() or not args.metadata.is_file() or not args.module.is_file():
        raise ValueError("Database, metadata, and retail module must already exist")
    if digest(args.module) != CANONICAL_TRIM:
        raise ValueError("Retail module does not match canonical Trim.dll SHA-256")
    metadata_hash = digest(args.metadata)
    stream = iter(rows(args.metadata))
    header = next(stream, None)
    if (not header or header.get("record") != "header" or
            header.get("schema") != "cfw.retail-metadata.v1" or
            header.get("image_sha256", "").upper() != CANONICAL_TRIM):
        raise ValueError("Metadata header does not identify canonical retail Trim.dll")
    base = address(header["image_base"])
    image = pefile.PE(str(args.module), fast_load=True)
    if image.FILE_HEADER.Machine != 0x14C or image.OPTIONAL_HEADER.ImageBase != base:
        raise ValueError("Retail UI metadata and PE architecture/image base disagree")
    run_id = "retail-module-" + metadata_hash[:20].lower()
    connection = sqlite3.connect(args.db)
    connection.execute("PRAGMA foreign_keys=ON")
    count = 0
    pointer_tables = 0
    footer = None
    try:
        with connection:
            connection.execute(
                "INSERT INTO index_runs VALUES (?,?,?,?,?,?,?)",
                (run_id, "retail", datetime.now(timezone.utc).isoformat(),
                 f"Trim.dll:{CANONICAL_TRIM}:base={base:X}", metadata_hash,
                 0, "independent saved-analysis UI module; no matcher run"),
            )
            connection.execute(
                "INSERT INTO binary_modules VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                (CANONICAL_TRIM, "Trim.dll", "RETAIL", None, args.module.stat().st_size,
                 image.FILE_HEADER.TimeDateStamp, base, "IMAGE_FILE_MACHINE_I386",
                 f"{image.OPTIONAL_HEADER.MajorLinkerVersion}.{image.OPTIONAL_HEADER.MinorLinkerVersion}",
                 None, None, None, None, None,
                 f"canonical retail module SHA-256 verified; Ghidra metadata {metadata_hash}"),
            )
            for row in stream:
                if row.get("record") == "footer":
                    footer = row
                    break
                if row.get("record") == "pointer_table_candidate":
                    connection.execute(
                        "INSERT INTO retail_pointer_table_candidates VALUES (?,?,?,?)",
                        (CANONICAL_TRIM, address(row["address"]),
                         json.dumps(row.get("targets", []), separators=(",", ":")),
                         f"independent Ghidra metadata {metadata_hash}; pointer table, not vtable proof"),
                    )
                    pointer_tables += 1
                    continue
                if row.get("record") != "function":
                    raise ValueError(f"Unexpected retail metadata record {row.get('record')}")
                va = address(row["entry"])
                rva = va - base
                if rva < 0 or address(row["rva"]) != rva:
                    raise ValueError(f"Invalid image-relative function address {va:X}")
                retail_id = f"{CANONICAL_TRIM.lower()}:0x{rva:08x}"
                if row.get("retail_id") != retail_id:
                    raise ValueError(f"Metadata identity mismatch at {va:X}")
                provenance = row.get("function_provenance")
                if provenance != "SAVED_ANALYSIS_FUNCTION":
                    raise ValueError("UI module importer accepts saved-analysis Functions only")
                name = row.get("name")
                recovered = name if row.get("name_source", "").upper() not in {"", "DEFAULT", "ANALYSIS"} else None
                connection.execute(
                    "INSERT INTO retail_functions VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                    (retail_id, run_id, CANONICAL_TRIM, "Trim.dll", base, va, rva,
                     row.get("body_size"), name, recovered, row.get("signature"),
                     row.get("calling_convention"), row.get("parameter_count"),
                     row.get("block_count"), row.get("instruction_count"),
                     row.get("cyclomatic_complexity"), row.get("body_sha256"),
                     row.get("instruction_shape_sha256"), row.get("pseudocode_fingerprint"),
                     provenance),
                )
                for edge in row.get("call_edges", []):
                    connection.execute(
                        "INSERT OR IGNORE INTO retail_call_edges VALUES (?,?,?,?,?)",
                        (retail_id, address(edge["site"]), str(edge["target"]),
                         str(edge.get("type", "UNKNOWN")), edge.get("external_name")),
                    )
                count += 1
            if not footer or not footer.get("complete") or count != footer.get("functions"):
                raise ValueError("UI metadata is incomplete or footer count disagrees")
            if any(True for _ in stream):
                raise ValueError("Unexpected content after UI metadata footer")
            connection.execute("UPDATE index_runs SET item_count=? WHERE run_id=?", (count, run_id))
        print(json.dumps({"run_id": run_id, "module_sha256": CANONICAL_TRIM,
                          "metadata_sha256": metadata_hash, "saved_functions": count,
                          "pointer_table_candidates": pointer_tables}, sort_keys=True))
    finally:
        connection.close()


if __name__ == "__main__":
    main()
