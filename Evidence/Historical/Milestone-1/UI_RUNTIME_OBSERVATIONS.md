# UI runtime observations

> **Historical documentation — provenance only.** This document is not current project authority. See [PROJECT_BIBLE.md](../../../PROJECT_BIBLE.md) for canonical status; preserved for provenance.

These are user-observed runtime regressions. They are recorded for later investigation and are
not yet root-cause findings.

## Top-right toolbar buttons — 2026-09-06

At 1600x1200, both buttons at the far right of the resource bar render but do not respond when
clicked:

- The checkmark button should open **Options**.
- The list button immediately to its right should open **Mission Objectives**.

Reference capture:
`C:/Users/Joel/AppData/Local/Temp/codex-clipboard-27df8bcf-d805-4ca9-befe-ee5a202b1f99.png`

Later investigation should compare the rendered button rectangles with their active hot-rects,
notification routing, and retail toolbar definitions. The recent resource-counter positioning
work should be checked for coordinate or overlap side effects, but is not assumed to be causal.

### Root cause and compatibility repair — 2026-09-07

At 1600x1200 the active rectangles are correctly placed: Options occupies x=1502..1551 and
Mission Objectives x=1550..1599, both at y=0..31. Click dispatch also reaches the ordinary
`HotButton` hotkey event path. The shipped `gendata.db`, however, stores older numeric hotkey IDs
(`0x38` and `0x4E`) while the CQ2-era generated `Hotkeys.h` used by this build assigns the intended
actions `0x51` and `0x6A`.

`CONTROL_NODE::initControl()` now translates only the two stable toolbar control names,
`options` and `missionObjectives`, to the current enum values using a local copy of their data.
Geometry, art, and the database remain unchanged. A global numeric remap was deliberately avoided
because the same values can occur in unrelated contextual controls.

Runtime acceptance remains pending: the checkmark must open Options and the adjacent list button
must open Mission Objectives without changing resource-counter placement or other toolbar actions.

## Debug title marker — confirmed expected 2026-09-06

The trailing glyph visible after the game title in the Escape menu and window caption is the
intentional Debug-build marker. The user confirmed this is expected behavior. It is not a
localization defect and requires no remediation.
