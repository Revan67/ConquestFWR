#ifndef BASEHEAP_H
#define BASEHEAP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              BaseHeap.H                                  //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Libs/Dev/Src/DACOM/BaseHeap.h 6     3/20/00 10:11a Jasony $

		                       Base Heap Object

*/
//--------------------------------------------------------------------------//

#ifndef HEAPOBJ_H
#include "HeapObj.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
struct BASE_BLOCK
{
	DWORD				dwSize;		// size in bytes of whole block
	struct BASE_BLOCK *	pUpper;		// upper adjacent block

	union 
	{
		const C8 *		pMsg;	// pointer to user defined message
		struct FREE_BLOCK *	pPrev;	// previous block in free list
	};

	union 
	{
		U32				pOwner;		// user defined, defaults to addr of code that allocated the block
		struct FREE_BLOCK *	pNext;	// next block in free list
	};

	struct BASE_BLOCK * getUpper (void);

	struct BASE_BLOCK * getLower (void);

	BOOL isAllocated (void);

	const C8 * getMsg (void)
	{
		return pMsg;
	}

	void setMsg (const C8 *_pMsg)
	{
		pMsg = _pMsg;
	}

	U32 getOwner (void)
	{
		return pOwner;
	}

	void setOwner (U32 _pOwner)
	{
		pOwner = _pOwner;
	}
};

struct FREE_BLOCK : public BASE_BLOCK
{
	struct FREE_BLOCK * getPrev (void);

	struct FREE_BLOCK * getNext (void);
};


inline struct BASE_BLOCK * BASE_BLOCK::getUpper (void)
{
	return pUpper;
}

inline struct BASE_BLOCK * BASE_BLOCK::getLower (void)
{
	return (BASE_BLOCK *) (((C8 *)this) + (dwSize & ~1));
}

inline BOOL BASE_BLOCK::isAllocated (void)
{
	return (dwSize & 1);
}

inline struct FREE_BLOCK * FREE_BLOCK::getPrev (void)
{
	return pPrev;
}

inline struct FREE_BLOCK * FREE_BLOCK::getNext (void)
{
	return pNext;
}


//--------------------------------------------------------------------------//
//------------------------BaseHeap Class Declaration------------------------//
//--------------------------------------------------------------------------//


struct BaseHeap : public IHeap
{
	DWORD					dwRefs;
	DA_ERROR_HANDLER		pErrorHandler;
	DWORD					dwFlags;
	struct HeapInstance *	pNext;

	BaseHeap (void)
	{
		memset(((C8 *) this)+sizeof(DWORD), 0, sizeof(*this)-sizeof(DWORD));
		dwRefs=1;
	}

	// *** IDAComponent methods ***

    DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance);
	DEFMETHOD_(U32,AddRef)           (void);
	DEFMETHOD_(U32,Release)          (void);

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

	virtual void * __stdcall malloc_pass_through (const C8 * msg = 0);
	virtual void * __stdcall realloc_pass_through (const C8 * msg = 0);
	virtual void * __stdcall calloc_pass_through (const C8 * msg = 0);

	// *** Heap methods ***

	BOOL __fastcall AddToList (struct HeapInstance *pHeap);

	struct HeapInstance * __fastcall FindTheHeap (void *baseAddress);

	BOOL32 doError (DWORD dwErrorNum, DWORD dwNum1=0, DWORD dwNum2=0);
};

//--------------------------------------------------------------------------//
//-----------------------HeapInstance Class Declaration---------------------//
//--------------------------------------------------------------------------//


struct HeapInstance : public BaseHeap
{
	FREE_BLOCK *			pFirstFreeBlock;
	DWORD					dwBaseBlockSize;
	void *					pHeapBase;
	DWORD					dwHeapSize;		// allocation size
	DWORD					dwGrowSize;
	CRITICAL_SECTION		criticalSection;	// used only in multithreaded instances


	HeapInstance (void)
	{
		memset(((C8 *) this)+sizeof(BaseHeap), 0, sizeof(*this)-sizeof(BaseHeap));
	}

	// *** IDAComponent methods ***
	
	DEFMETHOD_(U32,Release)          (void);

	// *** IHeap methods ***

	DEFMETHOD_(void *,AllocateMemory) (U32 numBytes, const C8 *msg);

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

	// *** HeapInstance methods ***

	GENRESULT initHeap (DAHEAPDESC *lpDesc);

	void __fastcall sort (FREE_BLOCK *pBlock);

	BASE_BLOCK * __fastcall malloc (DWORD dwNumBytes);

	BOOL __fastcall mergeWithLower (BASE_BLOCK *pBlock);

	BOOL __fastcall free (FREE_BLOCK *pBlock);

	BASE_BLOCK * verifyBlock (void *_pBlock);

	void RELINK (FREE_BLOCK *pBlock);

	void UNLINK (FREE_BLOCK *pBlock);
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
inline void HeapInstance::RELINK (FREE_BLOCK *pBlock)
{
	FREE_BLOCK *pStart;

  	if ((pStart = pFirstFreeBlock) == 0)		
	{
		// empty list
		pFirstFreeBlock = pBlock;
		pBlock->pPrev = 
		pBlock->pNext = pBlock;
		return;
	}

	if ((dwFlags & DAHEAPFLAG_NOBESTFIT) == 0)
	{
		FREE_BLOCK *pTmp;
		DWORD dwSize;

		pTmp = pStart;
		dwSize = pBlock->dwSize;

		if (dwSize < pStart->dwSize)
			pFirstFreeBlock = pBlock;
		else
		{
			while ((pStart = pStart->getNext()) != pTmp)
			{
				if (dwSize <= pStart->dwSize)
					break;
			}
		}
		
		pStart = pStart->getPrev();
	}

	pBlock->pNext = pStart->pNext;
	pBlock->pPrev = pStart;
	pStart->pNext = pBlock;
	pBlock->pNext->pPrev = pBlock;
}

//--------------------------------------------------------------------------//
//
inline void HeapInstance::UNLINK (FREE_BLOCK *pBlock)
{
	FREE_BLOCK *pPrev, *pNext;

	if ((pNext = pBlock->getNext()) == pBlock)
	{
		// list is empty

		pFirstFreeBlock = 0;
		return;
	}

	if (pBlock == pFirstFreeBlock)
		pFirstFreeBlock = pNext;

	// unlink the element

	pPrev = pBlock->getPrev();

	pPrev->pNext = pNext;
	pNext->pPrev = pPrev;
}	

//--------------------------------------------------------------------------//
//
inline BASE_BLOCK * HeapInstance::verifyBlock (void *_pBlock)
{
	BASE_BLOCK *pBlock;
	
	if (_pBlock == 0)
		return 0;
		
	pBlock = (BASE_BLOCK *) (((C8 *)_pBlock) - dwBaseBlockSize);
	if (dwFlags & DAHEAPFLAG_NOVERIFYPTR)
		return pBlock;

	// lower block check
	if (pBlock->getLower()->getUpper() != pBlock)
		goto Error;
	// upper block check
	if (pBlock->getUpper() && pBlock->getUpper()->getLower() != pBlock)
		goto Error;

	return pBlock;
Error:
	if (EnumerateBlocks(0,0) == 0)
		doError(DAHEAP_HEAP_CORRUPTED);
	else
		doError(DAHEAP_INVALID_PTR, (DWORD)_pBlock);
	return 0;
}
//--------------------------------------------------------------------------//
//---------------------MTHeapInstance definition----------------------------//
//--------------------------------------------------------------------------//
//
struct MTHeapInstance : public HeapInstance
{
	// *** IHeap methods ***

	DEFMETHOD_(void *,AllocateMemory) (U32 numBytes, const C8 *msg)
	{
		EnterCriticalSection(&criticalSection);
		void * result = HeapInstance::AllocateMemory(numBytes, msg);
		LeaveCriticalSection(&criticalSection);
		return result;
	}

	DEFMETHOD_(void *,ReallocateMemory) (void *prevBlock, U32 newSize, const C8 *msg)
	{
		EnterCriticalSection(&criticalSection);
		void * result = HeapInstance::ReallocateMemory(prevBlock, newSize, msg);
		LeaveCriticalSection(&criticalSection);
		return result;
	}

	DEFMETHOD_(BOOL32,FreeMemory) (void *allocatedBlock)
	{
		if (allocatedBlock)
		{
			EnterCriticalSection(&criticalSection);
			BOOL32 result = HeapInstance::FreeMemory(allocatedBlock);
			LeaveCriticalSection(&criticalSection);
			return result;
		}
		return 0;
	}

	DEFMETHOD_(BOOL32,EnumerateBlocks) (IHEAP_ENUM_PROC proc, void *context=0)
	{
		EnterCriticalSection(&criticalSection);
		BOOL32 result = HeapInstance::EnumerateBlocks(proc, context);
		LeaveCriticalSection(&criticalSection);
		return result;
	}

	DEFMETHOD_(U32,GetBlockSize) (void *allocatedBlock)
	{
		EnterCriticalSection(&criticalSection);
		U32 result = HeapInstance::GetBlockSize(allocatedBlock);
		LeaveCriticalSection(&criticalSection);
		return result;
	}

	DEFMETHOD_(const C8 *,GetBlockMessage) (void *allocatedBlock)
	{
		EnterCriticalSection(&criticalSection);
		const C8 * result = HeapInstance::GetBlockMessage(allocatedBlock);
		LeaveCriticalSection(&criticalSection);
		return result;
	}

	DEFMETHOD_(BOOL32,DidAlloc) (void *allocatedBlock)
	{
		EnterCriticalSection(&criticalSection);
		BOOL32 result = HeapInstance::DidAlloc(allocatedBlock);
		LeaveCriticalSection(&criticalSection);
		return result;
	}
	
	DEFMETHOD_(U32,GetAvailableMemory) (void)
	{
		EnterCriticalSection(&criticalSection);
		U32 result = HeapInstance::GetAvailableMemory();
		LeaveCriticalSection(&criticalSection);
		return result;
	}

	DEFMETHOD_(U32,GetLargestBlock) (void)
	{
		EnterCriticalSection(&criticalSection);
		U32 result = HeapInstance::GetLargestBlock();
		LeaveCriticalSection(&criticalSection);
		return result;
	}

	DEFMETHOD_(U32,GetHeapSize) (void)		// original alloc - overhead
	{
		EnterCriticalSection(&criticalSection);
		U32 result = HeapInstance::GetHeapSize();		// original alloc - overhead
		LeaveCriticalSection(&criticalSection);
		return result;
	}

    DEFMETHOD_(DA_ERROR_HANDLER,SetErrorHandler) (DA_ERROR_HANDLER proc)
	{
		EnterCriticalSection(&criticalSection);
		DA_ERROR_HANDLER result = HeapInstance::SetErrorHandler(proc);
		LeaveCriticalSection(&criticalSection);
		return result;
	}

	DEFMETHOD_(DA_ERROR_HANDLER,GetErrorHandler) (void)
	{
		EnterCriticalSection(&criticalSection);
		DA_ERROR_HANDLER result = HeapInstance::GetErrorHandler();
		LeaveCriticalSection(&criticalSection);
		return result;
	}

	DEFMETHOD_(BOOL32,SetBlockOwner) (void *allocatedBlock, U32 caller)
	{
		EnterCriticalSection(&criticalSection);
		BOOL32 result = HeapInstance::SetBlockOwner(allocatedBlock, caller);
		LeaveCriticalSection(&criticalSection);
		return result;
	}

	DEFMETHOD_(U32,GetBlockOwner) (void *allocatedBlock)
	{
		EnterCriticalSection(&criticalSection);
		U32 result = HeapInstance::GetBlockOwner(allocatedBlock);
		LeaveCriticalSection(&criticalSection);
		return result;
	}

	DEFMETHOD_(BOOL32,SetBlockMessage) (void *allocatedBlock, const C8 *msg)
	{
		EnterCriticalSection(&criticalSection);
		BOOL32 result = HeapInstance::SetBlockMessage(allocatedBlock, msg);
		LeaveCriticalSection(&criticalSection);
		return result;
	}
};

#endif

//--------------------------------------------------------------------------//
//---------------------------END BaseHeap.h---------------------------------//
//--------------------------------------------------------------------------//
