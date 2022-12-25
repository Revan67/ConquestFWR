//--------------------------------------------------------------------------//
//                                                                          //
//                                MultiCloakLaunch.cpp                           //
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

#include "MultiCloakLauncher.h"

#include "SuperTrans.h"
#include "ObjList.h"
#include "sfx.h"

#include "Mission.h"
#include "IMissionActor.h"
#include "ICloak.h"
#include "Startup.h"
#include "ArchHolder.h"
#include "TerrainMap.h"
#include "MPart.h"
#include "OpAgent.h"

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
MultiCloakLauncher::MultiCloakLauncher (void) 
{
	cloakLaunchIndex = -1;
	bCloakEnabled = false;
	bPrepareToggle = false;
}
//---------------------------------------------------------------------------
//
MultiCloakLauncher::~MultiCloakLauncher (void)
{
	if(bCloakingTarget)
	{
		if(cloakTarget)
		{
			cloakTarget->EnableCloak(false);
		}
	}
	cloakTargetID = 0;
	cloakTarget = NULL;
	bCloakingTarget = false;
}
//---------------------------------------------------------------------------
//
void MultiCloakLauncher::init (PARCHETYPE _pArchetype)
{
	const BT_MULTICLOAK_LAUNCHER * data = (const BT_MULTICLOAK_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(data);
	CQASSERT(data->type == LC_MULTICLOAK_LAUNCH);
	CQASSERT(data->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;

}
//---------------------------------------------------------------------------
//
void MultiCloakLauncher::DoCloak (void)
{
	decloakTime = 0;
	if(bCloakingTarget)
	{
		if(cloakTarget)
		{
			cloakTarget->EnableCloak(false);
		}
	}
	cloakTargetID = 0;
	cloakTarget = NULL;
	bCloakingTarget = false;

	// well, well, well, we meet again.
	// do our magical cloaking stuff
	if(THEMATRIX->IsMaster())
	{
		// check our owners supplies and make sure there is enough to turn on cloaking
		if (!bCloakEnabled)
		{
			if (checkSupplies())
			{
				// make sure we turn off cloaking!
				bPrepareToggle = true;
			}
		}
		else
		{
			bPrepareToggle = true;
		}
	}
}
//---------------------------------------------------------------------------
//
bool MultiCloakLauncher::CanCloak()
{
	return checkSupplies();
}
//---------------------------------------------------------------------------
//
void MultiCloakLauncher::SpecialAttackObject (IBaseObject * obj)
{
	decloakTime = 0;
	if(bCloakingTarget)
	{
		if(cloakTarget)
		{
			cloakTarget->EnableCloak(false);
		}
	}
	cloakTargetID = 0;
	cloakTarget = NULL;
	bCloakingTarget = false;

	// well, well, well, we meet again.
	// do our magical cloaking stuff
	if(THEMATRIX->IsMaster())
	{
		// check our owners supplies and make sure there is enough to turn on cloaking
		if (!bCloakEnabled)
		{
			if (checkSupplies())
			{
				// make sure we turn off cloaking!
				bPrepareToggle = true;
			}
			else
				return;//if I can't cloak myself the why bother
		}
	}
	if(obj)
	{
		obj->QueryInterface(ICloakID,cloakTarget,NONSYSVOLATILEPTR);
		cloakTargetID = obj->GetPartID();
		bCloakingTarget = false;
	}
}
//---------------------------------------------------------------------------
//
S32 MultiCloakLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();// cloakLaunchIndex;
}
//---------------------------------------------------------------------------
//
U32 MultiCloakLauncher::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
BOOL32 MultiCloakLauncher::Update (void)
{
	checkTechLevel();

	// has enough time gone by to decrease our supplies?

	if (THEMATRIX->IsMaster())
	{
		if (bCloakEnabled || bCloakingTarget)
		{
			if(bCloakEnabled)
				cloakSupplyCount += cloakSupplyUse;

			if(bCloakingTarget && cloakTarget)
			{
				cloakSupplyCount += MPart(cloakTarget.Ptr())->hullPointsMax * targetSupplyCostPerHull;
			}

			if (cloakSupplyCount >= 1)
			{
				U32 supplyUse = cloakSupplyCount;
				owner->UseSupplies(supplyUse);
				// do we need to turn off cloaking?
				if (!checkSupplies())
				{
					bPrepareToggle = true;
				}
				cloakSupplyCount -= supplyUse;
			}
		}
	}
	else
	{
		if(bCloakingTarget && !cloakTarget)
		{
			OBJLIST->FindObject(cloakTargetID,NONSYSVOLATILEPTR,cloakTarget,ICloakID);
			if(cloakTarget)
				cloakTarget->EnableCloak(true);
		}
	}

	return 1;
}
//---------------------------------------------------------------------------
//
void MultiCloakLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE _range)
{
	const BT_MULTICLOAK_LAUNCHER * data = (const BT_MULTICLOAK_LAUNCHER *) ARCHLIST->GetArchetypeData(pArchetype);

	cloakSupplyCount = 0.0f;
	cloakSupplyUse = data->cloakSupplyUse/REALTIME_FRAMERATE;
	cloakShutoff =	 data->cloakShutoff;
	targetSupplyCostPerHull = data->targetSupplyCostPerHull;
	techNode =		 data->techNode;

	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);

}
//---------------------------------------------------------------------------
//
const TRANSFORM & MultiCloakLauncher::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
BOOL32 MultiCloakLauncher::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "MULTICLOAK_LAUNCHER_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	MULTICLOAK_LAUNCHER_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	save = *static_cast<MULTICLOAK_LAUNCHER_SAVELOAD *>(this);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 MultiCloakLauncher::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "MULTICLOAK_LAUNCHER_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	MULTICLOAK_LAUNCHER_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("MULTICLOAK_LAUNCHER_SAVELOAD", buffer, &load);

	*static_cast<MULTICLOAK_LAUNCHER_SAVELOAD *>(this) = load;
		
	result = 1;

Done:	
	return result;
}

//---------------------------------------------------------------------------
//
void MultiCloakLauncher::ResolveAssociations()
{
	if(cloakTargetID)
		OBJLIST->FindObject(cloakTargetID,NONSYSVOLATILEPTR,cloakTarget,ICloakID);
}
//---------------------------------------------------------------------------
//
bool MultiCloakLauncher::checkSupplies()
{
	MPart part = owner.Ptr();
	SINGLE maxPts = part->supplyPointsMax;
	SINGLE curPts = part->supplies;

	if (curPts/maxPts <= cloakShutoff|| (!owner.Ptr()->effectFlags.canShoot()) || (owner.Ptr()->fieldFlags.suppliesLocked()))
	{
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------
//
void MultiCloakLauncher::enableCloak(bool bEnabled)
{
	CQASSERT(bEnabled != bCloakEnabled);
	VOLPTR(ICloak) cloakShip = owner;
	cloakShip->EnableCloak(bEnabled);
	bCloakEnabled = bEnabled;
}
//---------------------------------------------------------------------------
//
void MultiCloakLauncher::checkTechLevel()
{
	// check if we are able to use this special weapon
	TECHNODE techLevel = MGlobals::GetCurrentTechLevel(owner.Ptr()->GetPlayerID());

	// here, we are going to set the caps bit
	MPartNC partNC(owner.Ptr());

	if (techLevel.HasTech(techNode))
	{
		partNC->caps.cloakOk = true;
		partNC->caps.synthesisOk = true;
	}
	else
	{
//		partNC->caps.cloakOk = false;
	}
}


//---------------------------------------------------------------------------
//
inline IBaseObject * createMultiCloakLauncher (PARCHETYPE pArchetype)
{
	MultiCloakLauncher * cloakLauncher = new ObjectImpl<MultiCloakLauncher>;

	cloakLauncher->init(pArchetype);

	return cloakLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------MultiCloakLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE MultiCloakLauncherFactory : public IObjectFactory
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

	BEGIN_DACOM_MAP_INBOUND(MultiCloakLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	MultiCloakLauncherFactory (void) { }

	~MultiCloakLauncherFactory (void);

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

	/* MultiCloakLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
MultiCloakLauncherFactory::~MultiCloakLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void MultiCloakLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE MultiCloakLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_MULTICLOAK_LAUNCHER * data = (BT_MULTICLOAK_LAUNCHER *) _data;

		if (data->type == LC_MULTICLOAK_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 MultiCloakLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * MultiCloakLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createMultiCloakLauncher(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void MultiCloakLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _MultiCloakLauncher : GlobalComponent
{
	MultiCloakLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<MultiCloakLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _MultiCloakLauncher __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End MultiCloakLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------