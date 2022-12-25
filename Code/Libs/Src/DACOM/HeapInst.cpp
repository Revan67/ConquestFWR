//--------------------------------------------------------------------------//
//                                                                          //
//                             HeapInst.cpp                                 //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Libs/Dev/Src/DACOM/HeapInst.cpp 10    10/22/99 12:06p Jasony $

                       Heap Instance, without messages

*/
//--------------------------------------------------------------------------//


#include <windows.h>

#include "BaseHeap.h"
#include "TComponent.h"
#include "fdump.h"

// NOTE: HeapInstances are now allocated from within their own heap,
//  so they should never "delete" themselves!

//--------------------------------------------------------------------------//
//-------------------------HeapInst Class static data-----------------------//
//--------------------------------------------------------------------------//

// fill memory with SNAN's
static void __stdcall snanfill (void * pMemory, U32 numDwords)
{
	ASSERT(pMemory);
	__asm {
		mov edi, DWORD PTR pMemory
		cld
		mov ecx, numDwords
		mov eax, 0FFA01006h
		rep stosd
	}
}

//--------------------------------------------------------------------------//
//----------------------------HeapInst Class Methods------------------------//
//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//
U32 HeapInstance::Release (void)
{
	if (dwRefs > 0)
		dwRefs--;

	if (dwRefs == 0)
	{
		if (pNext)
			pNext->Release();
		
		VirtualFree(pHeapBase, 0, MEM_RELEASE);
		//pHeapBase=0;	// we have allocated from within our own heap
		//delete this;
		return 0;
	}

	return dwRefs;
}
//--------------------------------------------------------------------------//
//
void * HeapInstance::AllocateMemory (U32 numBytes, const char *msg)
{
	BASE_BLOCK *result;

	if ((result = malloc(numBytes)) == 0)
		return result;

	if ((dwFlags & DAHEAPFLAG_NOMSGS) == 0)
	{
	  	result->setMsg(msg);
	  	result->setOwner(0);
	}
	return (void *) ((char *)result+dwBaseBlockSize);
}
//--------------------------------------------------------------------------//
//
void * HeapInstance::ReallocateMemory (void *allocatedBlock, U32 newSize, const char *msg)
{
	void * result;
	BASE_BLOCK *pBlock;

	if ((pBlock = verifyBlock(allocatedBlock)) == 0)
	{
		result = 0;
		goto Done;
	}

	if (newSize == 0)
	{
		free((FREE_BLOCK *)pBlock);
		result=0;
	}
	else
	if ((result = AllocateMemory(newSize, msg)) != 0)
	{
		memcpy(result, allocatedBlock, __min((pBlock->dwSize&~1), newSize));
		free((FREE_BLOCK *)pBlock);
	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 HeapInstance::FreeMemory (void *allocatedBlock)
{
	BOOL32 result;
	BASE_BLOCK *pBlock;

	if ((pBlock = verifyBlock(allocatedBlock)) == 0)
	{
		result = 0;
		goto Done;
	}

	result = free((FREE_BLOCK *)pBlock);

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 HeapInstance::EnumerateBlocks (IHEAP_ENUM_PROC proc, void *context)
{
	BOOL32 result=1;
	DWORD dwFlags;
	FREE_BLOCK *pBlock = (FREE_BLOCK *) pHeapBase;

	while (result && pBlock->dwSize != 1)
	{
		if ((dwFlags = pBlock->isAllocated()) == 0)
		{
			// check the free list (circular list)
			if (pBlock->getNext()->getPrev() != pBlock)
				dwFlags |= DAHEAPFLAG_CORRUPTED_BLOCK;
			if (pBlock->getPrev()->getNext() != pBlock)
				dwFlags |= DAHEAPFLAG_CORRUPTED_BLOCK;
		}

		// lower block check
		if (pBlock->getLower()->getUpper() != pBlock)
			dwFlags |= DAHEAPFLAG_CORRUPTED_BLOCK;
		// upper block check
		if (pBlock->getUpper() && pBlock->getUpper()->getLower() != pBlock)
			dwFlags |= DAHEAPFLAG_CORRUPTED_BLOCK;

		if (proc)
			result = proc(this, ((char *)pBlock)+dwBaseBlockSize, dwFlags, context);

		if (dwFlags & DAHEAPFLAG_CORRUPTED_BLOCK)
			result=0;
		else
			pBlock = (FREE_BLOCK *) pBlock->getLower();
	}
	
	return result;
}
//--------------------------------------------------------------------------//
//
U32 HeapInstance::GetBlockSize (void *allocatedBlock)
{
	U32 result;
	
	BASE_BLOCK *pBlock;

	if ((pBlock = verifyBlock(allocatedBlock)) == 0)
		result = 0;
	else
		result = (pBlock->dwSize - dwBaseBlockSize) & ~1;

	return result;
}
//--------------------------------------------------------------------------//
//
const char * HeapInstance::GetBlockMessage (void *allocatedBlock)
{
	BASE_BLOCK *pBlock;

	if ((pBlock = verifyBlock(allocatedBlock)) != 0)
	{
		if (pBlock->isAllocated() && (dwFlags & DAHEAPFLAG_NOMSGS) == 0)
			return pBlock->getMsg();
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL32 HeapInstance::SetBlockMessage (void *allocatedBlock, const C8 *msg)
{
	BASE_BLOCK *pBlock;

	if ((pBlock = verifyBlock(allocatedBlock)) != 0)
	{
		if (pBlock->isAllocated() && (dwFlags & DAHEAPFLAG_NOMSGS) == 0)
			pBlock->setMsg(msg);
		return 1;
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL32 HeapInstance::DidAlloc (void *allocatedBlock)
{
	if (((DWORD)(pHeapBase) > (DWORD)allocatedBlock) ||
			((DWORD)(pHeapBase) + dwHeapSize <= (DWORD)allocatedBlock))
	{
		return 0;
	}
	return 1;
}
//--------------------------------------------------------------------------//
//
U32 HeapInstance::GetAvailableMemory (void)
{
	U32 result=0;
	S32 loops=10000;
	FREE_BLOCK *pBlock = pFirstFreeBlock;
	FREE_BLOCK *pStart = pFirstFreeBlock;

	if (pBlock)
	do
	{
		pBlock = pBlock->getNext();
		result += pBlock->dwSize - dwBaseBlockSize;
		if (--loops <= 0)
			break;

	} while (pBlock != pStart);

	return result;
}
//--------------------------------------------------------------------------//
//
U32 HeapInstance::GetLargestBlock (void)
{
	U32 result=0;
	FREE_BLOCK *pBlock = pFirstFreeBlock;
	
	if (pBlock)
	{
		if ((dwFlags & DAHEAPFLAG_NOBESTFIT) == 0)
			result = pBlock->getPrev()->dwSize - dwBaseBlockSize;
		else
		{
			FREE_BLOCK *pStart = pFirstFreeBlock;
			S32 loops=10000;
			
			do
			{
				pBlock = pBlock->getNext();
				result = __max(result, pBlock->dwSize);
				if (--loops <= 0)
					break;

			} while (pBlock != pStart);

			result -= dwBaseBlockSize;
		}
	}
	
	return result;
}
//--------------------------------------------------------------------------//
//
U32 HeapInstance::GetHeapSize (void)
{
	return dwHeapSize - (dwBaseBlockSize*3) - sizeof(HeapInstance);
}
//--------------------------------------------------------------------------//
//
DA_ERROR_HANDLER HeapInstance::SetErrorHandler (DA_ERROR_HANDLER proc)
{
	DA_ERROR_HANDLER result = pErrorHandler;

	pErrorHandler = proc;
 	
	return result;
}
//--------------------------------------------------------------------------//
//
DA_ERROR_HANDLER HeapInstance::GetErrorHandler (void)
{
	return pErrorHandler;
}
//--------------------------------------------------------------------------//
//
GENRESULT HeapInstance::initHeap (DAHEAPDESC *lpDesc)
{
	BASE_BLOCK *pEnding;
	FREE_BLOCK *pStart;
	DWORD dwSize;

	ASSERT(lpDesc);

	dwSize = __max((sizeof(FREE_BLOCK)*4 + sizeof(HeapInstance)), lpDesc->heapSize);
	dwSize = (dwSize+0xFFF)&~0xFFF;

	if ((long)lpDesc->growSize <= 0)
	{
		if (lpDesc->flags & DAHEAPFLAG_GROWHEAP)
			return GR_INVALID_PARMS;
	}
	
	if ((pHeapBase = VirtualAlloc(0, dwSize, MEM_COMMIT, PAGE_READWRITE)) == 0)
	{
		doError(DAHEAP_VALLOC_FAILED, dwSize);
		return GR_OUT_OF_MEMORY;
	}

	dwHeapSize = dwSize;
	dwFlags = lpDesc->flags;
	dwBaseBlockSize = (dwFlags & DAHEAPFLAG_NOMSGS) ? 8 : 16;
	dwGrowSize = lpDesc->growSize;

	if (dwFlags & DAHEAPFLAG_MULTITHREADED)
		InitializeCriticalSection(&criticalSection);	

	pStart = (FREE_BLOCK *) pHeapBase;
	pEnding = (BASE_BLOCK *) (((char *)pHeapBase) + dwHeapSize - dwBaseBlockSize);

	//----------------------------------------
	// Initialize heap
	//----------------------------------------

	pStart->dwSize = (DWORD)pEnding - (DWORD)pStart;
	pStart->pUpper = 0;		// first block in list
	pStart->pPrev = 
	pStart->pNext = pStart;
	
	pFirstFreeBlock = pStart;

	pEnding->dwSize = 1;		// mark as the last block in the heap
	pEnding->pUpper = pStart;

	// initialize all memory to -1

	memset(pStart+1, -1, pStart->dwSize - sizeof(FREE_BLOCK));

	return GR_OK;
}
//--------------------------------------------------------------------------//
// reposition a block already in the free list
//
void HeapInstance::sort (FREE_BLOCK *pBlock)
{
	ASSERT(pBlock);

	FREE_BLOCK *pFirst = pFirstFreeBlock, *edi, *esi;
	DWORD dwSize = pBlock->dwSize;

	// see if we have any work to do at all

	edi = pBlock->getNext();
	if (edi == pBlock->getPrev())
	{
		// one or two members in the list
		if (edi == pBlock)
			return;	// only member of the list
		// assume we are the smaller block
		pFirstFreeBlock = pBlock;
		if (dwSize >= edi->dwSize)
			pFirstFreeBlock = edi;		// oops! 
		return;
	}

	// else see if we are not in the correct order

	if (dwSize <= edi->dwSize || edi == pFirst)
	{
		edi = pBlock->pPrev;
		if (dwSize >= edi->dwSize || pBlock == pFirst)
			return;
	}

	//  else we must unlink our block, saving a pointer to lower neighbor

	UNLINK(pBlock);

	pFirst = pFirstFreeBlock;

	if (pBlock->pPrev != edi)
	{
		// stop when guy to the right is higher than us, or is the FirstMemBlock
		do
		{
			esi = edi;
			edi = edi->pNext;
		} while (edi != pFirst && dwSize > edi->dwSize);
	}
	else
	{
		//	;stop when guy to the left is lower than us, or we stopped on the FirstMemBlock
		//@@Loop2:
		esi = edi;
		do
		{
			edi = esi;
			esi = esi->pPrev;
		} while (edi != pFirst && dwSize < esi->dwSize);
	}


	// @@FoundPlace:	; esi-> prev block, edi->next block, ebx -> us

	pBlock->pPrev = esi;
	pBlock->pNext = edi;
	esi->pNext = pBlock;
	edi->pPrev = pBlock;

	if (dwSize < pFirst->dwSize)		// are we now the smallest block of them all?
		pFirstFreeBlock = pBlock;
}
//--------------------------------------------------------------------------//
//
BASE_BLOCK * HeapInstance::malloc (DWORD dwNumBytes)
{
	FREE_BLOCK *pBlock, *pStart;
	DWORD dwRemainder;


	dwNumBytes = (dwNumBytes + dwBaseBlockSize + 7) & ~7;	// enforce quad-word alignment
	//	dwNumBytes = __max(sizeof(FREE_BLOCK), dwNumBytes);		// already ensured

	// search the free list for available block

	if ((pBlock = pFirstFreeBlock) == 0)
		goto Error;		// out of memory

	pStart = pBlock;	// mark beginning of list

	do
	{
		if ((long)(dwRemainder = pBlock->dwSize - dwNumBytes) >= 0)
			break;		// found a block
	} while ((pBlock = pBlock->getNext()) != pStart);

	if ((long)dwRemainder < 0)
		goto Error;		// out of memory


	if (dwRemainder < sizeof(FREE_BLOCK)*2)
	{
		// just unlink the whole block
		UNLINK(pBlock);
		pBlock->dwSize |= 1;	// mark as allocated
	}
	else
	{
		// update the lower block to point to the new block
		BASE_BLOCK *pTmp;

		pTmp = pBlock->getLower();
		pBlock->dwSize = dwRemainder;

		if ((dwFlags & DAHEAPFLAG_NOBESTFIT) == 0)
			sort(pBlock);

		pTmp->pUpper = pBlock = (FREE_BLOCK *) (((char *)pTmp->pUpper) + dwRemainder);

		// pBlock -> new block
		pBlock->dwSize = dwNumBytes + 1;		// mark allocated
		pBlock->pUpper = (BASE_BLOCK *) (((char *)pBlock) - dwRemainder);
	}
	
	return pBlock;
	
Error:
	return 0;
}
//--------------------------------------------------------------------------//
//
inline BOOL HeapInstance::mergeWithLower (BASE_BLOCK *pBlock)
{
	ASSERT(pBlock);

	FREE_BLOCK *pLower;
	DWORD dwSize;

	if (((dwSize = pBlock->dwSize) & 1) == 0)
	{
		doError(DAHEAP_INVALID_PTR, ((DWORD)pBlock) + dwBaseBlockSize);
		return 0;
	}

	if (dwFlags & DAHEAPFLAG_DEBUGFILL_SNAN)		// includes regular mem fill too
	{
		if ((dwFlags & DAHEAPFLAG_DEBUGFILL_SNAN) == DAHEAPFLAG_DEBUGFILL_SNAN)
			snanfill(&pBlock->pMsg, ((dwSize&~1)-daoffsetofmember(BASE_BLOCK,pMsg)) >> 2);
		else
			memset(&pBlock->pMsg, 0xCB, (dwSize&~1)-daoffsetofmember(BASE_BLOCK,pMsg));
	}

	pLower = (FREE_BLOCK *) pBlock->getLower();
	dwSize += pLower->dwSize;
	
	if ((dwSize & 1) != 0)	// lower block was free
	{
		FREE_BLOCK *pNewLower;

		pBlock->dwSize = dwSize;
		pNewLower = (FREE_BLOCK *) pBlock->getLower();
		pNewLower->pUpper = pBlock;	// update new lower block

		UNLINK(pLower);
	}
	
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL HeapInstance::free (FREE_BLOCK *pBlock)
{
	ASSERT(pBlock);

	FREE_BLOCK *pUpper;
	DWORD dwSize;
	
	// merge this block with lower one if possible
	if (mergeWithLower(pBlock) == 0)
		return 0;

	// if block above is "free", add our size to it. no relinking is needed

	if ((pUpper = (FREE_BLOCK *)pBlock->getUpper()) != 0 && 
		((dwSize = pBlock->dwSize + pUpper->dwSize) & 1) != 0)
	{
		FREE_BLOCK *pLower;
		// just add new size to upper block
		pUpper->dwSize = dwSize-1;		// remove the allocated marker

		// inform lower neighbor of the change

		pLower = (FREE_BLOCK *) pUpper->getLower();
		pLower->pUpper = pUpper;

		if ((dwFlags & DAHEAPFLAG_NOBESTFIT) == 0)
			sort(pUpper);
	}
	else
	{
		// just relink the block into the list

		pBlock->dwSize &= ~1;		// remove the allocation marker
		RELINK(pBlock);

	}
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 HeapInstance::SetBlockOwner (void *allocatedBlock, U32 owner)
{
	BOOL32 result;
	BASE_BLOCK *pBlock;

	if ((pBlock = verifyBlock(allocatedBlock)) == 0)
	{
		result = 0;
		goto Done;
	}

	if (pBlock->isAllocated() && (dwFlags & DAHEAPFLAG_NOMSGS) == 0)
	  	pBlock->setOwner(owner);

	result=1;

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
U32 HeapInstance::GetBlockOwner (void *allocatedBlock)
{
	U32 result=0;
	BASE_BLOCK *pBlock;

	if ((pBlock = verifyBlock(allocatedBlock)) == 0)
		goto Done;

	if (pBlock->isAllocated())
	{
		if ((dwFlags & DAHEAPFLAG_NOMSGS) == 0)
			result = pBlock->getOwner();
		else
			result = 0xFFFFFFFF;		// signal that block is allocated, but we don't know who owns it
	}

Done:
	return result;
}

//--------------------------------------------------------------------------//
//---------------------------END HeapInst.cpp-------------------------------//
//--------------------------------------------------------------------------//
