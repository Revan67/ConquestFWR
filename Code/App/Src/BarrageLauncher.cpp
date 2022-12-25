//--------------------------------------------------------------------------//
//                                                                          //
//                                BarrageLauncher.cpp                       //
//                                                                          //
//                  COPYRIGHT (C) 2004 BY Warthog                           //
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
#include <DBarrageLauncher.h>
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
#include "MeshRender.h"
#include "IBlast.h"
#include "stdio.h"
#include "IBlast.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h> 
#include <MGlobals.h>
#include <DMTechNode.h>
#include <renderer.h>
#include <IHardPoint.h>

#include <stdlib.h>

// so that we can use the techtree stuff
using namespace TECHTREE;

struct BarrageLauncherArchetype
{
	PARCHETYPE pBoltType;
	PARCHETYPE pFlashType;
};

#define MAX_BARRAGE_SEGMENTS 32
struct BarragePointSeg
{
	HardpointInfo  hardpointinfo1;
	INSTANCE_INDEX pointIndex1;
	HardpointInfo  hardpointinfo2;
	INSTANCE_INDEX pointIndex2;
};

#define MAX_BARRAGE_TARGETS 8
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE BarrageLauncher : IBaseObject, ILauncher, ISaveLoad,BARRAGE_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(BarrageLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	BT_BARRAGE_LAUNCHER * pData;
	BarrageLauncherArchetype * archetype;

	OBJPTR<ILaunchOwner> owner;	// person who created us
	U32 ownerID;

	SINGLE timer;
	SINGLE fireTime;

	U32 numSegments;
	BarragePointSeg barrageSegs[MAX_BARRAGE_SEGMENTS];

	U32 numTargets;
	U32 targetIds[MAX_BARRAGE_TARGETS];
	U32 targetSegs[MAX_BARRAGE_TARGETS];
	U32 targetNumSegs[MAX_BARRAGE_TARGETS];

	//----------------------------------
	//----------------------------------
	
	BarrageLauncher (void);

	~BarrageLauncher (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const;

	virtual BOOL32 Update (void);	// returning FALSE causes destruction
	
	virtual void PhysicalUpdate (SINGLE dt);

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
	

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial);

	virtual void AttackObject (IBaseObject * obj);

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
	{}

	virtual void WormAttack (IBaseObject * obj)
	{};

	virtual void DoCreateWormhole(U32 systemID){};

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
	virtual U32 GetSyncDataSize (void) const;

	virtual U32 GetSyncData (void * buffer);

	virtual void PutSyncData (void * buffer, U32 bufferSize);

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

	/* BarrageLauncher methods */
	
	void init (PARCHETYPE pArchetype, BarrageLauncherArchetype * objtype);

	bool checkSupplies();

	bool checkTargetType(IBaseObject * obj);
};

//---------------------------------------------------------------------------
//
BarrageLauncher::BarrageLauncher (void) 
{
	timer = 0;
	fireTime = 0;
	numSegments = 0;
}
//---------------------------------------------------------------------------
//
BarrageLauncher::~BarrageLauncher (void)
{
}
//---------------------------------------------------------------------------
//
void BarrageLauncher::init (PARCHETYPE _pArchetype,BarrageLauncherArchetype * objtype)
{
	pData = (BT_BARRAGE_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->type == LC_BARRAGE_LAUNCH);
	CQASSERT(pData->objClass == OC_LAUNCHER);

	archetype = objtype;
	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;
}
//---------------------------------------------------------------------------
//
void BarrageLauncher::DoSpecialAbility (U32 specialID)
{
}
//---------------------------------------------------------------------------
//
S32 BarrageLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 BarrageLauncher::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
void BarrageLauncher::Render()
{
/*	BATCH->set_state(RPR_BATCH,false);
	DisableTextures();
	CAMERA->SetModelView();
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	PB.Begin(PB_LINES);
	PB.Color4ub(255,255,255,255);
	for(U32 i = 0; i < numSegments;++i)
	{
		TRANSFORM transHardpoint(barrageSegs[i].hardpointinfo1.orientation, barrageSegs[i].hardpointinfo1.point);
		TRANSFORM translocal = ENGINE->get_transform(barrageSegs[i].pointIndex1);

		TRANSFORM trans = translocal.multiply(transHardpoint);
		Vector pos1 = translocal.rotate_translate(barrageSegs[i].hardpointinfo1.point);
		PB.Vertex3f(pos1.x,pos1.y,pos1.z);
		Vector pos2 = pos1+trans.get_k()*4000;
		PB.Vertex3f(pos2.x,pos2.y,pos2.z);

		TRANSFORM transHardpoint2(barrageSegs[i].hardpointinfo2.orientation, barrageSegs[i].hardpointinfo2.point);
		TRANSFORM translocal2 = ENGINE->get_transform(barrageSegs[i].pointIndex2);

		TRANSFORM trans2 = translocal2.multiply(transHardpoint2);

		pos1 = translocal2.rotate_translate(barrageSegs[i].hardpointinfo2.point);
		PB.Vertex3f(pos1.x,pos1.y,pos1.z);
		pos2 = pos1+trans2.get_k()*4000;
		PB.Vertex3f(pos2.x,pos2.y,pos2.z);
	}
	PB.End();
*/}
//---------------------------------------------------------------------------
//
BOOL32 BarrageLauncher::Update (void)
{
	timer += ELAPSED_TIME;
	if(checkSupplies() && timer > 1.0)
	{
		timer = 0;
		numTargets = 0;
		MPart part(owner.Ptr());
		if(part->bReady)
		{
			ObjMapIterator iter(part->systemID,owner.Ptr()->GetPosition(),pData->rangeRadius*GRIDSIZE);
			while(iter)
			{
				if ((iter->flags & OM_UNTOUCHABLE) == 0 && (iter->flags & OM_TARGETABLE))
				{
					MPart part = iter->obj;
					if(part.isValid() && part->bReady && (checkTargetType(iter->obj)) && 
						(GetGridPosition() - iter->obj->GetGridPosition()) < pData->rangeRadius)
					{
						VOLPTR(IWeaponTarget) targ = iter->obj;
						if(targ)
						{
							if(owner->UseSupplies(pData->supplyCost))
							{
								targ->ApplyAOEDamage(owner.Ptr()->GetPlayerID(),pData->damagePerSec);
								if(numTargets < MAX_BARRAGE_TARGETS)
								{
									Vector targetPos =  iter->obj->GetPosition();
									U32 goodSegs = 0;
									U32 numGoodSegs = 0;
									for(U32 count = 0; count <  numSegments; ++count)
									{
										TRANSFORM transHardpoint(barrageSegs[count].hardpointinfo1.orientation, barrageSegs[count].hardpointinfo1.point);
										TRANSFORM translocal = ENGINE->get_transform(barrageSegs[count].pointIndex1);

										TRANSFORM trans = translocal.multiply(transHardpoint);

										Vector hpPos = translocal.rotate_translate(barrageSegs[count].hardpointinfo1.point);
										Vector dir1 = (targetPos-hpPos).fast_normalize();
										Vector dir2 = trans.get_k();
										dir2.fast_normalize();
										if(dot_product(dir1,dir2) > 0.4)
										{
											goodSegs |= (0x00000001 << count);
											numGoodSegs++;
										}
									}
									if(goodSegs)
									{
										targetIds[numTargets] = iter->obj->GetPartID();
										targetSegs[numTargets] = goodSegs;
										targetNumSegs[numTargets] = numGoodSegs;
										numTargets++;
									}
								}
							}
						}
					}
				}
				++iter;
			}
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void BarrageLauncher::PhysicalUpdate (SINGLE dt)
{
	fireTime += dt;
	while(fireTime > pData->refirePeriod)
	{
		fireTime -= pData->refirePeriod;
		for(U32 count = 0; count < numTargets; ++count)
		{
			IBaseObject * targ = OBJLIST->FindObject(targetIds[count]);
			if(targ && targetNumSegs[count])
			{
				U32 seg = rand()%targetNumSegs[count];
				for(U32 segCounter = 0; segCounter < MAX_BARRAGE_SEGMENTS ;++segCounter)
				{
					if(targetSegs[count] & (0x00000001<<segCounter))
					{
						if(!seg)
						{
							seg = segCounter;
							break;
						}
						--seg;
					}
				}
				SINGLE delta = (((SINGLE)(rand()%10000))/10000.0f);
				Vector pos1 = ENGINE->get_transform(barrageSegs[seg].pointIndex1).rotate_translate(barrageSegs[seg].hardpointinfo1.point);
				Vector pos2 = ENGINE->get_transform(barrageSegs[seg].pointIndex2).rotate_translate(barrageSegs[seg].hardpointinfo2.point);
				Transform trans;
				trans.translation = delta*(pos1-pos2)+pos2;
				
				Vector k = -(targ->GetPosition()-trans.translation).fast_normalize();
				Vector j(0,0,1);
				Vector i = cross_product(j,k);
				i.fast_normalize();
				j = cross_product(k,i);
				j.fast_normalize();
				trans.set_i(i);
				trans.set_j(j);
				trans.set_k(k);

				IBaseObject * obj = ARCHLIST->CreateInstance(archetype->pBoltType);
				if (obj)
				{
					OBJPTR<IWeapon> bolt;

					OBJLIST->AddObject(obj);
					if (obj->QueryInterface(IWeaponID,bolt) != 0)
					{
						bolt->InitWeapon(owner.Ptr(), trans, targ, IWF_ALWAYS_MISS);
					}
				}
				if(archetype->pFlashType != 0)
				{
					IBaseObject * obj = ARCHLIST->CreateInstance(archetype->pFlashType);
					if(obj)
					{
						OBJPTR<IBlast> blast;
						obj->QueryInterface(IBlastID,blast);
						if(blast != 0)
						{
							blast->InitBlast(trans,owner.Ptr()->GetSystemID(),NULL,0.1);
							OBJLIST->AddObject(obj);
						}
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void BarrageLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE _range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
	ownerID = _owner->GetPartID();

	U32 masterSegments = 1;
	numSegments = 0;
	char buffer[256];
	while(numSegments < MAX_BARRAGE_SEGMENTS)
	{
		HardpointInfo  hardpointinfo;
		INSTANCE_INDEX pointIndex;
		HardpointInfo  hardpointinfo2;
		INSTANCE_INDEX pointIndex2;
		char letter[2];
		letter[0] = 'A';
		letter[1] = 0;
		sprintf(buffer,"\\hp_cannon%d_%s",masterSegments,letter);
		FindHardpointSilent(buffer, pointIndex, hardpointinfo, ownerIndex);
		if(pointIndex != -1)
		{
			while(numSegments < MAX_BARRAGE_SEGMENTS)
			{
				++(letter[0]);
				sprintf(buffer,"\\hp_cannon%d_%s",masterSegments,letter);
				FindHardpointSilent(buffer, pointIndex2, hardpointinfo2, ownerIndex) ;
				if(pointIndex2 != -1)
				{
					barrageSegs[numSegments].hardpointinfo1 = hardpointinfo;
					barrageSegs[numSegments].pointIndex1 = pointIndex;
					barrageSegs[numSegments].hardpointinfo2 = hardpointinfo2;
					barrageSegs[numSegments].pointIndex2 = pointIndex2;
					++numSegments;
					hardpointinfo = hardpointinfo2;
					pointIndex = pointIndex2;
				}
				else
					break;
			}
			++masterSegments;
		}
		else 
			break;
	}
}
//---------------------------------------------------------------------------
//
void BarrageLauncher::InformOfCancel()
{
}
//---------------------------------------------------------------------------
//
void BarrageLauncher::LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void BarrageLauncher::LauncherReceiveOpData(U32 workingID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void BarrageLauncher::LauncherOpCompleted(U32 agentID)
{
}
//---------------------------------------------------------------------------
//
U32 BarrageLauncher::GetSyncDataSize (void) const
{
	return 0;
}
//---------------------------------------------------------------------------
//
U32 BarrageLauncher::GetSyncData (void * buffer)
{
	return 0;
}
//---------------------------------------------------------------------------
//
void BarrageLauncher::PutSyncData (void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void BarrageLauncher::AttackPosition (const struct GRIDVECTOR * position, bool bSpecial)
{
}
//---------------------------------------------------------------------------
//
void BarrageLauncher::AttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
const TRANSFORM & BarrageLauncher::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
bool BarrageLauncher::checkSupplies()
{
	MPart part = owner.Ptr();

	return pData->supplyCost <= part->supplies;
}
//---------------------------------------------------------------------------
//
bool BarrageLauncher::checkTargetType(IBaseObject * obj)
{
	if(!obj)
		return false;
	return 	!MGlobals::AreAllies(obj->GetPlayerID(),MGlobals::GetPlayerFromPartID(ownerID));
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createBarrageLauncher (PARCHETYPE pArchetype,BarrageLauncherArchetype * objtype)
{
	BarrageLauncher * barrageLauncher = new ObjectImpl<BarrageLauncher>;

	barrageLauncher->init(pArchetype,objtype);

	return barrageLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------BarrageLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE BarrageLauncherFactory : public IObjectFactory
{
	struct OBJTYPE : BarrageLauncherArchetype
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
			if (pBoltType)
				ARCHLIST->Release(pBoltType,OBJREFNAME);
			if (pFlashType)
				ARCHLIST->Release(pFlashType,OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(BarrageLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	BarrageLauncherFactory (void) { }

	~BarrageLauncherFactory (void);

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

	/* BarrageLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
BarrageLauncherFactory::~BarrageLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void BarrageLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE BarrageLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_BARRAGE_LAUNCHER * data = (BT_BARRAGE_LAUNCHER *) _data;

		if (data->type == LC_BARRAGE_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);

			result->pBoltType = ARCHLIST->LoadArchetype(data->weaponType);
			CQASSERT(result->pBoltType);
			ARCHLIST->AddRef(result->pBoltType, OBJREFNAME);

			result->pFlashType = ARCHLIST->LoadArchetype(data->flashType);
			CQASSERT(result->pFlashType);
			ARCHLIST->AddRef(result->pFlashType, OBJREFNAME);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 BarrageLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * BarrageLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createBarrageLauncher(objtype->pArchetype,objtype);
}
//-------------------------------------------------------------------
//
void BarrageLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _barrageLauncher : GlobalComponent
{
	BarrageLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<BarrageLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _barrageLauncher __barrageLauncher;

//---------------------------------------------------------------------------------------------
//-------------------------------End BarrageLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------