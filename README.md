# Conquest: Frontier Wars — Retail Restoration

This project restores the officially released **Conquest: Frontier Wars** development source so
the 2001 game can build and run on modern Windows with Visual Studio 2022.

The released code is not the exact retail source tree. It is a later development snapshot for the
unshipped sequel, commonly identified in the code as **CQ2** or **Conquest: Vyrium Uprising**.
Retail data and media are therefore being matched against a codebase containing changed binary
layouts, object IDs, renderer paths, and unfinished sequel features.

The project has two ordered goals:

1. Restore the game as closely as practical to shipped retail behavior.
2. Modernize the renderer, platform code, and supporting systems only after the retail baseline is
   stable and measurable.

Modification notice: this is a modified version of the Program. The modern build conversion,
retail-restoration work, diagnostics, and documentation are community modifications by Revan67,
principally dated 2022-12-25 and 2026-03-25 through 2026-08-30. The entire Program Derivative is
released solely under the exact terms of the Fever Pitch Studios source license. See
[MODIFICATIONS.md](MODIFICATIONS.md) for the prominent modification record and
[LEGAL.md](LEGAL.md) for the repository's compliance and interoperability policy.

## Current status

Last verified: **2026-08-30**

| Area | Status |
|---|---|
| VS2022 Win32 build | **Working** — Debug and Final rebuild successfully |
| Retail campaign DLLs | **Working** — all 18 build, verify, and deploy together |
| Startup and mission loading | **Working**, with repeated-launch soak still required |
| Ship movement and path completion | **Fixed and runtime-confirmed** |
| Ship model orientation and combat facing | **Fixed and runtime-confirmed** |
| Opening Mission 1 combat progression | **Fixed and runtime-confirmed** |
| HQ/platform construction crash | **Fixed and runtime-confirmed** |
| Combat renderer lighting crash | **Fixed and runtime-confirmed** |
| Completed platform textures | **Fixed and runtime-confirmed** |
| Explosion mesh splitting | **Stabilized**, broader combat soak required |
| Mission 1 final-wave progression | **Fix deployed**, runtime retest pending |
| Projectile and engine-light transforms | **Open** — effects are offset or fire along the wrong axis |
| Planet rendering | **Partial** — geometry works; diffuse/depth behavior remains wrong |
| Refinery/shipyard transient materials | **Open** — some active construction meshes sample wrong textures |
| Remaining retail schema differences | **Approximately 11**, down from 37 |
| UI parity | **Partial** — several layout and information-panel differences remain |
| Modern renderer/codebase | **Not started as the primary line of work** |

Mission 1 currently reaches and survives combat, advances to the Fabricator, constructs the HQ and
other available orbital structures, builds ships, and survives later combat waves. The immediate
test target is the final Tau Ceti wave: after its last Mantis ship dies, the mission should advance
to the beacon-recovery objective.

For a detailed working-state handoff, see [HANDOFF.md](HANDOFF.md). For acceptance testing, see
[RETAIL_TEST_MATRIX.md](RETAIL_TEST_MATRIX.md).

## What the source release does and does not provide

The source release contains enough material to rebuild the game executable, application DLLs,
engine modules, data-schema generator, and all 18 retail campaign modules. The project is not
blocked merely because the source came from a development branch.

What it does **not** provide is the complete set of retail media assets. A legitimate retail
installation is still required to run the rebuilt code because music, video, textures, meshes,
missions, and other shipped content are not duplicated in this repository.

The practical restoration problem is compatibility:

- Retail `.db` files must be parsed with retail-compatible structure layouts and enum values.
- Campaign DLLs and `Mission.dll` share a decorated C++ ABI and must be built/deployed together.
- CQ2 rendering paths sometimes interpret retail meshes and materials differently.
- Modern wrappers and worker threads expose races that were rarely encountered in the original
  single-threaded runtime environment.

Retail binaries, resources, and observed behavior are treated as the authority whenever the
development source disagrees with shipped data.

## Requirements

- Windows 10 or Windows 11 with 32-bit application support.
- Visual Studio 2022 with the **Desktop development with C++** workload.
- Python 3 with the Windows `py` launcher. It replaces the original 16-bit XMIFF build utility.
- DirectX SDK (June 2010), extracted beside the repository as `DXSDK`.
- DirectX 9 runtime components.
- A retail installation of Conquest: Frontier Wars for runtime media.
- MFC for Visual Studio 2022 only if building the separate map editor.

### DirectX SDK setup

Installing `DXSDK_Jun10.exe` often fails with error `S1023` on modern systems containing newer
Visual C++ redistributables. Do not remove system redistributables. Extract the SDK instead:

```text
7z x DXSDK_Jun10.exe -o"<repository-parent>" "DXSDK/Include/*" "DXSDK/Lib/*"
```

`Directory.Build.props` resolves the SDK at `..\DXSDK` relative to this repository. The important
files are `Include\d3dx9.h`, `Lib\x86\d3dx9.lib`, `d3dx9d.lib`, and `DxGuid.lib`.

## Build and run

Use the supported root-level entry points:

```bat
build-debug.cmd
build-final.cmd
```

Both commands perform a full Win32 rebuild of:

- engine renderer modules and their dependencies;
- `Conquest.exe`, `Globals.dll`, `Mission.dll`, `Trim.dll`, and `ZBatcher.dll`;
- all 18 retail campaign modules: `SCRIPT01`–`SCRIPT16`, `Mantis_T`, and `Sol_T`.

The wrapper verifies required outputs, deploys them to the selected retail installation, and
compares SHA-256 hashes so a successful message cannot conceal a stale deployed DLL.

The default local destinations are:

- Debug: `D:\Games\GOG\Conquest Frontier Wars`
- Final: `D:\Games\GOG-Final\Conquest Frontier Wars`

Override the destination or disable deployment:

```powershell
.\Build-Conquest.ps1 -Configuration Debug -NoDeploy
.\Build-Conquest.ps1 -Configuration Final -DeployDirectory 'D:\Games\Another CFW Test'
```

Launch through the wrappers so the process starts in the retail installation directory:

```bat
run-debug.cmd
run-final.cmd
```

The working directory matters. Launching from an unrelated command prompt can make the game fail
to locate `Conquest.ini` even when the executable itself is valid.

### Solutions and configurations

- `Code/App/Src/Conquest.sln` builds the main application using `Debug | Win32` or
  `Final | Win32`.
- `Code/App/Src/Scripts/ConquestCampaign.sln` builds exactly the retail campaign set using
  `Debug | Win32` or `Release | Win32`.
- The root build wrapper maps campaign Debug to application Debug and campaign Release to
  application Final, preventing mixed import libraries.

Always use **Rebuild**, not an incremental Build. Legacy tracked intermediates, resource steps,
and precompiled-header behavior can otherwise make MSBuild reuse stale output. Building only
`Conquest.vcxproj` is also unsafe because it consumes sibling import libraries without rebuilding
the corresponding DLL projects.

Debug keeps `CQASSERT` and `CQBOMB` active; they intentionally execute `int 3`. Final defines
`FINAL_RELEASE` and is the closer retail-parity configuration. They are configurations, not Git
branches.

The application projects must remain on the static `/MT` runtime while DACOM/COMHeap owns the
game heap. Switching them to `/MDd` creates incompatible allocation paths.

## Restoration principles

### Retail first, modernization second

CQ2 additions are not automatically improvements. If an unshipped feature changes retail data
layout or behavior, the retail path is restored first. Modernization should later occur on a
separate line of work with parity tests available to expose regressions.

### Measure layout, not only size

A `sizeof` assertion cannot prove a structure is correct. Two opposite layout errors can cancel
and leave the final size unchanged. That exact failure broke ship movement: a 32-byte-short
mission-data block was masked by 32 bytes of unrelated CQ2 fields elsewhere in the structure.

Important layouts therefore use field-offset and end-of-structure invariants in addition to size
checks. The parser schema embedded in the shipped retail `Globals.dll` is the primary source for
archetype layout.

### Keep ABI-linked modules together

Campaign scripts import decorated C++ symbols from `Mission.dll` and `Trim.dll`. A symbol can keep
the same name while a referenced structure changes size, so successful linking alone is not proof
of ABI safety. Critical cross-module types have compile-time size guards, and the complete retail
campaign is rebuilt and deployed with the engine.

The current ABI audit accounts for 153 imported `Mission.dll` symbols and two imported `Trim.dll`
symbols. Re-runnable checks are under `Tools/abicheck`.

## Major fixes completed

### Retail object IDs and Mission 1 progression

The sequel source inserted new values throughout `M_OBJCLASS`, shifting every later retail object
ID. Retail Mantis frigates arrived as class 73 while the source compared them with class 90, so
destruction events could not update the intended campaign state. Retail values `0..110` are now
preserved exactly, sequel-only values follow the retail sentinel, and critical IDs have compile-
time guards.

Mission 1's later combat counter could also become stale or underflow when construction waves
overlapped the final attack. The final Tau Ceti step now derives the counter from live local Mantis
frigates/scout carriers and guards zero before decrementing it.

### Ship movement and model orientation

Retail stores the complete 72-byte `MISSION_DATA` inside ship archetypes. The development source
substituted a 40-byte `MISSION_DATA_BIN` and later masked the size deficit with fabricated rocking
fields. `dynamicsData` was consequently read from the wrong offset and could produce a negative
`maxAngVelocity`, forcing a nonzero turn every frame and preventing path completion.

The retail layout and offset guards are restored. Ships now move, complete paths, stop, face their
targets, and render level with the authored bow pointing forward. The visual model-axis correction
is applied through the renderer so the gameplay/physics transform remains planar.

### Explosion and debris stability

Explosion splitting previously reached mesh nodes without valid vertex bindings, wrote past
fixed-size split buffers on unsuitable meshes, and could destroy child engine instances twice when
debris expired. MeshInfo bindings are restored, incomplete/unsafe meshes are rejected before
splitting, ownership is normalized before destruction, and extreme angular debris velocity is
bounded.

The fixes have survived representative combat and multiple destroyed ships. Broader soak testing
is still required before treating the entire explosion system as closed.

### HQ/platform construction crash

A placement shadow could initialize a shared renderer array with fewer children than the completed
platform. Constructing the Mission 1 HQ then indexed beyond that array. Completed objects now grow
undersized shared arrays and bind every actual mesh child safely. HQ construction, all other
available structures, and ship production have completed successfully in runtime testing.

### Combat lighting crash

The D3D renderer stored four selected lights locally but searched for a key light using an enabled
count of up to eight. An out-of-range selection copied a full `D3DLIGHT9` over the stack frame. All
selection and key-light loops are now bounded to local/shader capacity, zero capacity cannot
underflow, and the enabled-light list is bounded in Final builds as well as Debug.

### Startup allocator race

Modern Direct3D wrappers may create worker threads while engine DLLs are attaching. Their CRT
bookkeeping can allocate through the shared DACOM heap concurrently. The main game heap now enables
DACOM's existing multithreaded path. A complete mission run has passed since the fix; repeated rapid
launch/exit testing remains on the acceptance list.

### Retail rendering path restoration

Completed orbital platforms now render through the retained retail engine-instance path, fixing
their black/white and unrelated striped materials. Planets use the same path and are visible again,
but their diffuse texture and depth interaction with the far side of the orbital ring remain open.

## Current priority queue

1. Retest the Mission 1 final-wave live-count fix through the beacon-recovery transition.
2. Correct the shared hardpoint/effect axis used by projectile direction and engine lights.
3. Restore planet diffuse material and depth/occlusion behavior.
4. Fix refinery and shipyard active-construction material binding.
5. Perform repeated startup testing and broader explosion/debris combat soak.
6. Resolve the remaining retail schema differences, led by `BT_FLAGSHIP_DATA`.
7. Restore retail cinematic paths and finish the smaller UI-parity issues.
8. Begin renderer/code modernization only after Priority 1 has a stable test baseline.

## Debugging and evidence

### Crash capture

`run-debug-capture.cmd` launches the Debug build with crash capture enabled. Local dumps are kept
under `CrashDumps` and intentionally ignored by Git because a single dump can be several gigabytes.
`Tools/analyze_minidump.py` provides a repeatable first-pass analysis.

When recording a crash, retain:

- build configuration and binary timestamps;
- mission/save and reproduction steps;
- call stack or minidump;
- the deployed binary hashes;
- a Time Travel Debugging trace when the reproduction is short enough.

### Time Travel Debugging

TTD is particularly useful for old stateful code where the visible failure occurs long after the
corrupting write. Useful lessons from this restoration:

- `L<n>` debugger counts are hexadecimal; use `L0n12` for twelve decimal elements.
- Prefer named fields (`dt Type address field`) over copied offsets.
- Query a full trace rather than logging only the first N calls, which usually samples mission
  loading instead of steady-state gameplay.
- Archive matching PDBs with traces; rebuilding makes old source-level symbol results unreliable.
- Do not interrupt large trace indexing. A partial index is normally unusable.

### Retail schema

The shipped retail `Globals.dll` contains a `PARSER` resource holding the authoritative `data.i`
schema. Compare it with generated `Code/App/Src/data.i`, but edit the corresponding headers under
`Code/App/DInclude`; `data.i` is generated output.

## Repository layout

```text
Code/
  App/
    DB/                 Development-source database files
    DInclude/           Shared data/ABI declarations and parser source of truth
    Src/                Main executable and application DLL sources
      Scripts/          All campaign sources and ConquestCampaign.sln
  Editor/               Separate MFC map editor
  Libs/
    Include/            Shared engine headers and compatibility headers
    Src/                DACOM, renderer, MeshManager, and other engine sources
    Static/             Engine import/static libraries
    ExplicitDLL/        Explicitly loaded runtime engine modules
    ImplicitDLL/        DACOM runtime

Tools/
  abicheck/             Campaign/Mission decorated-symbol checks
  analyze_minidump.py   Repeatable minidump summary tool
  read_gendata.py       Database/archetype inspection utility
  retail_gametypes.h    Retail structure-layout reference
  xmiff2/               Python replacement for the legacy XMIFF data compiler

Build-Conquest.ps1      Supported full build, verification, and deployment wrapper
HANDOFF.md              Detailed current engineering state
RETAIL_TEST_MATRIX.md   Retail-parity acceptance ledger
```

## Legal and compliance

The controlling license is the unmodified
[Public License for Conquest: Frontier Wars Program and Source Code](Conquest%20Source%20License.txt),
copyright Fever Pitch Studios, December 2013. This repository is a **Program Derivative**, is
non-commercial, and is conveyed as a whole solely under that license's exact terms. The license is
revocable and prohibits payment, donations, expenses, gratuities, usage fees, or any other
consideration in exchange for the Program, a Program Derivative, warranty, or support.

Every source or binary distribution must include, conspicuously and intact:

- `Conquest Source License.txt`, including its warranty and liability notices;
- [MODIFICATIONS.md](MODIFICATIONS.md), identifying the date and author of modifications; and
- a prominent statement that the entire Program Derivative and every recipient are subject to the
  exact same license terms.

[LEGAL.md](LEGAL.md) contains the full release checklist, third-party-material policy, and the
narrow rules for compatibility research. In short, retail comparison is limited to functional
facts needed for interoperability with a lawfully obtained copy. Do not commit or distribute retail
executables, extracted retail media, disassembly/decompiler output, access-control keys, or
circumvention tools. The project neither authorizes nor requires bypassing copy protection.

`LEGAL.md`, this summary, and the modification record are notices and project procedures only. They
do not amend, replace, or add terms to the Fever Pitch Studios license. If any summary conflicts
with the license, the license controls. This is a good-faith compliance policy, not legal advice or
a guarantee that every historical file has sufficient third-party redistribution rights.

## Contributing

Contributions are welcome when they follow the project order of operations:

- establish retail evidence before changing behavior;
- explain binary-layout or ABI reasoning explicitly;
- add or update a reproducible test where practical;
- rebuild Debug and Final plus the complete retail campaign set;
- do not add retail media, crash dumps, local tool installations, or other large generated files;
- identify the author and date of modifications in [MODIFICATIONS.md](MODIFICATIONS.md);
- document the provenance and license of every new third-party dependency or binary; and
- keep all work non-commercial and follow [LEGAL.md](LEGAL.md) and the controlling source license.

## Acknowledgements

- Fever Pitch Studios for releasing the development source.
- The original Conquest: Frontier Wars development and publishing teams.
- The Conquest modding community for preserving documentation, tools, and knowledge of the game.
