//--------------------------------------------------------------------------//
//                                                                          //
//                                CloakLaunch.cpp                           //
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

#include "CloakLauncher.h"

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
CloakLauncher::CloakLauncher (void) 
{
	cloakLaunchIndex = -1;
	bCloakEnabled = false;
	bPrepareToggle = false;
}
//---------------------------------------------------------------------------
//
CloakLauncher::~CloakLauncher (void)
{
}
//---------------------------------------------------------------------------
//
void CloakLauncher::init (PARCHETYPE _pArchetype)
{
	const BT_CLOAK_LAUNCHER * data = (const BT_CLOAK_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(data);
	CQASSERT(data->type == LC_CLOAK_LAUNCH);
	CQASSERT(data->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;
}
//---------------------------------------------------------------------------
//
void CloakLauncher::DoCloak (void)
{
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
bool CloakLauncher::CanCloak()
{
	return checkSupplies();
}
//---------------------------------------------------------------------------
//
S32 CloakLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();// cloakLaunchIndex;
}
//---------------------------------------------------------------------------
//
U32 CloakLauncher::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
BOOL32 CloakLauncher::Update (void)
{
	checkTechLevel();

	if(THEMATRIX->IsMaster())
	{
		if (bCloakEnabled)
		{
			cloakSupplyCount += cloakSupplyUse;

			if (cloakSupplyCount >= 1)
			{
				owner->UseSupplies(cloakSupplyCount);

				// do we need to turn off cloaking?
				if (!checkSupplies())
				{
					bPrepareToggle = true;
				}

				cloakSupplyCount = 0.0f;
			}

		}
	}

	return 1;
}
//---------------------------------------------------------------------------
//
void CloakLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	const BT_CLOAK_LAUNCHER * data = (const BT_CLOAK_LAUNCHER *) ARCHLIST->GetArchetypeData(pArchetype);

	cloakSupplyCount = 0.0f;
	cloakSupplyUse = data->cloakSupplyUse/REALTIME_FRAMERATE;
	cloakShutoff =	 data->cloakShutoff;
	techNode =		 data->techNode;

	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
}
//---------------------------------------------------------------------------
//
const TRANSFORM & CloakLauncher::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
BOOL32 CloakLauncher::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "CLOAK_LAUNCHER_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	CLOAK_LAUNCHER_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	save = *static_cast<CLOAK_LAUNCHER_SAVELOAD *>(this);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 CloakLauncher::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "CLOAK_LAUNCHER_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	CLOAK_LAUNCHER_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("CLOAK_LAUNCHER_SAVELOAD", buffer, &load);

	*static_cast<CLOAK_LAUNCHER_SAVELOAD *>(this) = load;
		
	result = 1;

Done:	
	return result;
}

//---------------------------------------------------------------------------
//
void CloakLauncher::ResolveAssociations()
{
}
//---------------------------------------------------------------------------
//
bool CloakLauncher::checkSupplies()
{
	MPart part = owner.Ptr();
	SINGLE maxPts = part->supplyPointsMax;
	SINGLE curPts = part->supplies;

	if (curPts/maxPts <= cloakShutoff || (!owner.Ptr()->effectFlags.canShoot()) || (owner.Ptr()->fieldFlags.suppliesLocked()))
	{
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------
//
void CloakLauncher::enableCloak(bool bEnable)
{
	CQASSERT(bEnable != bCloakEnabled);
	VOLPTR(ICloak) cloakShip = owner;
	cloakShip->EnableCloak(bEnable);
	bCloakEnabled = bEnable;
}
//---------------------------------------------------------------------------
//
void CloakLauncher::checkTechLevel()
{
	// check if we are able to use this special weapon
	TECHNODE techLevel = MGlobals::GetCurrentTechLevel(owner.Ptr()->GetPlayerID());

	// here, we are going to set the caps bit
	MPartNC partNC(owner.Ptr());

	if (techLevel.HasTech(techNode))
	{
		partNC->caps.cloakOk = true;
	}
}


//---------------------------------------------------------------------------
//
inline IBaseObject * createCloakLauncher (PARCHETYPE pArchetype)
{
	CloakLauncher * cloakLauncher = new ObjectImpl<CloakLauncher>;

	cloakLauncher->init(pArchetype);

	return cloakLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------CloakLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE CloakLauncherFactory : public IObjectFactory
{
	struct OBJTYPE 
	{
		PARCHETYPE pArchetype;

		void * operator new (size_t size)
		{
			return calloc(size,1);
		}

		OBJTYPE (void)
		{
		}

		~OBJTYPE (void)
		{
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(CloakLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	CloakLauncherFactory (void) { }

	~CloakLauncherFactory (void);

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

	/* CloakLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
CloakLauncherFactory::~CloakLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void CloakLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE CloakLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_CLOAK_LAUNCHER * data = (BT_CLOAK_LAUNCHER *) _data;

		if (data->type == LC_CLOAK_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 CloakLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * CloakLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createCloakLauncher(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void CloakLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _cloakLauncher : GlobalComponent
{
	CloakLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<CloakLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _cloakLauncher __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End CloakLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------