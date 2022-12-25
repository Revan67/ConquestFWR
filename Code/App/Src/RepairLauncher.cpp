//--------------------------------------------------------------------------//
//                                                                          //
//                                RepairLauncher.cpp                        //
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

#include "ObjList.h"

#include "Startup.h"
#include "TObject.h"
#include <DRepairLauncher.h>
#include "ILauncher.h"
#include "IWeapon.h"
#include "ObjMapIterator.h"
#include "MPart.h"
#include "IAdmiral.h"
#include <TComponent.h>
#include <TSmartPointer.h>
#include <IConnection.h> 

struct RepairLauncherArchetype
{
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE RepairLauncher : IBaseObject, ILauncher, ISaveLoad, REPAIR_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(RepairLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	BT_REPAIR_LAUNCHER * pData;

	OBJPTR<ILaunchOwner> owner;	// person who created us
	U32 ownerID;
	SINGLE updateTimer;

	//----------------------------------
	//----------------------------------
	
	RepairLauncher (void);

	~RepairLauncher (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const;

	virtual BOOL32 Update (void);	// returning FALSE causes destruction

	virtual void Render (void);

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

	/* RepairLauncher methods */
	
	void init (PARCHETYPE pArchetype);
};

//---------------------------------------------------------------------------
//
RepairLauncher::RepairLauncher (void) 
{
	updateTimer = 0;
}
//---------------------------------------------------------------------------
//
RepairLauncher::~RepairLauncher (void)
{
}
//---------------------------------------------------------------------------
//
void RepairLauncher::init (PARCHETYPE _pArchetype)
{
	pData = (BT_REPAIR_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->type == LC_REPAIR_LAUNCH);
	CQASSERT(pData->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;
}
//---------------------------------------------------------------------------
//
void RepairLauncher::DoSpecialAbility (U32 specialID)
{

}
//---------------------------------------------------------------------------
//
S32 RepairLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 RepairLauncher::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
#define UPDATE_SECONDS 2

BOOL32 RepairLauncher::Update (void)
{
	MPartNC part(owner.Ptr());
	if(part->bReady)
	{
		updateTimer += ELAPSED_TIME;
		if(updateTimer > UPDATE_SECONDS)
		{
			updateTimer -= UPDATE_SECONDS;
			ObjMapIterator iter(part->systemID,owner.Ptr()->GetPosition(),pData->rangeRadius*GRIDSIZE);
			while(iter)
			{
				if ((iter->flags & OM_UNTOUCHABLE) == 0 && (iter->flags & OM_TARGETABLE))
				{
					MPartNC targetPart = iter->obj;
					if(targetPart.isValid() && targetPart->bReady && MGlobals::AreAllies(iter->obj->GetPlayerID(),owner.Ptr()->GetPlayerID()) &&
						targetPart->hullPoints < targetPart->hullPointsMax && (GetGridPosition() - iter->obj->GetGridPosition()) < pData->rangeRadius)
					{
						S32 neededHull = targetPart->hullPointsMax-targetPart->hullPoints;
						neededHull = __min(neededHull,UPDATE_SECONDS*pData->repairRate);
						S32 maxAvail = part->supplies/pData->repairCostPerPoint;
						neededHull = __min(maxAvail,neededHull);
						if(neededHull > 0)
						{
							targetPart->hullPoints += neededHull;
							S32 cost = neededHull*pData->repairCostPerPoint;
							part->supplies = __max(0,part->supplies-cost);
						}
					}
				}
				++iter;
			}
			MPartNC part = owner.ptr;
			if(part->fleetID)
			{
				VOLPTR(IAdmiral)admiral = OBJLIST->FindObject(part->fleetID);
				if(admiral)
				{
					if(admiral->IsInFleetRepairMode())
					{
						U32 fleetList[MAX_SELECTED_UNITS];
						U32 numMembers = admiral->GetFleetMembers(fleetList);
						for(U32 count = 0; count < numMembers; ++ count)
						{
							IBaseObject * obj = OBJLIST->FindObject(fleetList[count]);
							if(obj)
							{
								MPartNC targetPart = obj;
								if(targetPart.isValid() && targetPart->bReady &&
									targetPart->hullPoints < targetPart->hullPointsMax)
								{
									S32 neededHull = targetPart->hullPointsMax-targetPart->hullPoints;
									neededHull = __min(neededHull,UPDATE_SECONDS*pData->repairRate);
									S32 maxAvail = part->supplies/pData->repairCostPerPoint;
									neededHull = __min(maxAvail,neededHull);
									if(neededHull > 0)
									{
										targetPart->hullPoints += neededHull;
										S32 cost = neededHull*pData->repairCostPerPoint;
										part->supplies = __max(0,part->supplies-cost);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return 1;
}
//---------------------------------------------------------------------------
//
void RepairLauncher::Render (void)
{
	if(owner.ptr->bSelected)
	{
		owner.ptr->drawRangeCircle(pData->rangeRadius,RGB(255,0,0));
	}
}
//---------------------------------------------------------------------------
//
void RepairLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
	ownerID = _owner->GetPartID();
}
//---------------------------------------------------------------------------
//
void RepairLauncher::InformOfCancel()
{
}
//---------------------------------------------------------------------------
//
void RepairLauncher::LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void RepairLauncher::LauncherReceiveOpData(U32 workingID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void RepairLauncher::LauncherOpCompleted(U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void RepairLauncher::AttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
void RepairLauncher::DoCreateWormhole(U32 systemID)
{
}

//---------------------------------------------------------------------------
//
const TRANSFORM & RepairLauncher::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createRepairLauncher (PARCHETYPE pArchetype)
{
	RepairLauncher * repairLauncher = new ObjectImpl<RepairLauncher>;

	repairLauncher->init(pArchetype);

	return repairLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------RepairLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE RepairLauncherFactory : public IObjectFactory
{
	struct OBJTYPE : RepairLauncherArchetype
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

	BEGIN_DACOM_MAP_INBOUND(RepairLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	RepairLauncherFactory (void) { }

	~RepairLauncherFactory (void);

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

	/* RepairLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
RepairLauncherFactory::~RepairLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void RepairLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE RepairLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_REPAIR_LAUNCHER * data = (BT_REPAIR_LAUNCHER *) _data;

		if (data->type == LC_REPAIR_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 RepairLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * RepairLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createRepairLauncher(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void RepairLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _repairLauncher : GlobalComponent
{
	RepairLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<RepairLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _repairLauncher __repairLauncher;

//---------------------------------------------------------------------------------------------
//-------------------------------End RepairLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------