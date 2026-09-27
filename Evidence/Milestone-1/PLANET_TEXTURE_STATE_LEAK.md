# Incident Report: Intermittently Untextured Planet and Ships

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

Date resolved: 2026-09-06

Status: runtime-confirmed in two consecutive Mission 1 runs; narrow diagnostics retained for
broader regression testing.

## Summary

Tau Ceti Prime intermittently rendered as a white/grey sphere rather than with its authored Earth
diffuse texture. The delay varied from apparently instant to more than a minute, and on some runs
the texture never appeared. Affected runs could also show simplified/untextured ship geometry.

The texture was not loading late. The original PolyMesh renderer was being instructed not to use
textures because the process-global `RP_TEXTURE` pipeline state had leaked as disabled. An
unrelated later render path could set it true, creating the misleading appearance of asynchronous
loading or a random texture upload delay.

The correction establishes `RP_TEXTURE` from the configured texture option at the beginning of
every world-object render frame in `ObjectList::Render()`:

```cpp
PIPE->set_pipeline_state(RP_TEXTURE, CQEFFECTS.bTextures != 0);
```

This is a frame invariant, not a planet-specific workaround. It preserves the user's texture
option and prevents one object's incomplete state cleanup from disabling texturing for later
objects or frames.

## User-visible symptoms

- The planet initially appeared solid black during earlier, separate renderer experiments.
- After restoring the Engine/RenderMgr/PolyMesh path, it appeared as a lit white/grey ball.
- The diffuse texture sometimes appeared after roughly 2, 5, 6, 8, 12, 20, or 62 seconds.
- Other runs remained untextured beyond three minutes.
- Warm mission reloads often appeared faster than cold client starts, but not reliably.
- At least one failed run also displayed Blackwell using simplified/untextured polygons.
- Mission start could hitch, which initially made streaming or synchronous loading plausible.

Timing was not a dependable diagnostic signal. The decisive evidence came from comparing the
render state at the same Earth PolyMesh boundary in one working and one failed run.

## Architecture relevant to the failure

The original authored path is:

`Planetoid -> Engine -> RenderMgr -> PolyMesh -> ZBatcher -> CQPipeline -> D3DRenderPipe`

The reconstructed application also contains CQ2/manual rendering paths. These paths ultimately
share the same pipeline and D3D device with the original path. `RP_TEXTURE` is mutable global
pipeline state, not object-local or material-local state.

In `POLYMESH::render()`, the state is converted into render-with-material flags:

```cpp
renderpipeline->get_pipeline_state(RP_TEXTURE, &rwm_flags);
if (!rwm_flags)
    rwm_flags = RWM_DONT_TEXTURE;
else
    rwm_flags = 0;
```

When `RWM_DONT_TEXTURE` reaches `render_facegroup_with_material()`, PolyMesh deliberately skips
the diffuse bind and submits untextured geometry. A valid texture resource cannot compensate for
that instruction.

## Decisive A/B evidence

Narrow logging identified the live material by its authored texture name, `earth1_128.bmp`, then
recorded index count, material flags, `rwm_flags`, requested texture handle, setter result, and
readback handle.

### Working run before the correction

- Earth texture handle: `34dc0760`.
- Early draws initially reported `rwm=1` (`RWM_DONT_TEXTURE`) and skipped diffuse texturing.
- At tick `368478406`, `rwm` changed to `0` without the Earth asset changing.
- The texture setter then succeeded and the immediate readback matched `34dc0760`.
- The planet visibly gained its texture at that incidental state transition.

### Failed run before the correction

- Earth texture handle: `34e107c0`.
- Geometry was valid (`index_cnt=528`), excluding missing or empty planet geometry.
- `rwm=1` persisted throughout the capture.
- Every diffuse pass was skipped by `RWM_DONT_TEXTURE`.
- The planet remained white/grey.

The different numeric texture handles are expected process-local allocations and are not evidence
of different assets. The meaningful difference was exclusively the render-with-material flag.

### Runs after the correction

The user performed two consecutive Mission 1 runs and reported that the planet texture appeared
instantaneously in both. The fresh log contained two distinct Earth texture handles, proving two
separate object lifetimes/runs:

- First sequence: handle `346d2e50`, `rwm=00000000` from the first logged draw.
- Second sequence: handle `346dd2e0`, `rwm=00000000` from the first logged draw.
- Both sequences had valid index counts (`528`/`534`).
- Every recorded texture setter returned success.
- Every immediate readback matched the requested handle.
- No diffuse pass was skipped.

The full Debug rebuild completed successfully, rebuilding and verifying all 22 engine modules and
18 campaign modules. The deployed `Conquest.exe` matched the build output by SHA-256. The deployed
`PolyMesh.dll` was also verified by the build/deployment workflow and independently checked before
the test.

## Root cause and state writers

The immediate root cause was failure to establish ownership of the global `RP_TEXTURE` state at
the world-frame boundary. Known production writers include:

- `RenderBatcher.cpp`: disables it when `CQEFFECTS.bTextures == 0`.
- `SpaceShip.cpp`: disables it when textures are configured off.
- `SpaceShip.cpp`: explicitly enables it in a special/highlight rendering path.

The asymmetric writers explain why a later ship or highlight operation could apparently make the
planet texture “pop in.” The exact historical writer that first left the state disabled is less
important than the violated contract: world rendering assumed the global value was already
correct, while several object paths mutate it without reliable scoped restoration.

## Eliminated or separated hypotheses

The following were investigated and must not be resurrected as this incident's cause without new
contradictory evidence:

- **Mission corruption:** retail archive extraction and SHA-256 comparison found
  `T_Mission1.dmission`, `Map_Mission1_3.qmission`, and `Map_Mission1_3v2.qmission` identical to
  the source copies. Mission 1 correctly maps Tau Ceti Prime to `Planet Earth!!Earth 1`.
- **Missing Earth asset:** the authored texture reference resolved and decoded into nonuniform
  BGRA pixels.
- **Failed D3D texture creation/upload:** live Earth handles were nonzero, setters succeeded, and
  post-set readback matched whenever texturing was enabled.
- **Zero or culled geometry:** failed-run Earth draws had hundreds of submitted indices.
- **PolyMesh loading delay:** PolyMesh already had both valid geometry and a valid texture handle;
  it was explicitly told not to bind the latter.
- **Loose replacement textures:** deploying loose planet assets did not resolve the defect and is
  not an accepted dependency.
- **Mission-specific planet workaround:** unnecessary; the fault affected shared global state and
  could also affect ships.
- **ZBatcher stale texture-cache theory:** still a valid architectural hazard because direct PIPE
  users can bypass its cache, but it was not the differentiating cause in the captured A/B runs.

## Remaining independent planet defects

This incident resolves intermittent diffuse visibility only. Do not conflate it with:

- missing depth-surface creation/binding and failed far-side ring occlusion;
- cloud-shell transparency/blending fidelity;
- planet/cloud rotation or texture-coordinate animation;
- texture smearing observed at close range;
- broader cleanup of the mixed CFW and CQ2 rendering paths.

Those defects have separate acceptance criteria in `RENDER_PIPELINE_AUDIT.md`.

## Diagnostics retained

`Code/Libs/Src/RendComp/PolyMesh/polymesh.cpp` currently writes:

- `Debug/polymesh_earth_boundary.txt`: object/material ownership, module boundary, face count,
  texture handle, and render flags.
- `Debug/polymesh_earth_facegroup.txt`: submitted index count, `rwm_flags`, diffuse bind result,
  and immediate texture-stage readback.

The logging is narrowly gated by the resolved texture name and rate-limited. Retain it until the
planet is exercised across additional missions and a Final build. Remove it before declaring the
renderer milestone complete, after preserving representative logs or this report's evidence.

## Regression procedure

1. Perform a full rebuild; incremental builds are not acceptable for this repository.
2. Confirm all 22 engine DLLs and all 18 campaign DLLs are rebuilt, deployed, and hash-verified.
3. Clear old narrow diagnostic logs so run boundaries are unambiguous.
4. Cold-start Mission 1 and observe Tau Ceti Prime from its first visible frame.
5. Resign/re-enter or restart the mission for a second independent object lifetime.
6. Confirm both runs show the diffuse texture immediately.
7. Confirm each run's first Earth entry has `rwm=00000000`.
8. Confirm `requested == observed` and both setter/getter results indicate success.
9. Exercise the user texture toggle; configured-off texturing must remain off because the frame
   invariant derives from `CQEFFECTS.bTextures` rather than forcing true.
10. Repeat in a Final build and at least one non-Mission-1 planet scene before removing logs.

## Skirmish control case — 2026-09-06

The user initially observed that a planet in Skirmish appeared immediately and fully textured. In
the same Skirmish test, normally spawned ships were also correctly oriented. This proves that
planet geometry and texture data *can* initialize promptly in a fresh gameplay scene, but the
planet is not a fully clean renderer control (see the transition below).

The remaining delayed or intermittent Mission 1 planet appearance must therefore be investigated
through Mission 1's load/state sequence, object/archetype setup, or mission-specific render-state
initialization before attributing it to the global planet renderer. Preserve the instantaneous
Skirmish case when making any Mission 1 correction.

Visual reference:
`C:/Users/Joel/AppData/Local/Temp/codex-clipboard-c1575184-b217-4a74-9081-7755b9163f66.png`

The capture also shows ordinary ships on-plane and foreground geometry correctly crossing the
planet silhouette, supporting the normal-spawn and depth/occlusion control cases.

### Skirmish material transition

During the same running Skirmish session, Zeta Prime's surface abruptly changed to a visibly
different appearance. The later capture has a hard vertical division through the planet, with
substantially different shading/texture appearance on the adjacent halves:
`C:/Users/Joel/AppData/Local/Temp/codex-clipboard-f203dda1-5315-4273-8b67-34ba1619d940.png`.

This is not accepted as normal planet rotation. Treat it as evidence of unstable per-face-group or
per-material state, a partially applied animated texture/frame update, or state contamination at
the manual render boundary. Correlate the transition with the retained PolyMesh material logs and
with selection, camera, construction, and object-arrival events before changing code.

The user identified the replacement appearance as resembling **Tau Ceti Prime's** texture rather
than Zeta Prime's original texture. Later in the same uninterrupted Skirmish session, Zeta changed
again to a different appearance that the user identified as correct:
`C:/Users/Joel/AppData/Local/Temp/codex-clipboard-6906415e-80f9-4e2e-9dc3-33ac4757e72b.png`.

This later transition prevents us from concluding that Tau's texture was permanently assigned.
The leading possibilities are now an incorrectly sequenced/resolved animated texture frame, an
invalid frame in Zeta's authored reference, or cross-object cached material contamination. The hard
seam in the intermediate capture suggests that a transition may reach only some face groups or
that adjacent groups resolve different frames during a draw.

Required comparison: log planet object/archetype identity, material pointer/index, texture-reference
identity, resolved frame handle, and bound texture for every planet face group. Run Zeta alone, Tau
alone, and both across sequential mission/skirmish lifetimes to determine whether the wrong handle
is animation-time, process-lifetime, archetype-cache, or draw-order dependent. Also inventory the
complete authored frame sequence for Zeta before labeling any individual appearance incorrect.

## Reusable engineering lesson

In this renderer, delayed visual appearance does not imply delayed resource loading. Before
instrumenting asynchronous loaders, inspect the complete draw contract at the final material
boundary: geometry count, render-with-material flags, desired resource, setter result, and actual
state readback. For process-global render state, establish an authoritative owner at a stable
frame boundary and treat per-object mutations as scoped operations that must restore or invalidate
shared caches.
