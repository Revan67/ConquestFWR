# Milestone 1 Change Ledger

This ledger classifies the mixed pre-remediation working tree. “Verified” means directly observed
in the current runtime, not merely compiled or described in an archived note.

| Area | Paths | Classification | Evidence and disposition |
|---|---|---|---|
| Test resolution | `System.cpp` | Runtime verified | 1600×1200 works and materially improves testing. Preserve. |
| Sector navigation | `Sector.cpp` | Runtime verified | Sector map became visible and functional. Preserve pending retail-layout comparison. |
| Ship engine glows | `TObjGlow.h` | Runtime verified | User confirmed engine lights are correctly attached. Preserve. |
| Target/talking ring | `SpaceShip.cpp` | Runtime verified | Oscillation stopped and ring remains static. Preserve; archived “continuous rotation” wording was inaccurate. |
| Projectile direction | `Projectile.cpp`, `AnmBolt.cpp`, `PlasmaBolt.cpp`, `SpecialBolt.cpp`, `AutoCannon.cpp` | Runtime verified with scope caveat | Normal ships orient and fire toward targets. Blackwell’s rear fire may be a valid turret arc and requires retail hardpoint confirmation. Preserve as one testable group. |
| Harvester attachments | `HarvestShip.cpp` render hunks | Partially runtime verified | Mining beam origin is corrected and storage visuals improved. Preserve, but cargo-panel attachment still needs an explicit acceptance check. |
| Harvester state machine | `HarvestShip.cpp` AI/network hunks | Partially verified | Full-load return/unload works and multi-node selection improved, but next-node selection remains intermittent. Preserve as an isolated subsystem for targeted tests; do not mark complete. |
| Toolbar resources | `Menu_Toolbar.cpp`, deployed `gendata.db` | Runtime verified at 1600x1200 | Binary comparison proved the deployed database had altered all three races' common-control coordinates. Restoring the retail-identical database (SHA-256 `ADFA34D58F22163665EC0E9973A7D8E69A82166F265B22416EA2FF74E89A01C3`) and toolbar-relative initialization restored exact horizontal placement. At 1600x1200 the unadjusted font was slightly low, while `-2` ideal Y was too high; `-1` ideal Y plus one ideal unit of centered control height produced the user-accepted placement without changing the shared font. `shipclass` and 640x480 remain untouched. |
| Planet render switch | `Planetoid.cpp` | Contradicted | MeshManager-only rendering produced a black planet. Rolled back in the first Milestone 1 batch to the last visible engine-render baseline; this is containment, not the final fix. |
| Loose planet assets | `Build-Conquest.ps1`, three untracked texture files | Contradicted/unproven | Deploying loose files did not fix the planet. Removed the deployment requirement. Files remain untracked and hash-recorded; they are not accepted inputs. |
| Mesh renderer sources | both `MeshRender.cpp` paths | No working-content change | Git status was stale after rebuild; object hashes exactly match `HEAD` and the Milestone 0 checkpoint. Do not attribute current behavior to an uncommitted renderer edit. |
| Runtime libraries | tracked DLL/LIB/PDB files | Generated | Rebuild products, not source fixes. Exclude from behavioral review and future source commits. |
| Handoff/test notes | `HANDOFF.md`, `RETAIL_TEST_MATRIX.md` | Mixed evidence | Retain useful observations, but correct claims that conflict with actual source or runtime results before committing. |

## First controlled remediation batch

The first batch contains only containment of the failed planet experiment:

1. Stop requiring/deploying the three loose planet textures.
2. Restore the last visible `ENGINE->render_instance` planet path with MeshManager fallback.
3. Leave all runtime-verified gameplay fixes untouched.

Expected result: Tau Ceti Prime should return from solid black to the earlier visible gray/white
baseline. Texture fidelity, compound child animation, and far-side ring occlusion are explicitly
not claimed fixed by this batch.

## Build and deployment verification

- Debug no-deploy rebuild: PASS; five core binaries and 18 campaign DLLs produced.
- Final no-deploy rebuild: PASS; five core binaries and 18 campaign DLLs produced.
- Debug controlled deployment: PASS; all five deployed core hashes match their Debug outputs.
- Debug campaign deployment: PASS; all 18 expected DLLs are present.
- Experimental override check: PASS; zero candidate planet overrides remain in the Debug
  installation. The three files are preserved under `quarantined-runtime-assets/Debug` with the
  same hashes captured in Milestone 0.
- Static diff validation: PASS (`git diff --check`).

Runtime acceptance remains pending because the planet must be observed in Mission 1. This batch
passes only if it restores the visible baseline without regressing boot, mission load, ship
orientation, projectiles, sector navigation, or construction.

### Runtime result — 2026-09-03

PASS. Mission 1 played normally through the fabricator arrival and Tau Ceti Prime returned to the
known gray/white visible baseline. This validates removal of the failed experiment and confirms no
early-mission regression in this batch. It does **not** validate retail planet texturing, compound
surface/cloud animation, or far-side ring occlusion; those remain open and will be investigated
from retail assets and renderer behavior rather than additional loose-file guesses.

### Toolbar runtime result — 2026-09-04

PASS at 1600x1200. All four resource values are visible, horizontally aligned to their retail
toolbar fields, and vertically centered to the user's accepted position. The font remains
unchanged. The deployed five core binaries match the Debug build outputs, `Trim.dll` contains the
toolbar change, the retail `gendata.db` hash is preserved, and all 18 campaign modules were built
and deployed.
