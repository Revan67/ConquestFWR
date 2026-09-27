# D3D9 backend audit — first pass

> **Technical evidence retained for provenance.** Historical status statements here may be superseded. See [PROJECT_BIBLE.md](../../PROJECT_BIBLE.md) for current project status.

Read-only runtime/source investigation. No code fix, build, deployment, or runtime
failure injection performed. Scope: active D3DRenderPipe backend and application
callers, not obsolete OpenGL files. Findings below are source-confirmed contracts
or risks, not demonstrated causes of the planet/mining/orientation symptoms.

## Findings

### P1: reset can block indefinitely and changes device contract

Direct3D_pipe.cpp:3859 destroy_buffers does not destroy a device: it calls Reset
in a while(FAILED(...)) loop with Sleep(100), no retry bound, no error classification,
and no message processing. A persistent failure therefore never returns.
menu.cpp:511/517 reaches it from IDH_TOGGLE_SCREENMODE in the active if(true)
branch. No automated mode-switch test was performed against the user's install.

Startup selects/validates D24S8 or D16, records depth/stencil capability flags,
and sets presentation interval ONE. Reset instead requests D16 and interval
DEFAULT without re-verifying/updating those flags. clear_buffers relies on those
flags. Thus successful reset can leave stale stencil capability information.
Reset also reloads effects, but its interaction with resources and both state
caches needs an explicit contract. Do not assume ordinary D3D9 resource-loss
rules apply unchanged to the current Ex/DXVK combination; validate that runtime.

### P1: repeat device creation loses ownership of the previous device

create_buffers:3719 calls destroy_buffers, then overwrites direct3d_device with
NULL at 3723 and creates a new device. The active destroy_buffers path does not
release the previous device. shutdown:1361 has its destroy_buffers call disabled.
The global scratchIB is created for a device and has no release/reinitialization
in the searched backend file. Repeated creation therefore has an ownership gap
and risks retaining a buffer from the previous device. Startup success alone does
not exercise this path. Test device generations and COM resource lifetime before
claiming a particular observed crash was caused by it.

### P1: indexed drawing can dereference failed allocations/locks

The CPU-index VB path at 2817 creates a fixed 3,000-index scratch buffer without
checking CreateIndexBuffer's result. It then calls Lock without checking its
result and memcpy's into the output pointer. Capacity is protected only by
ASSERT(num_indices < SCRATCHICOUNT), not a runtime bound. A failed creation/lock
can lead to null/invalid memory access. Oversized draws need explicit rejection,
growth, or safe splitting. This route executed in the recorded mission capture,
but allocation failure/oversized inputs were NOT observed.

The separate create_index_buffer helper at 2895 ignores CreateIndexBuffer's
HRESULT and returns GR_OK. Audit its consumers before implementing error handling.

### P1: presentation errors are hidden

swap_buffers:3591 calls Present without a device guard, logs its HRESULT only for
the first ten frames, and returns GR_OK regardless. The former recovery block is
commented out. Modal/System callers use this API; a failed presentation cannot
be diagnosed reliably by its return value. Define transient versus terminal
handling and rate-limited ongoing diagnostics; do not blindly reset on all errors.

### P2: clear request differs from actual device operation

clear_buffers:3530 computes a requested rectangle, color, and normalized depth,
then calls Clear with no rectangles, black, and depth 1.0. The debug observer
receives the requested values rather than the actual values. This is a confirmed
interface mismatch, not proof of a visual bug: inspected mission/loading/modal
call sites request full clears, and black/depth-one may match their needs.
Acceptance needs non-default color/depth and a bounded-rectangle test as well as
normal gameplay to establish the intended viewport-relative contract.

### P2: texture DC access leaks surface references and ignores acquisition failure

get_texture_dc:2970 and release_texture_dc:2992 call GetSurfaceLevel without
checking its HRESULT and never release the acquired surface pointer. Both success
and failure paths need balanced references. Failure can also leave a pointer
unsafe to dereference. Source consumers exist in Sector.cpp and SysMap.cpp
(including terrain/unit/icon drawing), so this is not merely an unused API stub.
Runtime frequency and total retained resources remain unmeasured.

### P2: startup and state API error propagation is incomplete

System.cpp:2160 records create_buffers's result but sets g_bHasBuffers=1 and
continues into batch/sampler initialization without checking success.
Backend sampler setters/getters at 2204/2210 ignore device HRESULTs and return
GR_OK. get_texture_interface:2943 is a commented-out implementation returning
GR_OK without initializing the output; its required consumers are still unproven.
These should be treated as separate failure-contract repairs, not blanket render
defaults or reasons to change authored asset formats.

## Existing confirmed issue kept separate

HQ preview: valid MeshArch Engine index 250, stale platform archIndex -1,
unchecked preview creation and null dereference in manual material refresh.
See HQ_PREVIEW_CRASH_2026-09-15.md. Nothing above supersedes that dump diagnosis.

## Next audit gates

1. Finish resource ownership map: texture, vertex/index buffers, effects, state
   blocks, device recreation and shutdown. Check capabilities advertised versus
   implemented methods. Separate current code from disabled legacy bodies.
2. Trace direct PIPE changes against ZBatcher and backend cache invalidation;
   compare shader/fixed-function transitions and material flags with retail.
3. Inspect texture format/mip/upload contracts and vertex layouts with minimal
   tests; distinguish native D3D9Ex behavior from DXVK behavior.
4. Design bounded failure-injection tests outside gameplay for failed device
   creation, failed allocation/lock, failed Present, repeated recreation and reset.
5. Implement only reviewed narrow fixes; full Debug/Final rebuild and verified
   deployment, then mission/skirmish, repeated load/unload and mode-switch tests.

No honest completion percentage can be derived from working draw calls or TODO
counts. API conversion is operational; behavioral/lifecycle fidelity remains open.
