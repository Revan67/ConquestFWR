#!/usr/bin/env python3
"""Measure transferable exact-export neighborhoods without creating seeds.

Shared decorated exports are symbol-identity leads, not proven implementation
parity. This sidecar measures bounded graph-path overlap only. Capture never
loads old-pilot truth; evaluation does so only after capture is immutable.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path

import v4_graph_context as graph_tool


PRIVATE = Path(__file__).resolve().parents[3] / "Evidence/RetailMap/local"
SCHEMA = "cfw.v4.export-neighborhood-audit.v1"


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest().upper()


def write(path: Path, payload: dict) -> None:
    if path.exists() or PRIVATE.resolve() not in path.resolve().parents:
        raise ValueError("new private output required")
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(payload, stream, sort_keys=True, separators=(",", ":"))
        stream.write("\n")
    print(json.dumps({"output": str(path), "sha256": digest(path),
                      "summary": payload.get("summary")}, sort_keys=True))


def graph_tokens(saved: dict, exports: dict[int, set[str]]) -> dict[int, list[str]]:
    graph = graph_tool.restore_graph(saved)
    adjacent, _ = graph_tool.adjacency(graph)
    answer = {}
    for origin in graph["bounds"]:
        tokens = set()
        frontier = [(origin, (), (origin,))]
        for depth in range(3):
            following = []
            for node, directions, walk in frontier:
                route = "/".join(directions) or "self"
                for name in exports.get(node, ()):
                    tokens.add(route + ":" + name)
                if depth == 2:
                    continue
                for direction in ("in", "out"):
                    for neighbor in adjacent[node][direction]:
                        if neighbor not in walk:
                            following.append((neighbor, directions + (direction,), walk + (neighbor,)))
            frontier = following
        if tokens:
            answer[origin] = sorted(tokens)
    return answer


def capture(graph_path: Path, index_path: Path, ranking_path: Path) -> dict:
    captured = json.loads(graph_path.read_text(encoding="utf-8"))
    index = json.loads(index_path.read_text(encoding="utf-8"))
    ranking = json.loads(ranking_path.read_text(encoding="utf-8"))
    if (captured.get("status") != "INDEPENDENT_METADATA_CAPTURE_NO_GROUND_TRUTH"
            or ranking.get("schema") != "cfw.v4-anchor-free-graph-ranking.v2"
            or ranking.get("status") != "LABEL_FREE_OLD_SAMPLE_DEVELOPMENT_RANKING"
            or index.get("schema") != "cfw.v4.trim-domain-provenance.v1"):
        raise ValueError("unexpected independent input schema")
    module = next(m for m in captured["modules"] if m["retail_module_sha256"] == index["retail_contract"]["module_sha256"])
    if module["current_module_sha256"] != index["current_contract"]["module_sha256"]:
        raise ValueError("Trim current build mismatch")
    retail = graph_tool.restore_graph(module["retail_graph"])
    current = graph_tool.restore_graph(module["current_graph"])
    re = {entry["name"]: module["retail_image_base"] + int(entry["rva"], 16)
          for entry in index["retail_contract"]["exports"]}
    ce = {entry["name"]: module["current_image_base"] + int(entry["rva"], 16)
          for entry in index["current_contract"]["exports"]}
    common = sorted(set(re) & set(ce))
    eligible = [name for name in common if re[name] in retail["bounds"] and ce[name] in current["bounds"]]
    retail_names, current_names = defaultdict(set), defaultdict(set)
    for name in eligible:
        retail_names[re[name]].add(name)
        current_names[ce[name]].add(name)
    rtokens = graph_tokens(module["retail_graph"], retail_names)
    ctokens = graph_tokens(module["current_graph"], current_names)
    rows = []
    for record in ranking["functions"]:
        if record["retail_module_sha256"] != module["retail_module_sha256"]:
            continue
        if record["current_module_sha256"] != module["current_module_sha256"]:
            raise ValueError("frozen ranking uses another Trim build")
        va = int(record["retail_va"], 16)
        left = set(rtokens.get(va, ()))
        overlaps = []
        for candidate in record["candidates"]:
            right = ctokens.get(module["current_image_base"] + candidate["rva"], ())
            shared = sorted(left.intersection(right))
            if shared:
                overlaps.append({"current_rva": candidate["rva"], "shared_tokens": shared,
                                 "body_rank_worst": candidate["ranks"]["off"][1]})
        rows.append({"subsystem": record["subsystem"], "retail_va": record["retail_va"],
                     "retail_export_tokens": sorted(left), "candidate_count": record["candidate_count"],
                     "overlap_candidates": overlaps, "overlap_count": len(overlaps)})
    return {"schema": SCHEMA, "mode": "LABEL_FREE_CAPTURE", "tool_sha256": digest(Path(__file__)),
            "graph_context_dependency_sha256": digest(Path(graph_tool.__file__)),
            "inputs_sha256": {"graph": digest(graph_path), "domain_index": digest(index_path),
                              "ranking": digest(ranking_path)},
            "module_sha256": {"retail": module["retail_module_sha256"],
                              "current": module["current_module_sha256"]},
            "shared_export_names": len(common), "both_graph_bound_export_names": len(eligible),
            "retail_graph_functions_with_near_export": len(rtokens),
            "current_graph_functions_with_near_export": len(ctokens),
            "rows": rows,
            "summary": {"development_trim_functions": len(rows),
                        "with_retail_export_tokens": sum(bool(row["retail_export_tokens"]) for row in rows),
                        "with_any_candidate_overlap": sum(bool(row["overlap_count"]) for row in rows),
                        "total_overlap_candidates": sum(row["overlap_count"] for row in rows)},
            "limits": ["Exact shared export spelling establishes a symbol-identity lead, not semantic parity or a validated Tier-0/1 seed.",
                       "Only directly observed function-entry call edges and bounded two-hop paths are counted.",
                       "Missing path, compiler boundaries, thunk recovery or graph edges are unknown, never negative evidence.",
                       "No pilot true identity participates in capture, rule construction, or graph propagation."]}


def evaluate(capture_path: Path, posthoc_path: Path) -> dict:
    observed = json.loads(capture_path.read_text(encoding="utf-8"))
    reviewed = json.loads(posthoc_path.read_text(encoding="utf-8"))
    if (observed.get("schema") != SCHEMA or observed.get("mode") != "LABEL_FREE_CAPTURE"
            or reviewed.get("schema") != "cfw.v4-anchor-free-graph-posthoc.v1"
            or reviewed.get("ranking_sha256") != observed["inputs_sha256"]["ranking"]):
        raise ValueError("posthoc truth and capture do not share the frozen ranking")
    lookup = {row["retail_va"]: row for row in observed["rows"]}
    rows = []
    for truth in reviewed["rows"]:
        if truth["subsystem"] != "Trim/UI":
            continue
        captured = lookup[truth["retail_va"]]
        true_rvas = [int(rva, 16) for rva in truth["true_current_rvas"]]
        matches = [candidate for candidate in captured["overlap_candidates"]
                   if candidate["current_rva"] in true_rvas]
        rows.append({"retail_va": truth["retail_va"], "rank_scored": truth["rank_scored"],
                     "retail_has_export_context": bool(captured["retail_export_tokens"]),
                     "candidate_overlap_count": captured["overlap_count"],
                     "true_candidate_overlap": bool(matches) if truth["rank_scored"] else None,
                     "true_shared_tokens": matches[0]["shared_tokens"] if matches else []})
    scored = [row for row in rows if row["rank_scored"]]
    return {"schema": SCHEMA, "mode": "POSTHOC_DEVELOPMENT_EVALUATION",
            "tool_sha256": digest(Path(__file__)), "capture_sha256": digest(capture_path),
            "posthoc_sha256": digest(posthoc_path), "rows": rows,
            "summary": {"old_trim_functions": len(rows), "independently_scored": len(scored),
                        "retail_with_export_context": sum(row["retail_has_export_context"] for row in rows),
                        "true_pairs_with_shared_export_path": sum(row["true_candidate_overlap"] is True for row in scored),
                        "unresolved_not_forced": len(rows) - len(scored)},
            "limits": ["Posthoc old-development identities do not become export seeds or correspondence promotions.",
                       "Lack of observed export path cannot reject a candidate."]}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("capture", "evaluate"))
    for name in ("graph", "index", "ranking", "capture", "posthoc"):
        parser.add_argument("--" + name, type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    needed = {"capture": {"graph", "index", "ranking"},
              "evaluate": {"capture", "posthoc"}}[args.mode]
    provided = {name for name in ("graph", "index", "ranking", "capture", "posthoc")
                if getattr(args, name) is not None}
    if provided != needed:
        parser.error(f"{args.mode} requires exactly {sorted(needed)}")
    payload = (capture(args.graph, args.index, args.ranking) if args.mode == "capture"
               else evaluate(args.capture, args.posthoc))
    write(args.output, payload)


if __name__ == "__main__":
    main()
