#!/usr/bin/env python3
"""Transferable bounded graph metadata, with no matching or identity promotion.

Exact machine operands are interpreted using saved Ghidra function ranges and
current PDB or MAP/COFF extents. Only reviewed Tier0 identities label endpoints.
Paths have at most two direct-call hops; no predicted node ever becomes an
anchor. Imports and Tier0 paths are support-only observations: missing paths,
inlining, unknown indirect calls and unindexed targets are never contradictions.
Generated metadata must stay under ignored Evidence/RetailMap/local/.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path
import shutil

import v3_callgraph_context as machine

VERSION = "v4-graph-neighborhood-2-separated"
ROOT = Path(__file__).resolve().parents[3]
PRIVATE = ROOT / "Evidence/RetailMap/local"


def number(value):
    return int(value, 0) if isinstance(value, str) else int(value)


def map_coff_bounds(index):
    """Exact CodeView sizes, never next-MAP-symbol gap estimates.

    Folded/aliased addresses with conflicting sizes remain unavailable. Current
    source identity is separate from the availability of a machine extent.
    """
    grouped = defaultdict(list)
    for row in index["current_function_bridge"]:
        grouped[number(row["va"])].append(row)
    result = {}
    for va, rows in grouped.items():
        sizes = {r["size"] for r in rows if r["size"] > 0}
        if len(sizes) != 1:
            continue
        verified = all(r["identity_status"] == "MAP_COFF_SOURCE_BYTES_VERIFIED" for r in rows)
        result[va] = {"ranges": [(va, va + next(iter(sizes)))],
                      "provenance": "MAP_COFF_EXACT_EXTENT",
                      "identity_status": "MAP_COFF_SOURCE_BYTES_VERIFIED" if verified else "SOURCE_UNRESOLVED",
                      "names": sorted({r["display_name"] for r in rows if r.get("display_name")}),
                      "source_paths": sorted({s["source_path"].replace("\\", "/") for r in rows
                                              for s in r["primary_source_checksums"]
                                              if s["matches_current_bytes"]}),
                      "object_sha256": sorted({r["object_sha256"] for r in rows})}
    return result


def reviewed_anchors(review, retail_hash, rg, cg):
    """Reviewed identities are immutable inputs; pilot conclusions are not read.

    Reviewed source IDs require exactly one current source-proved function.
    MAP/COFF anchors additionally require exact qualified-name and source-path
    agreement. No symbol similarity, address proximity or propagation is used.
    """
    anchors, unavailable = [], []
    for row in review["reviewed_samples"]:
        if row["module_sha256"].upper() != retail_hash:
            continue
        sid = row["source_id"]
        source, _line, qualified = sid.split(":", 2)
        matches = []
        for va, bound in cg["bounds"].items():
            pdb = (bound.get("provenance") == "ONE_TO_ONE"
                   and bound.get("identity_status") == "PDB_SOURCE_BYTES_VERIFIED"
                   and bound.get("source_ids") == [sid])
            coff = (bound.get("identity_status") == "MAP_COFF_SOURCE_BYTES_VERIFIED"
                    and bound.get("names") == [qualified]
                    and any(p.lower().endswith("/" + source.lower()) for p in bound.get("source_paths", [])))
            if pdb or coff:
                matches.append(va)
        rva = int(row["va"], 16)
        if rva not in rg["bounds"] or len(matches) != 1:
            unavailable.append({"source_id": sid, "retail_boundary_available": rva in rg["bounds"],
                                "current_exact_identity_count": len(matches)})
            continue
        anchors.append({"tier": "Tier0", "source_id": sid, "retail_va": rva,
                        "current_va": matches[0], "evidence_ref": row["evidence_ref"],
                        "basis": "preexisting independently reviewed identity; never pilot or prediction"})
    return anchors, unavailable


def adjacency(graph):
    result = defaultdict(lambda: defaultdict(set))
    imports = defaultdict(set)
    for edge in graph["edges"]:
        if edge.get("ownership_ambiguous"):
            continue
        owner = edge["owner"]
        if edge["kind"] == "DIRECT_CALL" and edge.get("target_is_function_entry"):
            target = edge["target"]
            result[owner]["out"].add(target)
            result[target]["in"].add(owner)
        name = edge.get("import_name") or edge.get("import_via_one_hop_thunk")
        if name and edge["instruction_kind"] == "call":
            imports[owner].add(name)
    return result, imports


def neighborhood(graph, va, anchors, side, prepared=None):
    """Address-independent comparable tokens plus auditable bounded paths.

    Direction sequence is retained (out/in, then out/in). Cycles back to the
    root and repeated-node walks are excluded. Endpoint and path presence use
    sets, so duplicate call sites do not multiply identity support.
    """
    adjacent, imports = prepared or adjacency(graph)
    pool = {a[side + "_va"]: a["source_id"] for a in anchors if a[side + "_va"] != va}
    tokens, paths, visited = set(), [], {va}
    frontier = [(va, (), (va,))]
    for depth in range(3):
        following = []
        for node, directions, walk in frontier:
            route = "/".join(directions) or "self"
            if depth < 2:  # Import edge also consumes a hop.
                for name in imports[node]:
                    tokens.add(f"import:{route}:{name}")
            if node in pool:
                tokens.add(f"anchor:{route}:{pool[node]}")
                paths.append({"directions": list(directions), "nodes": list(walk),
                              "anchor_source_id": pool[node]})
            if depth == 2:
                continue
            for direction in ("in", "out"):
                for target in sorted(adjacent[node][direction]):
                    if target not in walk:
                        following.append((target, directions + (direction,), walk + (target,)))
                        visited.add(target)
        frontier = following
    outgoing = graph["outgoing"].get(va, [])
    incoming = graph["incoming"].get(va, [])
    coverage = {"function_boundary_available": va in graph["bounds"],
                "entry_instruction_available": graph["bounds"].get(va, {}).get("entry_instruction_available"),
                "saved_retail_call_edges": graph["bounds"].get(va, {}).get("saved_retail_call_edges"),
                "saved_retail_callers": graph["bounds"].get(va, {}).get("saved_retail_callers"),
                "saved_retail_unresolved_call_sites": graph["bounds"].get(va, {}).get("saved_retail_unresolved_call_sites"),
                "observed_outgoing_edges": len(outgoing), "observed_incoming_edges": len(incoming),
                "known_direct_callees": len(adjacent[va]["out"]),
                "known_direct_callers": len(adjacent[va]["in"]),
                "direct_targets_outside_boundary_index": sum(e["kind"] == "DIRECT_CALL" and not e.get("target_is_function_entry") for e in outgoing),
                "unresolved_indirect_call_sites": sum(e["kind"] == "UNRESOLVED_INDIRECT_CALL" for e in outgoing),
                "tail_candidates": sum(e["kind"] == "TAIL_JUMP_CANDIDATE" for e in outgoing),
                "ambiguous_owned_edges": sum(bool(e.get("ownership_ambiguous")) for e in outgoing),
                "two_hop_distinct_function_nodes": len(visited) - 1,
                "complete_callgraph": False,
                "absence": "UNKNOWN; no observed edge is not proof of no call"}
    return {"va": va, "boundary_provenance": graph["bounds"].get(va),
            "tokens": sorted(tokens), "anchor_paths": paths, "coverage": coverage}


def compare(left, right, ablation="full"):
    """Support channel activation only; deliberately no scalar/rank/confidence."""
    def select(tokens):
        return {t for t in tokens if ablation != "off"
                and (ablation != "anchor_off" or not t.startswith("anchor:"))
                and (ablation != "import_off" or not t.startswith("import:"))
                and (ablation != "one_hop_only" or t.split(":", 2)[1].count("/") < 1)}
    shared = sorted(select(left["tokens"]) & select(right["tokens"]))
    return {"shared_tokens": shared, "activated": bool(shared), "ablation": ablation,
            "negative_evidence": False, "promote": False, "score": None,
            "transfer_gate_credit": False,
            "decision": "CONTEXT_SUPPORT_ONLY" if shared else "INSUFFICIENT_CONTEXT"}


def run_module(config, selection, review, objdump):
    retail_path, current_path = Path(config["retail_image"]), Path(config["current_image"])
    metadata_path = Path(config["retail_metadata"])
    rr = machine.jsonl(metadata_path)
    rh = rr[0]
    retail_hash = machine.digest(retail_path)
    if retail_hash != rh["image_sha256"].upper() or not rr[-1].get("complete"):
        raise ValueError("retail image identity or complete exporter footer mismatch")
    current_hash = machine.digest(current_path)
    inputs = {"retail_image": retail_path, "current_image": current_path, "retail_metadata": metadata_path}
    if "compiled_bridge" in config:
        path = Path(config["compiled_bridge"])
        cr = machine.jsonl(path)
        if cr[0]["module_sha256"].upper() != current_hash:
            raise ValueError("current image/compiled bridge mismatch")
        cb = machine.compiled_bounds(cr)
        inputs["compiled_bridge"] = path
    else:
        path = Path(config["map_coff_index"])
        index = json.loads(path.read_text(encoding="utf-8"))
        if index["current_contract"]["module_sha256"].upper() != current_hash:
            raise ValueError("current image/MAP COFF index mismatch")
        cb = map_coff_bounds(index)
        inputs["map_coff_index"] = path
    rbase, ri = machine.pe_imports(retail_path)
    cbase, ci = machine.pe_imports(current_path)
    if rbase != int(rh["image_base"], 16):
        raise ValueError("retail image base mismatch")
    rb = machine.retail_bounds(rr)
    for row in rr:
        if row.get("record") == "function":
            rb[int(row["entry"], 16)].update(saved_retail_call_edges=len(row.get("call_edges", [])),
                                            saved_retail_callers=len(row.get("callers", [])),
                                            saved_retail_unresolved_call_sites=row.get("unresolved_call_sites", 0))
    rins, cins = machine.disassemble(retail_path, objdump), machine.disassemble(current_path, objdump)
    for bounds, instructions in ((rb, rins), (cb, cins)):
        for va, bound in bounds.items():
            bound["entry_instruction_available"] = va in instructions
    rg = machine.build_graph(rb, rins, ri)
    cg = machine.build_graph(cb, cins, ci)
    anchors, unavailable = reviewed_anchors(review, retail_hash, rg, cg)
    rp = adjacency(rg)
    rows = []
    for group, samples in selection.items():
        is_trim = "trim" in rh["image_name"].lower()
        if ("Trim" in group) != is_trim:
            continue
        for address, size in samples:
            va = int(address, 16)
            saved = next((r for r in rr if r.get("record") == "function" and int(r["entry"], 16) == va), None)
            if saved is None or saved["body_size"] != size:
                raise ValueError("selected address/size is not a saved retail boundary")
            rows.append({"subsystem": group, "retail_va": va,
                         "retail_context": neighborhood(rg, va, anchors, "retail", rp)})
    # A metadata-only machine graph is not a correspondence map. Retaining the
    # independent current graph permits any later candidate to be compared,
    # without using ground truth to choose the extraction neighborhood.
    def capture(graph):
        return {"bounds": graph["bounds"], "edges": graph["edges"], "coverage": graph["coverage"]}
    return {"retail_module_sha256": retail_hash, "current_module_sha256": current_hash,
            "inputs": {k: {"path": str(v), "sha256": machine.digest(v)} for k, v in inputs.items()},
            "retail_coverage": rg["coverage"], "current_coverage": cg["coverage"],
            "boundary_counts": {"retail": len(rg["bounds"]), "current": len(cg["bounds"])},
            "anchors": anchors, "unavailable_reviewed_anchors": unavailable, "functions": rows,
            "retail_image_base": rbase, "current_image_base": cbase,
            "retail_graph": capture(rg), "current_graph": capture(cg)}


def restore_graph(saved):
    bounds = {int(k): v for k, v in saved["bounds"].items()}
    outgoing, incoming = defaultdict(list), defaultdict(list)
    for edge in saved["edges"]:
        outgoing[edge["owner"]].append(edge)
        if edge.get("target_is_function_entry"):
            incoming[edge["target"]].append(edge)
    return {**saved, "bounds": bounds, "outgoing": outgoing, "incoming": incoming}


def evaluate_development(capture, inventory):
    """Posthoc truth access is confined to this evaluator, never capture."""
    truth = {(r["retail_module_sha256"], number(r["retail_va"])): r for r in inventory["functions"]}
    modules = []
    for module in capture["modules"]:
        cg = restore_graph(module["current_graph"])
        prepared = adjacency(cg)
        rows = []
        for captured in module["functions"]:
            request = truth[(module["retail_module_sha256"], captured["retail_va"])]
            left = captured["retail_context"]
            candidates = []
            for rva in request["channels"]["source_compiled_bridge"]["current_rvas"]:
                right = neighborhood(cg, module["current_image_base"] + number(rva), module["anchors"], "current", prepared)
                candidates.append({"current_rva": rva, "context": right,
                                   "comparisons": {a: compare(left, right, a) for a in
                                                   ("full", "off", "anchor_off", "import_off", "one_hop_only")}})
            rows.append({**captured, "current_candidates": candidates,
                         "current_identity_basis": request["channels"]["source_compiled_bridge"],
                         "development_only": True})
        modules.append({"retail_module_sha256": module["retail_module_sha256"], "functions": rows})
    return modules


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("config", "inventory", "anchors", "selection", "evaluate-capture"):
        parser.add_argument("--" + name, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--objdump", default="llvm-objdump")
    args = parser.parse_args()
    if args.output.exists() or PRIVATE.resolve() not in args.output.resolve().parents:
        raise ValueError("output must be new private Evidence/RetailMap/local evidence")
    if args.evaluate_capture:
        if not args.inventory or any((args.config, args.anchors, args.selection)):
            raise ValueError("evaluation requires capture + inventory only")
        capture = json.loads(args.evaluate_capture.read_text(encoding="utf-8"))
        if capture["status"] != "INDEPENDENT_METADATA_CAPTURE_NO_GROUND_TRUTH":
            raise ValueError("not an independent generic capture")
        inventory = json.loads(args.inventory.read_text(encoding="utf-8"))
        modules = evaluate_development(capture, inventory)
        input_paths = (("capture", args.evaluate_capture), ("posthoc_inventory", args.inventory))
        status = "POSTHOC_V4_DEVELOPMENT_CONTEXT_SUPPORT_ONLY"
    else:
        if args.inventory or not all((args.config, args.anchors, args.selection)):
            raise ValueError("independent capture requires config + anchors + address/size selection; forbids inventory")
        config = json.loads(args.config.read_text(encoding="utf-8"))
        anchors = json.loads(args.anchors.read_text(encoding="utf-8"))
        # Read only address/size sample; any analyst fields are inaccessible to extraction.
        selection = json.loads(args.selection.read_text(encoding="utf-8"))["sample"]
        modules = [run_module(c, selection, anchors, args.objdump) for c in config["modules"]]
        tool = Path(shutil.which(args.objdump) or args.objdump)
        input_paths = (("config", args.config), ("address_size_selection", args.selection),
                       ("anchors", args.anchors), ("objdump", tool), ("machine_extractor", Path(machine.__file__)))
        status = "INDEPENDENT_METADATA_CAPTURE_NO_GROUND_TRUTH"
    rows = [r for m in modules for r in m["functions"]]
    summary = {}
    for group in sorted({r["subsystem"] for r in rows}) + ["aggregate"]:
        selected = [r for r in rows if group == "aggregate" or r["subsystem"] == group]
        summary[group] = {"functions": len(selected),
                          "retail_saved_edge_or_caller_context": sum(bool(r["retail_context"]["coverage"]["saved_retail_call_edges"] + r["retail_context"]["coverage"]["saved_retail_callers"]) for r in selected),
                          "retail_unresolved_only_saved_context": sum(not (r["retail_context"]["coverage"]["saved_retail_call_edges"] + r["retail_context"]["coverage"]["saved_retail_callers"]) and bool(r["retail_context"]["coverage"]["saved_retail_unresolved_call_sites"]) for r in selected),
                          "retail_known_direct_neighborhood": sum(bool(r["retail_context"]["coverage"]["known_direct_callees"] + r["retail_context"]["coverage"]["known_direct_callers"]) for r in selected),
                          "paired_current_boundaries": None if not args.evaluate_capture else sum(any(c["context"]["coverage"]["function_boundary_available"] for c in r["current_candidates"]) for r in selected),
                          "retail_anchor_context": sum(any(t.startswith("anchor:") for t in r["retail_context"]["tokens"]) for r in selected),
                          "shared_support_activation": None if not args.evaluate_capture else {a: sum(any(c["comparisons"][a]["activated"] for c in r["current_candidates"]) for r in selected)
                                         for a in ("full", "off", "anchor_off", "import_off", "one_hop_only")}}
    inputs = {k: {"path": str(p), "sha256": machine.digest(p)} for k, p in input_paths}
    report = {"schema": "cfw.v4-graph-neighborhood.v1", "version": VERSION,
              "tool_sha256": machine.digest(__file__), "inputs": inputs, "modules": modules,
              "summary": summary, "promotion_count": 0,
              "status": status,
              "supersedes": "v4-graph-context-development-20261013.json (mixed extraction/evaluation architecture; do not use)",
              "limits": ["Old pilot is development, not a new blind validation sample",
                         "No matcher score, candidate ranking, confidence or semantic equivalence",
                         "Three preexisting reviewed Tier0 identities only; no Tier2 or recursive propagation",
                         "No inferred indirect targets; jumps remain tail candidates",
                         "Absence unknown; function-boundary coverage is incomplete",
                         "Two-hop imports can be generic and need negative-candidate discrimination before scoring",
                         "Current identity comparisons use reviewed development inventory, never anchor it"]}
    with args.output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, sort_keys=True, separators=(",", ":"))
        stream.write("\n")
    print(json.dumps({"output": str(args.output), "sha256": machine.digest(args.output), "summary": summary}))


if __name__ == "__main__":
    main()
