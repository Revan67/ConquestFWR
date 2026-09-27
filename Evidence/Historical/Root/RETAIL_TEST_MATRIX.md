# Retail-Parity Test Matrix

> **Historical documentation — provenance only.** This document is not current project authority. See [PROJECT_BIBLE.md](../../../PROJECT_BIBLE.md) for canonical status; preserved for provenance.

This is the acceptance ledger for Priority 1: restore behavior before modernization. Compare the
Debug and Final builds with the retail reference and record evidence, not impressions.

## Automated baseline

| Check | Debug | Final |
|---|---:|---:|
| Full `Win32` solution rebuild | PASS (2026-08-30) | PASS (2026-08-30) |
| Required EXE/DLL outputs present | PASS | PASS |
| All 18 retail campaign DLLs rebuild | PASS | PASS |
| Deployed binaries match build SHA-256 | PASS | NOT DEPLOYED IN LATEST CHECK |
| Isolated deployment directory | PASS | PASS |

Warnings are not a passing fidelity signal. In particular, the linker still reports `/FORCE`,
runtime-library conflicts, and shared intermediate directories.

## Runtime smoke tests

| Area | Test | Debug | Final | Retail comparison / evidence |
|---|---|---:|---:|---|
| Startup | Intro/menu reaches stable idle | TODO | TODO | |
| Mission | Start a known retail mission | TODO | TODO | |
| Rendering | Player ships use correct authored orientation | PASS | TODO | Level and forward in Mission 1 |
| Visibility | Enemy units become visible under retail conditions | PASS | TODO | Earlier report was a unit-identification mistake |
| Combat | Weapons, impacts, damage and destruction complete | PARTIAL | TODO | Combat completes; projectile visual axis remains wrong |
| Explosion | Mesh shatter completes without access violation | PARTIAL | TODO | Representative combat passed; broader soak required |
| Construction | HQ and available orbital structures complete | PASS | TODO | Includes ship production; sentry slot unavailable in test |
| Campaign | Opening combat advances to Fabricator | PASS | TODO | Final-wave/beacon transition still pending retest |
| UI | Selection, orders, build menus and HUD behave | PARTIAL | TODO | Several layout/parity defects remain |
| Save/load | Save and reload the same mission state | TODO | TODO | |
| Audio/video | Music, SFX and cinematics use retail assets | TODO | TODO | |
| Campaign | Complete representative mission from each campaign | TODO | TODO | |

For crashes, retain the build configuration, save/mission, exact reproduction steps, debugger
stack, and—when practical—a TTD trace. For visual differences, capture the same unit, camera,
heading, zoom and mission in retail and rebuilt versions.

## Immediate investigation order

1. Retest the Mission 1 final-wave live-count fix through the beacon-recovery transition.
2. Trace the shared hardpoint/effect transform used by projectiles and engine lights.
3. Restore planet diffuse material and depth/occlusion behavior.
4. Broaden startup and explosion/debris soak coverage.
5. Repeat the same acceptance path in the Final configuration.

### Ship-axis evidence and A/B switch (2026-08-30)

The shipped CMP files are UTF 1.01 containers. Direct inspection of representative Terran,
Celareon, and Mantis root vertex lists shows that most ships are authored with their longest hull
axis on local Z. Examples include `Ttakai.cmp` (Z 225.3 vs. Y 30.5), `TSteele.cmp` (Z 294.7 vs.
Y 45.8), `ssLegionare.cmp` (Z 1142.7 vs. Y 398.0), and `Mthripid.cmp` (Z 508.4 vs. Y 262.0).
This is consistent with a missing authored-Z-forward to game-planar-forward conversion.

`SpaceShip::renderSpaceShip` now honors the existing `CQEFFECTS.bFastRender` option:

- Fast Render on: sequel-era `IMeshInstance::Render` path.
- Fast Render off: legacy `ENGINE->render_instance` path.

This is an A/B diagnostic, not yet the final orientation fix. Compare the same ship and camera in
both modes before changing transforms or mesh data.

Result: Fast Render on and off produced the same approximately 90-degree nose-down orientation
without a crash. The final renderer branch is therefore not the source of the rotation. The next
test restores the dormant `transform.rotate_about_i(90*MUL_DEG_TO_RAD)` conversion in
`SpaceShip::initSpaceShip`, matching the active conversion in `Platform::initPlatform` and the
measured Z-forward authored ship meshes.

Result: restoring the initialization conversion produced no visible change because subsequent
mission/spawn placement replaces the complete gameplay transform. The next controlled test keeps
Fast Render off and passes the same +90-degree correction through the legacy engine's intended
`RF_TRANSFORM_LOCAL` render modifier, leaving gameplay physics unchanged.

Result: the local +90-degree render correction made ships level, proving the missing model-axis
conversion, but mapped the authored bow toward gameplay aft. Preserve the corrected local-up axis
and reverse forward with an additional 180-degree rotation around model-local Y. This also reverses
local right, preserving a proper right-handed rotation rather than mirroring the mesh.

### Overnight candidate fixes awaiting runtime validation (2026-09-01)

| Area | Candidate fix | Morning acceptance test |
|---|---|---|
| Planet | Keep legacy BMPs off the CQ2 color/bump promotion path; render compound planet children | Retail diffuse appearance and far-side ring occlusion both work |
| Refinery visuals | Apply the proven ship model-axis correction to beam and cargo hardpoints | Beam begins on hull; all cargo panels stay attached |
| Refinery AI | Repair assist packet size, stale-target transitions, dock timeout and node-reselect delay | Two ships mine multiple nodes, unload full/partial loads, and recover from contention |

These are build-verified candidates, not PASS entries. Alt-Tab appears to perturb the old modal
simulation timing, so obtain the first clean comparison without changing focus, then repeat with
Alt-Tab as a separate robustness test.

The 2026-09-01 01:20 Debug build also replaces the under-ship marker's explicit `sin(theta)`
left/right oscillation with continuous rotation. Runtime acceptance: the red marker rotates smoothly
in place and remains centered beneath the ship at every heading.

### Milestone 1 planet containment — 2026-09-03

| Test | Result | Scope |
|---|---|---|
| Remove loose planet overrides and restore engine-instance planet draw | PASS | Mission 1 reached fabricator arrival; planet returned from black to gray/white |

This is a rollback baseline only. Retail diffuse texture, compound animation, and ring occlusion
were not validated by this containment test.

### Milestone 1 planet diffuse state — 2026-09-06

| Test | Result | Scope |
|---|---|---|
| Two consecutive Mission 1 entries after full Debug rebuild/deployment | PASS | Planet textured immediately on both runs |
| First Earth PolyMesh draw has texturing enabled | PASS | Two distinct run/object handles logged `rwm=0` from first draw |
| Texture submission/readback | PASS | Setter succeeded and observed handle matched requested handle throughout |

Root cause was leaked process-global `RP_TEXTURE` state. `ObjectList::Render()` now establishes the
configured value at the world-frame boundary. This closes intermittent diffuse visibility only;
ring occlusion, cloud blending/motion, and close-range smearing remain independent tests. Full
evidence and regression steps are in `Evidence/Milestone-1/PLANET_TEXTURE_STATE_LEAK.md`.

### Milestone 1 depth candidate — 2026-09-06

| Test | Debug | Final | Acceptance evidence |
|---|---:|---:|---|
| Full rebuild/deploy (22 engine + 18 campaign modules) | PASS | PASS | Build script hash verification |
| Attached D3D9 depth surface | PASS | TODO | Debug creation trace and successful world depth behavior |
| Far-side planet ring occlusion | PASS | TODO | User confirmed far-side occlusion during Mission 1 |
| Ship/fabricator planetary-limb occlusion | PASS | TODO | User confirmed object/planet occlusion; normal ships maintain orbital height |
| Cloud alpha and close-range surface fidelity | TODO | TODO | No opaque shell, smearing, or ordering regression |
| Construction placement ghost | TODO | TODO | Alpha ghost visible and cursor-attached |
| Fabricator construction animation | TODO | TODO | Textured geometry stays attached while animating |
| Shipyard construction animation/orientation | TODO | TODO | Authored effect visible; ship on-plane from first visible frame |
| Refinery construction animation/orientation | TODO | TODO | Authored effect visible; ship on-plane from first visible frame |
| Active-build save/reload | TODO | TODO | Effect reattaches at correct progress; build completes normally |
| Top-right Options button | PASS | TODO | Checkmark opens Options; resource counters remain unchanged |
| Top-right mission-objectives button | PASS | TODO | Adjacent list button opens Mission Objectives |
| Harvester diagnostic capture | TODO | TODO | A full problematic cycle produces `debug/harvester_state.txt`; diagnose before behavior edits |
| Mission reload and Alt-Tab/device reset | TODO | TODO | No loss of depth, textures, or crash |

### Stable-baseline renderer ownership candidate — 2026-09-13

The Debug installation now contains the full schema-2 build that restores the CFW world-renderer
boundary and retains the modifier-aware MeshManager route for effects. It also removes the
application effect renderer's non-CFW diffuse vertex field and randomized vertex colors. Build and
deployment provenance pass (22 engine modules, 18 campaign modules, 47 deployed runtime
artifacts); every visual result below remains runtime-pending.

| Test | Debug | Final | Acceptance evidence |
|---|---:|---:|---|
| Mission 1 planet first visible frame | TODO | TODO | Immediately textured; no white/gray placeholder |
| Planet stability over several minutes | TODO | TODO | No texture swap, seam growth, or smeared surface |
| Fabricator/HQ/refinery/turret/harvester construction | TODO | TODO | Effect geometry uses correct authored materials; no rainbow or white masks |
| Laser-turret placement ghost | TODO | TODO | Visible, textured/alpha-correct, and cursor-attached |
| Mission 1 scripted arrivals | TODO | TODO | Blackwell, escorts, reinforcements, Travis, and Mantis enter on-plane without correction flip |
| Mining diagnostic cycle | TODO | TODO | Run until success or first stall, then preserve clean-exit trace |

The detailed procedure and remaining reset-lifecycle caveat are in
`Evidence/Milestone-1/DEPTH_LIFECYCLE_REPAIR.md`.

### Runtime triage — 2026-09-07

| Area | Result | Evidence / next acceptance |
|---|---|---|
| Top-right Options and Objectives buttons | PASS | Both opened correctly; resource bar unchanged |
| Construction lifecycle handoff | PASS | Effects appeared for Fabricator, HQ, Refinery, turret, and harvester |
| Construction materials | FAIL | Psychedelic/untextured effects and completed-Refinery corruption; retest legacy-material correction |
| Harvester target selection | PASS in captured failure | Both ships held valid ore targets and working IDs |
| Harvester downstream movement | FAIL | Mode 12, move active, position unchanged for roughly 80 seconds; new log records goal, target position/distance, and auto-movement state |
| Mission 1 initial Blackwell group | FAIL | Arrives inverted, then flips correct |
| Mission 1 Mantis/later reinforcements/Travis | FAIL | Arrives nose-down, then corrects |
| Ordinary Skirmish spawn | PASS | Preserve as the global orientation control |
| Planet initial appearance/texture stability | FAIL | 1–2 second appearance delay followed by a smeared texture transition |
