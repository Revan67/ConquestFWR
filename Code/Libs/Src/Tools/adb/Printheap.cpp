//--------------------------------------------------------------------------//
//                                                                          //
//                             PrintHeap.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Ajackson $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"

//--------------------------------------------------------------------------//
//
void __cdecl _localprintf (const char *fmt, ...)
{
	char buffer[256];
	va_list ap;

	va_start(ap, fmt);
	wvsprintf(buffer, fmt, ap);
	va_end(ap);

	OutputDebugString(buffer);
}
//--------------------------------------------------------------------------//
//
const char * __cdecl _localLoadString (U32 dwID)
{
	static char buffer[256];

	buffer[0] = 0;
	LoadString(hInstance, dwID, buffer, sizeof(buffer)-1);

	return buffer;
}
//--------------------------------------------------------------------------//
//
static int __stdcall PrintBlock (struct IHeap * pHeap, void *allocatedBlock, U32 dwFlags, void *context)
{
/*
	const char *pMsg = pHeap->GetBlockMessage(allocatedBlock);
	char buffer[64];
	U32 owner = pHeap->GetBlockOwner(allocatedBlock);

	if (owner == 0xFFFFFFFF)		// marked block
		return 1;

	wsprintf(buffer, "Owner=%08X", owner);

	_localprintf("%s block at %08X  Size = %4d  %s %s %s\n",
		(dwFlags & DAHEAPFLAG_ALLOCATED_BLOCK)?"Allocated":"Free     ",
		allocatedBlock,
		pHeap->GetBlockSize(allocatedBlock),
		(pMsg)?pMsg:"",
		(owner)?buffer:"",
		(dwFlags & DAHEAPFLAG_CORRUPTED_BLOCK)?"[CORRUPTED]":"[OK]");
*/
	return 1;
}
//--------------------------------------------------------------------------//
//
int PrintHeap (IHeap *pHeap)
{
/*
	int result;

	_localprintf("Largest Block=%d, HeapSize=%d.", pHeap->GetLargestBlock(), pHeap->GetHeapSize());

	if (pHeap->GetLargestBlock() == pHeap->GetHeapSize())
		_localprintf("   Heap is empty.\n");
	else
		_localprintf("   Heap is NOT empty.\n");

	if ((result = pHeap->EnumerateBlocks((IHEAP_ENUM_PROC) PrintBlock)) == 0)
		_localprintf("Enumerate failed!\n");
	
	return result;
*/
	return 1;
}
//--------------------------------------------------------------------------//
//
static int __stdcall MarkBlock (struct IHeap * pHeap, void *allocatedBlock, U32 dwFlags, void *context)
{
//	pHeap->SetBlockOwner(allocatedBlock, 0xFFFFFFFF);
	return 1;
}
//--------------------------------------------------------------------------//
//
int MarkAllocatedBlocks (IHeap *pHeap)
{
//	return pHeap->EnumerateBlocks((IHEAP_ENUM_PROC) MarkBlock);
	return 1;
}

//--------------------------------------------------------------------------//
//------------------------------End PrintHeap.cpp---------------------------//
//--------------------------------------------------------------------------//
