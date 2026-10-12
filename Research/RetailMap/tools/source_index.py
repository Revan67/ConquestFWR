#!/usr/bin/env python3
"""Independent source inventory for correspondence research; never compiles code.

Project membership is obtained from evaluated MSBuild ClCompile items for the
supported Build-Conquest.ps1 workflow. Function/features are lexical C++ leads,
NOT AST facts, compiler symbols, reachability, or accepted retail matches.
Only Python's standard library and VS2022 MSBuild 17.8+ are required.
"""
from __future__ import annotations

import argparse
import bisect
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

VERSION = "1.0"
TOKEN = re.compile(
    r"(?P<comment>//[^\n]*|/\*.*?\*/)"
    r'|(?P<string>(?:u8|[LuU])?"(?:\\.|[^"\\])*"|(?:[LuU])?\'(?:\\.|[^\'\\])*\')'
    r"|(?P<id>[A-Za-z_$][\w$]*)"
    r"|(?P<number>\b(?:0[xX][\da-fA-F]+|0[bB][01]+|\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)[uUlLfF]*)"
    r"|(?P<op>::|->|&&|\|\||==|!=|<=|>=|\S)", re.S)
CONTROLS = {"if", "for", "while", "switch", "catch", "sizeof", "alignof",
            "decltype", "__declspec", "static_assert", "return", "new", "delete"}
NS = {"m": "http://schemas.microsoft.com/developer/msbuild/2003"}


def digest(data):
    return hashlib.sha256(data).hexdigest().upper()


def relative(p, root):
    try:
        return Path(p).resolve().relative_to(root).as_posix()
    except ValueError:
        return str(Path(p).resolve()).replace("\\", "/")


def solution_projects(solution, config, platform):
    text = solution.read_text(encoding="utf-8-sig")
    projects = {}
    for match in re.finditer(r'Project\("[^"\n]+"\) = "[^"]+", "([^"]+\.vcxproj)", "([^"\n]+)"', text):
        projects[match[2].upper()] = (solution.parent / match[1].replace("\\", "/")).resolve()
    selected = []
    prefix = re.escape(config + "|" + platform)
    for match in re.finditer(r"\s*(\{[^}]+\})\." + prefix + r"\.Build\.0\s*=\s*([^|\r\n]+)\|([^\r\n]+)", text):
        if match[1].upper() in projects:
            selected.append((projects[match[1].upper()], match[2].strip(), match[3].strip()))
    return selected


def workflow_projects(root, configurations, platform):
    """Read explicit library lists and solution Build.0 entries from workflow."""
    script = (root / "Build-Conquest.ps1").read_text(encoding="utf-8-sig")
    libraries = []
    for group in ("foundationProjects", "engineProjects"):
        match = re.search(r"\$" + group + r"\s*=\s*@\((.*?)\n\)", script, re.S)
        if not match:
            raise ValueError("Unsupported Build-Conquest workflow: cannot locate " + group)
        libraries.extend((root / p.replace("\\", "/")).resolve()
                         for p in re.findall(r"Join-Path \$repoRoot '([^']+\.vcxproj)'", match[1]))
    def mapped_configuration(variable, config):
        match = re.search(r"\$" + variable + r"\s*=\s*if\s*\(\$Configuration -eq '([^']+)'\)\s*\{\s*'([^']+)'\s*\}\s*else\s*\{\s*'([^']+)'\s*\}", script)
        if not match:
            raise ValueError("Unsupported configuration mapping: " + variable)
        return match[2] if config == match[1] else match[3]
    solutions = []
    for name in ("solution", "campaignSolution"):
        match = re.search(r"\$" + name + r"\s*=\s*Join-Path \$repoRoot '([^']+)'", script)
        if not match:
            raise ValueError("Unsupported workflow solution variable: " + name)
        solutions.append((name, (root / match[1].replace("\\", "/")).resolve()))
    result = set()
    for config in configurations:
        engine_config = mapped_configuration("engineConfiguration", config)
        result.update((p, engine_config, platform) for p in libraries)
        for name, solution in solutions:
            selected_config = mapped_configuration("campaignConfiguration", config) if name == "campaignSolution" else config
            selected = solution_projects(solution, selected_config, platform)
            if not selected:
                raise ValueError(f"No Build.0 projects found: {solution} {selected_config}|{platform}")
            result.update(selected)
    return sorted(result, key=lambda row: (str(row[0]).lower(), row[1], row[2]))


def find_msbuild():
    for edition in ("Enterprise", "Professional", "Community"):
        p = Path(f"C:/Program Files/Microsoft Visual Studio/2022/{edition}/MSBuild/Current/Bin/MSBuild.exe")
        if p.is_file():
            return p
    p = Path("C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/MSBuild/Current/Bin/MSBuild.exe")
    if p.is_file():
        return p
    raise FileNotFoundError("VS2022 MSBuild not found; specify --msbuild")


def evaluated_project(msbuild, project, config, platform, root):
    command = [str(msbuild), str(project), "-nologo", "-verbosity:quiet",
               "-getItem:ClCompile,ProjectReference", "-getProperty:TargetName,TargetExt,ConfigurationType",
               "-p:Configuration=" + config, "-p:Platform=" + platform,
               "-p:BuildProjectReferences=false"]
    # Windows environment keys are case insensitive; collapse inherited PATH/Path.
    env = {k.upper(): v for k, v in os.environ.items()}
    query = subprocess.run(command, cwd=root, env=env, capture_output=True, text=True,
                           encoding="utf-8", errors="replace", timeout=120)
    if query.returncode:
        raise RuntimeError(f"MSBuild query failed: {project} {config}: {query.stdout}\n{query.stderr}")
    data = json.loads(query.stdout)
    items = []
    for item in data.get("Items", {}).get("ClCompile", []):
        full = item.get("FullPath") or str(project.parent / item["Identity"])
        items.append({"file": relative(full, root),
                      "excluded": item.get("ExcludedFromBuild", "false").lower() == "true",
                      "preprocessor_definitions": item.get("PreprocessorDefinitions", ""),
                      "include_directories": item.get("AdditionalIncludeDirectories", ""),
                      "compile_as": item.get("CompileAs", "Default")})
    refs = [relative(item.get("FullPath") or project.parent / item["Identity"], root)
            for item in data.get("Items", {}).get("ProjectReference", [])]
    return {"project": relative(project, root), "configuration": config, "platform": platform,
            "sha256": digest(project.read_bytes()), "properties": data.get("Properties", {}),
            "membership_evidence": "MSBuild evaluated ClCompile; no targets executed",
            "references": sorted(refs), "items": sorted(items, key=lambda x: x["file"])}


def lexical_functions(text):
    """Balanced token spans, with conservative function-header recognition.

    Preprocessing is intentionally not performed: inactive branch candidates are
    retained and marked as such in the schema. No overload/call resolution.
    """
    # Hide directives while preserving offsets/line numbers; macro-expanded
    # function definitions remain deliberately outside lexical coverage.
    masked = re.sub(r"(?m)^[ \t]*#[^\n]*(?:\n(?<=\\\n)[^\n]*)*",
                    lambda m: re.sub(r"[^\r\n]", " ", m.group()), text)
    tokens = [m for m in TOKEN.finditer(masked) if m.lastgroup != "comment"]
    value = [m.group() for m in tokens]
    pairs, stack = {}, []
    for i, v in enumerate(value):
        if v in ("(", "{", "["):
            stack.append(i)
        elif v in (")", "}", "]"):
            expected = {")": "(", "}": "{", "]": "["}[v]
            if stack and value[stack[-1]] == expected:
                start = stack.pop()
                pairs[start] = i
                pairs[i] = start
    lines = [-1] + [m.start() for m in re.finditer("\n", text)]
    scopes = []
    for i, v in enumerate(value):
        if v != "{" or i not in pairs:
            continue
        begin = i - 1
        while begin >= 0 and i - begin < 128 and value[begin] not in (";", "{", "}"):
            begin -= 1
        prefix = value[begin + 1:i]
        for keyword in ("class", "struct", "namespace"):
            if keyword in prefix:
                j = prefix.index(keyword) + 1
                if j < len(prefix) and re.fullmatch(r"[A-Za-z_]\w*", prefix[j]):
                    scopes.append((i, pairs[i], prefix[j]))
                break
    functions = []
    occupied_until = -1
    for i, v in enumerate(value):
        if v != "{" or i <= occupied_until or i not in pairs:
            continue
        # Search back to a structural separator. Bound header scanning explicitly.
        begin = i - 1
        while begin >= 0 and i - begin < 256 and value[begin] not in (";", "{", "}"):
            begin -= 1
        begin += 1
        # First parameter-like parentheses, skipping declaration annotations.
        candidate = None
        for j in range(begin, i):
            if value[j] != "(" or j not in pairs or pairs[j] >= i or j == 0:
                continue
            name_index = j - 1
            name = value[name_index]
            if name in CONTROLS or tokens[name_index].lastgroup != "id":
                continue
            first = name_index
            if first > begin and value[first - 1] == "~":
                first -= 1
            while first >= begin + 2 and value[first - 1] == "::" and tokens[first - 2].lastgroup == "id":
                first -= 2
            prefix = value[begin:first]
            if any(x in prefix for x in ("=", "return", "if", "for", "while", "switch", "typedef")):
                continue
            tail = value[pairs[j] + 1:i]
            if any(x in tail for x in ("=", ";")):
                continue
            candidate = (first, j, pairs[j])
            break
        if candidate is None:
            continue
        first, params, params_end = candidate
        end = pairs[i]
        header = text[tokens[begin].start():tokens[i].start()].strip()
        # Preprocessor directives may precede definitions; preserve explicit limits.
        header = re.sub(r"(?m)^\s*#.*$", "", header).strip()
        name = "".join(value[first:params])
        if "::" not in name:
            enclosing = [scope[2] for scope in scopes if scope[0] < i < scope[1]]
            if enclosing:
                name = "::".join(enclosing + [name])
        body = tokens[i + 1:end]
        strings = sorted({m.group() for m in body if m.lastgroup == "string"})
        numbers = sorted({m.group() for m in body if m.lastgroup == "number"})
        calls = set()
        for j in range(i + 1, end - 1):
            if tokens[j].lastgroup == "id" and value[j + 1] == "(" and value[j] not in CONTROLS:
                start = j
                while start >= i + 3 and value[start - 1] in ("::", "->", ".") and tokens[start - 2].lastgroup == "id":
                    start -= 2
                calls.add("".join(value[start:j + 1]))
        functions.append({"name": name, "signature": " ".join(header.split()),
                          "line_start": bisect.bisect_left(lines, tokens[begin].start()),
                          "body_line_start": bisect.bisect_left(lines, tokens[i].start()),
                          "line_end": bisect.bisect_left(lines, tokens[end].start()),
                          "strings": strings, "numeric_constants": numbers,
                          "outgoing_call_candidates": sorted(calls),
                          "source_span_sha256": digest(text[tokens[begin].start():tokens[end].end()].encode("utf-8")),
                          "extraction": "lexical-candidate", "preprocessor_branch": "unresolved"})
        occupied_until = end
    return functions


def self_test():
    sample = '''// fake() { }\nint A::f(int x) { if (x) { sink("text { }", 0x20); } return helper(x); }\nvoid g() { const char* s = "fake() { }"; /* bad() {} */ a->work(12); }\n'''
    result = lexical_functions(sample)
    assert [x["name"] for x in result] == ["A::f", "g"], result
    assert result[0]["line_start"] == 2 and result[1]["line_end"] == 3
    assert result[0]["numeric_constants"] == ["0x20"]
    assert result[0]["outgoing_call_candidates"] == ["helper", "sink"]
    assert result[1]["outgoing_call_candidates"] == ["a->work"]
    assert lexical_functions('void h(); int x = factory() { 0 };') == []
    scoped = lexical_functions('#include "x.h"\nstruct A { void f() { call(); } };\n')
    assert scoped[0]["name"] == "A::f" and scoped[0]["line_start"] == 2
    return {"status": "pass", "checks": 7,
            "scope": "synthetic comments/strings/braces/control rejection/constants/call leads"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument("--output", type=Path)
    parser.add_argument("--jsonl", type=Path, help="Optional normalized function-candidate JSONL")
    parser.add_argument("--msbuild", type=Path)
    parser.add_argument("--configurations", nargs="+", default=["Debug", "Final"])
    parser.add_argument("--platform", default="Win32")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    tests = self_test()
    if args.self_test:
        print(json.dumps(tests, sort_keys=True))
        return
    if not args.output:
        parser.error("--output is required; use a local Evidence path, not public retail evidence")
    root = args.root.resolve()
    msbuild = args.msbuild or find_msbuild()
    selected = workflow_projects(root, args.configurations, args.platform)
    projects = []
    for index, (project, config, platform) in enumerate(selected, 1):
        print(f"[{index}/{len(selected)}] {relative(project, root)} {config}", file=sys.stderr)
        projects.append(evaluated_project(msbuild, project, config, platform, root))
    memberships = {}
    for project in projects:
        for item in project["items"]:
            memberships.setdefault(item["file"], []).append({
                "project": project["project"], "configuration": project["configuration"],
                "platform": project["platform"], "excluded": item["excluded"]})
    project_paths = {p[0] for p in selected}
    excluded_targets = []
    for project in sorted((root / "Code").rglob("*.vcxproj")):
        if project.resolve() in project_paths:
            continue
        tree = ET.parse(project)
        items = [relative(project.parent / x.attrib["Include"].replace("\\", "/"), root)
                 for x in tree.findall(".//m:ClCompile", NS) if "Include" in x.attrib]
        excluded_targets.append({"project": relative(project, root),
                                 "classification": "outside-supported-full-build-workflow",
                                 "membership_evidence": "unevaluated XML item list only",
                                 "declared_compile_files": sorted(items)})
    # Inventory source files even when they are absent from all active project items.
    sources = {relative(p, root) for p in (root / "Code").rglob("*")
               if p.is_file() and p.suffix.lower() in (".cpp", ".c", ".cxx", ".cc")}
    sources.update(memberships)
    files, functions = [], []
    for file in sorted(sources):
        p = root / file
        member = memberships.get(file, [])
        active = any(not x["excluded"] for x in member)
        record = {"file": file, "memberships": member,
                  "classification": "compiled-translation-unit" if active else
                  ("excluded-from-build" if member else "not-in-supported-build-items")}
        if not p.is_file():
            record["error"] = "missing source file"
            files.append(record)
            continue
        raw = p.read_bytes()
        record["sha256"] = digest(raw)
        # Only active translation units feed function candidates; excluded file inventory remains explicit.
        if active:
            try:
                text = raw.decode("utf-8-sig")
                record["encoding"] = "utf-8-sig"
            except UnicodeDecodeError:
                text = raw.decode("cp1252", errors="replace")
                record["encoding"] = "cp1252-with-replacement"
            extracted = lexical_functions(text)
            record["function_candidates"] = len(extracted)
            for f in extracted:
                f["file"] = file
                f["id"] = f"{file}:{f['line_start']}:{f['name']}"
            functions.extend(extracted)
        files.append(record)
    head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
    status = subprocess.check_output(["git", "status", "--porcelain=v1", "--untracked-files=no"], cwd=root, text=True)
    dirty_paths = set(subprocess.check_output(
        ["git", "diff", "HEAD", "--name-only", "-z"], cwd=root).decode("utf-8").split("\0"))
    file_hashes = {f["file"]: f.get("sha256") for f in files}
    for f in functions:
        f["source_blob_sha256"] = file_hashes[f["file"]]
        f["worktree_dirty"] = f["file"] in dirty_paths
        f["worktree_identity"] = "sha256:" + f["source_blob_sha256"]
    report = {"schema": "cfw-source-index-v1", "tool_version": VERSION,
              "tool_sha256": digest(Path(__file__).read_bytes()), "head": head,
              "source_commit_role": "baseline HEAD only; source_blob_sha256 identifies actual indexed worktree bytes",
              "tracked_worktree_status": status.splitlines(),
              "tracked_dirty_files": [{"path": p, "sha256": digest((root / p).read_bytes()) if (root / p).is_file() else None}
                                      for p in sorted(dirty_paths) if p],
              "build_workflow_sha256": digest((root / "Build-Conquest.ps1").read_bytes()),
              "msbuild_version": subprocess.check_output([str(msbuild), "-nologo", "-version"], text=True).strip(),
              "configurations": args.configurations, "platform": args.platform,
              "validation": tests, "limitations": [
                  "MSBuild evaluated items prove target membership, not runtime reachability or emitted linker symbols.",
                  "Lexical balanced-token function recognition is not a C++ parser; templates, operators, macro definitions, constructor initializers and inline class scope can be missed or misidentified.",
                  "No preprocessing: inactive #if branches and configuration-dependent bodies remain unresolved candidates.",
                  "Only active translation-unit text is indexed; header-inline/template definitions and generated macro bodies are not expanded.",
                  "worktree_dirty is file-local, not transitive include dirt; tracked_dirty_files also identifies dirty headers with exact SHA256.",
                  "Call candidates include casts/macros; virtual dispatch, overloads and function pointers are unresolved.",
                  "String/number spelling is retained without C++ escape evaluation, macro expansion or constant folding.",
                  "Projects outside Build-Conquest workflow are inventoried from unevaluated XML, not claimed compiled.",
                  "No claim of retail correspondence or behavioral acceptance follows from this index."],
              "counts": {"project_configuration_queries": len(projects), "active_projects": len(project_paths),
                         "excluded_targets": len(excluded_targets), "files": len(files),
                         "file_classifications": dict(sorted(Counter(f["classification"] for f in files).items())),
                         "function_candidates": len(functions)},
              "projects": projects, "excluded_targets": excluded_targets,
              "files": files, "functions": functions}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=True, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    if args.jsonl:
        args.jsonl.parent.mkdir(parents=True, exist_ok=True)
        with args.jsonl.open("w", encoding="utf-8", newline="\n") as stream:
            for f in functions:
                member = [m for m in memberships[f["file"]] if not m["excluded"]]
                row = {"source_id": f["id"], "path": f["file"],
                       "qualified_name": f["name"], "signature": f["signature"],
                       "start_line": f["line_start"], "end_line": f["line_end"],
                       "compile_target": sorted({m["project"] for m in member}),
                       "configurations": sorted({m["configuration"] for m in member}),
                       "strings": f["strings"], "numeric_constants": f["numeric_constants"],
                       "calls": f["outgoing_call_candidates"], "fingerprint": f["source_span_sha256"],
                       "source_commit": head, "extraction": "lexical-candidate",
                       "source_blob_sha256": f["source_blob_sha256"],
                       "worktree_dirty": f["worktree_dirty"],
                       "worktree_identity": f["worktree_identity"],
                       "preprocessor_branch": "unresolved"}
                stream.write(json.dumps(row, sort_keys=True) + "\n")
    print(json.dumps({"counts": report["counts"], "output_sha256": digest(args.output.read_bytes())}, sort_keys=True))


if __name__ == "__main__":
    main()
