#!/usr/bin/env python3
"""Index PDB-emitted PE functions against source, without changing build artifacts.

The output is local research metadata, not proof of retail correspondence.  A
function's durable identity is the complete module SHA-256 plus its RVA.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import uuid

import pefile


FUNCTION_MARKER = re.compile(r"\bS_([A-Z_]*PROC32(?:_ID)?)\s*\[size\s*=\s*\d+\]")
ADDRESS_SIZE = re.compile(r"\baddr\s*=\s*(\d+):(\d+),\s*code size\s*=\s*(\d+)")
MODULE_MARKER = re.compile(r"^\s*Mod\s+(\d+)\s*\|\s*`([^`]*)`")
PDB_GUID = re.compile(r"\bGUID:\s*\{([^}]+)\}")
PDB_AGE = re.compile(r"\bAge:\s*(\d+)")
PDB_SOURCE_FILE = re.compile(r"^-\s*\(SHA-256:\s*([0-9A-Fa-f]{64})\)\s+(.+)$")
VALID_RELATIONSHIPS = {"ONE_TO_ONE", "SOURCE_INLINED", "BINARY_FOLDED",
                       "SOURCE_SPLIT", "BINARY_THUNK", "UNKNOWN"}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def command(*arguments: str, input_text: str | None = None) -> str:
    completed = subprocess.run(arguments, input=input_text, text=True,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               encoding="utf-8", errors="replace", check=False)
    if completed.returncode:
        raise RuntimeError(f"Tool failed ({completed.returncode}): {arguments[0]}\n"
                           + completed.stderr[-3000:])
    return completed.stdout


def manifest_artifact(root: Path, manifest: dict, relative_path: str) -> tuple[Path, dict]:
    wanted = relative_path.replace("\\", "/").lower()
    matches = [item for item in manifest["artifacts"]
               if item["path"].replace("\\", "/").lower() == wanted]
    if len(matches) != 1:
        raise ValueError(f"Manifest has {len(matches)} entries for {relative_path}")
    path = (root / matches[0]["path"]).resolve()
    if root.resolve() not in path.parents or not path.is_file():
        raise ValueError(f"Artifact missing or outside repository: {path}")
    measured = sha256(path)
    if measured != matches[0]["sha256"].upper():
        raise ValueError(f"Manifest hash mismatch for {path}: {measured}")
    return path, matches[0]


def pe_identity(path: Path) -> tuple[dict, list[int]]:
    image = pefile.PE(str(path), fast_load=False)
    sections = [section.VirtualAddress for section in image.sections]
    if image.FILE_HEADER.Machine != 0x14C:
        raise ValueError("Only the currently evidenced x86 PE build is supported")
    codeview = []
    for entry in getattr(image, "DIRECTORY_ENTRY_DEBUG", []):
        if entry.struct.Type != 2:
            continue
        data = image.get_data(entry.struct.AddressOfRawData, entry.struct.SizeOfData)
        if data[:4] == b"RSDS" and len(data) >= 24:
            codeview.append({"guid": str(uuid.UUID(bytes_le=data[4:20])).upper(),
                             "age": struct.unpack("<I", data[20:24])[0],
                             "recorded_pdb_path": data[24:].split(b"\0", 1)[0].decode("utf-8", "replace")})
    if len(codeview) != 1:
        raise ValueError(f"Expected one RSDS record, found {len(codeview)}")
    identity = {"pe_timestamp": image.FILE_HEADER.TimeDateStamp,
                "image_base": image.OPTIONAL_HEADER.ImageBase,
                "size_of_image": image.OPTIONAL_HEADER.SizeOfImage,
                "machine": "IMAGE_FILE_MACHINE_I386",
                "linker_version": f"{image.OPTIONAL_HEADER.MajorLinkerVersion}.{image.OPTIONAL_HEADER.MinorLinkerVersion}",
                "rsds": codeview[0]}
    image.close()
    return identity, sections


def pdb_identity(path: Path, pdbutil: str) -> dict:
    summary = command(pdbutil, "dump", "-summary", str(path))
    guid = PDB_GUID.search(summary)
    age = PDB_AGE.search(summary)
    if not guid or not age:
        raise ValueError("PDB summary lacks GUID or age")
    return {"guid": str(uuid.UUID(guid.group(1))).upper(), "age": int(age.group(1))}


def pdb_functions(path: Path, pdbutil: str, section_rvas: list[int]) -> list[dict]:
    process = subprocess.Popen([pdbutil, "dump", "--symbols", str(path)],
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               text=True, encoding="utf-8", errors="replace")
    if process.stdout is None:
        raise RuntimeError("PDB symbol stream unavailable")
    rows: list[dict] = []
    module_index = None
    compiland = ""
    pending: dict | None = None
    for line in process.stdout:
        module = MODULE_MARKER.search(line)
        if module:
            module_index, compiland = int(module.group(1)), module.group(2)
            pending = None
            continue
        marker = FUNCTION_MARKER.search(line)
        if marker:
            pending = {"symbol_kind": marker.group(1), "text": line[marker.end():].strip(),
                       "compiland_index": module_index, "compiland": compiland}
            continue
        if pending is None:
            continue
        pending["text"] += " " + line.strip()
        location = ADDRESS_SIZE.search(line)
        if not location:
            continue
        section, offset, size = map(int, location.groups())
        text = pending["text"].split("parent =", 1)[0]
        name = text.split("`", 1)[1].rsplit("`", 1)[0] if text.count("`") >= 2 else ""
        if section < 1 or section > len(section_rvas):
            raise ValueError(f"PDB function references unknown PE section {section}")
        rva = section_rvas[section - 1] + offset
        rows.append({"rva": rva, "size": size, "pdb_name": name,
                     "symbol_kind": pending["symbol_kind"],
                     "compiland_index": pending["compiland_index"],
                     "compiland": pending["compiland"]})
        pending = None
    error = process.stderr.read() if process.stderr else ""
    if process.wait():
        raise RuntimeError("PDB symbol extraction failed: " + error[-3000:])
    return rows


def pdb_source_checksums(path: Path, pdbutil: str) -> dict[int, dict[str, str]]:
    """Keep the compiler's per-compiland SHA-256, not just a path/line guess."""
    output = command(pdbutil, "dump", "--files", str(path))
    result: dict[int, dict[str, str]] = defaultdict(dict)
    module_index = None
    for line in output.splitlines():
        module = MODULE_MARKER.search(line)
        if module:
            module_index = int(module.group(1))
            continue
        source = PDB_SOURCE_FILE.match(line)
        if source and module_index is not None:
            result[module_index][str(Path(source.group(2)).resolve()).lower()] = source.group(1).upper()
    return result


def symbolize(image: Path, symbolizer: str, rvas: list[int]) -> dict[int, list[dict]]:
    input_text = "\n".join(f"0x{rva:x}" for rva in rvas) + "\n"
    output = command(symbolizer, "--relative-address", "--output-style=JSON",
                     "--inlines", f"--obj={image}", input_text=input_text)
    records = [json.loads(line) for line in output.splitlines() if line.strip()]
    if len(records) != len(rvas):
        raise ValueError(f"Symbolizer returned {len(records)} rows for {len(rvas)} RVAs")
    return {rva: row.get("Symbol", []) for rva, row in zip(rvas, records)}


def source_rows(path: Path) -> dict[str, list[dict]]:
    by_path: dict[str, list[dict]] = defaultdict(list)
    for line in path.read_text(encoding="utf-8").splitlines():
        row = json.loads(line)
        by_path[row["path"].lower()].append(row)
    return by_path


def verified_frames(frames: list[dict], symbols: list[dict],
                    checksums: dict[int, dict[str, str]], root: Path) -> list[dict]:
    result = []
    for frame in frames:
        item = dict(frame)
        file_name = frame.get("FileName", "")
        if file_name:
            path = Path(file_name).resolve()
            try:
                item["repository_relative"] = path.relative_to(root.resolve()).as_posix()
            except ValueError:
                item["repository_relative"] = None
            pdb_values = {checksums.get(symbol["compiland_index"], {}).get(str(path).lower())
                          for symbol in symbols}
            pdb_values.discard(None)
            item["pdb_source_sha256"] = next(iter(pdb_values)) if len(pdb_values) == 1 else None
            item["current_source_sha256"] = sha256(path) if path.is_file() else None
            item["source_checksum_verified"] = (item["pdb_source_sha256"] is not None and
                                                item["pdb_source_sha256"] == item["current_source_sha256"])
        else:
            item["source_checksum_verified"] = False
        result.append(item)
    return result


def source_candidates(frames: list[dict], index: dict[str, list[dict]], root: Path) -> list[dict]:
    results = []
    for frame in frames:
        file_name = frame.get("FileName", "")
        line_number = int(frame.get("Line", 0))
        if not file_name or not line_number:
            continue
        try:
            relative = Path(file_name).resolve().relative_to(root.resolve()).as_posix().lower()
        except ValueError:
            continue
        for row in index.get(relative, []):
            if row["start_line"] <= line_number <= row["end_line"]:
                results.append({"source_id": row["source_id"], "source_file": row["path"],
                                "source_line": line_number, "source_blob_sha256": row["source_blob_sha256"],
                                "source_index_commit": row["source_commit"],
                                "source_function": row["qualified_name"],
                                "name_matches_pdb": frame.get("FunctionName", "").startswith(
                                    row["qualified_name"] + "(")})
    return list({result["source_id"]: result for result in results}.values())


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--module", required=True, help="Repository-relative manifest artifact path")
    parser.add_argument("--source-index", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--pdbutil", default="llvm-pdbutil")
    parser.add_argument("--symbolizer", default="llvm-symbolizer")
    args = parser.parse_args()
    root = args.root.resolve()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    image, image_record = manifest_artifact(root, manifest, args.module)
    pdb_relative = str(Path(args.module).with_suffix(".pdb"))
    pdb, pdb_record = manifest_artifact(root, manifest, pdb_relative)
    pe, sections = pe_identity(image)
    pdb_info = pdb_identity(pdb, args.pdbutil)
    if {"guid": pe["rsds"]["guid"], "age": pe["rsds"]["age"]} != pdb_info:
        raise ValueError("Image RSDS and PDB GUID/age differ")
    symbols = pdb_functions(pdb, args.pdbutil, sections)
    checksums = pdb_source_checksums(pdb, args.pdbutil)
    by_rva: dict[int, list[dict]] = defaultdict(list)
    for symbol in symbols:
        by_rva[symbol["rva"]].append(symbol)
    frames = symbolize(image, args.symbolizer, sorted(by_rva))
    lexical = source_rows(args.source_index)
    rows = []
    module_hash = image_record["sha256"].upper()
    source_to_rvas: dict[str, set[int]] = defaultdict(set)
    for rva, group in sorted(by_rva.items()):
        checked_frames = verified_frames(frames[rva], group, checksums, root)
        candidates = source_candidates(checked_frames, lexical, root)
        matched = [candidate for candidate in candidates if candidate["name_matches_pdb"]]
        for candidate in matched:
            source_to_rvas[candidate["source_id"]].add(rva)
        relation = "UNKNOWN"
        if len({item["pdb_name"] for item in group}) > 1:
            relation = "BINARY_FOLDED"
        elif len(checked_frames) > 1:
            relation = "SOURCE_INLINED"
        elif len(matched) == 1 and checked_frames[0].get("source_checksum_verified"):
            relation = "ONE_TO_ONE"
        source_verified = any(frame.get("source_checksum_verified") for frame in checked_frames)
        rows.append({"record": "function", "function_id": f"{module_hash}:{rva:08X}",
                     "module_sha256": module_hash, "rva": rva, "va": pe["image_base"] + rva,
                     "pdb_symbols": group, "source_frames": checked_frames,
                     "source_candidates": candidates, "relationship": relation,
                     "identity_status": "PDB_SOURCE_BYTES_VERIFIED" if source_verified else "UNKNOWN"})
    for row in rows:
        if row["relationship"] == "ONE_TO_ONE":
            matched = [candidate for candidate in row["source_candidates"] if candidate["name_matches_pdb"]]
            if len(source_to_rvas[matched[0]["source_id"]]) > 1:
                row["relationship"] = "SOURCE_SPLIT"
    if any(row["relationship"] not in VALID_RELATIONSHIPS for row in rows):
        raise AssertionError("Invalid relationship state")
    header = {"record": "header", "schema": "cfw.compiled-source-bridge.v1",
              "module_sha256": module_hash, "module_path": args.module.replace("\\", "/"),
              "module_bytes": image_record["bytes"], "configuration": manifest["configuration"],
              "manifest_sha256": sha256(args.manifest),
              "build_input_fingerprint": manifest["buildInputFingerprint"],
              "source_revision": manifest["sourceRevision"],
              "pdb_sha256": pdb_record["sha256"].upper(), "pdb_guid_age": pdb_info,
              "pe": pe, "source_index_sha256": sha256(args.source_index),
              "pdb_extraction": "llvm-pdbutil S_GPROC32/S_LPROC32 family; image RSDS verified",
              "source_extraction": "llvm-symbolizer JSON plus per-compiland PDB SHA-256 and name-checked lexical source-index range; unresolved is retained",
              "provenance_warning": "PDB/source identity is not retail correspondence or semantic parity"}
    footer = {"record": "footer", "complete": True, "compiled_function_rvas": len(rows),
              "pdb_function_records": len(symbols),
              "functions_with_verified_source_bytes": sum(row["identity_status"] == "PDB_SOURCE_BYTES_VERIFIED" for row in rows),
              "relationship_counts": dict(Counter(row["relationship"] for row in rows))}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("x", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(header, sort_keys=True) + "\n")
        for row in rows:
            stream.write(json.dumps(row, sort_keys=True) + "\n")
        stream.write(json.dumps(footer, sort_keys=True) + "\n")
    print(json.dumps({"output": str(args.output), "sha256": sha256(args.output), **footer}, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError) as error:
        print(f"compiled-source bridge failed: {error}", file=sys.stderr)
        sys.exit(2)
