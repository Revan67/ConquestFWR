#!/usr/bin/env python3
"""Conservatively recover x86 Trim resource-API arguments from local machine code.

Unproven stack/register paths fail closed. This does not infer function identity.
"""
from __future__ import annotations

import argparse
from bisect import bisect_left
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

import pefile
import v3_callgraph_context as machine


ROOT = Path(__file__).resolve().parents[3]
PRIVATE = ROOT / "Evidence/RetailMap/local"
API = {"dialogboxparama": ("RESOURCE_ID_PROVEN", 1, 5),
       "createdialogparama": ("RESOURCE_ID_PROVEN", 1, 5),
       "findresourcea": ("RESOURCE_ID_PROVEN", 1, 3),
       "loadmenua": ("RESOURCE_ID_PROVEN", 1, 2),
       "loadstringa": ("STRING_ID_PROVEN", 1, 4),
       "loadicona": ("RESOURCE_ID_PROVEN", 1, 2),
       "loadbitmapa": ("RESOURCE_ID_PROVEN", 1, 2)}
IMM = re.compile(r"^\$(-?0x[0-9a-f]+|-?\d+)$")
REG = re.compile(r"^%([a-z]{2,3})$")
STACK = re.compile(r"^0x([0-9a-f]+)\(%(esp|ebp)\)$")
DIRECT = re.compile(r"^0x([0-9a-f]+)(?:\s|$)")
ALIASES = {"eax": {"eax", "ax", "al", "ah"}, "ebx": {"ebx", "bx", "bl", "bh"},
           "ecx": {"ecx", "cx", "cl", "ch"}, "edx": {"edx", "dx", "dl", "dh"},
           "esi": {"esi", "si"}, "edi": {"edi", "di"},
           "ebp": {"ebp", "bp"}, "esp": {"esp", "sp"}}


def sha(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest().upper()


def source(value: str, preceding: list[tuple[int, str, str]]) -> dict:
    immediate = IMM.fullmatch(value)
    if immediate:
        return {"kind": "IMMEDIATE", "value": int(immediate[1], 0), "origin_rva": preceding[-1][0]}
    register = REG.fullmatch(value)
    if not register:
        return {"kind": "UNKNOWN_OPERAND", "operand": value}
    name = register[1]
    for at, op, operand in reversed(preceding[:-1]):
        parts = [x.strip() for x in operand.split(",")]
        if not parts or parts[-1] not in {"%" + alias for alias in ALIASES.get(name, {name})}:
            continue
        if parts[-1] != "%" + name:
            return {"kind": "REGISTER_PARTIAL_WRITE", "origin_rva": at, "opcode": op}
        if op.startswith("mov") and len(parts) == 2:
            nested = IMM.fullmatch(parts[0])
            if nested:
                return {"kind": "REGISTER_IMMEDIATE", "value": int(nested[1], 0), "origin_rva": at}
            stack = STACK.fullmatch(parts[0])
            if stack:
                return {"kind": "REGISTER_STACK_READ", "stack_offset": int(stack[1], 16),
                        "stack_base": stack[2], "origin_rva": at}
            return {"kind": "REGISTER_UNKNOWN_WRITE", "origin_rva": at}
        if op.startswith("xor") and len(parts) == 2 and parts[0] == parts[1]:
            return {"kind": "REGISTER_ZERO", "value": 0, "origin_rva": at}
        return {"kind": "REGISTER_UNKNOWN_WRITE", "origin_rva": at, "opcode": op}
    return {"kind": "REGISTER_ORIGIN_OUTSIDE_BLOCK", "register": name}


def analyze_call(site: int, instructions: dict, addresses: list, base: int,
                 argument_index: int, arity: int, owner: int):
    pos = bisect_left(addresses, site)
    if pos == len(addresses) or addresses[pos] != site:
        return {"classification": "AMBIGUOUS", "reason": "CALL_SITE_NOT_DISASSEMBLED"}
    block = []
    owner_pos = bisect_left(addresses, owner)
    for prior in reversed(addresses[max(owner_pos, pos - 48):pos]):
        op, operand = instructions[prior]
        if op.startswith(("call", "ret", "j", "loop", "int")):
            break
        block.append((prior - base, op, operand))
    block.reverse()
    pushes = [(i, x) for i, x in enumerate(block) if x[1].startswith("push")]
    if len(pushes) < arity:
        return {"classification": "AMBIGUOUS", "reason": "NOT_ENOUGH_LOCAL_PUSHES",
                "local_pushes": len(pushes), "api_arity": arity}
    first_argument_index = pushes[-arity][0]
    selected_index, selected = pushes[-1 - argument_index]
    first_arg_va = base + block[first_argument_index][0]
    for branch_site in addresses[owner_pos:pos]:
        opcode, operand = instructions[branch_site]
        if not (opcode.startswith("j") or opcode.startswith("loop")):
            continue
        direct = DIRECT.match(operand)
        if direct and first_arg_va < int(direct[1], 16) <= site:
            return {"classification": "AMBIGUOUS", "reason": "BRANCH_ENTERS_PARTIAL_ARGUMENT_SEQUENCE",
                    "branch_site_rva": branch_site - base, "branch_target_rva": int(direct[1], 16) - base}
    suffix = block[first_argument_index + 1:]
    forbidden = [(at, op) for at, op, operand in suffix
                 if op.startswith(("pop", "pushf", "leave"))
                 or ("%esp" in operand and not op.startswith("push"))]
    if forbidden:
        return {"classification": "AMBIGUOUS", "reason": "UNMODELED_STACK_MUTATION",
                "argument_push_rva": selected[0], "mutations": forbidden,
                "api_arity": arity}
    resolved = source(selected[2], block[:selected_index + 1])
    if resolved["kind"] == "REGISTER_STACK_READ":
        offset, register = resolved["stack_offset"], resolved["stack_base"]
        prologue = (instructions.get(owner) == ("pushl", "%ebp")
                    and instructions.get(owner + 1) == ("movl", "%esp, %ebp"))
        esp_first = register == "esp" and offset == 4 and resolved["origin_rva"] == owner - base
        ebp_first = register == "ebp" and offset == 8 and prologue
        if esp_first or ebp_first:
            return {"classification": "FORWARDED_PARAMETER", "parameter_index": 0,
                    "argument_push_rva": selected[0], "argument": resolved,
                    "basis": "entry-stack first parameter" if esp_first else "standard EBP frame first parameter"}
        return {"classification": "AMBIGUOUS", "reason": "STACK_READ_NOT_PROVEN_PARAMETER",
                "argument_push_rva": selected[0], "argument": resolved}
    if resolved.get("kind") not in {"IMMEDIATE", "REGISTER_IMMEDIATE", "REGISTER_ZERO"}:
        return {"classification": "AMBIGUOUS", "reason": resolved["kind"],
                "argument_push_rva": selected[0], "argument": resolved}
    value = resolved["value"]
    if not 0 < value <= 0xFFFF:
        return {"classification": "POINTER_OR_HANDLE" if value > 0xFFFF else "GENERIC_INTEGER",
                "reason": "NOT_MAKEINTRESOURCE_RANGE", "argument_push_rva": selected[0],
                "argument": resolved}
    return {"classification": "PROVEN_RESOURCE_ARGUMENT", "value": value,
            "argument_push_rva": selected[0], "argument": resolved,
            "local_pushes": len(pushes), "bounded_block_start_rva": block[0][0] if block else None}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--prior", required=True, type=Path)
    p.add_argument("--graph", required=True, type=Path)
    p.add_argument("--objdump", required=True, type=Path)
    p.add_argument("--output", required=True, type=Path)
    a = p.parse_args()
    output = a.output.resolve()
    if output.exists() or PRIVATE.resolve() not in output.parents:
        p.error("output must be a new file under private evidence")
    prior = json.loads(a.prior.read_text(encoding="utf-8"))
    graph_capture = json.loads(a.graph.read_text(encoding="utf-8"))
    modules = []
    for module in prior["modules"]:
        side = module["side"]
        image_record = prior["inputs"][side + "_image"]
        path = Path(image_record["path"])
        if sha(path) != image_record["sha256"] or sha(path) != module["module_sha256"]:
            raise ValueError(f"{side} image hash mismatch")
        pe = pefile.PE(str(path), fast_load=True)
        if pe.FILE_HEADER.Machine != 0x14C or pe.OPTIONAL_HEADER.Magic != 0x10B:
            raise ValueError("x86 PE32 required")
        base = pe.OPTIONAL_HEADER.ImageBase
        pe.close()
        instructions = machine.disassemble(path, str(a.objdump))
        addresses = sorted(instructions)
        graph_module = next(x for x in graph_capture["modules"]
                            if x[side + "_module_sha256"].upper() == module["module_sha256"])
        graph = graph_module[side + "_graph"]
        bounds = {int(k): v for k, v in graph["bounds"].items()}
        incoming = Counter(edge["target"] for edge in graph["edges"]
                           if edge.get("kind") == "DIRECT_CALL" and edge.get("target_is_function_entry")
                           and not edge.get("ownership_ambiguous"))
        outgoing = Counter(edge["owner"] for edge in graph["edges"]
                           if edge["instruction_kind"] == "call" and not edge.get("ownership_ambiguous"))
        calls = []
        for function in module["functions"]:
            for imported in function["imports"]:
                name = imported["name"].rsplit("!", 1)[-1].lower()
                if name not in API:
                    continue
                kind, index, arity = API[name]
                site = imported["site_rva"] + base
                call = instructions.get(site)
                if call is None or not call[0].startswith("call"):
                    result = {"classification": "AMBIGUOUS", "reason": "NOT_A_DECODED_CALL"}
                else:
                    result = analyze_call(site, instructions, addresses, base, index,
                                          arity, base + function["rva"])
                if result["classification"] == "PROVEN_RESOURCE_ARGUMENT":
                    result["classification"] = kind
                calls.append({"owner_rva": function["rva"], "site_rva": imported["site_rva"],
                              "api": imported["name"], "via_one_hop_thunk": imported["via_thunk"],
                              "result": result})
        owner_context = []
        for owner in sorted({x["owner_rva"] for x in calls}):
            va = base + owner
            bound = bounds.get(va)
            body = sum(end - begin for begin, end in bound["ranges"]) if bound else None
            direct_resource = [x for x in calls if x["owner_rva"] == owner]
            forwarded = [x for x in direct_resource
                         if x["result"]["classification"] == "FORWARDED_PARAMETER"]
            owner_context.append({"owner_rva": owner, "saved_body_bytes": body,
                                  "known_direct_callers": incoming[va], "all_machine_calls": outgoing[va],
                                  "resource_import_calls": len(direct_resource),
                                  "wrapper_candidate_only": bool(body and body <= 128 and incoming[va]
                                                                 and outgoing[va] <= 2),
                                  "forwarding_proven": bool(body and body <= 128 and len(direct_resource) == 1
                                                            and len(forwarded) == 1)})
        counts = Counter(x["result"]["classification"] for x in calls)
        modules.append({"side": side, "image_sha256": sha(path), "image_path": str(path),
                        "call_count": len(calls), "classifications": dict(sorted(counts.items())),
                        "owner_context": owner_context,
                        "calls": sorted(calls, key=lambda x: (x["owner_rva"], x["site_rva"], x["api"]))})
    result = {"schema": "cfw.trim-resource-arguments.v1", "status": "CONSERVATIVE_LOCAL_DATAFLOW",
              "tool_sha256": sha(Path(__file__)), "python": sys.version.split()[0],
              "commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
              "inputs": {"prior": {"path": str(a.prior.resolve()), "sha256": sha(a.prior)},
                         "graph": {"path": str(a.graph.resolve()), "sha256": sha(a.graph)},
                         "objdump": {"path": str(a.objdump.resolve()), "sha256": sha(a.objdump)}},
              "limits": ["Only directly observed imported API call sites from the prior image-pinned sidecar are evaluated.",
                         "Local straight-line push/register provenance only; unknown paths are ambiguous.",
                         "No wrapper forwarding or source correspondence is inferred from shared ID values."],
              "modules": modules}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, sort_keys=True, separators=(",", ":")) + "\n", encoding="utf-8")
    print(json.dumps({"output": str(output), "sha256": sha(output),
                      "counts": {m["side"]: m["classifications"] for m in modules}}, sort_keys=True))


if __name__ == "__main__":
    main()
