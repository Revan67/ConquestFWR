# ConquestFWR — Project Handoff

_Last updated: 2026-08-30_

Restoration + modernization of **Conquest: Frontier Wars**. The released source is an
unshipped-sequel development snapshot ("Conquest: Vyrium Uprising" / "CQ2"); the goal is to make
it build and run on VS2022 / Windows 11 while matching **retail** behavior, then modernize on a
separate branch.

---

## 1. Current status (one screen)

| Area | State |
|------|-------|
| Build (VS2022 / Win11, 32-bit) | ✅ Full solution builds, 0 errors (Debug + Final/Release configs) |
| Game launch + mission load | ✅ Launches, loads Mission 1 |
| Ship movement / pathing | ✅ SOLVED (archetype struct-layout root cause) |
| Combat loop | ✅ Runs; explosions play |
| Struct schema vs retail | ⚠️ ~11 divergences left (was 37); see remediation plan |
| **Ship models render ~90° nose-down** | ❌ OPEN — render path mapped, **paused for a design decision** |
| **Enemy units invisible on contact** | ❌ OPEN — same mesh/render layer |
| **Explosion mesh-shatter AV** | ❌ OPEN — real bug in Debug + Final (SplitInstance/GetBoundingBox) |
| UI layout bugs | ⚠️ Several minor (resource bar, context window, sector map) |

**Biggest blocker: the mesh/render layer.** Three symptoms (90° pitch, invisible enemies,
explosion AV) all cluster in the prebuilt `MeshManager.dll` / `D3DRenderPipe.dll` render path.

### The mesh 90° blocker — decision needed
Fully mapped this session (details in memory `project_ship_orientation.md`, 2026-08-30 section):
- The ship hands the mesh a **level** transform (physics is correct: pitch 0, |j_xy| = 1).
- The ~90° is applied **inside the CQ2 `instanceMesh` / MeshManager path**, which uses a
  different model-to-world axis convention than retail's `ENGINE->render_instance(instanceIndex)`
  path (still in the code but dead for ships).
- **Three options, none chosen — this is a restore-vs-fix-forward call for the user:**
  1. Fix-forward: bake a −90° correction into the transform or mesh archetype (cheap; verify it
     doesn't also rotate hardpoints/effects).
  2. Restore retail: route ships back through `ENGINE->render_instance(instanceIndex)` (faithful;
     but MeshManager may be required by shatter/slice effects — check first).
  3. Inspect the authored retail mesh orientation to learn the correct convention, then pick 1/2.

---

## 2. Uncommitted / working-tree state

Tree is clean of source changes — everything committed. Two **untracked build outputs** remain
and are intentionally NOT committed (candidates for `.gitignore`):
- `Code/App/Src/Final/` — Final-config obj/exe output
- `Code/Libs/ExplicitDLL/release/x86Math.dll` — build product

Latest commits (all authored `Revan67 <revan67@users.noreply.github.com>`):
- `cd74a3c` Rebuild engine libs (DACOM/ComHeap/MathLib/RPUL) against current source
- `9e90c1f` MISSION_DATA_BIN: revert CQ2 bitfield widths to retail (8/4/18)
- `c6e1901` Revert CQ2 inRootSupply to retail's single inSupply; fix copy_release_static.bat

---

## 3. Open work queue (priority order)

1. **Mesh 90° / invisible enemies / explosion AV** — the render-layer cluster (see §1). Start
   from the mapped render path; get the user's decision on the 90° approach before coding.
2. **Remaining struct divergences** (`project_remediation_plan.md`): `BT_FLAGSHIP_DATA` is the
   one big restructure; `MISSION_DATA` flatten is cosmetic (layout-identical); **leave** the
   documented enum-bitfield-packing SAVELOAD structs (`BASE_FIGHTER_SAVELOAD`, `SLOT`,
   `MISSION_SAVELOAD`). Re-run the script-ABI check after any SAVELOAD change.
3. **Dead-code sweep follow-up** (`project_deadcode_sweep.md`): verify/restore
   `SoundMan::PlayMovie` + `MovieCamera::Render` (likely the cinematic bug) — verify each vs
   retail, do NOT bulk-uncomment.
4. **Deploy rebuilt scripts to the Final install** (only the Debug install got them).
5. **Delete orphaned `ObjectRender` (`TObjRender.h`)** once confirmed unreferenced.
6. Minor: `USER_DEFAULTS` 104B/v11 vs retail 100B/v10; UI layout bugs.

---

## 4. Build & run

- **Solution:** `Code/App/Src/Conquest.sln` (VS2022). Always **full rebuild** (`/t:Rebuild`) —
  git-tracked `.obj`/`.res` cause MSBuild to skip recompiles (see `feedback_build_no_cache`).
- **Configs are NOT branches:** *Debug* (asserts live: `CQASSERT`/`CQBOMB` do `__asm int 3`)
  vs *Final*/*Release* (`FINAL_RELEASE` → asserts compiled out; retail-parity test build).
- **Working game install:** `D:\Games\GOG\Conquest Frontier Wars\` — a post-build xcopy
  auto-deploys; no manual copy needed. (The `E:\Games\GOG` copy is a stock decoy.)
- **App projects stay on `/MT`** — switching to `/MDd` breaks COMHeap → DACOM `exit(255)`.
- **Retail reference install:** `D:\Games\RetailRef` (launches but renders nothing; use for
  binary/behavior reference, not runtime).

---

## 5. Key reference assets & tools

- **Authoritative retail schema:** `D:\Dev\traces\retail_data_i.txt` — the parser resource
  extracted from retail `Globals.dll` (`PARSER/156/1033`). This is ground truth for struct
  layout. `Code/App/Src/data.i` is a **build output** (generated from the DInclude headers) —
  edit the headers, not `data.i`.
- **Ghidra:** `D:\Dev\ghidra-proj` (retail `Conquest.exe` map; `callgraph.tsv`;
  get_yaw/get_pitch/get_roll/get_angle/doMove located with offsets).
- **TTD (Time Travel Debugging):** `tttracer` records, headless WinDbgX replays; query with
  `TTD.Calls(...)`. Traces in `D:\Dev\traces\` (e.g. `path.run` + symbols in
  `symbols-current\`). Replaces printf/rebuild/run. Note `L<n>` counts are HEX (use `L0n12`).

---

## 6. Project memory (persistent, loaded each session)

Location: `C:\Users\Joel\.claude\projects\D--Dev-ConquestFWR\memory\`
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
