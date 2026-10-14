#!/usr/bin/env python3
"""Capture bounded Menu_tb ownership on each side without matching functions.

Retail membership starts from observed table stores and a callback slot; current
membership starts from compiler symbols plus MAP/COFF source-byte verification.
Graph neighborhoods are supporting context, never proof of class ownership.
Outputs are private evidence, not correspondence decisions or matcher inputs.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import subprocess
import sys

import v4_graph_context as graph_tool


VERSION = "v4-menu-ownership-capture-7"
SCHEMA = "cfw.v4.menu-ownership-capture.v7"
ROOT = Path(__file__).resolve().parents[3]
PRIVATE = ROOT / "Evidence/RetailMap/local"


def sha(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest().upper()


def read(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def number(value: str | int) -> int:
    return int(value, 0) if isinstance(value, str) and value.startswith("0x") else int(value, 16) if isinstance(value, str) else value


def inputs(paths: dict[str, Path]) -> dict:
    return {name: {"path": str(path.resolve()), "sha256": sha(path)}
            for name, path in sorted(paths.items())}


def table_lookup(module: dict) -> dict[int, dict]:
    return {number(row["table_va"]): row for row in module["tables"]}


def check_observation(obs: dict, module: dict, side: str) -> None:
    expected = next(row["sha256"] for row in obs["inputs"]
                    if row["path"].lower().endswith("trim.dll") and
                    ("RetailRef" in row["path"]) == (side == "retail"))
    if module["module_sha256"] != expected:
        raise ValueError(f"{side} machine observation is from another binary")


def check_observation_inputs(obs: dict) -> None:
    for row in obs["inputs"]:
        path = Path(row["path"])
        if not path.is_absolute():
            path = ROOT / path
        if not path.is_file() or sha(path) != row["sha256"]:
            raise ValueError(f"machine observation dependency missing or changed: {path}")


def check_consumed_observation_input(obs: dict, path: Path) -> None:
    expected = {}
    for row in obs["inputs"]:
        candidate = Path(row["path"])
        if not candidate.is_absolute():
            candidate = ROOT / candidate
        expected[str(candidate.resolve()).lower()] = row["sha256"]
    identity = str(path.resolve()).lower()
    if identity not in expected or sha(path) != expected[identity]:
        raise ValueError(f"consumed input is not the machine-reviewed dependency: {path}")


def graph_facts(saved: dict) -> tuple[dict, dict]:
    graph = graph_tool.restore_graph(saved)
    adjacency, imports = graph_tool.adjacency(graph)
    return adjacency, imports


def neighbors(adjacency: dict, roots: set[int]) -> tuple[set[int], set[int], set[int]]:
    first = set().union(*(adjacency[va]["out"] | adjacency[va]["in"] for va in roots))
    second = set().union(*(adjacency[va]["out"] | adjacency[va]["in"] for va in first))
    return first - roots, second - roots - first, set().union(*(adjacency[va]["out"] for va in roots))


def retail_capture(obs: dict, module: dict, graph_module: dict, metadata: Path) -> dict:
    tables = table_lookup(module)
    constructor = obs["retail_menu_class_constructor"]
    allocation = obs["retail_allocation_and_publication"]
    retail_graph = graph_module["retail_graph"]
    adjacency, imports = graph_facts(retail_graph)
    callback = number(obs["retail_anchor_to_virtual_dispatch"]["sole_saved_direct_caller_va"])
    load_interface = number(obs["retail_anchor_to_virtual_dispatch"]["reviewed_anchor_va"])
    table_stores = constructor["class_family_table_stores"] + allocation["allocated_wrapper_table_stores"]
    for store in table_stores:
        table = tables.get(number(store["table"]))
        if not table or not table["structural_object_stores"]:
            raise ValueError(f"retail constructor table lacks independently scanned store: {store['table']}")
    for occurrence in obs["retail_anchor_to_virtual_dispatch"]["exact_pointer_occurrences"]:
        start = occurrence.get("constructor_installed_table_start") or occurrence["allocation_installed_table_start"]
        table = tables[number(start)]
        slot = occurrence["observed_zero_based_slot"]
        if number(table["slots"][slot]["target_va"]) != callback:
            raise ValueError("retail callback slot identity changed")
    metadata_rows = [json.loads(line) for line in metadata.read_text(encoding="utf-8").splitlines() if line]
    if (not metadata_rows or metadata_rows[0].get("record") != "header"
            or metadata_rows[0].get("image_sha256", "").upper() != module["module_sha256"]
            or metadata_rows[-1].get("record") != "footer"
            or not metadata_rows[-1].get("complete")):
        raise ValueError("retail metadata header/footer does not identify the complete retail image")
    saved = {int(row["entry"], 16): row for row in metadata_rows
             if row.get("record") == "function"}
    if callback not in saved or load_interface not in saved:
        raise ValueError("retail callback or reviewed callee lacks saved Function")
    constructor_va = number(constructor["entry"])
    destructor_va = number(obs["retail_teardown_corrobation"]["entry"])
    roots = {callback, constructor_va, destructor_va}
    one, two, _ = neighbors(adjacency, roots)
    callback_callees = set(adjacency[callback]["out"])
    addresses = roots | one | two | {load_interface}
    rows = []
    for va in sorted(addresses):
        source = saved.get(va)
        flags = []
        if va == callback:
            flags.append("CONSTRUCTOR_INSTALLED_TABLE_SLOT")
        if va == constructor_va:
            flags.append("CONSTRUCTOR")
        if va == destructor_va:
            flags.append("DESTRUCTOR_CANDIDATE")
        if va == load_interface:
            flags.append("REVIEWED_ANCHOR_CALLEE")
        if va in callback_callees:
            flags.append("ONE_HOP_CALLBACK_CALLEE")
        if va in one:
            flags.append("ONE_HOP_FAMILY_GRAPH")
        elif va in two:
            flags.append("TWO_HOP_FAMILY_GRAPH")
        if not source:
            flags.append("NO_SAVED_FUNCTION_BOUNDARY")
        refs = source.get("data_references", []) if source else []
        rows.append({
            "va": f"0x{va:08X}", "rva": f"0x{va - graph_module['retail_image_base']:X}",
            "flags": flags, "saved_function": bool(source),
            "size": source.get("body_size") if source else None,
            "constant_keys": sorted(source.get("constants", {})) if source else [],
            "string_hashes": sorted({ref.get("sha256") or ref.get("hash")
                                     for ref in refs if ref.get("kind") == "string_reference"
                                     and (ref.get("sha256") or ref.get("hash"))}),
            "string_reference_count": sum(ref.get("kind") == "string_reference" for ref in refs),
            "data_references": refs,
            "callers": [f"0x{x:08X}" for x in sorted(adjacency[va]["in"])],
            "callees": [f"0x{x:08X}" for x in sorted(adjacency[va]["out"])],
            "imports": sorted(imports[va]),
            "ownership_warning": "Callgraph proximity does not establish the receiver or class of this function."
                                 if va not in roots else None,
        })
    return {
        "module_sha256": module["module_sha256"], "module": "retail Trim.dll",
        "image_base": graph_module["retail_image_base"],
        "roots": {"callback": f"0x{callback:08X}", "constructor": f"0x{constructor_va:08X}",
                  "destructor_candidate": f"0x{destructor_va:08X}",
                  "reviewed_callee": f"0x{load_interface:08X}"},
        "table_family": [{"table_va": store["table"], "object_offset": store["object_offset"],
                          "store_instruction": store["instruction"]}
                         for store in table_stores],
        "validated_callback_slot": obs["retail_anchor_to_virtual_dispatch"]["exact_pointer_occurrences"],
        "rows": rows,
        "limits": ["Only the observed callback slot has exact table-member identity here.",
                   "Maximal executable-pointer runs do not establish declared vtable lengths.",
                   "A direct callee or caller may be a generic helper, not a class member.",
                   "String/data references are saved-analysis metadata, not named semantic fields."],
    }


def current_capture(obs: dict, module: dict, graph_module: dict, domain: dict) -> dict:
    tables = table_lookup(module)
    current_graph = graph_module["current_graph"]
    adjacency, imports = graph_facts(current_graph)
    named = obs["current_named_ownership"]
    family = named["class_family"] + named["wrapper_family"]
    for store in family:
        table = tables.get(number(store["table"]))
        if not table or not table["compiler_symbols"]:
            raise ValueError(f"current class-family table lacks compiler identity: {store['table']}")
    bridge = domain["current_function_bridge"]
    by_va = defaultdict(list)
    for row in bridge:
        by_va[number(row["va"])].append(row)
    direct = {va for va, entries in by_va.items()
              if any(row["display_name"].startswith("Menu_tb::")
                     and "::" not in row["display_name"][len("Menu_tb::"):]
                     and row["identity_status"] == "MAP_COFF_SOURCE_BYTES_VERIFIED"
                     and row["owner"] == "Menu_Toolbar.obj" for row in entries)}
    static = {va for va, entries in by_va.items()
              if any(row["display_name"] in ("Menu_tb::operator new", "Menu_tb::operator delete",
                                                "Menu_tb::_GetEntriesIn")
                     and row["identity_status"] == "MAP_COFF_SOURCE_BYTES_VERIFIED"
                     for row in entries)}
    nested = {va for va, entries in by_va.items()
              if any(row["display_name"].startswith("Menu_tb::")
                     and "::" in row["display_name"][len("Menu_tb::"):]
                     and row["identity_status"] == "MAP_COFF_SOURCE_BYTES_VERIFIED"
                     and row["owner"] == "Menu_Toolbar.obj" for row in entries)}
    wrapper = {va for va, entries in by_va.items()
               if any("DAComponent<Menu_tb>" in row["display_name"]
                      and row["identity_status"] == "MAP_COFF_SOURCE_BYTES_VERIFIED"
                      for row in entries)}
    owned = {va for va, entries in by_va.items()
             if any(row["owner"] == "Menu_Toolbar.obj" for row in entries)}
    if not direct or number(named["callback_anchor_link"]["entry"]) not in direct:
        raise ValueError("current compiled Menu_tb callback not verified")
    one, two, _ = neighbors(adjacency, direct)
    addresses = direct | nested | wrapper | owned | one | two
    vtable_members = defaultdict(list)
    for store in family:
        table = tables[number(store["table"])]
        for slot in table["slots"]:
            # Compiler symbols prove table identity; the scanner's maximal
            # pointer run is not a declared interface length. Treat slots as
            # candidates unless they have compiler-named method identity.
            if slot["current_symbols"]:
                vtable_members[number(slot["target_va"])].append({
                    "table_va": store["table"], "slot": slot["index"],
                    "symbols": slot["current_symbols"]})
    addresses |= set(vtable_members)
    rows = []
    for va in sorted(addresses):
        entries = by_va[va]
        flags = []
        if va in direct:
            flags.append("DIRECT_MEMBER")
        if va in static:
            flags.append("STATIC_CLASS_FUNCTION")
        if any(row["display_name"] in ("Menu_tb::Menu_tb", "Menu_tb::~Menu_tb")
               for row in entries):
            flags.append("CONSTRUCTOR_DESTRUCTOR")
        if va in nested:
            flags.append("NESTED_CLASS_MEMBER")
        if va in wrapper:
            flags.append("WRAPPER_MEMBER")
        if va in vtable_members:
            flags.append("COMPILER_NAMED_TABLE_SLOT_CANDIDATE")
        if va in owned and va not in direct and va not in nested and va not in wrapper:
            flags.append("OWNED_OBJECT_FILE_HELPER")
        if va in one and va not in direct:
            flags.append("ONE_HOP_CLASS_GRAPH")
        elif va in two and va not in direct:
            flags.append("TWO_HOP_CLASS_GRAPH")
        rows.append({
            "va": f"0x{va:08X}", "rva": f"0x{va - graph_module['current_image_base']:X}",
            "flags": flags,
            "compiled": [{"decorated": row.get("linkage_name"), "name": row["display_name"],
                          "owner": row["owner"], "size": row["size"],
                          "identity_status": row["identity_status"],
                          "source_files": sorted({ref["source_path"] for ref in
                                                  row["primary_source_checksums"]
                                                  if ref["matches_current_bytes"]})}
                         for row in entries],
            "vtable_memberships": vtable_members[va],
            "callers": [f"0x{x:08X}" for x in sorted(adjacency[va]["in"])],
            "callees": [f"0x{x:08X}" for x in sorted(adjacency[va]["out"])],
            "imports": sorted(imports[va]),
            "strings_data_status": "NOT_INDEXED_FOR_GENERAL_CURRENT_TRIM_FUNCTIONS",
            "ownership_warning": "Object-file or graph proximity is not class ownership."
                                 if va not in direct and va not in wrapper else None,
        })
    return {
        "module_sha256": module["module_sha256"], "module": "current Debug Trim.dll",
        "image_base": graph_module["current_image_base"],
        "configuration": "Debug",
        "compiler_named_family_tables": family,
        "rows": rows,
        "limits": ["MAP/COFF source-byte verification is current machine/source identity, not retail correspondence.",
                   "Class-qualified decorated names distinguish direct members from same-object-file helpers.",
                   "Nested createCallback methods and static allocation/interface-map functions are not outer-instance receivers.",
                   "Compiler-named table symbols identify table starts; scanned maximal runs do not prove slot extent.",
                   "General current Trim string/data references are not available from this ownership index."],
    }


def capture(args: argparse.Namespace) -> dict:
    paths = {name: getattr(args, name) for name in
             ("domain", "graph", "vtable", "metadata", "observation")}
    domain, graph, vtable, obs = (read(paths[name]) for name in
                                  ("domain", "graph", "vtable", "observation"))
    if (domain.get("schema") != "cfw.v4.trim-domain-provenance.v1"
            or graph.get("status") != "INDEPENDENT_METADATA_CAPTURE_NO_GROUND_TRUTH"
            or obs.get("schema") != "cfw.v4.trim-toolbar-family-observations.v1"):
        raise ValueError("unexpected input schema/status")
    retail_module = next(m for m in vtable["modules"] if m["label"] == "retail_trim")
    current_module = next(m for m in vtable["modules"] if m["label"] == "current_debug_trim")
    graph_module = next(m for m in graph["modules"]
                        if m["retail_module_sha256"] == retail_module["module_sha256"])
    if (graph_module["current_module_sha256"] != current_module["module_sha256"]
            or domain["retail_contract"]["module_sha256"] != retail_module["module_sha256"]
            or domain["current_contract"]["module_sha256"] != current_module["module_sha256"]):
        raise ValueError("retail/current module identity disagreement")
    check_observation(obs, retail_module, "retail")
    check_observation(obs, current_module, "current")
    check_observation_inputs(obs)
    for name in ("domain", "vtable", "metadata"):
        check_consumed_observation_input(obs, paths[name])
    head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    return {
        "schema": SCHEMA, "tool_version": VERSION,
        "tool_sha256": sha(Path(__file__)),
        "python_version": sys.version.split()[0],
        "graph_context_dependency_sha256": sha(Path(graph_tool.__file__)),
        "commit": head, "mode": "INDEPENDENT_SIDE_CAPTURE",
        "configuration": {"graph_hops": 2, "no_matcher_rankings": True,
                          "no_pilot_truth": True, "no_hard_domain": True},
        "inputs": inputs(paths),
        "retail": retail_capture(obs, retail_module, graph_module, paths["metadata"]),
        "current": current_capture(obs, current_module, graph_module, domain),
        "status": "COMPLETE_STAGED_UNREVIEWED",
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("domain", "graph", "vtable", "metadata", "observation", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() or PRIVATE.resolve() not in output.parents:
        parser.error("output must be a new private evidence file")
    result = capture(args)
    with output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(result, stream, sort_keys=True, separators=(",", ":"))
        stream.write("\n")
    print(json.dumps({"output": str(output), "sha256": sha(output),
                      "retail_rows": len(result["retail"]["rows"]),
                      "current_rows": len(result["current"]["rows"])}, sort_keys=True))


if __name__ == "__main__":
    main()
