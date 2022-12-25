#ifdef _DEBUG

/****************************************************************************
 * blockinfo is a structure that contains the memory log information
 * for one allocated memory block. Every allocated memory block has a
 * corresponding blockinfo structure in the memory log.
 */

typedef struct BLOCKINFO
{
	struct BLOCKINFO *pbiNext;
	byte *pb;						/* Start of block */
	size_t size;					/* Length of block */
	BOOL fReferenced;				/* Ever referenced? */
	char *pFileName;				/* File this allocation was called from */
	unsigned wLineNumber;			/* Line number this allocation was called from */
} blockinfo;

BOOL FCreateBlockInfo(byte *pbNew, size_t sizeNew, char *filename, unsigned linenumber);
void FreeBlockInfo(void *pvToFree);
void UpdateBlockInfo(byte *pbOld, byte *pbNew, size_t sizeNew);
size_t SizeOfBlock(void *pv);

void ClearMemoryRefs(void);
void NoteMemoryRefs(void *pv);
void CheckMemoryRefs(void);
BOOL FValidPointer(void *pv, size_t size);

#endif
