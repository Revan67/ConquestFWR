#!/usr/bin/env python3
"""Recompute development coverage and negatives from private raw context indexes.

This posthoc audit opens old reviewed identities only after label-free extraction.
It neither scores candidates nor changes Matcher V4.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import subprocess
import sys


VERSION = "v4-context-remeasure-1"
SCHEMA = "cfw.v4.context-remeasure.v1"
ROOT = Path(__file__).resolve().parents[3]
PRIVATE = ROOT / "Evidence/RetailMap/local"


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest().upper()


def read(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def status(retail: bool, current: bool) -> str:
    return ("AVAILABLE_BOTH_SIDES" if retail and current else "RETAIL_ONLY" if retail else
            "CURRENT_ONLY" if current else "UNKNOWN")


def tokens(row: dict, group: str) -> set:
    if group == "IMPORTS":
        return {x["name"] for x in row["imports"]}
    if group == "STRINGS":
        return {x["sha256_utf8"] for x in row["strings"]}
    if group == "RESOURCES":
        return {(x["kind"], x["id"]) for x in row["resource_id_candidates"]}
    raise ValueError(group)


def freq(row: dict, group: str, token) -> int:
    fields = {"IMPORTS": ("imports", "name"), "STRINGS": ("strings", "sha256_utf8"),
              "RESOURCES": ("resource_id_candidates", None)}
    name, key = fields[group]
    return min((x["module_function_frequency"] for x in row[name]
                if ((x[key] if key else (x["kind"], x["id"])) == token)), default=10**9)


def metadata_validation(module: dict, metadata_path: Path) -> dict:
    records = [json.loads(line) for line in metadata_path.read_text(encoding="utf-8").splitlines() if line]
    if (not records or records[0].get("record") != "header" or
            records[0].get("image_sha256", "").upper() != module["module_sha256"] or
            not records[-1].get("complete")):
        raise ValueError("retail metadata identity/completion mismatch")
    indexed = {r["rva"]: r for r in module["functions"]}
    seen, recovered, functions = 0, 0, 0
    examples = []
    for record in records:
        if record.get("record") != "function":
            continue
        expected = {ref["string_sha256_utf8"].upper() for ref in record.get("data_references", [])
                    if ref.get("kind") == "string_reference" and ref.get("string_sha256_utf8")}
        if not expected:
            continue
        rva = int(record["entry"], 16) - module["image_base"]
        row = indexed.get(rva)
        if row is None:
            continue
        observed = tokens(row, "STRINGS")
        functions += 1
        seen += len(expected)
        recovered += len(expected & observed)
        if expected - observed and len(examples) < 10:
            examples.append({"rva": hex(rva), "expected": len(expected),
                             "recovered": len(expected & observed)})
    return {"functions_with_saved_strings": functions, "saved_unique_function_hashes": seen,
            "recovered_unique_function_hashes": recovered, "miss_examples": examples}


def summarize(rows: list[dict]) -> dict:
    result = {}
    for group in ("Object/archetype", "Flagship", "Trim/UI", "ALL"):
        subset = [x for x in rows if group == "ALL" or x["subsystem"] == group]
        result[group] = {"functions": len(subset),
                         "paired_identities": sum(x["paired_identity_available"] for x in subset),
                         "candidate_family_at_least_one": sum(bool(x["candidate_families"]) for x in subset),
                         "candidate_family_at_least_two": sum(len(x["candidate_families"]) >= 2 for x in subset),
                         "admitted_meaningful_at_least_one": 0,
                         "admitted_meaningful_at_least_two": 0,
                         "fields": {name: dict(Counter(x["fields"][name]["status"] for x in subset))
                                    for name in ("IMPORTS", "RESOURCES", "STRINGS", "GLOBAL_DATA")}}
    return result


def run(args: argparse.Namespace) -> dict:
    initial, exe, trim, graph, ranked, review, domain, menu = (read(getattr(args, name)) for name in
                                               ("matrix", "exe", "trim", "graph", "ranking", "review", "domain", "menu"))
    if (initial["schema"] != "cfw.v4.context-matrix.v1" or
            exe["schema"] != trim["schema"] or
            exe["schema"] != "cfw.v4.resource-context.v2" or
            ranked["status"] != "PROTOTYPE_NOT_MATCHER_V4" or
            review["status"] != "DEVELOPMENT_ONLY_NOT_VALIDATION"):
        raise ValueError("input schema/status mismatch")
    if digest(args.graph) != initial["inputs"]["graph"]["sha256"]:
        raise ValueError("matrix/graph identity mismatch")
    for capture in (exe, trim):
        if digest(args.graph) != capture["inputs"]["graph"]["sha256"]:
            raise ValueError("resource/graph identity mismatch")
    graph_modules = {m["retail_module_sha256"]: m for m in graph["modules"]}
    resources = {m["module_sha256"]: m for capture in (exe, trim) for m in capture["modules"]}
    resource_rows = {key: {r["rva"]: r for r in m["functions"]} for key, m in resources.items()}
    rows = []
    for prior in initial["functions"]:
        retail_hash = prior["retail_module_sha256"]
        current_hash = graph_modules[retail_hash]["current_module_sha256"]
        rr = resource_rows[retail_hash].get(int(prior["retail_va"], 16) - resources[retail_hash]["image_base"])
        current_rvas = [int(v, 16) for v in prior["current_rvas_for_posthoc_coverage_only"]]
        cr = [resource_rows[current_hash][v] for v in current_rvas if v in resource_rows[current_hash]]
        if not rr:
            raise ValueError("selected retail function missing from new resource index")
        fields = {name: value.copy() for name, value in prior["fields"].items()}
        candidates = prior["paired_context_family_candidates"].copy()
        matched = {}
        for name in ("IMPORTS", "RESOURCES", "STRINGS"):
            rset = tokens(rr, name)
            cset = set().union(*(tokens(x, name) for x in cr)) if cr else set()
            shared = rset & cset
            fields[name] = {"status": status(bool(rset), bool(cset)),
                            "retail_observed": bool(rset), "current_observed": bool(cset),
                            "paired_comparable": bool(shared),
                            "quality": "UNKNOWN", "retail_token_count": len(rset),
                            "current_token_count": len(cset), "exact_shared_count": len(shared),
                            "note": "Exact overlap is a candidate signal, not admitted transfer."}
            if name == "RESOURCES":
                matched[name] = sorted({f"{kind}:{value}" for kind, value in shared})
            else:
                matched[name] = sorted(shared)
            if shared:
                rare = any(freq(rr, name, token) <= 10 and any(freq(x, name, token) <= 10 for x in cr)
                           for token in shared)
                if name == "STRINGS" and rare:
                    candidates.append("RARE_EXACT_STRING")
                elif name == "RESOURCES":
                    candidates.append("RESOURCE_API_ID_CANDIDATE")
                elif name == "IMPORTS" and rare:
                    candidates.append("RARE_EXACT_IMPORT")
        fields["GLOBAL_DATA"] = {"status": status(bool(rr["data_references"]),
                                                   any(x["data_references"] for x in cr)),
                                 "retail_observed": bool(rr["data_references"]),
                                 "current_observed": any(x["data_references"] for x in cr),
                                 "paired_comparable": False, "quality": "WEAK",
                                 "note": "Section/normalized addresses are indexed; global symbol identity is not."}
        rows.append({"subsystem": prior["subsystem"], "retail_va": prior["retail_va"],
                     "retail_module_sha256": retail_hash,
                     "current_rvas_for_posthoc_coverage_only": prior["current_rvas_for_posthoc_coverage_only"],
                     "paired_identity_available": prior["paired_identity_available"],
                     "fields": fields, "exact_overlaps": matched,
                     "candidate_families": sorted(set(candidates)),
                     "admitted_transferable_families": []})
    validation = {}
    for saved in graph["modules"]:
        path = Path(saved["inputs"]["retail_metadata"]["path"])
        if digest(path) != saved["inputs"]["retail_metadata"]["sha256"]:
            raise ValueError("retail metadata file changed")
        validation[saved["retail_module_sha256"]] = metadata_validation(resources[saved["retail_module_sha256"]], path)
    # Exposed old Trim truths are diagnostic only. Selection is the old body
    # Top-5 wrong set, not a newly chosen validation sample.
    truths = {x["retail_va"]: x for x in review["rows"] if x["resolved"]}
    current_trim = next(m for m in trim["modules"] if m["side"] == "current")
    retail_trim = next(m for m in trim["modules"] if m["side"] == "retail")
    owner_labels = defaultdict(set)
    for bridge in domain["current_function_bridge"]:
        if bridge["identity_status"] == "MAP_COFF_SOURCE_BYTES_VERIFIED":
            owner_labels[int(bridge["va"], 16) - current_trim["image_base"]].add(bridge["owner"])
    menu_flags = {int(r["rva"], 16): set(r["flags"]) for r in menu["current"]["rows"]}
    negative = []
    for function in ranked["functions"]:
        if function["retail_va"] not in truths:
            continue
        truth = truths[function["retail_va"]]
        true_rvas = {int(x, 16) for x in truth["expected_true_rvas"]}
        # Include the entire conservative body Top-10 equivalence set, rather
        # than breaking near-ties by address or an arbitrary five-row cap.
        wrong = [x for x in function["candidates"] if x["rva"] not in true_rvas
                 and x["rank_intervals"]["body"][0] <= 10]
        source = resource_rows[current_trim["module_sha256"]]
        retail_row = resource_rows[retail_trim["module_sha256"]][int(function["retail_va"], 16)
                                                                  - retail_trim["image_base"]]
        retail_tokens = {name: tokens(retail_row, name) for name in ("IMPORTS", "STRINGS", "RESOURCES")}
        def profile(rva: int) -> dict:
            row = source.get(rva)
            if row is None:
                return {"indexed": False}
            return {"indexed": True, "owner": sorted(owner_labels[rva]),
                    "menu_flags": sorted(menu_flags.get(rva, set())),
                    "exact_retail_overlap": {name: len(retail_tokens[name] & tokens(row, name))
                                             for name in retail_tokens},
                    "imports": sorted(tokens(row, "IMPORTS")),
                    "string_hashes": sorted(tokens(row, "STRINGS")),
                    "resource_ids": sorted(f"{k}:{v}" for k, v in tokens(row, "RESOURCES")),
                    "data_reference_count": len(row["data_references"])}
        true_profiles = {hex(x): profile(x) for x in sorted(true_rvas)}
        wrong_profiles = {hex(x["rva"]): profile(x["rva"]) for x in wrong}
        better = {}
        for name in retail_tokens:
            true_best = max((p.get("exact_retail_overlap", {}).get(name, 0)
                             for p in true_profiles.values()), default=0)
            wrong_best = max((p.get("exact_retail_overlap", {}).get(name, 0)
                              for p in wrong_profiles.values()), default=0)
            better[name] = {"true_best": true_best, "wrong_best": wrong_best,
                            "true_exceeds_every_wrong": true_best > wrong_best}
        negative.append({"retail_va": function["retail_va"], "true_profiles": true_profiles,
                         "top_wrong_body_profiles": wrong_profiles,
                         "wrong_equivalence_set_size": len(wrong),
                         "exact_overlap_comparison": better,
                         "true_with_string_and_all_wrong_without": any(p.get("string_hashes") for p in true_profiles.values())
                         and all(not p.get("string_hashes") for p in wrong_profiles.values()),
                         "true_with_import_and_all_wrong_without": any(p.get("imports") for p in true_profiles.values())
                         and all(not p.get("imports") for p in wrong_profiles.values()),
                         "true_with_resource_and_all_wrong_without": any(p.get("resource_ids") for p in true_profiles.values())
                         and all(not p.get("resource_ids") for p in wrong_profiles.values())})
    paths = {name: getattr(args, name) for name in
             ("matrix", "exe", "trim", "graph", "ranking", "review", "domain", "menu")}
    return {"schema": SCHEMA, "version": VERSION, "tool_sha256": digest(Path(__file__)),
            "python_version": sys.version.split()[0],
            "commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
            "status": "COMPLETE_POSTHOC_COVERAGE_NOT_MATCHER_INPUT",
            "configuration": {"rare_module_function_threshold": 10,
                              "wrong_body_candidate_rule": "conservative body rank interval lower bound <= 10",
                              "admitted_feature_count": 0},
            "inputs": {name: {"path": str(path.resolve()), "sha256": digest(path)}
                       for name, path in paths.items()},
            "limitations": ["Exact token overlap and candidate families do not confer V4 admission.",
                            "Resource IDs are API-call argument candidates; resource payload ownership is unproven.",
                            "Top wrong candidates are selected from exposed development rankings only.",
                            "Current general-function PE data addresses are not named global identities."],
            "summary": summarize(rows), "metadata_string_validation": validation,
            "wrong_body_candidates": negative, "functions": rows}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("matrix", "exe", "trim", "graph", "ranking", "review", "domain", "menu", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() or PRIVATE.resolve() not in output.parents:
        parser.error("output must be a new private evidence file")
    payload = run(args)
    with output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(payload, stream, sort_keys=True, separators=(",", ":"))
        stream.write("\n")
    print(json.dumps({"output": str(output), "sha256": digest(output),
                      "summary": payload["summary"],
                      "string_validation": payload["metadata_string_validation"]}, sort_keys=True))


if __name__ == "__main__":
    main()
