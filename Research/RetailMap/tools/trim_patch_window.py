#!/usr/bin/env python3
"""Bounded, read-only provenance and machine-reference audit of a Trim patch window.

Output is private evidence, not a function correspondence or retail behavior claim.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

import pefile


ROOT = Path(__file__).resolve().parents[3]
PRIVATE = ROOT / "Evidence/RetailMap/local"
BASELINE = "AC13625FEF6DA8D553196C894351E93F842BD8FB5344B97FEBA1FD6835E830F0"
EXPECTED = {
    "A1382855362CA80BBA7F44477CA616073EEDA9091526428BF22086D506ED89AA",
    "95CE6D15E5EDF55E5D6FD37D559D37997EC1DA893F0E112E6B8B8F1A637E89DE",
    "131A9C6D94C39EC79B9A900121F500648338094494A4A3822C625CCA030997A1",
    "FE08D12170C72B0D7502E97CF9FE2EAF576CA0E4496D0778E1F2F796A1F2A20D",
}
START, LENGTH, PROBABLE_ENTRY = 0x3C865, 38, 0x3C7F0
LINE = re.compile(r"^\s*([0-9a-fA-F]+):\s+([\w.]+)\s*(.*?)\s*$")
DIRECT = re.compile(r"^0x([0-9a-fA-F]+)(?:\s|$)")


def sha(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest().upper()


def section(pe: pefile.PE, rva: int):
    for s in pe.sections:
        if s.VirtualAddress <= rva < s.VirtualAddress + max(s.Misc_VirtualSize, s.SizeOfRawData):
            return s
    raise ValueError(f"RVA {rva:X} is unmapped")


def section_shape(pe: pefile.PE):
    return [(s.Name.decode("ascii", "replace").rstrip("\0"), s.VirtualAddress,
             s.Misc_VirtualSize, s.SizeOfRawData, s.PointerToRawData, s.Characteristics)
            for s in pe.sections]


def relocations(pe: pefile.PE, left: int, right: int):
    return sorted({e.rva for b in getattr(pe, "DIRECTORY_ENTRY_BASERELOC", [])
                   for e in b.entries if left <= e.rva < right and e.type != 0})


def region(pe: pefile.PE):
    s = section(pe, START)
    if START + LENGTH > s.VirtualAddress + s.SizeOfRawData:
        raise ValueError("window is not wholly file-backed")
    raw = s.PointerToRawData + START - s.VirtualAddress
    return {"image_base": pe.OPTIONAL_HEADER.ImageBase,
            "section": s.Name.decode("ascii", "replace").rstrip("\0"),
            "section_rva": s.VirtualAddress, "section_raw_offset": s.PointerToRawData,
            "section_raw_alignment": pe.OPTIONAL_HEADER.FileAlignment,
            "section_virtual_alignment": pe.OPTIONAL_HEADER.SectionAlignment,
            "rva_begin": START, "rva_end_inclusive": START + LENGTH - 1,
            "va_begin": pe.OPTIONAL_HEADER.ImageBase + START,
            "va_end_inclusive": pe.OPTIONAL_HEADER.ImageBase + START + LENGTH - 1,
            "file_offset_begin": raw, "file_offset_end_inclusive": raw + LENGTH - 1,
            "bytes_hex": pe.get_data(START, LENGTH).hex(),
            "before_32_hex": pe.get_data(START - 32, 32).hex(),
            "after_32_hex": pe.get_data(START + LENGTH, 32).hex(),
            "relocations_in_window": relocations(pe, START, START + LENGTH),
            "relocations_nearby": relocations(pe, START - 32, START + LENGTH + 32)}


def pointer_refs(pe: pefile.PE, target_rva: int):
    target = pe.OPTIONAL_HEADER.ImageBase + target_rva
    needle = struct.pack("<I", target)
    results = []
    for s in pe.sections:
        data = s.get_data()
        off = data.find(needle)
        while off >= 0:
            rva = s.VirtualAddress + off
            results.append({"rva": rva, "section": s.Name.decode("ascii", "replace").rstrip("\0"),
                            "relocated": bool(relocations(pe, rva, rva + 1))})
            off = data.find(needle, off + 1)
    return results


def disasm(image: Path, objdump: Path):
    process = subprocess.run([str(objdump), "-d", "--no-show-raw-insn", str(image)],
                             check=True, text=True, capture_output=True)
    result = {}
    for line in process.stdout.splitlines():
        match = LINE.match(line)
        if match:
            result[int(match[1], 16)] = (match[2].lower(), match[3].split("#", 1)[0].strip())
    return result


def machine_context(instructions: dict, base: int):
    entry = base + PROBABLE_ENTRY
    begin, end = base + START, base + START + LENGTH
    refs = []
    for site, (op, arg) in instructions.items():
        if not (op.startswith("j") or op.startswith("call") or op.startswith("loop")):
            continue
        match = DIRECT.match(arg)
        if not match:
            continue
        dest = int(match[1], 16)
        if dest == entry or begin <= dest < end or dest == end:
            refs.append({"site_rva": site - base, "target_rva": dest - base, "opcode": op,
                         "site_inside_candidate": entry <= site <= base + 0x3C8C3})
    bounded = [{"rva": va - base, "opcode": op, "operand": arg}
               for va, (op, arg) in sorted(instructions.items())
               if base + 0x3C7D0 <= va <= base + 0x3C8D0]
    return {"probable_entry_rva": PROBABLE_ENTRY,
            "direct_references_to_entry_window_or_exit": refs,
            "bounded_instructions": bounded,
            "entry_decodes": entry in instructions,
            "every_window_start_decodes": begin in instructions}


def saved_neighbors(metadata: Path):
    rows = [json.loads(line) for line in metadata.read_text(encoding="utf-8").splitlines()]
    ranges = []
    for row in rows:
        if row.get("record") == "function":
            for body in row["body_ranges"]:
                ranges.append((int(body["start"], 16) - 0x10000000,
                               int(body["end"], 16) - 0x10000000, row["rva"], row["name"]))
    covering = [r for r in ranges if r[0] <= START <= r[1] or r[0] <= START + LENGTH - 1 <= r[1]]
    before = max((r for r in ranges if r[1] < START), key=lambda r: r[1])
    after = min((r for r in ranges if r[0] >= START + LENGTH), key=lambda r: r[0])
    def pack(r):
        return {"range_start_rva": r[0], "range_end_rva": r[1], "entry_rva": r[2], "name": r[3]}
    return {"covering": [pack(r) for r in covering], "preceding": pack(before),
            "following": pack(after), "bytes_after_preceding": START - before[1] - 1,
            "bytes_before_following": after[0] - (START + LENGTH)}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--specimens", type=Path, required=True)
    p.add_argument("--canonical", type=Path, required=True)
    p.add_argument("--metadata", type=Path, required=True)
    p.add_argument("--objdump", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    a = p.parse_args()
    output = a.output.resolve()
    if output.exists() or PRIVATE.resolve() not in output.parents:
        p.error("output must be a new path inside private evidence")
    if sha(a.canonical) != BASELINE:
        raise ValueError("canonical Trim SHA-256 mismatch")
    specimens = json.loads(a.specimens.read_text(encoding="utf-8"))["specimens"]
    if set(specimens) != EXPECTED | {BASELINE}:
        raise ValueError("unexpected specimen set")
    found = {}
    with pefile.PE(str(a.canonical), fast_load=False) as canonical:
        baseline_shape = section_shape(canonical)
        baseline_region = region(canonical)
        for identity, info in sorted(specimens.items()):
            path = Path(info["occurrences"][0]["absolute_path"])
            if sha(path) != identity:
                raise ValueError(f"specimen changed: {path}")
            with pefile.PE(str(path), fast_load=False) as pe:
                same_layout = (pe.OPTIONAL_HEADER.ImageBase == canonical.OPTIONAL_HEADER.ImageBase
                               and section_shape(pe) == baseline_shape)
                found[identity] = {"path": str(path), "sha256": identity, "size": path.stat().st_size,
                                   "same_layout": same_layout,
                                   "same_rva_window": region(pe) if same_layout else None}
                if same_layout:
                    variant = region(pe)
                    found[identity]["same_window_as_canonical"] = variant["bytes_hex"] == baseline_region["bytes_hex"]
                    found[identity]["changed_byte_offsets_within_window"] = [i for i, (x, y) in enumerate(
                        zip(bytes.fromhex(baseline_region["bytes_hex"]), bytes.fromhex(variant["bytes_hex"]))) if x != y]
                    found[identity]["linear_machine"] = machine_context(
                        disasm(path, a.objdump), pe.OPTIONAL_HEADER.ImageBase)
        ptrs = {f"0x{rva:X}": pointer_refs(canonical, rva)
                for rva in (PROBABLE_ENTRY, START, START + LENGTH,
                            0xA49F0, 0xA4A04, 0xA4A18, 0xA4A3C)}
        dirs = {"exception": {"rva": canonical.OPTIONAL_HEADER.DATA_DIRECTORY[3].VirtualAddress,
                               "size": canonical.OPTIONAL_HEADER.DATA_DIRECTORY[3].Size},
                "debug": {"rva": canonical.OPTIONAL_HEADER.DATA_DIRECTORY[6].VirtualAddress,
                          "size": canonical.OPTIONAL_HEADER.DATA_DIRECTORY[6].Size}}
        machine = machine_context(disasm(a.canonical, a.objdump), canonical.OPTIONAL_HEADER.ImageBase)
    inputs = {"specimens": a.specimens, "canonical": a.canonical,
              "metadata": a.metadata, "objdump": a.objdump}
    result = {"schema": "cfw.retailmap.trim-patch-window.v1", "status": "MACHINE_OBSERVATION_NOT_OWNERSHIP",
              "tool_sha256": sha(Path(__file__)), "python": sys.version.split()[0],
              "pefile": pefile.__version__, "commit": subprocess.check_output(
                  ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
              "inputs": {k: {"path": str(v.resolve()), "sha256": sha(v)} for k, v in inputs.items()},
              "window": baseline_region, "specimens": found,
              "saved_ghidra": saved_neighbors(a.metadata), "data_pointer_references": ptrs,
              "pe_directories": dirs, "linear_machine": machine}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, sort_keys=True, separators=(",", ":")) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(output), "sha256": sha(output),
                      "specimens": len(found), "direct_refs": len(machine["direct_references_to_entry_window_or_exit"])},
                     sort_keys=True))


if __name__ == "__main__":
    main()
