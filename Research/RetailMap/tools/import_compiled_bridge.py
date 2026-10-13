#!/usr/bin/env python3
"""Add verified compiled/source bridge rows to a copy of the frozen v1 index.

This importer never changes matcher-v1 leads, scores, or reviewed identities.
Generated SQLite databases belong in ignored/private evidence storage.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sqlite3


V1_RUN = "match-154af00dbdc917d19140"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def read_records(path: Path):
    with path.open(encoding="utf-8") as stream:
        for number, line in enumerate(stream, 1):
            try:
                yield json.loads(line)
            except json.JSONDecodeError as error:
                raise ValueError(f"{path}:{number}: invalid JSON") from error


def preserve_v1(connection: sqlite3.Connection) -> None:
    row = connection.execute(
        "SELECT input_identity,metadata_sha256,item_count FROM index_runs WHERE run_id=?",
        (V1_RUN,),
    ).fetchone()
    if row is None or row[2] != 6758:
        raise ValueError("The expected frozen v1 matcher run/lead count is absent")
    actual = connection.execute("SELECT count(*) FROM match_candidates WHERE matcher_run_id=?",
                                (V1_RUN,)).fetchone()[0]
    if actual != 6758:
        raise ValueError("Frozen v1 candidate count differs from recorded run")
    connection.execute(
        "INSERT INTO matcher_versions VALUES (?,?,?,?,?,?)",
        (V1_RUN, "MATCHER_V1_UNVALIDATED", row[1].upper(),
         "exact string_hash/numeric_constant/symbol/import joins; old features and scores unchanged",
         row[0], "PROJECT_BIBLE.md §21; frozen v1 database SHA-256 2B49713841A9600663B4869BC97DD1994BF25D524FE53D0DF01916779B6F2007"),
    )


def import_bridge(connection: sqlite3.Connection, path: Path) -> dict:
    bridge_hash = sha256(path)
    records = iter(read_records(path))
    header = next(records, None)
    if not header or header.get("record") != "header" or header.get("schema") != "cfw.compiled-source-bridge.v1":
        raise ValueError(f"Incomplete compiled bridge header: {path}")
    module_hash = header["module_sha256"].upper()
    pe = header["pe"]
    pdb = header["pdb_guid_age"]
    connection.execute(
        "INSERT INTO binary_modules VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
        (module_hash, Path(header["module_path"]).name, "RECONSTRUCTED", header["configuration"],
         header["module_bytes"], pe["pe_timestamp"], pe["image_base"], pe["machine"],
         pe["linker_version"], header["manifest_sha256"].upper(),
         header["build_input_fingerprint"].upper(), header["pdb_sha256"].upper(),
         pdb["guid"].upper(), pdb["age"],
         f"compiled bridge SHA-256 {bridge_hash}; image/PDB RSDS verified by extractor"),
    )
    count = 0
    links = 0
    footer = None
    for row in records:
        if row.get("record") == "footer":
            footer = row
            break
        if row.get("record") != "function" or row.get("module_sha256", "").upper() != module_hash:
            raise ValueError("Unexpected bridge record or module identity")
        rva = int(row["rva"])
        extent = max((int(symbol["size"]) for symbol in row["pdb_symbols"]), default=None)
        connection.execute(
            "INSERT INTO compiled_functions VALUES (?,?,?,?,?,?,?,?,?)",
            (module_hash, rva, int(row["va"]), extent,
             json.dumps(row["pdb_symbols"], sort_keys=True, separators=(",", ":")),
             json.dumps(row["source_frames"], sort_keys=True, separators=(",", ":")),
             row["relationship"], row["identity_status"],
             f"{bridge_hash}:PDB-guid-age={pdb['guid']}/{pdb['age']}"),
        )
        name_matches = {candidate["source_id"]: candidate
                        for candidate in row["source_candidates"] if candidate["name_matches_pdb"]}
        for frame in row["source_frames"]:
            relative = frame.get("repository_relative")
            if not relative or not frame.get("source_checksum_verified"):
                continue
            for candidate in row["source_candidates"]:
                if not candidate["name_matches_pdb"] or candidate["source_file"].lower() != relative.lower():
                    continue
                if candidate["source_id"] not in name_matches:
                    continue
                if connection.execute("SELECT 1 FROM source_functions WHERE source_id=?",
                                      (candidate["source_id"],)).fetchone() is None:
                    raise ValueError(f"Source ID absent from retained lexical index: {candidate['source_id']}")
                cursor = connection.execute(
                    "INSERT OR IGNORE INTO compiled_source_links VALUES (?,?,?,?,?,?,?,?,?,?)",
                    (module_hash, rva, candidate["source_id"], candidate["source_file"],
                     frame.get("Line"), candidate["source_function"],
                     frame.get("pdb_source_sha256"), frame.get("current_source_sha256"),
                     row["relationship"], "PDB symbol + line + compiler SHA-256 + lexical name/range"),
                )
                links += max(cursor.rowcount, 0)
        count += 1
    if not footer or not footer.get("complete") or count != footer.get("compiled_function_rvas"):
        raise ValueError(f"Incomplete compiled bridge footer: {path}")
    if any(True for _ in records):
        raise ValueError("Unexpected content after bridge footer")
    return {"bridge_sha256": bridge_hash, "module_sha256": module_hash,
            "configuration": header["configuration"], "compiled_function_rvas": count,
            "source_links": links}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--db", required=True, type=Path,
                        help="Existing separate copy of frozen matcher-v1 SQLite database")
    parser.add_argument("--schema", required=True, type=Path)
    parser.add_argument("--bridge", required=True, type=Path, action="append")
    args = parser.parse_args()
    if not args.db.is_file():
        raise ValueError("Database must already exist as a separate copy of frozen v1")
    connection = sqlite3.connect(args.db)
    connection.execute("PRAGMA foreign_keys=ON")
    try:
        connection.executescript(args.schema.read_text(encoding="utf-8"))
        with connection:
            preserve_v1(connection)
            imported = [import_bridge(connection, path) for path in args.bridge]
        print(json.dumps({"database": str(args.db), "v1_status": "MATCHER_V1_UNVALIDATED",
                          "v1_leads_preserved": 6758, "imported": imported}, sort_keys=True))
    finally:
        connection.close()


if __name__ == "__main__":
    main()
