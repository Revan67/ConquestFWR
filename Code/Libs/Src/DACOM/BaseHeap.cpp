//--------------------------------------------------------------------------//
//                                                                          //
//                             BaseHeap.cpp                                 //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Libs/Dev/Src/DACOM/BaseHeap.cpp 19    3/20/00 10:11a Jasony $

		                       Base Heap Object

*/
//--------------------------------------------------------------------------//


#include <windows.h>

#include "BaseHeap.h"
#include "FDump.h"

#pragma warning (disable : 4100)		// formal parameter unused

//--------------------------------------------------------------------------//
//-------------------------BaseHeap Class static data-----------------------//
//--------------------------------------------------------------------------//

static char interface_name[] = "IHeap";

extern IHeap * HEAP;
extern IHeap * g_pMSHeap;
extern HINSTANCE hInstance;

static BaseHeap g_heap;

#define pHeapList pNext

//

int __cdecl STANDARD_DUMP (ErrorCode code, const C8 *fmt, ...);

//
//--------------------------------------------------------------------------//
//----------------------------Heap Class Methods----------------------------//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//
GENRESULT BaseHeap::QueryInterface (const C8 *interface_name, void **instance)
{
	ASSERT(instance);

	*instance = 0;
	return GR_INTERFACE_UNSUPPORTED;
}
//--------------------------------------------------------------------------//
//
GENRESULT BaseHeap::CreateInstance (DACOMDESC *descriptor, void **instance)
{
	ASSERT(instance);
	ASSERT(descriptor);

	DAHEAPDESC *lpInfo = (DAHEAPDESC *) descriptor;
	GENRESULT result = GR_OK;
	HeapInstance *pNewHeap = NULL;
	HeapInstance stheap;		// single threaded heap
	MTHeapInstance mtheap;		// multi threaded heap
	HeapInstance & heap = (lpInfo->flags & DAHEAPFLAG_MULTITHREADED) ? *static_cast<HeapInstance *>(&mtheap) : stheap;

	//
	// If unsupported interface requested, fail call
	//

	if ((lpInfo->size != sizeof(*lpInfo)) || 
		strcmp(lpInfo->interface_name, interface_name))
	{
		result = GR_INTERFACE_UNSUPPORTED;
		goto Done;
	}



	heap.pErrorHandler = pErrorHandler;		// propogate error handler to new heap
	
	if ((result = heap.initHeap(lpInfo)) == GR_OK)
	{
		if ((pNewHeap = (HeapInstance *) heap.AllocateMemory(sizeof(HeapInstance), "Heap control block")) == 0)
		{
			result = GR_OUT_OF_MEMORY;
			goto Done;
		}
	
		memcpy(pNewHeap, &heap, sizeof(HeapInstance));
		if (pNewHeap->dwFlags & DAHEAPFLAG_MULTITHREADED)
		{
			// move criticalsection to the new location
			DeleteCriticalSection(&heap.criticalSection);
			InitializeCriticalSection(&pNewHeap->criticalSection);	
		}
		
		// successfully created the heap, now add it to a list
		if (((dwFlags ^ lpInfo->flags) & DAHEAPFLAG_PRIVATE) == 0)
		{
			// adding to the size of an existing heap
			AddToList(pNewHeap);
			AddRef();
			pNewHeap = (HeapInstance *) this;
		}
		else
		if ((dwFlags & DAHEAPFLAG_PRIVATE) == 0)
		{
			// we are public, creating a private heap
			BaseHeap *pNewBase;

			if ((pNewBase = new BaseHeap) == 0)
			{
				pNewHeap->Release();
				pNewHeap = 0;
				result = GR_OUT_OF_MEMORY;
				goto Done;
			}

			pNewBase->pErrorHandler = pErrorHandler;
			pNewBase->dwFlags = lpInfo->flags;
			pNewBase->AddToList(pNewHeap);

			pNewHeap = (HeapInstance *) pNewBase;
		}
		else
		{
			// we are private, creating a public heap

			g_heap.AddToList(pNewHeap);
			g_heap.AddRef();
			pNewHeap = (HeapInstance *) &g_heap;
		}
	}

Done:
	*instance = pNewHeap;

	return result;
}
//--------------------------------------------------------------------------//
//
U32 BaseHeap::AddRef (void)
{
	dwRefs++;
	return dwRefs;
}
//--------------------------------------------------------------------------//
//
U32 BaseHeap::Release (void)
{
	if (dwFlags & DAHEAPFLAG_PRIVATE)
	{
		if (dwRefs > 0)
			dwRefs--;

		if (dwRefs == 0)
		{
			if (pHeapList)
				pHeapList->Release();
			pHeapList=0;
			delete this;
			return 0;
		}
	}
	else
	{
		if (dwRefs > 1)
			dwRefs--;
	}

	return dwRefs;
}
//--------------------------------------------------------------------------//
//
BOOL BaseHeap::AddToList (struct HeapInstance *pHeap)
{
	ASSERT(pHeap);

	BOOL result=0;

	if (pHeapList==0)
	{
		if (this == &g_heap)
			HEAP = this;
		pHeapList = pHeap;
	}
	else
	{
		pHeap->pNext = pHeapList;
		pHeapList = pHeap;
	}

	pHeap->SetErrorHandler(pErrorHandler);		// propogate error handler to new heap
	
	return result;
}
//--------------------------------------------------------------------------//
//
inline struct HeapInstance * BaseHeap::FindTheHeap (void *baseAddress)
{
	HeapInstance *pList;

	for (pList=pHeapList; pList; pList=pList->pNext)
	{
		// single comparison range check
		DWORD pos = (DWORD)baseAddress - (DWORD)pList->pHeapBase;
		if (pos < pList->dwHeapSize)
			break;
	}
	return pList;
}
//--------------------------------------------------------------------------//
//
void * BaseHeap::AllocateMemory (U32 numBytes, const char *msg)
{
	void * result=0;
	HeapInstance *pList;
	
	if (numBytes == 0)
	{
		doError(DAHEAP_ALLOC_ZERO);
		goto Done;
	}
	if ((long)numBytes < 0)
	{
		doError(DAHEAP_OVERFLOW, numBytes);
		goto Done;
	}

	if ((pList = pHeapList) != 0)
	do
	{
		if ((result = pList->AllocateMemory(numBytes, msg)) != 0)
			goto Done;
		pList = pList->pNext;

	} while (pList != 0);

	if (result == 0)
	{
		if (pHeapList && (pHeapList->dwFlags & DAHEAPFLAG_GROWHEAP))
		{
			DAHEAPDESC desc(interface_name);
			BaseHeap * pHeap;

			desc.growSize = pHeapList->dwGrowSize;
			desc.heapSize = __max(desc.growSize, numBytes + sizeof(FREE_BLOCK)*4 + sizeof(HeapInstance));
			desc.flags = pHeapList->dwFlags;

			if (CreateInstance(&desc, (void **)&pHeap) == GR_OK)
			{
				result = pHeap->pHeapList->AllocateMemory(numBytes, msg);
				pHeap->Release();		// remove the extra reference
				if (result != 0)
				{
					if( (pHeapList->dwFlags & DAHEAPFLAG_NOHEAPEXPANDMSG) == 0 )
					{
						doError(DAHEAP_HEAP_EXPANDING, desc.heapSize);	  // actually just a warning
					}
					goto Done;
				}
			}
		}
		if (doError(DAHEAP_OUT_OF_MEMORY, numBytes, GetLargestBlock()))
			return AllocateMemory(numBytes, msg);
	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
void * BaseHeap::ClearAllocateMemory (U32 numBytes, const char *msg, U8 initChar)
{
	void * result;

	if ((result = AllocateMemory(numBytes, msg)) != 0)
		memset(result, initChar, numBytes);

	return result;
}
//--------------------------------------------------------------------------//
//
void * BaseHeap::ReallocateMemory (void *allocatedBlock, U32 newSize, const char *msg)
{
	if (allocatedBlock == 0)
		return AllocateMemory(newSize, msg);

	HeapInstance *pHeap = FindTheHeap(allocatedBlock);
	void * result;

	if (pHeap)
	{
		if ((result = pHeap->ReallocateMemory(allocatedBlock, newSize, msg)) == 0)
		{
			BASE_BLOCK *pBlock;

			if (newSize == 0)
				goto Done;		// already handled
			if ((pBlock = pHeap->verifyBlock(allocatedBlock)) == 0)
			{
				result = 0;
				goto Done;
			}
			if ((result = AllocateMemory(newSize, msg)) != 0)
			{
				memcpy(result, allocatedBlock, __min((pBlock->dwSize&~1), newSize));
				pHeap->free((FREE_BLOCK *)pBlock);
			}
		}
	}
	else
	{
		result = g_pMSHeap->ReallocateMemory(allocatedBlock, newSize, msg);
	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 BaseHeap::FreeMemory (void *allocatedBlock)
{
	BOOL32 result;
	
	if ((result = U32(allocatedBlock)) != 0)
	{
		BaseHeap *pHeap = FindTheHeap(allocatedBlock);

		if (pHeap)
			result = pHeap->FreeMemory(allocatedBlock);
		else
			result = g_pMSHeap->FreeMemory(allocatedBlock);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 BaseHeap::EnumerateBlocks (IHEAP_ENUM_PROC proc, void *context)
{
	HeapInstance *pHeap = pHeapList;
	BOOL32 result=1;

	while (pHeap && result)
	{
		result = pHeap->EnumerateBlocks(proc, context);
		pHeap = pHeap->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
U32 BaseHeap::GetBlockSize (void *allocatedBlock)
{
	HeapInstance *pHeap = FindTheHeap(allocatedBlock);
	U32 result;

	if (pHeap)
		result = pHeap->GetBlockSize(allocatedBlock);
	else
		result = g_pMSHeap->GetBlockSize(allocatedBlock);

	return result;
}
//--------------------------------------------------------------------------//
//
const char * BaseHeap::GetBlockMessage (void *allocatedBlock)
{
	BaseHeap *pHeap = FindTheHeap(allocatedBlock);
	const char *result;

	if (pHeap)
		result = pHeap->GetBlockMessage(allocatedBlock);
	else
		result = g_pMSHeap->GetBlockMessage(allocatedBlock);

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 BaseHeap::SetBlockMessage (void *allocatedBlock, const C8 *msg)
{
	BaseHeap *pHeap = FindTheHeap(allocatedBlock);
	BOOL32 result;

	if (pHeap)
		result = pHeap->SetBlockMessage(allocatedBlock, msg);
	else
		result = g_pMSHeap->SetBlockMessage(allocatedBlock, msg);

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 BaseHeap::DidAlloc (void *allocatedBlock)
{
	return (FindTheHeap(allocatedBlock) != 0);
}
//--------------------------------------------------------------------------//
//
U32 BaseHeap::GetAvailableMemory (void)
{
	U32 result=0;
	HeapInstance *pList;
	
	if ((pList = pHeapList) != 0)
	do
	{
		result += pList->GetAvailableMemory();
		pList = pList->pNext;

	} while (pList != 0);

	return result;
}
//--------------------------------------------------------------------------//
//
U32 BaseHeap::GetLargestBlock (void)
{
	U32 result=0, tmp;
	HeapInstance *pList;
	
	if ((pList = pHeapList) != 0)
	do
	{
		tmp = pList->GetLargestBlock();
		result = __max(result, tmp);

		pList = pList->pNext;

	} while (pList != 0);

	return result;
}
//--------------------------------------------------------------------------//
//
U32 BaseHeap::GetHeapSize (void)
{
	U32 result=0;
	HeapInstance *pList;
	
	if ((pList = pHeapList) != 0)
	do
	{
		result += pList->GetHeapSize();
		pList = pList->pNext;

	} while (pList != 0);

	return result;
}
//--------------------------------------------------------------------------//
//
DA_ERROR_HANDLER BaseHeap::SetErrorHandler (DA_ERROR_HANDLER proc)
{
	DA_ERROR_HANDLER result = pErrorHandler;
	HeapInstance *pList;

	pErrorHandler = proc;
 	
	if ((pList = pHeapList) != 0)
	do
	{
		pList->SetErrorHandler(proc);
		pList = pList->pNext;

	} while (pList != 0);
	
	return result;
}
//--------------------------------------------------------------------------//
//
DA_ERROR_HANDLER BaseHeap::GetErrorHandler (void)
{
	return pErrorHandler;
}
//--------------------------------------------------------------------------//
//
BOOL32 BaseHeap::doError (DWORD dwErrorNum, DWORD dwNum1, DWORD dwNum2)
{
	char buffer[128];
	BOOL32 result = 0;

	if (pErrorHandler)
	{
		LoadString(hInstance, dwErrorNum, buffer, sizeof(buffer));
 		if (dwErrorNum == DAHEAP_VALLOC_FAILED)
		{
			result = pErrorHandler(ErrorCode(ERR_MEMORY, SEV_WARNING),
								   buffer,
								   this,
								   dwErrorNum,
								   dwNum1,
								   dwNum2);
		}
		else
 		if (dwErrorNum == DAHEAP_HEAP_EXPANDING || dwErrorNum == DAHEAP_ALLOC_ZERO)
		{
			result = pErrorHandler(ErrorCode(ERR_MEMORY, SEV_TRACE_2),
								   buffer,
								   this,
								   dwErrorNum,
								   dwNum1,
								   dwNum2);
		}
		else
		{
			result = pErrorHandler(ErrorCode(ERR_MEMORY, SEV_FATAL),
								   buffer,
								   this,
								   dwErrorNum,
								   dwNum1,
								   dwNum2);
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 BaseHeap::SetBlockOwner (void *allocatedBlock, U32 owner)
{
	BOOL32 result;
	BaseHeap *pHeap = FindTheHeap(allocatedBlock);

	if (pHeap)
		result = pHeap->SetBlockOwner(allocatedBlock, owner);
	else
		result = g_pMSHeap->SetBlockOwner(allocatedBlock, owner);

	return result;
}
//--------------------------------------------------------------------------//
//
U32 BaseHeap::GetBlockOwner (void *allocatedBlock)
{
	U32 result;
	BaseHeap *pHeap = FindTheHeap(allocatedBlock);

	if (pHeap)
		result = pHeap->GetBlockOwner(allocatedBlock);
	else
		result = g_pMSHeap->GetBlockOwner(allocatedBlock);

	return result;
}
//--------------------------------------------------------------------------//
//
void BaseHeap::HeapMinimize (void)
{
	HeapInstance *pList=pHeapList, *prev=0;

	if (pList)
	while (pList->pNext)		// don't deallocate the initial heap
	{
		// if only allocated block is the control block, free it.
		if (((BASE_BLOCK *)pList->pHeapBase)->isAllocated() == 0 &&
			(((DWORD)((BASE_BLOCK *)pList->pHeapBase)->getLower()) + pList->dwBaseBlockSize) == (DWORD) pList)
		{
			HeapInstance *pTmp;

			if (prev)
				pTmp = prev->pNext = pList->pNext;
			else
				pTmp = pHeapList = pList->pNext;

			if (pList->dwFlags & DAHEAPFLAG_MULTITHREADED)
				DeleteCriticalSection(&pList->criticalSection);		

			VirtualFree(pList->pHeapBase, 0, MEM_RELEASE);			// actually frees the instance too

			pList = pTmp;
		}
		else
		{
			prev = pList;
			pList = pList->pNext;
		}
	}
}
//--------------------------------------------------------------------------//
//
U32 BaseHeap::GetHeapFlags (void)
{
	return (pHeapList) ? pHeapList->dwFlags : dwFlags;
}
/*
//--------------------------------------------------------------------------//
//
void * BaseHeap::malloc_pass_through (const C8 * msg)
{
	void * result;
	
	if ((result = AllocateMemory(12, msg)) != 0 && (dwFlags & DAHEAPFLAG_NOMSGS)==0)
		SetBlockOwner(result, 20);
	
	return result;
}
//--------------------------------------------------------------------------//
//
void * BaseHeap::realloc_pass_through (const C8 * msg)
{
	void * result;
	
	if ((result = ReallocateMemory(0, 12, msg)) != 0 && (dwFlags & DAHEAPFLAG_NOMSGS)==0)
		SetBlockOwner(result, 20);
	
	return result;
}
//--------------------------------------------------------------------------//
//
void * BaseHeap::calloc_pass_through (const C8 * msg)
{
	void * result;
	
	if ((result = ClearAllocateMemory(12, msg, 10)) != 0 && (dwFlags & DAHEAPFLAG_NOMSGS)==0)
		SetBlockOwner(result, 20);
	
	return result;
}
*/
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
void RegisterHeap (ICOManager *pManager)
{
	pManager->RegisterComponent(&g_heap, interface_name);
}
//--------------------------------------------------------------------------//
//---------------------------END BaseHeap.cpp-------------------------------//
//--------------------------------------------------------------------------//
