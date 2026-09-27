# Parser conversion investigation

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

Status: regression reproduced; focused and complete-schema correction tests pass in the
rebuilt Debug parser. Debug candidate deployed and hash-verified; gameplay acceptance pending.

## Evidence and corrected interpretation

The retail parser resource is `Evidence/Milestone-0/parser-resources/GOG-retail-PARSER-156.bin`.
Current definitions are `Code/App/Src/data.i`. `Compare-ParserSchemas.ps1` inventories named
struct/class text differences, including nested declarations. It is a discovery tool, not
a C++ layout verifier: it does not resolve scopes, inheritance, aliases or bitfield packing.

There are 14 differing common blocks before this pass. The earlier proposal to remove runtime
fields from three save records was withdrawn before compilation/deployment. Offset differences
alone do not establish corrupt loading: `Mission::CorrelateSymbol` delegates to DocuView,
which clears the destination and maps record members by name. Removing persisted fields would
also discard their state across saves. The three original schemas and runtime storage remain.

## Reproduced conversion failures

`Tools/Verification/ParserConversionProbe.cpp` loads the actual DACOM and DocuView DLLs without
launching Conquest. Initial reproduction used equal 12-byte records, with an enum-to-integer
race field, record-to-integer capability field and a trailing integer sentinel:

- Input race: 2. Output race: 0.
- Input capability bits: 0x00000300. Output capability bits: 0.
- Trailing 0x12345678 sentinel survived.
- Same-schema conversion preserved all three fields.

The initial process returned 1 (conversion failure), with both record sizes verified as 12.
This establishes a parser conversion defect; it does not yet establish which reported game
symptoms it causes or which mission data takes this conversion path.

`ViewConstructor::CorrelateSymbol` zeroes the destination before conversion. In `DataView.cpp`,
`CopySymValue` converts an enum to its label, then asks `SetSymValue` to parse that label into
the destination. Integer parsing uses `strtoul`, so a label such as MANTIS becomes zero.
The same function has no source RECORDSYM branch; a record supplied to an integer destination
therefore receives no copy. The outer conversion interface returns void, hiding partial failure.

## Candidate correction

1. Preserve numeric enum values when converting to integer schema fields. Keep existing
   name-based enum-to-enum conversion for schemas whose enumeration values changed.
2. Describe `MISSION_SAVELOAD.caps` as `MISSION_DATA_BIN::M_CAPS` in the parser schema.
   That is its declaring scope. The C++ runtime field remains `MISSION_DATA::M_CAPS`.
   Both refer to the same record; the change preserves field semantics during correlation.
3. Test retail conversion, same-schema conversion and conversion of previous integer-caps
   saves. Parse both complete schemas using the same DLL before accepting the change.

## Remaining 14-block ledger

| Block | Concern / required validation |
|---|---|
| MISSION_DATA | Full retail record is 72 bytes. Current 40-byte prefix plus inheritance needs parser offset and value verification. Stale comments saying the retail record is 40 bytes were corrected. |
| MISSION_SAVELOAD | 104-byte C++ assertion exists; race enum conversion and caps record semantics require value tests. |
| M_CAPS | Two extra capability bits occupy retail spare bits; verify old flags and defaults rather than deleting live consumers. |
| BASE_FIGHTER_SAVELOAD | Three enum bitfields became integers for compiler packing; affected by enum conversion defect. |
| SLOT | Enum bitfields became integers for compiler packing; affected by enum conversion defect. |
| OPTIONS | Signed enum/bitfield adaptations require negative-value conversion tests as well as offsets. |
| BASE_GUNBOAT_SAVELOAD | Launcher operation IDs and artifact flag added. Keep persistence until load/save policy and conversion are verified. |
| BASE_REFINEPLAT_SAVELOAD | Default harvest stance inserted before dock state; correlation may handle displacement. Test dock and stance values. |
| BASE_PLANET_SAVELOAD | Three regeneration boosts appended; correlation zero-initializes absent source fields. |
| AdmiralBonuses | CQ2 changed both shape and names; retail field names cannot map automatically to the new bonus semantics. |
| BT_FLAGSHIP_DATA | Retail bonuses/hotkey replaced by command-kit/formation fields; requires explicit retail interpretation and runtime adapters. |
| BASE_FLAGSHIP_SAVELOAD | CQ2 formation and command-kit state appended; audit initial defaults and serialized compatibility. |
| FLAGSHIP_SAVELOAD | Added build queue is consumed by TBuildQueue; removing it requires an explicit runtime/save policy. |
| GT_SPECIALABILITIES | Extra entries inserted before transfer; audit symbolic conversion versus raw indexing and enum ordering. |

## Other seven-step work

The full-schema probe also reproduced ten parser errors for leaked C++ `static_assert`
declarations in generated `data.i`. `DSpaceship.h` and `DPlatform.h` now guard those checks
with `#ifndef _ADB`, preserving compiler checks while excluding them from parser input.
This is a separate defect from enum conversion. The parser APIs return success/non-null
tables even after diagnostics, so the probe additionally counts error diagnostics.

Focused corrected-DLL results: retail conversion PASS (race 2, caps 0x300, sentinel intact),
same-schema control PASS, previous integer-caps save PASS, signed three-bit enum -2 PASS.

Size measurements repeated after clean full-schema parsing (parser-derived; compiler
offsets still require validation):

| Record | Current bytes | Retail bytes |
|---|---:|---:|
| MISSION_DATA | 72 | 72 |
| MISSION_SAVELOAD | 104 | 104 |
| BASE_GUNBOAT_SAVELOAD | 40 | 28 |
| BASE_REFINEPLAT_SAVELOAD | 40 | 36 |
| BASE_PLANET_SAVELOAD | 304 | 292 |
| BASE_FIGHTER_SAVELOAD | 44 | 44 |
| BASE_FLAGSHIP_SAVELOAD | 104 | 36 |
| FLAGSHIP_SAVELOAD | 616 | 460 |
| BT_FLAGSHIP_DATA | 1384 | 744 |
| GT_SPECIALABILITIES | 512 | 400 |
| CQGAMETYPES::OPTIONS | 8 | 8 |
| CQGAMETYPES::SLOT | 8 | 8 |

Full-schema verification returned exit code 0 with zero parser error diagnostics. Nested
OPTIONS/SLOT lookup uses the parser namespace structure, not the top-level-only GetSymbol
API. Both complete schemas parse, and the following values survive conversion of the
actual retail MISSION_SAVELOAD record into the current record:

| Field | Retail offset | Current offset | Preserved value |
|---|---:|---:|---:|
| race | 40 | 40 | 2 |
| caps | 70 | 70 | 0x00000300 |
| pendingOp | 100 | 100 | 0x12345678 |

These test values are synthetic, not bytes extracted from a live mission. They prove the
conversion mechanism and representative fields; they do not establish the cause of a
particular mining or rendering failure. The source/destination size is checked as 104
before writing either test buffer. All three regression cases, the same-schema control,
the signed enum case and full-schema diagnostics are included in the process exit status.

Reproduction commands (from repository root):

```powershell
cmd.exe /c Tools\Verification\Build-ParserConversionProbe.cmd
& "$env:TEMP\CFW-ParserConversionProbe.exe" `
  'D:\Dev\CFW\Code\Libs\ImplicitDLL\debug\DACOM.dll' `
  'D:\Dev\CFW\Code\Libs\ExplicitDLL\debug\DocuView.dll' `
  'D:\Dev\CFW\Code\App\Src\data.i' `
  'D:\Dev\CFW\Evidence\Milestone-0\parser-resources\GOG-retail-PARSER-156.bin'
```

- Runtime acceptance of the deployed renderer candidate remains pending a focused game run.
- Existing reverse engineering is at `D:/Dev/ghidra-proj`; TTD recordings are at `D:/Dev/traces`.
  Those recordings are tied to older executables/symbols and cannot validate current addresses.
- Retail Enable3DMode call sites exist in the Ghidra call graph. Current startup calls
  Start3DMode directly and performs device, buffer, batching and sampler initialization.
  Restoring the disabled historical function wholesale is not supported by this evidence.
- Fresh planet/harvester TTD captures still require a reproduction using the final candidate
  binaries and their matching PDBs.
- Export presence and parser record compatibility are distinct checks; a satisfied import
  table does not prove that data passed through that interface has the expected values.

## Flagship follow-up: semantic incompatibility, not a padding fix

2026-09-14 correction: tracing the actual archetype loader shows GameTypes.db is read as
raw consecutive records, not passed through this conversion path. See
[Raw database compatibility](RAW_DATABASE_COMPATIBILITY.md) for the verified 744-byte
installed flagship records versus 1384-byte runtime consumers and their out-of-record
offsets. The name-mapping discussion below describes why a generic converter would also
be insufficient; it is not the current raw loader's behavior.

Retail `BT_FLAGSHIP_DATA` declares `admiralHotkey` and a member named `bonuses`. Its
`AdmiralBonuses` contains `generalBonuses`, `specialShipBonus`, `bonusShips[5]`, and six
special-ability booleans. The current record instead declares `admiralBonueses` (that
spelling), command kits, starting formations and queue size. Its bonus record uses
`baseBonuses`, `favoredShipBonus`, `favoredArmor`, and race/armor damage modifiers.

Downstream consumers in `Flagship.cpp` read `flagData->admiralBonueses` for damage,
defence, speed, supply, sensors and range (functions around lines 3415–3738). Generic
name-based correlation cannot translate retail `bonuses` into that different name and
shape. Matching the total size, deleting appended fields, or applying the numeric-enum
fix does not solve this. No speculative flagship rewrite was included in this candidate.
An explicit retail schema/runtime adapter and value tests are still required; shield and
special-ability semantics must be traced before choosing their mappings.

## Candidate provenance and next acceptance gate

Final Debug rebuild completed 2026-09-13 at 17:44:34 UTC, exit 0. All 22 engine modules and
18 campaign modules rebuilt and verified. Build-input fingerprint:
`1E1C439D5E2BE5B6BBC73BCF27FB5B0FC6DBB761BDEECDF21F80F2BE410FCDBF`.
The complete parser probe was repeated against these outputs and returned 0 with zero
diagnostic errors. Scoped `git diff --check` passed for the four changed implementation
headers/source files. Existing compiler deprecation/project-path warnings remain; this
is not a warning-free build.

`Deploy-Conquest.ps1 -Configuration Debug` then validated source-input identity and artifact
hashes, deployed to `D:\Games\GOG\Conquest Frontier Wars`, and reported 47 artifacts
hash-verified, exit 0. No game process was running. No Final candidate was rebuilt/deployed
in this pass. Full manifest: `Build-Manifests/Debug-latest.json`.

Required gameplay gate (not performed by this probe):

1. Cold Mission 1 start: planet/ship appearance timing and stable materials through camera motion.
2. Construction and placement: fabricator, HQ/refinery and laser-turret ghost materials.
3. Two harvesters completing several independent cycles, including dock contention and node depletion.
4. Resign/reload the mission and repeat initialization; confirm clean exit.

Record failures against this exact build. Fresh build-matched traces, full compiler/parser
offset coverage, explicit flagship compatibility and Final parity remain open. Passing
synthetic conversion tests does not close the seven-step remediation plan.
