#!/usr/bin/env python3
"""Control-only, machine-evidenced call context; never promotes correspondences.

Uses exact Ghidra body ranges and PDB extents, direct instruction operands and
PE import slots. Outgoing jumps are tail candidates, not proven semantic tails.
Missing edges are unknown: optimization/inlining and incomplete saved analysis
prevent treating absence as contradiction. Only reviewed calibration identities
can be fixed anchors; holdouts, predictions and recursive anchors are excluded.
All emitted evidence must remain in the ignored private evidence directory.
"""
from __future__ import annotations

import argparse
from bisect import bisect_left
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess

VERSION = "callgraph-context-v3-control-only-1"
RETAIL = "ED8741C0CB727A43B7DE7BE0E73566B04FBC1CBD58B54A62D538E3F39DCB3F52"
LINE = re.compile(r"^\s*([0-9a-fA-F]+):\s+([a-zA-Z][\w.]*)\s*(.*)$")
ADDRESS = re.compile(r"^(0x[0-9a-fA-F]+)\b")


def digest(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest().upper()


def jsonl(path):
    with Path(path).open(encoding="utf-8") as stream:
        return [json.loads(line) for line in stream if line.strip()]


def pe_imports(path):
    """Read PE32 IAT names independently; no dependency or binary export."""
    data = Path(path).read_bytes()
    u16 = lambda p: struct.unpack_from("<H", data, p)[0]
    u32 = lambda p: struct.unpack_from("<I", data, p)[0]
    if data[:2] != b"MZ":
        raise ValueError("not PE")
    nt = u32(0x3c)
    if data[nt:nt + 4] != b"PE\0\0" or u16(nt + 4) != 0x14c:
        raise ValueError("only x86 PE supported")
    opt = nt + 24
    if u16(opt) != 0x10b:
        raise ValueError("only PE32 supported")
    base = u32(opt + 28)
    sections = []
    for n in range(u16(nt + 6)):
        p = opt + u16(nt + 20) + n * 40
        sections.append((u32(p + 12), max(u32(p + 8), u32(p + 16)),
                         u32(p + 20), u32(p + 16)))

    def offset(rva):
        for start, extent, raw, raw_size in sections:
            if start <= rva < start + extent and rva - start < raw_size:
                return raw + rva - start
        raise ValueError(f"RVA outside raw sections: {rva:x}")

    def string(rva):
        p = offset(rva)
        end = data.index(b"\0", p)
        return data[p:end].decode("ascii", "strict")

    imports = {}
    directory = u32(opt + 104)
    if not directory:
        return base, imports
    p = offset(directory)
    for _ in range(4096):
        lookup, _, _, name, iat = struct.unpack_from("<IIIII", data, p)
        if not any((lookup, name, iat)):
            return base, imports
        dll = string(name).lower()
        for index in range(65536):
            value = u32(offset((lookup or iat) + 4 * index))
            if not value:
                break
            symbol = f"ordinal:{value & 0xffff}" if value & 0x80000000 else string(value + 2)
            imports[base + iat + 4 * index] = dll + "!" + symbol
        else:
            raise ValueError("unterminated import lookup")
        p += 20
    raise ValueError("unterminated import descriptors")


def disassemble(path, executable="llvm-objdump"):
    result = subprocess.run([executable, "-d", "--no-show-raw-insn", str(path)],
                            capture_output=True, text=True, check=True)
    insns = {}
    for line in result.stdout.splitlines():
        match = LINE.match(line)
        if match:
            insns[int(match[1], 16)] = (match[2].lower(), match[3].split("#", 1)[0].strip())
    return insns


def retail_bounds(rows):
    return {int(r["entry"], 16): {
        "ranges": [(int(b["start"], 16), int(b["end"], 16) + 1) for b in r["body_ranges"]],
        "provenance": r.get("function_provenance", "UNKNOWN"),
        "ghidra_call_edges": r.get("call_edges", []),
    } for r in rows if r.get("record") == "function"}


def compiled_bounds(rows):
    result = {}
    for r in rows:
        if r.get("record") != "function":
            continue
        sizes = {s["size"] for s in r.get("pdb_symbols", []) if s.get("size", 0) > 0}
        # Conflicting folded extents are not silently collapsed.
        if len(sizes) != 1:
            continue
        result[r["va"]] = {
            "ranges": [(r["va"], r["va"] + next(iter(sizes)))],
            "provenance": r.get("relationship", "UNKNOWN"),
            "identity_status": r.get("identity_status", "UNKNOWN"),
            "source_ids": [s["source_id"] for s in r.get("source_candidates", [])],
        }
    return result


def build_graph(bounds, instructions, imports):
    """Returns independently observed directed edges without inferred targets."""
    addresses = sorted(instructions)
    edges, outgoing, incoming = [], defaultdict(list), defaultdict(list)
    coverage = Counter()
    for start, info in sorted(bounds.items()):
        if start not in instructions:
            coverage["missing_entry_instruction"] += 1
            continue
        coverage["functions_with_entry_instruction"] += 1
        for left, right in info["ranges"]:
            selected = addresses[bisect_left(addresses, left):bisect_left(addresses, right)]
            for site in selected:
                op, arg = instructions[site]
                if op not in {"call", "calll", "jmp", "jmpl"}:
                    continue
                is_jump = op.startswith("jmp")
                edge = {"owner": start, "site": site, "instruction_kind": "jump" if is_jump else "call"}
                direct = ADDRESS.match(arg)
                indirect = ADDRESS.match(arg[1:]) if arg.startswith("*") else None
                if direct:
                    target = int(direct[1], 16)
                    if is_jump and any(a <= target < b for a, b in info["ranges"]):
                        continue
                    edge.update(target=target, kind="TAIL_JUMP_CANDIDATE" if is_jump else "DIRECT_CALL",
                                evidence_type="Machine direct operand", target_is_function_entry=target in bounds)
                    # A one-instruction JMP [IAT] thunk is separately identified.
                    thunk = instructions.get(target)
                    if thunk and thunk[0] in {"jmp", "jmpl"} and thunk[1].startswith("*"):
                        m = ADDRESS.match(thunk[1][1:])
                        if m and int(m[1], 16) in imports and "(" not in thunk[1]:
                            edge["import_via_one_hop_thunk"] = imports[int(m[1], 16)]
                elif indirect and "(" not in arg and int(indirect[1], 16) in imports:
                    edge.update(kind="IMPORT_IAT_JUMP" if is_jump else "IMPORT_IAT_CALL",
                                import_name=imports[int(indirect[1], 16)],
                                evidence_type="Machine indirect operand plus exact PE import slot")
                else:
                    edge.update(kind="UNRESOLVED_INDIRECT_JUMP" if is_jump else "UNRESOLVED_INDIRECT_CALL",
                                evidence_type="Machine indirect operand; target unknown",
                                operand_class="register_or_register_memory" if "%" in arg else "other")
                edges.append(edge)
                outgoing[start].append(edge)
                if edge.get("target_is_function_entry"):
                    incoming[edge["target"]].append(edge)
                coverage[edge["kind"]] += 1
    owners = defaultdict(set)
    for edge in edges:
        owners[edge["site"]].add(edge["owner"])
    for edge in edges:
        edge["ownership_ambiguous"] = len(owners[edge["site"]]) > 1
        if edge["ownership_ambiguous"]:
            coverage["edges_with_ambiguous_owner"] += 1
    return {"bounds": bounds, "outgoing": outgoing, "incoming": incoming,
            "coverage": dict(coverage), "edges": edges}


def fixed_anchors(controls, retail_graph, compiled_graph):
    """Calibration-only, independently reviewed, exact one-to-one anchor pool."""
    reviewed = {(r["module"].lower(), int(r["va"], 16), r["source_id"])
                for r in controls["reviewed_samples"]}
    result = []
    for r in controls["controls"]:
        key = (r["module"].lower(), int(r["va"], 16), r["source_id"])
        if r["role"] != "calibration" or key not in reviewed:
            continue
        found = [va for va, b in compiled_graph["bounds"].items()
                 if b["provenance"] == "ONE_TO_ONE" and b["identity_status"] == "PDB_SOURCE_BYTES_VERIFIED"
                 and b["source_ids"] == [r["source_id"]]]
        if key[1] in retail_graph["bounds"] and len(found) == 1:
            result.append({"retail_va": key[1], "current_va": found[0], "source_id": r["source_id"],
                           "basis": "predeclared reviewed calibration identity, not matcher output"})
    return result


def context(graph, va, anchors, side):
    anchor_map = {a[side + "_va"]: a["source_id"] for a in anchors if a[side + "_va"] != va}
    features = Counter()
    for edge in graph["outgoing"].get(va, []):
        if edge.get("ownership_ambiguous"):
            continue
        # Tail candidates and unknown indirect targets cannot become anchor hits.
        if edge["kind"] == "DIRECT_CALL" and edge.get("target") in anchor_map:
            features["direct_callee_anchor:" + anchor_map[edge["target"]]] += 1
        name = edge.get("import_name") or edge.get("import_via_one_hop_thunk")
        if name and edge["instruction_kind"] == "call":
            features["import_call:" + name] += 1
    for edge in graph["incoming"].get(va, []):
        if not edge.get("ownership_ambiguous") and edge["kind"] == "DIRECT_CALL" and edge["owner"] in anchor_map:
            features["direct_caller_anchor:" + anchor_map[edge["owner"]]] += 1
    return {"va": va, "provenance": graph["bounds"].get(va),
            "outgoing": graph["outgoing"].get(va, []), "incoming": graph["incoming"].get(va, []),
            "comparable_features": dict(sorted(features.items())),
            "absence_policy": "unknown, never rejection; inlining/folding/analysis coverage differ"}


def rva_contexts(graph, image_base):
    """Adapter for an index runner; evidence addresses become module-relative.

    Does not resolve indirect targets, score nodes, or modify an index. Function
    entry membership is explicit on callees; an outside-range jump remains only
    a tail candidate, including jumps to known function entries.
    """
    result = {}
    for va, bounds in sorted(graph["bounds"].items()):
        outgoing = [e for e in graph["outgoing"].get(va, []) if not e.get("ownership_ambiguous")]
        incoming = [e for e in graph["incoming"].get(va, []) if not e.get("ownership_ambiguous")]
        result[va - image_base] = {
            "provenance": bounds,
            "direct_callers": sorted({e["owner"] - image_base for e in incoming
                                      if e["kind"] == "DIRECT_CALL"}),
            "direct_callees": sorted({e["target"] - image_base for e in outgoing
                                      if e["kind"] == "DIRECT_CALL"}),
            "imports": dict(Counter(e.get("import_name") or e["import_via_one_hop_thunk"]
                                    for e in outgoing if e["instruction_kind"] == "call"
                                    and (e.get("import_name") or e.get("import_via_one_hop_thunk")))),
            "indirect_count": sum(e["kind"] == "UNRESOLVED_INDIRECT_CALL" for e in outgoing),
            "tail_calls": sorted({e["target"] - image_base for e in outgoing
                                  if e["kind"] == "TAIL_JUMP_CANDIDATE"}),
            "tail_policy": "CANDIDATE machine jump, not verified semantic tail call",
            "complete_callgraph": False,
        }
    return result


def compare_context(left, right):
    a, b = Counter(left["comparable_features"]), Counter(right["comparable_features"])
    shared = {key: min(a[key], b[key]) for key in a.keys() & b.keys()}
    return {"shared_features": dict(sorted(shared.items())), "shared_count": sum(shared.values()),
            "score": None if not shared else sum(shared.values()) / sum((a | b).values()),
            "decision": "CONTEXT_SUPPORT_ONLY" if shared else "INSUFFICIENT_CONTEXT",
            "negative_evidence": False, "promote": False}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ("retail-metadata", "compiled-bridge", "retail-image", "current-image", "controls", "output"):
        p.add_argument("--" + name, type=Path, required=True)
    p.add_argument("--objdump", default="llvm-objdump")
    args = p.parse_args()
    root = Path(__file__).resolve().parents[3]
    private = (root / "Evidence/RetailMap/local").resolve()
    if private not in args.output.resolve().parents:
        raise ValueError("output must be private Evidence/RetailMap/local")
    if args.output.exists():
        raise ValueError("refuse to overwrite evidence")
    rr, cr = jsonl(args.retail_metadata), jsonl(args.compiled_bridge)
    rh, ch = rr[0], cr[0]
    if digest(args.retail_image) != RETAIL or rh["image_sha256"].upper() != RETAIL:
        raise ValueError("canonical retail SHA mismatch")
    if digest(args.current_image) != ch["module_sha256"].upper():
        raise ValueError("current image/bridge SHA mismatch")
    rbase, ri = pe_imports(args.retail_image)
    cbase, ci = pe_imports(args.current_image)
    if rbase != int(rh["image_base"], 16) or cbase != ch["pe"]["image_base"]:
        raise ValueError("metadata image base mismatch")
    rg = build_graph(retail_bounds(rr), disassemble(args.retail_image, args.objdump), ri)
    cg = build_graph(compiled_bounds(cr), disassemble(args.current_image, args.objdump), ci)
    controls = json.loads(args.controls.read_text(encoding="utf-8"))
    anchors = fixed_anchors(controls, rg, cg)
    rows = []
    for item in controls["controls"] + controls["hard_negatives"]:
        if item["module"].lower() != rh["image_name"].lower():
            continue
        source = item.get("source_id", item.get("wrong_source_id"))
        matches = [va for va, b in cg["bounds"].items() if source in b.get("source_ids", [])]
        retail = context(rg, int(item["va"], 16), anchors, "retail")
        candidates = []
        for va in sorted(matches):
            current = context(cg, va, anchors, "current")
            candidates.append({"context": current, "comparison": compare_context(retail, current)})
        rows.append({"control": item, "retail_context": retail, "current_candidates": candidates})
    inputs = {name: {"path": str(getattr(args, name)), "sha256": digest(getattr(args, name))}
              for name in ("retail_metadata", "compiled_bridge", "retail_image", "current_image", "controls")}
    tool = Path(shutil.which(args.objdump) or args.objdump)
    inputs["objdump"] = {"path": str(tool), "sha256": digest(tool)}
    report = {"schema": "cfw.v3-callgraph-context.controls.v1", "version": VERSION,
              "script_sha256": digest(__file__), "inputs": inputs, "anchors": anchors,
              "retail_coverage": rg["coverage"], "current_coverage": cg["coverage"],
              "controls": rows, "promotion_count": 0,
              "limits": ["No register-dataflow inferred targets", "Tail jumps remain candidates",
                         "No holdout or recursive anchors", "No missing-edge rejection",
                         "Only original controls and hard negative pairs; no bulk ranking",
                         "Compiler inline/fold provenance retained, not treated as exact identities"]}
    with args.output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, sort_keys=True, indent=2)
        stream.write("\n")
    print(json.dumps({"output": str(args.output), "sha256": digest(args.output),
                      "anchors": len(anchors), "control_pairs": len(rows), "promotion_count": 0}))


if __name__ == "__main__":
    main()
