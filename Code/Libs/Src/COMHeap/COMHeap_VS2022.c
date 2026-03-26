/*
 * COMHeap_VS2022.c — VS2022 CRT compatibility shim for COMHeap.
 *
 * Problem: VS2003-era CRT called _heap_init() directly during startup, which
 * let COMHeap.asm hook it to call HEAP_Acquire() and set _HEAP.  VS2022's CRT
 * no longer calls _heap_init(); instead it uses __acrt_initialize_heap() and
 * then immediately calls __acrt_initialize_multibyte(), which calls malloc().
 * COMHeap intercepts malloc but _HEAP is still NULL → access violation.
 *
 * Fix: place a pointer to __heap_init in the .CRT$XIB segment.  The VS2022
 * CRT iterates .CRT$XIA … .CRT$XIZ and calls every non-NULL function pointer
 * it finds there.  .CRT$XIB fires before .CRT$XIC (where locale/multibyte
 * init lives), so _HEAP is set before any CRT subsystem calls malloc.
 *
 * DACOM.dll is an implicit dependency of every module that links COMHeap.lib
 * (via the EXTRN __imp__HEAP_Acquire in COMHeap.asm), so the loader always
 * initialises DACOM before running this module's CRT init — HEAP_Acquire()
 * will therefore return a valid pointer when __heap_init fires here.
 */

#pragma warning(disable: 4152) /* non-standard function/object pointer conversion */

extern int __cdecl __heap_init(void);

typedef void (__cdecl *_PVFV)(void);

#pragma section(".CRT$XIB", long, read)
__declspec(allocate(".CRT$XIB")) _PVFV _comheap_vs2022_init = (_PVFV)__heap_init;
