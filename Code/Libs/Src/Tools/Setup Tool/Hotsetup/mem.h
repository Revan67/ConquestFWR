/****************************************************************************
 *	mem.h
 *
 *	group of routines which replace the standard memory allocation and free
 *	command. Debug version adds a bunch of code to help keep track of lost
 *	and unused memory. Also see mem.cpp, block.h, & block.cpp
 */

#ifndef _MEM
#define _MEM

#ifdef _DEBUG
//constant used to fill memory we are freeing
#define bGarbage 0xCC
#endif

void FreeMemory(void *pv);
#ifdef _DEBUG
BOOL _FNewMemory(void **ppv, size_t size, char *pFileName, unsigned wLineNumber);
#else
BOOL _FNewMemory(void **ppv, size_t size);
#endif //_DEBUG
BOOL FResizeMemory(void **ppv, size_t sizeNew);

#ifdef _DEBUG
#define FNewMemory(ppv, size) _FNewMemory(ppv, size, __FILE__, __LINE__)
#else
#define FNewMemory(ppv, size) _FNewMemory(ppv, size)
#endif  //_DEBUG

#endif //_MEM
