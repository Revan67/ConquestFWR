#!/usr/bin/env python3
"""Check matcher leads against independently reviewed function-identity controls."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sqlite3


def retail_id(connection, module: str, va: str) -> str | None:
    row = connection.execute(
        "SELECT retail_id FROM retail_functions WHERE module_name=? AND va=?",
        (module, int(va, 16)),
    ).fetchone()
    return row[0] if row else None


def missing_retail_status(connection, module: str, va: str) -> str:
    # Currently the canonical EXE alone is bulk-indexed. A code lead is not
    # promoted to a function merely because a bounded block was decoded.
    header = connection.execute("SELECT module_sha256 FROM retail_functions WHERE module_name=? LIMIT 1",
                                (module,)).fetchone()
    if header and connection.execute(
        "SELECT 1 FROM retail_code_leads WHERE module_sha256=? AND va=?",
        (header[0], int(va, 16)),
    ).fetchone():
        return "BOUNDED_CODE_ONLY"
    return "RETAIL_FUNCTION_NOT_INDEXED"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--db", required=True, type=Path)
    parser.add_argument("--anchors", type=Path,
                        default=Path(__file__).resolve().parents[1] / "validation_anchors.json")
    parser.add_argument("--matcher-run", required=True)
    args = parser.parse_args()
    anchors = json.loads(args.anchors.read_text(encoding="utf-8"))
    results = []
    with sqlite3.connect(args.db) as connection:
        for control in anchors["controls"]:
            fid = retail_id(connection, control["module"], control["va"])
            source_id = control["source_id"]
            source_exists = connection.execute(
                "SELECT 1 FROM source_functions WHERE source_id=?", (source_id,)
            ).fetchone() is not None
            if fid is None:
                outcome = missing_retail_status(connection, control["module"], control["va"])
                rank = None
            elif not source_exists:
                outcome = "SOURCE_FUNCTION_NOT_INDEXED"
                rank = None
            else:
                row = connection.execute(
                    "SELECT rank,score,signal_groups FROM match_candidates "
                    "WHERE matcher_run_id=? AND retail_id=? AND source_id=?",
                    (args.matcher_run, fid, source_id),
                ).fetchone()
                rank = row[0] if row else None
                outcome = "RANKED" if row else "NOT_RANKED"
            results.append({"kind": "positive", "family": control["family"],
                            "role": control["role"], "module": control["module"],
                            "va": control["va"], "outcome": outcome, "rank": rank})
        for control in anchors["hard_negatives"]:
            fid = retail_id(connection, control["module"], control["va"])
            if fid is None:
                outcome = missing_retail_status(connection, control["module"], control["va"])
                rank = None
            else:
                row = connection.execute(
                    "SELECT rank FROM match_candidates WHERE matcher_run_id=? AND retail_id=? AND source_id=?",
                    (args.matcher_run, fid, control["wrong_source_id"]),
                ).fetchone()
                rank = row[0] if row else None
                outcome = "FALSE_POSITIVE" if row else "REJECTED"
            results.append({"kind": "hard_negative", "module": control["module"],
                            "va": control["va"], "outcome": outcome, "rank": rank})
    summary = {outcome: sum(item["outcome"] == outcome for item in results)
               for outcome in sorted({item["outcome"] for item in results})}
    print(json.dumps({"schema": "cfw-matcher-validation-v1", "matcher_run": args.matcher_run,
                      "summary": summary, "results": results}, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
