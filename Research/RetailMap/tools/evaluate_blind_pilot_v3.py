#!/usr/bin/env python3
"""Join separately sealed pilot rankings and independent identity review.

This evaluates function identity only. It does not infer ground truth from scores,
create new anchors, set confidence thresholds, or modify the frozen matcher.
All joined results remain in ignored private evidence.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
import gzip
import hashlib
import json
from pathlib import Path
import sqlite3
from statistics import median

import validate_contextual_matcher_v3 as v3
import validate_machine_matcher_v2 as body


RAW_SHA256 = "B0BAAC8AE2326DBEB6E1E012B19D9FF6E5911FECD450C110062E1EF33DB2D3CD"
TRUTH_SHA256 = "9E9604D15D58448E91BA15D9972017D29F67866F36C1E375519084EC397FD79B"
SELECTION_SHA256 = "E92CDB2904F8BDA1B5992C0E9A90946D4491915919BCA5EF4E9E1C197E63A857"
PILOT_DB_SHA256 = "173B502B2784D91BA314E3D1BF447FC2D65A5355969871CFD602E08139F3685A"
TRIM_BRIDGE_SHA256 = "E0D5CD0BD6D542F1BDBEFF220D04C292037E895F02AD9AE95BA40C60FC1A2948"
OBJECT_BRIDGE_SHA256 = "73380604E4A906D9AB92E403F236AAC4948499A2DA6B38278F8FD3226BADC3E3"
POSITIVE_CLASSES = {"V", "S"}
CLASS_NAMES = {"V": "VERIFIED_CORRESPONDENCE",
               "S": "STRONG_INDEPENDENT_CORRESPONDENCE",
               "E": "EQUIVALENCE_SET", "R": "RETAIL_ONLY_CANDIDATE",
               "M": "SPLIT_OR_MERGED", "U": "UNRESOLVED"}


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest().upper()


def source_rva(db: sqlite3.Connection, source_id: str, module_hash: str,
               supplemental: dict[tuple[str, str], int]) -> tuple[int | None, str]:
    if module_hash == body.CURRENT_TRIM:
        value = supplemental.get((module_hash, source_id))
        return value, "INDEPENDENT_TRIM_MAP_COFF_BRIDGE" if value is not None else "NO_TRIM_MACHINE_BRIDGE"
    rows = list(db.execute(
        "SELECT DISTINCT l.rva,l.relationship,f.source_identity_status "
        "FROM compiled_source_links l JOIN compiled_functions f "
        "ON f.module_sha256=l.module_sha256 AND f.rva=l.rva "
        "WHERE l.module_sha256=? AND l.source_id=?", (module_hash, source_id)))
    valid = [rva for rva, relation, status in rows
             if relation == "ONE_TO_ONE" and status == "PDB_SOURCE_BYTES_VERIFIED"]
    if len(valid) != 1 or len(rows) != 1:
        value = supplemental.get((module_hash, source_id))
        return value, ("SUPPLEMENTAL_PDB_SOURCE_BYTES_RAW_ROW" if value is not None
                       else "NO_UNIQUE_PDB_SOURCE_BYTES_BRIDGE")
    return valid[0], "PDB_SOURCE_BYTES_VERIFIED"


def metrics(rows: list[dict]) -> dict:
    eligible = [row for row in rows if row["rank_scored"]]
    ranks = [row["true_rank_max"] for row in eligible]
    found = [rank for rank in ranks if rank is not None]
    return {"denominator": len(ranks),
            **{f"top_{limit}": sum(rank is not None and rank <= limit for rank in ranks)
               for limit in (1, 3, 5, 10)},
            "mrr_conservative": None if not ranks else round(
                sum(1 / rank for rank in found) / len(ranks), 6),
            "median_true_rank": None if not found else median(found),
            "candidate_generation_misses": sum(rank is None for rank in ranks)}


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    for name in ("raw", "truth", "selection", "pilot-db", "output"):
        p.add_argument("--" + name, required=True, type=Path)
    p.add_argument("--trim-bridge", type=Path)
    p.add_argument("--object-bridge", type=Path)
    args = p.parse_args()
    private = Path("Evidence/RetailMap/local").resolve()
    if args.output.exists() or private not in args.output.resolve().parents:
        raise ValueError("Evaluation output must be new and private")
    for path, expected in ((args.raw, RAW_SHA256), (args.truth, TRUTH_SHA256),
                           (args.selection, SELECTION_SHA256),
                           (args.pilot_db, PILOT_DB_SHA256)):
        if digest(path) != expected:
            raise ValueError(f"Sealed input hash changed: {path}")
    truth = json.loads(args.truth.read_text(encoding="utf-8"))
    if truth["selection_sha256"] != SELECTION_SHA256:
        raise ValueError("Ground truth does not refer to the sealed selection")
    selected = json.loads(args.selection.read_text(encoding="utf-8"))["sample"]
    gt = {}
    for subsystem, rows in truth["rows"].items():
        if len(rows) != len(selected[subsystem]):
            raise ValueError("Truth count differs from selected sample")
        for va, code, source_id, evidence in rows:
            key = (subsystem, int(va, 16))
            if key in gt or code not in CLASS_NAMES:
                raise ValueError("Duplicate or unknown ground-truth row")
            gt[key] = (code, source_id, evidence)
    supplemental: dict[tuple[str, str], int] = {}
    bridge_hashes = {}
    if args.trim_bridge:
        if digest(args.trim_bridge) != TRIM_BRIDGE_SHA256:
            raise ValueError("Sealed Trim bridge hash changed")
        payload = json.loads(args.trim_bridge.read_text(encoding="utf-8"))
        if (payload.get("state") != "SEALED_WITHOUT_MATCHER_OUTPUT" or
            payload.get("primary_ground_truth_sha256") != TRUTH_SHA256):
            raise ValueError("Trim bridge was not independently sealed")
        bridge_hashes["trim"] = TRIM_BRIDGE_SHA256
        for retail_va, source_id, current_rva, _compiland in payload["rows"]:
            if not any(int(row[0], 16) == int(retail_va, 16) and row[2] == source_id
                       for row in truth["rows"]["Trim/UI"]):
                raise ValueError("Trim bridge does not match independent truth")
            supplemental[(body.CURRENT_TRIM, source_id)] = int(current_rva, 16)
    if args.object_bridge:
        if digest(args.object_bridge) != OBJECT_BRIDGE_SHA256:
            raise ValueError("Sealed Object bridge hash changed")
        payload = json.loads(args.object_bridge.read_text(encoding="utf-8"))
        if (payload.get("state") != "SEALED_WITHOUT_MATCHER_OUTPUT" or
            payload.get("primary_ground_truth_sha256") != TRUTH_SHA256):
            raise ValueError("Object bridge was not independently sealed")
        bridge_hashes["object_static"] = OBJECT_BRIDGE_SHA256
        for row in payload["rows"]:
            if not any(int(item[0], 16) == int(row["retail_va"], 16) and
                       item[2] == row["source_id"]
                       for item in truth["rows"]["Object/archetype"]):
                raise ValueError("Object bridge does not match independent truth")
            supplemental[(body.CURRENT_EXE, row["source_id"])] = int(
                row["current_final_rva"], 16)
    db = sqlite3.connect(f"file:{args.pilot_db.resolve().as_posix()}?mode=ro", uri=True)
    output = {"schema": "cfw-retail-map-v3-blind-pilot-evaluation-1",
              "input_sha256": {"raw": RAW_SHA256, "truth": TRUTH_SHA256,
                               "selection": SELECTION_SHA256, "pilot_db": PILOT_DB_SHA256,
                               "supplemental_bridges": bridge_hashes},
              "classification_counts": {}, "ranked": [], "metrics": {},
              "calibration": "UNAVAILABLE_IN_FROZEN_V3",
              "false_unique_high_confidence": "NOT_MEASURABLE_NO_SUCH_CATEGORY_EMITTED",
              "ablation": "anchor-off and vtable-off identical to full on these unseen functions if every row has zero context"}
    classifications = defaultdict(int)
    seen = set()
    try:
        with gzip.open(args.raw, "rt", encoding="utf-8") as stream:
            for line in stream:
                record = json.loads(line)
                subsystem, va = record["subsystem"], int(record["retail_va"], 16)
                key = (subsystem, va)
                if key not in gt or key in seen:
                    raise ValueError("Raw output disagrees with sealed ground truth")
                seen.add(key)
                code, source_id, evidence = gt[key]
                classifications[(subsystem, code)] += 1
                candidates = record["candidates"]
                for item in candidates:
                    if item["context"]["contextual_evidence_count"]:
                        raise ValueError("Unexpected context on unseen pilot function")
                    for ablation in v3.ABLATIONS:
                        value = v3.augmented(item["body_score"], item["context"], ablation)
                        if abs(value - item["score"]) > 1e-10:
                            raise ValueError("Ablation differs from frozen full score")
                result = {"subsystem": subsystem, "retail_va": record["retail_va"],
                          "ground_truth_class": CLASS_NAMES[code],
                          "independent_evidence": evidence,
                          "source_id": source_id,
                          "candidate_count": len(candidates),
                          "top_equivalence_size": candidates[0]["equivalence_size"] if candidates else 0,
                          "rank_scored": False, "true_rva": None, "true_rank": None,
                          "true_rank_max": None, "bridge_status": None}
                if code in POSITIVE_CLASSES:
                    if not source_id:
                        raise ValueError("Resolved positive lacks independent source ID")
                    rva, bridge = source_rva(db, source_id,
                                             record["current_module_sha256"], supplemental)
                    result["bridge_status"] = bridge
                    result["true_rva"] = None if rva is None else f"0x{rva:X}"
                    result["rank_scored"] = rva is not None
                    if rva is not None:
                        match = next((item for item in candidates if item["rva"] == rva), None)
                        if match:
                            result["true_rank"] = match["rank"]
                            result["true_rank_max"] = match["rank_max"]
                output["ranked"].append(result)
        if len(seen) != 45 or seen != set(gt):
            raise ValueError("Not exactly the sealed 45-function pilot")
        output["classification_counts"] = {
            subsystem: {CLASS_NAMES[code]: count for (section, code), count in classifications.items()
                        if section == subsystem}
            for subsystem in selected}
        output["metrics"]["aggregate"] = metrics(output["ranked"])
        for subsystem in selected:
            output["metrics"][subsystem] = metrics(
                [item for item in output["ranked"] if item["subsystem"] == subsystem])
        args.output.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    finally:
        db.close()
    print(json.dumps({"output_sha256": digest(args.output),
                      "metrics": output["metrics"],
                      "classification_counts": output["classification_counts"]}, sort_keys=True))


if __name__ == "__main__":
    main()
