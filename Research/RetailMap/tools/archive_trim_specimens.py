#!/usr/bin/env python3
"""Read-only, deterministic PE and exact-byte-window audit of cataloged Trim specimens.

The archive catalog supplies provenance; this tool independently checks payload
hashes before inspection. Exact RVA-window equality is historical continuity
evidence only, never a retail/current function correspondence.
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

import pefile


VERSION = "archive-trim-specimens-1"
SCHEMA = "cfw.retailmap.archive-trim-specimens.v1"
ROOT = Path(__file__).resolve().parents[3]
PRIVATE = ROOT / "Evidence/RetailMap/local"
ARCHIVE = Path("D:/Dev/ConquestResearchArchive")
CANONICAL = "AC13625FEF6DA8D553196C894351E93F842BD8FB5344B97FEBA1FD6835E830F0"
ASCII = re.compile(rb"[\x20-\x7e]{5,}")


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest().upper()


def read(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def pe_metadata(path: Path) -> dict:
    pe = pefile.PE(str(path), fast_load=False)
    image_base = pe.OPTIONAL_HEADER.ImageBase
    sections = [{"name": s.Name.rstrip(b"\0").decode("ascii", "replace"),
                 "rva": s.VirtualAddress, "virtual_size": s.Misc_VirtualSize,
                 "raw_size": s.SizeOfRawData, "raw_sha256": hashlib.sha256(s.get_data()).hexdigest().upper(),
                 "characteristics": s.Characteristics}
                for s in pe.sections]
    imports = []
    for entry in getattr(pe, "DIRECTORY_ENTRY_IMPORT", []):
        module = entry.dll.decode("ascii", "replace").lower()
        imports.extend({"module": module,
                        "symbol": symbol.name.decode("ascii", "replace") if symbol.name else None,
                        "ordinal": symbol.ordinal if symbol.name is None else None}
                       for symbol in entry.imports)
    imports.sort(key=lambda x: (x["module"], x["symbol"] or "", x["ordinal"] or 0))
    exports = []
    if hasattr(pe, "DIRECTORY_ENTRY_EXPORT"):
        exports = [{"name": s.name.decode("ascii", "replace") if s.name else None,
                    "ordinal": s.ordinal, "rva": s.address,
                    "forwarder": s.forwarder.decode("ascii", "replace") if s.forwarder else None}
                   for s in pe.DIRECTORY_ENTRY_EXPORT.symbols]
        exports.sort(key=lambda x: (x["ordinal"], x["name"] or ""))
    resources = []
    for kind in getattr(getattr(pe, "DIRECTORY_ENTRY_RESOURCE", None), "entries", []):
        if not hasattr(kind, "directory"):
            continue
        for item in kind.directory.entries:
            if not hasattr(item, "directory"):
                continue
            for lang in item.directory.entries:
                if hasattr(lang, "data"):
                    resources.append({"type_id": kind.id, "type_name": str(kind.name) if kind.name else None,
                                      "item_id": item.id,
                                      "item_name_sha256": hashlib.sha256(str(item.name).encode()).hexdigest().upper()
                                      if item.name else None,
                                      "language": lang.id, "size": lang.data.struct.Size})
    resources.sort(key=lambda x: (x["type_id"] or -1, x["item_id"] or -1, x["language"] or -1))
    codeview = []
    for entry in getattr(pe, "DIRECTORY_ENTRY_DEBUG", []):
        if entry.struct.Type != 2 or not entry.struct.SizeOfData:
            continue
        raw = pe.get_data(entry.struct.AddressOfRawData, entry.struct.SizeOfData)
        if raw.startswith(b"RSDS") and len(raw) >= 24:
            codeview.append({"format": "RSDS", "guid_hex": raw[4:20].hex().upper(),
                             "age": struct.unpack_from("<I", raw, 20)[0],
                             "pdb_basename": Path(raw[24:].split(b"\0", 1)[0].decode("latin1", "replace").replace("\\", "/")).name})
        elif raw.startswith(b"NB10"):
            codeview.append({"format": "NB10", "record_sha256": hashlib.sha256(raw).hexdigest().upper()})
    strings = Counter()
    for section in pe.sections:
        if section.Characteristics & 0x20000000:
            continue
        data = section.get_data()
        for match in ASCII.finditer(data):
            strings[hashlib.sha256(match.group()).hexdigest().upper()] += 1
    version = []
    for fixed in getattr(pe, "VS_FIXEDFILEINFO", []):
        version.append({"file_version_ms": fixed.FileVersionMS, "file_version_ls": fixed.FileVersionLS,
                        "product_version_ms": fixed.ProductVersionMS, "product_version_ls": fixed.ProductVersionLS})
    directories = pe.OPTIONAL_HEADER.DATA_DIRECTORY
    result = {"sha256": digest(path), "size": path.stat().st_size,
              "machine": pe.FILE_HEADER.Machine, "pe_magic": pe.OPTIONAL_HEADER.Magic,
              "pe_timestamp": pe.FILE_HEADER.TimeDateStamp, "image_base": image_base,
              "entry_rva": pe.OPTIONAL_HEADER.AddressOfEntryPoint,
              "sections": sections, "executable_code_raw_bytes": sum(s["raw_size"] for s in sections
                                                                      if s["characteristics"] & 0x20000000),
              "imports": imports, "exports": exports, "resources": resources,
              "codeview": codeview, "version_fixed": version,
              "debug_directory_rva": directories[6].VirtualAddress,
              "debug_directory_size": directories[6].Size,
              "relocation_directory_rva": directories[5].VirtualAddress,
              "relocation_directory_size": directories[5].Size,
              "embedded_ascii_string_hashes": sorted(strings),
              "embedded_ascii_string_count": sum(strings.values())}
    pe.close()
    return result


def exact_windows(canonical_metadata: Path, canonical_path: Path, variant_path: Path) -> dict:
    """Compare saved canonical function bodies at identical RVAs, not inferred matches."""
    canonical = pefile.PE(str(canonical_path), fast_load=True)
    variant = pefile.PE(str(variant_path), fast_load=True)
    same_base = canonical.OPTIONAL_HEADER.ImageBase == variant.OPTIONAL_HEADER.ImageBase
    same_sections = [(s.Name, s.VirtualAddress, s.Misc_VirtualSize, s.SizeOfRawData, s.PointerToRawData, s.Characteristics)
                     for s in canonical.sections] == [(s.Name, s.VirtualAddress, s.Misc_VirtualSize, s.SizeOfRawData, s.PointerToRawData, s.Characteristics)
                                                     for s in variant.sections]
    raw_sections = []
    if same_sections:
        for a, b in zip(canonical.sections, variant.sections):
            aa, bb = a.get_data(), b.get_data()
            changed = [i for i, (x, y) in enumerate(zip(aa, bb)) if x != y]
            runs = []
            for offset in changed:
                if runs and offset == runs[-1][1] + 1:
                    runs[-1][1] = offset
                else:
                    runs.append([offset, offset])
            raw_sections.append({"name": a.Name.rstrip(b"\0").decode("ascii", "replace"),
                                 "canonical_sha256": hashlib.sha256(aa).hexdigest().upper(),
                                 "variant_sha256": hashlib.sha256(bb).hexdigest().upper(),
                                 "different_bytes": len(changed),
                                 "changed_rva_ranges_inclusive":
                                 [[a.VirtualAddress + start, a.VirtualAddress + end] for start, end in runs],
                                 "length": len(aa)})
    results = Counter()
    examples = defaultdict(list)
    saved_ranges = []
    if same_base and same_sections:
        for line in canonical_metadata.read_text(encoding="utf-8").splitlines():
            row = json.loads(line)
            if row.get("record") != "function":
                continue
            chunks = []
            for region in row["body_ranges"]:
                start = int(region["start"], 16) - canonical.OPTIONAL_HEADER.ImageBase
                end = int(region["end"], 16) - canonical.OPTIONAL_HEADER.ImageBase
                saved_ranges.append((start, end))
                chunks.append((canonical.get_data(start, end - start + 1),
                               variant.get_data(start, end - start + 1)))
            label = "EXACT_BYTES_SAME_RVA" if all(a == b and a for a, b in chunks) else "CHANGED_BYTES_SAME_RVA"
            results[label] += 1
            if len(examples[label]) < 20:
                examples[label].append(row["rva"])
    merged = []
    for start, end in sorted(saved_ranges):
        if merged and start <= merged[-1][1] + 1:
            merged[-1][1] = max(merged[-1][1], end)
        else:
            merged.append([start, end])
    canonical.close()
    variant.close()
    return {"same_image_base": same_base, "same_section_address_size_file_offset_characteristics": same_sections,
            "raw_sections": raw_sections,
            "counts": dict(results), "bounded_examples": dict(examples),
            "saved_range_union_bytes": sum(end - start + 1 for start, end in merged),
            "saved_range_count": len(saved_ranges),
            "interpretation": "Exact bytes in saved functions do not establish whole-module equality. Changed bytes outside saved extents have unproven ownership."}


def export_prefixes(canonical_path: Path, variant_path: Path) -> dict:
    """A conservative export-name/first-byte lead; not a whole-function match."""
    a, b = pefile.PE(str(canonical_path)), pefile.PE(str(variant_path))
    def named(pe):
        return {s.name.decode("ascii", "replace"): s.address
                for s in pe.DIRECTORY_ENTRY_EXPORT.symbols if s.name and not s.forwarder}
    aa, bb = named(a), named(b)
    common = sorted(set(aa) & set(bb))
    exact16 = []
    exact32 = []
    for name in common:
        x, y = a.get_data(aa[name], 32), b.get_data(bb[name], 32)
        if len(x) == len(y) == 32 and x[:16] == y[:16]:
            exact16.append(name)
            if x == y:
                exact32.append(name)
    a.close()
    b.close()
    return {"common_named_exports": len(common), "canonical_only": len(set(aa) - set(bb)),
            "variant_only": len(set(bb) - set(aa)), "exact_first_16_bytes": len(exact16),
            "exact_first_32_bytes": len(exact32),
            "limitation": "Names and entry prefixes are leads; generic prologues and changed call operands can mislead."}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trim", required=True, type=Path)
    parser.add_argument("--trimb", required=True, type=Path)
    parser.add_argument("--trimc", type=Path)
    parser.add_argument("--canonical", required=True, type=Path)
    parser.add_argument("--canonical-functions", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() or PRIVATE.resolve() not in output.parents:
        parser.error("output must be a new file under private evidence")
    captures = [args.trim, args.trimb] + ([args.trimc] if args.trimc else [])
    capture_records = [read(capture) for capture in captures]
    catalog_identities = {capture["catalog_jsonl_sha256"] for capture in capture_records}
    if len(catalog_identities) != 1:
        raise ValueError("archive captures refer to different catalog identities")
    rows = [row for capture in capture_records for row in capture["rows"]]
    specimens = defaultdict(list)
    for row in rows:
        path = ARCHIVE / row["path"]
        if digest(path) != row["sha256"] or path.stat().st_size != row["size"]:
            raise ValueError("cataloged payload identity changed: " + str(path))
        specimens[row["sha256"]].append({**row, "absolute_path": str(path)})
    if digest(args.canonical) != CANONICAL:
        raise ValueError("canonical retail Trim identity mismatch")
    metadata = {}
    for sha, occurrences in sorted(specimens.items()):
        representative = Path(occurrences[0]["absolute_path"])
        metadata[sha] = {"pe": pe_metadata(representative), "occurrences": occurrences,
                         "classification": "BYTE_IDENTICAL_RETAIL" if sha == CANONICAL else "DISTINCT_TRIM_VARIANT"}
        if sha != CANONICAL:
            metadata[sha]["canonical_same_rva_windows"] = exact_windows(args.canonical_functions, args.canonical, representative)
            metadata[sha]["canonical_export_prefixes"] = export_prefixes(args.canonical, representative)
    for occurrence in metadata[CANONICAL]["occurrences"]:
        if occurrence["basename"].lower() == "trimb.dll":
            occurrence["classification"] = "BYTE_IDENTICAL_RENAMED"
    package = defaultdict(list)
    for sha, info in metadata.items():
        for row in info["occurrences"]:
            package[row["root_artifact"]].append({"name": row["basename"], "sha256": sha, "path": row["path"]})
    coexistence = [{"root_artifact": root, "members": sorted(members, key=lambda x: x["path"])}
                   for root, members in sorted(package.items())
                   if any(m["name"].lower() == "trim.dll" and m["sha256"] != CANONICAL for m in members)
                   and any(m["name"].lower() == "trimb.dll" and m["sha256"] == CANONICAL for m in members)]
    inputs = {"canonical": args.canonical, "canonical_functions": args.canonical_functions,
              "trim_capture": args.trim, "trimb_capture": args.trimb}
    if args.trimc:
        inputs["trimc_capture"] = args.trimc
    result = {"schema": SCHEMA, "version": VERSION, "status": "COMPLETE_ARCHIVE_CONTINUITY_NOT_RETAIL_BEHAVIOR",
              "tool_sha256": digest(Path(__file__)), "python_version": sys.version.split()[0],
              "pefile_version": pefile.__version__,
              "commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
              "catalog_sha256": catalog_identities.pop(),
              "inputs": {name: {"path": str(path.resolve()), "sha256": digest(path)} for name, path in inputs.items()},
              "specimens": metadata, "modified_trim_plus_canonical_trimb_packages": coexistence,
              "limitations": ["Archive provenance and exact bytes do not establish retail runtime behavior.",
                              "Saved same-RVA extents cover only part of the executable sections; changed windows do not have established function ownership.",
                              "Package co-presence does not prove simultaneous installation or runtime selection.",
                              "No historical executable was run; no V4 anchors or weights were changed."]}
    with output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(result, stream, sort_keys=True, separators=(",", ":"))
        stream.write("\n")
    print(json.dumps({"output": str(output), "sha256": digest(output),
                      "distinct_hashes": len(metadata), "coexistence_packages": len(coexistence),
                      "same_rva": {sha[:12]: info.get("canonical_same_rva_windows", {}).get("counts", {})
                                   for sha, info in metadata.items()}}, sort_keys=True))


if __name__ == "__main__":
    main()
