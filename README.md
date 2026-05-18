# Conquest: Frontier Wars — VS2022/Win11 Restoration

A community effort to make **Conquest: Frontier Wars** (Fever Pitch Studios / Ubisoft, 2001) build and run natively on modern Windows (10/11) using Visual Studio 2022, starting from the officially released source code.

---

## Project Status

| Phase | Goal | Status |
|-------|------|--------|
| 1 | Build cleanly under VS2022 | **Complete** |
| 2 | Run on Windows 11 — fix runtime crashes, binary layout, and heap/API compat | **Complete** |
| 3 | Modernize — D3D11+, modern networking, widescreen | Future |

**Phase 2 detail:** The retail source audit is complete. All six build projects (Conquest.exe, Mission.dll, Trim.dll, Globals.dll, D3DRenderPipe.dll, and all Libs) have been verified: VS6→VS2022 porting fixes retained, CQ2 (unshipped sequel) additions removed, and all archetype struct sizes locked with `static_assert` against the retail binary.

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
- DirectX 9 runtime (included with Windows 10/11 via DirectX End-User Runtime)
- A retail copy of Conquest: Frontier Wars for the runtime media assets

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

**Note:** The engine library DLLs under `Code/Libs/` are prebuilt. Only the App projects (Conquest, Mission, Trim, ZBatcher, Globals) are rebuilt from source.

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
