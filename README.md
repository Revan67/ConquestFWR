# Conquest: Frontier Wars — VS2022/Win11 Restoration

A community effort to make **Conquest: Frontier Wars** (Fever Pitch Studios / Ubisoft, 2001) build and run natively on modern Windows (10/11) using Visual Studio 2022, starting from the officially released source code.

---

## Project Status

| Phase | Goal | Status |
|-------|------|--------|
| 1 | Build cleanly under VS2022 | **Complete** |
| 2 | Run on Windows 11 — fix runtime crashes, binary layout, and heap/API compat | **In Progress** |
| 3 | Modernize — D3D11+, modern networking, widescreen | Future |

**Phase 2 detail:** The game window opens, the front-end initializes, briefings play, and mission load is progressing. Active work is fixing VS6→VS2022 binary struct layout mismatches that cause crashes and garbled archetype reads during mission startup.

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

Conquest Source License.txt   — Fever Pitch Studios Public License (must accompany all distributions)
Conquest Source Readme.txt    — Original release notes from Fever Pitch Studios
```

---

## What Has Been Changed

All modifications are tracked in git with descriptive commit messages. Major changes from the original VS6/VS2008 source:

- **Compiler:** Retargeted all projects to VS2022 toolset (v143), Windows 10 SDK.
- **CRT linkage:** All App projects use `/MT` (static CRT) to avoid COMHeap conflicts with the DACOM component model.
- **Binary struct layout:** Fixed ~15 VS6→VS2022 layout mismatches in archetype structs (`SHIELD_DATA`, `ROCKING_DATA`, `MISSION_DATA_BIN`, `BASE_SPACESHIP_DATA`, `BASE_PLATFORM_DATA`, toolbar structs, etc.) where the VS2022 compiler packs bitfields and enums differently.
- **Runtime guards:** Added null checks, playerID guards, and archetype-type guards that were unnecessary under VS6's looser undefined-behavior handling.
- **Window title:** Restored "Conquest: Frontier Wars" (the sequel source had changed it).

See the commit log for a full change history.

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
