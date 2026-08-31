# Retail-Parity Test Matrix

This is the acceptance ledger for Priority 1: restore behavior before modernization. Compare the
Debug and Final builds with the retail reference and record evidence, not impressions.

## Automated baseline

| Check | Debug | Final |
|---|---:|---:|
| Full `Win32` solution rebuild | PASS (2026-08-30) | PASS (2026-08-30) |
| Required EXE/DLL outputs present | PASS | PASS |
| Deployed binaries match build SHA-256 | PASS | PASS |
| Isolated deployment directory | PASS | PASS |

Warnings are not a passing fidelity signal. In particular, the linker still reports `/FORCE`,
runtime-library conflicts, and shared intermediate directories.

## Runtime smoke tests

| Area | Test | Debug | Final | Retail comparison / evidence |
|---|---|---:|---:|---|
| Startup | Intro/menu reaches stable idle | TODO | TODO | |
| Mission | Start a known retail mission | TODO | TODO | |
| Rendering | Player ships use correct authored orientation | FAIL (known) | TODO | Ships appear ~90 degrees nose-down |
| Visibility | Enemy units become visible under retail conditions | FAIL (known) | TODO | Invisible-enemy issue remains open |
| Combat | Weapons, impacts, damage and destruction complete | TODO | TODO | |
| Explosion | Mesh shatter completes without access violation | FAIL (known) | FAIL (known) | `SplitInstance` / `GetBoundingBox` path |
| UI | Selection, orders, build menus and HUD behave | TODO | TODO | |
| Save/load | Save and reload the same mission state | TODO | TODO | |
| Audio/video | Music, SFX and cinematics use retail assets | TODO | TODO | |
| Campaign | Complete representative mission from each campaign | TODO | TODO | |

For crashes, retain the build configuration, save/mission, exact reproduction steps, debugger
stack, and—when practical—a TTD trace. For visual differences, capture the same unit, camera,
heading, zoom and mission in retail and rebuilt versions.

## Immediate investigation order

1. Inspect an authored retail ship mesh to establish its forward/up axes.
2. A/B test the current CQ2 `instanceMesh` path against the surviving retail
   `ENGINE->render_instance(instanceIndex)` path with a reversible change.
3. Resolve ship orientation from evidence; do not bake a corrective rotation until the mesh-axis
   convention is known.
4. Re-test visibility after orientation/render-path selection because both symptoms may share the
   renderer boundary.
5. Diagnose the explosion shatter access violation independently.

### Ship-axis evidence and A/B switch (2026-08-30)

The shipped CMP files are UTF 1.01 containers. Direct inspection of representative Terran,
Celareon, and Mantis root vertex lists shows that most ships are authored with their longest hull
axis on local Z. Examples include `Ttakai.cmp` (Z 225.3 vs. Y 30.5), `TSteele.cmp` (Z 294.7 vs.
Y 45.8), `ssLegionare.cmp` (Z 1142.7 vs. Y 398.0), and `Mthripid.cmp` (Z 508.4 vs. Y 262.0).
This is consistent with a missing authored-Z-forward to game-planar-forward conversion.

`SpaceShip::renderSpaceShip` now honors the existing `CQEFFECTS.bFastRender` option:

- Fast Render on: sequel-era `IMeshInstance::Render` path.
- Fast Render off: legacy `ENGINE->render_instance` path.

This is an A/B diagnostic, not yet the final orientation fix. Compare the same ship and camera in
both modes before changing transforms or mesh data.

Result: Fast Render on and off produced the same approximately 90-degree nose-down orientation
without a crash. The final renderer branch is therefore not the source of the rotation. The next
test restores the dormant `transform.rotate_about_i(90*MUL_DEG_TO_RAD)` conversion in
`SpaceShip::initSpaceShip`, matching the active conversion in `Platform::initPlatform` and the
measured Z-forward authored ship meshes.

Result: restoring the initialization conversion produced no visible change because subsequent
mission/spawn placement replaces the complete gameplay transform. The next controlled test keeps
Fast Render off and passes the same +90-degree correction through the legacy engine's intended
`RF_TRANSFORM_LOCAL` render modifier, leaving gameplay physics unchanged.

Result: the local +90-degree render correction made ships level, proving the missing model-axis
conversion, but mapped the authored bow toward gameplay aft. Preserve the corrected local-up axis
and reverse forward with an additional 180-degree rotation around model-local Y. This also reverses
local right, preserving a proper right-handed rotation rather than mirroring the mesh.
