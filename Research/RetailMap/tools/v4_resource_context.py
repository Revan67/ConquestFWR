#!/usr/bin/env python3
"""Read-only x86 PE function context: imports, verified literals, data and API resource paths.

No numeric constant is called a resource without a local call-site argument path.
No address-normalized token or candidate identity is promoted to a correspondence.
Outputs contain string hashes, not copied retail strings or code.
"""
from __future__ import annotations

import argparse
from bisect import bisect_left
from collections import Counter, defaultdict
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import subprocess
import sys

import pefile
import v3_callgraph_context as machine
import v4_graph_context as graph_tool


VERSION = "v4-resource-context-2"
SCHEMA = "cfw.v4.resource-context.v2"
ROOT = Path(__file__).resolve().parents[3]
PRIVATE = ROOT / "Evidence/RetailMap/local"
ADDRESS = re.compile(r"(?<![\w])0x[0-9a-fA-F]+")
PUSH_IMMEDIATE = re.compile(r"^\$0x([0-9a-fA-F]+)$")
RESOURCE_APIS = {
    "loadstringa": ("STRING_ID", 1), "loadstringw": ("STRING_ID", 1),
    "_localloadstring": ("STRING_ID", 0), "_localloadstringw": ("STRING_ID", 0),
    "loadmenua": ("MENU_ID", 1), "loadmenuw": ("MENU_ID", 1),
    "loadicona": ("ICON_ID", 1), "loadiconw": ("ICON_ID", 1),
    "loadbitmapa": ("BITMAP_ID", 1), "loadbitmapw": ("BITMAP_ID", 1),
    "findresourcea": ("RESOURCE_NAME_ID", 1), "findresourcew": ("RESOURCE_NAME_ID", 1),
    "findresourceexa": ("RESOURCE_NAME_ID", 2), "findresourceexw": ("RESOURCE_NAME_ID", 2),
    "dialogboxparama": ("DIALOG_ID", 1), "dialogboxparamw": ("DIALOG_ID", 1),
    "createdialogparama": ("DIALOG_ID", 1), "createdialogparamw": ("DIALOG_ID", 1),
}


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest().upper()


def read(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def api_role(name: str) -> tuple[str, int] | None:
    symbol = name.rsplit("!", 1)[-1].lower()
    for api, role in RESOURCE_APIS.items():
        if api in symbol:
            return role
    return None


def resource_catalog(pe: pefile.PE) -> list[dict]:
    out = []
    if not hasattr(pe, "DIRECTORY_ENTRY_RESOURCE"):
        return out
    for kind in pe.DIRECTORY_ENTRY_RESOURCE.entries:
        if not hasattr(kind, "directory"):
            continue
        for item in kind.directory.entries:
            if not hasattr(item, "directory"):
                continue
            for lang in item.directory.entries:
                if not hasattr(lang, "data"):
                    continue
                out.append({"type_id": kind.id, "type_name": str(kind.name) if kind.name else None,
                            "item_id": item.id, "item_name_hash": hashlib.sha256(str(item.name).encode()).hexdigest().upper() if item.name else None,
                            "language": lang.id, "size": lang.data.struct.Size})
    return sorted(out, key=lambda x: (x["type_id"] or -1, x["item_id"] or -1, x["language"] or -1))


def sections(pe: pefile.PE) -> list[dict]:
    base = pe.OPTIONAL_HEADER.ImageBase
    return [{"name": section.Name.rstrip(b"\0").decode("ascii", "replace"),
             "begin": base + section.VirtualAddress,
             "end": base + section.VirtualAddress + max(section.Misc_VirtualSize, section.SizeOfRawData),
             "writable": bool(section.Characteristics & 0x80000000),
             "executable": bool(section.Characteristics & 0x20000000)}
            for section in pe.sections]


def decode_string(pe: pefile.PE, va: int, section: dict) -> dict | None:
    if section["executable"] or section["name"] == ".rsrc":
        return None
    available = min(512, section["end"] - va)
    if available < 5:
        return None
    try:
        raw = pe.get_data(va - pe.OPTIONAL_HEADER.ImageBase, available)
    except (ValueError, pefile.PEFormatError):
        return None
    if not raw:
        return None
    # Both decoders require an in-section terminator and at least four visible
    # characters. This deliberately misses some real strings rather than
    # interpreting arbitrary constant/data bytes as text.
    for encoding, step in (("ASCII", 1), ("UTF16LE", 2)):
        chars = []
        for at in range(0, len(raw) - step + 1, step):
            value = raw[at:at + step]
            if value == b"\0" * step:
                if len(chars) < 4:
                    break
                if encoding == "ASCII":
                    decoded = bytes(chars).decode("ascii")
                else:
                    decoded = bytes(chars).decode("ascii")
                return {"type": encoding, "characters": len(chars),
                        "sha256_utf8": hashlib.sha256(decoded.encode("utf-8")).hexdigest().upper()}
            if (encoding == "UTF16LE" and (len(value) != 2 or value[1] != 0)) or value[0] not in range(32, 127):
                break
            chars.append(value[0])
    return None


def argument_id(site: int, addresses: list[int], instructions: dict[int, tuple[str, str]], index: int) -> int | None:
    position = bisect_left(addresses, site)
    pushes = []
    for prior in reversed(addresses[max(0, position - 18):position]):
        op, operand = instructions[prior]
        if op.startswith(("call", "ret", "jmp", "j", "loop")):
            break
        if op.startswith("push"):
            match = PUSH_IMMEDIATE.match(operand)
            pushes.append(int(match[1], 16) if match else None)
        if len(pushes) > index:
            break
    value = pushes[index] if len(pushes) > index else None
    return value if value is not None and 0 < value <= 0xFFFF else None


def index_image(saved: dict, side: str, image: Path, objdump: Path) -> dict:
    graph = graph_tool.restore_graph(saved[side + "_graph"])
    pe = pefile.PE(str(image), fast_load=False)
    if pe.FILE_HEADER.Machine != 0x14C or pe.OPTIONAL_HEADER.Magic != 0x10B:
        raise ValueError("resource context requires x86 PE32")
    if pe.OPTIONAL_HEADER.ImageBase != saved[side + "_image_base"]:
        raise ValueError("PE image base differs from saved graph")
    expected = saved[side + "_module_sha256"]
    if digest(image) != expected:
        raise ValueError("PE binary differs from graph capture")
    instructions = machine.disassemble(image, str(objdump))
    addresses = sorted(instructions)
    owners = defaultdict(set)
    function_sites = {}
    for va, bound in graph["bounds"].items():
        sites = []
        for begin, end in bound["ranges"]:
            sites.extend(addresses[bisect_left(addresses, begin):bisect_left(addresses, end)])
        function_sites[va] = sorted(set(sites))
        for site in sites:
            owners[site].add(va)
    section_list = sections(pe)
    def data_section(va: int) -> dict | None:
        return next((s for s in section_list if s["begin"] <= va < s["end"] and not s["executable"]), None)
    edges_by_owner = defaultdict(list)
    thunk_edges = set()
    for edge in graph["edges"]:
        if not edge.get("ownership_ambiguous") and edge.get("instruction_kind") == "call":
            imported = edge.get("import_name") or edge.get("import_via_one_hop_thunk")
            if imported:
                edges_by_owner[edge["owner"]].append((edge["site"], imported.lower()))
                if edge.get("import_via_one_hop_thunk"):
                    thunk_edges.add((edge["owner"], edge["site"]))
    rows = []
    literal_cache = {}
    for va in sorted(graph["bounds"]):
        site_set = function_sites[va]
        imports = [{"site_rva": site - pe.OPTIONAL_HEADER.ImageBase, "name": name,
                    "via_thunk": (va, site) in thunk_edges}
                   for site, name in sorted(edges_by_owner[va])]
        data = []
        strings = []
        for site in site_set:
            if len(owners[site]) != 1:
                continue
            op, operand = instructions[site]
            if op.startswith(("call", "ret", "jmp", "j", "loop")):
                continue
            for token in set(ADDRESS.findall(operand)):
                target = int(token, 16)
                section = data_section(target)
                if not section:
                    continue
                data.append({"site_rva": site - pe.OPTIONAL_HEADER.ImageBase,
                             "target_rva": target - pe.OPTIONAL_HEADER.ImageBase,
                             "section": section["name"], "writable": section["writable"],
                             "operand_form": "IMMEDIATE" if "$" + token in operand else "ABSOLUTE_ADDRESS",
                             "width": 1 if op.endswith("b") else 2 if op.endswith("w") else 4 if op.endswith("l") else None})
                if target not in literal_cache:
                    literal_cache[target] = decode_string(pe, target, section)
                literal = literal_cache[target]
                if literal:
                    strings.append({"site_rva": site - pe.OPTIONAL_HEADER.ImageBase,
                                    "target_rva": target - pe.OPTIONAL_HEADER.ImageBase, **literal})
        resource_ids = []
        for site, name in sorted(edges_by_owner[va]):
            role = api_role(name)
            if not role or site not in site_set or len(owners[site]) != 1:
                continue
            resource_id = argument_id(site, site_set, instructions, role[1])
            if resource_id is not None:
                resource_ids.append({"site_rva": site - pe.OPTIONAL_HEADER.ImageBase,
                                     "api": name, "kind": role[0], "id": resource_id,
                                     "evidence": "API_CALL_WITH_LOCAL_IMMEDIATE_PUSH_ARGUMENT_CANDIDATE"})
        rows.append({"rva": va - pe.OPTIONAL_HEADER.ImageBase,
                     "extent_provenance": graph["bounds"][va].get("provenance"),
                     "imports": imports, "resource_id_candidates": resource_ids,
                     "strings": sorted({(x["site_rva"], x["target_rva"], x["sha256_utf8"]): x
                                        for x in strings}.values(), key=lambda x: (x["site_rva"], x["target_rva"])),
                     "data_references": sorted({(x["site_rva"], x["target_rva"]): x
                                               for x in data}.values(), key=lambda x: (x["site_rva"], x["target_rva"]))})
    catalog = resource_catalog(pe)
    pe.close()
    return {"side": side, "module_sha256": expected, "image_base": saved[side + "_image_base"],
            "architecture": "x86 PE32", "function_extent_count": len(graph["bounds"]),
            "resource_catalog": catalog, "functions": rows,
            "limits": ["Missing references are unknown because function extents/disassembly may be incomplete.",
                       "A verified literal address is a machine data reference, not a semantic resource name.",
                       "Resource IDs are API-argument candidates, not certified IDs without control review.",
                       "Global names and switch ownership require separate symbol/dataflow evidence."]}


def add_rarity(modules: list[dict]) -> None:
    corpus = Counter()
    corpus_functions = sum(len(module["functions"]) for module in modules)
    local = {}
    for module in modules:
        frequencies = Counter()
        for row in module["functions"]:
            tokens = {("import", x["name"]) for x in row["imports"]}
            tokens |= {("string", x["sha256_utf8"]) for x in row["strings"]}
            tokens |= {("resource", x["kind"], x["id"]) for x in row["resource_id_candidates"]}
            frequencies.update(tokens)
            corpus.update(tokens)
        local[module["side"], module["module_sha256"]] = frequencies
    for module in modules:
        frequency = local[module["side"], module["module_sha256"]]
        for row in module["functions"]:
            for group, kind in (("imports", "import"), ("strings", "string"),
                                ("resource_id_candidates", "resource")):
                for x in row[group]:
                    token = (kind, x["name"]) if kind == "import" else ((kind, x["sha256_utf8"])
                            if kind == "string" else (kind, x["kind"], x["id"]))
                    x["module_function_frequency"] = frequency[token]
                    x["corpus_function_frequency"] = corpus[token]
                    x["module_rarity_bits"] = round(math.log2((len(module["functions"]) + 1) /
                                                               (frequency[token] + 1)), 6)
                    x["corpus_rarity_bits"] = round(math.log2((corpus_functions + 1) /
                                                               (corpus[token] + 1)), 6)
        module["summary"] = {"functions_with_imports": sum(bool(r["imports"]) for r in module["functions"]),
                             "functions_with_strings": sum(bool(r["strings"]) for r in module["functions"]),
                             "functions_with_data": sum(bool(r["data_references"]) for r in module["functions"]),
                             "functions_with_resource_id_candidates": sum(bool(r["resource_id_candidates"])
                                                                         for r in module["functions"])}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--graph", type=Path, required=True)
    parser.add_argument("--module", choices=("exe", "trim"), required=True)
    parser.add_argument("--objdump", type=Path, default=Path(shutil.which("llvm-objdump") or "llvm-objdump"))
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    target = args.output.resolve()
    if target.exists() or PRIVATE.resolve() not in target.parents:
        parser.error("output must be a new private evidence file")
    graph = read(args.graph)
    if graph["status"] != "INDEPENDENT_METADATA_CAPTURE_NO_GROUND_TRUTH":
        raise ValueError("graph capture status mismatch")
    saved = next(m for m in graph["modules"] if ("trim" in m["inputs"]["retail_image"]["path"].lower()) == (args.module == "trim"))
    paths = {side: Path(saved["inputs"][side + "_image"]["path"]) for side in ("retail", "current")}
    for side, path in paths.items():
        if digest(path) != saved["inputs"][side + "_image"]["sha256"]:
            raise ValueError(f"{side} binary changed since graph capture")
    objdump = args.objdump.resolve()
    modules = [index_image(saved, side, paths[side], objdump) for side in ("retail", "current")]
    add_rarity(modules)
    result = {"schema": SCHEMA, "version": VERSION,
              "tool_sha256": digest(Path(__file__)), "python_version": sys.version.split()[0],
              "pefile_version": getattr(pefile, "__version__", "UNKNOWN"),
              "commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
              "status": "COMPLETE_UNREVIEWED_RESOURCE_IDS",
              "configuration": {"module": args.module, "max_string_bytes": 512,
                                "resource_rule": "observed import call plus bounded local immediate push; review required",
                                "missing_evidence": "UNKNOWN"},
              "inputs": {"graph": {"path": str(args.graph.resolve()), "sha256": digest(args.graph)},
                         "objdump": {"path": str(objdump), "sha256": digest(objdump)},
                         **{side + "_image": {"path": str(path.resolve()), "sha256": digest(path)}
                            for side, path in paths.items()}},
              "modules": modules}
    with target.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(result, stream, sort_keys=True, separators=(",", ":"))
        stream.write("\n")
    print(json.dumps({"output": str(target), "sha256": digest(target),
                      "summary": {m["side"]: m["summary"] for m in modules}}, sort_keys=True))


if __name__ == "__main__":
    main()
