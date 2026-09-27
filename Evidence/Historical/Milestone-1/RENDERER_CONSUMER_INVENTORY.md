# Renderer Consumer Inventory

> **Historical documentation — provenance only.** This document is not current project authority. See [PROJECT_BIBLE.md](../../../PROJECT_BIBLE.md) for canonical status; preserved for provenance.

Date: 2026-09-06

2026-09-14 correction: the library MeshRender.cpp below is excluded from the current
project build. Its direct-pipeline references must not be counted as runtime exposure.
MeshManager remains active-capable through MeshInstance -> MaterialManager. See
`Evidence/Stable-Baseline/RENDERER_USAGE_TRACKING.md` for verified build coverage and
14 compiled opt-in counter sites. Queue2's current loop disables grouping but does not
by itself drop/repeat draws; earlier correctness claims require that qualification.

## Purpose

This inventory separates the authoritative CFW authored-content route from
manual/CQ2 consumers that share and can contaminate the same mutable pipeline.
Counts are static call-site counts, not proof that every path executes in Mission 1.

## Static concentration of direct pipeline access

| Source | Direct `PIPE`/`renderpipeline` uses | Classification |
|---|---:|---|
| `Code/Libs/Src/MeshManager/MeshRender.cpp` | 198 | CQ2/manual renderer; high-risk exception |
| `Code/App/Src/MeshRender.cpp` | 197 | Duplicate app manual renderer; high-risk exception |
| `Code/App/Src/RenderBatcher.cpp` | 166 | Authoritative CFW batching/state cache |
| `Code/App/Src/System.cpp` | 77 | Mixed world/effect/test/UI consumer |
| `Code/App/Src/SysMap.cpp` | 44 | UI/system-map consumer |
| `Code/App/Src/DrawAgent.cpp` | 21 | UI drawing consumer |
| Light code | 16 | World-light consumer |
| `SpaceEnv` / `FighterWing` | 15 | World/effect consumers |
| `FogOfWar` | 14 | World overlay consumer |
| MaterialManager material code | 14 | CQ2/manual material boundary |

The two MeshRender implementations plus MaterialManager contain 482 calls matching
the principal state, texture, and shader mutation patterns. They can bind directly
through `PIPE`, bypassing ZBatcher's desired/committed cache.

## Ownership decision

The authoritative retail-content route remains:

`object -> Engine -> RenderMgr -> PolyMesh -> ZBatcher -> CQPipeline -> D3D9`

MeshManager, MaterialManager, D3DX-effect, and other direct-pipe paths are exceptions,
not peer renderers. They are retained only until runtime consumers are proven. No
component is removed solely because it was absent from retail configuration.

## Required follow-up

1. Instrument entry/exit of each active exception by caller category.
2. Capture the render, texture-stage, sampler, shader, transform, and texture state
   changed by that exception.
3. Restore captured state and explicitly invalidate/synchronize ZBatcher on exit.
4. Prove which of the duplicate MeshRender implementations has live consumers.
5. Consolidate/remove only after that proof.
6. Fix the separately confirmed ZBatcher Queue2 index defect only after a focused
   retail comparison and queue-path regression test.

Blanket per-object defaults are rejected: they would hide ownership defects and can
break legitimate translucent/effect passes. State boundaries belong at renderer
transitions and the world-frame boundary.
