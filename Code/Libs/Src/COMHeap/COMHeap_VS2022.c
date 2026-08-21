/*
 * COMHeap_VS2022.c — VS2022 CRT compatibility shim for COMHeap.
 *
 * The VS2022 CRT calls malloc/free during DLL CRT initialization (via
 * __acrt_initialize_multibyte and related functions) before DACOM has had a
 * chance to call __heap_init and set _HEAP.  COMHeap.asm intercepts these
 * calls but historically crashed because _HEAP was NULL.
 *
 * Fix: provide a private early heap (g_early_heap, created via HeapCreate on
 * first use) for all allocations that arrive before _HEAP is set.  When _HEAP
 * is subsequently set by DACOM, free/realloc calls use is_early_heap_block()
 * to route early-heap blocks back to g_early_heap rather than the DACOM heap,
 * avoiding the STATUS_HEAP_CORRUPTION that results from freeing a block through
 * the wrong heap.
 *
 * Block identification — inline header tag:
 *   Every early-heap allocation stores EARLY_HEAP_MAGIC in the 4 bytes
 *   immediately before the pointer returned to the caller.  is_early_heap_block()
 *   reads those 4 bytes and checks the magic — no HeapValidate() call, no
 *   "Invalid address specified to RtlValidateHeap" debug output, no debugger
 *   breaks caused by probing a foreign heap.
 *
 * Note: the .CRT$XIB pre-init hook (used in older versions of this shim) is
 * NOT used here.  Trim.dll, Mission.dll, ZBatcher.dll and others are loaded
 * dynamically by DACOM's plugin system, so DACOM (and therefore HEAP_Acquire)
 * is not available when their DLL_PROCESS_ATTACH fires.  Calling __heap_init
 * at that point would call through an uninitialized IAT entry and crash.
 * The early-heap approach handles pre-DACOM allocations safely without needing
 * DACOM to be present at CRT init time.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* -------------------------------------------------------------------------
 * Pre-DACOM fallback heap.
 *
 * Every allocation carries a 4-byte header immediately before the user
 * pointer:
 *
 *   [ EARLY_HEAP_MAGIC (4 bytes) ][ user data (size bytes) ]
 *                                  ^--- returned to caller
 *
 * is_early_heap_block(ptr) reads ptr[-4] and compares to EARLY_HEAP_MAGIC.
 * No Windows heap API is called, so there is no debugger-visible
 * "Invalid address specified to RtlValidateHeap" side-effect.
 * ---------------------------------------------------------------------- */

#define EARLY_HEAP_MAGIC  0xEA12EA12UL

typedef struct { DWORD magic; } early_hdr;

static HANDLE g_early_heap = NULL;

static HANDLE get_early_heap(void)
{
    /* Use the process heap so early-heap blocks are owned by a single heap
     * that all DLLs and DACOM's MSHeap::FreeMemory can HeapFree into. */
    g_early_heap = GetProcessHeap();
    return g_early_heap;
}

void * __stdcall early_heap_alloc(size_t size)
{
    early_hdr *h = (early_hdr *)HeapAlloc(get_early_heap(), 0,
                                           sizeof(early_hdr) + size);
    if (!h) return NULL;
    h->magic = EARLY_HEAP_MAGIC;
    return h + 1;
}

void * __stdcall early_heap_calloc(size_t count, size_t size)
{
    early_hdr *h = (early_hdr *)HeapAlloc(get_early_heap(), HEAP_ZERO_MEMORY,
                                           sizeof(early_hdr) + count * size);
    if (!h) return NULL;
    h->magic = EARLY_HEAP_MAGIC;   /* set after zero-fill */
    return h + 1;
}

void * __stdcall early_heap_realloc(void *ptr, size_t size)
{
    early_hdr *h;
    early_hdr *nh;
    if (!ptr)
        return early_heap_alloc(size);
    h  = ((early_hdr *)ptr) - 1;
    nh = (early_hdr *)HeapReAlloc(get_early_heap(), 0, h,
                                   sizeof(early_hdr) + size);
    if (!nh) return NULL;
    nh->magic = EARLY_HEAP_MAGIC;
    return nh + 1;
}

/*
 * Returns non-zero if ptr was allocated from the early heap.
 * Reads the 4-byte magic stored immediately before the user pointer.
 * O(1), no Windows heap API, no debugger-visible side effects.
 */
int __stdcall is_early_heap_block(void *ptr)
{
    if (!ptr) return 0;
    /* ptr-4 may be unmapped if the UCRT allocated the block via HeapAlloc
     * without going through our malloc interception.  Use SEH to avoid the
     * AV.  Return 1 on fault so the caller routes to early_heap_free, which
     * uses HeapFree and fails gracefully on a wrong-heap pointer. */
    __try {
        return (((early_hdr *)ptr) - 1)->magic == EARLY_HEAP_MAGIC;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return 1;
    }
}

void __stdcall early_heap_free(void *ptr)
{
    early_hdr *h;
    if (!ptr || !g_early_heap) return;
    h = ((early_hdr *)ptr) - 1;
    /* The header write may fault if the block was not allocated through us
     * (ptr-4 unmapped).  In that case skip the free — small leak is safe. */
    __try {
        h->magic = 0;   /* poison before free to catch double-free */
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return;
    }
    HeapFree(g_early_heap, 0, h);
}

size_t __stdcall early_heap_msize(void *ptr)
{
    SIZE_T s;
    early_hdr *h;
    if (!ptr || !g_early_heap) return 0;
    h = ((early_hdr *)ptr) - 1;
    s = HeapSize(g_early_heap, 0, h);
    return (s == (SIZE_T)-1) ? 0 : (size_t)(s - sizeof(early_hdr));
}
