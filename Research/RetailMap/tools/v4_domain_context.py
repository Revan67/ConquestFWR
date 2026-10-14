#!/usr/bin/env python3
"""Independent domain-support capture; no identity selection or promotion.

Capture reads whole retail/current modules, never a pilot inventory or labels.
Evaluation is a separate posthoc operation over an immutable capture. SOFT
shortlists always retain the complete machine candidate universe as an escape.
"""
from __future__ import annotations

import argparse
from bisect import bisect_left
from collections import Counter, defaultdict
import json
from pathlib import Path
import re

import pefile
import v3_callgraph_context as machine
from v3_trim_factory_context import extract_string_hashes
from v4_graph_context import map_coff_bounds

ROOT = Path(__file__).resolve().parents[3]
PRIVATE = ROOT / "Evidence/RetailMap/local"
VERSION = "v4-independent-domain-1"

POLICY = {
    "HARD": "Exact named export or separately established constructor/destructor this-pointer ownership establishes a bounded role, not arbitrary interior identity. No HARD exclusion implemented.",
    "SOFT": "Named UI API use, rare shared literal hashes and verified current source-object ownership support a review shortlist; never semantic equivalence.",
    "UNKNOWN": "Missing extraction, constants without dataflow role, pointer-table membership without ownership, module continuity alone, and absent support remain unknown.",
    "event_dispatch": "Requires event parameter dataflow into dispatch and independent enum meaning. Numeric comparisons alone remain UNKNOWN.",
    "resource_constants": "Requires resolved resource namespace and consumer argument/dataflow. Coincident immediate values remain UNKNOWN.",
    "pointer_class": "Requires constructor/destructor this-pointer store plus independently named class/interface. Consecutive pointer arrays do not establish class or slot continuity.",
    "escape": "Full original candidate universe is always retained; SOFT pool cannot reject candidates or force a unique winner. Empty support returns the full pool.",
    "confidence": "No confidence, scoring, Tier1 anchors, recursive propagation or correspondences are emitted.",
}


def ui_import(name):
    """API names supply functional meaning independently of pilot identities."""
    dll, _, symbol = name.lower().partition("!")
    return dll in {"user32.dll", "gdi32.dll"} and bool(re.search(
        r"cursor|mouse|window|focus|capture|keyboard|key(state|board)|drawtext|textout|font|message|paint", symbol))


def current_owner(rows):
    verified = [r for r in rows if r["identity_status"] == "MAP_COFF_SOURCE_BYTES_VERIFIED"]
    paths = sorted({s["source_path"].replace("\\", "/") for r in verified
                    for s in r["primary_source_checksums"] if s["matches_current_bytes"]})
    # Naming is only a SOFT source-domain hint, not independent retail evidence.
    ui = any(re.match(r"(?:Menu_|Ctl|Control|HotRect|BaseHotRect|FullScreen)", Path(p).stem, re.I) for p in paths)
    return {"source_bytes_verified": len(verified) == len(rows), "source_paths": paths,
            "object_owners": sorted({r["owner"] for r in rows}),
            "domain": "UI_SOURCE_OWNER_HINT" if ui else "UNKNOWN", "strength": "SOFT" if ui else "UNKNOWN"}


def function_contexts(path, bounds, owners=None):
    base, imports = machine.pe_imports(path)
    instructions = machine.disassemble(path)
    addresses = sorted(instructions)
    graph = machine.build_graph(bounds, instructions, imports)
    image = pefile.PE(str(path))
    # A literal in overlapping body extents cannot be assigned to one owner.
    # Do not let it create candidate support for either ambiguous function.
    ambiguous = set()
    active = []
    intervals = sorted((left, right, va) for va, bound in bounds.items()
                       for left, right in bound["ranges"])
    for left, right, va in intervals:
        active = [(stop, owner) for stop, owner in active if stop > left]
        for _, owner in active:
            if owner != va:
                ambiguous.update((owner, va))
        active.append((right, va))
    rows = []
    for va, bound in sorted(bounds.items()):
        subset = []
        for left, right in bound["ranges"]:
            subset.extend((a, *instructions[a]) for a in addresses[bisect_left(addresses, left):bisect_left(addresses, right)])
        hashes = set()
        if va not in ambiguous:
            for left, right in bound["ranges"]:
                hashes.update(extract_string_hashes(image, left, right-left, subset))
        names = sorted({e.get("import_name") or e.get("import_via_one_hop_thunk")
                        for e in graph["outgoing"].get(va, [])
                        if not e.get("ownership_ambiguous") and e["instruction_kind"] == "call"
                        and (e.get("import_name") or e.get("import_via_one_hop_thunk"))})
        row = {"va": va, "rva": va-base, "boundary_provenance": bound["provenance"],
               "entry_instruction_available": va in instructions, "imports": names,
               "ui_imports": [n for n in names if ui_import(n)], "string_hashes": sorted(hashes),
               "string_attribution": "AMBIGUOUS_NOT_SCORED" if va in ambiguous else "UNAMBIGUOUS_BOUNDARY",
               "unresolved_indirect_calls": sum(e["kind"] == "UNRESOLVED_INDIRECT_CALL" for e in graph["outgoing"].get(va, [])),
               "event_dispatch": "UNKNOWN", "resource_constants": "UNKNOWN", "pointer_class_ownership": "UNKNOWN"}
        if owners is not None:
            row["current_owner"] = current_owner(owners[va])
        rows.append(row)
    image.close()
    return rows


def capture(retail, current, metadata, index_path):
    meta = machine.jsonl(metadata)
    index = json.loads(index_path.read_text(encoding="utf-8"))
    if not meta[-1].get("complete") or machine.digest(retail) != meta[0]["image_sha256"].upper():
        raise ValueError("Retail identity/footer mismatch")
    if machine.digest(current) != index["current_contract"]["module_sha256"].upper():
        raise ValueError("Current image/MAP COFF mismatch")
    owners = defaultdict(list)
    for row in index["current_function_bridge"]:
        owners[int(row["va"], 16)].append(row)
    rb, cb = machine.retail_bounds(meta), map_coff_bounds(index)
    left, right = function_contexts(retail, rb), function_contexts(current, cb, owners)
    rdf = Counter(h for f in left for h in f["string_hashes"])
    cdf = Counter(h for f in right for h in f["string_hashes"])
    # Fixed corpus-relative rarity, no pilot success optimization.
    rare = {h for h in rdf.keys() & cdf.keys() if rdf[h]/len(left) <= .02 and cdf[h]/len(right) <= .02}
    inverted = defaultdict(set)
    for f in right:
        for token in ["import:"+x for x in f["ui_imports"]] + ["string:"+x for x in f["string_hashes"] if x in rare]:
            inverted[token].add(f["rva"])
    ui_pool = {f["rva"] for f in right if f["current_owner"]["domain"] == "UI_SOURCE_OWNER_HINT"}
    for f in left:
        tokens = ["import:"+x for x in f["ui_imports"]] + ["string:"+x for x in f["string_hashes"] if x in rare]
        support = defaultdict(list)
        for t in tokens:
            for rva in inverted[t]:
                support[rva].append(t)
        f["candidate_support"] = [{"current_rva": rva, "strength": "SOFT", "shared_tokens": sorted(ts)} for rva, ts in sorted(support.items())]
        ui_active = bool(f["ui_imports"] or (set(support) & ui_pool))
        shortlist = ui_pool | set(support) if ui_active else set(support)
        f["soft_pool"] = sorted(shortlist) if shortlist else [c["rva"] for c in right]
        f["domain_strength"] = "SOFT" if shortlist else "UNKNOWN"
        f["ui_domain_supported"] = ui_active
        f["full_pool_escape_required"] = True
    return {"schema": "cfw.v4-independent-domain.v1", "version": VERSION, "policy": POLICY,
            "inputs": {k: {"path": str(p), "sha256": machine.digest(p)} for k,p in
                       [("retail_image",retail),("current_image",current),("retail_metadata",metadata),("current_map_coff_index",index_path)]},
            "tool_sha256": machine.digest(__file__), "retail_functions": left, "current_functions": right,
            "candidate_universe_rvas": [f["rva"] for f in right], "rare_shared_hash_count": len(rare),
            "module_continuity_strength": "SOFT", "hard_exclusions": 0, "promotion_count": 0,
            "limitations": ["MAP/COFF extents cover only a subset of body matcher candidates; unbridged bodies must remain eligible in any integration.",
                            "Literal hashes lack argument/dataflow roles; shared support can be generic or source path literals.",
                            "UI source filename rule is a current-side SOFT hint only; it cannot establish retail class ownership.",
                            "Exported destructor ownership is not propagated to unowned interior functions.",
                            "No pilot selection, truth, old rank or current true RVA is read by capture."]}


def evaluate(data, selection):
    """Selection addresses only, after capture; no true current identities."""
    selected = {int(a, 16) for a, _ in selection["sample"]["Trim/UI"]}
    rows = [f for f in data["retail_functions"] if f["va"] in selected]
    return {"schema": "cfw.v4-domain-development-pools.v1", "development_only": True,
            "identity_labels_read": False, "row_count": len(rows),
            "full_map_coff_pool": len(data["candidate_universe_rvas"]),
            "support_activated_rows": sum(bool(f["candidate_support"]) for f in rows),
            "soft_domain_rows": sum(f["domain_strength"] == "SOFT" for f in rows),
            "hard_exclusions": 0, "promotion_count": 0,
            "rows": [{"retail_va": f["va"], "soft_pool_size": len(f["soft_pool"]),
                      "candidate_specific_support_count": len(f["candidate_support"]),
                      "ui_domain_supported": f["ui_domain_supported"], "strength": f["domain_strength"],
                      "full_pool_escape_required": True} for f in rows]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="mode", required=True)
    c = sub.add_parser("capture")
    for name in ("retail", "current", "metadata", "index", "output"):
        c.add_argument("--"+name, required=True, type=Path)
    e = sub.add_parser("evaluate")
    for name in ("capture", "selection", "output"):
        e.add_argument("--"+name, required=True, type=Path)
    args = parser.parse_args()
    if args.output.exists() or PRIVATE.resolve() not in args.output.resolve().parents:
        raise ValueError("Output must be a new private evidence file")
    if args.mode == "capture":
        data = capture(args.retail, args.current, args.metadata, args.index)
    else:
        data = evaluate(json.loads(args.capture.read_text(encoding="utf-8")), json.loads(args.selection.read_text(encoding="utf-8")))
        data["inputs_sha256"] = {"capture": machine.digest(args.capture), "selection": machine.digest(args.selection)}
    with args.output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(data, stream, sort_keys=True, separators=(",", ":")); stream.write("\n")
    print(json.dumps({"output": str(args.output), "sha256": machine.digest(args.output),
                      "summary": {k:v for k,v in data.items() if k in {"row_count","full_map_coff_pool","support_activated_rows","soft_domain_rows","rare_shared_hash_count","hard_exclusions"}}}))


if __name__ == "__main__":
    main()
