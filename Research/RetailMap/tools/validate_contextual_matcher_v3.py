#!/usr/bin/env python3
"""Control-only, fail-closed contextual re-ranking of frozen machine v2 evidence.

This is an identity research probe, not a bulk matcher or parity assertion. All
context is extracted without consulting the expected source ID of a control.
The nine positive controls and fifteen hard negatives remain immutable inputs.
"""
from __future__ import annotations

import argparse
from bisect import bisect_left
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import sqlite3

import pefile

import validate_machine_matcher_v2 as body
import v3_callgraph_context as graph
from v3_trim_factory_context import extract_string_hashes


ABLATIONS = ("body", "body_callgraph", "body_vtable", "body_source_class", "full")
RETAIL_EXE_BASE = 0x400000
TRIM_BASE = 0x10000000
CONTEXT_WEIGHTS = {"vtable_slot": 0.65, "source_class": 0.08,
                   "factory_subtype": 0.30, "rare_string": 0.42}
SCORE_TIE_EPSILON = 0.005


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def va_to_rva(module: str, va: int) -> int:
    return va - (TRIM_BASE if module == "Trim.dll" else RETAIL_EXE_BASE)


def indexed_instructions(path: Path) -> tuple[list[int], dict]:
    instructions = graph.disassemble(path)
    return sorted(instructions), instructions


def bounded_instructions(index: tuple[list[int], dict], va: int, extent: int):
    addresses, instructions = index
    lo, hi = bisect_left(addresses, va), bisect_left(addresses, va + extent)
    return ((address, *instructions[address]) for address in addresses[lo:hi])


def factory_subtypes(index: tuple[list[int], dict], base: int,
                     candidates: dict[int, dict]) -> dict[int, set[int]]:
    result = {}
    pattern = re.compile(r"\$(0x[\da-f]+|\d+),\s*(?:0x0*8|8)\(%\w+\)")
    for rva, item in candidates.items():
        values = set()
        for _, op, args in bounded_instructions(index, base + rva,
                                                 item["data"]["byte_extent"]):
            match = pattern.match(args.split("#", 1)[0].strip()) if op.startswith("cmp") else None
            if match:
                values.add(int(match[1], 0))
        if values:
            result[rva] = values
    return result


def candidate_strings(index: tuple[list[int], dict], image: pefile.PE,
                      candidates: dict[int, dict]) -> dict[int, Counter]:
    result = {}
    base = image.OPTIONAL_HEADER.ImageBase
    for rva, item in candidates.items():
        va, extent = base + rva, item["data"]["byte_extent"]
        values = extract_string_hashes(image, va, extent,
                                       bounded_instructions(index, va, extent))
        if values:
            result[rva] = values
    return result


def string_score(retail: set[str], current: Counter, frequency: Counter,
                 population: int) -> tuple[float, int]:
    common = retail & current.keys()
    if not common:
        return 0.0, 0
    # A rare exact literal has stronger attribution than a common UI word.
    # Coverage, not Jaccard, tolerates current functions containing extra text.
    import math
    weight = lambda key: 1.0 + math.log((population + 1) / (frequency[key] + 1))
    return sum(weight(key) for key in common) / sum(weight(key) for key in retail), len(common)


def table_evidence(report: dict) -> tuple[dict[int, int], set[int], set[int], str]:
    relation = report["class_table_relationship"]
    alignment = report["slot_alignment"]
    if relation["status"] != "VERIFIED_BOUNDED_CLASS_INTERFACE_RELATIONSHIP":
        return {}, set(), set(), "TABLE_RELATIONSHIP_UNVERIFIED"
    retail_slots = {row["zero_based_slot"]: row["target_va"] for row in
                    report["retail"]["admiral_table"]["getter_slots"]}
    current_table = next((row for row in report["current"]["tables"]
                          if row["va"] == relation["current_table_va"]), None)
    if current_table is None:
        return {}, set(), set(), "CURRENT_TABLE_UNRESOLVED"
    current_slots = {row["zero_based_slot"]: row["target_va"] for row in
                     current_table["getter_slots"]}
    family = {va - RETAIL_EXE_BASE for va in current_slots.values()}
    retail_family = {va - RETAIL_EXE_BASE for va in retail_slots.values()}
    if (alignment["status"] != "VERIFIED" or
        alignment["selection_basis"].startswith("Toolbar") or
        len(alignment["anchors"]) < 2):
        return {}, family, retail_family, "BODY_EQUIVALENT_UNRESOLVED"
    # Bracketing non-getter slots does not by itself rule out an internal
    # permutation. Each getter also needs an independently established retail
    # field-read -> parser-name -> current named source-field relationship.
    semantic = report.get("per_getter_mapping", {})
    if semantic.get("status") != "VERIFIED_FIELD_IDENTITY" or len(semantic.get("mappings", [])) != 4:
        return {}, family, retail_family, "BODY_EQUIVALENT_UNRESOLVED"
    delta = alignment["current_minus_retail_slot"]
    mapping = {}
    for slot, retail_va in retail_slots.items():
        if not alignment["retail_interval_inclusive"][0] <= slot <= alignment["retail_interval_inclusive"][1]:
            continue
        current_va = current_slots.get(slot + delta)
        if current_va is not None:
            mapping[retail_va - RETAIL_EXE_BASE] = current_va - RETAIL_EXE_BASE
    stated = {(int(row["retail"]["retail_va"]), int(row["current"]["current_va"]))
              for row in semantic["mappings"]}
    inferred = {(retail_rva + RETAIL_EXE_BASE, current_rva + RETAIL_EXE_BASE)
                for retail_rva, current_rva in mapping.items()}
    if stated != inferred:
        raise ValueError("Independent getter field semantics contradict slot alignment")
    return mapping, family, retail_family, "BODY_EQUIVALENT_CONTEXT_DISTINGUISHED"


def source_class_map(connection: sqlite3.Connection, module_hash: str) -> dict[int, set[str]]:
    result: dict[int, set[str]] = {}
    for rva, name, relation, status in connection.execute(
        "SELECT l.rva,l.qualified_name,l.relationship,f.source_identity_status "
        "FROM compiled_source_links l JOIN compiled_functions f "
        "ON f.module_sha256=l.module_sha256 AND f.rva=l.rva "
        "WHERE l.module_sha256=?", (module_hash,)):
        if relation != "ONE_TO_ONE" or status != "PDB_SOURCE_BYTES_VERIFIED" or not name:
            continue
        if "::" in name:
            result.setdefault(rva, set()).add(name.split("::", 1)[0])
    return result


def pair_context(module: str, retail_rva: int, current_rva: int, inputs: dict) -> dict:
    result = {"vtable_slot": 0.0, "source_class": 0.0,
              "factory_subtype": 0.0, "rare_string": 0.0,
              "contextual_evidence_count": 0, "evidence": []}
    if module == "Conquest_retail.exe":
        if retail_rva in inputs["retail_getters"] and current_rva in inputs["getter_family"]:
            if not inputs["getter_map"] or inputs["getter_map"].get(retail_rva) == current_rva:
                result["vtable_slot"] = 1.0
                result["evidence"].append("bounded IAdmiral getter family" if not inputs["getter_map"] else
                                          "independent getter field semantics and bracketed vtable slot")
        if retail_rva in inputs["retail_getters"] or retail_rva == 0x83BF0:
            expected_class = "Flagship" if retail_rva in inputs["retail_getters"] else "FlagshipFactory"
            if expected_class in inputs["source_classes"].get(current_rva, set()):
                result["source_class"] = 1.0
                result["evidence"].append("verified PDB/source class family; supporting only")
        if retail_rva == 0x83BF0 and inputs["retail_subtype"] in inputs["subtypes"].get(current_rva, set()):
            result["factory_subtype"] = 1.0
            result["evidence"].append("matching raw-record +8 subtype compare; no ownership proof")
    elif module == "Trim.dll" and retail_rva == 0x61240:
        value, hits = string_score(inputs["retail_strings"],
                                   inputs["trim_strings"].get(current_rva, Counter()),
                                   inputs["string_frequency"], inputs["trim_population"])
        result["rare_string"] = value
        if hits:
            result["evidence"].append(f"{hits} exact ASCII literal hashes shared; source text not exported")
    result["contextual_evidence_count"] = len(result["evidence"])
    return result


def augmented(body_score: float, context: dict, ablation: str) -> float:
    features = {"body": (), "body_callgraph": (), "body_vtable": ("vtable_slot",),
                "body_source_class": ("source_class",),
                "full": ("vtable_slot", "source_class", "factory_subtype", "rare_string")}[ablation]
    # The independent calibration-only graph has no anchored discriminator for
    # any held-out control; missing edges are not scored as negative evidence.
    return body_score + sum(CONTEXT_WEIGHTS[key] * context[key] for key in features)


def rank_rows(rows: list[tuple[int, float, dict]], module: str, retail_rva: int,
              inputs: dict, ablation: str) -> list[dict]:
    scored = []
    for rva, body_score, body_detail in rows:
        context = pair_context(module, retail_rva, rva, inputs)
        scored.append({"rva": rva, "body_score": body_score,
                       "score": augmented(body_score, context, ablation),
                       "context": context, "body_detail": body_detail})
    scored.sort(key=lambda row: -row["score"])
    # Do not use address/source order to break equal or near-equal scores.
    start = 0
    while start < len(scored):
        stop = start + 1
        while stop < len(scored) and abs(scored[stop]["score"] - scored[start]["score"]) < SCORE_TIE_EPSILON:
            stop += 1
        equivalence_members = tuple(item["rva"] for item in scored[start:stop])
        for row in scored[start:stop]:
            row["rank"] = start + 1
            row["rank_max"] = stop
            row["equivalence_size"] = stop - start
            row["equivalence_members"] = equivalence_members
        start = stop
    return scored


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("frozen-db", "controls", "vtable", "callgraph", "trim-factory",
                 "final-image", "trim-image", "output-db", "output-json"):
        parser.add_argument("--" + name, required=True, type=Path)
    args = parser.parse_args()
    for path in (args.output_db, args.output_json):
        if path.exists():
            raise ValueError(f"Refusing to overwrite evidence: {path}")
        if (Path(__file__).resolve().parents[3] / "Evidence/RetailMap/local").resolve() not in path.resolve().parents:
            raise ValueError("V3 outputs must remain in private local evidence")
    controls, tables, calls, probes = (load_json(path) for path in
                                      (args.controls, args.vtable, args.callgraph, args.trim_factory))
    if len(controls["controls"]) != 9 or len(controls["hard_negatives"]) != 15:
        raise ValueError("Fixed validation suite changed")
    if digest(args.frozen_db) != "9F71BA4B6FA050245D336EB64D25780E1D0D4CE37953D14FC592E0C3EA0BA82E":
        raise ValueError("Frozen v2 evidence identity changed")
    if digest(args.final_image) != body.CURRENT_EXE or digest(args.trim_image) != body.CURRENT_TRIM:
        raise ValueError("Current machine image identity changed")
    if (calls["inputs"]["retail_image"]["sha256"] != body.RETAIL_EXE or
        calls["inputs"]["current_image"]["sha256"] != body.CURRENT_EXE or
        calls["inputs"]["controls"]["sha256"] != digest(args.controls)):
        raise ValueError("Callgraph capture does not match frozen retail/current/controls")
    if (tables["inputs"]["Code\\App\\Src\\Final\\Conquest.exe"] != body.CURRENT_EXE or
        tables["inputs"]["D:\\Dev\\ghidra-proj\\Conquest_retail.exe"] != body.RETAIL_EXE):
        raise ValueError("Vtable capture module identity mismatch")
    getter_map, getter_family, retail_getters, table_status = table_evidence(tables)
    probe_by_label = {item["label"]: item for item in probes["probes"]}
    retail_factory = probe_by_label["retail_flagship_factory"]
    subtype_values = [entry["immediate"] for entry in retail_factory["immediate_memory_comparisons"]
                      if entry["displacement"] == 8 and not entry["stack_base"]]
    if len(subtype_values) != 1:
        raise ValueError("Retail factory subtype is not unique")
    if calls["anchors"] and any(anchor["source_id"] != controls["controls"][0]["source_id"]
                                for anchor in calls["anchors"]):
        raise ValueError("Held-out control leaked into graph anchors")
    input_db = sqlite3.connect(f"file:{args.frozen_db.as_posix()}?mode=ro", uri=True)
    output_db = sqlite3.connect(args.output_db)
    input_db.backup(output_db)
    output_db.executescript((Path(__file__).resolve().parents[1] / "schema.sql").read_text(encoding="utf-8"))
    current = {module: body.load_features(input_db, module) for module in (body.CURRENT_EXE, body.CURRENT_TRIM)}
    retail = {module: body.load_features(input_db, module) for module in (body.RETAIL_EXE, body.RETAIL_TRIM)}
    idf = {module: body.weights(values) for module, values in current.items()}
    final_index = indexed_instructions(args.final_image)
    trim_index = indexed_instructions(args.trim_image)
    trim_pe = pefile.PE(str(args.trim_image))
    trim_strings = candidate_strings(trim_index, trim_pe, current[body.CURRENT_TRIM])
    trim_pe.close()
    frequency = Counter(key for values in trim_strings.values() for key in values)
    inputs = {"getter_map": getter_map, "getter_family": getter_family,
              "retail_getters": retail_getters,
              "source_classes": source_class_map(input_db, body.CURRENT_EXE),
              "retail_subtype": subtype_values[0],
              "subtypes": factory_subtypes(final_index, RETAIL_EXE_BASE, current[body.CURRENT_EXE]),
              "retail_strings": set(probe_by_label["retail_toolbar"]["ascii_immediate_string_sha256"]),
              "trim_strings": trim_strings, "string_frequency": frequency,
              "trim_population": len(current[body.CURRENT_TRIM])}
    schema_path = Path(__file__).resolve().parents[1] / "schema.sql"
    dependency_hashes = {"validator": digest(Path(__file__)), "schema": digest(schema_path),
                         "v2_scorer": digest(Path(body.__file__)),
                         "graph_helper": digest(Path(graph.__file__)),
                         "string_helper": digest(Path(extract_string_hashes.__code__.co_filename))}
    run_id = "v3-control-" + hashlib.sha256(json.dumps({
        "dependencies": dependency_hashes, "controls": digest(args.controls),
        "vtable": digest(args.vtable), "callgraph": digest(args.callgraph),
        "trim_factory": digest(args.trim_factory), "v2_db": digest(args.frozen_db)
    }, sort_keys=True).encode()).hexdigest()[:20]
    v2_run = input_db.execute("SELECT run_id FROM matcher_v2_runs ORDER BY run_id DESC LIMIT 1").fetchone()[0]
    report = {"run_id": run_id, "v2_run_id": v2_run,
              "status": "MATCHER_V3_CONTROL_ONLY_DEVELOPMENT", "table_status": table_status,
              "inputs": {name: digest(getattr(args, name)) for name in
                         ("frozen_db", "controls", "vtable", "callgraph", "trim_factory", "final_image", "trim_image")},
              "weights": CONTEXT_WEIGHTS, "dependencies": dependency_hashes,
              "ablations": {}, "positives": [],
              "hard_negatives": [], "contextual_hard_negatives": [],
              "promoted_correspondences": 0}
    modules = {"Conquest_retail.exe": (body.RETAIL_EXE, body.CURRENT_EXE),
               "Trim.dll": (body.RETAIL_TRIM, body.CURRENT_TRIM)}
    body_cache = {}
    ranked_cache = {}

    def candidates_for(module: str, retail_rva: int, ablation: str):
        key = (module, retail_rva)
        if key not in body_cache:
            rh, ch = modules[module]
            body_cache[key] = body.rank_control(retail[rh][retail_rva], current[ch], idf[ch], None)[0]
        ranked_key = (*key, ablation)
        if ranked_key not in ranked_cache:
            ranked_cache[ranked_key] = rank_rows(body_cache[key], module, retail_rva, inputs, ablation)
        return body_cache[key], ranked_cache[ranked_key]

    with output_db:
        for item in controls["reviewed_samples"]:
            module = item["module"]
            rh, ch = modules[module]
            rva = va_to_rva(module, int(item["va"], 16))
            current_rva = body.expected_rva(input_db, item["source_id"], ch)
            if current_rva is None:
                raise ValueError("Reviewed anchor missing exact current machine identity")
            role = next((control["role"] for control in controls["controls"] if
                         control["module"] == module and control["va"] == item["va"]), "holdout")
            output_db.execute("INSERT INTO correspondence_anchors VALUES (?,?,?,?,?,?,?,?,?,?,?,?)",
                (f"{rh}:{rva:X}", rh, rva, ch, current_rva, item["source_id"], item["evidence_ref"],
                 item["confidence"], "VERIFIED" if item["confidence"] >= 98 else "REVIEWED_HIGH_CONFIDENCE",
                 int(role == "calibration"), "CALIBRATION" if role == "calibration" else "HOLDOUT",
                 "independent review manifest; never inferred from v2 rank"))
        # These are bounded observed edges, not a claim of complete callgraph
        # extraction or an inferred correspondence. The separate full graph
        # research capture remains the durable machine-level source.
        graph_hash = digest(args.callgraph)
        for entry in calls["controls"]:
            for side, module_hash, base in (("retail_context", body.RETAIL_EXE, RETAIL_EXE_BASE),):
                contexts = [entry[side]]
                for item_context in entry["current_candidates"]:
                    contexts.append(item_context["context"])
                for context in contexts:
                    side_hash = module_hash if context is entry[side] else body.CURRENT_EXE
                    side_base = base
                    for edge in context["outgoing"] + context["incoming"]:
                        target = edge.get("target") if edge.get("target_is_function_entry") else None
                        output_db.execute("INSERT OR IGNORE INTO machine_call_edges_v3 VALUES (?,?,?,?,?,?,?)",
                            (side_hash, edge["owner"]-side_base, edge["site"]-side_base,
                             edge["kind"], None if target is None else target-side_base,
                             edge.get("import_name") or edge.get("import_via_one_hop_thunk"),
                             "bounded callgraph capture SHA256 " + graph_hash))
        relation = tables["class_table_relationship"]
        for side, module_hash, base, table_va, entries, owner in (
            ("retail", body.RETAIL_EXE, RETAIL_EXE_BASE,
             tables["retail"]["admiral_table"]["table_va"],
             tables["retail"]["admiral_table"]["entries"],
             tables["retail"]["admiral_table"]["owner_evidence"]),
            ("current", body.CURRENT_EXE, RETAIL_EXE_BASE, relation["current_table_va"],
             next(row["entries"] for row in tables["current"]["tables"]
                  if row["va"] == relation["current_table_va"]),
             "PDB named table installed by source-linked FlagshipFactory::CreateInstance")):
            for slot, entry in enumerate(entries):
                target_va = entry["target_va"]
                output_db.execute("INSERT INTO machine_vtable_slots_v3 VALUES (?,?,?,?,?,?)",
                    (module_hash, table_va-base, slot, target_va-base,
                     owner, f"bounded {side} pointer table; capture SHA256 {digest(args.vtable)}"))
        output_db.execute("INSERT INTO vtable_relationships_v3 VALUES (?,?,?,?,?,?,?,?)",
            (body.RETAIL_EXE, relation["retail_table_va"]-RETAIL_EXE_BASE,
             body.CURRENT_EXE, relation["current_table_va"]-RETAIL_EXE_BASE,
             relation["status"], table_status,
             "bounded vtable capture SHA256 " + digest(args.vtable), 98))
        output_db.execute("INSERT INTO matcher_v3_runs VALUES (?,?,?,?,?,?,?,?,?)",
            (run_id, v2_run, digest(Path(__file__)), digest(args.controls),
             json.dumps({"inputs": report["inputs"], "dependencies": dependency_hashes}, sort_keys=True),
             json.dumps(CONTEXT_WEIGHTS, sort_keys=True),
             "UNVALIDATED", 0, "same nine positives/fifteen hard negatives; no bulk promotion"))
        for item in controls["controls"]:
            module, va = item["module"], int(item["va"], 16)
            rh, ch = modules[module]
            rva = va_to_rva(module, va)
            expected = body.expected_rva(input_db, item["source_id"], ch)
            base_rows, _ = candidates_for(module, rva, "body")
            old_rank = next((index for index, row in enumerate(base_rows, 1)
                             if row[0] == expected), None)
            outcomes = {}
            for ablation in ABLATIONS:
                _, rows = candidates_for(module, rva, ablation)
                true = next((row for row in rows if row["rva"] == expected), None)
                if true is None:
                    raise ValueError("Control counterpart excluded by original v2 candidate generation")
                top_score = rows[0]["score"]
                equivalence = list(true["equivalence_members"])
                second_score = next((row["score"] for row in rows if row["score"] < top_score - SCORE_TIE_EPSILON), None)
                outcome = ("BODY_EQUIVALENT_CONTEXT_DISTINGUISHED" if
                           rva in retail_getters and ablation in ("body_vtable", "full") and len(equivalence) == 1 else
                           "BODY_EQUIVALENT_UNRESOLVED" if rva in retail_getters and len(equivalence) > 1 else
                           "AMBIGUOUS" if len(equivalence) > 1 else
                           "UNIQUE_RANKED_LEAD" if true["rank"] > 1 else "UNIQUE_TOP_LEAD")
                control_id = f"{module}:{item['va']}"
                output_db.execute("INSERT INTO matcher_v3_control_results VALUES (?,?,?,?,?,?,?,?,?,?,?)",
                    (run_id, control_id, ablation, true["rank"], json.dumps([f"0x{x:X}" for x in equivalence]),
                     top_score, second_score, None if second_score is None else top_score-second_score,
                     true["context"]["contextual_evidence_count"], outcome,
                     json.dumps(true["context"], sort_keys=True)))
                for row in rows[:10] + ([true] if true["rank"] > 10 else []):
                    output_db.execute("INSERT OR IGNORE INTO matcher_v3_candidate_leads VALUES (?,?,?,?,?,?,?,?,?,?)",
                        (run_id, control_id, ablation, ch, row["rva"], row["rank"], row["score"],
                         row["body_score"], json.dumps(row["context"], sort_keys=True),
                         "AMBIGUOUS" if row["equivalence_size"] > 1 else "UNREVIEWED"))
                outcomes[ablation] = {"rank": true["rank"], "rank_max": true["rank_max"],
                                      "equivalence_size": len(equivalence),
                                      "outcome": outcome, "score": round(true["score"], 5)}
            report["positives"].append({"control": f"{module}:{item['va']}", "old_v2_rank": old_rank,
                                        "outcomes": outcomes})
        # The original 15 negatives are evaluated on the same retail control
        # and candidate universe. No identity is promoted from this comparison.
        positive_by_va = {row["control"].split(":", 1)[1].lower(): row for row in report["positives"]}
        for item in controls["hard_negatives"]:
            module = item["module"]
            rh, ch = modules[module]
            rva = va_to_rva(module, int(item["va"], 16))
            wrong = body.expected_rva(input_db, item["wrong_source_id"], ch)
            _, rows = candidates_for(module, rva, "full")
            wrong_row = next((row for row in rows if row["rva"] == wrong), None)
            positive = positive_by_va.get(item["va"].lower())
            true_score = positive["outcomes"]["full"]["score"] if positive else None
            margin = None if true_score is None or wrong_row is None else true_score - wrong_row["score"]
            negative_outcome = (
                "WRONG_NOT_TOP_TRUE_UNSPECIFIED" if true_score is None and wrong_row is not None
                                             and wrong_row["rank"] > 1 else
                "WRONG_TOP_TRUE_UNSPECIFIED" if true_score is None and wrong_row is not None else
                "UNRESOLVED" if margin is None or abs(margin) < SCORE_TIE_EPSILON else
                "TRUE_ABOVE_WRONG" if margin > 0 else "WRONG_ABOVE_TRUE")
            report["hard_negatives"].append({"retail_va": item["va"],
                "wrong_source_id": item["wrong_source_id"],
                "wrong_rank": None if wrong_row is None else wrong_row["rank"],
                "true_minus_wrong": None if margin is None else round(margin, 5),
                "outcome": negative_outcome})
        # These three contextual negatives are additional probes, not edits to
        # the fixed 15-negative manifest. Each false candidate was identified
        # independently from its machine role/slot or from the v2 false top.
        for label, retail_module, retail_rva, false_rva, true_rva in (
            ("same-allocator-wrong-subtype-SupplyShip", "Conquest_retail.exe", 0x83BF0,
             int(probe_by_label["current_supply_false_top"]["rva"], 16), 0xA4DD0),
            ("neighboring-IAdmiral-tactic-slot", "Conquest_retail.exe", 0x80B90,
             tables["slot_alignment"]["anchors"][-1]["current_va"]-RETAIL_EXE_BASE,
             getter_map.get(0x80B90)),
            ("same-module-similar-CFG-setStateInfo", "Trim.dll", 0x61240,
             int(probe_by_label["current_state_false_top"]["rva"], 16), 0x8DF10),
        ):
            if true_rva is None:
                report["contextual_hard_negatives"].append({"label": label, "outcome": "UNRESOLVED"})
                continue
            rh, ch = modules[retail_module]
            _, ranked = candidates_for(retail_module, retail_rva, "full")
            false = next((row for row in ranked if row["rva"] == false_rva), None)
            true = next((row for row in ranked if row["rva"] == true_rva), None)
            if false is None or true is None:
                outcome, margin = "OUTSIDE_ORIGINAL_CANDIDATE_UNIVERSE", None
            else:
                margin = true["score"] - false["score"]
                outcome = ("TRUE_ABOVE_WRONG" if margin >= SCORE_TIE_EPSILON else
                           "TIED_OR_AMBIGUOUS" if abs(margin) < SCORE_TIE_EPSILON else
                           "WRONG_ABOVE_TRUE")
            report["contextual_hard_negatives"].append({"label": label,
                "retail_rva": f"0x{retail_rva:X}", "false_rva": f"0x{false_rva:X}",
                "true_rva": f"0x{true_rva:X}", "true_minus_wrong": margin,
                "outcome": outcome})
    for ablation in ABLATIONS:
        outcomes = [row["outcomes"][ablation] for row in report["positives"]]
        report["ablations"][ablation] = {f"top_{n}": sum(row["rank_max"] <= n for row in outcomes)
                                            for n in (1,3,5,10)}
        report["ablations"][ablation]["mrr_conservative"] = round(
            sum(1/row["rank_max"] for row in outcomes)/len(outcomes), 5)
    getter_negatives = [row for row in report["hard_negatives"]
                        if row["retail_va"].lower() in
                        {"0x00480b30", "0x00480b50", "0x00480b70", "0x00480b90"}]
    nongetter_negatives = [row for row in report["hard_negatives"]
                           if row not in getter_negatives]
    contextual_by_label = {row["label"]: row for row in report["contextual_hard_negatives"]}
    trim_control = next(row for row in report["positives"] if row["control"].startswith("Trim.dll:"))
    report["gate"] = {
        "all_nine_comparable": all(row["outcomes"]["body"]["rank"] is not None
                               for row in report["positives"]),
        "top5_at_least_7": report["ablations"]["full"]["top_5"] >= 7,
        "top10_at_least_8": report["ablations"]["full"]["top_10"] >= 8,
        "getter_context_distinguished_or_bounded_set": table_status in
            ("BODY_EQUIVALENT_CONTEXT_DISTINGUISHED", "BODY_EQUIVALENT_UNRESOLVED"),
        "getter_hard_negatives_separated": len(getter_negatives) == 12 and all(
            row["outcome"] == "TRUE_ABOVE_WRONG" for row in getter_negatives),
        "other_original_hard_negatives_not_top": len(nongetter_negatives) == 3 and all(
            row["outcome"] == "WRONG_NOT_TOP_TRUE_UNSPECIFIED" for row in nongetter_negatives),
        "flagship_above_supply": contextual_by_label["same-allocator-wrong-subtype-SupplyShip"]["outcome"]
            == "TRUE_ABOVE_WRONG",
        "trim_improves_from_34": trim_control["outcomes"]["full"]["rank_max"]
            < trim_control["old_v2_rank"],
        "trim_false_top_separated": contextual_by_label["same-module-similar-CFG-setStateInfo"]["outcome"]
            == "TRUE_ABOVE_WRONG",
        "no_bulk_promotion": report["promoted_correspondences"] == 0,
        "getter_slot_status": table_status,
        "trim_old_rank": trim_control["old_v2_rank"],
        "trim_new_rank_interval": [trim_control["outcomes"]["full"]["rank"],
                                   trim_control["outcomes"]["full"]["rank_max"]],
    }
    required = ("all_nine_comparable", "top5_at_least_7", "top10_at_least_8",
                "getter_context_distinguished_or_bounded_set", "getter_hard_negatives_separated",
                "other_original_hard_negatives_not_top",
                "flagship_above_supply", "trim_improves_from_34", "trim_false_top_separated",
                "no_bulk_promotion")
    report["gate"]["passed"] = all(report["gate"][key] for key in required)
    report["status"] = ("MATCHER_V3_BOUNDED_DEVELOPMENT_CONTROLS_PASSED" if
                        report["gate"]["passed"] else "MATCHER_V3_DEVELOPMENT_CONTROLS_FAILED")
    report["limits"] = [
        "No general retail correspondence has been auto-promoted",
        "Only one independent calibration anchor is usable for graph propagation; callgraph ablation has no holdout discriminator",
        "Getter field evidence is a manually bounded semantic review, not a validated generic vtable matcher",
        "Factory +8 comparison is supporting evidence outside the individually reviewed raw-pointer lineage",
        "Trim MAP/COFF/source checksum is not a fresh Trim PDB and does not establish all UI source mappings",
        "Top-K counts use worst rank in each near-tie equivalence group; v2 historical ranks used ordinal sorting",
        "Control success is not evidence of gameplay parity, broad-map generalization, or authorization for mass mapping",
    ]
    output_db.execute("UPDATE matcher_v3_runs SET validation_state=? WHERE run_id=?",
                      (report["status"], run_id))
    output_db.commit()
    args.output_json.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    output_db.close()
    input_db.close()
    print(json.dumps({"run_id": run_id, "output_db_sha256": digest(args.output_db),
                      "output_json_sha256": digest(args.output_json), "ablations": report["ablations"],
                      "gate": report["gate"]}, sort_keys=True))


if __name__ == "__main__":
    main()
