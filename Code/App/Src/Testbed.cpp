//--------------------------------------------------------------------------//
//                                                                          //
//                                Testbed.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Testbed.cpp 18    6/30/00 4:48p Tmauer $

	
	This file exists solely for testing game features.
*/			   
//---------------------------------------------------------------------------
#include "pch.h"
#include <globals.h>

#include "IObject.h"
#include "ObjList.h"
#include "Startup.h"
#include "DBHotkeys.h"
//building
#include "IFabricator.h"
//warping
#include "IGotoPos.h"
#include "IJumpgate.h"
//cloaking
#include "ICloak.h"
#include "Sector.h"
//
#include "ObjWatch.h"
#include "MGlobals.h"
#include "INugget.h"
#include "UserDefaults.h"

//flashing buttons
#include "IInterfaceManager.h"
#include "hotkeys.h"

//textures
#include "TManager.h"

#include <HeapObj.h>
#include <TSmartPointer.h>
#include <EventSys.h>
#include <TComponent.h>
#include <IConnection.h>
#include <WindowManager.h>

#include <stdlib.h>
#include <stdio.h>
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//---------------------------------Testbed Class----------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE Testbed : public IEventCallback
{
	U32 eventHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(Testbed)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	U32 currentTexture;

	Testbed (void);

	~Testbed (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* Testbed methods */

	void draw (U32 textureID);

	void init (void);

	IDAComponent * getBase (void)
	{
		return (IEventCallback *) this;
	}
};
//--------------------------------------------------------------------------//
//
Testbed::Testbed (void)
{
	currentTexture = 0;
}
//--------------------------------------------------------------------------//
//
Testbed::~Testbed (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);
}

void removeJumpgatesFromSystem(U32 systemID)
{
	IBaseObject * jumpSearch = OBJLIST->GetObjectList();
	while(jumpSearch)
	{
		if(jumpSearch->objClass == OC_JUMPGATE && jumpSearch->GetSystemID() == systemID)
		{
			IBaseObject * otherEnd = SECTOR->GetJumpgateDestination(jumpSearch);
			SECTOR->RemoveLink(jumpSearch,otherEnd);
			jumpSearch = OBJLIST->GetObjectList();//not infinite loop because jumpSearch is removed;
		}
		else
			jumpSearch = jumpSearch->next;
	}
};

void deleteObjectsInSystem(U32 systemID)
{
	NUGGETMANAGER->RemoveNuggetsFromSystem(systemID);
	IBaseObject * search = OBJLIST->GetObjectList();
	while(search)
	{
		if(search->GetSystemID() == systemID)
		{
			IBaseObject * obj = search;
			search = search->next;
			delete obj;
		}
		else
			search = search->next;
	}
}

void deleteSystem(U32 systemID)
{
	SECTOR->DeleteSystem(systemID);
}

void reassignSystem(U32 oldSystemID,U32 newSystemID)
{
	SECTOR->ReassignSystemID(oldSystemID,newSystemID);
	NUGGETMANAGER->MoveNuggetsToSystem(oldSystemID,newSystemID);
	IBaseObject * search = OBJLIST->GetObjectList();
	while(search)
	{
		if(search->GetSystemID() == oldSystemID)
		{
			OBJPTR<IPhysicalObject> phys;
			search->QueryInterface(IPhysicalObjectID,phys);
			if(phys)
				phys->SetSystemID(newSystemID);
		}
		search = search->next;
	}
}

void Testbed::draw (U32 textureID)
{
	BATCH->set_state(RPR_BATCH,FALSE);
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	OrthoView(0);
	SetupDiffuseBlend(textureID,TRUE);

	PB.Begin(PB_QUADS);
	PB.Color4ub(255,255,255,255);
	PB.TexCoord2f(0,0); PB.Vertex3f(100,100,0);
	PB.TexCoord2f(1,0); PB.Vertex3f(200,100,0);
	PB.TexCoord2f(1,1); PB.Vertex3f(200,200,0);
	PB.TexCoord2f(0,1); PB.Vertex3f(100,200,0);
	PB.End();
}
//-------------------------------------------------------------------
// receive notifications from event system
//
GENRESULT Testbed::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	static bool test_pause = 0;

	switch (message)
	{
	case CQE_DEBUG_HOTKEY:
		if (CQFLAGS.bGameActive)
		{
			switch ((U32)param)
			{
			case IDH_DELETE_SYSTEM:
				{
					U32 numSystems = SECTOR->GetNumSystems();
					if(numSystems > 1 && DEFAULTS->GetDefaults()->bEditorMode)
					{
						U32 currentSystem = SECTOR->GetCurrentSystem();
						if(currentSystem == numSystems)
							SECTOR->SetCurrentSystem(currentSystem-1);
						removeJumpgatesFromSystem(currentSystem);
						deleteObjectsInSystem(currentSystem);
						deleteSystem(currentSystem);
						while(currentSystem < numSystems)
						{
							reassignSystem(currentSystem+1,currentSystem);
							++currentSystem;
						}
					}
				}
				break;
			case IDH_NEXT_TEXTURE:
				{
					INTERMAN->StartHotkeyFlash(IDH_FORMATION_2);
					INTERMAN->StartHotkeyFlash(IDH_FORM_FLEET);
					if(currentTexture)
					{
						currentTexture = TMANAGER->GetNextTexture(currentTexture);
					}
					else
					{
						currentTexture = TMANAGER->GetFirstTexture();
					}
				}
				break;
			case IDH_PREV_TEXTURE:
				{
					INTERMAN->StopHotkeyFlash(IDH_FORMATION_2);
					INTERMAN->StopHotkeyFlash(IDH_FORM_FLEET);
					if(currentTexture)
					{
						currentTexture = TMANAGER->GetPrevTexture(currentTexture);
					}
					else
					{
						currentTexture = TMANAGER->GetFirstTexture();
					}
				}
				break;
/*			case IDH_CLOAK:
				{
					IBaseObject *obj2 = OBJLIST->GetSelectedList();
					OBJPTR<ICloak> ship;
					if (obj2->QueryInterface(ICloakID,ship))
					{
						ship->EnableCloak();
					}
				}
				break;
*/			}
		}
		break;
	case CQE_ENDFRAME:
		{
			if(currentTexture)
			{
				draw(currentTexture);
			}
		}
		break;
	}

	return GR_OK;
}
//-------------------------------------------------------------------
//
void Testbed::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Advise(getBase(), &eventHandle);
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
struct _testbed : GlobalComponent
{
	Testbed * test;
	virtual void Startup (void)
	{
		test = new DAComponent<Testbed>;
		AddToGlobalCleanupList(&test);
	}

	virtual void Initialize (void)
	{
		test->init();
	}
};

static _testbed testbed;

//-------------------------------------------------------------------
//-------------------------END Testbed.cpp---------------------------
//-------------------------------------------------------------------
