#!/usr/bin/env python3
"""Import private metadata and rank correspondence leads, never retail code.

Automated ranks are search aids, not accepted function identities or parity claims.
The SQLite output should stay in local/private evidence storage until reviewed.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import sqlite3

CANONICAL_EXE_SHA256 = "ED8741C0CB727A43B7DE7BE0E73566B04FBC1CBD58B54A62D538E3F39DCB3F52"
COMMON_NUMBERS = {0, 1, 2, 3, 4, 8, 16, 32, 64, 128, 255, 256, 512, 1024, -1}


def sha256_file(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


def records(path: Path):
    with path.open("r", encoding="utf-8") as stream:
        for number, line in enumerate(stream, 1):
            if line.strip():
                try:
                    yield json.loads(line)
                except json.JSONDecodeError as exc:
                    raise ValueError(f"{path}:{number}: {exc}") from exc


def run_id(kind: str, path: Path) -> str:
    return f"{kind}-{sha256_file(path)[:20]}"


def init_db(db: Path, schema: Path) -> None:
    db.parent.mkdir(parents=True, exist_ok=True)
    with sqlite3.connect(db) as connection:
        connection.executescript(schema.read_text(encoding="utf-8"))


def parse_address(value) -> int:
    if isinstance(value, int):
        return value
    if value is None:
        raise ValueError("Missing address")
    text = str(value).strip()
    return int(text, 16) if re.fullmatch(r"(?:0[xX])?[0-9a-fA-F]+", text) else int(text)


def feature(connection, kind: str, owner_id: str, category: str, value, provenance="extractor") -> None:
    if value is None or value == "":
        return
    connection.execute(
        "INSERT OR IGNORE INTO features VALUES (?,?,?,?,?)",
        (kind, owner_id, category, str(value), provenance),
    )


def source_literal_hash(spelling: str) -> str | None:
    """Conservatively hash only plain ASCII C++ literals without escapes."""
    match = re.fullmatch(r'"([^"\\]*)"', spelling)
    if not match or not match[1].isascii():
        return None
    return hashlib.sha256(match[1].encode("utf-8")).hexdigest().lower()


def import_basename(value: str) -> str:
    name = value.rsplit("::", 1)[-1]
    if len(name) > 2 and name[-1] in "AW" and name[-2].islower():
        name = name[:-1]
    return name


def integer_value(spelling) -> int | None:
    if isinstance(spelling, int):
        return spelling
    text = re.sub(r"[uUlL]+$", "", str(spelling).strip())
    if re.fullmatch(r"\d+:0[xX][0-9a-fA-F]+", text):
        text = text.split(":", 1)[1]
    try:
        return int(text, 0)
    except ValueError:
        return None


def import_retail(connection, path: Path) -> tuple[str, int]:
    stream = iter(records(path))
    header = next(stream, None)
    if not header or header.get("record") != "header":
        raise ValueError("Retail metadata is missing its header")
    image_hash = str(header.get("image_sha256", "")).upper()
    if image_hash != CANONICAL_EXE_SHA256:
        raise ValueError(f"Retail image identity differs from canonical EXE: {image_hash}")
    base = parse_address(header.get("image_base"))
    module = str(header.get("image_name", "Conquest_retail.exe"))
    identity = f"{module}:{image_hash}:base={base:X}"
    rid = run_id("retail", path)
    count = 0
    footer = None
    with connection:
        connection.execute("INSERT INTO index_runs VALUES (?,?,?,?,?,?,?)",
                           (rid, "retail", utc_now(), identity, sha256_file(path), 0, "metadata-only Ghidra export"))
        for row in stream:
            if row.get("record") == "footer":
                footer = row
                break
            if row.get("record") == "pointer_table_candidate":
                connection.execute(
                    "INSERT INTO retail_pointer_table_candidates VALUES (?,?,?,?)",
                    (image_hash, parse_address(row["address"]),
                     json.dumps(row.get("targets", []), separators=(",", ":")),
                     "Ghidra saved-analysis pointers to known Functions; not vtable proof"),
                )
                continue
            if row.get("record") != "function":
                continue
            address = parse_address(row.get("entry"))
            if address < base:
                raise ValueError(f"Function below image base: {address:X}")
            fid = f"{image_hash[:16]}:{address:X}"
            provenance = row.get("function_provenance", "SAVED_ANALYSIS_FUNCTION")
            if provenance not in {"SAVED_ANALYSIS_FUNCTION", "EPHEMERAL_RECOVERED_CANDIDATE"}:
                raise ValueError(f"Unknown retail function provenance at {address:X}: {provenance}")
            source_name = str(row.get("name_source", ""))
            name = str(row.get("name", ""))
            recovered = name if source_name and source_name.upper() not in {"DEFAULT", "ANALYSIS"} else None
            connection.execute(
                "INSERT INTO retail_functions VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                (fid, rid, image_hash, module, base, address, address - base,
                 row.get("body_size"), name, recovered, row.get("signature"),
                 row.get("calling_convention"), row.get("parameter_count"),
                 row.get("block_count"), row.get("instruction_count"),
                 row.get("cyclomatic_complexity"), row.get("body_sha256"),
                 row.get("instruction_shape_sha256"), row.get("pseudocode_fingerprint"),
                 provenance),
            )
            if recovered:
                feature(connection, "retail", fid, "symbol", recovered)
            for ref in row.get("data_references", []):
                if ref.get("string_sha256_utf8"):
                    feature(connection, "retail", fid, "string_hash", ref["string_sha256_utf8"].lower())
                elif ref.get("kind") == "global_reference" and ref.get("data_start"):
                    feature(connection, "retail", fid, "global", ref["data_start"])
            for value in row.get("constants", []):
                number = integer_value(value)
                if number is not None and number not in COMMON_NUMBERS:
                    feature(connection, "retail", fid, "numeric_constant", number)
            for target in row.get("calls", []):
                feature(connection, "retail", fid, "callee", target)
            for target in row.get("callers", []):
                feature(connection, "retail", fid, "caller", target)
            for edge in row.get("call_edges", []):
                connection.execute(
                    "INSERT OR IGNORE INTO retail_call_edges VALUES (?,?,?,?,?)",
                    (fid, parse_address(edge["site"]), str(edge["target"]),
                     str(edge.get("type", "UNKNOWN")), edge.get("external_name")),
                )
                if edge.get("external_name"):
                    feature(connection, "retail", fid, "import", import_basename(edge["external_name"]))
            count += 1
        if not footer or not footer.get("complete"):
            raise ValueError("Retail metadata has no complete footer")
        if int(footer.get("functions", -1)) != count:
            raise ValueError("Retail footer function count disagrees with parsed rows")
        if any(True for _ in stream):
            raise ValueError("Unexpected data after retail footer")
        connection.execute("UPDATE index_runs SET item_count=? WHERE run_id=?", (count, rid))
    return rid, count


def import_source(connection, path: Path) -> tuple[str, int]:
    rid = run_id("source", path)
    count = 0
    heads = set()
    with connection:
        connection.execute("INSERT INTO index_runs VALUES (?,?,?,?,?,?,?)",
                           (rid, "source", utc_now(), "pending source commit", sha256_file(path), 0,
                            "lexical leads from evaluated MSBuild projects; preprocessing unresolved"))
        for row in records(path):
            head = str(row["source_commit"])
            heads.add(head)
            sid = str(row["source_id"])
            path_value = str(row["path"])
            name = str(row["qualified_name"])
            targets = row.get("compile_target") or []
            target_value = json.dumps(targets, separators=(",", ":")) if isinstance(targets, list) else str(targets)
            connection.execute(
                "INSERT INTO source_functions VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                (sid, rid, head, path_value, target_value, name, row.get("signature"),
                 row.get("start_line"), row.get("end_line"), row.get("fingerprint"),
                 row["source_blob_sha256"], int(bool(row["worktree_dirty"])),
                 row["worktree_identity"], "COMPILED"),
            )
            feature(connection, "source", sid, "symbol", name)
            for spelling in row.get("strings", []):
                literal_hash = source_literal_hash(spelling)
                if literal_hash:
                    feature(connection, "source", sid, "string_hash", literal_hash, "plain ASCII literal")
            for value in row.get("numeric_constants", []):
                number = integer_value(value)
                if number is not None and number not in COMMON_NUMBERS:
                    feature(connection, "source", sid, "numeric_constant", number)
            for call in row.get("calls", []):
                feature(connection, "source", sid, "call", call)
                # A lexical call is only an import *candidate*; matching the
                # name is weak evidence until linkage and semantics are checked.
                feature(connection, "source", sid, "import", import_basename(call), "lexical call candidate")
            count += 1
        if len(heads) != 1:
            raise ValueError(f"Source rows must all share one commit; got {len(heads)}")
        connection.execute("UPDATE index_runs SET item_count=?,input_identity=? WHERE run_id=?",
                           (count, next(iter(heads)), rid))
    return rid, count


def import_seeds(connection, path: Path) -> tuple[str, int]:
    stream = iter(records(path))
    header = next(stream, None)
    if not header or header.get("record") != "header":
        raise ValueError("Seed probe is missing its header")
    image_hash = str(header.get("image_sha256", "")).upper()
    if image_hash != CANONICAL_EXE_SHA256:
        raise ValueError("Seed probe is not from the canonical retail EXE")
    rid = run_id("seed", path)
    count = 0
    footer = None
    with connection:
        connection.execute("INSERT INTO index_runs VALUES (?,?,?,?,?,?,?)",
                           (rid, "seed", utc_now(), image_hash, sha256_file(path), 0,
                            "bounded first code blocks only; no function promotion"))
        for row in stream:
            if row.get("record") == "footer":
                footer = row
                break
            if row.get("record") != "seed_probe":
                continue
            if not row.get("defined_instruction_at"):
                raise ValueError("Seed does not begin at a defined instruction")
            connection.execute("INSERT INTO retail_code_leads VALUES (?,?,?,?,?,?,?,?,?,?)",
                               (image_hash, parse_address(row["entry"]),
                                int(bool(row.get("defined_function_at"))),
                                int(row["probe_instruction_count"]), int(row["probe_byte_count"]),
                                row["probe_byte_sha256"], row["probe_instruction_shape_sha256"],
                                row["stop_reason"], json.dumps(row.get("incoming_references", [])),
                                "UNREVIEWED"))
            count += 1
        if not footer or not footer.get("complete") or int(footer.get("seeds", -1)) != count:
            raise ValueError("Seed probe lacks a matching complete footer")
        if any(True for _ in stream):
            raise ValueError("Unexpected data after seed footer")
        connection.execute("UPDATE index_runs SET item_count=? WHERE run_id=?", (count, rid))
    return rid, count


def import_reviewed_samples(connection, path: Path) -> tuple[str, int]:
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("schema") != "cfw-correspondence-validation-v1":
        raise ValueError("Unexpected reviewed-sample schema")
    manifest_hash = sha256_file(path)
    count = 0
    with connection:
        for row in manifest.get("reviewed_samples", []):
            source = connection.execute(
                "SELECT source_blob_sha256 FROM source_functions WHERE source_id=? AND source_commit=?",
                (row["source_id"], manifest["source_commit"]),
            ).fetchone()
            if not source:
                raise ValueError(f"Reviewed sample source is not indexed: {row['source_id']}")
            connection.execute(
                "INSERT INTO reviewed_identities VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)",
                (row["module_sha256"].upper(), row["module"], parse_address(row["va"]),
                 row["source_id"], source[0], row["relationship"], int(row["confidence"]),
                 row["evidence_type"], row["evidence_ref"], row["reviewer"],
                 row["review_date"], "UNTESTED", manifest_hash),
            )
            count += 1
    return manifest_hash, count


def rank_candidates(connection, retail_run: str, source_run: str, maximum_df=0.02, top=10):
    """Two-signal conservative leads; raw numeric overlap alone is never a match."""
    retail_count = connection.execute("SELECT item_count FROM index_runs WHERE run_id=?", (retail_run,)).fetchone()[0]
    source_count = connection.execute("SELECT item_count FROM index_runs WHERE run_id=?", (source_run,)).fetchone()[0]
    if not retail_count or not source_count:
        raise ValueError("Both indices must be nonempty")
    retail_features = defaultdict(set)
    source_features = defaultdict(set)
    allowed = "('string_hash','numeric_constant','symbol','import')"
    for owner_id, category, value in connection.execute(
        "SELECT f.owner_id,f.feature_kind,f.feature_value FROM features f "
        "JOIN retail_functions r ON r.retail_id=f.owner_id "
        f"WHERE f.owner_kind='retail' AND r.run_id=? AND f.feature_kind IN {allowed}",
        (retail_run,),
    ):
        retail_features[(category, value)].add(owner_id)
    for owner_id, category, value in connection.execute(
        "SELECT f.owner_id,f.feature_kind,f.feature_value FROM features f "
        "JOIN source_functions s ON s.source_id=f.owner_id "
        f"WHERE f.owner_kind='source' AND s.run_id=? AND f.feature_kind IN {allowed}",
        (source_run,),
    ):
        source_features[(category, value)].add(owner_id)
    pair_features = defaultdict(list)
    for key, retail_ids in retail_features.items():
        source_ids = source_features.get(key)
        if not source_ids:
            continue
        if len(retail_ids) > max(2, int(retail_count * maximum_df)):
            continue
        if len(source_ids) > max(2, int(source_count * maximum_df)):
            continue
        for retail_id in retail_ids:
            for source_id in source_ids:
                pair_features[(retail_id, source_id)].append((key[0], key[1], len(retail_ids), len(source_ids)))
    weighted = {}
    for pair, features in pair_features.items():
        groups = {entry[0] for entry in features}
        score = 0.0
        for category, _, retail_df, source_df in features:
            weight = {"string_hash": 10.0, "symbol": 5.0,
                      "numeric_constant": 1.0, "import": 2.0}[category]
            score += weight / ((retail_df * source_df) ** 0.5)
        # Generic constants or one unique string are leads only, never accepted matches.
        weighted[pair] = (score, len(groups), features)
    by_retail = defaultdict(list)
    for pair, item in weighted.items():
        by_retail[pair[0]].append((pair[1], item))
    matcher_identity = json.dumps({"retail": retail_run, "source": source_run,
                                   "max_df": maximum_df, "top": top}, sort_keys=True)
    matcher_run = "match-" + hashlib.sha256(matcher_identity.encode()).hexdigest()[:20]
    total = 0
    with connection:
        connection.execute("INSERT INTO index_runs VALUES (?,?,?,?,?,?,?)",
                           (matcher_run, "match", utc_now(), matcher_identity,
                            sha256_file(Path(__file__)), 0, "unreviewed leads; scores not confidence"))
        for retail_id, items in by_retail.items():
            ordered = sorted(items, key=lambda item: (-item[1][0], -item[1][1], item[0]))[:top]
            for rank, (source_id, (score, groups, shared)) in enumerate(ordered, 1):
                warning = "Single signal group; manual identity proof required" if groups < 2 else \
                          "Multiple shared features; review hard negatives and callgraph"
                connection.execute("INSERT INTO match_candidates VALUES (?,?,?,?,?,?,?,?)",
                                   (retail_id, source_id, matcher_run, rank, score, groups,
                                    json.dumps(shared, separators=(",", ":")), warning))
                total += 1
        connection.execute("UPDATE index_runs SET item_count=? WHERE run_id=?", (total, matcher_run))
    return matcher_run, total, len(by_retail)


def stats(connection, matcher_run: str | None = None) -> dict:
    scalar = lambda query: connection.execute(query).fetchone()[0]
    match_filter = " WHERE matcher_run_id=?" if matcher_run else ""
    parameters = (matcher_run,) if matcher_run else ()
    match_count = lambda select, suffix="": connection.execute(
        f"SELECT {select} FROM match_candidates{match_filter}{suffix}", parameters
    ).fetchone()[0]
    multi_filter = (" WHERE matcher_run_id=? AND" if matcher_run else " WHERE") + " signal_groups>=2"
    return {
        "retail_function_records_total": scalar("SELECT count(*) FROM retail_functions"),
        "retail_saved_analysis_functions": scalar(
            "SELECT count(*) FROM retail_functions WHERE evidence_status='SAVED_ANALYSIS_FUNCTION'"
        ),
        "retail_ephemeral_recovered_candidates": scalar(
            "SELECT count(*) FROM retail_functions WHERE evidence_status='EPHEMERAL_RECOVERED_CANDIDATE'"
        ),
        "bounded_code_leads_not_functions": scalar("SELECT count(*) FROM retail_code_leads WHERE defined_function_at=0"),
        "source_lexical_function_candidates": scalar("SELECT count(*) FROM source_functions"),
        "source_rows_from_dirty_translation_units": scalar("SELECT count(*) FROM source_functions WHERE worktree_dirty=1"),
        "retail_call_edges": scalar("SELECT count(*) FROM retail_call_edges"),
        "retail_pointer_table_candidates_not_vtables": scalar("SELECT count(*) FROM retail_pointer_table_candidates"),
        "matcher_run": matcher_run or "ALL_RUNS_COMBINED",
        "automated_pair_leads": match_count("count(*)"),
        "retail_functions_with_leads": match_count("count(DISTINCT retail_id)"),
        "source_functions_with_leads": match_count("count(DISTINCT source_id)"),
        "pair_leads_with_two_or_more_signal_groups": connection.execute(
            "SELECT count(*) FROM match_candidates" + multi_filter, parameters
        ).fetchone()[0],
        "retail_functions_with_multisignal_leads": connection.execute(
            "SELECT count(DISTINCT retail_id) FROM match_candidates" + multi_filter, parameters
        ).fetchone()[0],
        "independently_reviewed_function_identities": scalar("SELECT count(*) FROM reviewed_identities"),
        "behaviorally_accepted_from_mapping": 0,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--db", required=True, type=Path)
    parser.add_argument("--schema", type=Path, default=Path(__file__).resolve().parents[1] / "schema.sql")
    actions = parser.add_subparsers(dest="action", required=True)
    for action in ("import-retail", "import-source", "import-seeds", "import-reviewed"):
        actions.add_parser(action).add_argument("jsonl", type=Path)
    matcher = actions.add_parser("match")
    matcher.add_argument("--retail-run", required=True)
    matcher.add_argument("--source-run", required=True)
    matcher.add_argument("--top", type=int, default=10)
    statistics = actions.add_parser("stats")
    statistics.add_argument("--matcher-run")
    args = parser.parse_args()
    init_db(args.db, args.schema)
    with sqlite3.connect(args.db) as connection:
        connection.execute("PRAGMA foreign_keys=ON")
        if args.action == "import-retail":
            result = import_retail(connection, args.jsonl)
        elif args.action == "import-source":
            result = import_source(connection, args.jsonl)
        elif args.action == "import-seeds":
            result = import_seeds(connection, args.jsonl)
        elif args.action == "import-reviewed":
            result = import_reviewed_samples(connection, args.jsonl)
        elif args.action == "stats":
            result = stats(connection, args.matcher_run)
        else:
            result = rank_candidates(connection, args.retail_run, args.source_run, top=args.top)
    print(json.dumps({"action": args.action, "result": result}, sort_keys=True))


if __name__ == "__main__":
    main()
