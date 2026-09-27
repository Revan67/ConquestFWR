# HQ preview crash — 2026-09-15

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

Status: immediate fault and upstream invalid-archetype input identified (2026-09-16).
No runtime source changes or deployment made during this investigation.

## Evidence identity

- Dump: `D:\Dev\CFW\CrashDumps\Conquest.exe_260915_202400.dmp`.
- Tracking process: 36200; exception thread: 54540.
- Loaded executable: `D:\Games\GOG\Conquest Frontier Wars\Conquest.exe`.
- PE timestamp: 1789416568; image base 0x00400000, size 6025216.
- Installed executable SHA256:
  `54F03434B8F14A3B50F75999694888B83912BBE8E13C88F0A266627ACA79FD64`.
  Matches `Build-Manifests/Debug-latest.json`.
- Fault instruction bytes match the installed executable at RVA 0x1BD417.

## Confirmed fault and upstream state

Exception address 0x005BD417: `cmp dword ptr [eax+0x50], 0`.
EAX is zero; attempted read address is 0x50. This is the newly added
`mesh->material_list` condition in `MeshRender::Render`, MeshRender.cpp:852.
The null result from `REND->get_instance_mesh` is dereferenced before the
existing `bHasMesh == 0` Engine fallback.

Dump MeshInfo at 0x12F6FCD0:

- instanceIndex at offset 0x30 = 0xFFFFFFFF (invalid).
- fgi at offset 0x1C = null.
- bHasMesh (bit 2 of byte at offset 0x38) = false.
- parent tree at 0x12E330B8 has this MeshInfo, no children, and count 1.

The TreeRender frame receives MeshChain at 0x12F7AEEC. Its containing MeshEntry
at 0x12F7AED8 has archetype ID 0x9038 and instanceIndex 0xFFFFFFFF too; this is
not merely a valid compound parent with no mesh.

`UnbornMeshList::createMesh` stores the result of `ENGINE->create_instance2`
without testing for invalid, builds a MeshInfo tree, and caches the entry.
`RenderMeshAt` checks only the entry pointer and proceeds to TreeRender.
`ListChildren` deliberately includes the root. `SetupMeshInfo` attaches a
renderer but returns without allocating face-group data when bHasMesh is false.
This accounts for the downstream state in the dump.

Engine's create_instance2 can return invalid for an unavailable archetype or
failed compound creation. The specific reason for this preview's invalid ID
has NOT yet been established. Do not label this as a texture-upload failure,
ordinary geometry-free compound node, or proof that PolyMesh is broken.

## Repair requirements (not implemented here)

1. Establish why this HQ preview receives an invalid Engine instance: inspect
   the platform archetype handle, creation result, and instance lifetime.
2. Do not construct/cache a preview entry after failed instance creation; ensure
   callers and reference cleanup handle failure, with a useful diagnostic.
3. Put mesh-dependent material refresh after valid-instance/no-mesh handling.
   Preserve the legitimate Engine fallback for valid non-mesh nodes. A blanket
   null return alone can hide the crash while leaving an invisible HQ preview.
4. Audit neighboring preview transform/color calls and shared renderer-array
   assumptions. Validate HQ placement, cancellation, construction, repeated
   placement, and other structures after a complete verified rebuild/deploy.

## Renderer tracking from the same run

Last cumulative snapshots (crash, NOT clean-exit totals):

| Route | Calls |
| --- | ---: |
| world-frame | 6,213 |
| cfw-polymesh-render | 31,402 |
| appmanual-Render | 2,036 |
| backend-draw_primitive | 991,469 |
| backend-draw_indexed_primitive | 55,405 |
| backend-indexed-vb-cpuindices | 3,893 |
| backend-indexed-vb-gpuindices | 2,499 |

Read via `Tools/Verification/Read-RenderUsage.ps1 -GameProcessId 36200`.
Source logs: `%TEMP%\CFW-RenderUsage-36200-*.log`.
No recorded MeshManager-instance or MaterialManager route entries in this run.
Both CFW PolyMesh and application manual rendering are active. These layered,
overlapping counters are not GPU-time shares or a percentage of scene content.
The four libmanual hooks excluded from the build cannot be assessed by absence.

## 2026-09-16 follow-up: upstream cause established

This supersedes the earlier open question about why the preview instance was invalid.
The dump's ARCHNODE at 0x1301EBD8 has hArchetype = 0x12E9BFB0. Its PLATFORM_INIT
contains pData = 0x131A59FE, whose model filename is `THQ.cmp`, archIndex = -1,
and meshArch = 0x12F6D308. The MeshArch is not a reference (bRef false), has
refCount 1, and its own Engine archetype index at offset 0x20 is **0xFA (250)**.
GetEngineArchtype therefore returns 250, not -1.

PLATFORM_INIT::loadPlatformArchetype creates meshArch and uses its Engine index
for bounds, but never assigns the legacy archIndex field initialized to -1.
UnbornMeshList instead passes that obsolete field to create_instance2. The
downstream cached preview and MeshInfo thus retain an invalid instance ID.
This is an initialization/consumer mismatch, not evidence of failed THQ texture
loading or missing THQ.cmp. The ship initializer already assigns archIndex from
meshArch; copying that assignment alone is not an adequate ownership audit.

Additional affected platform consumers: shadow preview creation (TPlatform.h:1770)
and blinker setup (TPlatform.h:3101). The destructor releases archIndex directly,
but currently does not release meshArch. MeshManager::ReleaseArch also releases
the Engine archetype when its last wrapper reference is released. A repair must
explicitly choose borrowed versus independently held legacy handles to avoid
leaks or double releases. The ship destructor releases both representations,
so its existing pattern must not be treated as authoritative without auditing
matching holds.

No runtime source changes or builds were made during this follow-up. The repair
should address the stale index, preview creation failure, null dereference, and
reference ownership together, with placement/cancel/reload tests afterward.
