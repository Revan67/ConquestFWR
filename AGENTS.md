# ConquestFWR — Codex Project Rules

## Project mission

ConquestFWR exists to restore the released retail version of **Conquest: Frontier Wars** from the surviving source tree.

The surviving source is an **unshipped Conquest 2 / Vyrium Uprising development snapshot**. It is implementation material, not the behavioral specification.

The restoration branch has one primary rule:

**Retail first. Modernize later.**

Do not modernize, redesign, simplify, reinterpret, or “improve” behavior while retail restoration is incomplete. Modernization belongs on a separate branch after retail parity has been established.

---

# Canonical project documentation

There are only two living project-control documents:

1. `AGENTS.md` — rules for working on the repository.
2. `PROJECT_BIBLE.md` — canonical project knowledge, current state, accepted findings, active investigations, and restoration plan.

`PROJECT_BIBLE.md` is the single narrative source of truth.

Do **not** create new narrative Markdown files for:

- investigations
- findings
- audits
- handoffs
- progress reports
- milestone reports
- renderer reports
- crash reports
- build reports
- implementation checkpoints
- session notes
- temporary hypotheses
- “latest status” documents

unless the user explicitly authorizes a separate document.

Update `PROJECT_BIBLE.md` instead.

Machine-generated evidence, raw debugger material, dumps, traces, extracted schemas, manifests, disassembly output, Ghidra artifacts, test logs, screenshots, and similar primary evidence are not subject to this restriction.

Those are evidence, not project documentation.

---

# Evidence hierarchy

Use evidence in this order:

1. Directly observed retail behavior.
2. Retail executable/DLL disassembly or decompilation.
3. Retail TTD/debugger traces.
4. Retail parser resources and shipped database contents.
5. Retail models, animations, effects, archives, and other shipped assets.
6. Retail campaign DLL ABI and behavior.
7. Contemporary retail documentation.
8. Direct observation of the reconstructed build.
9. Current source/build graph.
10. Released CQ2 source comments and implementation.
11. Technical inference.

The CQ2 source does not override demonstrated retail behavior.

Archived Claude/Codex notes are leads only. They are never evidence by themselves.

A successful compilation is not evidence of runtime correctness.

A successful Debug test is not automatically evidence of Final retail parity.

A matching version string is not evidence of binary identity.

---

# Evidence labels

Keep **status** separate from **evidence type**.

Use these project statuses:

- `VERIFIED` — accepted project truth supported by sufficient evidence.
- `CANDIDATE` — implemented or strongly supported but still awaiting required acceptance.
- `OPEN` — unresolved.
- `CONTRADICTED` — evidence demonstrates the current implementation or prior claim is wrong.
- `SUPERSEDED` — historical conclusion replaced by stronger later evidence.

When useful, identify the evidence as:

- Retail behavior
- Retail binary
- Retail data/schema
- Runtime trace
- Source-confirmed
- Build/deployment proof
- Inference

Do not use vague states such as:

- probably fixed
- seems solved
- likely correct
- should work
- mostly fixed

State exactly what has and has not been demonstrated.

---

# Before changing behavior

For every behavioral correction, determine:

1. What is currently wrong?
2. What did retail do?
3. What evidence establishes retail behavior?
4. What code/data path currently produces the divergence?
5. What is the first confirmed point where reconstructed behavior diverges?
6. What is the smallest correction at that boundary?
7. What exact test will accept or reject the correction?
8. What rollback boundary exists?

If retail behavior is unknown, investigate before modifying behavior.

Do not patch from visual intuition alone.

---

# Fix causes, not symptoms

Do not add arbitrary:

- rotations
- positional offsets
- timing delays
- asset replacements
- texture substitutions
- render-state defaults
- AI retries
- object-specific exceptions
- special-case IDs

merely because they make one test look correct.

Prefer one root cause that explains multiple correlated symptoms.

Examples:

If projectile direction and engine effects are both wrong, investigate their shared hardpoint/transform path before introducing separate corrections.

If scripted arrivals are wrong but ordinary Skirmish spawns are correct, compare the creation/placement paths before touching global ship orientation.

If a planet becomes textured after unrelated rendering, investigate shared render state rather than forcing a planet texture bind.

If a retail database record is smaller than the current runtime type, do not assume padding or parser conversion makes the raw load safe. Trace the actual loader.

---

# Transform discipline

Keep these coordinate layers conceptually separate:

1. authored model coordinates
2. model-local retail correction
3. gameplay/world transform
4. hardpoint-local transform
5. joint/turret transform
6. effect/projectile transform
7. renderer-local transform

Never change gameplay transforms merely to repair presentation unless retail evidence proves gameplay is wrong.

Before adding a transform correction, trace whether the same conversion is already applied upstream or downstream.

Avoid double corrections.

---

# Renderer discipline

For authored retail CFW world content, preserve the established CFW route as the primary path:

`Engine -> RenderMgr -> PolyMesh -> ZBatcher -> CQPipeline -> D3D9`

The present D3D9 backend is retained during restoration.

MeshManager, MaterialManager, application manual rendering, and related CQ2-era services are not a second independent renderer. They are front ends sharing backend state and some currently support required effects or services.

Do not delete or bypass them solely because retail did not load an equivalent component.

Classify their individual consumers first.

Do not install blanket per-object render-state defaults to hide state ownership defects.

State ownership belongs at deliberate renderer transitions or established frame boundaries.

Do not infer renderer usage from source-file presence. Verify the actual project/build graph and, when needed, runtime usage.

---

# Retail data discipline

Keep distinct:

- raw retail database loading
- parser/schema conversion
- mission/save correlation
- live runtime structures

Do not assume one mechanism repairs another.

`GameTypes.db` and `GenData.db` use raw consecutive record loading where established by current evidence.

Mission/save conversion through DocuView is a separate contract.

Never shrink or reshape a live runtime/save structure solely because a retail disk record is smaller.

Use explicit retail representations, checked boundaries, semantic adapters, or equivalent evidence-backed solutions where appropriate.

Retail parser resources extracted from shipped `Globals.dll` are authoritative schema evidence.

---

# ABI discipline

The retail campaign DLLs are fixed binary consumers.

Do not casually change:

- exported signatures
- decorated names
- calling conventions
- parameter types
- enum representation
- struct size
- field order
- bitfield packing
- alignment

Matching exports do not prove compatible parameter/structure layouts.

Re-run the existing ABI verification after relevant changes.

Use measured compile-time assertions where appropriate.

---

# Build discipline

Use the supported full rebuild workflow.

Do not trust incremental builds where tracked object/resource files can hide stale output.

Maintain Debug and Final as separate validation subjects.

Application projects remain on the currently required CRT model unless retail-restoration evidence explicitly justifies a change.

Build/deployment manifests establish artifact provenance.

Compilation and hash verification prove what was built and deployed; they do not prove gameplay or visual parity.

---

# Investigation discipline

Work on one defect class at a time whenever practical.

Preferred loop:

1. reproduce
2. establish retail control
3. capture reconstructed behavior
4. identify first divergence
5. form the smallest testable hypothesis
6. patch only when justified
7. full rebuild
8. run the same acceptance test
9. update `PROJECT_BIBLE.md`
10. remove obsolete diagnostics

Do not opportunistically fix neighboring problems during an unrelated investigation.

Record them in the Bible's open-work section instead.

---

# Diagnostics and raw evidence

Prefer:

- Ghidra/disassembly
- TTD
- debugger inspection
- minidumps
- narrow instrumentation
- deterministic probes

over broad logging.

Do not leave high-volume synchronous logging in render, allocation, or update hot paths after its question has been answered.

Retain raw evidence when it:

- establishes retail behavior that is difficult to reproduce;
- proves an accepted root cause;
- supports an unresolved investigation;
- preserves unique reverse-engineering work.

Old traces/logs that have no unique evidentiary value may be archived or deleted after their durable conclusion is captured in `PROJECT_BIBLE.md`.

Never delete Ghidra work, parser resources, retail reference data, unique dumps, or unique retail traces merely as repository cleanup.

---

# CQ2-only behavior

Do not assume a feature belongs in retail merely because it exists in the released source.

For behavior proven to be CQ2-only:

- do not spend restoration effort making it work unless shared infrastructure is required by retail;
- isolate or remove the CQ2 execution path when safe;
- first prove that no required retail consumer depends on it.

Do not bulk-delete CQ2 systems.

---

# Updating PROJECT_BIBLE.md

Every meaningful investigation must finish by updating the Bible.

Update the existing relevant section rather than appending a new dated report.

For each affected finding record:

- current status
- concise conclusion
- evidence
- relevant source/data paths
- accepted implementation, if any
- acceptance test/result
- remaining uncertainty

When stronger evidence overturns an earlier conclusion:

1. replace the current canonical statement;
2. record the old statement under the Bible's `Superseded Findings` section;
3. state what evidence superseded it.

Do not leave two contradictory claims presented as simultaneously current.

---

# Existing documentation

The repository currently contains many older investigation, audit, milestone, handoff, and progress documents.

During consolidation:

- treat them as source material, not equal authorities;
- prefer later evidence-backed corrections over earlier hypotheses;
- preserve unique evidence references;
- move historical narrative documents out of the active documentation surface after consolidation;
- do not destroy raw evidence merely because the narrative document referencing it is superseded.

After consolidation, old narrative files must not be consulted as current implementation authority when `PROJECT_BIBLE.md` contains a newer canonical conclusion.

---

# Research tooling and sidecar protocol

ConquestFWR uses an evidence-first, sidecar-first research model. The primary agent owns research direction, evidence adjudication, status changes, and the decision to move from research to implementation. Deterministic extraction, indexing, tracing, measurement, comparison, and repeatable experiments belong in bounded, versioned sidecars when practical. Use subagents for ambiguous interpretation, architectural reasoning, historical reconstruction, experiment design, or skeptical review—not as the default workers for bulk mechanical extraction.

The primary host is Windows, with project work under `D:\Dev\CFW`. Before creating or installing research tooling, inspect the project tools, `C:\Utilities\TOOLS.md`, and relevant existing utilities. The known shared installations are `C:\Utilities\ghidra_12.1.3_PUBLIC` and `C:\Utilities\x32dbg`; verify their identity and the target architecture before relying on them. Keep CFW-specific offsets, binary identities, schemas, and matchers in the project. A genuinely cross-project tool belongs under `C:\Utilities` only after project assumptions are removed and independent validation, provenance, versioning, dependent-project review, and registry update are complete. Do not overwrite a stable shared tool for an experiment or install a duplicate without checking the registry. Changes outside this repository require their own authorization and filesystem permission.

Consequential sidecars must be read-only against authoritative retail inputs, offline where practical, deterministic where possible, and explicit about input hashes, configuration, tool/version identity, environment dependencies, output hash, warnings, and validation status. Prefer structured JSON/JSONL/SQLite output and meaningful failure exit codes. Preserve failed methods and negative results. Generated evidence is staged/unreviewed until the primary agent reviews it; a sidecar score or pointer-table candidate is never automatically a `VERIFIED` correspondence. Do not use reviewed development identities as matcher features, seeds, or same-run validation data.

Use `Research/RetailMap/` and its SQLite schema as the managed M2.5 correspondence-evidence layer when relationships must persist and correlate. Keep raw captures, generated databases, tool runs, hypotheses, reviewed decisions, contradictions, and provenance distinguishable. Record durable project conclusions and open questions in `PROJECT_BIBLE.md`; it also holds this project's toolchain/dependency manifest. Do not create the generic protocol's suggested `TOOLCHAIN.md`, `RESEARCH_STATUS.md`, agent-report, or other narrative Markdown files: the canonical-document rule above takes precedence for ConquestFWR. Project-specific machine-readable manifests and raw evidence remain permitted in appropriate evidence locations.

For significant claims, distinguish the project status (`VERIFIED`, `CANDIDATE`, `OPEN`, `CONTRADICTED`, `SUPERSEDED`) from evidence character (`OBSERVED`, `DERIVED`, `CORRELATED`, `INFERRED`, `HYPOTHESIZED`, `UNKNOWN`). Preserve artifact SHA-256, size/architecture/build identity where relevant. A failed research or generalization gate is a valid result; do not weaken it after seeing outcomes or silently replace unknown retail behavior with an approximation.

---

# Stop condition

If evidence contradicts itself and the conflict cannot be resolved cheaply, stop the implementation change.

Record the contradiction in `PROJECT_BIBLE.md` and identify the next observation capable of resolving it.

Do not choose whichever interpretation makes the patch easier.
