//--------------------------------------------------------------------------//
//                                                                          //
//                               MSHeap.cpp                                 //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Libs/dev/Src/DACOM/MSHeap.cpp 4     3/21/00 4:30p Pbleisch $

*/
//--------------------------------------------------------------------------//


#include <windows.h>

#include "BaseHeap.h"
#include "FDump.h"
#include "TComponent.h"
#include "Malloc.h"

#pragma warning (disable : 4100)		// formal parameter unused

/* Magic written by COMHeap_VS2022.c before every early-heap user pointer.
 * Old COMHeap.asm (in pre-built DLLs) routes all frees through
 * IHeap::FreeMemory without checking this tag.  We detect it here and
 * route to HeapFree(GetProcessHeap()) so the right heap is used. */
#define EARLY_HEAP_MAGIC  0xEA12EA12UL

//--------------------------------------------------------------------------//
//-------------------------BaseHeap Class static data-----------------------//
//--------------------------------------------------------------------------//

static char interface_name[] = "IHeap";

IHeap * HEAP;
extern HINSTANCE hInstance;
IHeap * g_pMSHeap;

int __cdecl STANDARD_DUMP (ErrorCode code, const C8 *fmt, ...);

//--------------------------------------------------------------------------//
//
struct MSHeap : public IHeap
{
	//
	// interface mapping
	//
	BEGIN_DACOM_MAP_INBOUND(MSHeap)
	DACOM_INTERFACE_ENTRY(IHeap)
	DACOM_INTERFACE_ENTRY2(IID_IHeap,IHeap)
	END_DACOM_MAP()

	DA_ERROR_HANDLER		pErrorHandler;

	// *** IDAComponent methods ***
	
	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance);

   // *** IHeap methods ***

	DEFMETHOD_(void *,AllocateMemory) (U32 numBytes, const C8 *msg);

	DEFMETHOD_(void *,ClearAllocateMemory) (U32 numBytes, const C8 *msg, U8 initChar);
	
	DEFMETHOD_(void *,ReallocateMemory) (void *prevBlock, U32 newSize, const C8 *msg);

	DEFMETHOD_(BOOL32,FreeMemory) (void *allocatedBlock);

	DEFMETHOD_(BOOL32,EnumerateBlocks) (IHEAP_ENUM_PROC proc, void *context=0);

	DEFMETHOD_(U32,GetBlockSize) (void *allocatedBlock);

	DEFMETHOD_(const C8 *,GetBlockMessage) (void *allocatedBlock);

	DEFMETHOD_(BOOL32,DidAlloc) (void *allocatedBlock);
	
	DEFMETHOD_(U32,GetAvailableMemory) (void);

	DEFMETHOD_(U32,GetLargestBlock) (void);

	DEFMETHOD_(U32,GetHeapSize) (void);		// original alloc - overhead

    DEFMETHOD_(DA_ERROR_HANDLER,SetErrorHandler) (DA_ERROR_HANDLER proc);

	DEFMETHOD_(DA_ERROR_HANDLER,GetErrorHandler) (void);

	DEFMETHOD_(BOOL32,SetBlockOwner) (void *allocatedBlock, U32 caller);

	DEFMETHOD_(U32,GetBlockOwner) (void *allocatedBlock);

	DEFMETHOD_(BOOL32,SetBlockMessage) (void *allocatedBlock, const C8 *msg);

	DEFMETHOD_(void,HeapMinimize) (void);		// return unused memory to the OS

	DEFMETHOD_(U32,GetHeapFlags) (void);		// return flags used on last heap creation

	// *** MSHeap methods ***
	virtual void * __stdcall malloc_pass_through (const C8 * msg);
	virtual void * __stdcall realloc_pass_through (const C8 * msg);
	virtual void * __stdcall calloc_pass_through (const C8 * msg);
};
//--------------------------------------------------------------------------//
//----------------------------MSHeap Class Methods--------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
GENRESULT MSHeap::CreateInstance (DACOMDESC *descriptor, void **instance)
{
	*instance = 0;
	return GR_GENERIC;
}
//--------------------------------------------------------------------------//
//
void * MSHeap::AllocateMemory (U32 numBytes, const char *msg)
{
	return malloc(numBytes);
}
//--------------------------------------------------------------------------//
//
void * MSHeap::ClearAllocateMemory (U32 numBytes, const char *msg, U8 initChar)
{
	void * result = malloc(numBytes);
	if (result)
		memset(result, initChar, numBytes);
	return result;
}
//--------------------------------------------------------------------------//
//
void * MSHeap::ReallocateMemory (void *allocatedBlock, U32 newSize, const char *msg)
{
	if (allocatedBlock) {
		DWORD *tag = (DWORD *)allocatedBlock - 1;
		if (*tag == EARLY_HEAP_MAGIC &&
		    HeapSize(GetProcessHeap(), 0, tag) != (SIZE_T)-1) {
			DWORD *nh = (DWORD *)HeapReAlloc(GetProcessHeap(), 0, tag,
			                                  sizeof(DWORD) + newSize);
			if (!nh) return NULL;
			*nh = EARLY_HEAP_MAGIC;
			return nh + 1;
		}
	}
	return realloc(allocatedBlock, newSize);
}
//--------------------------------------------------------------------------//
//
BOOL32 MSHeap::FreeMemory (void *allocatedBlock)
{
	/* Windows permanently reserves the first 64 KB of virtual address space.
	 * A pointer below 0x10000 is never a valid heap allocation — it is an
	 * integer, offset, or sentinel value cast to pointer.  The original CRT's
	 * HeapFree simply returned FALSE for such values; we do the same. */
	if ((ULONG_PTR)allocatedBlock >= 0x10000) {
		DWORD *tag = (DWORD *)allocatedBlock - 1;
		/* Route early-heap blocks (pre-DACOM allocations with magic header) back to
		 * GetProcessHeap().  HeapSize guards against false positives from foreign
		 * pointers whose preceding 4 bytes coincidentally equal EARLY_HEAP_MAGIC;
		 * it returns (SIZE_T)-1 for any address not at the start of a valid block. */
		if (*tag == EARLY_HEAP_MAGIC &&
		    HeapSize(GetProcessHeap(), 0, tag) != (SIZE_T)-1) {
			*tag = 0;
			HeapFree(GetProcessHeap(), 0, tag);
			return 1;
		}
		/* If the block is on GetProcessHeap() (e.g. from HeapAlloc or a foreign
		 * subsystem like D3D/NVIDIA), free it there.
		 *
		 * HeapSize(GetProcessHeap(), 0, ptr) can crash with STATUS_ACCESS_VIOLATION
		 * (0xC0000005) when ptr is a Debug CRT user pointer: the Debug CRT allocates
		 * the full block (header + user data) on GetProcessHeap(), so ptr is an
		 * *interior* pointer to a valid heap block.  RtlSizeHeap interprets the bytes
		 * at ptr as a block header, reads a NULL forward-link from offset +0x14, and
		 * faults dereferencing it.  Use SEH to catch this and fall through to the CRT
		 * free path. */
		{
			BOOL bIsProcessHeap = FALSE;
			__try {
				bIsProcessHeap = (HeapSize(GetProcessHeap(), 0, allocatedBlock) != (SIZE_T)-1);
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				bIsProcessHeap = FALSE;
			}
			if (bIsProcessHeap) {
				HeapFree(GetProcessHeap(), 0, allocatedBlock);
				return 1;
			}
		}
		/* Block is not on GetProcessHeap() as a top-level block.  Try the CRT heap.
		 * Same SEH guard: _msize(userPtr) also calls HeapSize internally and can
		 * crash for the same reason if the CRT and process heaps share storage. */
		{
			BOOL bIsCRT = FALSE;
			__try {
				bIsCRT = (_msize(allocatedBlock) != (size_t)-1);
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				bIsCRT = FALSE;
			}
			if (bIsCRT)
				::free(allocatedBlock);
		}
	}
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 MSHeap::EnumerateBlocks (IHEAP_ENUM_PROC proc, void *context)
{
	return (_heapchk() == _HEAPOK);
}
//--------------------------------------------------------------------------//
//
U32 MSHeap::GetBlockSize (void *allocatedBlock)
{
	/* _msize can crash for Debug CRT user pointers — same SEH guard as FreeMemory. */
	size_t sz = (size_t)-1;
	__try { sz = _msize(allocatedBlock); } __except (EXCEPTION_EXECUTE_HANDLER) {}
	return (sz != (size_t)-1) ? (U32)sz : 0;
}
//--------------------------------------------------------------------------//
//
const char * MSHeap::GetBlockMessage (void *allocatedBlock)
{
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL32 MSHeap::SetBlockMessage (void *allocatedBlock, const C8 *msg)
{
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL32 MSHeap::DidAlloc (void *allocatedBlock)
{
	return 1;
}
//--------------------------------------------------------------------------//
//
U32 MSHeap::GetAvailableMemory (void)
{
	return 0x7FFFFFFF;
}
//--------------------------------------------------------------------------//
//
U32 MSHeap::GetLargestBlock (void)
{
	return 0x7FFFFFFF;
}
//--------------------------------------------------------------------------//
//
U32 MSHeap::GetHeapSize (void)
{
	return 0x7FFFFFFF;
}
//--------------------------------------------------------------------------//
//
DA_ERROR_HANDLER MSHeap::SetErrorHandler (DA_ERROR_HANDLER proc)
{
	DA_ERROR_HANDLER result = pErrorHandler;

	pErrorHandler = proc;
 	
	return result;
}
//--------------------------------------------------------------------------//
//
DA_ERROR_HANDLER MSHeap::GetErrorHandler (void)
{
	return pErrorHandler;
}
//--------------------------------------------------------------------------//
//
BOOL32 MSHeap::SetBlockOwner (void *allocatedBlock, U32 owner)
{
	return 0;
}
//--------------------------------------------------------------------------//
//
U32 MSHeap::GetBlockOwner (void *allocatedBlock)
{
	return 0;
}
//--------------------------------------------------------------------------//
//
void MSHeap::HeapMinimize (void)
{
	_heapmin();
}
//--------------------------------------------------------------------------//
//
U32 MSHeap::GetHeapFlags (void)
{
	return DAHEAPFLAG_GROWHEAP|DAHEAPFLAG_NOMSGS|DAHEAPFLAG_MULTITHREADED;
}
/* COMHeap.asm calls malloc_pass_through/realloc_pass_through/calloc_pass_through with
 * the actual size/ptr arguments sitting above msg on the caller's stack.
 * Stack layout at entry (after function prologue sets up EBP):
 *   &msg         == [ebp+12]  -- msg parameter
 *   &msg + 1     == [ebp+16]  -- return address to original caller (skip)
 *   &msg + 2     == [ebp+20]  -- first arg to the original CRT function:
 *                                  malloc:  size
 *                                  realloc: old ptr
 *                                  calloc:  count
 *   &msg + 3     == [ebp+24]  -- second arg (realloc: new_size; calloc: elem_size)
 * DACOM.dll does not link COMHeap.lib, so malloc/realloc/calloc here go directly
 * to the static CRT without interception. */
//--------------------------------------------------------------------------//
//
void * MSHeap::malloc_pass_through (const C8 * msg)
{
	U32 size = *((U32*)&msg + 2);
	return malloc(size);
}
//--------------------------------------------------------------------------//
//
void * MSHeap::realloc_pass_through (const C8 * msg)
{
	void *ptr  = *(void**)((U32 *)&msg + 2);  // first realloc arg = old ptr
	U32   size = *((U32 *)&msg + 3);           // second realloc arg = new size
	if (ptr) {
		DWORD *tag = (DWORD *)ptr - 1;
		if (*tag == EARLY_HEAP_MAGIC &&
		    HeapSize(GetProcessHeap(), 0, tag) != (SIZE_T)-1) {
			DWORD *nh = (DWORD *)HeapReAlloc(GetProcessHeap(), 0, tag,
			                                  sizeof(DWORD) + size);
			if (!nh) return NULL;
			*nh = EARLY_HEAP_MAGIC;
			return nh + 1;
		}
	}
	return realloc(ptr, size);
}
//--------------------------------------------------------------------------//
//
void * MSHeap::calloc_pass_through (const C8 * msg)
{
	U32 count     = *((U32*)&msg + 2);
	U32 elem_size = *((U32*)&msg + 3);
	return calloc(count, elem_size);
}
//--------------------------------------------------------------------------//
//
BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			hInstance = hinstDLL;

			HEAP = g_pMSHeap = new DAComponent<MSHeap>;
			// Setup the standard error report function.
			FDUMP = STANDARD_DUMP;
		}
		break;

		case DLL_PROCESS_DETACH:
			delete g_pMSHeap;
			g_pMSHeap = HEAP = 0;
		break;
	}

   return TRUE;
}

extern "C" 
{
//--------------------------------------------------------------------------//
//
DXDEF IHeap * __cdecl HEAP_Acquire(void)
{
	if (HEAP)
		HEAP->AddRef();
	return HEAP;
}


}
//--------------------------------------------------------------------------//
//---------------------------END MSHeap.cpp-------------------------------//
//--------------------------------------------------------------------------//
