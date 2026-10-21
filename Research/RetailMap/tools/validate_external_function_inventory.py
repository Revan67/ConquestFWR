#!/usr/bin/env python3
"""Compare an external retail-EXE boundary/call inventory with saved Ghidra facts.

This tool does not promote function boundaries or correspondences.  It emits
deterministic candidate-quality measurements and a stratified review sample.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from bisect import bisect_right
from pathlib import Path


SCHEMA = "cfw.external-function-inventory-validation.v1"
TOOL_VERSION = "1.0.0"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def parse_external_functions(path: Path) -> dict[int, dict]:
    rows: dict[int, dict] = {}
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        if not line.startswith("0x") or "\t" not in line:
            continue
        fields = line.split("\t")
        if len(fields) != 11:
            continue
        entry = int(fields[0], 16)
        rows[entry] = {
            "entry": entry,
            "size": int(fields[1]),
            "direct_calls": int(fields[2]),
            "labels": int(fields[3]),
            "imports": int(fields[4]),
            "virtual_calls": int(fields[5]),
            "register_indirect": int(fields[6]),
            "tail_calls": int(fields[7]),
            "called_by": int(fields[8]),
            "in_pointer_table": bool(int(fields[9])),
            "note": fields[10],
        }
    return rows


def parse_external_edges(path: Path) -> tuple[dict[int, set[int]], dict[int, set[int]]]:
    header = re.compile(r"^0x([0-9A-Fa-f]{8}) \(size \d+\)")
    direct = re.compile(r"^  -> call 0x([0-9A-Fa-f]{8})$")
    tail = re.compile(r"^  -> jmp  0x([0-9A-Fa-f]{8}) \(tail\)$")
    calls: dict[int, set[int]] = {}
    tails: dict[int, set[int]] = {}
    current: int | None = None
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        match = header.match(line)
        if match:
            current = int(match.group(1), 16)
            calls.setdefault(current, set())
            tails.setdefault(current, set())
            continue
        match = direct.match(line)
        if current is not None and match:
            calls[current].add(int(match.group(1), 16))
            continue
        match = tail.match(line)
        if current is not None and match:
            tails[current].add(int(match.group(1), 16))
    return calls, tails


def parse_ghidra(path: Path) -> dict[int, dict]:
    functions: dict[int, dict] = {}
    with path.open("r", encoding="utf-8-sig") as stream:
        for line in stream:
            row = json.loads(line)
            if row.get("record") != "function":
                continue
            entry = int(row["entry"], 16)
            ranges = [
                (int(item["start"], 16), int(item["end"], 16) + 1)
                for item in row.get("body_ranges", [])
            ]
            calls = set()
            for edge in row.get("call_edges", []):
                target = edge.get("target", "")
                if re.fullmatch(r"[0-9A-Fa-f]{8}", target):
                    calls.add(int(target, 16))
            functions[entry] = {
                "entry": entry,
                "ranges": ranges,
                "body_size": int(row.get("body_size", 0)),
                "calls": calls,
            }
    return functions


def containing_entry(address: int, ranges: list[tuple[int, int, int]]) -> int | None:
    starts = [item[0] for item in ranges]
    index = bisect_right(starts, address) - 1
    if index < 0:
        return None
    start, end, entry = ranges[index]
    return entry if start <= address < end else None


def even_sample(values: list[int], count: int) -> list[int]:
    if len(values) <= count:
        return values
    if count == 1:
        return [values[len(values) // 2]]
    return [values[(index * (len(values) - 1)) // (count - 1)] for index in range(count)]


def hex_address(value: int) -> str:
    return f"0x{value:08X}"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--external-functions", type=Path, required=True)
    parser.add_argument("--external-callgraph", type=Path, required=True)
    parser.add_argument("--ghidra-metadata", type=Path, required=True)
    parser.add_argument("--external-analyzer", type=Path, required=True)
    parser.add_argument("--retail-exe", type=Path, required=True)
    parser.add_argument("--disassembly", type=Path, required=True)
    parser.add_argument("--repo-commit", required=True)
    parser.add_argument("--llvm-version", required=True)
    parser.add_argument("--seed-probe", type=Path)
    parser.add_argument("--sample-per-stratum", type=int, default=15)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    external = parse_external_functions(args.external_functions)
    external_calls, external_tails = parse_external_edges(args.external_callgraph)
    ghidra = parse_ghidra(args.ghidra_metadata)
    external_entries, ghidra_entries = set(external), set(ghidra)
    shared = external_entries & ghidra_entries
    external_only = external_entries - ghidra_entries
    ghidra_only = ghidra_entries - external_entries

    exact_ranges = sorted(
        (start, end, entry)
        for entry, row in ghidra.items()
        for start, end in row["ranges"]
    )

    direct_targets = {target for targets in external_calls.values() for target in targets}
    tail_targets = {target for targets in external_tails.values() for target in targets}
    strata: dict[str, list[int]] = {
        "INSIDE_SAVED_GHIDRA_BODY": [],
        "OUTSIDE_BODY_DIRECT_CALL_AND_POINTER": [],
        "OUTSIDE_BODY_DIRECT_CALL": [],
        "OUTSIDE_BODY_TAIL_AND_POINTER": [],
        "OUTSIDE_BODY_TAIL": [],
        "OUTSIDE_BODY_POINTER": [],
        "OUTSIDE_BODY_HEURISTIC": [],
    }
    for entry in sorted(external_only):
        if containing_entry(entry, exact_ranges) is not None:
            strata["INSIDE_SAVED_GHIDRA_BODY"].append(entry)
            continue
        row = external[entry]
        direct, tail, pointer = entry in direct_targets, entry in tail_targets, row["in_pointer_table"]
        if direct and pointer:
            key = "OUTSIDE_BODY_DIRECT_CALL_AND_POINTER"
        elif direct:
            key = "OUTSIDE_BODY_DIRECT_CALL"
        elif tail and pointer:
            key = "OUTSIDE_BODY_TAIL_AND_POINTER"
        elif tail:
            key = "OUTSIDE_BODY_TAIL"
        elif pointer:
            key = "OUTSIDE_BODY_POINTER"
        else:
            key = "OUTSIDE_BODY_HEURISTIC"
        strata[key].append(entry)

    extent_counts = {
        "ANALYZER_END_BEFORE_GHIDRA_END": 0,
        "ANALYZER_END_AT_GHIDRA_END": 0,
        "ANALYZER_END_AFTER_GHIDRA_END": 0,
        "NONCONTIGUOUS_GHIDRA_BODY": 0,
    }
    for entry in shared:
        analyzer_end = entry + external[entry]["size"]
        ranges = ghidra[entry]["ranges"]
        ghidra_end = max((end for _, end in ranges), default=entry)
        if len(ranges) > 1:
            extent_counts["NONCONTIGUOUS_GHIDRA_BODY"] += 1
        if analyzer_end < ghidra_end:
            extent_counts["ANALYZER_END_BEFORE_GHIDRA_END"] += 1
        elif analyzer_end == ghidra_end:
            extent_counts["ANALYZER_END_AT_GHIDRA_END"] += 1
        else:
            extent_counts["ANALYZER_END_AFTER_GHIDRA_END"] += 1

    true_positive = false_positive = false_negative = exact_callers = 0
    callers_compared = 0
    mismatch_rows = []
    for entry in sorted(shared):
        left = external_calls.get(entry, set())
        right = ghidra[entry]["calls"]
        callers_compared += 1
        true_positive += len(left & right)
        false_positive += len(left - right)
        false_negative += len(right - left)
        if left == right:
            exact_callers += 1
        elif len(mismatch_rows) < 100:
            mismatch_rows.append({
                "entry": hex_address(entry),
                "external_only_targets": [hex_address(value) for value in sorted(left - right)],
                "ghidra_only_targets": [hex_address(value) for value in sorted(right - left)],
            })

    precision_denominator = true_positive + false_positive
    recall_denominator = true_positive + false_negative
    samples = {
        key: [hex_address(value) for value in even_sample(values, args.sample_per_stratum)]
        for key, values in strata.items()
    }

    result = {
        "schema": SCHEMA,
        "tool_version": TOOL_VERSION,
        "status": "COMPLETE",
        "policy": "candidate inventory validation only; no function or correspondence promotion",
        "repo_commit": args.repo_commit,
        "inputs": {
            "retail_exe": {"path": str(args.retail_exe), "sha256": sha256(args.retail_exe)},
            "external_analyzer": {"path": str(args.external_analyzer), "sha256": sha256(args.external_analyzer)},
            "external_functions": {"path": str(args.external_functions), "sha256": sha256(args.external_functions)},
            "external_callgraph": {"path": str(args.external_callgraph), "sha256": sha256(args.external_callgraph)},
            "ghidra_metadata": {"path": str(args.ghidra_metadata), "sha256": sha256(args.ghidra_metadata)},
            "disassembly": {"path": str(args.disassembly), "sha256": sha256(args.disassembly)},
            "llvm_version": args.llvm_version,
        },
        "function_overlap": {
            "external": len(external_entries),
            "ghidra_saved": len(ghidra_entries),
            "shared": len(shared),
            "external_only": len(external_only),
            "ghidra_only": len(ghidra_only),
            "ghidra_only_entries": [hex_address(value) for value in sorted(ghidra_only)],
        },
        "external_only_strata": {key: len(values) for key, values in strata.items()},
        "shared_extent_comparison": extent_counts,
        "direct_call_comparison": {
            "shared_callers_compared": callers_compared,
            "exact_target_set_callers": exact_callers,
            "exact_target_set_rate": exact_callers / callers_compared if callers_compared else None,
            "true_positive_edges": true_positive,
            "external_only_edges": false_positive,
            "ghidra_only_edges": false_negative,
            "precision_against_saved_ghidra": true_positive / precision_denominator if precision_denominator else None,
            "recall_against_saved_ghidra": true_positive / recall_denominator if recall_denominator else None,
            "first_mismatches": mismatch_rows,
        },
        "stratified_review_sample": samples,
    }
    if args.seed_probe:
        seed_rows = {}
        with args.seed_probe.open("r", encoding="utf-8-sig") as stream:
            for line in stream:
                row = json.loads(line)
                if row.get("record") == "seed_probe":
                    seed_rows[int(row["entry"], 16)] = row
        seed_summary = {}
        for stratum, addresses in strata.items():
            selected = even_sample(addresses, args.sample_per_stratum)
            rows = [seed_rows[value] for value in selected if value in seed_rows]
            counts = {
                "selected": len(selected),
                "captured": len(rows),
                "defined_instruction": sum(bool(row.get("defined_instruction_at")) for row in rows),
                "inside_saved_function": sum(bool(row.get("containing_function")) for row in rows),
                "incoming_call": 0,
                "incoming_jump": 0,
                "incoming_data": 0,
                "no_saved_incoming_reference": 0,
            }
            for row in rows:
                types = set(row.get("incoming_reference_counts", {}))
                counts["incoming_call"] += int(any("CALL" in value for value in types))
                counts["incoming_jump"] += int(any("JUMP" in value for value in types))
                counts["incoming_data"] += int("DATA" in types)
                counts["no_saved_incoming_reference"] += int(not types)
            seed_summary[stratum] = counts
        result["seed_probe"] = {
            "input": {"path": str(args.seed_probe), "sha256": sha256(args.seed_probe)},
            "strata": seed_summary,
        }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    args.output.write_text(encoded, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
