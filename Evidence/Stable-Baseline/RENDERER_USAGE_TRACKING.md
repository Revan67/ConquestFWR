# Renderer ownership and usage tracking — 2026-09-14

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

Latest status: Debug and Final tracking builds are now deployed and verified, including
tracking-enabled launchers. See TRACKING_BUILD_UPDATE_2026-09-14.md for current provenance.
The earlier NOT-deployed statement below describes the initial instrumentation-only build.

## Decision status

We have not proved that a wholesale renderer rollback is safe. Preserve the retail CFW
Engine/RenderMgr/PolyMesh path for authored world objects. Current effects have MeshManager,
MaterialManager and application-manual-renderer dependencies that require explicit migration
before removal. These are front ends sharing the same backend, not two isolated devices.
Retaining an effect dependency temporarily does not establish it as the desired final design.

The planet's primary draw currently calls ENGINE->render_instance; its fallback can call
instanceMesh->Render. MeshInstance::Render passes modList and timer to Material::DrawVB;
redirecting it to the engine would not forward those effect modifiers. Runtime fidelity of
the deployed candidate is still pending the user's focused test.

## Opt-in measurements

Set `CFW_RENDER_USAGE=1` in the environment of a NEW instrumented game process. Counters
write `%TEMP%\CFW-RenderUsage-<PID>-<route>.log`. They report cumulative entry calls at
approximately five-second intervals while the route is active and once at clean teardown.
When disabled (default), no files are created. No registry or game configuration is changed.
Build identity must come from the build/deployment manifest, not the PID alone.

| Layer | Routes |
|---|---|
| World frame entry | world-frame |
| CFW renderer entry | cfw-polymesh-render |
| MeshManager object entry | meshmanager-instance |
| Manual renderer entry, application copy | appmanual-Render, appmanual-RenderPortionZAlign, appmanual-RenderPortion, appmanual-RenderPuffy |
| Manual renderer entry, library copy | libmanual-Render, libmanual-RenderPortionZAlign, libmanual-RenderPortion, libmanual-RenderPuffy |
| Material layer | materialmanager-vb, materialmanager-pass |
| Backend API attempts, all callers | backend-draw_primitive, backend-draw_indexed_primitive, backend-draw_primitive_vb, backend-indexed-vb-cpuindices, backend-indexed-vb-gpuindices |

There are 18 source call-site hooks, but only 14 are compiled in the current game build.
The four `libmanual-*` hooks are in a source file excluded from MeshManager.vcxproj. A
repository-wide .vcxproj search lists application and separate Editor MeshRender entries,
but none for the library copy; Editor is not part of this game runtime build. Binary coverage
checks found all other route labels in their expected freshly rebuilt EXE/DLLs, and none
of the four library-manual labels. Those absent routes mean NOT BUILT, not an idle renderer.
This corrects earlier audit claims that both duplicate MeshRender files were built/live.

A PolyMesh entry can cull without drawing or represent a
compound child; a material can produce multiple passes. Backend entries include UI and other
callers and do not carry upstream attribution. Three backend counters occur after buffer
creation checks, while the indexed-VB counters occur before them; they are not success counts.
This first tier establishes which fronts execute and their call volume. It cannot honestly
produce a percentage of pixels, GPU time, or old-versus-new backend draws.

Read with `Tools/Verification/Read-RenderUsage.ps1 -GameProcessId <PID>`. The reader takes
only the latest cumulative sample for each route, never sums samples, and labels the layers.
An absent route is not automatically zero until instrumented build coverage and enablement
are established. Crash/forced termination can omit final totals. An idle route's latest
sample can predate its final active calls until clean teardown. Counts are unsigned 32-bit;
long captures must account for wrap. No textures, models, save content, or user input is logged.

## Verification

`Build-RenderUsageProbeTest.cmd` compiles a standalone x86 test. Four worker threads each
record 10,000 calls; after teardown the file contains 40,000. An opt-out counter creates
no file. Test and reader passed on 2026-09-14. This is instrumentation validation, not a
gameplay usage measurement. Full game rebuild status is recorded separately below.

Full Debug rebuild completed at 2026-09-14 14:59:09 UTC, exit 0: 22 engine and 18 campaign
modules rebuilt/verified. Build-input fingerprint:
`99A0861F321CB162F37A47923B2E8D80AC7D9A275777EA031EC174AE084705E6`.
The complete parser conversion probe was rerun against these DLLs: exit 0, zero parser
errors. This instrumented build has NOT been deployed; the existing test installation is
unchanged. No gameplay usage measurements have yet been collected.

## Capture plan

Use a cold Mission 1 load followed by distinct intervals: opening arrival/idle, combat,
fabricator/structure construction, placement ghost, and harvesting. Record interval times
and process/build ID. Repeat Skirmish construction because its spawn path differs. Preserve
logs on clean exit. First report route activity/call deltas; only then decide whether a
second-tier caller-attributed backend trace is needed for an actual draw-share percentage.

## Corrected Queue2 interpretation

Earlier audits claimed FlushQueue2's i/j indexing redraws one item and discards others.
Current source first enters with j=i, draws i and clears i.num_verts. The inner condition
then fails for every j>i. The next outer iteration draws its own still-live entry. A three
item A/B/A model yields indices 0,1,2 exactly once. This disables intended material grouping,
but is not by itself evidence of dropped or repeated geometry. Retail comparison and draw
ordering still matter before optimizing it. No Queue2 code change was made in this pass.

## Remaining state-boundary question

Material::DrawVB calls SetShaderConstants before effect Begin. SetShaderConstants directly
sets texture bindings and render state through PIPE. There is no explicit local restoration
of those pre-Begin changes in DrawVB. Whether downstream draws repair each state must be
traced against ZBatcher's cached/committed state; usage counts identify exposure, not the
cause of a particular texture failure. Removing unused fronts is preferable only after
their consumers and replacement behavior are established.

Additional source concern: CreateModifierRenderState stores the override value in
mod.stateValue and identifies the original MatState through stateNode. SetShaderConstants
then uses stateValue as the render-state identifier and the original value as its argument.
Those roles appear reversed. A repository-wide search found declarations/implementation but
no call sites for CreateModifierRenderState, so this is not currently evidence of an active
gameplay failure. No speculative change was made. The same distinction applies to the empty
Material::DrawUP implementation: no callers were found in the current source search.
