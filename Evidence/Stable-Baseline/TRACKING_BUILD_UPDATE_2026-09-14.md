# Tracking build update — 2026-09-14

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

Requested: update builds and launchers for renderer usage capture.

Changes: run-debug.cmd, run-debug-capture.cmd and run-final.cmd set CFW_RENDER_USAGE=1
inside setlocal. The capture launcher additionally runs ProcDump as before. Deploy-Conquest
now copies and hash-verifies the corresponding launchers in each installation. No mission,
texture, or save data is modified. Running Conquest.exe directly still leaves tracking opt-in.

Standalone counter test: 40,000 calls across four threads, clean-exit snapshot, and opt-out
no-file behavior passed. Debug and Final complete-schema parser probes returned 0 with zero
diagnostic errors.

First Debug rebuild completed 20:03:43 UTC. Its deployment stopped before copying because
the user had launched Conquest (PID 46940, Debug installation). No game process was terminated.
Final rebuilt successfully at 20:06:51 UTC, with all 22 engine and 18 campaign modules.
Both builds use input fingerprint
`CF7B7CEFD8193F389B52BC78E653A6BA4E1CCA989DB6E8A9F8FD899E5EF753E9`.

Post-Final manifest audit found three Debug files changed by the Final build:
Code/Libs/Static/debug/COMHeap.lib, Code/Libs/Static/debug/MathLib.lib and
Code/Libs/ImplicitDLL/debug/DACOM.dll. Final's own artifact hashes all matched. This is a
cross-configuration build dependency issue, not permission to bypass verification. Debug
is rebuilt last and both manifests must be checked again before deployment. No solution
mapping changes are included in this update; configuration isolation needs a separate audit.

Deployment remains gated on Conquest being closed. Final status will be appended below.

## Completed

Final Debug rebuild completed 20:09:52 UTC. After it finished, both Debug and Final manifests
had zero artifact hash mismatches. Once Conquest was closed, the guarded deploy completed
for both installations, reporting 47 runtime artifacts hash-verified in each. Launchers were
also copied and verified. Full-schema parser probes then ran against the INSTALLED DACOM
and DocuView DLLs in both installations: exit 0, all conversion tests passed, zero parser errors.
Final binary coverage checks found all 14 expected compiled route labels.

Use run-debug-capture.cmd for the next Mission 1 tracking run. It enables renderer counters
and ProcDump capture; no separate environment setup is required. These checks validate the
build/deployment/instrumentation, not in-game visual correctness or usage shares.
