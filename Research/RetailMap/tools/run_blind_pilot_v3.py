#!/usr/bin/env python3
"""Replay frozen v2/v3 scoring for a sealed address-only pilot.

The selection file is read solely for its subsystem/address/size sample. This
adapter does not read or write analyst conclusions, infer identities, or define
confidence thresholds. Detailed results and the working DB stay private.
"""
from __future__ import annotations

import argparse
import gzip
import hashlib
import json
from pathlib import Path
import sqlite3

import extract_machine_features as extractor
import validate_machine_matcher_v2 as body
import validate_contextual_matcher_v3 as v3


SUBSYSTEM_MODULES = {
    "Object/archetype": ("Conquest_retail.exe", body.RETAIL_EXE, body.CURRENT_EXE, 0x400000),
    "Flagship": ("Conquest_retail.exe", body.RETAIL_EXE, body.CURRENT_EXE, 0x400000),
    "Trim/UI": ("Trim.dll", body.RETAIL_TRIM, body.CURRENT_TRIM, 0x10000000),
}


def require_hash(path: Path, expected: str) -> None:
    actual = extractor.digest(path)
    if actual != expected:
        raise ValueError(f"SHA-256 mismatch for {path}: {actual}, expected {expected}")


def json_line(value: dict) -> bytes:
    return (json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n").encode("utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("freeze", "selection", "v2-db", "vtable", "retail-exe",
                 "retail-trim", "current-exe", "current-trim", "output-db",
                 "output-raw", "output-summary"):
        parser.add_argument("--" + name, required=True, type=Path)
    args = parser.parse_args()
    private = Path("Evidence/RetailMap/local").resolve()
    for path in (args.output_db, args.output_raw, args.output_summary):
        if path.exists() or private not in path.resolve().parents:
            raise ValueError(f"Output must be new and under private evidence: {path}")

    freeze = json.loads(args.freeze.read_text(encoding="utf-8"))
    if freeze["frozen_at_commit"] != "9b4923e20244945e139cdb58e2462732e4f1bcc8":
        raise ValueError("Unexpected frozen v3 version")
    method = freeze["method"]
    for name, path in (("validator_sha256", Path(v3.__file__)),
                       ("v2_scorer_sha256", Path(body.__file__)),
                       ("feature_extractor_sha256", Path(extractor.__file__))):
        require_hash(path, method[name])
    inputs = freeze["frozen_inputs_sha256"]
    for name, path in (("v2_database", args.v2_db), ("vtable_capture", args.vtable),
                       ("retail_exe", args.retail_exe), ("retail_trim", args.retail_trim),
                       ("current_final_exe", args.current_exe),
                       ("current_debug_trim", args.current_trim)):
        require_hash(path, inputs[name])
    selection_hash = extractor.digest(args.selection)
    if selection_hash != "E92CDB2904F8BDA1B5992C0E9A90946D4491915919BCA5EF4E9E1C197E63A857":
        raise ValueError("Sealed selection hash changed")

    # Only the preselected address/size pairs are accessed from this file.
    selected = json.loads(args.selection.read_text(encoding="utf-8"))["sample"]
    if set(selected) != set(SUBSYSTEM_MODULES) or any(len(selected[key]) != 15 for key in selected):
        raise ValueError("Pilot sample is not the sealed 15/15/15 selection")
    samples = []
    for subsystem, rows in selected.items():
        module, retail_hash, current_hash, base = SUBSYSTEM_MODULES[subsystem]
        for address, size in rows:
            va = int(address, 16)
            samples.append((subsystem, module, retail_hash, current_hash, base, va, int(size)))
    if len({(row[2], row[5]) for row in samples}) != 45:
        raise ValueError("Duplicate pilot retail address")

    vtable = json.loads(args.vtable.read_text(encoding="utf-8"))
    getter_map, getter_family, retail_getters, table_status = v3.table_evidence(vtable)
    if table_status != "BODY_EQUIVALENT_CONTEXT_DISTINGUISHED":
        raise ValueError("Frozen vtable evidence no longer resolves")
    if any((module == "Conquest_retail.exe" and va - base in retail_getters | {0x83BF0})
           or (module == "Trim.dll" and va - base == 0x61240)
           for _, module, _, _, base, va, _ in samples):
        raise ValueError("Selection includes a v3 development-context identity")

    original = sqlite3.connect(f"file:{args.v2_db.resolve().as_posix()}?mode=ro", uri=True)
    output = sqlite3.connect(args.output_db)
    try:
        original.backup(output)
        for _, _, retail_hash, _, _, va, size in samples:
            saved = output.execute("SELECT size FROM retail_functions WHERE module_sha256=? AND va=?",
                                   (retail_hash, va)).fetchone()
            if saved != (size,):
                raise ValueError(f"Not the saved Function boundary: {va:#x}, {size}")
        addresses_by_hash = {body.RETAIL_EXE: args.retail_exe,
                             body.RETAIL_TRIM: args.retail_trim}
        feature_map = {}
        for module_hash, image in addresses_by_hash.items():
            addresses, instructions = extractor.read_disassembly(image)
            for _, _, retail_hash, _, base, va, size in samples:
                if retail_hash == module_hash:
                    feature = extractor.feature(addresses, instructions, va, size)
                    if feature is None:
                        raise ValueError(f"Unextractable saved Function: {va:#x}")
                    feature_map[(module_hash, va-base)] = feature
        with output:
            for (module_hash, rva), feature in feature_map.items():
                output.execute("INSERT INTO machine_features VALUES (?,?,?,?,?,?,?,?,?)",
                    ("RETAIL", module_hash, rva, "normalized_ir_json",
                     json.dumps(feature, separators=(",", ":")), 1,
                     method["feature_extractor_sha256"], extractor.VERSION,
                     "sealed blind pilot saved retail Function; unchanged v2 extractor"))
        current = {module: body.load_features(output, module)
                   for module in (body.CURRENT_EXE, body.CURRENT_TRIM)}
        retail = {module: body.load_features(output, module)
                  for module in (body.RETAIL_EXE, body.RETAIL_TRIM)}
        idf = {module: body.weights(values) for module, values in current.items()}
        context_inputs = {"getter_map": getter_map, "getter_family": getter_family,
                          "retail_getters": retail_getters,
                          "source_classes": v3.source_class_map(output, body.CURRENT_EXE),
                          "retail_subtype": None, "subtypes": {},
                          "retail_strings": set(), "trim_strings": {},
                          "string_frequency": {}, "trim_population": len(current[body.CURRENT_TRIM])}
        summary = {"schema": "cfw-retail-map-v3-blind-pilot-raw-1",
                   "status": "FROZEN_V3_UNSEEN_REPLAY_PRE_GROUND_TRUTH",
                   "freeze_sha256": extractor.digest(args.freeze),
                   "selection_sha256": selection_hash,
                   "source_v2_db_sha256": inputs["v2_database"],
                   "v3_validator_sha256": method["validator_sha256"],
                   "extractor_sha256": method["feature_extractor_sha256"],
                   "v2_scorer_sha256": method["v2_scorer_sha256"],
                   "vtable_capture_sha256": inputs["vtable_capture"],
                   "calibrated_confidence": "UNAVAILABLE_IN_FROZEN_V3",
                   "graph_propagation": "NONE_FOR_SELECTED_UNSEEN_IDENTITIES",
                   "rows": []}
        with args.output_raw.open("xb") as stream:
            with gzip.GzipFile(filename="", fileobj=stream, mode="wb", mtime=0) as zipped:
                for subsystem, module, retail_hash, current_hash, base, va, size in samples:
                    rva = va - base
                    body_rows, _ = body.rank_control(retail[retail_hash][rva],
                                                     current[current_hash], idf[current_hash], None)
                    ranked = v3.rank_rows(body_rows, module, rva, context_inputs, "full")
                    if any(row["context"]["contextual_evidence_count"] for row in ranked):
                        raise ValueError(f"Unexpected contextual bonus for unseen identity {va:#x}")
                    no_match = not ranked
                    top10 = [item for item in ranked if item["rank"] <= 10]
                    record = {"subsystem": subsystem, "retail_module": module,
                              "retail_module_sha256": retail_hash, "retail_va": f"0x{va:08X}",
                              "retail_rva": f"0x{rva:X}", "saved_body_size": size,
                              "normalized_retail_feature": feature_map[(retail_hash, rva)],
                              "current_module_sha256": current_hash,
                              "candidate_count": len(ranked),
                              "no_match": no_match,
                              "confidence_category": "UNAVAILABLE_IN_FROZEN_V3",
                              "top10_rvas": [f"0x{item['rva']:X}" for item in top10],
                              "candidates": ranked}
                    zipped.write(json_line(record))
                    summary["rows"].append({"subsystem": subsystem, "retail_va": f"0x{va:08X}",
                                            "candidate_count": len(ranked),
                                            "no_match": no_match,
                                            "top10": [{"rva": f"0x{item['rva']:X}",
                                                       "score": item["score"],
                                                       "rank": item["rank"],
                                                       "rank_max": item["rank_max"],
                                                       "equivalence_size": item["equivalence_size"]}
                                                      for item in top10]})
                    print(f"{subsystem} {va:#x}: {len(ranked)} candidates, "
                          f"top tie {ranked[0]['equivalence_size'] if ranked else 0}", flush=True)
        summary["raw_sha256"] = extractor.digest(args.output_raw)
        summary["pilot_db_sha256"] = extractor.digest(args.output_db)
        args.output_summary.write_bytes(json.dumps(summary, sort_keys=True,
                                                  separators=(",", ":")).encode("utf-8"))
        print(json.dumps({"raw_sha256": summary["raw_sha256"],
                          "db_sha256": summary["pilot_db_sha256"],
                          "summary_sha256": extractor.digest(args.output_summary),
                          "sampled": len(summary["rows"])}))
    finally:
        output.close()
        original.close()


if __name__ == "__main__":
    main()
