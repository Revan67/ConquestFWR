#!/usr/bin/env python3
"""Score only independently reviewed controls against normalized machine metadata.

All scores are development leads. Positive identities come from the predeclared
validation manifest and verified compiled-source bridge, never this matcher.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import sqlite3


RETAIL_EXE = "ED8741C0CB727A43B7DE7BE0E73566B04FBC1CBD58B54A62D538E3F39DCB3F52"
RETAIL_TRIM = "AC13625FEF6DA8D553196C894351E93F842BD8FB5344B97FEBA1FD6835E830F0"
CURRENT_EXE = "88ACFDE07FCBD120C1436492B4CAB6D9791C1DD772A830071C15C99E1BC09F8C"
CURRENT_TRIM = "BB9D7017902A9C6B338EC27D61B46C9D4E5A75B5EA8972E989F57E78225051D9"
VERSION = "machine-v2-control-only-development-2"
NORMALIZATION = "normalized-x86-v2-development"
SCORING = {"shape_trigram": 0.50, "shape_bigram": 0.18,
           "mnemonic_trigram": 0.12, "mnemonic_bag": 0.10,
           "flow": 0.07, "length": 0.03}


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def ngrams(values: list[str], width: int) -> Counter:
    return Counter(tuple(values[index:index + width]) for index in range(len(values) - width + 1))


def prepare(data: dict) -> dict:
    shape = data["shapes"]
    names = data["mnemonics"]
    return {"data": data, "tri": ngrams(shape, 3), "bi": ngrams(shape, 2),
            "mtri": ngrams(names, 3), "mbag": Counter(names)}


def weighted_overlap(left: Counter, right: Counter, weights: dict) -> tuple[float, int]:
    if not left or not right:
        return 0.0, 0
    keys = left.keys() | right.keys()
    intersection = sum(min(left[key], right[key]) * weights.get(key, 1.0) for key in keys)
    union = sum(max(left[key], right[key]) * weights.get(key, 1.0) for key in keys)
    return intersection / union if union else 0.0, sum(min(left[key], right[key]) for key in keys)


def weights(candidates: dict) -> dict:
    result = {}
    count = len(candidates)
    for family in ("tri", "bi", "mtri", "mbag"):
        frequency = Counter()
        for item in candidates.values():
            frequency.update(item[family].keys())
        result[family] = {key: math.log((count + 1) / (value + 1)) + 1
                          for key, value in frequency.items()}
    return result


def score(retail: dict, current: dict, idf: dict) -> tuple[float, dict]:
    values = {}
    shared = 0
    for family, label in (("tri", "shape_trigram"), ("bi", "shape_bigram"),
                          ("mtri", "mnemonic_trigram"), ("mbag", "mnemonic_bag")):
        values[label], hits = weighted_overlap(retail[family], current[family], idf[family])
        shared += hits
    left = retail["data"]["control_flow"]
    right = current["data"]["control_flow"]
    values["flow"] = sum(1 - abs(left[key] - right[key]) / max(left[key], right[key], 1)
                         for key in left) / len(left)
    a = retail["data"]["instruction_count"]
    b = current["data"]["instruction_count"]
    values["length"] = min(a, b) / max(a, b)
    total = sum(values[key] * weight for key, weight in SCORING.items())
    return total, {"components": values, "shared_comparable_features": shared,
                   "retail_instruction_count": a, "current_instruction_count": b}


def load_features(connection: sqlite3.Connection, module: str) -> dict[int, dict]:
    result = {}
    for rva, raw in connection.execute(
        "SELECT rva,feature_value FROM machine_features WHERE module_sha256=? AND feature_family='normalized_ir_json' AND normalization_version=?",
        (module, NORMALIZATION),
    ):
        result[rva] = prepare(json.loads(raw))
    return result


def feature_identity(connection: sqlite3.Connection, module: str) -> str:
    row = connection.execute(
        "SELECT min(extractor_sha256),max(extractor_sha256),count(*) FROM machine_features WHERE module_sha256=? AND feature_family='normalized_ir_json' AND normalization_version=?",
        (module, NORMALIZATION),
    ).fetchone()
    if not row or row[2] == 0 or row[0] != row[1]:
        raise ValueError(f"Missing or mixed extractor versions for {module}")
    return f"{module}:{row[0]}:{row[2]}"


def expected_rva(connection: sqlite3.Connection, source_id: str, module: str) -> int | None:
    if module == CURRENT_TRIM:
        return 0x8DF10 if source_id == "Code/App/Src/Menu_Toolbar.cpp:1685:Menu_tb::loadInterface" else None
    matches = [rva for (rva,) in connection.execute(
        "SELECT DISTINCT rva FROM compiled_source_links WHERE module_sha256=? AND source_id=?",
        (module, source_id),
    )]
    return matches[0] if len(matches) == 1 else None


def rank_control(retail: dict, candidates: dict, idf: dict, target: int | None):
    count = retail["data"]["instruction_count"]
    rows = []
    for rva, current in candidates.items():
        other_count = current["data"]["instruction_count"]
        if other_count < max(2, count // 4) or other_count > count * 4:
            continue
        value, detail = score(retail, current, idf)
        if detail["shared_comparable_features"] == 0:
            continue
        rows.append((rva, value, detail))
    rows.sort(key=lambda item: (-item[1], item[0]))
    rank = next((index for index, item in enumerate(rows, 1) if item[0] == target), None)
    return rows, rank


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--db", required=True, type=Path)
    parser.add_argument("--controls", required=True, type=Path)
    args = parser.parse_args()
    controls = json.loads(args.controls.read_text(encoding="utf-8"))
    connection = sqlite3.connect(args.db)
    connection.execute("PRAGMA foreign_keys=ON")
    implementation = digest(Path(__file__))
    input_identity = "|".join(feature_identity(connection, module) for module in
                              (RETAIL_EXE, RETAIL_TRIM, CURRENT_EXE, CURRENT_TRIM))
    run_id = "v2-control-" + hashlib.sha256((implementation + digest(args.controls) + input_identity).encode()).hexdigest()[:20]
    modules = {"Conquest_retail.exe": (RETAIL_EXE, CURRENT_EXE),
               "Trim.dll": (RETAIL_TRIM, CURRENT_TRIM)}
    current = {module: load_features(connection, module) for module in (CURRENT_EXE, CURRENT_TRIM)}
    retail = {module: load_features(connection, module) for module in (RETAIL_EXE, RETAIL_TRIM)}
    idf = {module: weights(items) for module, items in current.items()}
    output = []
    try:
        with connection:
            connection.execute("INSERT INTO matcher_versions VALUES (?,?,?,?,?,?)",
                               (VERSION, "MATCHER_V2_DEVELOPMENT", implementation,
                                NORMALIZATION, json.dumps(SCORING, sort_keys=True),
                                "predeclared independent controls; no retail correspondence accepted"))
            connection.execute("INSERT INTO matcher_v2_runs VALUES (?,?,?,?,?,?,?,?,?)",
                               (run_id, VERSION,
                                hashlib.sha256((feature_identity(connection, RETAIL_EXE) + feature_identity(connection, RETAIL_TRIM)).encode()).hexdigest().upper(),
                                hashlib.sha256((feature_identity(connection, CURRENT_EXE) + feature_identity(connection, CURRENT_TRIM)).encode()).hexdigest().upper(),
                                "compiled bridge v2 + fresh Trim map/CodeView anchor",
                                json.dumps({"instruction_ratio": [0.25, 4.0],
                                            "requires_shared_normalized_feature": True}, sort_keys=True),
                                json.dumps(SCORING, sort_keys=True),
                                "UNVALIDATED", "control-only run; no bulk retail ranking"))
            for index, item in enumerate(controls["controls"]):
                retail_hash, current_hash = modules[item["module"]]
                retail_rva = int(item["va"], 16) - (0x10000000 if item["module"] == "Trim.dll" else 0x400000)
                expected = expected_rva(connection, item["source_id"], current_hash)
                if retail_rva not in retail[retail_hash]:
                    raise ValueError(f"Retail control lacks machine features: {item['va']}")
                rows, rank = rank_control(retail[retail_hash][retail_rva], current[current_hash],
                                          idf[current_hash], expected)
                true = next((row for row in rows if row[0] == expected), None)
                top = rows[0] if rows else None
                tied = sum(abs(row[1] - top[1]) < 0.005 for row in rows) if top else 0
                outcome = "RANKED" if rank else "NOT_RANKED"
                control_id = f"{item['module']}:{item['va']}"
                for candidate_rank, (rva, value, detail) in enumerate(rows[:10], 1):
                    connection.execute("INSERT INTO ranked_machine_links VALUES (?,?,?,?,?,?,?,?,?)",
                                       (run_id, retail_hash, retail_rva, current_hash, rva,
                                        candidate_rank, value, json.dumps(detail, sort_keys=True),
                                        "AMBIGUOUS" if tied > 1 else "UNKNOWN"))
                if true and rank and rank > 10:
                    connection.execute("INSERT INTO ranked_machine_links VALUES (?,?,?,?,?,?,?,?,?)",
                                       (run_id, retail_hash, retail_rva, current_hash, expected,
                                        rank, true[1], json.dumps(true[2], sort_keys=True), "UNKNOWN"))
                connection.execute("INSERT INTO matcher_control_results VALUES (?,?,?,?,?,?,?,?,?,?)",
                                   (run_id, control_id,
                                    "DEVELOPMENT_POSITIVE" if item["role"] == "calibration" else "HELD_OUT_POSITIVE",
                                    rank, top[1] if top else None, true[1] if true else None,
                                    (true[1] - top[1]) if true and top else None,
                                    len(rows), outcome, item["basis"]))
                output.append({"control": control_id, "role": item["role"],
                               "expected_rva": None if expected is None else f"0x{expected:X}",
                               "rank": rank, "candidate_count": len(rows),
                               "true_score": None if true is None else round(true[1], 4),
                               "true_shared": None if true is None else true[2]["shared_comparable_features"],
                               "top_rva": None if top is None else f"0x{top[0]:X}",
                               "top_score": None if top is None else round(top[1], 4),
                               "near_top_ties": tied})
            negatives = []
            positive_by_va = {row["control"].split(":", 1)[1].lower(): row for row in output}
            for item in controls["hard_negatives"]:
                retail_hash, current_hash = modules[item["module"]]
                retail_rva = int(item["va"], 16) - 0x400000
                wrong = expected_rva(connection, item["wrong_source_id"], current_hash)
                rows, rank = rank_control(retail[retail_hash][retail_rva], current[current_hash],
                                          idf[current_hash], wrong)
                wrong_row = next((row for row in rows if row[0] == wrong), None)
                positive = positive_by_va.get(item["va"].lower())
                true_score = positive["true_score"] if positive else None
                margin = true_score - wrong_row[1] if true_score is not None and wrong_row else None
                outcome = ("AMBIGUOUS_SAME_SCORE" if margin is not None and abs(margin) < 0.005 else
                           "WRONG_OUTRANKS_TRUE" if margin is not None and margin < 0 else
                           "WRONG_TOP" if rank == 1 else "REJECTED_OR_LOWER")
                negative = {"control": item["va"], "wrong_rva": None if wrong is None else f"0x{wrong:X}",
                            "wrong_rank": rank, "wrong_score": None if wrong_row is None else round(wrong_row[1], 4),
                            "top_score": round(rows[0][1], 4) if rows else None,
                            "true_minus_wrong": None if margin is None else round(margin, 4),
                            "outcome": outcome}
                negatives.append(negative)
                connection.execute("INSERT INTO matcher_control_results VALUES (?,?,?,?,?,?,?,?,?,?)",
                                   (run_id, "negative:" + item["va"] + ":" + item["wrong_source_id"], "HARD_NEGATIVE",
                                    rank, rows[0][1] if rows else None,
                                    wrong_row[1] if wrong_row else None,
                                    margin, len(rows), outcome,
                                    item["reason"]))
        ranked = [row["rank"] for row in output]
        metrics = {f"top_{limit}": sum(value is not None and value <= limit for value in ranked)
                   for limit in (1, 3, 5, 10)}
        metrics["positives"] = len(ranked)
        metrics["mrr"] = round(sum(1 / value for value in ranked if value) / len(ranked), 4)
        print(json.dumps({"run_id": run_id, "status": "MATCHER_V2_DEVELOPMENT",
                          "metrics": metrics, "positives": output, "hard_negatives": negatives}, sort_keys=True))
    finally:
        connection.close()


if __name__ == "__main__":
    main()
