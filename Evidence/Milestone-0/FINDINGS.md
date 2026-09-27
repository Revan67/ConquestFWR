# Milestone 0 Findings

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

Date: 2026-09-03

Status: baseline established; behavioral remediation has not begun.

## Verified facts

1. `master` began this milestone at the same commit as `origin/master`:
   `8051dd2fd54e141000694a4c24e027a086e6a52f`.
2. The pre-remediation tracked working tree is recoverable through local ref
   `refs/checkpoints/milestone-0-pre-remediation`, commit
   `c8500e873213a5c1c3fa7cb9d1e0d56e9c64bbdb`. Untracked files are not in that
   commit; their hashes are in `baseline-manifest.json`.
3. Three independently located retail installations have byte-identical copies of the core
   retail executable, runtime DLLs, `gendata.db`, and `gametypes.db`. Their `Conquest.ini`
   files are not identical and therefore are not canonical identity evidence.
4. The retail executable baseline is 1,732,608 bytes, reports file version
   `01.07.10.1802` / product version `1.05`, and has SHA-256
   `ED8741C0CB727A43B7DE7BE0E73566B04FBC1CBD58B54A62D538E3F39DCB3F52`.
5. Resource `PARSER/156` independently extracted from all three retail `Globals.dll` copies
   is identical: 164,530 bytes, SHA-256
   `C8A1215C3422C75B2BD29D793C9E28FFA1BAF2DE5E0C244C20D26F7EE737860D`.
6. The archived `retail_data_i.txt` is the same parser data after its LF line endings were
   converted to CRLF: 17,127 CRLF pairs account exactly for its 181,657-byte size. After
   CRLF-to-LF normalization its bytes and SHA-256 exactly match the extracted retail resource.
7. The current source `Code/App/Src/data.i` exactly matched the Debug deployment's embedded
   parser resource when the baseline was captured: 184,980 bytes, SHA-256
   `B242E982E8AE69784B750C1FF80C4EE09AB07BD4A3AD823AF7E1558604D7AD3B`.
8. The current deployed `gendata.db` and retail `gendata.db` have equal lengths but differ in
   exactly 126 byte positions. The first difference is offset 28,388 and the last is 226,600.
   This confirms the byte-level fact only; the meaning of those bytes remains unproven.
9. A no-deploy Debug rebuild completed successfully and produced all five required core
   binaries plus all 18 campaign modules.
10. A no-deploy Final rebuild completed successfully and produced all five required core
    binaries plus all 18 campaign modules.

## Control problems found

- Before rebuilding, the existing Debug outputs matched the Debug deployment byte-for-byte.
  The existing Final outputs were newer than the Final deployment and all five core hashes
  differed. The Final deployment therefore cannot be treated as the current Final build.
- Rebuilding changes PE hashes even without source edits, so raw equality to a prior deployment
  is useful for identifying the exact deployed artifact but is not by itself a reproducible-build
  test.
- Both configurations generate many warnings. Particularly important are MSBuild warnings about
  shared intermediate directories and target/output path mismatches, plus linker warnings that
  images are produced under `/FORCE` and may not run. These are future build-system remediation,
  not evidence that the current runtime behavior is correct.
- The campaign solution uses one shared `Scripts/Build` output directory. A Debug build followed
  by Final replaces those campaign artifacts. Configuration-specific evidence must therefore be
  captured immediately or the output layout must later be separated.
- The working tree contains a mixed batch of behavior, renderer, UI, build, documentation, and
  generated-binary changes. None are accepted as correct merely because they compile or because
  an archived note describes them as fixes.

## Authority hierarchy going forward

1. Reproducible retail behavior and independently matching retail binaries/data.
2. Source and binary-level evidence tied to a recorded hash and path.
3. Controlled A/B tests with one classified change at a time.
4. Archived notes only as hypotheses and navigation aids.

## Milestone 1 entry conditions

- Use the checkpoint ref when separating or reverting the current mixed changes.
- Do not modify retail reference trees.
- Keep Debug and Final results distinct.
- Map the 126 database byte differences through the verified retail parser before describing
  them as field changes.
- Establish a small smoke-test sequence for boot, mission start, combat, construction, mining,
  sector navigation, and clean exit before accepting remediation batches.
