# Milestone 0: Evidence Baseline

> **Historical documentation — provenance only.** This document is not current project authority. See [PROJECT_BIBLE.md](../../../PROJECT_BIBLE.md) for canonical status; preserved for provenance.

This directory records the reproducible baseline used before retail-parity remediation.
Archived assistant notes are leads, not authority. A claim becomes authoritative here only
when it is reproduced from source, a retail binary, a source-built binary, or observed retail
behavior with a recorded provenance.

## Durable rollback point

The pre-remediation tracked working tree is preserved locally at:

`refs/checkpoints/milestone-0-pre-remediation`

Inspect it with:

```powershell
git show refs/checkpoints/milestone-0-pre-remediation
git diff HEAD refs/checkpoints/milestone-0-pre-remediation
```

The checkpoint commit does not include untracked files. Their paths and SHA-256 hashes are
recorded in `baseline-manifest.json`.

## Reproduce the baseline

From the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\Verification\Measure-Milestone0Baseline.ps1
```

The script inventories Debug, Final, and three retail reference trees; hashes the core game
binaries and object databases; extracts resource `PARSER/156` directly from each `Globals.dll`;
and compares the independent retail copies with the archived parser text and current source
parser.

`baseline-manifest.json` is generated evidence. `parser-resources/` contains the exact extracted
resource bytes. Machine-specific absolute paths are intentional because provenance is part of
this local recovery baseline.

The archived `retail_data_i.txt` is a CRLF-converted text copy, so its raw file hash does not
match the resource. The verifier separately normalizes CRLF to LF and requires the normalized
bytes to match the extracted retail resource exactly.

## Interpretation rules

- Matching independent retail hashes establish the current binary reference baseline.
- Debug and Final builds are separate subjects and must both be validated.
- A matching executable version string is not evidence of matching code or data.
- `Conquest.ini` is installation state, not a canonical retail identity marker.
- Byte differences in `gendata.db` are facts; their semantic meaning remains unproven until
  mapped through the independently verified parser layout.
- Archived Claude/Codex memory is never sufficient evidence on its own.
