# Engine DLL Rebuild Log

Records every rebuild of `Code/Libs/ExplicitDLL/debug/` DLLs: date, toolset, trigger, and which DLLs changed.
Commit this file alongside the DLL changes so the reason is always in the history.

---

## 2026-04-09 — VS2022/Win11 working baseline (commit 442041e)

- **Trigger:** First full rebuild of all 22 engine DLLs for VS2022/Win11 compatibility
- **MSVC toolset:** 14.44.35207 — cl.exe 19.44.35225 (VS2022 Enterprise)
- **DLLs rebuilt:** All 22 (Anim, Channel, DACOM, D3DRenderPipe, DAHotKey, DocuView,
  DosFile, Engine, HardPoint, LightManager, MaterialManager, MeshManager, Optics,
  ParticleEffect, PolyMesh, RenderMgr, SceneGraph, Streamer, StringTable, System,
  TextureLibrary, TextureManager, VertexBufferManager, VideoSystem, x86Math)
- **Source changes:** Multiple — see commit history up to 442041e for full list
  (DirectInput DI7→DI8, CQASSERT guards, VideoSystem binkw32 guard, etc.)

---

## 2026-05-07 — Full rebuild after Windows SDK update

- **Trigger:** Windows SDK update (April 2026) altered UCRT init ordering; rebuilding
  against the updated SDK produced different code-gen and DLL sizes. No source changes.
- **MSVC toolset:** 14.44.35207 — cl.exe 19.44.35225 (VS2022 Enterprise)
- **DLLs rebuilt:** All except MaterialManager.dll, StringTable.dll, System.dll
  (Anim, Channel, D3DRenderPipe, DAHotKey, DocuView, DosFile, Engine, HardPoint,
  LightManager, MeshManager, Optics, ParticleEffect, PolyMesh, RenderMgr, SceneGraph,
  Streamer, TextureLibrary, TextureManager, VertexBufferManager, VideoSystem, x86Math)
- **Source changes:** None

---

## 2026-05-13 — TextureLibrary.dll only

- **Trigger:** Unknown — rebuilt in isolation. Likely a specific fix or settings change
  in the TextureLibrary project. (Reason not recorded at build time.)
- **MSVC toolset:** 14.44.35207 — cl.exe 19.44.35225 (VS2022 Enterprise)
- **DLLs rebuilt:** TextureLibrary.dll only
- **Source changes:** Unknown (not tracked in git diff)

---

## 2026-05-14 — D3DRenderPipe.dll: silence missing shader errors

- **Trigger:** `Direct3D_pipe.cpp` modified — removed `GENERAL_TRACE_1` spam when
  a CQ2 shader/effect file is not found. Retail build has no CQ2 shader files;
  the three trace calls produced noise on every render. Now a silent `return NULL`.
- **MSVC toolset:** 14.44.35207 — cl.exe 19.44.35225 (VS2022 Enterprise)
- **DLLs rebuilt:** D3DRenderPipe.dll only
- **Source changes:** `Code/Libs/Src/RenderPipeline/DirectDraw/D3D/Direct3D_pipe.cpp`
  - Removed 3x `GENERAL_TRACE_1` calls in `loadEffectHelper` on file-not-found path

---

## 2026-05-15 — D3DRenderPipe.dll: re-rebuild (May 14 DLL was out of sync)

- **Trigger:** The 2026-05-14 commit included a DLL built from the OLD source (before
  the GENERAL_TRACE_1 removal). The source edit happened after the DLL was last built,
  so the committed DLL still fired the __debugbreak() assert on every missing shader.
  Rebuilt from current source to produce the correct DLL.
- **MSVC toolset:** 14.44.35207 — cl.exe 19.44.35225 (VS2022 Enterprise)
- **DLLs rebuilt:** D3DRenderPipe.dll only
- **Source changes:** None (same source as 2026-05-14 entry — this is just the correct build)

---

## 2026-05-15 — Mission.dll: three runtime fixes

- **Trigger:** Three bugs found after first successful in-game run:
  (1) FogOfWar::UpdateFog nested ObjMapIterator assert every frame,
  (2) CMenu_Ind::onUpdate AV when selecting a ship (namearea null),
  (3) GT_TOOLBAR COMMON STATIC_DATA field `shipclass` missing before `gas`
  in data.i schema — caused DataParser to enumerate shipclass at ICON_DATA
  boundary (blob offset 1080 = garbage) instead of blob offset 760.
- **MSVC toolset:** 14.44.35207 — cl.exe 19.44.35225 (VS2022 Enterprise)
- **DLLs rebuilt:** Mission.dll only
- **Source changes:**
  - `Code/App/Src/FogOfWar.cpp`: remove nested ObjMapIterator it2 diagnostic
  - `Code/App/Src/CMenu_Ind.cpp`: guard namearea dereferences with if(namearea)
  - `Code/App/Src/data.i` + `Code/App/DInclude/DToolbar.h`:
    reorder COMMON STATICs: shipclass before gas,metal,crew

---

## Template for future entries

```
## YYYY-MM-DD — <short description>

- **Trigger:** <why the rebuild happened>
- **MSVC toolset:** 14.44.35207 — cl.exe 19.44.35225 (VS2022 Enterprise)
- **DLLs rebuilt:** <list, or "All">
- **Source changes:** <file(s) and what changed, or "None / settings change">
```
