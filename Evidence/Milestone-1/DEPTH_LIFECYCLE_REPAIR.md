# D3D9 Depth Lifecycle Repair

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

Date: 2026-09-06

## Finding

The prior D3D9 device path selected a depth format and advertised
`D3DRP_F_DEPTH_CREATED`, but created the device with automatic depth disabled and
never manually created or attached a depth surface. Render code could therefore
enable and clear depth while the device had no depth attachment. This is a direct,
source-confirmed explanation for far-side planet ring sections remaining visible.

The released source baseline (`a9872e3`) used automatic depth creation during both
device creation and reset. That establishes automatic depth as the lowest-risk
restoration path; it is not a speculative CQ2 feature.

## Implemented repair

`Direct3D_pipe.cpp` now:

1. clears the legacy buffer/depth/stencil flags before creating the device;
2. enables automatic depth using the already-probed D24S8-to-D16 fallback;
3. obtains the attached surface with `GetDepthStencilSurface()` and verifies its
   description before advertising depth capability;
4. advertises stencil only when the attached format is actually D24S8; and
5. sends the configured `RP_CLEAR_DEPTH` value to `Clear()` instead of always
   clearing to `1.0`.

Device creation fails rather than continuing with a false depth-capability flag if
the attachment cannot be verified. The existing `debug/d3d_diag.txt` receives the
attached format, dimensions, and multisample type.

## Build and deployment proof

- Debug full rebuild: PASS; 22 engine modules and 18 campaign modules rebuilt and
  deployed.
- Final full rebuild: PASS; 22 engine modules and 18 campaign modules rebuilt and
  deployed.
- Debug runtime pipe SHA-256:
  `26DE512AD45642705FA95D671CD553EF8F92A76F1FDB1B5AEC2C68E3C5839246`.
- Final runtime pipe SHA-256:
  `5F88463353104E27C0FF722F1AD5A6EB5C392D5C1D587B577C4885D89270D5C6`.
- Each deployed `DLL/D3DRenderPipe.dll` matches its configuration output exactly.
- The same-named DLL in each installation root is an older, unused file. Both
  `Conquest.ini` files explicitly load `DLL/D3DRenderPipe.dll`; root-file hashes
  must not be used as deployment proof.

## Runtime acceptance test

Status: **pending**.

The first runtime test exposed a separate render-order defect: when `bShowGrids`
was enabled, `ObjectList::renderGridLines()` drew a full-plane grid before world
objects while inheriting depth writes. With a real depth surface, later objects on
the gameplay plane failed `D3DCMP_LESS`; the background and UI remained visible but
Tau Ceti Prime and Blackwell disappeared from the main playfield. The tactical-map
icons remained present, proving mission object creation was intact. The grid path
now disables depth test/write for its draw and restores both prior states afterward.
This follow-up requires the same runtime acceptance test below.

1. Cold-start Debug and enter Mission 1 without changing focus.
2. Confirm Tau Ceti Prime is textured immediately.
3. Inspect the ring from the known close angle; far-side sections must disappear
   behind the planet while near-side sections remain visible.
4. Check ships and the fabricator crossing the planetary limb for correct depth.
5. Check cloud transparency, selection markers, projectile effects, construction
   preview, and UI overlays for ordering regressions.
6. Reload/restart the mission twice, then repeat once after Alt-Tab.
7. Retain `debug/d3d_diag.txt`. It must contain `depth attached`; record its format
   and dimensions.
8. Repeat the smoke path in Final only after Debug passes.

## Deliberately deferred risk

`destroy_buffers()` still performs a Reset rather than a true teardown and uses a
separately assembled presentation structure. Reworking it touches default-pool
texture/buffer lifetime and is not safely bundled with the attachment repair. It
requires a dedicated device-loss/reset trace before alteration. Runtime success of
this patch must not be described as complete reset-lifecycle remediation.
