# Conquest: Frontier Wars — VS2022/Win11 Restoration

A community effort to make **Conquest: Frontier Wars** (Fever Pitch Studios / Ubisoft, 2001) build and run natively on modern Windows (10/11) using Visual Studio 2022, starting from the officially released source code.

---

## Project Status

| Phase | Goal | Status |
|-------|------|--------|
| 1 | Build cleanly under VS2022 | **Complete** |
| 2 | Run on Windows 11 — fix runtime crashes, binary layout, and heap/API compat | **In Progress** |
| 3 | Modernize — D3D11+, modern networking, widescreen | Future |

**Phase 2 — audit complete:** All six build projects (Conquest.exe, Mission.dll, Trim.dll, Globals.dll, D3DRenderPipe.dll, and all Libs) have been verified: VS6→VS2022 porting fixes retained, CQ2 (unshipped sequel) additions removed, and all archetype struct sizes locked with `static_assert` against the retail binary.

**Phase 2 — runtime status:** The game launches, loads a mission, and ships are visible and respond to move orders. Open bugs: ship spawn orientation, resource bar position, overlay panels, sector map display.

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
2. Open `Code/App/Src/` in Visual Studio 2022 (or open a `.vcxproj` directly).
3. Select the **Debug | Win32** configuration.
4. Build `Mission.vcxproj`, `Trim.vcxproj`, and `Conquest.vcxproj` using **Rebuild** (not incremental Build) due to a precompiled-header issue with `PlayerMenu.cpp`:

   ```
   MSBuild Code\App\Src\Mission.vcxproj /t:Rebuild /m:1 /p:Configuration=Debug /p:Platform=Win32
   ```

5. Copy the outputs to your Conquest installation directory.

**Build the solution, not a single project.** `Conquest.vcxproj` does *not* build Mission/Globals/Trim/ZBatcher — they are not project references, they are consumed as prebuilt `.lib` files from the shared `.\Debug\` directory. Building the project alone silently produces a fresh `Conquest.exe` linked against stale sibling DLLs, which is exactly the exe/DLL ABI mismatch this codebase is sensitive to. Use `Code\App\Src\Conquest.sln` with `/t:Rebuild`.

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

---

## Repository Layout

```
Code/
  App/
    DB/               — Game data files (GenData.db, GameTypes.db, StringPack.db)
    DInclude/         — Shared headers for archetype/game-data structs
    Src/              — Main game source (Conquest.exe, Mission.dll, Trim.dll, ...)
  Libs/
    ExplicitDLL/      — Prebuilt engine DLLs (loaded explicitly at runtime)
    ImplicitDLL/      — Prebuilt DACOM DLL (component object model)
    Include/          — Shared engine headers
    Src/              — Engine library sources (DACOM, D3DRenderPipe, MeshManager, ...)
    Static/           — Static import libs

Tools/
  read_gendata.py     — Python script for hex-dumping archetype blobs from .db files
  retail_gametypes.h  — Struct layout reference extracted from the retail VS6 binary
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
- **Binary struct layout — enum bitfields:** VS6 packed differently-typed enum bitfields into one `int` if they fit; VS2022 gives each distinct enum type its own storage unit. Fixed by changing affected bitfields to `unsigned int:N` in `MISSION_DATA`, `SLOT` (DCQGame.h), `BASE_FIGHTER_SAVELOAD` (DFighter.h), `MISSION_SAVELOAD` (DMBaseData.h), and others.
- **Binary struct layout — `MISSION_DATA` split:** The runtime struct was split into `MISSION_DATA_BIN` (40 bytes — binary-stored fields read directly from `.db` files) and `MISSION_DATA : MISSION_DATA_BIN` (72 bytes — adds armor, silhouette, special ability, speech priority at runtime). All archetype headers updated accordingly.
- **`MAX_EXTENSIONS` 4 → 5** in `DExtension.h`, confirmed by binary measurement of `BASE_PLATFORM_DATA` (560 bytes).
- **DirectPlay / DirectInput8 stubs:** Added `Code/Libs/Include/Compat/dplay.h`, `dplobby.h`, and `ddrawex.h` since DirectPlay was removed from the modern Windows SDK.
- **DACOM_MAP lazy-fill (`TComponent.h`):** Changed static array initializers to a lazy-fill pattern to avoid an MSVC 2022 internal compiler error (`toinil.c:899`).
- **Miscellaneous porting:** Null guards, playerID guards, archetype-type guards, DirectInput8 migration, DirectPlay stubs, enum casts, thread guard, `bHiRes` fix, `afxres.h` → `winres.h` in resource files, `_FARQ` compat macro, C++ `bool` guards on math headers, and operator inlining in `matrix4.h`/`quat.h`/`vector.h`/`vector4.h`.
- **Static asserts:** Added `static_assert` on all archetype struct sizes to catch future regressions:

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

The retail mission scripts — `Scripts\script01`–`script16.dll`, `Mantis_T.dll`, `Sol_T.dll`, all dated Oct 2012 — are prebuilt binaries that can never be recompiled. They bind to our `Mission.dll` by **decorated C++ export name**, which makes it a frozen ABI: any change to an `MScript`/`MGlobals` signature, parameter type, or calling convention breaks script binding outright.

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

### Known Open Issues

- **Post-arrival drift.** Ships do not stop on reaching their destination. Traced as far as: velocity is never recomputed because `physUpdateControl` returns early on `bVisible == 0`, and `bVisible` measured 0 for every object across 2787 samples — yet `TestVisible` demonstrably runs (6205 calls) and `MGlobals::GetAllyMask` returns valid masks (23601 calls). Cause still open.
- **Access violations in `_free()`.** 65 identical stacks through DACOM's `HEAP->FreeMemory` into `RtlFreeHeap`, the signature of a corrupted or foreign heap block. Not memory exhaustion — the faults occur during *release*, no allocation failures appear in any log, and the process sits at ~426 MB. Note that debug CRT DLLs (`ucrtbased`, `VCRUNTIME140D`, `MSVCP140D`) are present in the process, so more than one CRT heap is live.
- **`USER_DEFAULTS` layout drift.** Ours serialises as 104 bytes at version 11; retail wrote 100 bytes at version 10. It is persisted to and reloaded from the registry, so the divergence round-trips.
- **UI layout.** Resource values render in the toolbar rather than the top strip; the single-unit context window omits the six upgrade bars and shows a system name instead of the unit name; the sector-map circle renders empty.

### CQ2 (Unshipped Sequel) Removals

The released source was a development snapshot of an unshipped sequel ("Conquest: Vyrium Uprising"). The following additions were not present in the retail binary and have been removed:

- **`BT_PLANET_DATA`:** Removed `ambientEffect[GT_PATH]`, `teraParticle`, `TeraColor`, `teraExplosions`, `Halo`, `bMoon`, `bUncommon`. Restored correct field order: `maxCrew` follows `maxGas`. Fixed `MISSION_DATA_BIN` → `MISSION_DATA` for the embedded mission data block.
- **`DMTechNode.h`:** Removed `cq2Vars1`/`cq2Vars2` from `SINGLE_TECHNODE` and `TECHNODE::_races`, and all related operations (`AddToNode`, `RemoveFromNode`, `HasTech`, `HasSomeTech`, `InitLevel`, `IsEqual`).
- **`DBaseData.h`:** Removed `specialAbility1` and `specialAbility2` instance members (kept as `static const = USA_NONE` for source compatibility). `ResourceCost resourceCost` confirmed as a real retail instance member and restored to correct position.
- **Toolbar schema (`data.i`):** Removed five CQ2-only toolbar control entries that were not present in the retail Globals.dll resource.
- **`CMenu_*.cpp`, `explosion.cpp`:** Removed dead CQ2 code paths throughout.
- **`ObjGen.cpp`:** Removed dead `bMoon` branch (retail had no moon archetype type and ran the planet-spawn path unconditionally).
- **`Damage.cpp`:** Restored `fizzSoundID = data.pData->shield.fizzOut` (was incorrectly zeroed).
- **`TSpaceShip.h`:** Restored `SFXMANAGER->Preload(pData->shield.fizzOut)` preload.
- **`Menu_SPGame.cpp`:** Restored retail campaign-flow launch path (was hardcoded to a CQ2 demo mission).
- **`BuildButton.cpp`:** Removed `cq2Vars1`/`cq2Vars2` initialization.
- **`Direct3D_pipe.cpp`:** Removed `GENERAL_TRACE_1` spam on missing CQ2 shader files.
- **Window title:** Restored "Conquest: Frontier Wars" throughout (sequel source had changed it to "Vyrium Uprising").

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
