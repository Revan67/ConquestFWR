# Prominent Notice of Modifications

This repository is a modified version of the *Conquest: Frontier Wars* Program. The entire Program
Derivative, as a whole, and every recipient are subject to the exact terms of the unmodified
[Public License for Conquest: Frontier Wars Program and Source Code](Conquest%20Source%20License.txt),
copyright Fever Pitch Studios, December 2013.

This notice records modifications; it does not amend or add to the license. Git history contains the
detailed file-level record.

| Relevant date | Author | Modification notice |
|---|---|---|
| 2022-12-25 | Revan67 | Imported the source release, converted initial projects to Visual Studio 2022, and established the public repository. |
| 2026-03-25 through 2026-05-26 | Revan67 | Ported the Windows and DirectX-era build to current Visual Studio tooling; added compatibility shims, reproducible data-generation support, diagnostics, and initial runtime restoration fixes. |
| 2026-08-20 through 2026-08-30 | Revan67 | Restored retail ABI and data layouts; repaired renderer, movement, combat, explosion, construction, and mission behavior; rebuilt engine components; added campaign projects, verified build/deploy tooling, crash analysis, tests, and project documentation. |
| 2026-08-30 | Revan67 | Added the legal-compliance policy, distribution checklist, interoperability-research boundaries, third-party provenance warnings, and automatic deployment of required legal notices. |
| 2026-09-04 | Revan67 | Corrected resource-bar alignment at scaled resolutions and recorded the change in restoration evidence. |
| 2026-09-27 | Revan67 | Consolidated restoration knowledge into `PROJECT_BIBLE.md`, archived historical notes, and updated documentation links and project guidance. |
| 2026-09-27 | Revan67 | Stabilized the Debug and Final full-build workflow, configuration-specific outputs, build manifests, deployment verification, and project build paths. |
| 2026-09-27 | Revan67 | Audited source-license, Conquest manual, and Bink provenance; clarified the legal checklist and excluded the manual and three Bink DLLs from current Git tracking while retaining local copies. |
| 2026-09-27 | Revan67 / ConquestFWR | Isolated previously runtime-accepted retail-restoration corrections for projectile direction, harvester packet and attachment coordinates, toolbar actions, world-frame texture state, sector visibility, construction effects, ship glows and marker, and D3D9 depth attachment. This does not establish universal retail parity. |
| 2026-09-27 | Revan67 / ConquestFWR | Removed completed, previously uncommitted renderer-route measurement hooks and default launcher opt-ins from the live working tree; retained the historical captures and verification tools. No renderer behavior change is claimed. |
| 2026-10-12 | Revan67 / ConquestFWR | Isolated the DocuView parser/schema compatibility candidate for mission/save conversion, with measured layout guards; synthetic conversion probes pass, but retail save compatibility remains unverified. |
| 2026-10-12 | Revan67 / ConquestFWR | Removed completed one-off toolbar and texture-construction logging from normal execution; retained separate active harvester diagnostics and raw investigation evidence. No gameplay or renderer parity is claimed. |
| 2026-10-12 | Revan67 / ConquestFWR | Recorded Phase A worktree classification, recovery-checkpoint identity, candidate and diagnostic isolation, and remaining baseline gate in the canonical project Bible. No new restoration behavior is claimed. |
| 2026-10-12 | Revan67 / ConquestFWR | Isolated the HQ placement-preview instance/ownership repair from unaccepted renderer experiments. Debug and Final full builds passed; repeated placement, cancellation, reload, and shutdown acceptance remains pending. |
| 2026-10-12 | Revan67 / ConquestFWR | Reconciled the Phase A source baseline by restoring unaccepted renderer, D3D9 texture, and speculative warp experiments from the live checkout; retained their recovery archive, disabled harvesting diagnostics in ordinary builds, recorded fresh isolated Debug/Final build provenance, and checkpointed the source baseline. No additional gameplay fix or runtime parity is claimed. |

Future material modifications must add a dated and attributed entry here before distribution.
