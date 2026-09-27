# ConquestFWR Project Bible

> **Canonical project knowledge.** This is the single living narrative source of project truth. Update in place. Historical notes are provenance, not parallel authority. Raw traces, parser resources, dumps, manifests, screenshots, and verification tools remain primary evidence.

Last updated: 2026-09-27. Status terms: **VERIFIED** (accepted and evidenced), **CANDIDATE** (implemented/supporting tests, acceptance pending), **OPEN** (unresolved), **CONTRADICTED** (disproved), **SUPERSEDED** (replaced by later evidence). Evidence type is stated separately from status. Compilation and hash matching prove artifact identity, not gameplay parity.

## 1. Project mission

Restore released retail Conquest: Frontier Wars behavior from an unshipped CQ2/Vyrium Uprising development source tree. Retail behavior is the specification; modernization belongs on a later branch. See `AGENTS.md` for rules.

## 2. Authority and evidence

Authority descends from direct retail behavior, retail binary analysis, retail TTD/debugging, shipped parser/data/assets and campaign DLL ABI, contemporary documentation, reconstructed runtime, current build graph/source, CQ2 source, then inference. Old Claude/Codex documents are navigation aids, not independent evidence. Resolve conflicts using later, more direct evidence; preserve unresolved contradictions here.

## 3. Retail reference baseline

- **VERIFIED (retail binary/hash):** Three independent retail installations had identical core EXE/DLLs and `GameTypes.db`/`GenData.db`; their `Conquest.ini` files differ and are not identity evidence. Retail EXE: 1,732,608 bytes, file version `01.07.10.1802`, product version `1.05`, SHA-256 `ED8741C0CB727A43B7DE7BE0E73566B04FBC1CBD58B54A62D538E3F39DCB3F52`. Source: `Evidence/Milestone-0/FINDINGS.md`, `baseline-manifest.json`.
- **VERIFIED (retail resource/hash):** `Globals.dll` `PARSER/156`: 164,530 bytes, SHA-256 `C8A1215C3422C75B2BD29D793C9E28FFA1BAF2DE5E0C244C20D26F7EE737860D`, identical across three copies. `Evidence/Milestone-0/parser-resources/GOG-retail-PARSER-156.bin` preserves bytes. Archived `retail_data_i.txt` matches after CRLF→LF normalization; generated `Code/App/Src/data.i` is not retail authority.
- **VERIFIED (retail data/hash):** Installed and retail-reference `GameTypes.db` match SHA-256 `20A454B8269140D824B3A42CEAC0412FBD31085550275AB4AB8A2C64BF173741` in the September 14 comparison. Retail `GenData.db` at `D:/Games/RetailRef/gendata.db` hashes `ADFA34D58F22163665EC0E9973A7D8E69A82166F265B22416EA2FF74E89A01C3`; the September 3 current deployment had 126 differing byte positions at baseline. Do not infer their semantic meaning from the count. `StringPack.db` identity/compatibility remains **OPEN**.
- Recheck external reference paths against the baseline manifest before relying on them; the reference database is not at repository-root `RetailRef/GameTypes.db`.

## 4. Repository and build baseline

- Current checkout at consolidation: branch `codex/stable-baseline-remediation`, HEAD `4e44d182dfa938fb62e8bc6659fb3131e840651a`, dirty tree. Recovery refs: `refs/checkpoints/milestone-0-pre-remediation` and `refs/checkpoints/stable-baseline-pre-remediation-20260912`. Untracked baseline hashes: `Evidence/Milestone-0/baseline-manifest.json`. September 17 targeted copies: `Evidence/Stable-Baseline/checkpoints/2026-09-17-pre-m1` (not a complete workspace backup). Three loose experimental planet textures are quarantined under `Evidence/Milestone-1/quarantined-runtime-assets/Debug`, not accepted runtime inputs.
- **VERIFIED (build/deployment proof only):** September 17 local / September 18 UTC full Debug and Final rebuilds covered 22 engine modules, five core binaries and 18 campaign modules each. Both current manifests have input fingerprint `01416AF4C6C0CC7D0339711153033E55C63FFDDCBBB45238E226FC6D1FFF43CB`; Final completed `2026-09-18T01:44:29Z`, Debug `2026-09-18T01:46:35Z`. Read-only September 27 recheck: installed Debug manifest exactly matches `Debug-latest.json` and all 47 installed artifacts match its hashes; deployed EXE is `04F2F5039C1A0F526DB8FF4F55AD195F4F1ECEE6DC12648DACDBDFB3CFAA030C`. **Final installation is older than `Final-latest.json`:** installed manifest completed September 14 20:06:51 UTC, fingerprint `CF7B7CEFD8193F389B52BC78E653A6BA4E1CCA989DB6E8A9F8FD899E5EF753E9`, EXE `41018703B1853B0ACF6C4A2986C90D65F4A01DDC14E3EBA7FBDEEC290AC985FD`; its 47 installed artifacts match that installed manifest. No claim of latest Final deployment or runtime parity. See `Evidence/Stable-Baseline/M0_M1_PROGRESS_2026-09-17.md` and `Build-Manifests/{Debug,Final}-latest.json`; reverify current hashes before deployment.
- **VERIFIED (build isolation):** Before correction, Final rebuilt Debug `COMHeap.lib`, `MathLib.lib`, `DACOM.dll`. Explicit `BuildProjectReferences=false` prevented this in the September 17 isolation test; Final then left these Debug files byte-identical. Older “Final pending” statements are obsolete.
- Full rebuilds are required; Debug and Final are separate runtime subjects. Campaign modules historically shared a build-output directory. /FORCE and intermediate-directory warnings need separate review. Current CRT-model changes require evidence; Debug DACOM /MTd versus COMHeap and Debug D3D pipe /MT is an **OPEN** heap-ownership lead, not a diagnosed crash cause.
- The guarded deploy computes a fingerprint over working-tree inputs, including these documentation edits. Although the installed Debug artifacts still match their saved manifest, a new deploy from this changed tree requires a new full build/manifest; do not bypass the fingerprint check.

## 5. Retail mission ABI

- **VERIFIED (symbol comparison):** The August 20 `dumpbin` three-way comparison found all 155 retail campaign-module imports from `Mission.dll`/`Trim.dll` satisfied (153 + 2). `Tools/abicheck/{symdiff,allmods}.ps1` reproduces it. Nine script-boundary types subsequently received measured compile-time size assertions and the symbol check still passed 155/155. This does **not** establish every field's semantic compatibility.
- **OPEN:** Four retail `wchar_t` names differ but are not imported by retail scripts; 28 extra CQ2 exports are not an active import failure. Field/enum semantics, `MGlobals::initMissionData` guard and silent-return changes need focused audit. See the correction at the end of `AUDIT_2026-08-20.md`. Re-run ABI and measured layout checks after export/signature/enum/save changes.

## 6. Retail data compatibility

- **VERIFIED (source + disk bytes):** `ObjectList::loadTypesData`/`GetArchetypeData` read `GameTypes.db` as raw consecutive records; `GenData.cpp::load_bytes` does likewise for `GenData.db`. No DocuView conversion/padding happens at these boundaries. Source/evidence: `ObjList.cpp`, `GenData.cpp`, `Evidence/Stable-Baseline/RAW_DATABASE_COMPATIBILITY.md`.
- **VERIFIED (probe/source):** `Mission::CorrelateSymbol` uses DocuView name-based correlation for mission/save data, zero-initializing destination fields. The original parser lost enum→integer and record→integer values; focused/full-schema probes reproduced failure, and corrected Debug/Final/installed DLL probes passed with zero parser diagnostics. Current `MISSION_SAVELOAD.caps` uses `MISSION_DATA_BIN::M_CAPS`; runtime storage was not shrunk. **CANDIDATE** for full gameplay/save acceptance, not a fix for raw databases. See `Evidence/Stable-Baseline/PARSER_CONVERSION_FINDINGS.md`, `Tools/Verification/ParserConversionProbe.cpp`.
- **VERIFIED (layout/bytes), OPEN (repair):** Twenty retail FSHIP records are 744 bytes each; current x86 `BT_FLAGSHIP_DATA` is 1384. Current consumers read command kits at offset 852 and starting formations at 1300, beyond a retail record, and interpret bonus fields differently even within the first 744 bytes. The 816-byte Blackwell character records are distinct and this does not explain inversion. Need retail representation, bounded adapter, semantics and value tests; padding alone fails. `Build-FlagshipLayoutProbe.cmd` is the layout check.
- **VERIFIED (layout/bytes), OPEN (repair):** Retail SpecialAbilities is 25 × 16 = 400 bytes, `USA_DOCK=24`, `USA_LAST=25`; current table is 512 bytes, `USA_DOCK=31`, `USA_LAST=32`, with menu indexing into raw data. Audit launcher/menu/network/save consumers before correcting IDs and bounds.
- **OPEN:** Other parser differences include M_CAPS, BASE_FIGHTER_SAVELOAD, SLOT, OPTIONS, BASE_GUNBOAT_SAVELOAD, BASE_REFINEPLAT_SAVELOAD, BASE_PLANET_SAVELOAD, AdmiralBonuses, BASE_FLAGSHIP_SAVELOAD and FLAGSHIP_SAVELOAD. Exact sizes/concerns are in `PARSER_CONVERSION_FINDINGS.md`. Some current-save fields are live; do not remove merely to match retail disk size.

## 7. Object-class and enumeration compatibility

- **VERIFIED (retail script/runtime):** Retail Mission 1 Mantis frigate object class is 73; CQ2 enum had moved `M_FRIGATE` to 90. Retail `M_OBJCLASS` order 0–110 was restored, sequel-only names placed after retail sentinel, critical IDs compile-time guarded. August 30 runtime then advanced combat and spawned Fabricator. See `Evidence/Historical/Root/HANDOFF.md` and `Code/App/Src` enum/mission sources. This does not validate every object class or ABI field.
- Other sensitive enumerations: `UNIT_SPECIAL_ABILITY` mismatch above; save parser enum/bitfield values require ongoing tests.

## 8. Rendering architecture

- **VERIFIED (build graph/runtime):** Primary authored CFW world path is `Object → Engine → RenderMgr → PolyMesh → ZBatcher → CQPipeline → D3D9`. MeshManager, MaterialManager and application manual rendering are front ends sharing backend state, not an independent second device. The duplicate library `MeshRender.cpp` is excluded from the game build. Tracking has 18 hooks but 14 compiled. PID 36200 recorded 31,402 PolyMesh and 2,036 application-manual entries before crash; overlapping calls are neither draw-share percentages nor visual correctness. See `RENDERER_USAGE_TRACKING.md`, `HQ_PREVIEW_CRASH_2026-09-15.md`.
- **VERIFIED for historical incident; current regression OPEN:** A working/failed A/B Earth trace found valid geometry and texture resource but global `RP_TEXTURE=0`, causing `RWM_DONT_TEXTURE`. Setting `RP_TEXTURE` from `CQEFFECTS.bTextures` at world-frame start yielded two immediately textured September 6 runs with successful bind/readback. September 17 Debug later again showed a white/grey planet. Do not claim all diffuse failures solved or reassign that historical A/B incident to asset loading. Determine the latest build's first failing material/state boundary. Evidence: `PLANET_TEXTURE_STATE_LEAK.md`, `RUNTIME_FEEDBACK_2026-09-17.md`.
- **VERIFIED for observed occlusion, lifecycle OPEN:** D3D9 path advertised depth but initially did not attach a surface. Automatic depth creation and real capability verification, plus grid draw with depth test/write disabled and restored, returned world objects and produced user-observed ring/limb occlusion. The older `DEPTH_LIFECYCLE_REPAIR.md` says runtime pending; later `CHANGE_LEDGER.md` records acceptance. Reset/Alt-Tab/Final coverage remains open.
- **VERIFIED (source analysis), acceptance OPEN:** Placement caller requests green/red RGBA with alpha; `FaceGroupRender::Render` legacy branch selects TFACTOR over bound texture/instance color, disables alpha blend, and mixes cached BATCH state with immediate PIPE draw. `SetCommonStates` exits before old setup; cleanup writes depth directly. This is a contract defect, but not GPU proof that it causes every white object. `WHITE_RENDERING_INVESTIGATION_2026-09-18.md`.
- **SUPERSEDED:** Early Queue2 claim that it skipped/repeated geometry was corrected: A/B/A source walk draws 0,1,2 once, though intended grouping is disabled. Direct PIPE mutation versus ZBatcher cache remains an **OPEN** state-ownership hazard.

## 9. D3D9 backend

**OPEN overall.** D3D9Ex backend is operational and remains the restoration backend; its conversion completeness cannot be expressed as a credible percentage. Source-confirmed risks in `D3D9_BACKEND_AUDIT_2026-09-16.md`: unbounded reset retry; reset depth/interval contract differing from create; repeated create overwrites prior device ownership; scratch IB lifetime; unchecked indexed-buffer allocation/lock and fixed 3,000-index limit; Present failure hidden behind success return; clear rectangle/color/depth mismatch; texture DC surface references unbalanced; startup/sampler failures under-propagated. These are not yet all reproduced runtime faults. Test create/reset/destroy, Alt-Tab/minimize, presentation and allocation failures, lifetime and both native/DXVK behavior narrowly. Preserve verified depth creation while repairing lifecycle.

## 10. Ship coordinate and transform contract

- **VERIFIED (source/runtime control):** `ObjectWarp::startWarpIn` creates planar gameplay orientation; `SpaceShip::renderSpaceShip` applies local +90° X and 180° Y model correction. Ordinary Skirmish-spawned ships appear correct. Mission scripted arrivals (Blackwell, escorts, Mantis, Travis) visibly begin inverted/nose-down and correct; ships built by Fabricator, Shipyard and Refinery appear vertical, including in Skirmish. These special entry/production boundaries are **OPEN**, not grounds for a global axis change. Compare first three frames with retail and inspect gameplay, warp-local, final display and producer preview transforms separately. `CONSTRUCTION_AND_SHIP_FINDINGS.md`.
- Engine glows, normal projectile direction and mining-beam origin had runtime-accepted corrections in `CHANGE_LEDGER.md`; cargo-panel material and Blackwell's unique authored turret/rear-fire arc remain open. Hardpoints, joints and effect transforms must not be conflated with gameplay orientation.

## 11. Construction and placement

- **CANDIDATE (repair), runtime crash acceptance OPEN:** HQ preview dump has null `mesh` dereference at `MeshRender.cpp:852`, after an invalid instance was cached; upstream platform legacy `archIndex=-1` while wrapper Engine index is 250. September 17 repair borrows wrapper handle for previews/shadow/blinkers, rejects failed creation and guards non-mesh fallback. Full Debug/Final builds passed, but repeated place/cancel/reload/shutdown/refcount acceptance is not recorded. Distinct from the older August 30 HQ extent-array crash, which was runtime-confirmed fixed. `HQ_PREVIEW_CRASH_2026-09-15.md`, `M0_M1_PROGRESS_2026-09-17.md`.
- **VERIFIED (lifecycle only):** Shared ObjectBuild handoff again instantiates Fabricator, HQ, Refinery, turret and harvester build effects. **CONTRADICTED (visual parity):** September 17 placement ghosts are white instead of green/red and animated geometry white/untextured. Valid versus invalid placement cannot be visually distinguished. Repair at material/state boundary, not with per-object colors. `CONSTRUCTION_AND_SHIP_FINDINGS.md`, `RUNTIME_FEEDBACK_2026-09-17.md`.
- **OPEN gameplay:** Stop during construction did not stop the operation; separately trace command dispatch, producer/target cancellation and completion. Constructed ship orientation is separate from build-material rendering. Construction save/reload acceptance unrecorded.

## 12. Planets and orbital rendering

- Geometry: **VERIFIED** Earth geometry/texture handle existed in historical A/B trace, not an asset-streaming delay there. Mission 1 source mission files matched retail archive and Earth archetype mapping.
- Diffuse: **OPEN current regression** despite September 6 `RP_TEXTURE` incident fix; September 17 user screenshot shows white/grey sphere. Establish first material/state divergence on that build.
- Ring/limb depth: **VERIFIED in observed Debug scenario**, Final and reset regression **OPEN**. See section 8.
- Initial timing, texture swapping/smearing/seams, cloud shell transparency/rotation, compound animation, moon/halo/terraform behavior: **OPEN**. Some visual changes may be authored layers; compare retail before labeling all as defects. Do not use loose texture overrides.

## 13. Combat

- Mission 1 class-ID progression correction **VERIFIED** for opening battle/Fabricator event (section 7). Final-wave `mantis_ships` counter synchronization and underflow guard are **CANDIDATE** pending full runtime gate; tutorial harvesting reminder is not a resource-threshold gate. `Evidence/Historical/Root/HANDOFF.md`.
- Normal ship facing, automatic target acquisition and projectile direction had scoped runtime acceptance; Blackwell turret arc remains **OPEN**. Combat renderer key-light crash had dump-proven out-of-bounds local-light swap and runtime-confirmed correction (August 30). HQ extent-array crash is distinct. Explosion mesh-shatter stabilization requires broader soak. Do not infer all combat missions pass.

## 14. Harvesting and refinery

- **VERIFIED scoped:** Assist packet used pointer size instead of buffer size; correction retained. Mining beam origin was observed corrected. Cargo-pod discoloration/attachment remains **OPEN**.
- **CONTRADICTED:** Claims that autonomous mining was fixed. September 17 trace shows two harvesters still holding valid node targets with mode 12, work values 2422/2333, final waypoint at the same X/Y but z=0 versus ship z=-400/-200, zero velocity, movement flags active and cargo 0/10. They are not simply failing target acquisition. Hypothesis: horizontal arrival versus vertical/operation completion mismatch; mode and caller/callee contract not proven. `runtime-2026-09-17-pid46456/harvester_state.txt`.
- Dock contention, unload/undock, resource reselection/depletion, partial loads, auto-recovery and save/load are **OPEN**. User observed both stalled and prior manual unload/ship destruction altered outcomes. Do not insert arbitrary retries. Acceptance target: two ships, ten consecutive independent cycles each, dock contention/depletion, stock/cargo consistency, then repeat after save/load.

## 15. UI

- **VERIFIED scoped:** Resource bar accepted at 1600×1200 using retail-identical `GenData.db`, relative alignment and one-unit control adjustment; font unchanged. `CHANGE_LEDGER.md`.
- **VERIFIED scoped:** Options and Mission Objectives buttons open. Source found shipped hotkey IDs `0x38/0x4E` versus generated CQ2 IDs `0x51/0x6A`; name-scoped compatibility mapping preserved geometry. `Evidence/Historical/Milestone-1/UI_RUNTIME_OBSERVATIONS.md`, later user acceptance.
- **SUPERSEDED:** Apparent title “localization glyph” is an intentional Debug marker, confirmed by user. No fix required. Other UI/resolution/faction coverage remains **OPEN**.

## 16. Startup, memory and crashes

- **CANDIDATE:** August 30 startup dump implicated DACOM main-heap allocation during DXVK-created worker thread attachment; main heap was switched to existing multithreaded mode. Repeated-launch soak not recorded. Do not present as fully accepted. `Evidence/Historical/Root/HANDOFF.md`.
- **OPEN separate assertion:** Debug `DACOM.dll` produced `_CrtIsValidHeapPointer(block)` at debug_heap.cpp:1059. No captured failing allocation/free stack; CRT model mismatch is a lead only. September 17 HQ AV dump is not that assertion. `M0_M1_PROGRESS_2026-09-17.md`.
- Two distinct HQ failures: August 30 completed-HQ extent-array indexing was runtime-confirmed repaired; September 15/17 placement-preview invalid instance and null material dereference have candidate repair pending repeated runtime acceptance. Do not merge them.
- Combat lighting AV fix was dump-backed and survived opening plus construction-triggered waves with clean exit. Explosion-shatter and other crash signatures remain scoped to recorded tests, not general stability.

## 17. Save/load compatibility

**OPEN overall.** The parser conversion probes pass representative retail/current `MISSION_SAVELOAD` fields (race 2, caps 0x300, pendingOp sentinel), signed enum and same-schema control with zero diagnostics. They use synthetic values, not a full retail mission/save round trip. Preserve current live save fields; raw DB layouts are a separate contract. Test actual retail/current saves, dock state, flagship semantics, construction mid-save/reload, mission state and Debug/Final builds.

## 18. Accepted fix ledger

Accepted within stated runtime scope, not universal parity:

| Fix | Evidence and acceptance | Regression boundary |
|---|---|---|
| Retail `M_OBJCLASS` sequence | Retail ID 73 versus CQ2 90; August 30 opening battle advanced to Fabricator | Mission 1 script events and all campaign DLL ABI |
| HQ completed-instance extent array | August 30 dump plus full structure build/clean exit | Placement shadow followed by completed compound HQ |
| Combat light-count bounds | August 30 dump plus full combat/construction-wave run/clean exit | High-light combat, explosions |
| Resource toolbar layout | User-accepted 1600×1200 position, unchanged font, retail DB | Other resolutions/races still needed |
| Toolbar action ID compatibility | User confirmed both buttons work | Names, hotkeys, neighboring controls |
| Planet `RP_TEXTURE` frame invariant | Working/failed A/B and two immediate-texture September 6 runs | Current white regression remains separate/open |
| D3D9 depth attachment + grid boundary | User observed ring/limb occlusion and world/map visibility restored | Reset/Alt-Tab/Final still open |
| Construction ObjectBuild handoff | Effects appear across multiple structures | Material/tint/Stop still fail |
| Ordinary glows/projectiles/static marker | Scoped user-observed runtime correction | Scripted arrival and Blackwell turret separate |
| Harvester assist packet length | Source-sized packet correction retained | Full mining AI not accepted |

Per-fix technical paths and original captures: `Evidence/Milestone-1/CHANGE_LEDGER.md`, `Evidence/Historical/Root/HANDOFF.md`, `PLANET_TEXTURE_STATE_LEAK.md`. Preserve scoped acceptance; never promote adjacent symptoms from these rows.

## 19. Candidates awaiting acceptance

| Candidate | Implemented/proof | Exact pending gate |
|---|---|---|
| September 17 preview ownership/null-guard repair | Both full builds/deploy hash proof; dump/source root cause | Repeated HQ/turret placement, cancel, construct, reload, shutdown, leak/refcount check |
| DocuView parser conversion and schema fix | Synthetic/full-schema Debug/Final probes pass | Real retail/current save and campaign value round trip |
| Startup multithreaded main heap | August 30 dump-backed source repair | Repeated cold launch under relevant backend; separate CRT assertion |
| Mission 1 final-wave counter | Source/trace-backed and deployed earlier | Complete final-wave/beacon objective run |
| Manual legacy material handling/layout | Source-supported candidate | Ghost color/alpha, construction texture, cargo, planet across cold/warm runs; latest Debug currently fails |
| D3D9 broader reset/lifetime work | Audit only, not implemented as accepted fix | Failure injection and mode/focus cycling before promotion |

Rollback boundaries are the recovery refs in section 4 plus the targeted pre-M1 copies for the September 17 preview changes; verify the exact current diff before using either.

## 20. Open investigations

Highest current defects: white placement ghosts/construction materials and planet diffuse regression; autonomous mining stall; preview crash runtime acceptance; raw flagship/special-ability record safety; D3D9 lifecycle; scripted arrivals/vertical production. Separate construction Stop, cargo/refinery discoloration, cloud/smear/motion and save/reload. No behavioral patch is justified solely by this consolidation. For each, establish retail control and first reconstructed divergence before changing code.

## 21. Restoration order

Plan, not completion; based on `Evidence/Historical/Stable-Baseline/MILESTONES_2026-09-16.md`.

1. **M0 build/evidence integrity:** preserve checkout, manifest provenance and Debug/Final isolation; known cross-config contamination closed, broader dirty-source reconciliation open.
2. **M1 preview ownership/crash:** candidate repair; repeated runtime placement/cancel/reload gate open.
3. **M2 retail data safety:** flagship and special ability raw-record hazards before broad gameplay conclusions; parser round trips separately.
4. **M3 D3D9 lifecycle:** bounded reset, resource ownership and failure propagation tests.
5. **M4 render-state/material contract:** trace white ghost/effect/planet at first failing state; preserve CFW primary draw and required manual consumers.
6. **M5 planet/object visual parity:** layers, smearing, timing, cargo/refinery materials, moons and retail comparisons.
7. **M6 construction/jump transforms:** authored effect materials, producer orientation, scripted arrivals, Stop and save/reload.
8. **M7 autonomous harvesting:** movement/operation boundary, docking and ten-cycle gate; research in parallel with renderer.
9. **M8 remaining UI/content:** only verified defects; debug marker is intentional.
10. **M9 CQ2 retirement:** only after consumer closure and retail replacements.
11. **M10 stable baseline gate:** full isolated Debug/Final rebuild/deploy, ABI/data probes, mission/skirmish matrix and soak. No milestone exit from compilation alone.

## 22. Retail-parity acceptance matrix

| Area | Debug | Final | Current evidence / missing gate |
|---|---|---|---|
| Full rebuild/deploy identity | PASS September 17 build/current installation | PASS latest build/isolation; installed Final is self-consistent but older September 14 build | Manifests, not runtime parity |
| Mission opening combat | PASS scoped (August 30) | NOT TESTED | Later build regression open |
| HQ completed construction crash | PASS scoped (August 30) | NOT TESTED | Distinct preview crash candidate |
| HQ preview repeat/cancel/reload | NOT TESTED after repair | NOT TESTED | September 17 candidate |
| Options/Objectives and resource bar | PASS scoped | NOT TESTED | 1600×1200 accepted |
| Planet diffuse | FAIL latest observed Debug | NOT TESTED | September 17 white/grey; historical RP_TEXTURE fix remains |
| Ring/limb depth | PASS scoped Debug | NOT TESTED | Reset/focus open |
| Placement ghost tint/alpha | FAIL | NOT TESTED | White, valid/invalid indistinguishable |
| Construction material | FAIL | NOT TESTED | Animation/lifecycle present, white |
| Scripted arrival/produced orientation | FAIL | NOT TESTED | Ordinary Skirmish spawn control correct |
| Mining autonomous cycles | FAIL | NOT TESTED | Two target-holding stalled harvesters |
| Parser conversion probe | PASS synthetic | PASS synthetic | Real save round trip open |
| Save/load, startup soak, D3D9 reset | NOT TESTED | NOT TESTED | Separate gates |

PASS is bounded by its recorded build/run; it is not a claim of current universal parity.

## 23. Raw evidence index

- **Retail identity/parser:** `Evidence/Milestone-0/baseline-manifest.json`, `parser-resources/*PARSER-156.bin`; verifier `Tools/Verification/Measure-Milestone0Baseline.ps1`.
- **Retail binary research:** `D:/Dev/ghidra-proj/CQRetail.gpr` and `.rep` exist; its `Conquest_retail.exe` hashes `ED8741C0CB727A43B7DE7BE0E73566B04FBC1CBD58B54A62D538E3F39DCB3F52`, matching the retail EXE baseline. Decompiled exports/callgraph are present, but individual script/session provenance is not fully mapped. `D:/Dev/traces` contains ten `.run` TTD recordings; `.out` companions establish process/session/time for many, and several `.idx` files exist. Individual trace-to-loaded-image/PDB identity and finding map remains **OPEN**; these are older recordings, not evidence for current rebuilt addresses.
- **Data/schema tools:** `Compare-ParserSchemas.ps1`, `ParserConversionProbe.cpp`, `Build-FlagshipLayoutProbe.cmd`, `FlagshipLayoutProbe.cpp`, `Tools/abicheck/*`.
- **Crash dumps:** `CrashDumps/Conquest.exe_260830_210620.dmp` (HQ extent), `260830_220036.dmp` (light bounds), `260915_202400.dmp` and `260917_144331.dmp` (preview null read); hashes/build identities recorded in respective historical analysis, not assumed from names.
- **Build/deploy:** `Build-Manifests/Debug-latest.json`, `Final-latest.json`, `Debug-isolation-before-final.json`; September 17 targeted pre-M1 copies under `Evidence/Stable-Baseline/checkpoints`. Manifest “latest” can change; archive identity per run.
- **Runtime trace/captures:** `Evidence/Stable-Baseline/runtime-2026-09-17-pid46456` contains manifest snapshot, harvester/startup logs, renderer counters and four screenshots. It is a live snapshot, not clean-exit totals. Earlier PolyMesh Earth A/B values survive in `PLANET_TEXTURE_STATE_LEAK.md`, but the original raw log files were not located in the repository; re-establish their build/run identity before new analysis from them.
- **Renderer tracking:** `Tools/Verification/Read-RenderUsage.ps1`, `Build-RenderUsageProbeTest.cmd`; 14 compiled sites, route counters overlap and cannot yield GPU utilization.

### Provenance audit — September 27 (read-only)

**Ghidra.** `D:/Dev/ghidra-proj/analyze.log` identifies a headless import of `/Conquest_retail.exe` into `CQRetail.gpr/.rep`; logged MD5 `3DF7F6B586D93FDED1A42756EB621995` and independently measured SHA-256 `ED8741C0CB727A43B7DE7BE0E73566B04FBC1CBD58B54A62D538E3F39DCB3F52` match the canonical retail EXE. No separate retail game DLL import was found in this project's top-level import log (Windows dependency imports are present). `export.log` names `/Conquest_retail.exe`, `scripts/ExportCallGraph.java` and output `callgraph.tsv`; source script and export remain present, so the callgraph is reproducible from the saved project with the logged headless script. `DecompileAt.java`, `FindOffsetUsers.java`, `ExportFpuProfile.java`, exported `.c`/`.tsv` and logs are present. The Bible does not currently cite an exact Ghidra function address as proof; older notes mention yaw/pitch/roll/angle and path offsets, which must be linked to a specific export before reuse. No evidence of a different retail EXE imported into this Ghidra project was found.

**TTD classification.** All ten `.run` files below are reconstructed-game captures, not retail traces: recorder output names the GOG Debug or GOG-Final install or an attached process, and replay output shows rebuilt game modules/PDB source paths. Their capture-time EXE SHA-256 cannot be recovered from present recorder output; current installed hashes must not be assigned retroactively. Dates are local in August 2026. `Conquest01/02/10.run.err` are empty failed recording attempts, not retained `.run` traces. Classification is a future cleanup recommendation only; no trace moved or deleted.

| Trace | Recorder/replay provenance | Issue and unique fact currently evidenced | Classification |
|---|---|---|---|
| `path.run` | Aug 21, attach PID 64020; `p1out`–`p10out` replay, GOG Debug module path | Path movement and `SetPath`/`doPathMove` state inspected; older build, not a current harvester trace | RETAIN UNIQUE |
| `Conquest03.run` | Aug 28 19:27, GOG Debug launch; no issue-specific replay found | Recorder/exited process only; no durable unique conclusion identified | ARCHIVE |
| `Conquest04.run` | Aug 28 19:27, GOG Debug launch; no issue-specific replay found | Recorder/exited process only; no durable unique conclusion identified | ARCHIVE |
| `Conquest05.run` | Aug 28 19:28, GOG Debug launch; `c1out`–`c8out` replay | `doPathMove` call sequence/count and path-state investigation; supports historical movement analysis, not September 17 mining root cause | RETAIN UNIQUE |
| `Conquest06.run` | Aug 28 23:23, GOG Debug launch; `c9out`/`c10out` exception replay | Older exception inspected; no current unique root-cause conclusion mapped | ARCHIVE |
| `Conquest07.run` | Aug 28 23:32, attach PID 75432; no issue-specific replay located | Attached reconstructed process; issue not identified | ARCHIVE |
| `Conquest08.run` | Aug 28 23:40, attach PID 64516; `c11out` replay | Older crash stack; no distinct current finding mapped | ARCHIVE |
| `Conquest09.run` | Aug 29 09:50, attach PID 56740; `c12out`–`c16out` replay | `MeshExplode.cpp` `GetCenterOfMass`/`SplitInstance` exception analysis; unique historical crash evidence, broader runtime acceptance still open | RETAIN UNIQUE |
| `Conquest11.run` | Aug 29 10:46, GOG-Final launch, exit 0; no issue-specific replay found | Final clean-run control only; does not validate current Final parity | ARCHIVE |
| `Conquest12.run` | Aug 29 10:46, attach PID 57540; `c17out`/`c18out` replay loads GOG-Final modules | Breakpoint/exception replay; whether same process/build as `Conquest11` is unproven | ARCHIVE |

The three `RETAIN UNIQUE` traces preserve old movement/explosion investigations; none is `RETAIN ACTIVE` for the current white-rendering, HQ-preview or September 17 mining defects. Reclassify only if replay reveals a unique unresolved fact. Preserve `.idx`, `.out`, replay outputs and matching symbol snapshots with any retained trace. `symbols-current/bin/Conquest.exe` is a reconstructed snapshot hash `F409BEBA6408B076BE51BB4EFBE47F040657388E9EAA45950FEA4F8594E81C0D`, but it was not proven to be the loaded image in any specific `.run` and must not be used as such.

**Planet A/B capture.** `D:/Games/GOG/Conquest Frontier Wars/Debug/` retains later September 7 logs. `polymesh_earth_facegroup.txt` (SHA-256 `D40E58B02FB8552DEB5E3EFF6DFAEF61BF2A0F8FE7C20DF825E2EC5EFD076D24`) directly contains two successful-run handles `346d2e50` and `346dd2e0`, first entries with `rwm=0`, valid 534/528 indices, successful setter and matching readback, as the incident report says. `polymesh_earth_boundary.txt` (SHA-256 `D77316A853CFE76294B2E927E274DD7AFD5F791A094B642C2C21E83DB69D045C`) and `planet_texture_timing.txt` (SHA-256 `5EA3424B472BCEDFA01E78681E48A82FDD1122E4BDC63D3901DCD6341D723BD7`) corroborate successful Earth routing/loading. The installed manifest has since changed, so these logs' exact build hash remains unrecovered. `renderbatcher_texture_boundary.txt` contains the older working handle `34dc0760`, but does not alone capture the failing Earth `rwm=1` boundary; handle `34e107c0` was not found in searched repository, Debug/Final installations, trace text, or temp logs. **PRIMARY FAILED-RUN CAPTURE NOT CURRENTLY LOCATED — conclusion preserved by contemporaneous evidence report.** Retain `PLANET_TEXTURE_STATE_LEAK.md`; do not substitute successful-run logs for the original A/B proof.

**Crash-dump identities.** The four Bible-cited dumps exist. `260830_210620` (900,048,334 bytes, SHA-256 `F313E537F9B7D278CD5246D4E0672B4F730E2C28904C26F47E6F16362489D2AF`) and `260830_220036` (885,543,751 bytes, `404E38E773CD7EF93A9C45093BA7DEA9DB89331605BA27CA70E955B4011AB19E`) have fault/build interpretation in `Evidence/Historical/Root/HANDOFF.md`, but that note lacks an exact loaded EXE hash; reproduce their precise binary identity before address-level reuse. `260915_202400` (788,393,810 bytes, `8B06A48D365F4B576A1EAB93243AE7CA4CE1318F6785AC50CCCD381915D529E4`) has loaded EXE hash `54F03434B8F14A3B50F75999694888B83912BBE8E13C88F0A266627ACA79FD64`, PE timestamp/RVA/fault bytes in `HQ_PREVIEW_CRASH_2026-09-15.md`. `260917_144331` (756,661,883 bytes, `6D90C295D0651E069688AEBD53E8FC2B8D53FD22DAA748B3F287BEB1770D7AE1`) repeats the same fault instruction and pre-change EXE/DACOM hashes in `M0_M1_PROGRESS_2026-09-17.md`; it is not a dump of the later CRT assertion. Other dumps remain physically present but were not cited by current Bible conclusions.

**Manifests and tools.** `*-latest.json` is mutable; the installed `build-manifest.json` is an immutable-in-place snapshot only until the next deploy. Record future run copies as `Build-Manifests/<Configuration>-<buildCompletedUtc-basic>-<buildInputFingerprint-prefix>.json` (or equivalent) and retain their SHA-256; this is a recommendation, not a build change. Current Debug and Final identities/installation mismatch are in §4. All named baseline, parser comparison/probe, flagship layout, ABI, renderer-counter and build/deploy verification scripts exist. Their source matches the claimed purpose: `Build-Conquest.ps1` writes fingerprinted manifests; `Deploy-Conquest.ps1` checks revision/input/artifact hashes and verifies copies. Historical scripts may depend on local VS/SDK/reference paths; existence/source inspection is not a fresh execution result, and no tool was rewritten in this pass.

## 24. Superseded findings

| Old claim | Later evidence / canonical result |
|---|---|
| “Both MeshRender copies are live” | Project/binary audit: library duplicate excluded; only application manual source participates in game build. Section 8. |
| “Queue2 skips/repeats A/B/A geometry” | Corrected control-flow walkthrough draws each once; grouping still ineffective. Section 8. |
| “Final pending” in September 13 stable-baseline notes | September 14 and 17 full Final build/hash verification; Final runtime parity still open. Section 4. |
| “Parser/name conversion pads raw flagship data” | Actual `GameTypes.db` loader performs raw consecutive reads; DocuView is mission/save contract only. Section 6. |
| “HQ invalid instance reason unknown” | September 16 dump/upstream walk found stale platform -1 versus wrapper 250; null read still needed repair. Section 11. |
| “Planet pop-in is necessarily late texture loading” | Historical working/failed A/B proved `RP_TEXTURE` state leak for that incident; latest white regression remains open. Sections 8/12. |
| “Planet diffuse solved generally” | September 17 Debug white/grey recurrence; historical fix accepted only for its A/B failure mode. Section 12. |
| “Construction visuals fixed because effects appear” | September 17 effects move but are white/untextured; lifecycle only accepted. Section 11. |
| “Harvesters lost their target” | September 17 trace shows valid targets and active movement with zero velocity; operation boundary unresolved. Section 14. |
| “Escape title glyph is localization corruption” | User confirmed intentional Debug marker. Section 15. |
| “One HQ fix covers all HQ crashes” | August 30 extent-array and September 15/17 preview invalid-instance faults are distinct. Section 16. |
| “Nine script ABI types still lack assertions” | The August 20 audit's own final section records measured assertions added and 155/155 symbols rechecked. Section 5. |

## 25. Historical-document disposition

Approved cleanup completed on 2026-09-27. The seven historical narratives are archived under `Evidence/Historical/` with provenance banners and original filenames. The 18 technical narratives remain in their evidence locations with evidence banners; their historical status statements do not compete with this Bible. `README.md` points readers to this Bible for restoration status. Legal/licensing files remain active outside project-state consolidation.

| Document | Disposition | Reason |
|---|---|---|
| `README.md` | KEEP ACTIVE | User-facing setup/legal overview; restoration status points to Bible |
| `MODIFICATIONS.md`, `LEGAL.md` | KEEP ACTIVE | Legal/compliance obligations, not competing restoration status |
| `Evidence/Historical/Root/HANDOFF.md` | ARCHIVE | Historical run chronology; corrections consolidated |
| `Evidence/Historical/Root/RETAIL_TEST_MATRIX.md` | ARCHIVE | Superseded test-status narrative; current matrix in §22 |
| `AUDIT_2026-08-20.md` | RETAIN AS EVIDENCE | Unique ABI symbol/layout investigation |
| `Evidence/Historical/Milestone-0/README.md` | ARCHIVE | Baseline procedure summarized; raw manifest/tool kept |
| `Evidence/Milestone-0/FINDINGS.md` | RETAIN AS EVIDENCE | Retail baseline/hashes and checkpoint provenance |
| `Evidence/Milestone-1/CHANGE_LEDGER.md` | RETAIN AS EVIDENCE | Build hashes and scoped runtime fix provenance |
| `Evidence/Milestone-1/RENDER_PIPELINE_AUDIT.md` | RETAIN AS EVIDENCE | Historical architecture/source audit; corrected claims flagged |
| `Evidence/Historical/Milestone-1/RENDERER_CONSUMER_INVENTORY.md` | ARCHIVE | Consumer summary consolidated; source audit remains |
| `Evidence/Milestone-1/PLANET_TEXTURE_STATE_LEAK.md` | RETAIN AS EVIDENCE | Unique working/failed A/B values and acceptance |
| `Evidence/Milestone-1/DEPTH_LIFECYCLE_REPAIR.md` | RETAIN AS EVIDENCE | Attachment implementation and artifact hashes; pending status superseded |
| `Evidence/Milestone-1/CONSTRUCTION_AND_SHIP_FINDINGS.md` | RETAIN AS EVIDENCE | Distinct runtime controls and transform/source details |
| `Evidence/Historical/Milestone-1/UI_RUNTIME_OBSERVATIONS.md` | ARCHIVE | Button finding consolidated; debug-marker correction carried |
| `Evidence/Historical/Stable-Baseline/README.md` | ARCHIVE | Historical handoff and stale stage/pending states |
| `Evidence/Historical/Stable-Baseline/MILESTONES_2026-09-16.md` | ARCHIVE | Plan now in §21 |
| `Evidence/Stable-Baseline/TRACKING_BUILD_UPDATE_2026-09-14.md` | RETAIN AS EVIDENCE | Build/deploy isolation and hash provenance |
| `Evidence/Stable-Baseline/RENDERER_USAGE_TRACKING.md` | RETAIN AS EVIDENCE | Instrumentation/build graph and Queue2 correction |
| `Evidence/Stable-Baseline/CFW_RENDERER_RECONNECTION_SCOPE_2026-09-16.md` | RETAIN AS EVIDENCE | Source call-site/consumer mapping; 12 packages were budget, not broken-connection count |
| `Evidence/Stable-Baseline/D3D9_BACKEND_AUDIT_2026-09-16.md` | RETAIN AS EVIDENCE | Unique backend control-flow/line anchors |
| `Evidence/Stable-Baseline/HQ_PREVIEW_CRASH_2026-09-15.md` | RETAIN AS EVIDENCE | Dump identity, fault bytes, upstream handle values |
| `Evidence/Stable-Baseline/RAW_DATABASE_COMPATIBILITY.md` | RETAIN AS EVIDENCE | Disk/runtime sizes and raw loader proof |
| `Evidence/Stable-Baseline/PARSER_CONVERSION_FINDINGS.md` | RETAIN AS EVIDENCE | Reproduction, schema table, probe details |
| `Evidence/Stable-Baseline/M0_M1_PROGRESS_2026-09-17.md` | RETAIN AS EVIDENCE | Preview implementation and build identity |
| `Evidence/Stable-Baseline/RUNTIME_FEEDBACK_2026-09-17.md` | RETAIN AS EVIDENCE | Latest build-tied visual/mining observations |
| `Evidence/Stable-Baseline/WHITE_RENDERING_INVESTIGATION_2026-09-18.md` | RETAIN AS EVIDENCE | First source-identified material/state defects |
| `Code/Libs/ExplicitDLL/REBUILD_LOG.md` | RETAIN AS EVIDENCE | Build provenance; verify against manifests before promotion |

Other source-tree readmes, licenses, TODOs, model/material TXT, speech TXT and raw logs are not competing project-state narratives and are outside cleanup. Keep verification scripts, parser resources, Ghidra/TTD work, dumps, manifests and unique captures.

Disposition count (table entries as files): **ARCHIVE 7; RETAIN AS EVIDENCE 18; DELETE CANDIDATE 0; KEEP ACTIVE 3.** All seven approved archive moves and 18 evidence banners were completed. Active documentation paths and relative links affected by the moves were updated; a repository-wide Markdown relative-link check found no broken links. No planned archive move was deferred. Raw-evidence retention recommendations, including TTD classifications, were not executed as part of this cleanup.

## 26. Session closeout

2026-09-27 documentation-only consolidation and primary-evidence provenance audit. Inventoried repository narratives and evidence references, reconciled later corrections (including the August ABI audit's own completion note), and distributed durable conclusions by subsystem. The README's stale status table was replaced with a Bible pointer. Retail parser and Ghidra EXE hashes were independently checked; ten TTD recordings classified without deletion; four cited dumps hashed; all named verification tools located. The September 7 successful Earth raw logs were found, but the original failed-run PolyMesh capture is not currently located. Installed Debug matches its latest manifest; installed Final matches an older September 14 manifest, not the latest Final output. Current runtime failures and Final parity remain explicit.

The subsequently approved documentation cleanup moved the seven §25 historical narratives to `Evidence/Historical/`, added historical/evidence banners, updated the Bible and README references, and repaired affected relative links. All 39 checked Markdown relative links and 54 checked Bible path references resolved. No raw evidence, game source, runtime assets, build outputs, or verification tools were intentionally modified by this cleanup. The repository had pre-existing non-documentation working-tree changes; they remain outside this cleanup. No discrepancy required a departure from the approved disposition. Restoration implementation has not resumed; the next technical task remains for user selection.
