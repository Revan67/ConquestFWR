# ConquestFWR — Project Handoff

> **Historical documentation — provenance only.** This document is not current project authority. See [PROJECT_BIBLE.md](../../../PROJECT_BIBLE.md) for canonical status; preserved for provenance.

_Last updated: 2026-09-12_

Restoration + modernization of **Conquest: Frontier Wars**. The released source is an
unshipped-sequel development snapshot ("Conquest: Vyrium Uprising" / "CQ2"); the goal is to make
it build and run on VS2022 / Windows 11 while matching **retail** behavior, then modernize on a
separate branch.

---

## 1. Current status (one screen)

| Area | State |
|------|-------|
| Build (VS2022 / Win11, 32-bit) | ✅ Game + all 18 retail campaign DLLs build in Debug and Final/Release |
| Game launch + mission load | 🧪 Startup allocator race fixed; one full Mission 1 run passed, repeated-launch test pending |
| Ship movement / pathing | ⚠️ Ordinary combat movement works; harvester movement can freeze with an active goal |
| Ship model orientation/facing | ⚠️ Ordinary Skirmish spawn works; scripted jump arrivals begin inverted/nose-down and correct later |
| Combat loop | 🧪 Auto-attack/facing work; projectile direction wrong; final-wave progression fix pending retest |
| Ship hardpoints/effects | ⚠️ Engine lights misplaced on every player ship; likely same axis mismatch as projectiles |
| Struct schema vs retail | ⚠️ ~11 divergences left (was 37); see remediation plan |
| Explosion mesh-shatter AV | 🧪 MeshInfo binding fix survived full opening combat; needs broader soak |
| Combat renderer lighting AV | ✅ SOLVED (full construction-wave combat completed; normal exit) |
| **HQ/platform construction AV** | ✅ SOLVED (all structures available in Mission 1 built; normal exit) |
| Platform textures | ✅ SOLVED (HQ and all completed structures runtime-confirmed) |
| Planet rendering | ⚠️ Texture-state and depth failures repaired; initial delay, surface/cloud transition and smearing remain |
| Construction/transient materials | ⚠️ Lifecycle works; active-build and placement-ghost materials remain corrupt or absent |
| UI layout bugs | 🧪 Resource bar and top-right actions pass; escape-menu title localization remains |

### Intermittent startup crash — fix deployed, verification pending
The 2026-08-30 minidump resolved to `DACOM.dll!HeapInstance::malloc` during
`hardpoint.dll`'s CRT `DLL_THREAD_ATTACH`. DXVK was creating worker threads which allocated
concurrently through Conquest's single-threaded main DACOM heap, corrupting its free list.
`Conquest.cpp` now enables DACOM's existing `DAHEAPFLAG_MULTITHREADED` implementation for the
main heap. The Debug build completed with zero errors and was deployed; launch it repeatedly to
verify that the long-standing intermittent startup failure is gone.

### Mission 1 combat progression — root cause fixed and runtime-confirmed
Destruction diagnostics proved events reached `TM1_ShipDestroyed`, but retail Mantis frigates
arrived with `mObjClass == 73` while the sequel enum had compiled `M_FRIGATE == 90`. Sequel-only
classes had been inserted throughout `M_OBJCLASS`, shifting every later retail ID. The enum now
matches the complete retail sequence exactly (`0..110`), keeps CQ2-only names after the retail
sentinel, and has compile-time guards for critical IDs. The engine and `SCRIPT01.dll` were rebuilt
and deployed together.

The 2026-08-30 retest completed the battle, automatically acquired/attacked targets, advanced
the quest, and spawned the Fabricator. Ship facing and projectile direction remain separate
render/weapon-orientation defects.

### Mission 1 HQ construction crash — fixed and runtime-confirmed
The full dump `CrashDumps/Conquest.exe_260830_210620.dmp` showed
`ObjectExtent::initExtents` indexing element 1 of an archetype renderer array that contained only
one element. The HQ placement shadow initialized the shared array with one mesh child, while the
completed HQ instance contained three. `TObjExtent.h` now grows undersized shared arrays before
binding the completed object's `MeshInfo` entries. Existing shadows keep the old renderers alive
through their own references. The 2026-08-30 retest built the HQ and every other structure the
mission allowed, across the available ring locations, without a crash. The sentry alone was not
tested because every build location was occupied. The game then exited normally.
Ship production was also exercised during this run without an error.

### Combat renderer lighting crash — fixed and runtime-confirmed
The full dump `CrashDumps/Conquest.exe_260830_220036.dmp` mapped the recurring
`d3drenderpipe.dll+0x1E14` signature to the return path of
`Direct3D_RenderPipeline::set_default_constants`. The routine allocated four `D3DLIGHT9` and
attenuation entries, but searched for its key light using `enabledLightCount` (up to eight).
An out-of-bounds attenuation read could select index 4–7; swapping that `D3DLIGHT9` then copied
104 bytes over the function's stack frame. The dump contained the resulting light fields in the
saved frame/arguments, matching this failure exactly. Selection and key-light loops are now
bounded to the shader/local-array capacity, the replacement comparison uses `worstAtten`, zero
light capacity cannot underflow, and the global enabled-light list is release-build bounded.
The full Debug build completed and the deployed `D3DRenderPipe.dll` matches its output by SHA-256.
The following run completed the opening battle and all later construction-triggered waves,
including multiple explosions, then exited normally without the old renderer crash.

### Mission 1 post-combat stall — exact counter defect fixed, verification pending
The harvesting objective is a persistent tutorial reminder: `SCRIPT01` contains no resource
threshold, simultaneous-harvest condition, or completion call for it. The actual post-combat gate
is `data.mantis_ships == 0`. Construction waves add to this `U16` counter, the final attack resets
it to five even if an older wave overlaps, and destruction events previously decremented it at
zero. It could therefore become stale or wrap to 65,535 while no enemy remained. The final Tau
Ceti attack now synchronizes the counter from live Mantis frigates/scout carriers actually in Tau
Ceti, and the destruction handler cannot underflow. `SCRIPT01.dll` was rebuilt explicitly,
deployed, and verified by SHA-256; runtime verification is pending.

### Planet diffuse state leak — fixed and twice runtime-confirmed

Working/failed A/B traces proved the apparent planet “loading delay” was not asset streaming.
PolyMesh had valid Earth geometry and a valid texture handle, but the failed run retained global
`RP_TEXTURE` as disabled and therefore set `RWM_DONT_TEXTURE` on every draw. In a working run, an
incidental later path re-enabled the same state, explaining delays ranging from seconds to more
than a minute. `ObjectList::Render()` now establishes `RP_TEXTURE` from the configured
`CQEFFECTS.bTextures` value at the beginning of every world frame. Two consecutive Mission 1 runs
showed the texture immediately; both logged `rwm=0`, successful binds, and matching readback from
the first Earth draw. See `Evidence/Milestone-1/PLANET_TEXTURE_STATE_LEAK.md`. Narrow diagnostics
remain until Final and additional-mission regression testing. Ring occlusion/depth, cloud motion,
and smearing remain separate defects.

Renderer direction is now explicit: retain the original
`Engine -> RenderMgr -> PolyMesh -> ZBatcher -> CQPipeline -> D3D9` route as authoritative for
retail CFW assets. CQ2/manual rendering is an exceptional path only for proven shipped consumers,
and every retained exception must restore shared state and synchronize ZBatcher before returning.
The immediate renderer priority is the confirmed missing/incoherent D3D9 depth-buffer lifecycle,
followed by state-boundary hardening, consumer inventory, duplicate MeshRender consolidation, and
removal of demonstrably unused CQ2 components. Full rationale and ordered work are recorded in
`Evidence/Milestone-1/RENDER_PIPELINE_AUDIT.md`.

### Completed-platform and earlier planet texture corruption — platforms fixed
Mission 1 showed a solid-black planet, black/white platform surfaces, and unrelated striped
textures on completed structures. Ships and UI textures were largely correct, pointing to the
CQ2 `instanceMesh` material path rather than missing texture files. Completed platforms and
planets now use the retained retail `ENGINE->render_instance` path, with `instanceMesh` kept for
animation, callbacks, hardpoints, effects, and construction shadows. A full Debug rebuild was
deployed and its SHA-256 matches the build output. Runtime testing confirmed correct HQ and
completed-structure textures. The planet is now visible instead of solid black, but appears as a
bare white/gray lit mesh with no diffuse texture and does not occlude the far side of its orbital
ring. The later `RP_TEXTURE` frame-boundary correction resolved the diffuse portion; depth/ring
occlusion remains open. The refinery ship and shipyard's active ship-build
preview also still sample unrelated textures on the CQ2 mesh/material path.

---

## 2. Uncommitted / working-tree state

The current renderer, construction, mining, build, diagnostic, and documentation work is a large
mixed working batch and is **not committed** on the branch. Do not treat a successful build as
acceptance of this batch. The complete pre-remediation state is recoverable from
`refs/checkpoints/stable-baseline-pre-remediation-20260912`; classification and staging are tracked
in `Evidence/Stable-Baseline/README.md`.

Latest commits (all authored `Revan67 <revan67@users.noreply.github.com>`):
- `11a0d85` Document restoration status and crash workflow
- `244b613` Add reproducible game and campaign build workflow
- `2ed123e` Restore retail mission behavior and stabilize combat
- `cd74a3c` Rebuild engine libs (DACOM/ComHeap/MathLib/RPUL) against current source
- `9e90c1f` MISSION_DATA_BIN: revert CQ2 bitfield widths to retail (8/4/18)
- `c6e1901` Revert CQ2 inRootSupply to retail's single inSupply; fix copy_release_static.bat

---

## 3. Open work queue (priority order)

1. **Verify Mission 1 final-wave live-count fix** — replay through the post-construction attack.
   After the last Tau Ceti Mantis dies, Halsey should introduce the beacon-recovery objective;
   the harvesting reminder itself is intentionally non-completing.
2. **Ship hardpoint/effect transform + projectile direction** — auto-attack and combat facing now
   work, but bolts travel toward the bottom of the map and engine lights are misplaced on every
   player ship. Treat these two
   as likely consumers of the same uncorrected CQ2-to-retail model-axis transform. Start from
   hardpoint world-transform generation and `Bolt::InitWeapon` in `Projectile.cpp`, which
   corrects pitch but not yaw; do not add separate visual offsets before tracing the shared path.
3. **Planet material/depth state** — retain the now-working geometry draw while restoring its
   diffuse texture, depth writes/test, and far-side ring occlusion. Then address the refinery ship
   and shipyard construction-preview material binding on the CQ2 path.
4. **Verify startup allocator fix** — perform repeated launches/exits. One complete opening combat
   run has passed since the fix, but repeated-launch coverage is still pending.
5. **Explosion mesh-shatter soak** — the restored MeshInfo binding survived the latest full combat
   with several destroyed ships; broaden coverage before declaring it closed.
6. **Remaining struct divergences** (`project_remediation_plan.md`): `BT_FLAGSHIP_DATA` is the
   one big restructure; `MISSION_DATA` flatten is cosmetic (layout-identical); **leave** the
   documented enum-bitfield-packing SAVELOAD structs (`BASE_FIGHTER_SAVELOAD`, `SLOT`,
   `MISSION_SAVELOAD`). Re-run the script-ABI check after any SAVELOAD change.
7. **Dead-code sweep follow-up** (`project_deadcode_sweep.md`): verify/restore
   `SoundMan::PlayMovie` + `MovieCamera::Render` (likely the cinematic bug) — verify each vs
   retail, do NOT bulk-uncomment.
8. **Delete orphaned `ObjectRender` (`TObjRender.h`)** once confirmed unreferenced.
9. Minor: `USER_DEFAULTS` 104B/v11 vs retail 100B/v10; UI layout bugs.

---

## 4. Build & run

- **Solution:** `Code/App/Src/Conquest.sln` (VS2022). Always **full rebuild** (`/t:Rebuild`) —
  git-tracked `.obj`/`.res` cause MSBuild to skip recompiles (see `feedback_build_no_cache`).
- **Campaign solution:** `Code/App/Src/Scripts/ConquestCampaign.sln` contains exactly the 18
  retail modules (`SCRIPT01`–`SCRIPT16`, `Mantis_T`, `Sol_T`). Its `Debug` configuration links
  against the Debug app/engine libraries; `Release` links against Final/release libraries.
- **Supported entry points:** `build-debug.cmd` and `build-final.cmd`. Both call
  `Build-Conquest.ps1`, rebuild the game, engine modules, and full retail campaign set, verify
  every required output, deploy all 18 campaign DLLs to `Scripts/`, and SHA-256-check deployment.
  Use `-NoDeploy` for a compile-only run.
- **Configs are NOT branches:** *Debug* (asserts live: `CQASSERT`/`CQBOMB` do `__asm int 3`)
  vs *Final*/*Release* (`FINAL_RELEASE` → asserts compiled out; retail-parity test build).
- **Debug install:** `D:\Games\GOG\Conquest Frontier Wars\`.
- **Final install:** `D:\Games\GOG-Final\Conquest Frontier Wars\` (isolated parity testing).
- **Verified 2026-08-30:** full Debug and Final rebuilds completed with zero errors; all 18
  campaign DLLs were verified in both configurations. The Debug game, engine modules, and
  campaign set were deployed and matched their build outputs by SHA-256. Warnings remain
  baseline debt.
- **App projects stay on `/MT`** — switching to `/MDd` breaks COMHeap → DACOM `exit(255)`.
- **Retail reference install:** `D:\Games\RetailRef` (launches but renders nothing; use for
  binary/behavior reference, not runtime).

---

## 5. Key reference assets & tools

- **Authoritative retail schema:** `D:\Dev\traces\retail_data_i.txt` — the parser resource
  extracted from retail `Globals.dll` (`PARSER/156/1033`). This is ground truth for struct
  layout. `Code/App/Src/data.i` is a **build output** (generated from the DInclude headers) —
  edit the headers, not `data.i`.
- **Ghidra install:** `C:\Utilities\ghidra_12.1.3_PUBLIC`.
- **Ghidra project:** `D:\Dev\ghidra-proj` (retail `Conquest.exe` map; `callgraph.tsv`;
  get_yaw/get_pitch/get_roll/get_angle/doMove located with offsets).
- **TTD (Time Travel Debugging):** `tttracer` records, headless WinDbgX replays; query with
  `TTD.Calls(...)`. Traces in `D:\Dev\traces\` (e.g. `path.run` + symbols in
  `symbols-current\`). Replaces printf/rebuild/run. Note `L<n>` counts are HEX (use `L0n12`).

---

## 6. Project memory (persistent, loaded each session)

Location: `%USERPROFILE%\.claude\projects\D--Dev-ConquestFWR\memory\`
Index (one line per memory, loaded automatically): `memory\MEMORY.md`

**Read first, in order:**
1. `project_environment_recovery.md` — repo is `D:\Dev\ConquestFWR`, working install is
   `D:\Games\GOG`.
2. `project_session_state.md` — where each session paused (has a 2026-08-30 end section).
3. `project_ship_orientation.md` — the mesh 90° blocker, fully mapped (2026-08-30 section).

**Standing rules (feedback memories — follow these):**
- `feedback_investigation_order.md` — verify before any change/assumption; check upstream;
  retail binary + Ghidra dump are the BASELINE.
- `feedback_restore_then_modernize.md` / `feedback_match_retail.md` — restore to retail first;
  modernize on a separate branch; flag anything that can't cleanly revert.
- `feedback_commit_author.md` — commit as `Revan67 <revan67@users.noreply.github.com>`; real
  name excluded from all git data.
- `feedback_crt_mt.md` — App projects stay `/MT`.
- `feedback_build_no_cache.md` — always `/t:Rebuild`.
- `feedback_unshipped_features.md` — CQ2/unshipped features: cut the path, don't stub.
- `feedback_diagnostic_sampling.md` — never sample the first N calls; never gate on the measured
  variable.

**Topic references:** `project_remediation_plan.md`, `project_deadcode_sweep.md`,
`project_move_drift.md` (movement SOLVED), `project_script_abi.md`, `project_ghidra_retail_map.md`,
`project_retail_binary_audit.md`, `project_ttd_workflow.md`, `project_build_installs.md`,
`project_game_install.md`, `project_runtime_fixes.md`, `project_runtime_ui_bugs.md`,
`project_source_origin.md`, `project_spaceship_binary_layout.md`, `project_vs6_struct_layout.md`.

---

## 7. History context

- Source is a **sequel dev build** — expect sequel additions in structs; three `.db` files ship:
  `gametypes.db` (unit archetypes), `gendata.db` (UI/static), `stringpack.db` (strings).
- The parser schema lives in **`Globals.dll`** (not Mission.dll); `data.i` changes require
  rebuilding `Globals.vcxproj`.
- Method lesson that cost real time: verify by derivation/measurement BEFORE editing — 11+
  hypotheses were refuted this investigation; the cheap ones caught were the ones checked first.

---

## 8. Overnight test build — 2026-09-01

The latest Debug deployment rebuilt cleanly with **0 errors** and includes three deliberately
narrow fixes that still require runtime confirmation:

- **Planet material path:** Mission 1 and the retail archives were verified unchanged. Tau Ceti
  Prime resolves to `Planets\earth1_LG.cmp`, whose surface uses the legacy
  `earth1_128.bmp`. MeshManager had been classifying every filename with an extension as a CQ2
  color/bump material, rewriting the BMP name to `_color.tga`/`_bump.tga`; that explains the
  black compound planet. Legacy BMPs now remain on the retail material path and Planetoid again
  renders the compound instance so its surface/cloud children are present. This is a test fix,
  not yet a runtime PASS.
- **Refinery attachments:** the mining-beam hardpoint and all four cargo-tank hardpoints now use
  the same local +90-degree X / 180-degree Y authored-mesh correction already proven for ship
  geometry and engine glows. This should move the beam onto the ship and may correct the displaced
  refinery cargo panels. It intentionally does not alter shared world transforms.
- **Refinery state machine:** fixed `sizeof(&buffer)` packet truncation for nugget-assist joins;
  added bounded recovery from a permanently locked refinery dock; cleared stale targets after
  depletion; inserted a short settling delay before selecting the next node; and forced prompt AI
  reconsideration after completion. Full-load unload already passed before this build; automatic
  next-node selection and partial-final-load return remain unverified.

**First morning test (Debug, avoid Alt-Tab):** confirm the planet has its retail gray-blue diffuse
surface and hides far-side ring sections; send two refinery ships through several nodes into one
refinery; verify both join mining, automatically select subsequent nodes, unload a partial final
load, and recover from dock contention. Also check that the mining beam originates on the hull and
the cargo panels remain attached. If the planet is still black, revert only the Planetoid draw to
the last visible-gray baseline and continue tracing material realization rather than adding more
asset substitutions.

**UI follow-up build (01:20):** corrected the red under-ship talking/target marker animation. The
source explicitly rotated it by `sin(theta)`, producing the reported left/right seesaw; it now
performs a bounded continuous rotation. The construction-placement preview path is present and is
reached through both deep-space and planet-slot placement. Its remaining failure is downstream in
the shared translucent unborn-mesh draw/material path, not a missing build-button or command path;
do not add a substitute UI marker before testing the current material-path changes.

---

## 9. 2026-09-06 overnight renderer follow-up

- Restored and verified an actual D3D9 depth attachment in `create_buffers()`. The
  prior path advertised depth without creating/binding a surface. Debug and Final
  full rebuilds each completed with 22 engine modules and 18 campaign modules, and
  both runtime `DLL/D3DRenderPipe.dll` files hash-match their configuration outputs.
- Runtime planet/ring acceptance is still pending. Use
  `Evidence/Milestone-1/DEPTH_LIFECYCLE_REPAIR.md`; do not claim occlusion fixed from
  compilation alone.
- The root-level `D3DRenderPipe.dll` in each install is stale but not loaded.
  `Conquest.ini` explicitly loads `DLL/D3DRenderPipe.dll`.
- Renderer ownership and direct-state consumers are recorded in
  `Evidence/Milestone-1/RENDERER_CONSUMER_INVENTORY.md`. Do not remove CQ2 modules
  until live consumers are proven, and do not install blanket per-object defaults.
- Construction preview and assembly call paths are present. Jump-in gameplay
  transforms are planar. Blackwell has a distinct authored turret archetype. These
  findings and their focused tests are in
  `Evidence/Milestone-1/CONSTRUCTION_AND_SHIP_FINDINGS.md`.
- The device Reset/teardown lifecycle remains deliberately deferred because its
  default-pool resource ownership has not yet been traced.
