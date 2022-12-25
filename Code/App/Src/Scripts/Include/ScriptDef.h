#ifndef SCRIPTDEF_H
#define SCRIPTDEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               ScriptDef.h                                //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Scripts/Include/ScriptDef.h 8     11/07/00 11:03a Jasony $

   Only included by scripts, defines helper macros 
*/
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
// 4514: unused inline function
// 4201: nonstandard no-name structure use
// 4100: formal parameter was not used
// 4512: assignment operator could not be generated
// 4245: conversion from signed to unsigned
// 4127: constant condition expression
// 4355: 'this' used in member initializer
// 4244: conversion from int to unsigned char, possible loss of data
// 4200: zero sized struct member
// 4710: inline function not expanded
// 4702: unreachable code
// 4786: truncating function name (255 chars) in browser info
#pragma warning (disable : 4514 4201 4100 4512 4245 4127 4355 4244 4710 4702 4786)
#include <windows.h>
#pragma warning (disable : 4355 4201)

#include <CQTRACE.h>
#include <heapobj.h>
#include <mscript.h>
#include <MovieCameraFlags.h>
#include <TriggerFlags.h>
#include <DMBaseData.h>
#include <MGroupRef.h>

#define scriptoffsetofclass(base, derived) ((U32)(static_cast<base*>((derived*)8))-8)

extern CQSCRIPTDATADESC g_CQScriptDataDesc;

//
// NOTE: we alloc 4 extra bytes to work around a compile problem when savestruct is empty
//

#define CQSCRIPTPROGRAM(ProgClass, ProgSave, EventFlags)  \
struct ProgClass : CQBaseProgram, ProgSave  \
{											\
	virtual void Initialize (U32, const MPartRef &); 			\
											\
	virtual bool Update (void);				\
											\
	virtual void Destroy (void)				\
	{	 \
		delete this;	\
	}	\
		\
	void * operator new (size_t size)		\
	{										\
		return HEAP->ClearAllocateMemory(size+4, #ProgClass);	\
	}										\
	void   operator delete (void *ptr)		\
	{	\
		HEAP->FreeMemory(ptr);	\
	}	\
};											\
											\
struct ___##ProgClass		\
{							\
	CQSCRIPTENTRY entry;	\
							\
	___##ProgClass (void)	\
	{						\
		entry.pNext = 0;	\
		entry.progName = #ProgClass;	\
		entry.saveLoadStruct = #ProgSave;	\
		entry.saveOffset = scriptoffsetofclass(ProgSave, ProgClass);	\
		entry.saveSize = sizeof(ProgSave);	\
		entry.eventFlags = EventFlags;		\
		entry.factory = factory;	\
		MScript::RegisterScriptProgram(&entry);	\
	}							\
								\
	static CQBaseProgram * __cdecl factory (void)	\
	{									\
		return new ProgClass;			\
	}									\
										\
} ____##ProgClass


#if defined (NOCOMPILEE)
void CQSCRIPTPROGRAM (struct ProgClass, struct SaveLoadStruct)		// generate tooltip
{
}
#endif


#define CQSCRIPTDATA(DataClass, DataInstance)	\
DataClass DataInstance;	\
CQSCRIPTDATADESC g_CQScriptDataDesc = { sizeof(g_CQScriptDataDesc),0,0,#DataClass,&DataInstance,sizeof(DataInstance) }


#if defined (NOCOMPILEE)
void CQSCRIPTDATA (struct DataClass, instanceName);
#endif

#define ELAPSED_TIME  (8.0F / 30.0F)		// 26.6 msecs per update
//--------------------------------------------------------------------------//
//-------------------------------End ScriptDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif