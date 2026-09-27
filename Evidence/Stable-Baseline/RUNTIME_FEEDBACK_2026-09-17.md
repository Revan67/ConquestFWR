# M0/M1 candidate runtime feedback

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

Run: process 46456, started September 17 at 20:49 local. Snapshot collected while
the game was still running. Evidence is in runtime-2026-09-17-pid46456, including
the deployed build manifest, four supplied screenshots, harvester/startup logs,
and renderer route counters. These are live snapshots, not clean-exit captures.

## User observations

- HQ/refinery/Marine Training Facility placement ghosts are white. Valid and
  invalid placement cannot be distinguished. Placement visual acceptance fails.
- Fabricator construction animation now moves correctly, but its animated
  geometry is fully white/untextured. Refinery construction has the same defect.
- Planet is white/grey without its expected texture (reference 1).
- Ships are still constructed vertically (reference 2); retail orientation
  verification remains required.
- Both harvesters stop beside resource nodes (reference 4).
- Separate gameplay defect: issuing Stop during construction does not actually
  stop construction. Track command delivery, active build operation, producer
  and target cancellation/completion separately from construction rendering.
- No crash was reported in this feedback. This does not establish repeated
  placement/cancel/reload/shutdown acceptance or resolve the CRT heap assertion.

## Direct trace evidence

Harvester 0000AAB1 remains in mode 12, work 2422, target 0100A260,
at (23552,72704,-400), with goal/next (23552,72704,0), zero velocity,
pathLen=1, final=1, move=1, auto=1, physics=1, canMove=1 and rotate=0.
Harvester 0000AAE1 remains in mode 12, work 2333, target 0500A260,
at (25600,70656,-200), with goal/next (25600,70656,0), and the same
movement flags. Both report cargo=0/10, lockOwner=-1, locking=0 and queue=0.
These values persist across repeated updates. Neither has lost its target.

Hypothesis to investigate: horizontal waypoint arrival and vertical offset may
disagree with operation completion. Verify mode meaning and caller/callee
arrival contracts before changing logic. Logged target distances alone do not
establish an invalid destination because resource-node extent may matter.

App-manual Render counters continue advancing beyond 44,000 calls. This proves
that route is reached, not that its tint, textures or GPU state are correct.
White ghosts, white animated geometry and the untextured planet may share a
render-state/material failure, but this run does not prove a common cause.

## Acceptance disposition

Keep M1 open for lifecycle/reload tests. Preserve M0's independently verified
build-isolation result. Carry rendering failures into M4/M5/M6, mining into M7,
and the construction-stop behavior as a distinct gameplay item under M6.
No additional source changes or deployment were performed for this feedback.
