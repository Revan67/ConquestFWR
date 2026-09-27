# Stable Baseline Remediation

> **Historical documentation — provenance only.** This document is not current project authority. See [PROJECT_BIBLE.md](../../../PROJECT_BIBLE.md) for canonical status; preserved for provenance.

## Current milestone plan — 2026-09-16

[Revised milestones](MILESTONES_2026-09-16.md) supersede the stage ordering below.
See ../../Stable-Baseline/TRACKING_BUILD_UPDATE_2026-09-14.md for completed Debug/Final build provenance;
older "Final pending" statements below are historical, not current build status.
Runtime acceptance remains separate. The HQ crash and D3D9 backend audit add
confirmed prerequisites before further broad playtesting.

Started 2026-09-12 from `master` at `4e44d182dfa938fb62e8bc6659fb3131e840651a`.

The complete meaningful pre-remediation working state is recoverable from:

`refs/checkpoints/stable-baseline-pre-remediation-20260912`

Transient Python bytecode was deliberately excluded. The three experimental loose planet
textures are retained in the checkpoint and under
`Evidence/Milestone-1/quarantined-runtime-assets/Debug`; they are not accepted runtime inputs.

## Evidence rules

1. Retail binary behavior, retail parser resources, and direct runtime traces are authoritative.
2. Released source is evidence, but not automatically retail: it includes unshipped CQ2 work.
3. Archived Claude notes are leads only and must be independently verified.
4. A successful build proves compilation and artifact identity, not runtime behavior.
5. Runtime acceptance must identify the exact build, test path, and observed result.

## Latest validation checkpoint — 2026-09-13

See [Parser conversion findings](../../Stable-Baseline/PARSER_CONVERSION_FINDINGS.md) for the latest candidate.
Three loader/schema defects are now independently reproduced and corrected: enum-to-integer
value loss, mission capabilities represented as an integer instead of a record, and compiler
assertions leaking into runtime parser input. Focused and complete retail-schema probes pass
with zero parser errors. Full Debug rebuild and 47-artifact hash-verified deployment passed.
Gameplay acceptance, flagship semantic compatibility, full offset coverage, fresh matching
traces and Final parity remain pending. This supersedes earlier deployment provenance below;
it does not promote unresolved visual or mining behavior to accepted status.

## Current truth at remediation start

### Accepted and preserved

- Toolbar resource alignment at 1600x1200.
- Top-right Options and Mission Objectives actions.
- Per-frame `RP_TEXTURE` ownership, which fixed one demonstrated white-planet failure mode.
- D3D9 depth creation and the grid depth-state boundary; world objects and map icons returned.
- Static target/talking ring.
- Ship engine glow and projectile/model-axis corrections for ordinary ships.
- Harvester assist packet length (`sizeof(buffer)`, not pointer size).
- Harvester beam/cargo hardpoint-axis correction, subject to a final cargo-material test.
- Construction lifecycle handoff; effects now instantiate.
- Expanded engine/campaign rebuild and deployment hash verification.

### Open or contradicted

- Planet initial appearance, surface/cloud stability, seam/smearing, and compound animation.
- Construction and placement-ghost materials.
- Scripted warp/jump orientation.
- Harvester movement, dock contention, unload completion, and autonomous recovery.
- Moon classification, moon-platform placement, halo, ambient, and terraforming behavior.
- D3D device reset/Alt-Tab lifecycle.
- Final-build parity with the latest Debug changes.
- Remaining parser/schema divergences.

## Change classification

| Group | Principal paths | Disposition |
|---|---|---|
| Accepted UI | `Menu_Toolbar.cpp`, `System.cpp`, `Sector.cpp` | Preserve; regression test |
| Accepted world-state repair | `ObjList.cpp`, depth creation in `Direct3D_pipe.cpp` | Preserve; reset path remains open |
| Accepted visual axes | projectile sources, `TObjGlow.h`, harvester hardpoint hunks | Preserve; scripted warp is separate |
| Construction candidate | `TObjBuild.h`, material-render paths | Lifecycle accepted; material behavior open |
| Mining experiment | behavioral hunks in `HarvestShip.cpp` | Remove unproven recovery changes; retain packet fix, attachments, diagnostics |
| Renderer candidate | MeshManager/PolyMesh/material changes | Keep isolated until consumer audit and visual acceptance |
| Diagnostics | planet, PolyMesh, texture, D3D and harvester logging | Debug-only and centrally gated; remove before Final acceptance |
| Generated artifacts | tracked DLL/LIB/PDB changes | Never use as source evidence; record via manifest |
| Documentation | `HANDOFF.md`, ledgers, test matrix | Reconcile to latest runtime result |

## Remediation stages

1. Recovery checkpoint and inventory — complete.
2. Truth-record reconciliation — in progress.
3. Split accepted fixes from experiments and generated artifacts.
4. Shared movement diagnosis and mining stabilization.
5. Scripted warp transform correction.
6. Complete planet compound/material trace.
7. Renderer consumer ownership and consolidation — Debug candidate deployed; runtime pending.
8. Full-build provenance for Debug and Final — schema-2 Debug build/deploy verified; Final pending.
9. Construction materials and placement ghosts — material-layout candidate deployed; runtime pending.
10. Retail moon/halo/ambient/terraform reconstruction.
11. Device-reset lifecycle repair.
12. Parser/schema parity ledger.
13. Expanded Debug and Final regression matrix.

## 2026-09-13 renderer ownership candidate

Consumer tracing showed that one renderer cannot safely replace every call site. Retail CFW world
objects and compound planets belong to `Engine -> RenderMgr -> PolyMesh`; construction effects,
placement ghosts, and particle mesh slices depend on MeshManager's per-call modifier list. The
previous broad redirect from `MeshInstance::Render()` into the engine path discarded those
modifiers and was removed. `Planetoid` now explicitly uses the engine instance for its world draw,
while retaining the MeshManager instance for animation, callbacks, hardpoints, and effects.

The application-side effect renderer also carried an unshipped vertex layout: it appended diffuse
RGBA data, advertised `D3DFVF_DIFFUSE`, and filled each vertex with random color. The library-side
CFW renderer has no such field. That mismatch directly fits the rainbow/untextured construction
geometry observed at runtime, so the extra field, FVF flag, and random color generation were
removed.

A complete Debug rebuild produced a schema-2 manifest from source revision
`4e44d182dfa938fb62e8bc6659fb3131e840651a`, fingerprint
`65E4517254AF90A80DFD427A44C8F887AE698EB1614806A664FB4615A43ED28F`, with 22 engine and 18
campaign modules. The guarded deploy revalidated the current inputs and artifact hashes, then
copied and verified 47 runtime artifacts. Compilation and deployment pass; visual behavior remains
unaccepted until the focused Mission 1 test is completed.
