# Construction and Ship Findings

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

Date: 2026-09-06

## Construction preview

The laser-turret ghost path is source-confirmed and is not missing from the command
or UI layer. `UnbornMeshList::Render()` reaches `RenderMeshAt()` for valid and invalid
placement, requesting green or red alpha-128 geometry. `RenderMeshAt()` realizes the
mesh, colors its face groups, transforms the Engine instance, and submits it through
the manual `TreeRender` path.

Disposition: do not add a substitute marker. Retest after the global texture-state
and depth repairs. If still absent, trace this translucent manual-render boundary,
including alpha blend, texture, depth write/test, transform, and ZBatcher cache state.

## Fabricator and platform assembly

Assembly scripting is also source-confirmed:

- `TFabricator::initFabricator()` creates `Sc_build` and `Sc_construct` script
  instances.
- `TFabricator::FabStartBuild()` invokes the concrete `LocalStartBuild()` path.
- Build-platform and support-platform paths call `fabStartAnims()` at construction
  start; completion reverses/stops the animation.

Disposition: the missing/floating/untextured geometry is not evidence of absent
animation calls. Retest first. If reproduced, log script instance, parent instance,
compound-child local/world transform, material texture handle, and render-state
boundary in the same frame. HQ animation parity remains unconfirmed and must be
checked against retail before adding behavior.

### Runtime scope confirmation — 2026-09-06

The user reports that **no structure displays its build animation**. The missing behavior is
therefore not isolated to the Fabricator or HQ and should be treated as a shared structure-build
pipeline failure.

The absence was subsequently reproduced in Skirmish as well as Mission 1. This rules out Mission 1
scripting as the sole cause and scopes the defect to a shared construction animation/update/render
path. Retail footage or a retail executable comparison must still establish which structure types
have authored animation and their intended sequences before implementation changes are accepted.

This is distinct from the systemic vertical orientation of ships produced by the Fabricator,
Shipyard, and Refinery. The two defects may meet at a common construction/render boundary, but
must be diagnosed and accepted independently.

Disposition: trace the common platform/structure creation path from build-state selection through
script start, animation-instance update, compound child transforms, and final render submission.
Compare the result against retail before restoring or adding any behavior. Acceptance requires
representative structures from each available producer/faction to show their authored build
sequence without detached or untextured geometry.

### Shared build-effect handoff restored for validation — 2026-09-07

The upstream producer side and downstream effect implementations are intact. Every Fabricator
creates its race-specific build-effect object in `TFabricator::FabStartBuild()`, and the Terran,
Mantis, and Solarian effect classes still implement mesh setup, percentage/rate updates,
Fabricator attachment, update, render, and completion.

The shared `ObjectBuild` boundary between those two halves was disabled in three places:

- initial construction (`Build`),
- save/reload reattachment (`AddFabToBuild`), and
- the fallback dismantle path (`BeginDismantle`).

Those three blocks now run again. This is a localized runtime-validation change: it does not alter
normal ship spawning, the global ship model-axis correction, producer-specific transforms, or the
effect implementations. The disabled blocks also exist in the repository's initial source import,
so their origin cannot be attributed to a recent edit and the restoration is not considered proven
until runtime acceptance.

Acceptance requires the effect to remain attached and textured, the constructed ship to be on-plane
from its first visible frame, and successful completion for Fabricator, Shipyard, and Refinery
production. Save/reload during an active build and dismantling must also be checked. A crash,
detached effect, or incorrect effect sequence is evidence to revert this handoff rather than add a
second compensating transform.

## Jump-in orientation

`ObjectWarp::startWarpIn()` constructs an on-plane gameplay transform explicitly:
local J is `(0,0,1)` and I/K are derived from the horizontal stop heading. The
nose-down image therefore does not originate in the initial gameplay transform.
`SpaceShip::renderSpaceShip()` applies the established +90-degree local-X and
180-degree local-Y model-axis correction, including while warp scaling is active.

Disposition: no speculative transform change. Capture the gameplay transform,
warp-local transform, final render modifier, and first three rendered frames. Compare
normal ships and Blackwell against retail jump-in footage.

## Systemic constructed-ship orientation — runtime confirmation 2026-09-06

The user has now observed vertically oriented ships being produced by all three relevant
construction sources: the Fabricator, Shipyard, and Refinery. This rules out a
Fabricator-specific animation or attachment defect as the primary cause of the constructed-ship
orientation error.

The shared symptom should be investigated with the jump-in inversion because both are boundaries
where a ship model first becomes visible and may bypass, duplicate, or prematurely apply the
normal ship model-axis correction. Construction animation/geometry visibility remains a separate
issue: correcting orientation must not be treated as proof that the Fabricator assembly animation
or other authored build effects are functioning.

Disposition: trace the common ship build-preview/new-object transform and compare its first visible
frame with the normal `SpaceShip::renderSpaceShip()` transform. Validate Fabricator, Shipyard, and
Refinery production after any shared correction, then independently validate assembly effects.

### Skirmish control case — 2026-09-06

Ships spawned normally in Skirmish appear correctly oriented. This disproves a universal ship-model
or base spawn-orientation defect. The vertical construction symptom is confined to the special
construction presentation/newly-built transition, while the observed Mission 1 inversion remains
confined to its reinforcement/jump presentation unless further testing shows otherwise.

The user subsequently confirmed that ships also build vertically in Skirmish. The construction
orientation defect is therefore global across game modes, not Mission-1-specific. Its scope is the
shared construction presentation path used by multiple producers; completed/ordinary Skirmish
spawns remain the correct control.

Disposition: use the correct Skirmish spawn/render path as the local control when comparing the
construction and warp paths. Avoid changing the global ship archetype or normal model transform,
which would risk breaking the already-correct Skirmish case.

## Blackwell rear fire

The deployed archetype database contains a Blackwell-specific turret reference,
`TUR!!T_Corvette Blackwell`, used by the Blackwell character archetype. This confirms
that Blackwell has authored turret behavior distinct from an ordinary fixed forward
gun. Rear fire is therefore plausible and must not be corrected merely because it
differs from the other ships.

Disposition: confirm turret hardpoint position and authored traverse/arc against
retail gameplay. Accept rear fire if the muzzle remains attached to the turret and
the turret tracks the target; fix only detached muzzle transforms or a demonstrably
incorrect arc.

## Focused runtime matrix

| Test | Current evidence | Acceptance |
|---|---|---|
| Laser-turret placement ghost | Source path present; runtime absent before fixes | Translucent ghost follows cursor and changes valid/invalid color |
| Fabricator construction | Scripts/calls present; runtime geometry floated/untextured | Geometry remains attached, textured, and animates through build |
| Structure build animations | Absent for every observed structure in Mission 1 and Skirmish | Retail-confirmed authored build sequences display across representative structure types |
| Constructed ship orientation | Vertical from Fabricator, Shipyard, and Refinery; reproduced in Mission 1 and Skirmish | Every producer presents ships on-plane from the first visible frame |
| Ordinary Skirmish spawn | Correctly oriented | Preserve as control; no global axis correction |
| HQ construction | Retail parity unknown | Observe retail first; reproduce only confirmed behavior |
| Normal ship jump-in | Gameplay transform on-plane; nose-down observed | First visible frame is on-plane; normal turn afterward |
| Blackwell jump-in | Same spawn path; left turn afterward observed | No flip/nose-down; authored left turn retained |
| Blackwell turret | Unique turret reference confirmed | Attached muzzle and retail-consistent tracking/arc |

## Runtime construction result — 2026-09-07

The restored construction handoff is now confirmed across the Fabricator, Headquarters,
Refinery, laser turret, and harvester.  The effect geometry appears, but its materials are
corrupt or untextured; the completed Refinery can subsequently inherit the same corruption.
This separates the problem into a passing lifecycle handoff and a failing shared-material
consumer.

The application-local `MeshRender` still contained CQ2 material promotion code which the
MeshManager implementation had already removed.  That code mutates the shared mesh archetype
and discards its authored legacy `Material` list after promotion.  CFW does not ship the CQ2
shader requirements, so the manual construction/ghost/cargo consumers must retain the legacy
route.  The promotion is now disabled and legacy texture handles are refreshed without changing
material ownership.  Runtime validation remains required.
