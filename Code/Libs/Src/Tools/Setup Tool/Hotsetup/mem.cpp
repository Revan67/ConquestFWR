/****************************************************************************
 *	mem.h
 *
 *	group of routines which replace the standard memory allocation and free
 *	command. Debug version adds a bunch of code to help keep track of lost
 *	and unused memory. Also see mem.h, block.h, & block.cpp
 */

#include "stubpch.h"
#ifdef _DEBUG
#include "block.h"
#include "mem.h"
#endif //_DEBUG

/****************************************************************************
 * Procedure	FreeMemory(pv)
 *
 * Purpose		Frees memory allocated by FNewMemory. pv MUST be a valid pointer
 *				into allocated memory or an assertion will fire. Handles
 *				freeing up Memory block info.
 *
 * Parameters	pv - pointer to memory to be freed
 *
 * Returns		nothing
 *
 * History		3/11/97 - a-drews - created
 */
void FreeMemory(void *pv)
{
#ifdef _DEBUG
	ASSERT(NULL != pv);

	size_t size = SizeOfBlock(pv);
	ASSERT(0 != size);
	ASSERT(FValidPointer(pv, size));

	// if this assert fires then you have written off the end of your buffer
	ASSERT(bGarbage == *((BYTE *)pv+size-1));

	// set memory being freed to trash value to help catch refs to
	// freed memory
	FillMemory(pv, size, bGarbage);
	FreeBlockInfo(pv);
#endif //_DEBUG

	free(pv);
}

/****************************************************************************
 * Procedure	FNewMemory(ppv, size)
 *
 * Purpose		Allocated memory of size "size" and puts pointer in ppv.
 *				Debug version records block info and presets memory to garbage
 *				chars.
 *
 * Parameters	ppv - pointer to the pointer which will recieve the new memory.
 *				size - size of block to allocate.
 *
 * Returns		True if memory allocated successfully, FALSE otherwise
 *
 * History		3/11/97 - a-drews - created
 */
#ifdef _DEBUG
BOOL _FNewMemory(void **ppv, size_t size, char *pFileName, unsigned wLineNumber)
#else
BOOL _FNewMemory(void **ppv, size_t size)
#endif //_DEBUG
{
	byte **ppb = (byte **)ppv;

	ASSERT(NULL != ppv && 0 != size);

#ifdef _DEBUG
	// Allocate 1 extra byte to use as a marker to watch for memory overwrites
	++size;
#endif //_DEBUG

	*ppb = (byte *)malloc(size);

#ifdef _DEBUG
	if (*ppb != NULL)
	{
		FillMemory(*ppb, size, bGarbage);

		// if unable to create block info, fake a total memory failue.
		if (!FCreateBlockInfo(*ppb, size, pFileName, wLineNumber))
		{
			free(*ppb);
			*ppb = NULL;
		}
	}
#endif //_DEBUG

	return(NULL != *ppb);
}

/****************************************************************************
 * Procedure	FResizeMemory(ppv, sizeNew)
 *
 * Purpose		resize memory allocation pointed to by ppv to sizeNew.
 *				Debug version makes sure memory moves and updates Block info.
 *
 * Parameters	ppv - pointer to pointer of memory to resize. This memory
 *				block MUST have been allocated by FNewMemory and be valid or
 *				an assertion will fire.
 *
 * Returns		TRUE if memory successfully resized, FALSE otherwise.
 *
 * History		3/11/97 - a-drews - Created
 */
BOOL FResizeMemory(void **ppv, size_t sizeNew)
{
	byte **ppb = (byte **)ppv;
	byte *pbNew;
#ifdef _DEBUG
	size_t sizeOld;
#endif //_DEBUG

	ASSERT(NULL  != ppb && 0 != sizeNew);

#ifdef _DEBUG
	// Allocate 1 extra byte to use as a marker to watch for memory overwrites
	++sizeNew;
#endif //_DEBUG

#ifdef _DEBUG
	sizeOld = SizeOfBlock(*ppb);
	ASSERT(FValidPointer(*ppv, sizeOld));

	// if this assert fires then you have written off the end of your buffer
	ASSERT(bGarbage == **((BYTE **)ppv+sizeOld-1));

	// if the block is shrinking, prefill the soon-to-be release memory.
	// if the block is expanding, force it to move (instead of expanding
	// in place by faking a realloc. If the block is the same size, don't 
	// do anything.
	if (sizeNew < sizeOld)
	{
		FillMemory((*ppb)+sizeNew, sizeOld-sizeNew, bGarbage);
	}
	else if (sizeNew > sizeOld)
	{
		byte *pbForceNew;

		if (FNewMemory((void **)&pbForceNew, sizeNew))
		{
			CopyMemory(pbForceNew, *ppb, sizeOld);
			FreeMemory(*ppb);
			*ppb = pbForceNew;
		}
	}
#endif //_DEBUG

	pbNew = (byte *)realloc(*ppb, sizeNew);
	if (NULL != pbNew)
	{
#ifdef _DEBUG
		UpdateBlockInfo(*ppb, pbNew, sizeNew);

		// if expanding, initialize the new tail. */
		if (sizeNew > sizeOld)
			FillMemory(pbNew + sizeOld, sizeNew - sizeOld, bGarbage);
#endif //_DEBUG

		*ppb = pbNew;
	}

	return (NULL != pbNew);
}
