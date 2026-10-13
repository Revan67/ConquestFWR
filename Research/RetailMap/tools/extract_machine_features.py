#!/usr/bin/env python3
"""Extract normalized x86 metadata for predeclared retail controls and fresh builds.

No instruction bytes or decompiled text are saved. Results belong in a private
SQLite copy. This does not assert correspondence or behavioral equivalence.
"""
from __future__ import annotations

import argparse
from bisect import bisect_right
import hashlib
import json
from pathlib import Path
import re
import sqlite3
import subprocess

import pefile


RETAIL_EXE = "ED8741C0CB727A43B7DE7BE0E73566B04FBC1CBD58B54A62D538E3F39DCB3F52"
RETAIL_TRIM = "AC13625FEF6DA8D553196C894351E93F842BD8FB5344B97FEBA1FD6835E830F0"
CURRENT_EXE = "88ACFDE07FCBD120C1436492B4CAB6D9791C1DD772A830071C15C99E1BC09F8C"
CURRENT_TRIM = "BB9D7017902A9C6B338EC27D61B46C9D4E5A75B5EA8972E989F57E78225051D9"
VERSION = "normalized-x86-v2-development"
INSTRUCTION = re.compile(r"^\s*([0-9a-fA-F]+):\s+([A-Za-z][\w.]*)\s*(.*)$")
MAP_SYMBOL = re.compile(r"^\s*0001:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})\s+f\s+(\S+)")
BRANCH = re.compile(r"^j(?!mp)|^loop")
SUFFIX_OPS = ("mov", "push", "pop", "lea", "add", "sub", "cmp", "test", "and", "or", "xor",
              "imul", "idiv", "shl", "shr", "sar", "inc", "dec", "call", "ret", "rep")


def digest(path: Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            hasher.update(block)
    return hasher.hexdigest().upper()


def mnemonic(value: str) -> str:
    value = value.lower().split(".", 1)[0]
    if value.endswith(("b", "w", "l", "q")) and value[:-1].startswith(SUFFIX_OPS):
        return value[:-1]
    return value


def operand(value: str) -> str:
    value = value.strip()
    if not value:
        return ""
    indirect = value.startswith("*")
    value = value.removeprefix("*")
    if value.startswith("%"):
        kind = "REG"
    elif value.startswith("$"):
        try:
            number = int(value[1:], 0)
            kind = "IMM_SMALL" if -128 <= number <= 255 else "IMM_LARGE"
        except ValueError:
            kind = "IMM"
    elif "(" in value:
        base = value.split("(", 1)[1]
        kind = "MEM_INDEX" if "," in base else "MEM_REG"
    elif re.fullmatch(r"-?(?:0x[0-9a-f]+|[0-9]+)", value, re.I):
        kind = "ABS_OR_BRANCH"
    else:
        kind = "OTHER"
    return "INDIRECT_" + kind if indirect else kind


def shape(op: str, arguments: str) -> tuple[str, str]:
    op = mnemonic(op)
    parts = re.split(r",\s*(?![^()]*\))", arguments)
    types = [operand(part) for part in parts if part.strip()]
    if op.startswith("j") or op.startswith("loop"):
        return op, "BRANCH"
    return op, op + "(" + ",".join(types) + ")"


def read_disassembly(path: Path) -> tuple[list[int], dict[int, tuple[str, str]]]:
    result = subprocess.run(["llvm-objdump", "-d", "--no-show-raw-insn", str(path)],
                            capture_output=True, text=True, check=True)
    instructions = {}
    for line in result.stdout.splitlines():
        match = INSTRUCTION.match(line)
        if match:
            instructions[int(match[1], 16)] = shape(match[2], match[3])
    return sorted(instructions), instructions


def feature(addresses: list[int], instructions: dict, start: int, size: int) -> dict | None:
    if size <= 0:
        return None
    left = bisect_right(addresses, start - 1)
    right = bisect_right(addresses, start + size - 1)
    addresses = addresses[left:right]
    if not addresses or addresses[0] != start:
        return None
    mnemonics = [instructions[va][0] for va in addresses]
    shapes = [instructions[va][1] for va in addresses]
    call_count = sum(op.startswith("call") for op in mnemonics)
    conditional = sum(bool(BRANCH.match(op)) for op in mnemonics)
    returns = sum(op.startswith("ret") for op in mnemonics)
    jumps = sum(op.startswith("jmp") for op in mnemonics)
    normalized = "|".join(shapes)
    return {"instruction_count": len(addresses), "byte_extent": size,
            "mnemonics": mnemonics, "shapes": shapes,
            "normalized_sha256": hashlib.sha256(normalized.encode()).hexdigest().upper(),
            "control_flow": {"calls": call_count, "conditional_branches": conditional,
                             "unconditional_jumps": jumps, "returns": returns}}


def trim_map_functions(path: Path, base: int) -> list[tuple[int, int, str]]:
    symbols = {}
    for line in path.read_text(encoding="latin-1").splitlines():
        match = MAP_SYMBOL.match(line)
        if match:
            va = int(match[2], 16)
            if va >= base and not match[1].startswith("__unwindfunclet$"):
                symbols.setdefault(va, match[1])
    addresses = sorted(symbols)
    result = []
    for index, va in enumerate(addresses[:-1]):
        size = addresses[index + 1] - va
        if 0 < size <= 0x10000:
            result.append((va, size, symbols[va]))
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("db", "controls", "retail-exe", "current-exe", "retail-trim",
                 "current-trim", "current-trim-map"):
        parser.add_argument("--" + name, required=True, type=Path)
    args = parser.parse_args()
    expected = [(args.retail_exe, RETAIL_EXE), (args.current_exe, CURRENT_EXE),
                (args.retail_trim, RETAIL_TRIM), (args.current_trim, CURRENT_TRIM)]
    for path, identity in expected:
        if digest(path) != identity:
            raise ValueError(f"Module SHA-256 disagrees with frozen identity: {path}")
    controls = json.loads(args.controls.read_text(encoding="utf-8"))
    retail_controls = {(item["module"], int(item["va"], 16))
                       for group in ("controls", "hard_negatives") for item in controls[group]}
    connection = sqlite3.connect(args.db)
    connection.execute("PRAGMA foreign_keys=ON")
    extractor_hash = digest(Path(__file__))
    counts = {}
    try:
        with connection:
            for module_path, module_hash, kind, selector in [
                (args.retail_exe, RETAIL_EXE, "RETAIL", "Conquest_retail.exe"),
                (args.retail_trim, RETAIL_TRIM, "RETAIL", "Trim.dll"),
                (args.current_exe, CURRENT_EXE, "RECONSTRUCTED", "Conquest.exe"),
                (args.current_trim, CURRENT_TRIM, "RECONSTRUCTED", "Trim.current.dll"),
            ]:
                image = pefile.PE(str(module_path), fast_load=True)
                base = image.OPTIONAL_HEADER.ImageBase
                if selector == "Conquest_retail.exe":
                    connection.execute(
                        "INSERT OR IGNORE INTO binary_modules VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                        (module_hash, selector, kind, None, module_path.stat().st_size,
                         image.FILE_HEADER.TimeDateStamp, base, "IMAGE_FILE_MACHINE_I386",
                         f"{image.OPTIONAL_HEADER.MajorLinkerVersion}.{image.OPTIONAL_HEADER.MinorLinkerVersion}",
                         None, None, None, None, None, "canonical retail EXE SHA-256 verified"),
                    )
                elif selector == "Trim.current.dll":
                    connection.execute(
                        "INSERT OR IGNORE INTO binary_modules VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                        (module_hash, selector, kind, "Debug", module_path.stat().st_size,
                         image.FILE_HEADER.TimeDateStamp, base, "IMAGE_FILE_MACHINE_I386",
                         f"{image.OPTIONAL_HEADER.MajorLinkerVersion}.{image.OPTIONAL_HEADER.MinorLinkerVersion}",
                         None, None, None, None, None,
                         "fresh Debug Trim.dll; SHA/PE map timestamp matched; no fresh PDB"),
                    )
                addresses, instructions = read_disassembly(module_path)
                if kind == "RETAIL":
                    intervals = [(va, size, None) for va, size in connection.execute(
                        "SELECT va,size FROM retail_functions WHERE module_sha256=? AND size IS NOT NULL",
                        (module_hash,)) if (selector, va) in retail_controls]
                elif selector == "Conquest.exe":
                    intervals = [(va, size, None) for va, size in connection.execute(
                        "SELECT va,byte_extent FROM compiled_functions WHERE module_sha256=? AND byte_extent IS NOT NULL",
                        (module_hash,))]
                else:
                    intervals = trim_map_functions(args.current_trim_map, base)
                total = 0
                unresolved = 0
                for va, size, symbol in intervals:
                    # The fresh CodeView object record supplies the exact UI holdout
                    # size; map-next-symbol bounds are only candidates elsewhere.
                    if selector == "Trim.current.dll" and va == 0x1008DF10:
                        size = 0x1370
                    data = feature(addresses, instructions, va, size)
                    if data is None:
                        unresolved += 1
                        continue
                    if selector == "Trim.current.dll":
                        is_ui_anchor = va == 0x1008DF10
                        if is_ui_anchor:
                            source_id = "Code/App/Src/Menu_Toolbar.cpp:1685:Menu_tb::loadInterface"
                            source_hash = digest(Path("Code/App/Src/Menu_Toolbar.cpp"))
                            indexed = connection.execute(
                                "SELECT source_blob_sha256 FROM source_functions WHERE source_id=? AND compiled_state='COMPILED'",
                                (source_id,),
                            ).fetchone()
                            if indexed is None or source_hash != indexed[0].upper() or source_hash != "8084E8B4BE3BC6F08B076C13A64F9BFF6F1D5638B74429F476937FD58ADE7A0D":
                                raise ValueError("UI CodeView compiler checksum/source index disagree")
                        connection.execute(
                            "INSERT OR IGNORE INTO compiled_functions VALUES (?,?,?,?,?,?,?,?,?)",
                            (module_hash, va - base, va, size,
                             json.dumps([{"map_symbol": symbol}], separators=(",", ":")),
                             "[]", "ONE_TO_ONE" if is_ui_anchor else "UNKNOWN",
                             "MAP_OBJECT_SOURCE_BYTES_VERIFIED" if is_ui_anchor else "UNKNOWN",
                             "fresh map symbol bound; UI control exact extent and source SHA-256 from CodeView object"),
                        )
                        if is_ui_anchor:
                            connection.execute(
                                "INSERT INTO compiled_source_links VALUES (?,?,?,?,?,?,?,?,?,?)",
                                (module_hash, va - base, source_id, "Code/App/Src/Menu_Toolbar.cpp",
                                 1686, "Menu_tb::loadInterface", source_hash, source_hash,
                                 "ONE_TO_ONE", "fresh Trim.map + CodeView object function extent/source SHA-256"),
                            )
                    connection.execute(
                        "INSERT INTO machine_features VALUES (?,?,?,?,?,?,?,?,?)",
                        (kind, module_hash, va - base, "normalized_ir_json", json.dumps(data, separators=(",", ":")),
                         1, extractor_hash, VERSION,
                         "llvm-objdump bounded by saved retail Function, verified PDB record, or current Trim map"),
                    )
                    total += 1
                counts[selector] = {"features": total, "unresolved_bounds": unresolved,
                                    "disassembled_addresses": len(addresses)}
        print(json.dumps({"extractor_sha256": extractor_hash,
                          "normalization_version": VERSION, "modules": counts}, sort_keys=True))
    finally:
        connection.close()


if __name__ == "__main__":
    main()
