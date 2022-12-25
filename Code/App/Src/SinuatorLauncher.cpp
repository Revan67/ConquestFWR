//--------------------------------------------------------------------------//
//                                                                          //
//                                BuffLauncher.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 2003 By Warthog TX, INC.                  //
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

struct BuffLauncherArchetype
{
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE BuffLauncher : IBaseObject, ILauncher, ISaveLoad, BUFF_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(BuffLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	BT_BUFF_LAUNCHER * pData;

	OBJPTR<ILaunchOwner> owner;	// person who created us

	//----------------------------------
	//----------------------------------
	
	BuffLauncher (void);

	~BuffLauncher (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const;

	virtual BOOL32 Update (void);	// returning FALSE causes destruction

	virtual S32 GetObjectIndex (void) const;

	virtual U32 GetPartID (void) const;

	virtual U32 GetSystemID (void) const
	{
		return owner.ptr->GetSystemID();
	}

	virtual U32 GetPlayerID (void) const
	{
		return owner.ptr->GetPlayerID();
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

	/* BuffLauncher methods */
	
	void init (PARCHETYPE pArchetype);

	bool checkSupplies();
};

//---------------------------------------------------------------------------
//
BuffLauncher::BuffLauncher (void) 
{
}
//---------------------------------------------------------------------------
//
BuffLauncher::~BuffLauncher (void)
{
	if(numTargets)
	{
		for(U32 i = 0; i < numTargets; ++i)
		{
			OBJPTR<IBaseObject> obj;
			OBJLIST->FindObject(targetIDs[i],NONSYSVOLATILEPTR,obj,IBaseObjectID);
			if(obj)
			{
				setEffectFlag(obj,false);
			}
		}
		numTargets = 0;
	}
}
//---------------------------------------------------------------------------
//
void BuffLauncher::init (PARCHETYPE _pArchetype)
{
	pData = (BT_BUFF_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->type == LC_BUFF_LAUNCH);
	CQASSERT(pData->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;

}
//---------------------------------------------------------------------------
//
void BuffLauncher::DoSpecialAbility (U32 specialID)
{

}
//---------------------------------------------------------------------------
//
S32 BuffLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.ptr->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 BuffLauncher::GetPartID (void) const
{
	return owner.ptr->GetPartID();
}
//---------------------------------------------------------------------------
//
BOOL32 BuffLauncher::Update (void)
{
	MPart part(owner.ptr);
	if(part->bReady && SECTOR->SystemInSupply(owner.ptr->GetSystemID(),owner.ptr->GetPlayerID()))
	{
		ObjMapIterator iter(part->systemID,owner.ptr->GetPosition(),pData->rangeRadius*GRIDSIZE);
		while(iter)
		{
			if(numTargets < MAX_BUFF_VICTIMS)
			{
				if ((iter->flags & OM_UNTOUCHABLE) == 0 && (iter->flags & OM_TARGETABLE))
				{
					MPart part = iter->obj;
					if(part.isValid() && part->bReady && (MGlobals::AreAllies(iter->obj->GetPlayerID(),MGlobals::GetPlayerFromPartID(ownerID))) && 
						(!checkEffectFlag(iter->obj)) && (GetGridPosition() - iter->obj->GetGridPosition()) < pData->rangeRadius)
					{
						targetIDs[numTargets] = obj->GetPartID();
						++numTargets;
						setEffectFlag(obj,true);
					}
				}
			}
			else
			{
				break;//already full
			}
			++iter;
		}
		for(i = 0; i < numTargets; ++i)
		{
			OBJPTR<IBaseObject> obj;
			OBJLIST->FindObject(targetIDs[i],NONSYSVOLATILEPTR,obj,IBaseObjectID);
			if(!obj)
			{
				if(i+1 != numTargets)
				{
					memmove(&(targetIDs[i]),&(targetIDs[i+1]),numTargets-i-1);
				}
				--numTargets;//this might skip people to remove but we will get them on the next update
			}
		}
	}
	else
	{
		if(numTargets)
		{
			for(U32 i = 0; i < numTargets; ++i)
			{
				OBJPTR<IBaseObject> obj;
				OBJLIST->FindObject(targetIDs[i],NONSYSVOLATILEPTR,obj,IBaseObjectID);
				if(obj)
				{
					setEffectFlag(obj,false);
				}
			}
			numTargets = 0;
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void BuffLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
}
//---------------------------------------------------------------------------
//
void BuffLauncher::InformOfCancel()
{
}
//---------------------------------------------------------------------------
//
void BuffLauncher::LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void BuffLauncher::LauncherReceiveOpData(U32 workingID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void BuffLauncher::LauncherOpCompleted(U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void BuffLauncher::AttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
void BuffLauncher::DoCreateWormhole(U32 systemID)
{
}

//---------------------------------------------------------------------------
//
const TRANSFORM & BuffLauncher::GetTransform (void) const
{
	return owner.ptr->GetTransform();
}
//---------------------------------------------------------------------------
//
bool BuffLauncher::checkSupplies()
{
	return true;
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createBuffLauncher (PARCHETYPE pArchetype)
{
	BuffLauncher * buffLauncher = new ObjectImpl<BuffLauncher>;

	buffLauncher->init(pArchetype);

	return buffLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------BuffLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE BuffLauncherFactory : public IObjectFactory
{
	struct OBJTYPE : BuffLauncherArchetype
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
			if(pBuffEffectType)
				ARCHLIST->Release(pBuffEffectType, OBJREFNAME);

		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(BuffLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	BuffLauncherFactory (void) { }

	~BuffLauncherFactory (void);

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

	/* BuffLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
BuffLauncherFactory::~BuffLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void BuffLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE BuffLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_BUFF_LAUNCHER * data = (BT_BUFF_LAUNCHER *) _data;

		if (data->type == LC_BUFF_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 BuffLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * BuffLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createBuffLauncher(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void BuffLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _buffLauncher : GlobalComponent
{
	BuffLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<BuffLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _buffLauncher __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End BuffLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------