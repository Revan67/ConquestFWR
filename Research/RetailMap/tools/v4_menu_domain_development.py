#!/usr/bin/env python3
"""Evaluate a frozen, non-pruning Menu_tb prior on the old V4 development set.

`score` never reads reviewed identities. `review` opens them only after the
scored capture exists. This is a diagnostic prototype, not Matcher V4.
"""
from __future__ import annotations

import argparse
from bisect import bisect_left, bisect_right
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys


VERSION = "v4-menu-domain-development-5"
SCHEMA = "cfw.v4.menu-domain-development.v5"
EPSILON = 0.005
PRIVATE = Path(__file__).resolve().parents[3] / "Evidence/RetailMap/local"
UI_OWNERS = frozenset({
    "Menu_Toolbar.obj", "Menu_mshell.obj", "Menu1.obj", "Menu_options.obj",
    "BuildButton.obj", "ResearchButton.obj", "MultiHotButton.obj",
    "Button2.obj", "Teletype.obj", "Listbox.obj", "Combobox.obj",
    "Dropdown.obj", "TabButton.obj", "Edit2.obj", "Slider.obj", "Cursor.obj",
})
WRONG_DOMAIN_OWNERS = frozenset({
    "Camera.obj", "cqpipeline.obj", "NetPacket.obj", "Animate.obj",
    "SoundManager.obj", "NetFileTransfer.obj", "NetBuffer.obj",
})
# A priori tiny prototype weights. They are not fitted to reviewed identities.
SCORE = {"generic_ui": 0.008, "exact_slot_instance": 0.040,
         "exact_slot_wrapper": 0.015, "callback_callee_instance": 0.020,
         "callback_callee_wrapper": 0.005, "other_one_hop_instance": 0.005,
         "wrong_domain": -0.005}


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest().upper()


def read(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def write(path: Path, payload: dict) -> None:
    path = path.resolve()
    if PRIVATE.resolve() not in path.parents:
        raise ValueError("development captures must remain in private evidence")
    if path.exists():
        raise FileExistsError(path)
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(payload, stream, sort_keys=True, separators=(",", ":"))
        stream.write("\n")
    print(json.dumps({"path": str(path.resolve()), "sha256": digest(path),
                      "summary": payload["summary"]}, sort_keys=True))


def current_class(flags: set[str]) -> str:
    if "DIRECT_MEMBER" in flags and "STATIC_CLASS_FUNCTION" not in flags:
        return "DIRECT_INSTANCE"
    if "WRAPPER_MEMBER" in flags:
        return "WRAPPER"
    if "NESTED_CLASS_MEMBER" in flags:
        return "NESTED_OBJECT"
    if "STATIC_CLASS_FUNCTION" in flags:
        return "STATIC_CLASS_FUNCTION"
    if "OWNED_OBJECT_FILE_HELPER" in flags:
        return "SAME_OBJECT_HELPER"
    if "ONE_HOP_CLASS_GRAPH" in flags:
        return "ONE_HOP_CONTEXT"
    if "TWO_HOP_CLASS_GRAPH" in flags:
        return "TWO_HOP_CONTEXT"
    return "NO_CLASS_EVIDENCE"


def retail_tier(flags: set[str]) -> str:
    if "CONSTRUCTOR_INSTALLED_TABLE_SLOT" in flags:
        return "OBSERVED_TABLE_SLOT"
    if "ONE_HOP_CALLBACK_CALLEE" in flags:
        return "CALLBACK_CALLEE_CONTEXT"
    if "ONE_HOP_FAMILY_GRAPH" in flags:
        return "OTHER_ONE_HOP_CONTEXT"
    if "TWO_HOP_FAMILY_GRAPH" in flags:
        return "TWO_HOP_CONTEXT"
    return "NO_FAMILY_EVIDENCE"


def rank(rows: list[dict], key: str) -> dict[int, list[int]]:
    scores = sorted(row[key] for row in rows)
    count = len(scores)
    return {row["rva"]: [count - bisect_right(scores, row[key] + EPSILON) + 1,
                          count - bisect_left(scores, row[key] - EPSILON)]
            for row in rows}


def score(args: argparse.Namespace) -> dict:
    capture, bridge, ranking = read(args.ownership), read(args.domain), read(args.ranking)
    if capture["schema"] != "cfw.v4.menu-ownership-capture.v7" or \
            bridge["schema"] != "cfw.v4.trim-domain-provenance.v1" or \
            ranking["status"] != "LABEL_FREE_OLD_SAMPLE_DEVELOPMENT_RANKING":
        raise ValueError("ownership/domain/ranking schema or status mismatch")
    if digest(args.domain) != capture["inputs"]["domain"]["sha256"]:
        raise ValueError("scoring bridge differs from independently reviewed ownership input")
    if capture["current"]["module_sha256"] != bridge["current_contract"]["module_sha256"] or \
            capture["retail"]["module_sha256"] != bridge["retail_contract"]["module_sha256"]:
        raise ValueError("module identity mismatch")
    for function in ranking["functions"]:
        if function["subsystem"] == "Trim/UI" and (
                function["current_module_sha256"] != capture["current"]["module_sha256"] or
                function["retail_module_sha256"] != capture["retail"]["module_sha256"]):
            raise ValueError("ranking function module identity mismatch")
    own = {int(row["rva"], 16): set(row["flags"]) for row in capture["current"]["rows"]}
    retail = {row["va"]: set(row["flags"]) for row in capture["retail"]["rows"]}
    owners: dict[int, set[str]] = {}
    for row in bridge["current_function_bridge"]:
        if row["identity_status"] == "MAP_COFF_SOURCE_BYTES_VERIFIED":
            owners.setdefault(int(row["va"], 16) - capture["current"]["image_base"], set()).add(row["owner"])
    out = []
    for function in ranking["functions"]:
        if function["subsystem"] != "Trim/UI":
            continue
        tier = retail_tier(retail.get(function["retail_va"], set()))
        rows = []
        for candidate in function["candidates"]:
            rva = candidate["rva"]
            flags = own.get(rva, set())
            category = current_class(flags)
            owner_set = owners.get(rva, set())
            ui = bool(owner_set & UI_OWNERS)
            wrong = bool(owner_set & WRONG_DOMAIN_OWNERS)
            generic = SCORE["generic_ui"] if tier != "NO_FAMILY_EVIDENCE" and ui else 0.0
            member = 0.0
            if tier == "OBSERVED_TABLE_SLOT":
                member = (SCORE["exact_slot_instance"] if category == "DIRECT_INSTANCE" else
                          SCORE["exact_slot_wrapper"] if category == "WRAPPER" else 0.0)
            elif tier == "CALLBACK_CALLEE_CONTEXT":
                member = (SCORE["callback_callee_instance"] if category == "DIRECT_INSTANCE" else
                          SCORE["callback_callee_wrapper"] if category == "WRAPPER" else 0.0)
            elif tier == "OTHER_ONE_HOP_CONTEXT" and category == "DIRECT_INSTANCE":
                member = SCORE["other_one_hop_instance"]
            # Only an observed table slot licenses negative weak evidence.
            if tier == "OBSERVED_TABLE_SLOT" and wrong:
                member += SCORE["wrong_domain"]
            body = candidate["body_score"]
            full = candidate["full"]
            rows.append({"rva": rva, "body": body, "v4_full": full,
                         "body_ui": body + generic, "body_class": body + member,
                         "body_both": body + generic + member,
                         "full_class": full + member,
                         "class": category, "ui_owner": ui,
                         "wrong_domain_owner": wrong,
                         "generic_delta": generic, "class_delta": member})
        for method in ("body", "v4_full", "body_ui", "body_class", "body_both", "full_class"):
            ranks = rank(rows, method)
            for row in rows:
                row.setdefault("rank_intervals", {})[method] = ranks[row["rva"]]
        out.append({"retail_va": function["retail_va"], "retail_tier": tier,
                    "candidate_count": len(rows),
                    "pool": {"direct_instance": sum(r["class"] == "DIRECT_INSTANCE" for r in rows),
                             "wrapper": sum(r["class"] == "WRAPPER" for r in rows),
                             "nested": sum(r["class"] == "NESTED_OBJECT" for r in rows),
                             "same_object_helper": sum(r["class"] == "SAME_OBJECT_HELPER" for r in rows),
                             "generic_ui_owner": sum(r["ui_owner"] for r in rows),
                             "outside_generic_ui": sum(not r["ui_owner"] for r in rows),
                             "wrong_domain_owner": sum(r["wrong_domain_owner"] for r in rows)},
                    "candidates": rows})
    return {"schema": SCHEMA, "version": VERSION, "mode": "LABEL_FREE_DEVELOPMENT_SCORING",
            "status": "PROTOTYPE_NOT_MATCHER_V4", "tool_sha256": digest(Path(__file__)),
            "commit": capture["commit"], "python_version": sys.version.split()[0],
            "retail_module_sha256": capture["retail"]["module_sha256"],
            "current_module_sha256": capture["current"]["module_sha256"],
            "inputs": {name: {"path": str(path.resolve()), "sha256": digest(path)}
                       for name, path in (("ownership", args.ownership),
                                          ("domain", args.domain), ("ranking", args.ranking))},
            "configuration": {"tie_epsilon": EPSILON, "ui_owners": sorted(UI_OWNERS),
                              "wrong_domain_owners": sorted(WRONG_DOMAIN_OWNERS),
                              "fixed_weights": SCORE, "no_hard_pruning": True,
                              "no_pilot_truth": True},
            "functions": out,
            "summary": {"functions": len(out),
                        "total_candidates": sum(x["candidate_count"] for x in out),
                        "class_tiers": dict(Counter(x["retail_tier"] for x in out))}}


def review(args: argparse.Namespace) -> dict:
    scored, posthoc = read(args.scored), read(args.posthoc)
    if scored["schema"] != SCHEMA or scored["mode"] != "LABEL_FREE_DEVELOPMENT_SCORING" or \
            posthoc["ranking_sha256"] != scored["inputs"]["ranking"]["sha256"]:
        raise ValueError("scored/posthoc provenance mismatch")
    truths = {row["retail_va"]: row for row in posthoc["rows"] if row["subsystem"] == "Trim/UI"}
    reviewed = []
    for function in scored["functions"]:
        truth = truths[function["retail_va"]]
        candidates = {row["rva"]: row for row in function["candidates"]}
        true_rows = [candidates[int(value, 16)] for value in truth["true_current_rvas"]
                     if int(value, 16) in candidates]
        missing_rvas = [value for value in truth["true_current_rvas"]
                        if int(value, 16) not in candidates]
        reviewed.append({"retail_va": function["retail_va"],
                         "retail_tier": function["retail_tier"],
                         "resolved": truth["rank_scored"],
                         "candidate_count": function["candidate_count"],
                         "pool": function["pool"],
                         "expected_true_rvas": truth["true_current_rvas"],
                         "true": [{"rva": f"0x{row['rva']:x}", "class": row["class"],
                                   "ui_owner": row["ui_owner"],
                                   "rank_intervals": row["rank_intervals"]}
                                  for row in true_rows],
                         "missing_true_rvas": missing_rvas,
                         "missing_true_candidate": bool(missing_rvas)})
    resolved = [row for row in reviewed if row["resolved"]]
    methods = ("body", "v4_full", "body_ui", "body_class", "body_both", "full_class")
    metrics = {method: {"top1": sum(min((t["rank_intervals"][method][1] for t in row["true"]), default=float("inf")) <= 1
                                   for row in resolved),
                        "top5": sum(min((t["rank_intervals"][method][1] for t in row["true"]), default=float("inf")) <= 5
                                   for row in resolved),
                        "top10": sum(min((t["rank_intervals"][method][1] for t in row["true"]), default=float("inf")) <= 10
                                    for row in resolved)} for method in methods}
    return {"schema": SCHEMA, "version": VERSION, "mode": "POSTHOC_OLD_DEVELOPMENT_REVIEW",
            "status": "DEVELOPMENT_ONLY_NOT_VALIDATION", "tool_sha256": digest(Path(__file__)),
            "commit": scored["commit"], "python_version": sys.version.split()[0],
            "inputs": {"scored": {"path": str(args.scored.resolve()), "sha256": digest(args.scored)},
                       "posthoc": {"path": str(args.posthoc.resolve()), "sha256": digest(args.posthoc)}},
            "rows": reviewed,
            "summary": {"sampled": len(reviewed), "resolved": len(resolved),
                        "unresolved": len(reviewed) - len(resolved), "metrics": metrics,
                        "resolved_with_missing_true_candidate": sum(bool(r["missing_true_rvas"])
                                                                    for r in resolved),
                        "true_direct_instance": sum(any(t["class"] == "DIRECT_INSTANCE" for t in r["true"])
                                                    for r in resolved),
                        "true_outside_generic_ui": sum(bool(r["true"])
                                                       and all(not t["ui_owner"] for t in r["true"])
                                                       for r in resolved)}}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="mode", required=True)
    score_parser = sub.add_parser("score")
    for name in ("ownership", "domain", "ranking", "output"):
        score_parser.add_argument("--" + name, type=Path, required=True)
    review_parser = sub.add_parser("review")
    for name in ("scored", "posthoc", "output"):
        review_parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    write(args.output, score(args) if args.mode == "score" else review(args))


if __name__ == "__main__":
    main()
