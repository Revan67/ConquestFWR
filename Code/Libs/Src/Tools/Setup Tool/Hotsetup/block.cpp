#ifdef _DEBUG

#include "stubpch.h"
#include "block.h"
#include "mem.h"

/****************************************************************************
 * The functions in this file must compare arbitrary pointers, an operation 
 * that the ANSI standard does not guarantee to be portable.
 *
 * The macros below isolate the pointer comparisons needed in this file. The
 * implementations assume "flat" pointers, for which straightforward
 * compariosns will always work. The definitions below will *not* work for
 * some of the common 80x86 memory models.
 */

#define fPtrLess(pLeft, pRight)		((pLeft) <  (pRight))
#define fPtrGrtr(pLeft, pRight)		((pLeft) >  (pRight))
#define fPtrEqual(pLeft, pRight)	((pLeft) == (pRight))
#define fPtrLessEq(pLeft, pRight)	((pLeft) <= (pRight))
#define fPtrGrtrEq(pLeft, pRight)	((pLeft) >= (pRight))

/****************************************************************************
 *				* * * * Private data/functions * * * *
 ****************************************************************************/


/****************************************************************************
 * pbiHead points to a singly linked list of debugging information
 * for the memory manager
 */
static blockinfo *v_pbiHead = NULL;

/****************************************************************************
 * Procedure	PbiGetBlockInfo(pb)
 *
 * Purpose		searches the memory log to find the block that pb points
 *				into and returns a pointer to the corresponding blockinfo
 *				structure of the memory log.
 *				Note: pb *must* point into an allocated block or you will get
 *				an assertion failure; the function either asserts or succeeds
 *				-- it never returns an error;
 *
 * Parameters	pb - pointer into an allocated block of memory.
 *
 * Returns		pbi - pointer to the info block for pb
 *
 * Usage		blockinfo *pbi;
 *				...
 *				pbi = PbiGetBlockInfo(pb);
 *				// pbi->pb points to start of pb's block
 *				// pbi->size is the size of the block that pb points into
 *
 * History		3/10/97 - a-drews - created
 */
static blockinfo *PbiGetBlockInfo(byte *pb)
{
	blockinfo *pbi;

	for (pbi = v_pbiHead; pbi != NULL; pbi = pbi->pbiNext)
	{
		byte *pbStart = pbi->pb;
		byte *pbEnd = pbi->pb + pbi->size - 1;

		if (fPtrGrtrEq(pb, pbStart) && fPtrLessEq(pb, pbEnd))
			break;
	}

	/* Couldn't find pointer? Is it garbage? pointer to a block that was freed?
	 * pointing to a block that moved when it was resized by FResizeMemory?
	 */
	ASSERT(pbi != NULL);

	return(pbi);
}

/****************************************************************************
 *					* * * * Public funtions * * * *
 ****************************************************************************/


/****************************************************************************
 * Procedure	FCreateBlockInfo(pbNew, sizeNew)
 *
 * Purpose		creats a log entry for the memory block defined by 
 *				pbNew:sizeNew.
 *
 * Parameters	pbNew - pointer to the start of the memory block
 *				sizeNew - size in bytes of the memory block
 *
 * Returns		TRUE if log info successfully created; FALSE otherwise
 *
 * Usage		if (FCreateBlockInfo(pbNew, sizeNew)
 *					// success - memory log has an entry
 *				else
 *					// failure - no entry, so release pbNew
 *
 * History		3/10/97 - a-drews - created
 */
BOOL FCreateBlockInfo(byte *pbNew, size_t sizeNew, char *pFileName, unsigned wLineNumber)
{
	blockinfo *pbi;

	ASSERT(pbNew != NULL && sizeNew != 0);
	ASSERT(NULL != pFileName);

	pbi = (blockinfo *)malloc(sizeof(blockinfo));
	if (pbi != NULL)
	{
		pbi->pFileName = (char *) malloc(lstrlen(pFileName) + 1);

		if (NULL != pbi->pFileName)
		{
			lstrcpy(pbi->pFileName, pFileName);
			pbi->wLineNumber = wLineNumber;
			pbi->pb = pbNew;
			pbi->size = sizeNew;
			pbi->pbiNext = v_pbiHead;
			v_pbiHead = pbi;
		}
		else
		{
			free(pbi);
			pbi = NULL;
		}
	}

	return (BOOL)(pbi != NULL);
}

/****************************************************************************
 * Procedure	FreeBlockInfo(pbToFree)
 *
 * Purpose		destroy the log entry for the memory block that pbToFree
 *				points to. pbToFree MUST point to the start of an allocated
 *				block; otherwise, you will get an assertion failure.
 *
 * Parameters	pbToFree - pointer to the lock to free.
 *
 * Returns		nothing
 *
 * History		3/10/97 - a-drews - created
 */
void FreeBlockInfo(void *pvToFree)
{
	blockinfo *pbi, *pbiPrev;

	pbiPrev = NULL;
	for (pbi = v_pbiHead; pbi != NULL; pbi = pbi->pbiNext)
	{
		if (fPtrEqual(pbi->pb, (byte *)pvToFree))
		{
			if (NULL == pbiPrev)
				v_pbiHead = pbi->pbiNext;
			else
				pbiPrev->pbiNext = pbi->pbiNext;
			break;
		}
		pbiPrev = pbi;
	}

	// If pbi is NULL, then pbToFree is invalid
	ASSERT(pbi != NULL);

	// destroy the contents pf *pbi before freeing them
	ASSERT(NULL != pbi->pFileName);

	FillMemory(pbi->pFileName, lstrlen(pbi->pFileName) + 1, bGarbage);
	free(pbi->pFileName);

	FillMemory(pbi, sizeof(blockinfo), bGarbage);
	free(pbi);
}

/****************************************************************************
 * Procedure	UpdateBlockInfo(pbOld, pbNew, sizeNew)
 *
 * Purpose		looks up the log info for the memory block that pbOld points
 *				to. The function then updates the log info to reflect the fact
 *				that the block now lives at pbNew and is "sizeNew bytes" long.
 *				pbOld MUST point to the start of an allocated block; otherwise,
 *				you will get an assertion failure.
 *
 * Parameters	pbOld - pointer to old memory block
 *				pbNew - pointer to new memory block
 *				sizeNew - size of new memory block
 *
 * Returns		nothing
 *
 * History		3/10/97 - a-drews - created
 */
void UpdateBlockInfo(byte *pbOld, byte *pbNew, size_t sizeNew)
{
	blockinfo *pbi;

	ASSERT(pbNew != NULL && sizeNew != 0);

	pbi = PbiGetBlockInfo(pbOld);
	ASSERT(pbOld == pbi->pb);

	pbi->pb = pbNew;
	pbi->size = sizeNew;
}

/****************************************************************************
 * Procedure	SizeOfBlock(pv)
 *
 * Purpose		returns the size of the block pv points to. pv MUST point to 
 *				the start of an allocated block; otherwise, you will get an
 *				assertion failure.
 *
 * Parameters	pv - pointer to allocated memory block.
 *
 * Returns		size of pv.
 *
 * History		3/10/97 - a-drews - created
 */
size_t SizeOfBlock(void *pv)
{
	blockinfo *pbi;

	pbi = PbiGetBlockInfo((byte *)pv);
	ASSERT(pv == (void *)pbi->pb);

	return(pbi->size);
}

/****************************************************************************
 * Procedure	ClearMemoryRefs(void)
 *
 * Purpose		marks all blocks in the memory log as being unreferenced.
 *
 * Parameters	none
 *
 * Returns		nothing
 *
 * History		3/10/97 - a-drews - created
 */
void ClearMemoryRefs(void)
{
	blockinfo *pbi;

	for (pbi = v_pbiHead; NULL != pbi; pbi = pbi->pbiNext)
		pbi->fReferenced = FALSE;
}

/****************************************************************************
 * Procedure	NoteMemoryRefs(pv)
 *
 * Purpose		marks the block that pv points into as being referenced.
 *				Note: pv does NOT have to point to the start of a block;
 *				it may point anywhere within an allocated block.
 *
 * Parameters	pv - pointer to memory.
 *
 * Returns		nothing
 *
 * History		3/10/97 - a-drews - created
 */
void NoteMemoryRef(void *pv)
{
	blockinfo *pbi;

	pbi = PbiGetBlockInfo((byte *)pv);
	pbi->fReferenced = TRUE;
}

/****************************************************************************
 * Procedure	CheckMemoryRefs(void)
 *
 * Purpose		scans the memory log looking for blocks that have not been
 *				marked with a call to NoteMemoryRef. If this function finds
 *				an unmarked block, it asserts.
 *
 * Parameters	none
 *
 * Returns		nothing
 *
 * History		3/10/97 - a-drews - created
 */
void CheckMemoryRefs(void)
{
	blockinfo *pbi;

	for (pbi = v_pbiHead; NULL != pbi; pbi = pbi->pbiNext)
	{
		// a simple check for block integrity. If this ASSERT fires,
		// it means that something is wrong with the debug code that 
		// manages blockinfo or, possibly, that a wild memory store
		// has trashed the data structure.
		ASSERT(pbi->pb != NULL && pbi->size != 0);

		// A check for lost or leaky memory. If this ASSERT fires, it
		// means that the app has either lost track of this block or
		// that not all global pointers have been accounted for in
		// NotememoryRef.
		if (!pbi->fReferenced)
		{
			char sz[512];

			wsprintf( sz, "Memory lost from FNewMemory() call %s, %d\n",
				pbi->pFileName, pbi->wLineNumber );
			OutputDebugStr(sz);
			ASSERT(FALSE);
		}
	}
}

/****************************************************************************
 * Procedure	FValidPointer(pv, size)
 *
 * Purpose		verifies that pv points into an allocated memory block and
 *				and that there are at least "size" allocated bytes from pv
 *				to the end of the block. If either condition is not met,
 *				FValidPointer will ASSERT; the function will never return
 *				FALSE.
 *
 *				The reason FValidPointer returns a flagh at all (always TRUE)
 *				is to allow you to call the function within an ASSERT macro.
 *				While this isn't the most efficient method to use, using the
 *				macro neatly handles the debug vs. ship version control issue
 *				without your having to resort to #ifdef DEBUG's or introducing
 *				other ASSERT-like macros.
 *
 * Parameters	pv - pointer to validate.
 *				size - size of memory pv points to.
 *
 * Returns		always TRUE.
 *
 * Usage		ASSERT(FValidPointer(pb, size));
 *
 * History		3/10/97 - a-drews - created.
 */
BOOL FValidPointer(void *pv, size_t size)
{
	blockinfo *pbi;
	byte *pb = (byte *)pv;

	ASSERT(pv != NULL && size != 0);

	pbi = PbiGetBlockInfo(pb);			// This validates pb. */

	/* size isn't valid if pb+size overflows the block. */
	ASSERT(fPtrLessEq(pb + size, pbi->pb + pbi->size));

	return(TRUE);
}

#endif //_DEBUG
