# Render Pipeline Audit: CFW Restoration

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

Date: 2026-09-05; planet diffuse incident resolved 2026-09-06

2026-09-14 correction: the Queue2 i/j loop described below does not by itself discard or
duplicate geometry: clearing i at j=i prevents later inner-loop matches, so it draws each
entry individually. See `Evidence/Stable-Baseline/RENDERER_USAGE_TRACKING.md` for corrected
control-flow analysis, current ownership decision and opt-in runtime usage counters.

2026-09-14 build-graph correction: MeshManager.vcxproj compiles MeshArch.cpp,
MeshInstance.cpp, MeshManager.cpp and stdafx.cpp, NOT its duplicate MeshRender.cpp.
Only the application MeshRender.cpp is in the game build; the separate Editor project
also lists its own MeshRender.cpp and is not part of this runtime build.
Fresh binary counter-label coverage confirms this. Counts of direct PIPE references in
the excluded library file are source inventory, not live runtime exposure.

## Scope and evidence rules

This audit traces the current render paths from game object to final D3D9 device and compares the reachable architecture with retail CFW. It does not treat prior notes or reconstructed source comments as authoritative.

Evidence labels:

- **Source-confirmed**: directly visible in the current source/build graph.
- **Runtime-confirmed**: directly observed in logs from the deployed build.
- **Retail-binary-confirmed**: visible in retail imports, component names, configuration, or asset hashes.
- **Inference**: a technically supported explanation that still needs a focused runtime test.

## Executive finding

The game is not selecting between two isolated renderers. It has two asset/rendering fronts that share one mutable rendering backend:

1. Original CFW authored path: `Planetoid/game object -> Engine -> RenderMgr -> PolyMesh -> ZBatcher -> CQPipeline -> D3DRenderPipe`.
2. CQ2/manual path: `MeshManager or Conquest MeshRender -> MaterialManager/manual state -> ZBatcher and/or direct CQPipeline calls -> D3DRenderPipe`.

The paths converge on the same D3D9 device and mutate the same texture, stage, blend, depth, transform, and shader state. The CQ2/manual path sometimes bypasses ZBatcher, so ZBatcher's cached idea of device state can diverge from the actual device. Retail has ZBatcher but does not load the CQ2 managers that introduce this bypass pattern.

Three independent defects are confirmed:

1. **No depth surface is created or bound.** `create_buffers()` sets `D3DRP_F_DEPTH_CREATED` while `EnableAutoDepthStencil` is false and no `CreateDepthStencilSurface`/`SetDepthStencilSurface` call exists. This explains failed planet/ring occlusion. It does not explain the white planet.
2. **Resolved: global `RP_TEXTURE` state leaked disabled.** A working/failed runtime A/B trace
   proved PolyMesh had valid Earth geometry and a valid texture handle in both runs. The failed
   run retained `RWM_DONT_TEXTURE`; the working run began texturing only after an incidental path
   re-enabled `RP_TEXTURE`. `ObjectList::Render()` now re-establishes the configured texture state
   at the world-frame boundary. Two consecutive runs rendered immediately and logged texturing
   enabled from the first Earth draw. See `PLANET_TEXTURE_STATE_LEAK.md`.
3. **Texture state can be changed behind ZBatcher's cache.** CQ2/manual render code calls
   `PIPE->set_texture_stage_texture()` directly. ZBatcher can therefore skip a required bind when
   `commitedTextureID[]` diverges from the device. This remains confirmed architectural debt, but
   it was not the differentiating cause of the captured intermittent white-planet incident.

Cloud alpha/blending remains relevant to material fidelity, but it did not cause the captured
intermittent loss of the Earth diffuse pass.

## Current component topology

### Startup and interface ownership

- **Source-confirmed:** `SysContainer.cpp` reads `[System]`, aggregates each component, then inserts `CQPipeline` at the front of the aggregate list.
- **Source-confirmed:** `CQPipeline` is a synchronized pass-through wrapper. It does not maintain a complete independent render-state model.
- **Source-confirmed:** `Conquest.cpp` obtains `IRenderPipeline` as `PIPE` and `IRenderPrimitive` as `BATCH` from the aggregate.
- **Source-confirmed:** `ZBatcher.dll` registers the high-priority `IRenderPrimitive` component. PolyMesh therefore submits through ZBatcher rather than directly to D3DRenderPipe.
- **Retail-binary-confirmed:** retail also imports and loads `ZBatcher.dll`; batching itself is original CFW architecture, not a CQ2 addition.

### Retail configuration delta

Current `Conquest.ini` adds these libraries absent from retail:

- `TextureManager.dll`
- `StringTable.dll`
- `MaterialManager.dll`
- `VideoSystem.dll`
- `ParticleEffect.dll`
- `MeshManager.dll`

It also aggregates their interfaces under `[System]`. The original retail libraries remain loaded alongside them. This is additive CQ2 integration, not a clean replacement of the CFW renderer.

### Built module ownership

- **Source-confirmed:** `Trim.vcxproj` builds `SysContainer.cpp` and `cqpipeline.cpp`.
- **Source-confirmed:** `ZBatcher.vcxproj` builds the application `RenderBatcher.cpp`.
- **Source-confirmed:** `Conquest.vcxproj` builds `Code/App/Src/MeshRender.cpp`.
- **Source-confirmed:** `MeshManager.vcxproj` builds `MeshInstance.cpp` and its own large `MeshRender.cpp` implementation.
- **Retail-binary-confirmed:** retail and current D3DRenderPipe/ZBatcher binaries have different hashes. The current final pipe imports D3D9/D3DX9; retail's final pipe does not and retains the DirectDraw-era implementation boundary.

## Original CFW authored path

### Upstream: object and archetype creation

1. A game object/factory creates an Engine instance from an authored `.3db` or compound `.cmp` asset.
2. Engine creates compound children and retains their local transforms.
3. RenderMgr chooses PolyMesh/Optics using `[RenderManager]`. Current and retail configuration both list PolyMesh then Optics.
4. `XMesh` reads the authored material library. For the planet children it takes the original `Material library` branch, not the `CQ2_Material_Library` branch.
5. `XMesh` asks TextureLibrary to load the embedded texture library before resolving material texture references.

### Planet-specific runtime evidence

- **Runtime-confirmed:** all four large Earth archetypes create separate `earth` and `clouds` child PolyMeshes.
- **Runtime-confirmed:** the Earth children have nonzero texture handles.
- **Runtime-confirmed:** the cloud children have a shared nonzero texture handle.
- **Runtime-confirmed:** the Earth texture decoded into nonuniform BGRA data; it is not a blank source asset or failed upload.
- **Runtime-confirmed:** the cloud material carries `TF_F_HAS_ALPHA` (`0x80000000` within its flags).
- **Runtime-confirmed:** the final-pipe diagnostic did not observe the identified Earth handle on the expected final non-VB draw path.
- **Qualification:** the earlier face-count diagnostic was invalid because PolyMesh performs face culling before submission. Texture-handle tracing supersedes it.

### Material submission

PolyMesh:

1. Computes lighting and object-to-view transform.
2. Reads `RP_TEXTURE`; if false it deliberately submits untextured geometry.
3. Configures stage 0 for texture x diffuse modulation.
4. Binds each material texture through `IRenderPrimitive`/ZBatcher.
5. Enables alpha blending when material transparency is below 255 or a bound texture reports alpha.
6. Enables depth test and depth writes by default.
7. Assigns `RPR_STATE_ID`, submits indexed triangles, then clears the state ID.

### ZBatcher behavior

ZBatcher maintains both desired and believed-device texture state:

- `currentTextureID[]`: desired texture.
- `commitedTextureID[]`: texture it believes the final pipe already has.
- `texture_stage_state[][]`: desired fixed-function stage state while batching.

Small, translucent, or delayed primitives are copied into material pools and restored later. Large opaque primitives can pass through after committing cached state. Large vertex-buffer submissions have two additional queues.

Important source issues:

- **Source-confirmed:** direct `PIPE` mutations outside ZBatcher do not invalidate `commitedTextureID[]` or the batcher's stage-state cache.
- **Source-confirmed:** `FlushQueue2()` uses item `i` for transform/draw while clearing item `j`; Queue1 correctly uses `j`. This can redraw one compatible item and discard others. Retail equivalence has not yet been proven, so this is a verified source defect but not yet classified as a retail regression.

## CQ2/manual path

### MeshManager path

`MeshManager::MeshInstance::Render()` contains a sample/manual face-group renderer for CQ2/dynamic meshes. The current working change redirects non-dynamic authored meshes back into Engine, while dynamic meshes retain the manual path.

The manual renderer:

- uses MaterialManager/D3DX effects for CQ2 materials;
- also contains a fixed-function path for non-CQ2 materials;
- writes render, sampler, texture-stage, shader, transform, and texture state;
- sometimes submits through BATCH and sometimes calls PIPE directly;
- has cleanup routines that set a handful of guessed defaults rather than restoring captured prior state.

### Conquest executable path

`Code/App/Src/MeshRender.cpp` is a second compiled manual renderer with similar logic. Its `SetCommonStates()` currently returns immediately after computing camera position, making the intended common-state initialization unreachable. Individual branches consequently own partial state setup and cleanup.

### Confirmed cache bypasses

Active direct texture writes exist in:

- both large MeshRender implementations;
- MaterialManager material setup;
- FighterWing trails;
- System test/effect rendering;
- DAUI big-image rendering and several supporting tools.

Not every bypass executes during the planet scene, but any active bypass between ZBatcher commits can invalidate its assumptions. Direct render-state and texture-stage-state calls create the same class of risk beyond textures.

### Historical ZBatcher hypothesis (not the captured incident cause)

The following was a reasonable pre-A/B hypothesis based on source architecture:

1. Earth and cloud textures are successfully loaded.
2. A CQ2/manual renderer binds a texture directly through PIPE.
3. ZBatcher's `commitedTextureID[0]` is not updated or invalidated.
4. PolyMesh requests Earth through BATCH.
5. ZBatcher sees desired Earth equal to its stale committed value and omits the real device bind.
6. The Earth geometry is drawn with another texture or stale stage state.
7. The cloud shell is later drawn and can visually dominate the result, yielding the uniform white/grey ball.

Subsequent PolyMesh-boundary A/B traces instead found `RWM_DONT_TEXTURE` before the diffuse bind.
The cache-bypass chain remains possible elsewhere and still warrants remediation, but must not be
cited as the cause of the resolved planet incident without a new reproducer and boundary trace.

## Final D3D9 pipe

### Device and resources

- Uses `IDirect3D9Ex`/`IDirect3DDevice9Ex` in a windowed X8R8G8B8 swap chain.
- Uses default-pool dynamic textures and buffers, required by the chosen D3D9Ex design.
- Converts source pixels to 32-bit data on upload. Opaque `DAOP` and `DAOT`
  texture classes must create `X8R8G8B8` resources; alpha classes create
  `A8R8G8B8`. The prior D3D9 port recognized only `DAOT`, causing palettized
  opaque textures such as the Earth surface to be misreported as alpha-bearing.
- Uses fixed-function FVF draws for original PolyMesh submissions and D3DX effects for CQ2 paths.

### Confirmed depth defect

`create_buffers()`:

- computes a supported `depthFmt`;
- sets `EnableAutoDepthStencil = FALSE`;
- never uses `depthFmt` to create a surface;
- never calls `SetDepthStencilSurface`;
- nevertheless sets `D3DRP_F_DEPTH_CREATED`.

`clear_buffers()` trusts that flag and requests `D3DCLEAR_ZBUFFER`. Render states can say depth is enabled while the device has no depth attachment. Ring sections therefore cannot be reliably occluded by the planet.

`destroy_buffers()` resets the device with a materially different presentation configuration (`EnableAutoDepthStencil = TRUE`, D16), another lifecycle inconsistency that should be removed.

### Other final-pipe contract defects

- `is_texture()` returns success without validation.
- `lock_texture()`/`unlock_texture()` report failure even after successful work.
- texture and render-state caches are compiled disabled, but their code still contains correctness hazards if re-enabled.
- buffer reset/recreation is inconsistent and does not have a single authoritative resource-lost/resource-reset lifecycle.
- diagnostic changes currently remain in the pipe and PolyMesh and must be removed after verification.

These require remediation, but none should be declared the white-planet cause without a boundary trace.

## Retail comparison: what is and is not proven

### Proven

- Retail uses Engine, RenderMgr, PolyMesh, TextureLibrary, ZBatcher, Trim, and D3DRenderPipe.
- Retail does not load the CQ2 TextureManager/MaterialManager/MeshManager stack in `Conquest.ini`.
- Retail and current authored object databases match by SHA-256 in the checked installations.
- Retail D3DRenderPipe is a DirectDraw-era binary; current D3DRenderPipe is a D3D9 port.
- Therefore the assets are not the primary retail/current difference, while component graph and final device implementation are.

### Not yet proven

- Source-level identity of current ZBatcher logic to retail ZBatcher.
- Exact retail depth-surface selection and creation flags.
- Whether retail cloud rendering depends on a D3D7 behavior not reproduced by D3D9 fixed-function state.
- Whether one specific CQ2 bypass is the only white-planet trigger.

These claims require either focused runtime instrumentation or retail disassembly; they must not be inferred from reconstructed comments.

## Remediation order

### Agreed target architecture — 2026-09-06

Do not replace the renderer wholesale and do not continue treating the CFW and CQ2 paths as equal,
free-standing pipelines. Make the original authored CFW route authoritative for retail content:

`CFW object -> Engine -> RenderMgr -> PolyMesh -> ZBatcher -> CQPipeline -> D3D9`

Retain CQ2/manual rendering only for a shipped feature that has been proven to require it. Every
retained exceptional path must have an explicit state boundary: capture/restore the state it
changes and synchronize or invalidate ZBatcher's cached state before returning to the CFW route.
Original `.3db` and `.cmp` content should not be migrated to the manual MeshManager path merely to
work around a backend defect.

The implementation sequence is:

1. Restore one coherent D3D9 depth-buffer creation, binding, clearing, reset, and teardown
   lifecycle. This is the immediate priority and directly targets missing planet/ring occlusion.
2. Establish authoritative world-frame defaults for texture, depth, blend, cull, shader,
   transform, and texture-stage state. The resolved `RP_TEXTURE` leak proves this ownership is
   necessary.
3. Inventory actual runtime consumers of MeshManager, MaterialManager, D3DX effects, and direct
   PIPE calls; constrain these paths to demonstrably required CQ2/dynamic features.
4. Replace guessed cleanup defaults in retained manual paths with scoped state restoration and
   explicit ZBatcher cache synchronization/invalidation.
5. Consolidate the two overlapping manual MeshRender implementations after their consumers are
   proven; remove or quarantine the redundant implementation.
6. Repair ZBatcher correctness, including the confirmed Queue2 indexing defect, and audit its
   desired-versus-committed state model.
7. Repair final D3D9-pipe contracts and resource-loss/reset handling, including truthful texture
   validation and lock/unlock return values.
8. Validate planet fidelity as separate properties: Earth diffuse, cloud alpha/order, animation,
   close-range smearing, and far-side ring occlusion.
9. Remove unshipped CQ2 system components only after proving no shipped CFW feature consumes them.
10. Regress planets, ships, projectiles, construction previews/animations, refinery attachments,
    explosions, translucent effects, UI overlays, mission reloads, Alt-Tab/device reset, and
    multiple missions in both Debug and Final builds.

The durable rule is: one authoritative CFW content route, explicit boundaries around every
retained CQ2 exception, and no unowned process-global render state.

### Phase 1: establish authoritative state boundaries

1. Add a narrow ZBatcher trace for the planet Earth/cloud handles:
   - desired texture;
   - believed committed texture;
   - actual PIPE texture before and after material restore;
   - queued material/state ID;
   - final draw route (pooled UP, direct UP, Queue1 VB, Queue2 VB).
2. Add a narrow trace to every active direct PIPE texture mutation during the same frames, including caller category and stage.
3. Confirm draw order of Earth and cloud child instances.
4. Record blend enable/functions, stage alpha operation/arguments, texture alpha range, Z enable/write/function, and actual depth attachment at the final draw.

Exit criterion: one frame can be reconstructed from authored child through final device without relying on face counts or guessed handles.

### Phase 2: make CFW state ownership coherent

1. Route fixed-function texture/stage/render mutations used during gameplay through BATCH, or add an explicit ZBatcher invalidation/commit API around unavoidable direct PIPE/D3DX effect blocks.
2. Replace guessed cleanup defaults with scoped state capture/restore at CQ2 boundaries.
3. Keep authored non-dynamic CFW meshes on Engine/RenderMgr/PolyMesh.
4. Restrict MeshManager/MaterialManager to assets that are demonstrably CQ2/dynamic and still required by shipped gameplay.
5. Remove the redundant Planetoid root TextureLibrary preload if the boundary trace confirms `XMesh::read_material_library()` already loads all child libraries.

Exit criterion: actual PIPE state and ZBatcher cached state agree after every renderer transition.

### Phase 3: restore depth lifecycle

1. Choose one D3D9 depth strategy. Auto depth is the least complex unless off-screen targets require manual attachment.
2. Create and bind a supported D24S8 or D16 surface during `create_buffers()`.
3. Set `D3DRP_F_DEPTH_CREATED` only after success.
4. Use the same presentation/depth configuration for creation and reset.
5. Verify depth clear succeeds and ring sections pass behind the planet.

Exit criterion: device reports a bound depth-stencil surface and the retail occlusion test passes.

### Phase 4: validate planet material fidelity

1. Verify Earth and cloud source pixels and alpha separately.
2. Verify D3D9 texture format and uploaded alpha ranges for base level and mip levels.
3. Verify cloud material blend and depth-write behavior against retail observation.
4. Verify Earth/cloud rotation and texture-coordinate animation independently of visibility.
5. Compare screenshots at the locked 1600-wide target and one legacy 4:3 mode.

Exit criterion: textured Earth is visible, cloud transparency is correct, texture motion does not smear, and compound ring geometry occludes correctly.

### Phase 5: retire unsafe split behavior

1. Remove unused CQ2 system components only after confirming no shipped CFW feature consumes them.
2. Consolidate or delete duplicate manual MeshRender implementations rather than maintaining two stateful copies.
3. Fix or quarantine Queue2 after retail behavior is established.
4. Repair final-pipe interface contracts and resource-reset lifecycle.
5. Remove temporary diagnostics and record a clean module/config manifest.

Exit criterion: original CFW content has one authoritative authored render route; exceptional dynamic/effect rendering has explicit state boundaries and cannot contaminate it.

## Current next implementation

The intermittent planet diffuse failure is resolved through centralized frame-state ownership and
documented in `PLANET_TEXTURE_STATE_LEAK.md`. Retain the narrow PolyMesh diagnostic through broader
regression testing. The next independent planet priority is depth lifecycle restoration because
its missing surface/attachment defect is already conclusive; then validate cloud blending and
animation. ZBatcher/direct-PIPE cache coherence remains a later pipeline-hardening task.
