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
	return realloc(allocatedBlock, newSize);
}
//--------------------------------------------------------------------------//
//
BOOL32 MSHeap::FreeMemory (void *allocatedBlock)
{
	::free(allocatedBlock);
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
	return _msize(allocatedBlock);
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
/*
//--------------------------------------------------------------------------//
//
void * MSHeap::malloc_pass_through (const C8 * msg)
{
	return malloc(12);
}
//--------------------------------------------------------------------------//
//
void * MSHeap::realloc_pass_through (const C8 * msg)
{
	return realloc(0, 12);
}
//--------------------------------------------------------------------------//
//
void * MSHeap::calloc_pass_through (const C8 * msg)
{
	return calloc(12, 1);
}
*/
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
