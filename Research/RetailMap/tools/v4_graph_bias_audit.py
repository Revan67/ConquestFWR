#!/usr/bin/env python3
"""Audit V4 graph-score availability separately from old-pilot truth.

The capture mode reads only label-free rankings. The evaluate mode introduces
reviewed identities solely for posthoc development diagnosis. Neither mode
changes candidate scores, propagates anchors, or promotes correspondences.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import statistics


SCHEMA = "cfw.v4.graph-bias-audit.v1"
PRIVATE = Path(__file__).resolve().parents[3] / "Evidence/RetailMap/local"
EPSILON = .005


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest().upper()


def write(path: Path, payload: dict) -> None:
    if path.exists() or PRIVATE.resolve() not in path.resolve().parents:
        raise ValueError("output must be a new private evidence file")
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(payload, stream, sort_keys=True, separators=(",", ":"))
        stream.write("\n")
    print(json.dumps({"output": str(path), "sha256": digest(path),
                      "summary": payload.get("summary", {})}, sort_keys=True))


def fraction(numerator: int, denominator: int) -> float | None:
    return numerator / denominator if denominator else None


def summary(rows: list[dict]) -> dict:
    result = {}
    for subsystem in sorted({row["subsystem"] for row in rows}) + ["aggregate"]:
        part = [row for row in rows if subsystem == "aggregate" or row["subsystem"] == subsystem]
        candidate_total = sum(row["candidate_count"] for row in part)
        result[subsystem] = {
            "functions": len(part),
            "candidates": candidate_total,
            "graph_covered_candidates": sum(row["graph_covered"] for row in part),
            "graph_covered_fraction": fraction(sum(row["graph_covered"] for row in part), candidate_total),
            "body_top10_graph_covered": sum(row["body_top10_graph_covered"] for row in part),
            "body_top10_count": sum(row["body_top10_count"] for row in part),
            "graph_top10_graph_covered": sum(row["graph_top10_graph_covered"] for row in part),
            "graph_top10_count": sum(row["graph_top10_count"] for row in part),
            "functions_with_only_broad_count_support": sum(row["only_broad_count_support"] for row in part),
            "functions_with_informative_candidate_support": sum(row["informative_candidates"] > 0 for row in part),
        }
    return result


def capture(ranking_path: Path) -> dict:
    ranking = json.loads(ranking_path.read_text(encoding="utf-8"))
    if (ranking.get("schema") != "cfw.v4-anchor-free-graph-ranking.v2"
            or ranking.get("status") != "LABEL_FREE_OLD_SAMPLE_DEVELOPMENT_RANKING"):
        raise ValueError("corrected label-free V4 ranking required")
    rows = []
    for record in ranking["functions"]:
        candidates = record["candidates"]
        if len(candidates) != record["candidate_count"]:
            raise ValueError("candidate count mismatch")
        covered = [row for row in candidates if row["graph_available"]]
        uncovered = [row for row in candidates if not row["graph_available"]]
        with_support = [row for row in covered if row["comparisons"]["full"]["support"] > 0]
        informative = [row for row in covered if row["comparisons"]["full"]["shared_informative_tokens"] > 0]
        family_counts = Counter()
        for row in with_support:
            for family, value in row["comparisons"]["full"]["components"].items():
                if value > 0:
                    family_counts[family] += 1
        body_top10 = [row for row in candidates if row["ranks"]["off"][0] <= 10]
        graph_top10 = [row for row in candidates if row["ranks"]["full"][0] <= 10]
        body_winners = [row for row in candidates if row["ranks"]["off"][0] == 1]
        graph_winners = [row for row in candidates if row["ranks"]["full"][0] == 1]
        rows.append({
            "subsystem": record["subsystem"], "retail_va": record["retail_va"],
            "retail_module_sha256": record["retail_module_sha256"],
            "current_module_sha256": record["current_module_sha256"],
            "candidate_count": len(candidates), "graph_covered": len(covered),
            "graph_uncovered": len(uncovered), "nonzero_support_candidates": len(with_support),
            "informative_candidates": len(informative),
            "only_broad_count_support": bool(with_support) and not informative,
            "active_family_candidate_counts": dict(family_counts),
            "body_top10_count": len(body_top10),
            "body_top10_graph_covered": sum(row["graph_available"] for row in body_top10),
            "graph_top10_count": len(graph_top10),
            "graph_top10_graph_covered": sum(row["graph_available"] for row in graph_top10),
            "body_winners": [{"rva": row["rva"], "graph_available": row["graph_available"],
                              "graph_support": row["comparisons"].get("full", {}).get("support", 0)}
                             for row in body_winners],
            "graph_winners": [{"rva": row["rva"], "graph_available": row["graph_available"],
                               "graph_support": row["comparisons"].get("full", {}).get("support", 0)}
                              for row in graph_winners],
            "covered_body_score_mean": statistics.fmean(row["body_score"] for row in covered) if covered else None,
            "uncovered_body_score_mean": statistics.fmean(row["body_score"] for row in uncovered) if uncovered else None,
            "covered_graph_bonus_mean": statistics.fmean(row["full"]-row["body_score"] for row in covered) if covered else None,
            "uncovered_graph_bonus_max": max((row["full"]-row["body_score"] for row in uncovered), default=0),
        })
    return {"schema": SCHEMA, "mode": "LABEL_FREE_CAPTURE", "tool_sha256": digest(Path(__file__)),
            "ranking_sha256": digest(ranking_path), "method": "candidate availability and score-contribution census; no truth",
            "rows": rows, "summary": summary(rows),
            "limits": ["Graph availability differs across candidates; zero bonus for missing metadata is not negative evidence.",
                       "Mechanical score variation and family activation are not identity discrimination.",
                       "No body score, graph score, rank, candidate domain or confidence is changed."]}


def evaluate(capture_path: Path, posthoc_path: Path) -> dict:
    observed = json.loads(capture_path.read_text(encoding="utf-8"))
    reviewed = json.loads(posthoc_path.read_text(encoding="utf-8"))
    if (observed.get("schema") != SCHEMA or observed.get("mode") != "LABEL_FREE_CAPTURE"
            or reviewed.get("schema") != "cfw.v4-anchor-free-graph-posthoc.v1"
            or reviewed.get("ranking_sha256") != observed.get("ranking_sha256")):
        raise ValueError("capture and posthoc identity mismatch")
    lookup = {(row["subsystem"], row["retail_va"]): row for row in observed["rows"]}
    rows = []
    for truth in reviewed["rows"]:
        info = lookup[(truth["subsystem"], truth["retail_va"])]
        rank = truth["true_ranks"]
        support = truth["true_graph_support"] or {}
        rows.append({"subsystem": truth["subsystem"], "retail_va": truth["retail_va"],
                     "rank_scored": truth["rank_scored"], "retained": truth["retained"],
                     "graph_available_for_true": truth["graph_available"],
                     "true_informative_tokens": support.get("shared_informative_tokens"),
                     "true_graph_support": support.get("support"),
                     "true_rank_body_worst": None if rank is None else rank["off"][1],
                     "true_rank_graph_worst": None if rank is None else rank["full"][1],
                     "candidate_graph_coverage_fraction": fraction(info["graph_covered"], info["candidate_count"]),
                     "graph_winner_covered": any(row["graph_available"] for row in info["graph_winners"]),
                     "graph_winner_unsupported": any(not row["graph_support"] for row in info["graph_winners"]),
                     "body_top10_coverage_fraction": fraction(info["body_top10_graph_covered"], info["body_top10_count"]),
                     "graph_top10_coverage_fraction": fraction(info["graph_top10_graph_covered"], info["graph_top10_count"]),
                     })
    summary_by_subsystem = {}
    for subsystem in sorted({row["subsystem"] for row in rows}) + ["aggregate"]:
        part = [row for row in rows if subsystem == "aggregate" or row["subsystem"] == subsystem]
        scored = [row for row in part if row["rank_scored"]]
        summary_by_subsystem[subsystem] = {
            "functions": len(part), "rank_scored": len(scored),
            "true_with_graph_metadata": sum(row["graph_available_for_true"] is True for row in scored),
            "true_with_informative_tokens": sum(bool(row["true_informative_tokens"]) for row in scored),
            "true_rank_improved": sum(row["true_rank_graph_worst"] < row["true_rank_body_worst"] for row in scored if row["retained"]),
            "true_rank_worsened": sum(row["true_rank_graph_worst"] > row["true_rank_body_worst"] for row in scored if row["retained"]),
            "true_rank_unchanged": sum(row["true_rank_graph_worst"] == row["true_rank_body_worst"] for row in scored if row["retained"]),
            "false_or_ambiguous_graph_winner": sum(row["true_rank_graph_worst"] > 1 for row in scored if row["retained"]),
            "mean_candidate_graph_coverage": statistics.fmean(row["candidate_graph_coverage_fraction"] for row in part),
        }
    return {"schema": SCHEMA, "mode": "POSTHOC_DEVELOPMENT_EVALUATION",
            "tool_sha256": digest(Path(__file__)), "capture_sha256": digest(capture_path),
            "posthoc_sha256": digest(posthoc_path), "rows": rows,
            "summary": summary_by_subsystem,
            "limits": ["Reviewed true identities are used only after the label-free capture is fixed.",
                       "This old pilot is a V4 development set, never a new blind validation set.",
                       "Rank effects alone do not prove a transferable context or confidence model."]}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("capture", "evaluate"))
    parser.add_argument("--ranking", type=Path)
    parser.add_argument("--capture", type=Path)
    parser.add_argument("--posthoc", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    required = {"capture": {"ranking"}, "evaluate": {"capture", "posthoc"}}[args.mode]
    passed = {key for key in ("ranking", "capture", "posthoc") if getattr(args, key) is not None}
    if passed != required:
        parser.error(f"{args.mode} requires exactly {sorted(required)}")
    write(args.output, capture(args.ranking) if args.mode == "capture" else evaluate(args.capture, args.posthoc))


if __name__ == "__main__":
    main()
