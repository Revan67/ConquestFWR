//--------------------------------------------------------------------------//
//                                                                          //
//                          SystemBuffLauncher.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 2004 By Warthog TX, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header:
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "SuperTrans.h"
#include "ObjList.h"
#include "sfx.h"

#include "Mission.h"
#include "IMissionActor.h"
#include "Startup.h"
#include "ArchHolder.h"
#include "TerrainMap.h"
#include "MPart.h"
#include "TObject.h"
#include <DBuffLauncher.h>
#include "ILauncher.h"
#include "IWeapon.h"
#include "DSpaceShip.h"
#include "IGotoPos.h"
#include "OpAgent.h"
#include "IShipMove.h"
#include "CommPacket.h"
#include "sector.h"
#include "ObjSet.h"
#include "IFabricator.h"
#include "IPlanet.h"
#include "ObjMapIterator.h"

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

struct SystemBuffLauncherArchetype
{
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE SystemBuffLauncher : IBaseObject, ILauncher, ISaveLoad, SYSTEM_BUFF_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(SystemBuffLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	BT_SYSTEM_BUFF_LAUNCHER * pData;

	OBJPTR<ILaunchOwner> owner;	// person who created us
	U32 ownerID;

	bool activeSettings[MAX_PLAYERS];

	//----------------------------------
	//----------------------------------
	
	SystemBuffLauncher (void);

	~SystemBuffLauncher (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const;

	virtual BOOL32 Update (void);	// returning FALSE causes destruction

	virtual S32 GetObjectIndex (void) const;

	virtual U32 GetPartID (void) const;

	virtual U32 GetSystemID (void) const
	{
		return owner.Ptr()->GetSystemID();
	}

	virtual U32 GetPlayerID (void) const
	{
		return owner.Ptr()->GetPlayerID();
	}

	/* ILauncher methods */

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range);
	

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial)
	{
	}

	virtual void AttackObject (IBaseObject * obj);

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
	{}

	virtual void WormAttack (IBaseObject * obj)
	{};

	virtual void DoCreateWormhole(U32 systemID);

	virtual const bool TestFightersRetracted (void) const 
	{ 
		return true;
	}

	virtual void SetFighterStance(FighterStance stance)
	{
	}

	virtual void HandlePreTakeover (U32 newMissionID, U32 troopID)
	{
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	virtual void DoSpecialAbility (U32 specialID);

	virtual void DoSpecialAbility (IBaseObject *obj)
	{
	}

	virtual void DoCloak (void)
	{
	}
	
	virtual void SpecialAttackObject (IBaseObject * obj)
	{
	}

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bSpecialEnabled)
	{
		ability = USA_NONE;
	}

	virtual const U32 GetApproxDamagePerSecond (void) const
	{
		return 0;
	}

	virtual void InformOfCancel();

	virtual void LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize);

	virtual void LauncherReceiveOpData(U32 agentID, void * buffer, U32 bufferSize);

	virtual void LauncherOpCompleted(U32 agentID);

	virtual bool CanCloak(){return false;};

	virtual bool IsToggle() {return false;};

	virtual bool CanToggle(){return false;};

	virtual bool IsOn() {return false;};

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const
	{
		return 0;
	}

	virtual U32 GetSyncData (void * buffer)
	{
		return 0;
	}

	virtual void PutSyncData (void * buffer, U32 bufferSize)
	{
	}

	virtual void OnAllianceChange (U32 allyMask)
	{
	}

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file)
	{
		return TRUE;
	}

	virtual BOOL32 Load (struct IFileSystem * file)
	{
		return FALSE;
	}
	
	virtual void ResolveAssociations()
	{
	}

	/* SystemBuffLauncher methods */
	
	void init (PARCHETYPE pArchetype);

	bool isTarget (U32 playerID);

	bool checkEffectState(U32 playerID);

	void setEffectState(U32 playerID, bool bSetting);
};

//---------------------------------------------------------------------------
//
SystemBuffLauncher::SystemBuffLauncher (void) 
{
	memset(activeSettings,0,sizeof(activeSettings));
}
//---------------------------------------------------------------------------
//
SystemBuffLauncher::~SystemBuffLauncher (void)
{
	for(U32 i = 0; i < MAX_PLAYERS; ++i)
	{
		if(activeSettings[i])
		{
			setEffectState(i+1,false);		
		}
	}
}
//---------------------------------------------------------------------------
//
void SystemBuffLauncher::init (PARCHETYPE _pArchetype)
{
	pData = (BT_SYSTEM_BUFF_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->type == LC_SYSTEM_BUFF_LAUNCHER);
	CQASSERT(pData->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;

}
//---------------------------------------------------------------------------
//
void SystemBuffLauncher::DoSpecialAbility (U32 specialID)
{

}
//---------------------------------------------------------------------------
//
S32 SystemBuffLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 SystemBuffLauncher::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
BOOL32 SystemBuffLauncher::Update (void)
{
	bool bInSupply = SECTOR->SystemInSupply(owner.Ptr()->GetSystemID(),owner.Ptr()->GetPlayerID());
	for(U32 i = 0; i < MAX_PLAYERS; ++i)
	{
		MPart part = owner.Ptr();
		if(bInSupply && part.isValid() && part->bReady && isTarget(i+1))//turn it on
		{
			if(!checkEffectState(i+1))
			{
				setEffectState(i+1,true);
			}
		}
		else//turn it off
		{
			if(activeSettings[i])
			{
				setEffectState(i+1,false);		
			}
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void SystemBuffLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
	ownerID = _owner->GetPartID();
}
//---------------------------------------------------------------------------
//
void SystemBuffLauncher::InformOfCancel()
{
}
//---------------------------------------------------------------------------
//
void SystemBuffLauncher::LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void SystemBuffLauncher::LauncherReceiveOpData(U32 workingID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void SystemBuffLauncher::LauncherOpCompleted(U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void SystemBuffLauncher::AttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
void SystemBuffLauncher::DoCreateWormhole(U32 systemID)
{
}

//---------------------------------------------------------------------------
//
const TRANSFORM & SystemBuffLauncher::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
bool SystemBuffLauncher::isTarget (U32 playerID)
{
	switch(pData->targetType)
	{
	case BT_SYSTEM_BUFF_LAUNCHER::ALL_PLAYERS:
		return true;
	case BT_SYSTEM_BUFF_LAUNCHER::ALLIES_ONLY:
		{
			return MGlobals::AreAllies(playerID,owner.Ptr()->GetPlayerID());
		}
		break;
	case BT_SYSTEM_BUFF_LAUNCHER::ENEMIES_ONLY:
		{
			return !MGlobals::AreAllies(playerID,owner.Ptr()->GetPlayerID());
		}
		break;
	}
	return false;
}
//---------------------------------------------------------------------------
//
bool SystemBuffLauncher::checkEffectState(U32 playerID)
{
	switch(pData->buffType)
	{
	case BT_SYSTEM_BUFF_LAUNCHER::INTEL_CENTER_EFFECT:
		{
			return SECTOR->GetSectorEffects(playerID,owner.Ptr()->GetSystemID())->bIntelegenceEffect;
		}
		break;
	}
	return true;
}
//---------------------------------------------------------------------------
//
void SystemBuffLauncher::setEffectState(U32 playerID, bool bSetting)
{
	activeSettings[playerID-1] = bSetting;
	switch(pData->buffType)
	{
	case BT_SYSTEM_BUFF_LAUNCHER::INTEL_CENTER_EFFECT:
		{
			SECTOR->GetSectorEffects(playerID,owner.Ptr()->GetSystemID())->bIntelegenceEffect = bSetting;
		}
		break;
	}
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createSystemBuffLauncher (PARCHETYPE pArchetype)
{
	SystemBuffLauncher * buffLauncher = new ObjectImpl<SystemBuffLauncher>;

	buffLauncher->init(pArchetype);

	return buffLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------SystemBuffLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE SystemBuffLauncherFactory : public IObjectFactory
{
	struct OBJTYPE : SystemBuffLauncherArchetype
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

	BEGIN_DACOM_MAP_INBOUND(SystemBuffLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	SystemBuffLauncherFactory (void) { }

	~SystemBuffLauncherFactory (void);

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

	/* SystemBuffLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
SystemBuffLauncherFactory::~SystemBuffLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void SystemBuffLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE SystemBuffLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_SYSTEM_BUFF_LAUNCHER * data = (BT_SYSTEM_BUFF_LAUNCHER *) _data;

		if (data->type == LC_SYSTEM_BUFF_LAUNCHER)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 SystemBuffLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * SystemBuffLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createSystemBuffLauncher(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void SystemBuffLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _systemBuffLauncher : GlobalComponent
{
	SystemBuffLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<SystemBuffLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _systemBuffLauncher __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End SystemBuffLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------