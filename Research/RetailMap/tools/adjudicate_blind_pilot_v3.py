#!/usr/bin/env python3
"""Apply a separately evidenced identity adjudication to sealed pilot results.

The primary ground truth, raw matcher rankings, and original evaluation stay
immutable. This tool changes no matcher weights, features, or candidates.
"""
from __future__ import annotations

import argparse
import gzip
import hashlib
import json
from pathlib import Path
from statistics import median


PRIMARY_SHA256 = "B49037504EB021F83DC56D1C09B96FC4D34F5893F3E662834F0BAE7CC2893D7E"
RAW_SHA256 = "B0BAAC8AE2326DBEB6E1E012B19D9FF6E5911FECD450C110062E1EF33DB2D3CD"
CROSSCHECK_SHA256 = "BED064F3B69F16935C78884C260189EB1B168A8AE98C4CF740D76665BAF9DFDB"
RETAIL_VA = "0x10062520"
SOURCE_ID = "Code/App/Src/Menu_Toolbar.cpp:1100:Menu_tb::sendMouseUpdate"
CURRENT_RVA = 0x949B0


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def metrics(rows: list[dict]) -> dict:
    eligible = [row for row in rows if row["rank_scored"]]
    ranks = [row["true_rank_max"] for row in eligible]
    found = [rank for rank in ranks if rank is not None]
    return {
        "denominator": len(ranks),
        **{f"top_{limit}": sum(rank is not None and rank <= limit for rank in ranks)
           for limit in (1, 3, 5, 10)},
        "mrr_conservative": None if not ranks else round(
            sum(1 / rank for rank in found) / len(ranks), 6),
        "median_true_rank": None if not found else median(found),
        "candidate_generation_misses": sum(rank is None for rank in ranks),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("primary", "raw", "crosscheck", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    private = Path("Evidence/RetailMap/local").resolve()
    if args.output.exists() or private not in args.output.resolve().parents:
        raise ValueError("Adjudication output must be new and private")
    for path, expected in ((args.primary, PRIMARY_SHA256), (args.raw, RAW_SHA256),
                           (args.crosscheck, CROSSCHECK_SHA256)):
        if digest(path) != expected:
            raise ValueError(f"Sealed input hash changed: {path}")
    primary = json.loads(args.primary.read_text(encoding="utf-8"))
    crosscheck = json.loads(args.crosscheck.read_text(encoding="utf-8"))
    witness = next(row for row in crosscheck["rows"] if row["retail_entry"] == RETAIL_VA)
    if witness["source_id"] != SOURCE_ID or int(witness["current_rva"], 16) != CURRENT_RVA:
        raise ValueError("Cross-check identity does not match adjudicated identity")
    rows = primary["ranked"]
    target = next(row for row in rows if row["retail_va"].lower() == RETAIL_VA)
    if target["source_id"] != "Code/App/Src/Menu_Toolbar.cpp:2022:FullScreen::onSetFocus":
        raise ValueError("Primary hypothesis unexpectedly changed")
    with gzip.open(args.raw, "rt", encoding="utf-8") as stream:
        raw = next(json.loads(line) for line in stream
                   if json.loads(line)["retail_va"].lower() == RETAIL_VA)
    match = next(item for item in raw["candidates"] if item["rva"] == CURRENT_RVA)
    revised = [dict(row) for row in rows]
    row = next(item for item in revised if item["retail_va"].lower() == RETAIL_VA)
    row.update(source_id=SOURCE_ID, true_rva=f"0x{CURRENT_RVA:X}",
               true_rank=match["rank"], true_rank_max=match["rank_max"],
               bridge_status="INDEPENDENT_TRIM_MAP_COFF_CROSSCHECK",
               independent_evidence="Retail toolbar Notify case 0x102B (CQE_GAME_ACTIVE) calls this unconditional cursor-message helper after button visibility changes; current toggleVisibleButtons calls sendMouseUpdate(FULLSCREEN). FullScreen::onSetFocus instead branches on focus and is called from focus events.")
    output = {
        "schema": "cfw-retail-map-v3-blind-pilot-adjudication-1",
        "status": "POSTHOC_INDEPENDENT_IDENTITY_ADJUDICATION_NOT_MATCHER_TUNING",
        "input_sha256": {"primary": PRIMARY_SHA256, "raw": RAW_SHA256,
                         "blind_crosscheck": CROSSCHECK_SHA256},
        "dispute": {"retail_va": RETAIL_VA, "primary_source_id": target["source_id"],
                    "adjudicated_source_id": SOURCE_ID,
                    "retail_event": "0x102B = CQE_GAME_ACTIVE",
                    "source_event_definition": "Code/App/Src/Include/Globals.h:149",
                    "source_caller": "Code/App/Src/Menu_Toolbar.cpp:1380-1382",
                    "source_helper_call": "Code/App/Src/Menu_Toolbar.cpp:1132",
                    "independent_reason": row["independent_evidence"],
                    "original_evaluation_preserved": True},
        "metrics": {"aggregate": metrics(revised)},
        "unresolved_policy": "Five primary unresolved Trim identities remain excluded; independent cross-check resolves two and proposes one equivalence set, but those additions are not forced into the primary denominator.",
    }
    for subsystem in ("Object/archetype", "Flagship", "Trim/UI"):
        output["metrics"][subsystem] = metrics(
            [item for item in revised if item["subsystem"] == subsystem])
    args.output.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n",
                           encoding="utf-8")
    print(json.dumps({"output_sha256": digest(args.output), "metrics": output["metrics"]},
                     sort_keys=True))


if __name__ == "__main__":
    main()
