
#include "stdafx.h"
#include "globals.h"

#include <DACOM.h>
#include <HeapObj.h>
#include <GameSys.h>
#include <FileSys.h>
#include <RendPipeline.h>
#include <renderer.h>
#include <IHardPoint.h>
#include <IRenderPrimitive.h>
#include <Streamer.h>
#include <LightMan.h>
#include <IVertexBufferManager.h>
#include <ITextureLibrary.h>
#include <DPlay.h>

#include "Object.h"
#include "Startup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//
static bool bInCleanUp = false;

static void clean_up (void)
{
	if (bInCleanUp)
		return;
	bInCleanUp = true;

//	Object::DeleteList();

	VB_MANAGER->cleanup();

	CleanupGlobals();

	if (hMainDC)
	{
		ReleaseDC(hMainWindow, hMainDC);
		hMainDC = 0;
	}

	PB.~PB(); // force early destruction of PB (JY)

	if( ENGINE )
	{
		// todo(aaj-4/21/2004): need to shut down engine?
		ENGINE = NULL;
	}

	if( SYSTEM )
	{
		SYSTEM->Shutdown();

		// todo(aaj-4/21/2004): each component seems to have a REF but did not let go during "Shutdown()"
		while( SYSTEM->Release() ) {}

		SYSTEM = NULL;
	}

	CoUninitialize();

	if (DACOM)
	{
		DACOM->ShutDown();
		DACOM=0;
	}
}

void SetCleanUpAtExit(void)
{
	atexit( clean_up );
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//
#define NUM_CLEANUP_PTRS 26
struct CLEANUP_NODE
{
	struct CLEANUP_NODE * pNext;
	U32 numUsed;
	IDAComponent ** component[NUM_CLEANUP_PTRS];

	CLEANUP_NODE()
	{
		pNext = NULL;
		numUsed = 0;
	}
};
static CLEANUP_NODE * cleanupList;

//------------------------------------------------------------------------
//
template <> void AddToGlobalCleanupList (IDAComponent ** component)
{
	//
	// find an empty place on the list
	//
	if (cleanupList == 0 || cleanupList->numUsed >= NUM_CLEANUP_PTRS)
	{
		CLEANUP_NODE * node = new CLEANUP_NODE;
		node->pNext = cleanupList;
		cleanupList = node;
	}

	cleanupList->component[cleanupList->numUsed++] = component;
}
//-----------------------------------------------------------------------------
// delete everyone in the cleanup list
//
void CleanupGlobals (void)
{
	S32 numUsed;
	CLEANUP_NODE * node = cleanupList;

	while (node)
	{
		numUsed = node->numUsed;
		while (numUsed-- > 0)
		{
			if (*node->component[numUsed])
				(*node->component[numUsed])->Release();
			*node->component[numUsed] = 0;
		}
		node = node->pNext;
		delete cleanupList;
		cleanupList = node;
	}
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// start up components
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
#define NUM_STARTUP_PTRS 26
struct STARTUP_NODE
{
	struct STARTUP_NODE * pNext;
	U32 numUsed;
	GlobalComponent * component[NUM_STARTUP_PTRS];
};
static STARTUP_NODE startupNode;
static STARTUP_NODE *startupList = &startupNode;
//------------------------------------------------------------------------
//
void __stdcall AddToGlobalStartupList (struct GlobalComponent & component)
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
static void __stdcall initializeGlobalComponents (void)
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
void __stdcall CreateGlobalComponents (void)
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
