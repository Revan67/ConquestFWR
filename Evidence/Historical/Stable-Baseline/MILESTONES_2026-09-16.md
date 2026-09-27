# Stable baseline milestones — revised 2026-09-16

> **Historical documentation — provenance only.** This document is not current project authority. See [PROJECT_BIBLE.md](../../../PROJECT_BIBLE.md) for canonical status; preserved for provenance.

This sequencing supersedes earlier remediation stage ordering, not the evidence
or accepted fixes in those documents. This is a plan, not a claim of completion.
Preserve the current D3D9Ex backend and restore verified CFW content semantics.
Do not swap in retail DLLs or delete CQ2-dependent services without replacements.

## M0 — Reproducible checkpoint and evidence reconciliation (P0)

Inventory dirty source, deployed EXE/DLLs, assets, launchers, configuration, captures
and current build manifests. Preserve the HQ dump and accepted UI/depth/texture
state repairs. Separate accepted fixes from unresolved experiments. Update stale
"Final pending" notes using the September 14 build report, without treating build
success as visual acceptance. Fix or explicitly isolate the known Final-build
modification of Debug COMHeap.lib, MathLib.lib and DACOM.dll.

Exit: repeatable Debug/Final full rebuild/deploy manifest verification; building
one configuration does not silently invalidate the other. Current reproductions
and rollback checkpoint are identifiable. No broad git reset or asset replacement.

## M1 — HQ crash and archetype/instance ownership (P0)

Resolve platform archIndex=-1 versus valid MeshArch index 250, not just the null
read. Define borrowed/owned handles, matching references and teardown. Audit ship,
platform and planet initialization, shared ObjectTransform callbacks, preview and
shadow creation. Reject failed creation before caching or manipulating previews.
Move mesh-dependent material work behind valid-instance/geometry handling while
preserving valid non-mesh fallback behavior.

Exit: HQ and turret previews, place/cancel/repeat, construction, resign/reload and
shutdown work without invalid handles, invisible fallback or reference imbalance.
Audit blinker and platform shadow consumers of the same legacy field.

## M2 — Retail data and save compatibility (P0 where memory safety; otherwise P1)

Keep raw database records distinct from name-correlated save records. Verify the
known flagship record-size/field mismatch and SpecialAbilities/DockID table-index
mismatch before their consumers can read beyond retail data. Use explicit retail
record representations, checked bounds and semantic adapters where needed; do
not shrink live runtime/save structures based only on byte differences.

Complete field/value probes for remaining parser/schema differences, including
dock state, stance, bonuses, capabilities and negative enum values. Preserve the
already tested enum/capability/parser corrections and validate installed DLLs.

Exit: no known out-of-bounds raw-record consumers, deterministic value-preserving
loads, and supported retail/current-save round trips. This gate prevents malformed
state from being misdiagnosed as rendering or mining behavior. Research can run
alongside M1; relevant unsafe consumers must be fixed before broad gameplay tests.

## M3 — D3D9 device/resource lifecycle (P1)

Replace unbounded reset retry with bounded, classified failure handling. Define
create/reset/destroy contracts and resource ownership. Preserve depth/stencil and
presentation settings across transitions; revalidate flags and invalidate caches.
Propagate startup/Present failures; check index-buffer allocations, locks, bounds
and helper returns. Balance texture-DC surface references. Audit texture/VB/IB,
effect and state-block lifetime. Fix clear rectangle/color/depth contract after
caller tests. Keep current D3D9Ex/DXVK assumptions explicit; native parity is a
separate validation result, not inferred from DXVK success.

Exit: bounded failure-injection tests; repeated create/reset/shutdown and mode
switches; Alt-Tab/minimize/restore without hang or resource growth; allocation and
presentation failures reported safely. Do not ask the user to reproduce an
unbounded hang before adding a controlled test path.

## M4 — One coherent render-state/material contract (P1)

Keep normal CFW drawing on Engine/RenderMgr/PolyMesh. Audit five MeshInstance draw
fallbacks and required effect exceptions. Trace direct PIPE mutations against
ZBatcher/backend caches, shader-to-fixed-function transitions, texture binding,
samplers, blending, alpha, depth and transforms. Restore state at explicit
boundaries, not with per-object blanket defaults. Preserve RP_TEXTURE ownership.
Validate vertex format/stride, texture format/mip/upload and per-instance color
semantics. Do not promote retail materials into CQ2 shader materials implicitly.

Exit: interleaved world/UI/preview/effect test scenes keep correct materials,
depth and state; traces identify caller categories and actual failure results.
Counter volume alone is not acceptance or GPU utilization.

## M5 — Planet and ordinary-object visual parity (P1)

Trace original surface/cloud layers and compound animation. Investigate initial
pop-in, apparent texture swaps, smearing/seams, alpha/depth and zoom transitions.
Verify retail expectations before treating a changing planet image as a wrong
asset. Fix harvester cargo-pod and refinery material corruption through the proven
material/geometry boundary. Preserve resource-bar placement and accepted occlusion.

Include moons, moon-platform placement, halos, ambient effects and terraform
transitions as explicit parity questions—not assumptions that removed CQ2 code
must be restored. Determine retail existence and data paths first.

Exit: at least three cold and three warm mission/skirmish loads with correct
first-visible-frame textures, stable layers at representative zoom levels, and
no material cross-contamination. Verify retail timing where any delay is intended.

## M6 — Animation, construction and jump transforms (P1)

Separate authored ship axes, gameplay transform, hardpoint transform and display
modifiers. Verify inverted/nose-down scripted arrivals and construction orientation
against retail before adding rotations. Preserve ordinary ship/projectile/glow
fixes unless contradicted by evidence. Reconnect Engine/Anim/hardpoint services
currently supplied by wrappers as necessary.

Restore textured construction/dismantle effects across producers and factions:
Fabricator, HQ where authored, Refinery, Shipyard, turret and harvester. Test
placement ghost tint/transparency, attachment, progress, cancellation and completion.

Exit: correct first-frame orientation and retail assembly sequence; no detached,
white or corrupted construction geometry; save/reload during construction works.
Skirmish spawning alone is not proof that scripted arrivals or production work.

## M7 — Mining, docking and autonomous recovery (P1, independent gameplay track)

Trace two or more harvesters by operation/target/dock reservation and transition
reason. Reproduce node depletion/reselection, partial-load turns, occupied docks,
arrival thresholds, unloading completion, undocking and stranded recovery. Compare
mission and skirmish; distinguish intentional partial unloading from faulty state.
Preserve the confirmed packet-size fix. Do not add arbitrary timeout/retry behavior
before the actual stalled state and its upstream/downstream ownership are known.

Exit: two harvesters complete at least ten consecutive cycles each without manual
intervention, including depletion and dock contention; cargo decreases and stock
increases consistently. Repeat after save/load and focus changes. Research starts
early; behavioral fixes depend on relevant M2 state validation, not full M9 removal.

## M8 — Remaining UI and retail-content coverage (P2)

Correct Options/Escape title encoding after identifying the bad string/encoding
boundary; the debug marker in the window title is not a defect. Regression-test
resource counters and working Options/Objectives buttons without moving them.
Expand faction/mission/special-weapon, map icon, selection and loading-screen
coverage. Track any unsupported content explicitly instead of silently dropping it.

Exit: accepted UI remains unchanged, title text is correct, and content matrix
records pass/fail/not-tested with build identity. No unsupported parity claims.

## M9 — CQ2 dependency retirement (P2, only after parity)

Inventory reachable shader/modifier/dynamic-mesh/EffectPlayer consumers. For retail
features restore verified retail equivalents, including compatible manual effects.
CQ2-only behavior has no retail rollback: port it, isolate it, or seek explicit
approval to exclude it. Do not replace clipped/deformed effects with an unchanged
Engine mesh. Retain wrapper services until animation/hardpoint/lifetime replacements
exist. Delete startup/INI/DLL dependencies only after dependency closure.

TextureManager/StringTable/VideoSystem need separate consumer audits. Excluded
library MeshRender source is not runtime exposure. Avoid "zero counter calls" as
the sole removal criterion; the HQ archetype is an example of non-drawing use.

Exit: no required retail consumer depends on removed components; remaining
exceptions have documented ownership/state contracts; all faction effects tested.

## M10 — Stable-baseline release gate (P1 acceptance)

Full isolated Debug/Final rebuilds, all engine/campaign/runtime artifacts and
launchers verified against manifests. Repeat parser/layout/unit tests and the
mission/skirmish matrix, including cold/warm loading, construction/save/load,
mining, combat, focus/mode transitions and clean exit. Perform a longer gameplay
soak; record duration and observed coverage rather than claim universal stability.
Gate noisy diagnostics, retain actionable error reporting, preserve a rollback
package and publish known limitations.

Exit: no open reproducible crash, data-safety issue, mining deadlock or core retail
visual regression in the acceptance matrix. Debug/Final and selected backend
configuration are explicitly identified; other configurations remain unverified.

## Operating rules

- Work in narrow changesets. Research both caller and callee before coding.
- Milestone completion requires evidence, not a successful compile or one good run.
- Do not hold mining research until renderer cleanup; keep causes independent.
- M1 remains the immediate implementation recommendation. M0 verification and M2
  safety research accompany it; M3 follows before demanding more broad playtests.
- If a gate reveals new unsafe data or ownership, fix that prerequisite first.
- Baseline stability does not require immediate removal of all CQ2-era code.

Supporting scope: CFW_RENDERER_RECONNECTION_SCOPE_2026-09-16.md (12 integration
packages, consolidated here), D3D9_BACKEND_AUDIT_2026-09-16.md,
HQ_PREVIEW_CRASH_2026-09-15.md, RAW_DATABASE_COMPATIBILITY.md,
PARSER_CONVERSION_FINDINGS.md and TRACKING_BUILD_UPDATE_2026-09-14.md.
