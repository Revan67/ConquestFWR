#!/usr/bin/env python3
"""Bounded metadata probes for two failed v2 controls; no correspondence decisions.

Reads frozen inputs and emits private functional facts, never raw instructions,
instruction bytes, or string text. Exact immediates require role-aware review.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import subprocess

import pefile

ROOT = Path(__file__).resolve().parents[3]
CASES = (
    ("retail_toolbar", "D:/Games/RetailRef/Trim.dll", 0x61240, 2245,
     "AC13625FEF6DA8D553196C894351E93F842BD8FB5344B97FEBA1FD6835E830F0"),
    ("current_toolbar", "Code/App/Src/Debug/Trim.dll", 0x8DF10, 0x1370,
     "BB9D7017902A9C6B338EC27D61B46C9D4E5A75B5EA8972E989F57E78225051D9"),
    ("current_state_false_top", "Code/App/Src/Debug/Trim.dll", 0x8CD40, 0xB2D,
     "BB9D7017902A9C6B338EC27D61B46C9D4E5A75B5EA8972E989F57E78225051D9"),
    ("retail_flagship_factory", "D:/Games/RetailRef/Conquest.exe", 0x83BF0, 0x86,
     "ED8741C0CB727A43B7DE7BE0E73566B04FBC1CBD58B54A62D538E3F39DCB3F52"),
    ("current_flagship_factory", "Code/App/Src/Final/Conquest.exe", 0xA4DD0, 187,
     "88ACFDE07FCBD120C1436492B4CAB6D9791C1DD772A830071C15C99E1BC09F8C"),
    ("current_supply_false_top", "Code/App/Src/Final/Conquest.exe", 0x1DD880, 167,
     "88ACFDE07FCBD120C1436492B4CAB6D9791C1DD772A830071C15C99E1BC09F8C"),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def extract_string_hashes(image, function_va, size, instructions):
    """Return ASCII literal hashes from bounded immediate operands, without text.

    ``image`` is an open pefile.PE. ``instructions`` is an iterable of
    (VA, mnemonic, operand_text), allowing callers to disassemble a module once.
    This intentionally does not follow memory pointers or scan unreferenced data.
    """
    strings = Counter()
    base = image.OPTIONAL_HEADER.ImageBase
    for va, _op, args in instructions:
        if not function_va <= va < function_va + size:
            continue
        for raw in re.findall(r"\$(0x[\da-f]+)", args.split("#", 1)[0]):
            value = int(raw, 16)
            if not base <= value < base + image.OPTIONAL_HEADER.SizeOfImage:
                continue
            data = image.get_data(value-base, 256).split(b"\0", 1)[0]
            if len(data) >= 4 and all(32 <= b < 127 for b in data):
                strings[hashlib.sha256(data).hexdigest()] += 1
    return strings


def probe(label, name, rva, size, expected):
    path = (ROOT / name).resolve()
    if digest(path) != expected:
        raise ValueError(f"Frozen module hash mismatch: {label}")
    image = pefile.PE(str(path))
    base = image.OPTIONAL_HEADER.ImageBase
    result = subprocess.run(["llvm-objdump", "-d", "--no-show-raw-insn",
                             f"--start-address={base+rva}",
                             f"--stop-address={base+rva+size}", str(path)],
                            capture_output=True, text=True, check=True)
    comparisons = []
    instructions = []
    direct_calls = Counter()
    indirect_slots = Counter()
    count = 0
    for line in result.stdout.splitlines():
        instruction = re.match(r"^\s*([\da-f]+):\s+(\w+)\s*(.*)", line)
        if not instruction:
            continue
        count += 1
        op, args = instruction[2], instruction[3].split("#", 1)[0].strip()
        instructions.append((int(instruction[1], 16), op, args))
        # Operand text never leaves this process. Stack comparisons and object
        # comparisons remain distinct; register names do not establish ownership.
        compare = re.match(r"\$(0x[\da-f]+|\d+),\s*(0x[\da-f]+|\d+)\(%(\w+)\)", args)
        if op.startswith("cmp") and compare:
            comparisons.append({"immediate": int(compare[1], 0),
                                "displacement": int(compare[2], 0),
                                "stack_base": compare[3] in {"esp", "ebp"}})
        if op.startswith("call"):
            direct = re.match(r"(0x[\da-f]+)", args)
            slot = re.match(r"\*(0x[\da-f]+|\d+)\(%\w+\)", args)
            if direct:
                direct_calls[hex(int(direct[1], 16)-base)] += 1
            elif slot:
                indirect_slots[hex(int(slot[1], 0))] += 1
    strings = extract_string_hashes(image, base+rva, size, instructions)
    image.close()
    return {"label": label, "module_sha256": expected, "rva": hex(rva),
            "byte_extent": size, "instruction_count": count,
            "immediate_memory_comparisons": comparisons,
            "direct_call_rvas": dict(direct_calls),
            "unresolved_indirect_slot_counts": dict(indirect_slots),
            "ascii_immediate_string_sha256": dict(strings),
            "limitations": "No dataflow ownership; no string pointer dereference; local call RVAs require independently reviewed identities."}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    private = ROOT / "Evidence/RetailMap/local"
    if private not in output.parents:
        raise ValueError("Output must remain in private local evidence")
    if output.exists():
        raise ValueError("Refusing to overwrite previous evidence")
    source = ROOT / "Code/App/Src/Menu_Toolbar.cpp"
    if digest(source) != "8084E8B4BE3BC6F08B076C13A64F9BFF6F1D5638B74429F476937FD58ADE7A0D":
        raise ValueError("Frozen toolbar source identity changed")
    obj = ROOT / "Code/App/Src/TrimDebug/Menu_Toolbar.obj"
    cv = subprocess.run(["llvm-readobj", "--codeview", str(obj)],
                        capture_output=True, text=True, check=True).stdout
    cv_sizes = {}
    for block in re.findall(r"GlobalProcIdSym \{(.*?)\n\s*\}", cv, re.S):
        name = re.search(r"DisplayName: (Menu_tb::(?:loadInterface|setStateInfo))", block)
        size = re.search(r"CodeSize: (0x[\dA-Fa-f]+)", block)
        if name and size:
            cv_sizes[name[1]] = int(size[1], 16)
    if cv_sizes != {"Menu_tb::loadInterface": 0x1370, "Menu_tb::setStateInfo": 0xB2D}:
        raise ValueError(f"CodeView exact bounds changed or unavailable: {cv_sizes}")
    source_hash = digest(source)
    checksum_blocks = re.findall(r"FileChecksum \{(.*?)\n\s*\}", cv, re.S)
    compiler_hashes = []
    for block in checksum_blocks:
        if "Menu_Toolbar.cpp" in block:
            compiler_hashes.append("".join(re.findall(r"^\s*[\dA-Fa-f]{4}: ([\dA-Fa-f ]+)\s*\|", block, re.M)).replace(" ", "").upper())
    if compiler_hashes != [source_hash]:
        raise ValueError("Toolbar compiler source checksum not present in CodeView")
    probes = [probe(*case) for case in CASES]
    by_name = {x["label"]: x for x in probes}
    shared = {}
    for name in ("current_toolbar", "current_state_false_top"):
        left = set(by_name["retail_toolbar"]["ascii_immediate_string_sha256"])
        right = set(by_name[name]["ascii_immediate_string_sha256"])
        shared[name] = {"retail_unique": len(left), "current_unique": len(right),
                        "shared_unique": len(left & right),
                        "shared_hashes": sorted(left & right)}
    result = {"schema": "cfw.v3.trim-factory-context.v1", "tool_sha256": digest(Path(__file__)),
              "scope": "bounded failure analysis; not a matcher or correspondence approval",
              "probes": probes, "toolbar_string_overlap": shared,
              "toolbar_compiler_provenance": {"source_sha256": source_hash,
                  "coff_object_sha256": digest(obj),
                  "map_sha256": digest(ROOT / "Code/App/Src/debug/Trim.map"),
                  "codeview_function_sizes": cv_sizes,
                  "fresh_pdb": "absent; no general UI PDB source linkage claimed"},
              "bound_provenance": "Retail saved Function extent; current factory PDB extents; Trim loadInterface and setStateInfo CodeView object CodeSize + fresh map."}
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf8")
    print(json.dumps({"output": str(output), "sha256": digest(output),
                      "toolbar_string_overlap": shared}, sort_keys=True))


if __name__ == "__main__":
    main()
