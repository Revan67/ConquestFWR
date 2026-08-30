# Conquest: Frontier Wars — VS2022/Win11 Restoration

A community effort to make **Conquest: Frontier Wars** (Fever Pitch Studios / Ubisoft, 2001) build and run natively on modern Windows (10/11) using Visual Studio 2022, starting from the officially released source code.

---

## Project Status

| Phase | Goal | Status |
|-------|------|--------|
| 1 | Build cleanly under VS2022 | **Complete** |
| 2 | Run on Windows 11 — fix runtime crashes, binary layout, and heap/API compat | **In Progress** |
| 3 | Modernize — D3D11+, modern networking, widescreen | Future |

**Phase 2 — audit:** All six build projects (Conquest.exe, Mission.dll, Trim.dll, Globals.dll, D3DRenderPipe.dll, and all Libs) have been verified: VS6→VS2022 porting fixes retained, CQ2 (unshipped sequel) additions removed, and archetype struct **sizes** locked with `static_assert` against the retail binary.

> **A size assert is not a layout assert.** Two opposite errors in the same struct cancel in `sizeof`, and the assert still passes while every field between them is displaced. This is not hypothetical — it is what hid the bug described under *Ship movement* below, and three such cancelling pairs have now been found. Use `offsetof` asserts on fields that straddle the suspect region.

**Phase 2 — runtime status:** The game launches, loads a mission, and **ships move, take orders, and engage**. Remaining bugs are no longer movement-related: ship models render rotated ~90°, enemy units are invisible on contact, and an assert fires in `Explosion::Update` on first combat.

---

## Legal

### Source License

The source code in this repository was **officially released** by Fever Pitch Studios under their public license. The full license text is in [`Conquest Source License.txt`](Conquest%20Source%20License.txt).

Key terms:

- **Non-commercial use only.** You may not sell, charge for, or receive compensation of any kind for this software or any derivative work.
- **Redistribution permitted** provided the full license text accompanies every copy.
- **Modifications must be attributed** — include the date, author, and a notice that the work is released under this license.
- **No warranty.** Fever Pitch Studios provides the program as-is with no support obligations.

If you distribute a modified version of this project, you must:
1. Include [`Conquest Source License.txt`](Conquest%20Source%20License.txt) unmodified.
2. Add a prominent notice identifying what you changed and when.
3. State that your modifications are released under the same license.

### Copyright

Original game copyright © Digital Anvil, Inc. / Microsoft Corporation / Ubisoft Entertainment.  
Source release copyright © 2013 Fever Pitch Studios.  
Modifications in this repository copyright © their respective contributors, released under the same Fever Pitch Studios Public License.

### What This Repository Contains

| Content | Source | License coverage |
|---------|--------|-----------------|
| C++ source code (`Code/App/Src/`, `Code/Libs/Src/`) | Official Fever Pitch Studios source release | Fever Pitch Studios Public License |
| Game data files (`Code/App/DB/*.db`) | Official Fever Pitch Studios source release | Fever Pitch Studios Public License |
| Prebuilt engine DLLs (`Code/Libs/ExplicitDLL/`) | Official Fever Pitch Studios source release | Fever Pitch Studios Public License |
| Build artifacts (`.obj`, `.dll`, `.exe` in intermediate dirs) | Compiled from the above | Fever Pitch Studios Public License |

This repository does **not** contain retail game assets extracted from a purchased copy, nor does it circumvent any copy-protection mechanism. The `.db` data files were distributed as part of the official source release, not extracted from the retail disc.

### No Retail Game Files Required to Build

You do **not** need a copy of the retail game to build from source. However, to **run** the compiled output you will need the retail game's media assets (audio, video, textures), which are not included here. The game is available legally via [GOG.com](https://www.gog.com).

---

## Requirements

- **Visual Studio 2022** (Community or higher) with the Desktop C++ workload
- **Windows 10 or 11** (32-bit target; build host must support WoW64)
- **Python 3** — required for the `sfxdata.xmf → sfxdata.dat` step in `Globals.vcxproj`. The step invokes `py` (the launcher), not bare `python`, because on a default Windows install `python` resolves to the Microsoft Store alias stub and fails.
- **DirectX SDK (June 2010)** — `Conquest.exe` uses `ID3DXEffect`/`D3DXMATRIX` directly and links `d3dx9d.lib`. See below.
- DirectX 9 runtime (included with Windows 10/11 via DirectX End-User Runtime)
- A retail copy of Conquest: Frontier Wars for the runtime media assets
- **MFC (optional)** — only the map Editor (`Code/Editor/`) needs it. Without it that project fails with `MSB8041: MFC libraries are required`. Install *C++ MFC for v143 build tools* from the Visual Studio Installer's Individual Components tab. The game itself does not use MFC.

### DirectX SDK — extract, do not install

`DXSDK_Jun10.exe` fails with **error S1023** on any machine carrying a modern VC++ 2010 redistributable, which is essentially all of them. Rather than uninstalling and reinstating system redistributables, extract the archive — it is a plain self-extractor:

```
7z x DXSDK_Jun10.exe -o"<repo-parent>" "DXSDK/Include/*" "DXSDK/Lib/*"
```

`Directory.Build.props` resolves the SDK repo-relatively (`$(MSBuildThisFileDirectory)..\DXSDK`), so placing `DXSDK/` beside the repo is all that is required. No install, no admin, no redistributable downgrade. Only four files actually matter: `Include\d3dx9.h`, `Lib\x86\d3dx9.lib`, `d3dx9d.lib`, and `DxGuid.lib` (`dinput8.lib` comes from the Windows SDK).

The original source targeted VS6 / Windows XP / DirectX 8–9. This project retargets to VS2022 while keeping the 32-bit Win32 ABI.

---

## Building

1. Clone this repository.
2. Open **`Code/App/Src/Conquest.sln`** in Visual Studio 2022.
3. Select the **Debug | Win32** configuration.
4. **Rebuild the solution** — not an incremental Build, and not a single project:

   ```
   MSBuild Code\App\Src\Conquest.sln /t:Rebuild /m:1 /p:Configuration=Debug /p:Platform=Win32
   ```

Outputs deploy themselves: each project has a post-build step that copies its DLL/EXE and PDB into the game folder, so no manual copy is needed.

**Why the solution and not a project.** `Conquest.vcxproj` does *not* build Mission/Globals/Trim/ZBatcher — they are not project references, they are consumed as prebuilt `.lib` files from the shared `.\Debug\` directory. Building the project alone silently produces a fresh `Conquest.exe` linked against stale sibling DLLs, which is exactly the exe/DLL ABI mismatch this codebase is sensitive to.

**Why Rebuild and not Build.** Incremental builds skip work they should not: a precompiled-header issue with `PlayerMenu.cpp`, git-tracked `.obj` files that make MSBuild think compilation is current, and `.res` resource steps that silently do not re-run — the last of which matters because `data.i` (the parser schema) is embedded in `Globals.dll` as a resource.

> **A failed rebuild leaves the game unrunnable.** `/t:Rebuild` cleans the deployed DLLs *before* the post-build copy replaces them, so a build that fails partway leaves the install missing `Globals.dll`, `Mission.dll`, `Trim.dll` and `ZBatcher.dll`. Fix the build and rebuild, or copy them back from `Code/App/Src/Debug/`. A common cause of a partway failure is a file lock — an orphaned `DbgX.Shell` process from a TTD replay holds the deployed DLLs open, and the post-build `xcopy` then fails with exit code 4.

`Conquest.vcxproj`'s post-build reports `Skip (not found): Globals.pdb / Trim.pdb / ZBatcher.pdb` — harmless, as each of those projects deploys its own PDB from its own post-build step.

**Note:** The engine library DLLs under `Code/Libs/` are prebuilt. Only the App projects (Conquest, Mission, Trim, ZBatcher, Globals) are rebuilt from source.

---

## Debugging

Symbols are deployed alongside the binaries, so the debugger is far more effective than print statements for this codebase.

**Time Travel Debugging** is the highest-leverage tool available. `tttracer.exe` ships with Windows; replay needs WinDbg (`winget install Microsoft.WinDbg`).

```
cd "<game install>"
tttracer.exe -out <trace dir> -launch "<game install>\Conquest.Exe"
```

The `cd` is required — the traced process inherits the working directory, and the game exits within a second if it cannot find its data. Traces run about **300 MB/sec**, so keep sessions short or attach to an already-running process with `-attach <PID>` to skip the load.

Replay is scriptable and headless:

```
WinDbgX.exe -y "<sympath>" -z "<trace>.run" -logo "<out>.txt" -c "$$><script.txt"
```

Set symbols with the `-y` flag; `.sympath` inside `-c` swallows the rest of the command line. Put commands in a script file — inline quoting of `dx` expressions does not survive the shell. Begin scripts with `.prefer_dml 0` so DML markup does not corrupt the log.

The decisive feature is querying every call to a function across an entire recorded run:

```
dx @$cursession.TTD.Calls("Conquest!IBaseObject::TestVisible").Count()
dx -r1 @$cursession.TTD.Calls("Mission!MGlobals::GetAllyMask").Select(c => c.ReturnValue).Take(20)
```

Practical notes, each learned the hard way:

- **`TTD.Events` does not exist** in current WinDbg builds — the surface is `Calls`, `Memory`, `Data`, `Utility`, `Bookmarks`. To find a crash, query the exception dispatcher instead: `dx @$cursession.TTD.Calls("ntdll!KiUserExceptionDispatcher").Last().TimeStart.SeekTo()`. Most dispatches are ordinary handled C++ exceptions; check the position is near 100% of the trace before assuming you have the fatal one.
- **`!tt` cannot be used inside a `$$><` script.** It seeks correctly, then raises *"Non-primary client caused an implicit wait"* and silently aborts every remaining command.
- **`L<n>` counts are hexadecimal.** `dd addr L12` returns **18** dwords, not 12. Write `L0n12` for decimal — otherwise every field after the first is silently misaligned.
- **Prefer `dt Type @ecx fieldName` to raw offsets.** Offsets move whenever a struct changes; field names do not.
- **Indexing a large trace takes far longer than any scripted timeout.** A 50 GB trace builds a ~13 GB index. Launch the debugger detached and poll for a completion marker in its log — killing it mid-index leaves a corrupt `.idx` that must be deleted and rebuilt from scratch.
- **Rebuilding orphans existing traces** — symbol queries fail against a trace whose PDBs have been replaced. Archive PDBs alongside each trace you intend to keep.

### Retail's own schema is inside retail Globals.dll

The most authoritative reference for any struct question is not the disassembly — it is the parser schema the shipped game carries as a resource. Retail `Globals.dll` holds a ~164 KB **`PARSER`** resource containing the full `data.i` text: every archetype struct, exactly as shipped.

Extract it by walking the PE resource directory for the `PARSER` entry and dumping the blob to a file, then diff it against `Code/App/Src/data.i`. Doing this across all 375 shared structs is what located the movement bug and two further cancelling pairs; guessing from byte patterns produced two wrong conclusions before it.

---

## Repository Layout

```
Code/
  App/
    DB/               — Game data files (GenData.db, GameTypes.db, StringPack.db)
    DInclude/         — Shared headers for archetype/game-data structs.
                        SOURCE OF TRUTH for the parser schema: data.i is generated from these.
    Src/              — Main game source (Conquest.exe, Mission.dll, Trim.dll, ...)
      Scripts/        — Mission-script source for all 18 shipped scripts (Script01T–Script16T,
                        Mantis_T, Sol_T) plus Helper/Include. Project files are legacy
                        .vcproj/.dsp and do not build under MSBuild — see Mission-Script ABI.
  Editor/             — Map editor (MFC). Separate solution; needs the MFC component installed.
  EffectEd/           — Effects editor.
  Libs/
    ExplicitDLL/      — Prebuilt engine DLLs (loaded explicitly at runtime)
    ImplicitDLL/      — Prebuilt DACOM DLL (component object model)
    Include/          — Shared engine headers
    Src/              — Engine library sources (DACOM, D3DRenderPipe, MeshManager, ...).
                        Prebuilt: NOT built by Conquest.sln.
    Static/           — Static import libs

Tools/
  abicheck/           — Re-runnable dumpbin set-arithmetic checks for the mission-script ABI
  read_gendata.py     — Python script for hex-dumping archetype blobs from .db files
  retail_gametypes.h  — Struct layout reference extracted from the retail VS6 binary
  CQ2Material/        — Sequel-era material tooling
  ShapeEdit/          — Shape editing tool
  xmiff2/
    xmiff2.py         — Replaces legacy 16-bit XMIFF.EXE; compiles sfxdata.xmf → sfxdata.dat

Conquest_Frontier_Wars_Manual.pdf   — Retail game manual (UI and gameplay reference)
Conquest Source License.txt         — Fever Pitch Studios Public License
Conquest Source Readme.txt          — Original release notes from Fever Pitch Studios
```

---

## What Has Been Changed

All modifications are tracked in git with descriptive commit messages. The changes fall into two categories: **VS2022/Win11 porting fixes** (required for correct compilation and behavior) and **CQ2 removal** (reverting unshipped sequel additions back to the retail baseline).

### VS2022 / Win11 Porting Fixes

- **Compiler:** Retargeted all projects to VS2022 toolset (v143), Windows 10 SDK.
- **CRT linkage:** All App projects use `/MT` (static CRT) to avoid COMHeap conflicts with the DACOM component model.
- **Early-heap routing (`COMHeap_VS2022.c`):** VS2022's CRT calls `malloc`/`free` during static initializers, before DACOM's `_HEAP` is set up. Added a `GetProcessHeap()`-backed early-heap layer (tagged with `EARLY_HEAP_MAGIC = 0xEA12EA12`) so these allocations succeed without corrupting the DACOM heap. `COMHeap.asm` routes all allocation calls through this layer when `_HEAP == NULL`.
- **Binary struct layout — enum bitfields:** VS6 packed differently-typed enum bitfields into one `int` if they fit; VS2022 gives each distinct enum type its own storage unit. Fixed by changing affected bitfields to `unsigned int:N` in `MISSION_DATA`, `SLOT` (DCQGame.h), `BASE_FIGHTER_SAVELOAD` (DFighter.h), `MISSION_SAVELOAD` (DMBaseData.h), and others. Note this deliberately diverges from retail's schema *text*, which names the real enum types (`M_RACE`, `FighterState`, …) — the goal is to reproduce VS6's *packing*, not its spelling. Retail's field widths still apply: `MISSION_DATA` is `mObjClass:8, race:4, displayName:18` with 22 caps bools, where this source currently has `9/4/17` and 24.
- **~~Binary struct layout — `MISSION_DATA` split~~ — REVERTED, this was itself a bug.** The runtime struct had been split into `MISSION_DATA_BIN` (40 bytes, believed to be the binary-stored subset) and `MISSION_DATA : MISSION_DATA_BIN` (72 bytes). Retail's own parser schema has **no such split** — archetypes store the full 72-byte `MISSION_DATA`. The 40-byte substitution left every field after `missionData` 32 bytes early in each struct that used it, which is what broke ship movement. Being reverted; see *Ship Archetype Layout* below.
- **~~`MAX_EXTENSIONS` 4 → 5~~ — REVERTED, and instructive.** `DExtension.h` was changed to 5 "confirmed by binary measurement of `BASE_PLATFORM_DATA` (560 bytes)". Retail has `extension[4]`. The measurement appeared to confirm 5 only because the extra 32-byte array element exactly cancelled the 32-byte `MISSION_DATA_BIN` deficit in the same struct — so the total came out right while the middle was displaced. **This is a worked example of why a size measurement cannot validate a layout.**
- **DirectPlay / DirectInput8 stubs:** Added `Code/Libs/Include/Compat/dplay.h`, `dplobby.h`, and `ddrawex.h` since DirectPlay was removed from the modern Windows SDK.
- **DACOM_MAP lazy-fill (`TComponent.h`):** Changed static array initializers to a lazy-fill pattern to avoid an MSVC 2022 internal compiler error (`toinil.c:899`).
- **Miscellaneous porting:** Null guards, playerID guards, archetype-type guards, DirectInput8 migration, DirectPlay stubs, enum casts, thread guard, `bHiRes` fix, `afxres.h` → `winres.h` in resource files, `_FARQ` compat macro, C++ `bool` guards on math headers, and operator inlining in `matrix4.h`/`quat.h`/`vector.h`/`vector4.h`.
- **Static asserts:** Added `static_assert` on archetype struct **sizes**. These catch a struct that changes size, but — as the two reverted entries above demonstrate — they cannot catch two opposite errors that cancel. Newer asserts state retail's invariants as relations instead (`offsetof(x, last) + sizeof(LastType) == sizeof(x)`), which fail on exactly that case:

  | Struct | Size |
  |--------|------|
  | `MISSION_DATA_BIN` | 40 B |
  | `MISSION_DATA` | 72 B |
  | `BT_PLANET_DATA` | 200 B |
  | `BASE_SPACESHIP_DATA` | 644 B |
  | `BT_GUNBOAT_DATA` | 816 B |
  | `BASE_PLATFORM_DATA` | 560 B |
  | `SHIELD_DATA` | 108 B |
  | `HOTBUTTON_DATA` | 32 B |
  | `BUILDBUTTON_DATA` | 104 B |
  | `RESEARCHBUTTON_DATA` | 64 B |
  | `TABCONTROL_DATA` | 108 B |
  | `STATIC_DATA` | 64 B |
  | `HOTSTATIC_DATA` | 40 B |
  | `ICON_DATA` | 16 B |
  | `MULTIHOTBUTTON_DATA` | 16 B |
  | `SHIPSILBUTTON_DATA` | 8 B |
  | `BT_FIGHTER_WING` | 208 B |

### Mission-Script ABI

The game ships 18 mission scripts — `Scripts\script01`–`script16.dll`, `Mantis_T.dll`, `Sol_T.dll`, all dated Oct 2012 — and the running game loads those prebuilt binaries. They bind to our `Mission.dll` by **decorated C++ export name**, which makes it a frozen ABI in practice: any change to an `MScript`/`MGlobals` signature, parameter type, or calling convention breaks script binding outright.

**Source for all 18 is present** under `Code/App/Src/Scripts/` (`Script01T`–`Script16T`, `Mantis_T`, `Sol_T`, plus `Demo2`, `DemoScript`, `ScriptTest` and a shared `Helper`/`Include`). What blocks rebuilding them is not the absence of source but the **project format**: all 39 project files are legacy `.vcproj` / `.dsp` that MSBuild cannot load (`MSB4025: Root element is missing`). Converting them to `.vcxproj` — as was done for the engine projects — would make the scripts buildable and relax this constraint. Until then, treat the exported ABI as frozen.

Verified by three-way `dumpbin` set arithmetic (script imports vs. retail `mission.dll` exports vs. ours): **153 symbols demanded from `Mission.dll` and 2 from `Trim.dll`, all satisfied.** Since MSVC mangling encodes calling convention, this also confirms `__cdecl`/`__stdcall` alignment. Re-runnable checks live in `Tools/abicheck/`; re-run them after touching any exported signature.

Matching symbol names prove nothing about **struct layout** — `OrderTeleportTo(const MPartRef&, ...)` mangles identically whether `MPartRef` is 16 or 20 bytes. Nine types cross that boundary and none previously had a size assertion:

  | Struct | Size | Notes |
  |--------|------|-------|
  | `MPartRef` | 16 B | passed to 82 exported functions |
  | `UNIT_STANCE` | 4 B | enum, passed by value |
  | `MGroupRef` | 92 B | |
  | `TECHNODE` | 64 B | returned by value |
  | `CQBRIEFINGITEM` | 80 B | stamps its own `sizeof` for run-time checks |
  | `CQSCRIPTDATADESC` | 28 B | |
  | `AIPersonality` | 40 B | stamps its own `sizeof`; `U32` bitfields |
  | `MISSION_SAVELOAD` | 104 B | `U32` bitfields |
  | `CQSCRIPTENTRY` | 24 B | |

All are guarded by `#ifndef _ADB` so the ADB schema compiler does not see them. `AIPersonality` and `CQBRIEFINGITEM` write their own `sizeof` into a `size` member for runtime verification, so a size change there fails at *runtime* rather than at link time — worth remembering when editing either.

Two pieces of known, benign drift remain: four symbols differ only in `wchar_t` mangling (VS6 typedef'd it as `unsigned short`, VS2022 treats it as native), none of which any script imports; and 28 exports exist that retail lacks, all present in the original source release as unshipped sequel API surface.

### Runtime Fixes (Phase 2)

- **Ship rendering:** `SpaceShip::renderSpaceShip` now falls back to `ENGINE->render_instance` when `instanceMesh` is NULL, restoring ship visibility in-mission.
- **MeshManager.dll deployment:** Added xcopy post-build step to `MeshManager.vcxproj`; the DLL was previously never auto-deployed to the game folder.
- **Context window position:** `Menu_tb::setStateInfo` and `LineDraw` calls now add `screenRect.left/top` when converting toolbar-relative `pContextRect` values to screen-space coordinates.
- **Null map guard:** `Menu_tb::checkRect` returns early if `map` is NULL, preventing an AV on mission load when mouse-move fires during toolbar teardown.
- **sfxdata build step:** `Globals.vcxproj` now runs `Tools/xmiff2/xmiff2.py` as a `<CustomBuild>` step on `sfxdata.xmf`, replacing the legacy 16-bit XMIFF.EXE tool that was incompatible with Win64.
- **Conquest.vcxproj post-build:** Replaced eight `xcopy` commands (which aborted on missing `.pdb` files) with a single PowerShell `Copy-Item` loop that silently skips files not yet present.
- **Uninitialised `velocity` / `ang_velocity`:** The `ObjectTransform` constructor set only `instanceMesh` and `instanceIndex`. Both `Vector` members lack a zeroing default constructor, so a newly created object inherited whatever the heap block contained and `ObjectPhysics::physUpdatePhysics` then integrated it as genuine world motion — objects drifting with no order given, and spurious rotation from garbage angular velocity. Invisible under VS6, whose allocator returned zeroed memory here; our COMHeap/MSHeap rerouting reuses blocks with stale contents. Same class as the earlier `teraParticle`/`halo` fix.
- **`initMissionData` bounds check:** `playerID` derives from `dwMissionID & PLAYERID_MASK` (`0x0F`) and so can legitimately arrive as 0–15, while `globalData.playerTechLevel` is `[9][5]` — values 9–15 read past the end across roughly eight subsequent accesses. Previously unchecked; only `race` was validated.

### Ship Archetype Layout — the movement bug

`BASE_SPACESHIP_DATA` declared a 40-byte `MISSION_DATA_BIN` where retail's shipped schema has the full 72-byte `MISSION_DATA`, and `ROCKING_DATA` carried 32 bytes of fabricated padding (`rockLinearAccel`, `rockAngAccel`, `_rock_vs6[6]`) that retail does not have.

The two errors were equal and opposite, so `sizeof(BASE_SPACESHIP_DATA) == 644` kept passing while every field between them sat 32 bytes early. `dynamicsData` was read at `record+84` instead of `record+116`, and `maxAngVelocity` picked up a neighbouring negative float.

That inverts the symmetric clamp in `ObjectControl`:

```cpp
if (vel >  maxAngVelocity) vel =  maxAngVelocity;   // 0 > -0.2  ->  vel = -0.2
```

`vel = 0` means "nothing to correct", but it satisfies `0 > -0.2` and is forced to a constant nonzero rate **every frame**. Ships accumulated pitch until `|j_xy|` collapsed; `get_yaw()` — computed from that horizontal projection — became meaningless; `relYaw` never fell below the 20° gate in `doPathMove`; `onPathComplete()` was never reached and `bMoveActive` never cleared. Ships could be ordered to move but never did.

Verified after the fix: `maxAngVelocity` positive on 16/16 sampled ships across all classes, `linearAcceleration` a real float rather than a raw-integer denormal, every ship level, and `onPathComplete` going from **0 to 11** calls in a comparable session.

`offsetof` asserts were added for `missionData`, `dynamicsData`, and `rockingData`.

**Two more cancelling pairs remain open**, found by diffing the whole schema against retail: `BILLBOARD_DATA` is 8 bytes short in `BASE_SPACESHIP_DATA` (masked by the CQ2 `formationFilter`/`bLargeShip` fields, displacing `techActive`), and `BASE_PLATFORM_DATA` pairs the same `MISSION_DATA_BIN` deficit against an extra `extension[5]` array element.

### Known Open Issues

- **Ship movement — SOLVED.** See *Ship archetype layout* under What Has Been Changed. Ships now move, complete paths, and stop.

  Two earlier explanations were disproved along the way and are recorded here because both were confidently wrong. First, that `physUpdateControl` returns early on `bVisible == 0`: Time Travel Debugging showed player ships do get `visibilityFlags = 0x1` and `bVisible = 1`, and the visibility system works. That came from printf diagnostics with sample caps (`_n < 300`), and a cap keeps the *first* N calls — which all occur during mission load, so the measurement described the loading screen, not gameplay. Second, that the pitch sign in `ObjectControl` was inverted: decompiling retail's `get_angle` (`0x005845fd`) proved `get_angle(x,y) == atan2(x,y)`, and the existing sign is correct. **Sample steady-state, and prefer a TTD query over the whole run to any printf cap.**

- **Ship models render ~90° nose-down.** Engines up, dorsal surface leading. This is *not* engine state: TTD shows every sampled ship with `pitch 0.00`, `roll 0.00`, `|j_xy| 1.000` and `targetPitch 0`. The transform is correct, so the model is being drawn rotated against it — a mesh/render convention issue. Note the engine's nose is `-J`, so a model authored nose-along `+Z` would render exactly this way.

  *Render path now mapped (2026-08-30).* The ship hands the mesh a **level** transform: `instanceMesh = MESHMAN->CreateMesh(data.meshArch, this)` binds the object, and the mesh reads its world matrix back through the object's `GetTransform()`; the normal frame is just `instanceMesh->Render()` with no per-frame `SetTransform` (that call exists only in the warp branch). So the ~90° is applied *inside* the CQ2 `instanceMesh`/MeshManager path (prebuilt `MeshManager.dll`/`D3DRenderPipe.dll`), which uses a different model-to-world axis convention than retail's `ENGINE->render_instance(instanceIndex)` path. Retail shipped the `render_instance` path — still present but dead for ships, taken only in the `else if (instanceIndex != -1)` fallback. Fix is a **restore-vs-fix-forward decision** (bake a −90° correction into the transform/archetype, revert ships to `render_instance`, or inspect the authored mesh orientation first) and is paused for that call.

- **Enemy units invisible on contact.** Appears when hostiles arrive in mission 1. Not caused by the archetype fix — `sensorRadius` sits inside the 40 bytes both struct versions share, so it never moved.

- **Assert in `Explosion::Update`.** `explosion.cpp:297` (`owner.Ptr()->Update()`) raises `0x80000003` at 99.99% of a recorded session; `Explosion::Update` had been called exactly once. `owner` is an `OBJPTR<IExplosionOwner>`, a watched pointer nulled when its target dies. Note `CQASSERT`/`CQBOMB` are **live** in the Debug configuration — `FINAL_RELEASE` is defined only in Release — so in-game asserts really do `__asm int 3` and kill the process.
- **Access violations in `_free()`.** 65 identical stacks through DACOM's `HEAP->FreeMemory` into `RtlFreeHeap`, the signature of a corrupted or foreign heap block. Not memory exhaustion — the faults occur during *release*, no allocation failures appear in any log, and the process sits at ~426 MB. Note that debug CRT DLLs (`ucrtbased`, `VCRUNTIME140D`, `MSVCP140D`) are present in the process, so more than one CRT heap is live.
- **`USER_DEFAULTS` layout drift.** Ours serialises as 104 bytes at version 11; retail wrote 100 bytes at version 10. It is persisted to and reloaded from the registry, so the divergence round-trips.
- **UI layout.** Resource values render in the toolbar rather than the top strip; the single-unit context window omits the six upgrade bars and shows a system name instead of the unit name; the sector-map circle renders empty.

### CQ2 (Unshipped Sequel) Removals

The released source was a development snapshot of an unshipped sequel ("Conquest: Vyrium Uprising"). The following additions were not present in the retail binary and have been removed:

- **`BT_PLANET_DATA`:** Removed `ambientEffect[GT_PATH]`, `teraParticle`, `TeraColor`, `teraExplosions`, `Halo`, `bMoon`, `bUncommon`. Restored correct field order: `maxCrew` follows `maxGas`. Fixed `MISSION_DATA_BIN` → `MISSION_DATA` for the embedded mission data block.
- **`DMTechNode.h`:** Removed `cq2Vars1`/`cq2Vars2` from `SINGLE_TECHNODE` and `TECHNODE::_races`, and all related operations (`AddToNode`, `RemoveFromNode`, `HasTech`, `HasSomeTech`, `InitLevel`, `IsEqual`).
- **`DBaseData.h`:** Removed `specialAbility1` and `specialAbility2` instance members (kept as `static const = USA_NONE` for source compatibility). `ResourceCost resourceCost` confirmed as a real retail instance member and restored to correct position.
- **`MISSION_DATA_BIN` bitfield widths:** Reverted the sequel's widened `mObjClass` (9→8) and narrowed `displayName` (17→18) to retail's `8/4/18` packing. We parse retail `.db` data, so the expansion layout misread `race`/`displayName`. Kept the `unsigned int` field type (the legitimate VS6 bitfield-packing fix) and the two sequel `M_CAPS` bits (`targetPositionOk`, `specialTargetPlanetOk`), which are used by `ObjComm`/`MScript` and are layout-harmless spare bits in the 4-byte `M_CAPS`.
- **Toolbar schema (`data.i`):** Removed five CQ2-only toolbar control entries that were not present in the retail Globals.dll resource.
- **`CMenu_*.cpp`, `explosion.cpp`:** Removed dead CQ2 code paths throughout.
- **`ObjGen.cpp`:** Removed dead `bMoon` branch (retail had no moon archetype type and ran the planet-spawn path unconditionally).
- **`Damage.cpp`:** Restored `fizzSoundID = data.pData->shield.fizzOut` (was incorrectly zeroed).
- **`TSpaceShip.h`:** Restored `SFXMANAGER->Preload(pData->shield.fizzOut)` preload.
- **`Menu_SPGame.cpp`:** Restored retail campaign-flow launch path (was hardcoded to a CQ2 demo mission).
- **`BuildButton.cpp`:** Removed `cq2Vars1`/`cq2Vars2` initialization.
- **`Direct3D_pipe.cpp`:** Removed `GENERAL_TRACE_1` spam on missing CQ2 shader files.
- **Window title:** Restored "Conquest: Frontier Wars" throughout (sequel source had changed it to "Vyrium Uprising").

**Still outstanding.** A full diff against retail's shipped schema shows 46 structs in this source that retail does not have, and 37 of the 375 shared structs still differ. Most of the 46 are harmless — standalone sequel types (artifact, formation, buff and nova launchers, command kits) that no retail struct embeds, so the parser never reaches them and no retail data can be corrupted by them.

Four are not harmless, because a retail struct embeds them and they therefore displace real parsed fields: `MISSION_DATA_BIN`, `FORMATION_FILTER`, `AdmiralBonuses`, and `BUILDQUEUE_SAVELOAD`.

`MISSION_DATA_BIN` deserves particular note: it does not exist in retail at all. It is an invention of this source, substituted for the real 72-byte `MISSION_DATA` in **19** archetype structs. One has been corrected (`BASE_SPACESHIP_DATA`); **18 remain**, each reading its mission data 32 bytes short. Seventeen of those have no size assert of any kind.

---

## Contributing

Contributions are welcome, subject to the license terms above (non-commercial, attribution required).

When submitting changes:
- Include a clear description of what was changed and why (the binary layout reasoning is particularly important to document).
- Do not include retail game assets.
- Ensure your changes remain non-commercial.

---

## Acknowledgements

- **Fever Pitch Studios** — for releasing the source code under a public license.
- **Digital Anvil / Microsoft / Ubisoft** — original development and publication.
- The Conquest modding community for keeping interest in this game alive.
