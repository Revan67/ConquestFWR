# Conquest: Frontier Wars — Restoration Project

An  effort to get **Conquest: Frontier Wars** (Ubisoft, 2001) running natively on modern Windows (Windows 10/11) and to modernize its codebase over time.

## Goals

### Phase 1 — Build (Complete)
Get the original C++ codebase compiling and linking cleanly under Visual Studio 2022. All five projects (`Conquest.exe`, `Trim.dll`, `Mission.dll`, `ZBatcher.dll`, `DACOM.dll`, plus the library DLLs) now build without errors.

### Phase 2 — Runtime (In Progress)
Get the game actually running on Windows 11. This involves fixing runtime crashes and compatibility issues with modern Windows APIs, D3D9 behavior under DWM/DXVK, and heap/memory subsystem quirks introduced by the VS2022 runtime.

Current state: the game window appears and the front-end initializes. Active work is on getting into a stable in-game state.

### Phase 3 — Modernization (Future)
Once the game runs reliably, the goal is to modernize incrementally:
- Replace deprecated DirectX 9 calls with D3D11/D3D12 or Vulkan
- Replace DirectPlay networking (stubbed out for Win11 compatibility) with a modern equivalent
- Improve resolution and widescreen support
- Reduce reliance on prebuilt legacy DLLs by rebuilding from source

## Repository Layout

```
Code/
  App/Src/          — Main executable and game DLLs (Conquest, Trim, Mission, ZBatcher)
  Libs/
    ExplicitDLL/    — Prebuilt engine DLLs (explicitly loaded at runtime)
    ImplicitDLL/    — Prebuilt DACOM DLL (component object model for the engine)
    Include/        — Shared headers and compat stubs
    Src/            — Source for engine library DLLs (DACOM, D3DRenderPipe, etc.)
    Static/         — Static import libs
Directory.Build.props — Shared MSBuild properties (SDK paths, compiler flags)
```

## Background

Conquest: Frontier Wars was released in 2001 by Fever Pitch Studios / Ubisoft. The source code used here is an officially released development build. The original build targeted Visual Studio 6/7 with DirectX 8/9 and DirectPlay; this project targets VS2022 with modern Windows 11 compatibility.
