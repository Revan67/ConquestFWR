#!/usr/bin/env python3
"""Anchor-free V4 graph signatures; independent capture/scoring, posthoc review.

No ground-truth labels, source names or Tier0/Tier2 identities enter signatures
or scoring. Frozen body candidate generation is retained unchanged. Observed
neighborhood overlap supplies a nonnegative bonus, never missing-edge rejection.
This is development research, not a validated matcher or semantic equivalence.
"""
from __future__ import annotations

import argparse
from bisect import bisect_left, bisect_right
from collections import Counter, defaultdict
import gzip
import json
import math
from pathlib import Path
import shutil
import sys

import v4_graph_context as graph
import v3_callgraph_context as machine
import extract_machine_features as normalized

VERSION = "v4-anchor-free-neighborhood-signatures-2"
SCORE_VERSION = "v4-anchor-free-neighborhood-ranking-3"
FAMILY_WEIGHTS = {"counts": .10, "imports": .20, "indirect": .05,
                  "neighbor_body": .30, "degree_role": .35}
BONUS_WEIGHT = .20
EPSILON = .005
INFORMATIVE_MAX_DF = .10


def bucket(value):
    return 0 if value <= 0 else min(12, int(math.log2(value)) + 1)


def body_fingerprint(bounds, instructions, addresses):
    selected = set()
    for left, right in bounds["ranges"]:
        selected.update(addresses[bisect_left(addresses, left):bisect_left(addresses, right)])
    if not selected or min(selected) != bounds["ranges"][0][0]:
        return None
    names = [normalized.mnemonic(instructions[va][0]) for va in sorted(selected)]
    c = Counter(names)
    flow = {"calls": sum(n for op, n in c.items() if op.startswith("call")),
            "branches": sum(n for op, n in c.items() if normalized.BRANCH.match(op)),
            "returns": sum(n for op, n in c.items() if op.startswith("ret"))}
    # Coarse numeric proportions are deliberately compiler-tolerant and cannot
    # establish identity. Mnemonic text is normalized vocabulary, not retail code.
    proportions = {kind: min(3, int(sum(n for op, n in c.items() if op.startswith(prefix)) * 4 / len(names)))
                   for kind, prefix in (("move", "mov"), ("stack", ("push", "pop")),
                                        ("compare", ("cmp", "test")), ("float", "f"))}
    token = ":".join(["n" + str(bucket(len(names))),
                      "c" + str(bucket(flow["calls"])), "b" + str(bucket(flow["branches"])),
                      "r" + str(bucket(flow["returns"]))] + [k + str(v) for k, v in proportions.items()])
    return {"instruction_count": len(names), "flow": flow, "proportions": proportions,
            "coarse_fingerprint": token, "evidence": "bounded normalized machine opcode counts; not semantic IR"}


def signatures(saved, instructions):
    g = graph.restore_graph(saved)
    adjacent, _ = graph.adjacency(g)
    addresses = sorted(instructions)
    bodies = {va: body_fingerprint(bound, instructions, addresses) for va, bound in g["bounds"].items()}
    imports, indirect, outside = defaultdict(Counter), Counter(), Counter()
    for e in g["edges"]:
        if e.get("ownership_ambiguous"):
            continue
        va = e["owner"]
        name = e.get("import_name") or e.get("import_via_one_hop_thunk")
        if name and e["instruction_kind"] == "call":
            imports[va][name] += 1
        indirect[va] += e["kind"] == "UNRESOLVED_INDIRECT_CALL"
        outside[va] += e["kind"] == "DIRECT_CALL" and not e.get("target_is_function_entry")
    result = {}
    for va, bound in sorted(g["bounds"].items()):
        families = {name: Counter() for name in FAMILY_WEIGHTS}
        callers, callees = len(adjacent[va]["in"]), len(adjacent[va]["out"])
        for role, value in (("callers", callers), ("callees", callees)):
            if value:
                families["counts"][role + ":" + str(bucket(value))] = 1
        families["imports"].update({k: min(4, v) for k, v in imports[va].items()})
        if indirect[va]:
            families["indirect"]["sites:" + str(bucket(indirect[va]))] = 1
        frontier = [(va, (), frozenset({va}))]
        unique_nodes = set()
        for depth in (1, 2):
            following = []
            # A node can be counted once per direction sequence; duplicate
            # callsites and distinct walks to the same endpoint do not amplify it.
            endpoints = set()
            for owner, route, seen in frontier:
                for direction in ("in", "out"):
                    for target in adjacent[owner][direction]:
                        if target in seen:
                            continue
                        path = route + (direction,)
                        following.append((target, path, seen | {target}))
                        endpoints.add((path, target))
                        unique_nodes.add(target)
            for route, target in endpoints:
                prefix = "/".join(route) + ":"
                ni, no = len(adjacent[target]["in"]), len(adjacent[target]["out"])
                # Zero counts are not positive observed facts about semantic
                # absence, so roles encode observed edges/sites only.
                role = ":".join(f"{k}{bucket(v)}" for k, v in
                                (("in", ni), ("out", no), ("ind", indirect[target]),
                                 ("imp", sum(imports[target].values()))) if v)
                if role:
                    families["degree_role"][prefix + role] += 1
                if bodies[target] is not None:
                    families["neighbor_body"][prefix + bodies[target]["coarse_fingerprint"]] += 1
            frontier = following
        result[str(va)] = {"body": bodies[va], "direct_callers": callers, "direct_callees": callees,
                           "import_multiset": dict(sorted(imports[va].items())), "indirect_calls": indirect[va],
                           "families": {k: dict(sorted(v.items())) for k, v in families.items()},
                           "coverage": {"boundary_provenance": bound.get("provenance"),
                                        "entry_instruction_available": bound.get("entry_instruction_available"),
                                        "body_available": bodies[va] is not None,
                                        "unknown_direct_targets": outside[va],
                                        "neighbor_nodes_observed": len(unique_nodes),
                                        "indirect_targets": "UNKNOWN", "missing_edges": "UNKNOWN",
                                        "complete_callgraph": False}}
    return result


def frequencies(current):
    n = len(current)
    counts = {family: Counter() for family in FAMILY_WEIGHTS}
    for row in current.values():
        for family in counts:
            counts[family].update(row["families"][family].keys())
    return {"population": n, "df": {k: dict(sorted(v.items())) for k, v in counts.items()}}


def postings(current, base):
    result = {family: defaultdict(list) for family in FAMILY_WEIGHTS}
    for va, row in current.items():
        rva = int(va) - base
        for family, tokens in row["families"].items():
            for key, value in tokens.items():
                result[family][key].append((rva, min(4, value)))
    return result


def sparse_compare(left, indexed, df, one_hop=False):
    """Equivalent positive-overlap comparator, evaluated through sparse postings."""
    components, observed, informative = defaultdict(dict), Counter(), Counter()
    for family, tokens in left["families"].items():
        sums, denominator = Counter(), 0.0
        for key, count in tokens.items():
            if one_hop and family in {"neighbor_body", "degree_role"} and "/" in key.split(":", 1)[0]:
                continue
            frequency = df["df"][family].get(key, 0)
            idf = math.log((df["population"] + 1) / (frequency + 1)) + 1
            denominator += min(count, 4) * idf
            rare = frequency / max(df["population"], 1) <= INFORMATIVE_MAX_DF
            for rva, value in indexed[family].get(key, []):
                sums[rva] += min(count, value, 4) * idf
                observed[rva] += 1
                informative[rva] += rare
        if denominator:
            for rva, value in sums.items():
                components[rva][family] = value / denominator
    return {rva: {"components": {f: c.get(f, 0.0) for f in FAMILY_WEIGHTS},
                  "shared_observed_tokens": observed[rva], "shared_informative_tokens": informative[rva],
                  "support": sum(FAMILY_WEIGHTS[f] * c.get(f, 0.0) for f in FAMILY_WEIGHTS)}
            for rva, c in components.items()}


def compare(left, right, df, ablation="full"):
    components, informative, observed = {}, 0, 0
    for family, weight in FAMILY_WEIGHTS.items():
        if ablation == "off" or ablation == family + "_off":
            components[family] = 0.0
            continue
        a, b = left["families"][family], right["families"][family]
        if ablation == "one_hop_only" and family in {"neighbor_body", "degree_role"}:
            a = {k: v for k, v in a.items() if "/" not in k.split(":", 1)[0]}
            b = {k: v for k, v in b.items() if "/" not in k.split(":", 1)[0]}
        numerator = denominator = 0.0
        for key, count in a.items():
            frequency = df["df"][family].get(key, 0)
            # Current document frequency is computed over all independent exact
            # current extents, never over pilot truth or selected true candidates.
            idf = math.log((df["population"] + 1) / (frequency + 1)) + 1
            denominator += min(count, 4) * idf
            shared = min(count, b.get(key, 0), 4)
            numerator += shared * idf
            if shared:
                observed += 1
                informative += frequency / max(df["population"], 1) <= INFORMATIVE_MAX_DF
        components[family] = numerator / denominator if denominator else 0.0
    return {"support": sum(FAMILY_WEIGHTS[k] * v for k, v in components.items()),
            "components": components, "shared_observed_tokens": observed,
            "shared_informative_tokens": informative, "ablation": ablation,
            "coverage": {"left": left["coverage"], "right": right["coverage"]},
            "negative_evidence": False, "missing_edge_rejection": False, "promote": False}


def ranked(rows, key):
    # Tolerance neighborhoods can overlap and are not transitive. Calculate
    # each candidate's own conservative rank interval rather than partitioning
    # a sorted list into leader-based tie groups.
    scores = sorted(row[key] for row in rows)
    size = len(scores)
    return {
        row["rva"]: [size - bisect_right(scores, row[key] + EPSILON) + 1,
                     size - bisect_left(scores, row[key] - EPSILON)]
        for row in rows
    }


def write(path, payload):
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(payload, stream, sort_keys=True, separators=(",", ":"))
        stream.write("\n")
    print(json.dumps({"output": str(path), "sha256": machine.digest(path),
                      "summary": payload.get("summary")}))


def capture(args):
    source = json.loads(args.graph_capture.read_text(encoding="utf-8"))
    if source["status"] != "INDEPENDENT_METADATA_CAPTURE_NO_GROUND_TRUTH":
        raise ValueError("independent graph metadata required")
    modules = []
    for module in source["modules"]:
        side_sigs = {}
        for side in ("retail", "current"):
            image = Path(module["inputs"][side + "_image"]["path"])
            if machine.digest(image) != module[side + "_module_sha256"]:
                raise ValueError("independent image changed")
            side_sigs[side] = signatures(module[side + "_graph"], machine.disassemble(image, args.objdump))
        modules.append({"retail_module_sha256": module["retail_module_sha256"],
                        "current_module_sha256": module["current_module_sha256"],
                        "retail_image_base": module["retail_image_base"], "current_image_base": module["current_image_base"],
                        "retail": side_sigs["retail"], "current": side_sigs["current"],
                        "document_frequency": frequencies(side_sigs["current"])})
    return {"schema": "cfw.v4-anchor-free-graph-signatures.v1", "version": VERSION,
            "status": "INDEPENDENT_SIGNATURE_CAPTURE_NO_LABELS_NO_ANCHORS",
            "tool_sha256": machine.digest(__file__), "graph_capture_sha256": machine.digest(args.graph_capture),
            "toolchain": {"python": {"path": sys.executable, "sha256": machine.digest(sys.executable), "version": sys.version},
                          "objdump": {"path": shutil.which(args.objdump) or args.objdump,
                                      "sha256": machine.digest(shutil.which(args.objdump) or args.objdump)},
                          "graph_helper_sha256": machine.digest(graph.__file__),
                          "machine_extractor_sha256": machine.digest(machine.__file__)},
            "family_weights": FAMILY_WEIGHTS, "modules": modules,
            "summary": [{"retail_functions": len(m["retail"]), "current_functions": len(m["current"])} for m in modules],
            "absence_policy": "Unobserved edge or target unknown, never rejection; no zero-count match tokens",
            "provenance": "Coarse bounded opcode fingerprints, not decompiler IR or semantic identity",
            "anchor_policy": "No anchors, Tier1, Tier2 or propagation"}


def score(args):
    sigs = json.loads(args.signatures.read_text(encoding="utf-8"))
    if (sigs.get("schema") != "cfw.v4-anchor-free-graph-signatures.v1"
            or sigs.get("version") != VERSION
            or sigs.get("status") != "INDEPENDENT_SIGNATURE_CAPTURE_NO_LABELS_NO_ANCHORS"):
        raise ValueError("unexpected or unreviewed signature capture")
    modules = {m["retail_module_sha256"]: m for m in sigs["modules"]}
    if len(modules) != len(sigs["modules"]):
        raise ValueError("duplicate retail module identity in signature capture")
    indices = {key: postings(m["current"], m["current_image_base"]) for key, m in modules.items()}
    outputs = []
    ablations = ("full", "off", "counts_off", "imports_off", "indirect_off", "neighbor_body_off", "degree_role_off", "one_hop_only")
    with gzip.open(args.body_raw, "rt", encoding="utf-8") as stream:
        for line in stream:
            raw = json.loads(line)
            module = modules[raw["retail_module_sha256"]]
            if raw["current_module_sha256"] != module["current_module_sha256"]:
                raise ValueError("frozen body pool and signatures use different current module builds")
            if raw["candidate_count"] != len(raw["candidates"]):
                raise ValueError("frozen body candidate count mismatch")
            if len({c["rva"] for c in raw["candidates"]}) != len(raw["candidates"]):
                raise ValueError("duplicate current candidate RVA")
            left = module["retail"][str(int(raw["retail_va"], 16))]
            full = sparse_compare(left, indices[raw["retail_module_sha256"]], module["document_frequency"])
            one = sparse_compare(left, indices[raw["retail_module_sha256"]], module["document_frequency"], True)
            candidates = []
            for frozen in raw["candidates"]:
                rva = frozen["rva"]
                right = module["current"].get(str(module["current_image_base"] + rva))
                row = {"rva": rva, "body_score": frozen["body_score"], "graph_available": right is not None}
                comparison = full.get(rva, {"components": {f: 0.0 for f in FAMILY_WEIGHTS},
                                           "support": 0.0, "shared_observed_tokens": 0, "shared_informative_tokens": 0})
                row["comparisons"] = {"full": comparison} if right else {}
                for a in ablations:
                    support = comparison["support"] if right else 0
                    if a == "off":
                        support = 0
                    elif a == "one_hop_only":
                        support = one.get(rva, {}).get("support", 0)
                    elif a.endswith("_off"):
                        family = a.removesuffix("_off")
                        support -= FAMILY_WEIGHTS[family] * comparison["components"][family]
                    row[a] = row["body_score"] + BONUS_WEIGHT * support
                candidates.append(row)
            ranks = {a: ranked(candidates, a) for a in ablations}
            for row in candidates:
                row["ranks"] = {a: ranks[a][row["rva"]] for a in ablations}
            supports = [r["comparisons"].get("full", {}).get("support", 0) for r in candidates]
            variable = bool(supports) and max(supports) - min(supports) > EPSILON
            informative = any(r["comparisons"].get("full", {}).get("shared_informative_tokens", 0) for r in candidates)
            outputs.append({"subsystem": raw["subsystem"], "retail_va": raw["retail_va"],
                            "retail_module_sha256": raw["retail_module_sha256"],
                            "current_module_sha256": raw["current_module_sha256"],
                            "candidate_count": len(candidates), "candidates": candidates,
                            "candidate_specific_context_contributes": variable and informative,
                            "signature_coverage_count": sum(r["graph_available"] for r in candidates),
                            "candidate_pool_unchanged": True, "promotion_count": 0})
    return {"schema": "cfw.v4-anchor-free-graph-ranking.v2", "version": SCORE_VERSION,
            "status": "LABEL_FREE_OLD_SAMPLE_DEVELOPMENT_RANKING",
            "signature_capture_sha256": machine.digest(args.signatures), "body_raw_sha256": machine.digest(args.body_raw),
            "tool_sha256": machine.digest(__file__), "method": {"family_weights": FAMILY_WEIGHTS,
                "bonus_weight": BONUS_WEIGHT, "rank_tie_epsilon": EPSILON,
                "informative_max_df_fraction": INFORMATIVE_MAX_DF,
                "score": "frozen_body + .20 * nonnegative graph support; no pruning; incomplete current graph yields unchanged body",
                "activation": "graph contribution varies across eligible candidates and an observed shared token has current corpus DF <=10%; not proof of correct identity"},
            "functions": outputs, "summary": {"functions": len(outputs),
                "candidate_specific_context_contributes": sum(r["candidate_specific_context_contributes"] for r in outputs)},
            "calibrated_confidence": "UNAVAILABLE", "promotion_count": 0}


def evaluate(args):
    ranking = json.loads(args.ranking.read_text(encoding="utf-8"))
    inventory = json.loads(args.inventory.read_text(encoding="utf-8"))
    if (ranking.get("schema") != "cfw.v4-anchor-free-graph-ranking.v2"
            or ranking.get("version") != SCORE_VERSION
            or ranking.get("status") != "LABEL_FREE_OLD_SAMPLE_DEVELOPMENT_RANKING"
            or inventory.get("schema") != "cfw.v4-context-coverage-inventory.v1"):
        raise ValueError("unexpected ranking or posthoc inventory schema")
    truth = {(r["retail_module_sha256"], int(r["retail_va"], 16)): r for r in inventory["functions"]}
    rows = []
    for record in ranking["functions"]:
        request = truth[(record["retail_module_sha256"], int(record["retail_va"], 16))]
        source = request["channels"]["source_compiled_bridge"]
        rvas = [int(r, 16) for r in source["current_rvas"]]
        eligible = len(rvas) == 1 and source["identity_review"] in {"V", "S"}
        candidate = next((c for c in record["candidates"] if eligible and c["rva"] == rvas[0]), None)
        rows.append({"subsystem": record["subsystem"], "retail_va": record["retail_va"],
                     "rank_scored": eligible, "true_current_rvas": source["current_rvas"],
                     "retained": candidate is not None, "graph_available": None if candidate is None else candidate["graph_available"],
                     "true_ranks": None if candidate is None else candidate["ranks"],
                     "true_graph_support": None if candidate is None else candidate["comparisons"].get("full"),
                     "candidate_specific_context_contributes": record["candidate_specific_context_contributes"]})
    summary = {}
    for group in sorted({r["subsystem"] for r in rows}) + ["aggregate"]:
        subset = [r for r in rows if group == "aggregate" or r["subsystem"] == group]
        eligible = [r for r in subset if r["rank_scored"]]
        methods = {}
        for method in ranking["functions"][0]["candidates"][0]["ranks"]:
            ranks = [r["true_ranks"][method][1] if r["true_ranks"] else None for r in eligible]
            methods[method] = {"denominator": len(eligible), **{f"top_{k}": sum(r is not None and r <= k for r in ranks) for k in (1,3,5,10)},
                               "mrr": sum(1/r for r in ranks if r) / len(eligible) if eligible else None}
        deltas = [(r["true_ranks"]["off"][1], r["true_ranks"]["full"][1]) for r in eligible if r["true_ranks"]]
        summary[group] = {"functions": len(subset), "retained_true_candidates": sum(r["retained"] for r in eligible),
                          "candidate_specific_context_contributes": sum(r["candidate_specific_context_contributes"] for r in subset),
                          "improved_worst_rank": sum(b < a for a,b in deltas), "worsened_worst_rank": sum(b > a for a,b in deltas),
                          "unchanged_worst_rank": sum(b == a for a,b in deltas), "methods": methods}
    return {"schema": "cfw.v4-anchor-free-graph-posthoc.v1", "status": "POSTHOC_DEVELOPMENT_ONLY_NOT_BLIND",
            "ranking_sha256": machine.digest(args.ranking), "inventory_sha256": machine.digest(args.inventory),
            "tool_sha256": machine.digest(__file__), "rows": rows, "summary": summary,
            "limits": ["Old pilot labels used only here, not in capture or scoring", "No weights tuned against these results",
                       "Candidate pool unchanged; graph metadata coverage and compiler artifacts differ",
                       "Coarse topology/body tokens are support, not identity proof; no calibrated confidence",
                       "Activation definition is mechanical and must be interpreted with rank quality and specificity"]}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("mode", choices=("capture", "score", "evaluate"))
    for name in ("graph-capture", "signatures", "body-raw", "ranking", "inventory"):
        p.add_argument("--" + name, type=Path)
    p.add_argument("--objdump", default="llvm-objdump")
    p.add_argument("--output", required=True, type=Path)
    args = p.parse_args()
    if args.output.exists() or graph.PRIVATE.resolve() not in args.output.resolve().parents:
        raise ValueError("new private output required")
    needed = {"capture": {"graph_capture"}, "score": {"signatures", "body_raw"}, "evaluate": {"ranking", "inventory"}}[args.mode]
    if {n for n in ("graph_capture", "signatures", "body_raw", "ranking", "inventory") if getattr(args, n)} != needed:
        raise ValueError("capture/scoring/evaluation inputs must remain separate")
    write(args.output, {"capture": capture, "score": score, "evaluate": evaluate}[args.mode](args))


if __name__ == "__main__":
    main()
