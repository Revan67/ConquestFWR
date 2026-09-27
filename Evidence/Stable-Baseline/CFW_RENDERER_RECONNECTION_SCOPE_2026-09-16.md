# CFW renderer reconnection scope

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

Status: proposed scope, source-audited 2026-09-16. Not authorization to remove
components; no runtime code changes made for this scope. Retail equivalence
must be established per behavior rather than inferred from historical notes.

## Decision and counting

Target: CFW authored-content semantics through Engine -> RenderMgr -> PolyMesh ->
ZBatcher -> CQPipeline, retaining the present D3D9 backend. Restoring those semantics
does NOT mean replacing the backend with retail's older graphics DLLs.

Budget **12 integration work packages**, not 12 line edits or 12 proven broken
connections. Ordinary world drawing is already primarily routed to CFW. Current
source contains five direct instanceMesh->Render fallback sites: two ship branches,
two platform branches, one planet branch. Audit/retire these only after valid
instance creation and specialized effects have explicit contracts.

Additional concrete junctions: two legacy-index preview creation branches in
UnbornMeshList (platform and ship), one platform shadow-preview creation site,
three MeshManager archetype-loading families (ship/platform/planet), and one
shared ObjectTransform creation/teardown boundary. These overlap the packages
below; summing them does not give a meaningful total number of fixes.

Runtime evidence: PID 36200 recorded 31,402 PolyMesh entries and 2,036 application
manual Render entries, with no MeshInstance/MaterialManager entries recorded.
This does not make the newer asset/setup components unused, nor cover unvisited
factions/effects. Counters are overlapping layer calls, not utilization percentages.

## Work packages and acceptance

| # | Boundary | Work and required evidence |
|---|---|---|
| 1 | Archetype ownership | Reconcile ship/platform/planet legacy Engine handles with meshArch; define borrowed versus owned references and failure behavior. HQ dump proves THQ wrapper index 250 versus legacy -1. Verify repeated load/unload, no double release. |
| 2 | Object instance lifetime | Make shared ObjectTransform creation, callbacks, updates, and destruction consistent with Engine ownership. Preserve the services currently supplied by wrappers until replacements exist. Test spawn, destruction, resign, reload. |
| 3 | Ordinary object draw selection | Audit five fallback sites; retain CFW normal drawing with explicit exceptional paths. Cover normal/warp ship, platform including exploding-parent rendering, and planet. No silent invalid-instance fallback. |
| 4 | Placement and shadow previews | Fix failed-creation handling and stale platform handle; retain tint, transparency, compound child transforms, and cancellation. HQ/turret/other placement plus platform shadow states; never cache an invalid preview. |
| 5 | Team colors and material ownership | Replace/bridge IMaterial TeamColor modifiers where CFW drawing needs legacy markings; distinguish per-instance overrides from shared archetype mutations. Check two players, cargo pods, capture/recolor. |
| 6 | Animation, hardpoints, callbacks | Restore Engine/Anim equivalents for MeshInstance animation and MeshArch hardpoint services where required. Test scripts, weapon/beam emitters, attachments, and construction joints. Do not infer animation correctness from draw correctness. |
| 7 | Ship presentation transforms | Establish retail axis/warp/producer transform contracts; separate display transforms from gameplay callbacks. Validate first jump frame, correction, and first visible construction frame in mission and skirmish. |
| 8 | Construction/dismantle effects | Trace race-specific build effects, ObjectBuild handoff, and manual geometry paths against retail. Preserve percentages, attachments, completion, cancellation, save/reload; verify all three races. |
| 9 | Cargo, debris, clipping/deformation | Audit application TreeRender consumers and specialized RenderPortion/ZAlign/Puffy behavior. Keep or restore retail manual effects; do not substitute ordinary Engine rendering for modified geometry. Test cargo appearance, explosions, jump clipping, special weapons. |
| 10 | Planet layers and asset transitions | Establish surface/cloud material, animation, alpha/depth and terraform transitions against retail assets/traces. Test first frame, camera zoom, repeated missions, and changing texture appearance. Do not assume every texture change is wrong. |
| 11 | CQ2 programmable materials and effects | Inventory actual asset/factory reachability through EffectPlayer/ParticleEffect/MeshManager/MaterialManager. Replace retail features with verified retail equivalents; isolate genuinely new shader/modifier/dynamic-mesh features until ported or explicitly excluded. |
| 12 | Shared state and component retirement | Establish direct-PIPE/ZBatcher state ownership and transition synchronization. Expand traces beyond HQ crash; remove manager initialization/INI/DLL dependencies only after no remaining required consumers. Full Debug/Final rebuild/deploy/hash and regression checks. |

## What cannot be redirected unchanged

- **Programmable materials:** MaterialManager uses shader/effect techniques,
  additional vertex attributes and named modifiers. Plain CFW material submission
  does not consume that interface. A fixed-function equivalent or original retail
  effect is a port, not a different call to render_instance.
- **Procedural geometry:** sliced/cut/dynamic meshes and manual partial/puffy
  rendering cannot be replaced with a draw of the untouched Engine archetype.
  Keep a compatible manual effect path or reconstruct the retail implementation.
- **Effect services:** EffectPlayer queries object meshes and hardpoints, drives
  effect parameters/listeners and hides ordinary object drawing. Removing wrappers
  solely because their drawing counter is zero can break these services.
- **Material modifiers:** MeshInstance forwards modList and timer to DrawVB;
  Engine rendering does not. Color/opacity/reveal semantics need an explicit
  replacement even where the underlying model is a retail CFW asset.

These are interface limitations of the present CFW path, not claims that the
features are impossible to implement using it. Nor are all these capabilities
proved exercised by the latest mission capture.

## Rollback policy

For a feature present in retail CFW, the intended baseline is its retail behavior.
Use original assets plus retail disassembly/traces to recover the appropriate
Engine/Anim/manual-effect contract. This is the preferred disposition for normal
ships, structures, planets, previews, build effects, cargo and retail weapons.
Exact parity is not yet proved for every effect or faction.

For genuinely CQ2-only behavior, retail has no equivalent to roll back to. Choices:
port to a CFW-compatible implementation, retain an isolated adapter, or explicitly
exclude the feature after proving it is not needed by target retail content.
Do not silently delete gameplay features or their saved/data fields.

The application manual renderer is not synonymous with CQ2. Its consumers include
BuildShip, MantisBuild, HarvestShip cargo, debris, explosion shrapnel, JumpLauncher,
MassDisruptor and placement previews. Classify individual operations/materials.
The duplicate library MeshRender.cpp is excluded from the current runtime build;
deleting it would not reduce current drawing exposure.

TextureManager, StringTable and VideoSystem retirement is not automatically part
of renderer restoration. Separate their consumers before any INI or DLL removal.
Do not mix retail binary modules into the rebuilt ABI merely to get a legacy path.

## Sequence and stop gates

1. Baseline repair: packages 1-4, with the HQ null-dereference fixed defensively
   and its invalid creation input corrected. Record current dump/build identity.
2. Presentation parity: 5-7 and 10, using repeatable mission/skirmish scenarios.
3. Effect parity: 8-9 and 11, tracing each reachable retail effect and faction.
4. Retirement: 12, after dependency closure, with startup/shutdown and reload tests.

State-boundary research begins in stage 1 even though final removal is stage 4.
Each stage requires static checks, focused tests, full verified builds, and user
visual acceptance. A component with unresolved required consumers stays in place.

## Source anchors checked for this scope

- TPlatform.h: loadPlatformArchetype, platform rendering, shadow instance creation,
  destructor; TSpaceShip.h and Planetoid.cpp archetype creation/lifetime.
- TObjTrans.h: initTransform/destructor; TObjTeam.h: SetColors;
  TObjEffectTarget.h: PlayAnimation/GetHardPointIndex/listeners.
- SpaceShip.cpp, TPlatform.h, Planetoid.cpp: five wrapper draw fallbacks.
- UnbornMeshList.cpp: createMesh/RenderMeshAt; application MeshRender.cpp and its
  TreeRender/RenderPortion/RenderPuffy callers.
- MeshInstance.cpp: Render -> Material::DrawVB(modList,timer);
  MeshArch.h/cpp and MeshManager.cpp: handles and reference release.
- EffectPlayer.cpp: GetObjectMesh, hardpoint mesh access and effect Render.
- Evidence/Stable-Baseline/HQ_PREVIEW_CRASH_2026-09-15.md and
  RENDERER_USAGE_TRACKING.md: current runtime/build evidence.

Retail research stores exist at D:\Dev\ghidra-proj and D:\Dev\traces. Older
RENDER_PIPELINE_AUDIT and `Evidence/Historical/Milestone-1/RENDERER_CONSUMER_INVENTORY.md` notes contain corrected claims
(excluded duplicate renderer and Queue2 behavior); do not reuse their raw counts
or obsolete defect statements as an implementation mandate.
