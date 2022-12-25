//--------------------------------------------------------------------------//
//                                                                          //
//                                PingLaunch.cpp                           //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header:
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "PingLauncher.h"

#include "SuperTrans.h"
#include "ObjList.h"
#include "sfx.h"

#include "Mission.h"
#include "IMissionActor.h"
#include "Startup.h"
#include "ArchHolder.h"
#include "TerrainMap.h"
#include "MPart.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h> 
#include <MGlobals.h>
#include <DMTechNode.h>

#include <stdlib.h>

// so that we can use the techtree stuff
using namespace TECHTREE;

//---------------------------------------------------------------------------
//
PingLaunch::PingLaunch (void) 
{
	pingLaunchIndex = -1;
}
//---------------------------------------------------------------------------
//
PingLaunch::~PingLaunch (void)
{
}
//---------------------------------------------------------------------------
//
void PingLaunch::init (PARCHETYPE _pArchetype)
{
	const BT_PING_LAUNCHER * data = (const BT_PING_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(data);
	CQASSERT(data->type == LC_PING_LAUNCH);
	CQASSERT(data->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;

}
//---------------------------------------------------------------------------
//
void PingLaunch::DoSpecialAbility (U32 specialID)
{
	{
		MPart part = owner.Ptr();
		if (part.isValid() && part->hullPoints==0)
			return;		// don't do this ability if already dead!
	}

	if (checkSupplies())
	{
		bPingIt = true;
	}
}
//---------------------------------------------------------------------------
//
S32 PingLaunch::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 PingLaunch::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
BOOL32 PingLaunch::Update (void)
{
	checkTechLevel();

	return 1;
}
//---------------------------------------------------------------------------
//
void PingLaunch::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	const BT_PING_LAUNCHER * data = (const BT_PING_LAUNCHER *) ARCHLIST->GetArchetypeData(pArchetype);

	pingSupplyCost = data->supplyCost;
	techNode =		 data->techNode;

	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
}
//---------------------------------------------------------------------------
//
const TRANSFORM & PingLaunch::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
bool PingLaunch::checkSupplies()
{
	return ((owner.Ptr()->effectFlags.canShoot()) && owner->UseSupplies(pingSupplyCost));
}
//---------------------------------------------------------------------------
//
void PingLaunch::checkTechLevel()
{
	// check if we are able to use this special weapon
	TECHNODE techLevel = MGlobals::GetCurrentTechLevel(owner.Ptr()->GetPlayerID());

	// here, we are going to set the caps bit
	MPartNC partNC(owner.Ptr());

	if (techLevel.HasTech(techNode))
	{
		partNC->caps.specialAbilityOk = true;
	}
	else
	{
		partNC->caps.specialAbilityOk = false;
	}
}


//---------------------------------------------------------------------------
//
inline IBaseObject * createPingLaunch (PARCHETYPE pArchetype)
{
	PingLaunch * pingLaunch = new ObjectImpl<PingLaunch>;

	pingLaunch->init(pArchetype);

	return pingLaunch;
}
//------------------------------------------------------------------------------------------
//---------------------------PingLaunch Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE PingLaunchFactory : public IObjectFactory
{
	struct OBJTYPE 
	{
		PARCHETYPE pArchetype;
		PARCHETYPE pBoltType;

		void * operator new (size_t size)
		{
			return calloc(size,1);
		}

		OBJTYPE (void)
		{
		}

		~OBJTYPE (void)
		{
			if (pBoltType)
				ARCHLIST->Release(pBoltType, OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(PingLaunchFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	PingLaunchFactory (void) { }

	~PingLaunchFactory (void);

	void init (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IObjectFactory methods */

	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);

	/* PingLaunchFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
PingLaunchFactory::~PingLaunchFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void PingLaunchFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE PingLaunchFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_PING_LAUNCHER * data = (BT_PING_LAUNCHER *) _data;

		if (data->type == LC_PING_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 PingLaunchFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * PingLaunchFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createPingLaunch(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void PingLaunchFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _pingLaunch : GlobalComponent
{
	PingLaunchFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<PingLaunchFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _pingLaunch __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End PingLaunch.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------