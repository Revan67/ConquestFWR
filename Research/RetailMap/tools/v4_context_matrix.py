#!/usr/bin/env python3
"""Audit old-development context availability without matching or scoring.

Reviewed current RVAs are used only to measure paired evidence availability.
The output is evaluation-only and must never become matcher input or an anchor.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import subprocess
import sys


VERSION = "v4-context-matrix-1"
SCHEMA = "cfw.v4.context-matrix.v1"
ROOT = Path(__file__).resolve().parents[3]
PRIVATE = ROOT / "Evidence/RetailMap/local"
FIELDS = (
    "MODULE", "SOURCE_OBJECT", "SOURCE_CLASS", "DIRECT_CLASS", "VTABLE",
    "CALLBACK_TABLE", "CONSTRUCTOR_LIFECYCLE", "CALLERS", "CALLEES",
    "ONE_HOP_GRAPH", "TWO_HOP_GRAPH", "IMPORTS", "RESOURCES", "STRINGS",
    "GLOBAL_DATA", "SWITCH_DATA", "EVENT_FRAMEWORK", "DOMAIN", "SEED_NEIGHBOR",
)
STATUSES = {"AVAILABLE_BOTH_SIDES", "RETAIL_ONLY", "CURRENT_ONLY",
            "EXTRACTABLE_MISSING", "NOT_SUPPORTED", "UNKNOWN"}


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest().upper()


def read(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def index_graph(graph: dict) -> dict:
    outgoing, incoming, imports = defaultdict(set), defaultdict(set), defaultdict(set)
    for edge in graph["edges"]:
        if edge.get("ownership_ambiguous"):
            continue
        owner = edge["owner"]
        if edge["kind"] == "DIRECT_CALL" and edge.get("target_is_function_entry"):
            outgoing[owner].add(edge["target"])
            incoming[edge["target"]].add(owner)
        imported = edge.get("import_name") or edge.get("import_via_one_hop_thunk")
        if imported and edge["instruction_kind"] == "call":
            imports[owner].add(imported)
    return {"out": outgoing, "in": incoming, "imports": imports}


def two_hops(graph: dict, va: int) -> bool:
    first = graph["in"][va] | graph["out"][va]
    return bool(set().union(*(graph["in"][x] | graph["out"][x] for x in first)) - first - {va}) if first else False


def field(retail: bool, current: bool, *, extracted: bool = True,
          comparable: bool = False, quality: str = "UNKNOWN", note: str = "") -> dict:
    if retail and current:
        status = "AVAILABLE_BOTH_SIDES"
    elif retail:
        status = "RETAIL_ONLY"
    elif current:
        status = "CURRENT_ONLY"
    elif extracted:
        status = "UNKNOWN"
    else:
        status = "EXTRACTABLE_MISSING"
    assert status in STATUSES
    return {"status": status, "retail_observed": retail, "current_observed": current,
            "paired_comparable": comparable, "quality": quality, "note": note}


def inventory(args: argparse.Namespace) -> dict:
    old, graph, menu = read(args.old), read(args.graph), read(args.menu)
    if (old["schema"] != "cfw.v4-context-coverage-inventory.v1"
            or graph["status"] != "INDEPENDENT_METADATA_CAPTURE_NO_GROUND_TRUTH"
            or menu["schema"] != "cfw.v4.menu-ownership-capture.v7"):
        raise ValueError("input schema/status mismatch")
    modules = {m["retail_module_sha256"]: m for m in graph["modules"]}
    menu_retail = {int(r["va"], 16): set(r["flags"]) for r in menu["retail"]["rows"]}
    menu_current = {int(r["rva"], 16): set(r["flags"]) for r in menu["current"]["rows"]}
    indexed = {}
    for retail_hash, module in modules.items():
        indexed[retail_hash] = {"module": module, "retail": index_graph(module["retail_graph"]),
                                "current": index_graph(module["current_graph"])}
    rows = []
    for old_row in old["functions"]:
        module_hash = old_row["retail_module_sha256"]
        m = indexed[module_hash]
        module, rg, cg = m["module"], m["retail"], m["current"]
        retail_va = int(old_row["retail_va"], 16)
        current_rvas = [int(v, 16) for v in old_row["channels"]["source_compiled_bridge"]["current_rvas"]]
        current_vas = [v + module["current_image_base"] for v in current_rvas]
        paired = bool(current_vas)
        source_id = old_row["channels"]["source_compiled_bridge"]["source_id"]
        source_class = source_id.split(":", 2)[-1].split("::", 1)[0] if source_id and "::" in source_id else None
        rflags = menu_retail.get(retail_va, set()) if old_row["subsystem"] == "Trim/UI" else set()
        cflags = set().union(*(menu_current.get(x, set()) for x in current_rvas)) if current_rvas else set()
        rc = old_row["channels"]
        def current_any(lookup: str) -> bool:
            return any(bool(cg[lookup][va]) for va in current_vas)
        result = {}
        result["MODULE"] = field(True, True, comparable=False, quality="WEAK",
                                 note="Hashed module identity, not function discrimination.")
        result["SOURCE_OBJECT"] = field(False, bool(source_id), quality="WEAK",
                                        note="Current compiled/source owner only; no retail object-file identity.")
        result["SOURCE_CLASS"] = field(False, bool(source_class), quality="MODERATE" if source_class else "UNKNOWN",
                                       note="Current source class is not automatically a retail class.")
        result["DIRECT_CLASS"] = field(False, "DIRECT_MEMBER" in cflags and "STATIC_CLASS_FUNCTION" not in cflags,
                                       quality="STRONG" if "DIRECT_MEMBER" in cflags else "UNKNOWN",
                                       note="Current source-byte-verified instance method; retail class receiver unproven.")
        result["VTABLE"] = field(False, "COMPILER_NAMED_TABLE_SLOT_CANDIDATE" in cflags,
                                  quality="UNKNOWN", note="Candidate table membership is not a proven cross-side slot pair.")
        retail_callback = "CONSTRUCTOR_INSTALLED_TABLE_SLOT" in rflags
        current_callback = "COMPILER_NAMED_TABLE_SLOT_CANDIDATE" in cflags
        result["CALLBACK_TABLE"] = field(retail_callback, current_callback,
                                         comparable=retail_callback and current_callback,
                                         quality="MODERATE" if retail_callback and current_callback else "UNKNOWN",
                                         note="Only observed slot 3 has bounded family evidence; pairing remains CANDIDATE.")
        lifecycle_retail = bool(rflags & {"CONSTRUCTOR", "DESTRUCTOR_CANDIDATE", "CONSTRUCTOR_INSTALLED_TABLE_SLOT"})
        lifecycle_current = bool(cflags & {"CONSTRUCTOR_DESTRUCTOR", "COMPILER_NAMED_TABLE_SLOT_CANDIDATE"})
        result["CONSTRUCTOR_LIFECYCLE"] = field(lifecycle_retail, lifecycle_current,
                                                comparable=lifecycle_retail and lifecycle_current and retail_callback,
                                                quality="MODERATE" if retail_callback and current_callback else "UNKNOWN")
        for name, direction in (("CALLERS", "in"), ("CALLEES", "out")):
            result[name] = field(bool(rg[direction][retail_va]), current_any(direction),
                                 comparable=False, quality="WEAK",
                                 note="Observed graph edges on both sides are not yet semantic endpoint pairs.")
        result["ONE_HOP_GRAPH"] = field(bool(rg["in"][retail_va] | rg["out"][retail_va]),
                                         current_any("in") or current_any("out"),
                                         comparable=False, quality="WEAK")
        result["TWO_HOP_GRAPH"] = field(two_hops(rg, retail_va), any(two_hops(cg, x) for x in current_vas),
                                         comparable=False, quality="WEAK")
        result["IMPORTS"] = field(bool(rg["imports"][retail_va]), current_any("imports"),
                                   comparable=False, quality="UNKNOWN",
                                   note="Direct import names are available; rarity and function-level validation pending.")
        result["RESOURCES"] = field(False, False, extracted=False,
                                     note="Require resource API/reference path; numeric constants are insufficient.")
        result["STRINGS"] = field(rc["retail_strings"]["saved_hashed_references"] > 0, False,
                                   extracted=False, note="Retail saved string xrefs exist; general current function refs not indexed.")
        result["GLOBAL_DATA"] = field(rc["retail_data_references"]["kinds"].get("global_reference", 0) > 0, False,
                                       extracted=False, note="Retail data xrefs exist; current global/data index pending.")
        result["SWITCH_DATA"] = field(False, False, extracted=False,
                                       note="Switch/case table ownership not independently indexed.")
        result["EVENT_FRAMEWORK"] = field(retail_callback, current_callback,
                                           comparable=retail_callback and current_callback,
                                           quality="MODERATE" if retail_callback and current_callback else "UNKNOWN",
                                           note="Only the bounded toolbar callback/event site is reviewed.")
        result["DOMAIN"] = field(bool(rflags), bool(cflags),
                                  comparable=retail_callback and current_callback,
                                  quality="WEAK" if rflags or cflags else "UNKNOWN",
                                  note="Broad graph proximity does not prove a class or transferable UI domain.")
        result["SEED_NEIGHBOR"] = field(False, False, extracted=False,
                                         note="Reviewed Tier-0 paths are bounded; development identities cannot seed this run.")
        assert set(result) == set(FIELDS)
        # Three toolbar fields above derive from the same single callback-table
        # observation. They count as one candidate family, never three.
        candidate_families = ["TABLE_LIFECYCLE_EVENT"] if retail_callback and current_callback else []
        rows.append({"subsystem": old_row["subsystem"], "retail_va": old_row["retail_va"],
                     "retail_module_sha256": module_hash,
                     "current_rvas_for_posthoc_coverage_only": [hex(x) for x in current_rvas],
                     "paired_identity_available": paired, "fields": result,
                     "paired_context_family_candidates": candidate_families,
                     "admitted_transferable_families": []})
    if len(rows) != 45:
        raise ValueError("expected sealed 45-function development inventory")
    summary = {}
    for subsystem in ("Object/archetype", "Flagship", "Trim/UI", "ALL"):
        group = [r for r in rows if subsystem == "ALL" or r["subsystem"] == subsystem]
        summary[subsystem] = {
            "functions": len(group),
            "paired_identities": sum(r["paired_identity_available"] for r in group),
            "candidate_context_at_least_one": sum(bool(r["paired_context_family_candidates"]) for r in group),
            "meaningful_context_at_least_one": sum(bool(r["admitted_transferable_families"]) for r in group),
            "meaningful_context_at_least_two": sum(len(r["admitted_transferable_families"]) >= 2 for r in group),
            "field_status_counts": {name: dict(Counter(r["fields"][name]["status"] for r in group))
                                    for name in FIELDS},
        }
    paths = {"old": args.old, "graph": args.graph, "menu": args.menu}
    return {"schema": SCHEMA, "tool_version": VERSION, "tool_sha256": digest(Path(__file__)),
            "python_version": sys.version.split()[0],
            "commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
            "status": "COMPLETE_EVALUATION_ONLY_NOT_MATCHER_INPUT",
            "configuration": {"fields": FIELDS, "quality_credit": ["STRONG", "MODERATE"],
                              "use_reviewed_rvas": "posthoc coverage measurement only"},
            "inputs": {name: {"path": str(path.resolve()), "sha256": digest(path)}
                       for name, path in paths.items()},
            "limitations": ["Both-side availability is not necessarily candidate-discriminating transfer.",
                            "Saved graph absence is UNKNOWN, never proof of no call.",
                            "Five unresolved Trim identities have no current true RVA for posthoc comparison.",
                            "The bounded Menu_tb bridge is CANDIDATE and cannot alone justify V4 admission.",
                            "Three toolbar class/table/event fields are one correlated family, not three independent ones.",
                            "No family is admitted as meaningful transferable context by this availability audit."],
            "summary": summary, "functions": rows}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("old", "graph", "menu", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    target = args.output.resolve()
    if target.exists() or PRIVATE.resolve() not in target.parents:
        parser.error("output must be a new private evidence file")
    payload = inventory(args)
    with target.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(payload, stream, sort_keys=True, separators=(",", ":"))
        stream.write("\n")
    print(json.dumps({"output": str(target), "sha256": digest(target),
                      "summary": payload["summary"]}, sort_keys=True))


if __name__ == "__main__":
    main()
