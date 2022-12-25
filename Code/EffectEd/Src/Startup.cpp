//--------------------------------------------------------------------------//
//                                                                          //
//                           Startup.cpp                                    //
//                                                                          //
//               COPYRIGHT (C) 2003 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /EffectEd/Src/Startup.cpp 1     7/18/03 11:54a Tmauer $

   $Author: Tmauer $
*/
//--------------------------------------------------------------------------//
//------------------------------------------------------------------------------
//
#include "stdafx.h"

#include "startup.h"

#include <stdlib.h>

#define NUM_STARTUP_PTRS 26
struct STARTUP_NODE
{
	struct STARTUP_NODE * pNext;
	U32 numUsed;
	GlobalComponent * component[NUM_STARTUP_PTRS];

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

};
static STARTUP_NODE startupNode;
static STARTUP_NODE *startupList = &startupNode;
//------------------------------------------------------------------------
//
void AddToGlobalStartupList (struct GlobalComponent & component)
{
	//
	// find an empty place on the list
	//
	if (startupList->numUsed >= NUM_STARTUP_PTRS)
	{
		STARTUP_NODE * node = new STARTUP_NODE;
		node->pNext = startupList;
		startupList = node;
	}

	startupList->component[startupList->numUsed++] = &component;
}
//------------------------------------------------------------------------
//
static void initializeGlobalComponents (void)
{
	S32 numUsed;
	STARTUP_NODE * node = startupList;

	while (node)
	{
		numUsed = node->numUsed;
		while (numUsed-- > 0)
	 		node->component[numUsed]->Initialize();
		if ((node = node->pNext) != 0)	// don't delete the last node (it's static)
		{
			delete startupList;
			startupList = node;
		}
	}
}
//------------------------------------------------------------------------
//
void CreateGlobalComponents (void)
{
	S32 numUsed;
	STARTUP_NODE * node = startupList;

	while (node)
	{
		numUsed = node->numUsed;
		while (numUsed-- > 0)
	 		node->component[numUsed]->Startup();
		node = node->pNext;
	}

	initializeGlobalComponents();
}