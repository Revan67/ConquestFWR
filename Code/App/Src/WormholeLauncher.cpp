//--------------------------------------------------------------------------//
//                                                                          //
//                                WormholeLauncher.cpp                           //
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
#include <DWormholeLauncher.h>
#include "ILauncher.h"
#include "IWeapon.h"
#include "DSpaceShip.h"
#include "IGotoPos.h"
#include "OpAgent.h"
#include "IShipMove.h"
#include "ObjMapIterator.h"
#include "CommPacket.h"
#include "sector.h"
#include "ObjSet.h"
#include "camera.h"
#include "IWormholeBlast.h"

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

struct WormLauncherArchetype
{
	PARCHETYPE pWormEffectType;
};

#define CIRC_TIME 4.0

#define LAST_CHANCE_TIME 15

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE WormholeLauncher : IBaseObject, ILauncher, ISaveLoad, WORMHOLE_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(WormholeLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	BT_WORMHOLE_LAUNCHER * pData;

	OBJPTR<ILaunchOwner> owner;	// person who created us
	OBJPTR<IWormholeBlast> hole1;
	OBJPTR<IWormholeBlast> hole2;

	OBJPTR<IShipMove> targets[MAX_WORM_VICTIMS];

	SINGLE radius;
	SINGLE accumedDamage;

	SINGLE circleTime;

	//----------------------------------
	//----------------------------------
	
	WormholeLauncher (void);

	~WormholeLauncher (void);	

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

	virtual void Render();

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
		MPart part = owner.Ptr();

		ability = USA_WORMHOLE;
		bSpecialEnabled = part->caps.createWormholeOk && checkSupplies();
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

	/* WormholeLauncher methods */
	
	void init (PARCHETYPE pArchetype);

	bool checkSupplies();
};

//---------------------------------------------------------------------------
//
WormholeLauncher::WormholeLauncher (void) 
{
	accumedDamage = 0;
}
//---------------------------------------------------------------------------
//
WormholeLauncher::~WormholeLauncher (void)
{
}
//---------------------------------------------------------------------------
//
void WormholeLauncher::init (PARCHETYPE _pArchetype)
{
	pData = (BT_WORMHOLE_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->type == LC_WORMHOLE_LAUNCH);
	CQASSERT(pData->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;
}
//---------------------------------------------------------------------------
//
void WormholeLauncher::DoSpecialAbility (U32 specialID)
{

}
//---------------------------------------------------------------------------
//
S32 WormholeLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 WormholeLauncher::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
void WormholeLauncher::Render()
{
	if(owner && (owner.Ptr()->bSelected || owner.Ptr()->bHighlight) && MGlobals::GetThisPlayer() == owner.Ptr()->GetPlayerID())
	{
		circleTime += OBJLIST->GetRealRenderTime();
		while(circleTime >CIRC_TIME)
		{
			circleTime -= CIRC_TIME;
		}

		BATCH->set_state(RPR_BATCH,true);
		DisableTextures();
		CAMERA->SetModelView();
		BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		PB.Begin(PB_LINES);
		PB.Color3ub(255,0,0);
		SINGLE range = radius/GRIDSIZE;
		Vector relDir =Vector(cos((2*PI*circleTime)/CIRC_TIME)*range*GRIDSIZE,
					sin((2*PI*circleTime)/CIRC_TIME)*range*GRIDSIZE,0);
		Vector oldVect = owner.Ptr()->GetPosition()+relDir;
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		oldVect = owner.Ptr()->GetPosition()-relDir;
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		PB.End();
		drawRangeCircle(range,RGB(255,0,0));
	}
}
//---------------------------------------------------------------------------
//
BOOL32 WormholeLauncher::Update (void)
{
	MPartNC part(owner.Ptr());
	if(part.isValid())
	{
		if(!(part->caps.createWormholeOk))
		{
			if(pData->techNode.raceID)
			{
				if(MGlobals::GetCurrentTechLevel(owner.Ptr()->GetPlayerID()).HasTech(pData->techNode))
				{
					part->caps.createWormholeOk = true;
				}
			}
			else
			{
				part->caps.createWormholeOk = true;
			}
		}
	}

	if(mode == WORM_IDLE && THEMATRIX->IsMaster())
	{
		MPart part(owner.Ptr());
		if(part->bReady)
		{
			accumedDamage += pData->damagePerSec*ELAPSED_TIME;
			if(accumedDamage > 5.0)
			{
				OBJPTR<IWeaponTarget> targ;
				owner.Ptr()->QueryInterface(IWeaponTargetID,targ,NONSYSVOLATILEPTR);
				U32 realDam = accumedDamage;
				if (targ)
					targ->ApplyAOEDamage(owner.Ptr()->GetPlayerID(),realDam);
				accumedDamage -= realDam;
			}
		}
	}

	if(mode == WORM_WORKING && THEMATRIX->IsMaster())
	{
		lastChanceTimer -= ELAPSED_TIME;

		U32 buffer[100];
		U32 numSent = 0;
		for(U32 i = 0; i < numTargets; ++i)
		{
			if(targetIDs[i])
			{
				if(targets[i].Ptr())
				{
					if(((targets[i].Ptr()->GetPosition()-GetPosition()).magnitude_squared() < 1000*1000) || (lastChanceTimer < 0))
					{
						buffer[0] = targetIDs[i];
						owner->LauncherSendOpData(workingID,buffer,sizeof(U32));

						OBJPTR<IPhysicalObject> phys;
						targets[i].Ptr()->QueryInterface(IPhysicalObjectID,phys);
						phys->SetPosition(targetPos,targetSystemID);
						SECTOR->RevealSystem(targetSystemID,targets[i].Ptr()->GetPlayerID());

						THEMATRIX->OperationCompleted(workingID,targetIDs[i]);
						targets[i]->ReleaseShipControl(owner.Ptr()->GetPartID());

						USR_PACKET<USRMOVE> packet;
						packet.objectID[0] = targetIDs[i];
						packet.position.init(targetPos,targetSystemID);
						packet.init(1);
						NETPACKET->Send(HOSTID,0,&packet);
						targetIDs[i] = 0;
						++numSent;
						hole1->Flash();
						hole2->Flash();
					}
				}
				else
				{
					buffer[0] = targetIDs[i];
					owner->LauncherSendOpData(workingID,buffer,sizeof(U32)+1);

					THEMATRIX->OperationCompleted(workingID,targetIDs[i]);

					targetIDs[i] = 0;
					++numSent;
				}
			}
		}
		targetsLeft -= numSent;
		if(!targetsLeft)
		{
			owner->LaunchOpCompleted(this,workingID);
			hole1->CloseUp();
			hole2->CloseUp();
			THEMATRIX->ObjectTerminated(owner.Ptr()->GetPartID(),0);
			mode = WORM_DONE;
		}
		return numSent;
	}

	return 1;
}
//---------------------------------------------------------------------------
//
void WormholeLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
	radius = range;
}
//---------------------------------------------------------------------------
//
void WormholeLauncher::InformOfCancel()
{
}
//---------------------------------------------------------------------------
//
void WormholeLauncher::LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize)
{
	workingID = agentID;
	THEMATRIX->SetCancelState(workingID,false);
	U32 * myBuf = (U32 *)buffer;
	targetsLeft = numTargets= (bufferSize - sizeof(GRIDVECTOR))/(sizeof(U32));
	targetPos = ((GRIDVECTOR *)(&(myBuf[numTargets])))[0];
	for(U32 i = 0; i < numTargets; ++i)
	{
		targetIDs[i] = myBuf[i];
		OBJLIST->FindObject(targetIDs[i],NONSYSVOLATILEPTR,targets[i],IShipMoveID);
		CQASSERT(targets[i].Ptr() && "Beware game may be out of sync");
		if(targets[i].Ptr())
		{
			VOLPTR(IMissionActor) actor=targets[i].Ptr();
//			actor->PrepareTakeover(targetIDs[i],0);
			targets[i]->PushShipTo(owner.Ptr()->GetPartID(),owner.Ptr()->GetPosition(),targets[i]->GetCurrentCruiseVelocity());				
		}
	}

	TRANSFORM trans;
	WormLauncherArchetype * arch = (WormLauncherArchetype *) ARCHLIST->GetArchetypeHandle(pArchetype);
	IBaseObject * obj = ARCHLIST->CreateInstance(arch->pWormEffectType);
	CQASSERT(obj);
	OBJLIST->AddObject(obj);
	obj->QueryInterface(IWormholeBlastID,hole1,NONSYSVOLATILEPTR);
	CQASSERT(hole1);
	trans = owner.Ptr()->GetTransform();
	hole1->InitWormholeBlast(trans,owner.Ptr()->GetSystemID(),radius,owner.Ptr()->GetPlayerID(),false);

	trans.set_identity();
	trans.set_position(targetPos);

	obj = ARCHLIST->CreateInstance(arch->pWormEffectType);
	CQASSERT(obj);
	OBJLIST->AddObject(obj);
	obj->QueryInterface(IWormholeBlastID,hole2,NONSYSVOLATILEPTR);
	CQASSERT(hole2);
	hole2->InitWormholeBlast(trans,targetSystemID,radius,owner.Ptr()->GetPlayerID(),true);	

	lastChanceTimer = LAST_CHANCE_TIME;
	mode = WORM_WORKING;
}
//---------------------------------------------------------------------------
//
void WormholeLauncher::LauncherReceiveOpData(U32 workingID, void * buffer, U32 bufferSize)
{
	U32 * sendBuffer  = (U32 *)buffer;
	bool bMove = (bufferSize != sizeof(U32)+1);
	for(U32 i = 0; i < numTargets; ++i)
	{
		if(targetIDs[i] == sendBuffer[0])
		{
			if(bMove)
			{
				if(targets[i].Ptr())
				{
					OBJPTR<IPhysicalObject> phys;
					targets[i].Ptr()->QueryInterface(IPhysicalObjectID,phys);
					phys->SetPosition(targetPos,targetSystemID);
					SECTOR->RevealSystem(targetSystemID,targets[i].Ptr()->GetPlayerID());
					targets[i]->ReleaseShipControl(owner.Ptr()->GetPartID());
				}

				hole1->Flash();
				hole2->Flash();
			}
			else
			{
				if(targets[i].Ptr())
				{
					targets[i]->ReleaseShipControl(owner.Ptr()->GetPartID());
				}
			}
			targetIDs[i] = 0;
		}
	}
	THEMATRIX->OperationCompleted(workingID,sendBuffer[0]);
	targetsLeft--;
}
//---------------------------------------------------------------------------
//
void WormholeLauncher::LauncherOpCompleted(U32 agentID)
{
	if(THEMATRIX->IsMaster())
	{
		for(U32 i = 0; i < numTargets; ++i)
		{
			if(targetIDs[i])
			{
				if(targets[i].Ptr())
					targets[i]->ReleaseShipControl(owner.Ptr()->GetPartID());
				U32 buffer[10];
				buffer[0] = targetIDs[i];
				owner->LauncherSendOpData(workingID,buffer,sizeof(U32)+1);

				THEMATRIX->OperationCompleted(workingID,targetIDs[i]);

				targetIDs[i] = 0;
			}
		}
	}
	hole1->CloseUp();
	hole2->CloseUp();
	mode = WORM_DONE;
}
//---------------------------------------------------------------------------
//
void WormholeLauncher::AttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
void WormholeLauncher::DoCreateWormhole(U32 systemID)
{
	//our owner should have already ended the op for us
	targetSystemID = systemID;

	if(THEMATRIX->IsMaster())
	{
		IBaseObject * posibleTargets[MAX_WORM_VICTIMS];
		U32 numPosibleTargets = 0;
		ObjMapIterator iter(owner.Ptr()->GetSystemID(),owner.Ptr()->GetPosition(),radius,0);
		while(iter)
		{
			MPart part(iter->obj);
			if(part.isValid() && part->caps.jumpOk && part->bReady && part->caps.moveOk && numPosibleTargets < MAX_WORM_VICTIMS)
			{
				posibleTargets[numPosibleTargets] = iter->obj;
				++numPosibleTargets;
			}
			++iter;
		}
		for(U32 i = 0; i < numPosibleTargets; ++i)
		{
			VOLPTR(IMissionActor) actor=posibleTargets[i];
			U32 id =  posibleTargets[i]->GetPartID();
			actor->PrepareTakeover(id,0);
			if(!THEMATRIX->GetWorkingOp(actor.Ptr()))
			{
				THEMATRIX->FlushOpQueueForUnit(actor.Ptr());
				targetIDs[numTargets] =id;
				posibleTargets[i]->QueryInterface(IShipMoveID,targets[numTargets],NONSYSVOLATILEPTR);

				targets[numTargets]->PushShipTo(owner.Ptr()->GetPartID(),owner.Ptr()->GetPosition(),targets[numTargets]->GetCurrentCruiseVelocity());
			
				++numTargets;
			}
		}
		targetsLeft = numTargets;
		if(numTargets)
		{
			COMPTR<ITerrainMap> map;
			SECTOR->GetTerrainMap(targetSystemID,map);
			RECT rect;
			SECTOR->GetSystemRect(targetSystemID,&rect);
			U32 width = rect.right-rect.left;
			targetPos = Vector(((rand()%1000)/1000.0f)*width,((rand()%1000)/1000.0f)*width,0);
			while(!(map->IsGridEmpty(targetPos,0,true)))
			{
				targetPos = Vector(((rand()%1000)/1000.0f)*width,((rand()%1000)/1000.0f)*width,0);
			}
			
			ObjSet set;
			set.objectIDs[0] = owner.Ptr()->GetPartID();
			U8 buffer[256];
			U32 * intBuf = (U32 *)(&buffer);
			for(U32 i  = 0; i < numTargets; ++i)
			{
				intBuf[i] = targetIDs[i];
				set.objectIDs[i+1] = targetIDs[i];
			}
			set.numObjects = numTargets+1;
			((GRIDVECTOR *)(&(intBuf[numTargets])))[0] = targetPos;
			workingID = owner->CreateLauncherOp(this,set,&buffer,numTargets*sizeof(U32)+sizeof(GRIDVECTOR));
			THEMATRIX->SetCancelState(workingID,false);
			lastChanceTimer = LAST_CHANCE_TIME;
			mode = WORM_WORKING;

			TRANSFORM trans;
			WormLauncherArchetype * arch = (WormLauncherArchetype *) ARCHLIST->GetArchetypeHandle(pArchetype);
			IBaseObject * obj = ARCHLIST->CreateInstance(arch->pWormEffectType);
			CQASSERT(obj);
			OBJLIST->AddObject(obj);
			obj->QueryInterface(IWormholeBlastID,hole1,NONSYSVOLATILEPTR);
			CQASSERT(hole1);
			trans = owner.Ptr()->GetTransform();
			hole1->InitWormholeBlast(trans,owner.Ptr()->GetSystemID(),radius,owner.Ptr()->GetPlayerID(),false);

			trans.set_identity();
			trans.set_position(targetPos);

			obj = ARCHLIST->CreateInstance(arch->pWormEffectType);
			CQASSERT(obj);
			OBJLIST->AddObject(obj);
			obj->QueryInterface(IWormholeBlastID,hole2,NONSYSVOLATILEPTR);
			CQASSERT(hole2);
			hole2->InitWormholeBlast(trans,targetSystemID,radius,owner.Ptr()->GetPlayerID(),true);
		
		}
		else
		{
			THEMATRIX->ObjectTerminated(owner.Ptr()->GetPartID(),0);
		}
	}

}

//---------------------------------------------------------------------------
//
const TRANSFORM & WormholeLauncher::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
bool WormholeLauncher::checkSupplies()
{
	return true;
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createWormholeLauncher (PARCHETYPE pArchetype)
{
	WormholeLauncher * wormholeLauncher = new ObjectImpl<WormholeLauncher>;

	wormholeLauncher->init(pArchetype);

	return wormholeLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------WormholeLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE WormholeLauncherFactory : public IObjectFactory
{
	struct OBJTYPE : WormLauncherArchetype
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
			if(pWormEffectType)
				ARCHLIST->Release(pWormEffectType, OBJREFNAME);

		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(WormholeLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	WormholeLauncherFactory (void) { }

	~WormholeLauncherFactory (void);

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

	/* WormholeLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
WormholeLauncherFactory::~WormholeLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void WormholeLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE WormholeLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_WORMHOLE_LAUNCHER * data = (BT_WORMHOLE_LAUNCHER *) _data;

		if (data->type == LC_WORMHOLE_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
			result->pWormEffectType = ARCHLIST->LoadArchetype("WORMEFF!!S_WORM");
			if(result->pWormEffectType)
				ARCHLIST->AddRef(result->pWormEffectType, OBJREFNAME);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 WormholeLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * WormholeLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createWormholeLauncher(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void WormholeLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _wormholeLauncher : GlobalComponent
{
	WormholeLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<WormholeLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _wormholeLauncher __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End WormholeLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------