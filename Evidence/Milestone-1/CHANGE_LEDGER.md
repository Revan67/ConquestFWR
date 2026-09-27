# Milestone 1 Change Ledger

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

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
| Harvester state machine | `HarvestShip.cpp` AI/network hunks | Contradicted/mixed | Later testing completely broke down despite nearby resources. The decisive trace showed valid targets and active movement while position remained unchanged. Retain the packet-size correction and diagnostics; remove or separately justify speculative retry/dock/target behavior before diagnosing shared movement. |
| Toolbar resources | `Menu_Toolbar.cpp`, deployed `gendata.db` | Runtime verified at 1600x1200 | Binary comparison proved the deployed database had altered all three races' common-control coordinates. Restoring the retail-identical database (SHA-256 `ADFA34D58F22163665EC0E9973A7D8E69A82166F265B22416EA2FF74E89A01C3`) and toolbar-relative initialization restored exact horizontal placement. At 1600x1200 the unadjusted font was slightly low, while `-2` ideal Y was too high; `-1` ideal Y plus one ideal unit of centered control height produced the user-accepted placement without changing the shared font. `shipclass` and 640x480 remain untouched. |
| Planet render switch | `Planetoid.cpp` | Contradicted | MeshManager-only rendering produced a black planet. Rolled back in the first Milestone 1 batch to the visible engine-render baseline. |
| Planet diffuse state ownership | `ObjList.cpp` | Runtime verified | Working/failed A/B traces proved `RP_TEXTURE` leaked disabled and caused PolyMesh to set `RWM_DONT_TEXTURE` despite valid Earth geometry and texture resources. The configured texture state is now established at the start of every world render frame. Two consecutive runs rendered immediately and logged `rwm=0` from the first Earth draw. See `PLANET_TEXTURE_STATE_LEAK.md`. |
| Loose planet assets | `Build-Conquest.ps1`, three untracked texture files | Contradicted/unproven | Deploying loose files did not fix the planet. Removed the deployment requirement. Files remain untracked and hash-recorded; they are not accepted inputs. |
| Mesh renderer sources | both `MeshRender.cpp` paths | No working-content change | Git status was stale after rebuild; object hashes exactly match `HEAD` and the Milestone 0 checkpoint. Do not attribute current behavior to an uncommitted renderer edit. |
| Runtime libraries | tracked DLL/LIB/PDB files | Generated | Rebuild products, not source fixes. Exclude from behavioral review and future source commits. |
| D3D9 depth attachment | `Direct3D_pipe.cpp` | Runtime verified, reset caveat | Far-side ring and planetary-limb occlusion passed. Creation is accepted; Alt-Tab/device-reset behavior remains unverified and uses a divergent reset path. See `DEPTH_LIFECYCLE_REPAIR.md`. |
| Mission grid depth boundary | `ObjList.cpp` | Runtime verified | Disabling depth test/write for the grid and restoring both prior states returned the planet, ships, and system-map icons. Preserve. |
| Construction visuals | `TObjBuild.h`, race build-effect sources | Lifecycle verified; materials failed | Fabricator, HQ, Refinery, turret, and harvester effects now instantiate. Their geometry/material presentation is corrupt or untextured; do not mark construction visuals complete. See `CONSTRUCTION_AND_SHIP_FINDINGS.md`. |
| Top-right toolbar actions | `Menu_Toolbar.cpp`, shipped `gendata.db`, generated `Hotkeys.h` | Runtime verified | Options and Mission Objectives both open correctly; resource alignment remains correct. Preserve the name-scoped compatibility binding. |
| Jump-in and Blackwell turret | `TObjWarp.h`, `SpaceShip.cpp`, `gametypes.db` | Investigated; no speculative change | Spawn gameplay transform is planar; nose-down is downstream in render/warp composition. Blackwell has a unique authored turret reference, so rear fire may be intentional. |
| Handoff/test notes | `Evidence/Historical/Root/HANDOFF.md`, `Evidence/Historical/Root/RETAIL_TEST_MATRIX.md` | Mixed evidence | Retain useful observations, but correct claims that conflict with actual source or runtime results before committing. |

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

### Planet diffuse runtime result — 2026-09-06

PASS for intermittent diffuse visibility. A failed pre-fix run retained `RWM_DONT_TEXTURE` while
a working pre-fix run began texturing only when that flag changed. After establishing
`RP_TEXTURE` from `CQEFFECTS.bTextures` at the start of `ObjectList::Render()`, two consecutive
Mission 1 runs displayed the texture immediately. Both run sequences logged valid geometry,
`rwm=0` from the first Earth draw, successful binds, and matching readback handles. Depth/ring
occlusion, cloud fidelity, and animation remain separate open tests.

### Debug rebuild and deployment — 2026-09-07

The clean Debug build completed after the construction-handoff and toolbar compatibility changes.
The deployed `Conquest.exe` is byte-identical to the build output: 4,706,816 bytes, SHA-256
`3AC2AA40449DFD3CDF594D42DB63CD3D7BEB40EADCE76F159281DD1E38773897`.
The four generated core DLLs are also byte-identical to their deployed copies:

| DLL | Bytes | SHA-256 |
|---|---:|---|
| `Globals.dll` | 1,299,456 | `DBB8D891D3A11C09C094C927644E2CF9129E65C8F80CBCE61B91708D00868FD0` |
| `Mission.dll` | 1,511,936 | `C7FB1A1692C8FB693BA5F0D5203D2F43E4144CA5E3F273A73D1A5E6AA41C679F` |
| `Trim.dll` | 1,654,272 | `9E89C0ECFBB21C45C7360110D703D8B7AD1BB39B8F1F09C16B3CD44C6139AA51` |
| `ZBatcher.dll` | 431,616 | `C9F625406315BB67F67AE1B20128A226D4568B4D6BD49458477653154C6B1E41` |

All 22 engine modules and all 18 campaign modules were included by the full build/deployment run.
This proves artifact identity only; runtime behavior remains pending.

### Manual-render material containment and movement diagnostics — 2026-09-07

Runtime proved that construction lifecycle restoration worked but exposed corrupt materials and
shared Refinery archetype contamination.  `Code/App/Src/MeshRender.cpp` now matches the established
CFW legacy-material policy used by MeshManager: it no longer promotes and destructively replaces
the shared legacy material list with the unshipped CQ2 shader path, and it refreshes realized
legacy texture handles before manual rendering.

The prior harvester trace proved that ore selection was successful while `HAR_MOVING_TO_NUGGET`
remained motionless.  `Code/App/Src/HarvestShip.cpp` now records auto-movement state, grid goal,
target position, and target distance so the next failure can be assigned to command goal, path,
or movement execution without another speculative AI change.

A complete Debug rebuild covered all 22 engine modules, the application, and all 18 campaign
modules.  Deployed hashes match the build outputs, including `Conquest.exe`
`B50457EB71227FB51CECE59E456336BBE00176391EC664FBD79317E905CD459C` and `MeshManager.dll`
`54FFE35950D105097D4A51C524AFF777259620B69503AF306C6759C4B50CD688`.
