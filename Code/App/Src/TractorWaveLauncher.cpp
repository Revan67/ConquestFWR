//--------------------------------------------------------------------------//
//                                                                          //
//                                TractorWaveLauncher.cpp                       //
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
#include <DTractorWaveLauncher.h>
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
#include "ISpaceWave.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h> 
#include <MGlobals.h>
#include <DMTechNode.h>
#include <renderer.h>

#include <stdlib.h>

// so that we can use the techtree stuff
using namespace TECHTREE;

struct TractorWaveLauncherArchetype
{
	PARCHETYPE pWaveType;
};

U32 testAddList[MAX_TRACTOR_WAVE_TARGETS];
U32 numTestAdd;
U32 testRemoveList[MAX_TRACTOR_WAVE_TARGETS];
U32 numTestRemove;

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE TractorWaveLauncher : IBaseObject, ILauncher, ISaveLoad,ITerrainSegCallback, TRACTOR_WAVE_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(TractorWaveLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	BT_TRACTOR_WAVE_LAUNCHER * pData;
	TractorWaveLauncherArchetype * archetype;

	OBJPTR<ILaunchOwner> owner;	// person who created us
	U32 ownerID;


	U32 syncAddList[MAX_TRACTOR_WAVE_TARGETS];
	U32 numSyncAdd;

	SINGLE waveTimer;

	//----------------------------------
	//----------------------------------
	
	TractorWaveLauncher (void);

	~TractorWaveLauncher (void);	

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
		MPart part = owner.Ptr();

		ability = USA_TRACTOR_WAVE;
		bSpecialEnabled = part->caps.specialEOAOk && checkSupplies();
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

	/* ITerrainSegCallback*/

	bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos);

	/* TractorWaveLauncher methods */
	
	void init (PARCHETYPE pArchetype, TractorWaveLauncherArchetype * objtype);

	bool checkSupplies();

	void resetTestTargetLists();

	void removeTargetHost(U32 missionID);

	void addTargetHost(U32 missionID);

	void freeAllTargetsHost();

	void removeTargetClient(U32 missionID);

	void addTargetClient(U32 missionID);
};

//---------------------------------------------------------------------------
//
TractorWaveLauncher::TractorWaveLauncher (void) 
{
	bActive = false;
	numTargets = 0;
	numSyncAdd = 0;
	numSyncRemove = 0;
	waveTimer = 0;
}
//---------------------------------------------------------------------------
//
TractorWaveLauncher::~TractorWaveLauncher (void)
{
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::init (PARCHETYPE _pArchetype,TractorWaveLauncherArchetype * objtype)
{
	pData = (BT_TRACTOR_WAVE_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->type == LC_TRACTOR_WAVE_LAUNCH);
	CQASSERT(pData->objClass == OC_LAUNCHER);

	archetype = objtype;
	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::DoSpecialAbility (U32 specialID)
{
}
//---------------------------------------------------------------------------
//
S32 TractorWaveLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 TractorWaveLauncher::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::Render()
{
}
//---------------------------------------------------------------------------
//
BOOL32 TractorWaveLauncher::Update (void)
{
	if(bActive)
	{
		timer+= ELAPSED_TIME;

		//the client will get all of this through sync data.
		if(THEMATRIX->IsMaster())
		{
			if(timer > (pData->waveLifeTime*0.5)+1.0)
			{
				if(timer > pData->durration)//are we done
				{
					freeAllTargetsHost();
					bActive = false;
				}
				else//look for new targets and to release old
				{
					//use obj map test segment ot find targets

					COMPTR<ITerrainMap> map;
					SECTOR->GetTerrainMap(owner.Ptr()->GetSystemID(),map);
					if(map)
					{
						Vector center = owner.Ptr()->GetPosition();
						Vector target = tractorPos;
						Vector testDir = target-center;
						testDir.z = 0;
						if(fabs(testDir.x) < 0.001 && fabs(testDir.y) < 0.001)
							testDir = Vector(0,1,0);
						else
							testDir.fast_normalize();
						Vector startPos = center+(testDir*GRIDSIZE);
						Vector endPos = center+(testDir*owner->GetWeaponRange());

						Vector up(0,0,1);
						Vector side = cross_product(testDir,up);

						resetTestTargetLists();

						GRIDVECTOR start;
						start = startPos;
						GRIDVECTOR end;
						end = endPos;
						map->TestSegment(start,end,this);
						end = endPos+(side*GRIDSIZE);
						map->TestSegment(start,end,this);
						end = endPos-(side*GRIDSIZE);
						map->TestSegment(start,end,this);			
					
						//remove old targets
						U32 count;
						for(count = 0; count < numTestRemove; ++count)
						{
							if(testRemoveList[count])
								removeTargetHost(testRemoveList[count]);
						}
						for(count = 0; count < numTestAdd; ++count)
						{
							addTargetHost(testAddList[count]);
						}
					}
				}
			}
		}
		//update targets motion
		for(U32 count = 0; count < numTargets; ++count)
		{
			VOLPTR(IShipMove) mover = OBJLIST->FindObject(targets[count]);
			if(mover)
			{
				Vector center = owner.Ptr()->GetPosition();
				Vector startPos = mover.Ptr()->GetPosition();
				Vector targetDir = startPos-center;
				targetDir.z = 0;
				if(fabs(targetDir.x) < 0.001 && fabs(targetDir.y) < 0.001)
				{
					Vector target = tractorPos;
					targetDir = target-center;
					targetDir.z = 0;
					if(fabs(targetDir.x) < 0.001 && fabs(targetDir.y) < 0.001)
						targetDir = Vector(0,1,0);
					else
						targetDir.fast_normalize();
				}
				else
				{
					targetDir.fast_normalize();
				}

				Vector targetPoint = center+(targetDir*GRIDSIZE);

				CQASSERT(mover.Ptr()->effectFlags.bTractorWave); //make sure we are still holding this guy
				mover->PushShipTo(ownerID,targetPoint,700);
			}
			else //this target is gone
			{
				if(THEMATRIX->IsMaster())
				{
					removeTargetHost(targets[count]);
				}
			}
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::PhysicalUpdate (SINGLE dt)
{
	if(bActive)
	{
		waveTimer += dt;
		if(waveTimer > pData->waveFrequency)
		{
			waveTimer -= pData->waveFrequency;
			if(archetype->pWaveType)
			{
				IBaseObject * wave = ARCHLIST->CreateInstance(archetype->pWaveType);
				if(wave)
				{
					VOLPTR(ISpaceWave) spWave = wave;
					if(spWave)
					{
						Vector center = owner.Ptr()->GetPosition();
						Vector target = tractorPos;
						Vector testDir = target-center;
						testDir.z = 0;
						if(fabs(testDir.x) < 0.001 && fabs(testDir.y) < 0.001)
							testDir = Vector(0,1,0);
						else
							testDir.fast_normalize();
						Vector startPos = center+(testDir*GRIDSIZE);
						Vector endPos = center+(testDir*owner->GetWeaponRange());

						spWave->StartSpaceWave(startPos, endPos, pData->waveLifeTime, owner.Ptr()->GetSystemID(), owner.Ptr()->GetPlayerID());
					}
					OBJLIST->AddObject(wave);
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE _range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
	ownerID = _owner->GetPartID();
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::InformOfCancel()
{
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::LauncherReceiveOpData(U32 workingID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::LauncherOpCompleted(U32 agentID)
{
}
//---------------------------------------------------------------------------
//
struct TWLSyncHeader
{
	U8 numSyncAdd;
	U8 numSyncRemove;
};
//sync packet format
//  TWLSyncHeader
//  <U32> * numSyncAdd - to be added
//  <U32> * numSyncRemove - to be removed;

U32 TractorWaveLauncher::GetSyncDataSize (void) const
{
	if(numSyncAdd || numSyncRemove)
	{
		return sizeof(TWLSyncHeader) + numSyncAdd*sizeof(U32) + numSyncRemove*sizeof(U32);
	}
	return 0;
}
//---------------------------------------------------------------------------
//
U32 TractorWaveLauncher::GetSyncData (void * buffer)
{
	if(numSyncAdd || numSyncRemove)
	{
		U8 * data = (U8*)buffer;
		TWLSyncHeader * header = (TWLSyncHeader*)buffer;
		header->numSyncAdd = numSyncAdd;
		header->numSyncRemove = numSyncRemove;
		if(numSyncAdd)
		{
			U32 * addBuffer = (U32 *)(data+sizeof(TWLSyncHeader));
			memcpy(addBuffer,syncAddList,sizeof(U32)*numSyncAdd);
		}
		if(numSyncRemove)
		{
			U32 * removeBuffer = (U32 *)(data+sizeof(TWLSyncHeader)+sizeof(U32)*numSyncAdd);
			memcpy(removeBuffer,syncRemoveList,sizeof(U32)*numSyncRemove);
			for(U32 count = 0 ; count <numSyncRemove; ++count)
			{
				VOLPTR(IShipMove) mover = OBJLIST->FindObject(syncRemoveList[count]);
				if(mover)
				{
					CQASSERT(mover.Ptr()->effectFlags.bTractorWave);
					CQTRACE42("Ship Released - Ship:%x  owner:%x",syncRemoveList[count],ownerID); 
					mover->ReleaseShipControl(ownerID);
					mover.Ptr()->effectFlags.bTractorWave = false;
				}
			}
		}
		U32 size = sizeof(TWLSyncHeader) + numSyncAdd*sizeof(U32) + numSyncRemove*sizeof(U32);
		numSyncAdd = 0;
		numSyncRemove = 0;
		return size;
	}
	return 0;
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::PutSyncData (void * buffer, U32 bufferSize)
{
	TWLSyncHeader * header = (TWLSyncHeader*)buffer;
	U8 * data = (U8*)buffer;
	U32 * addBuffer = (U32 *)(data+sizeof(TWLSyncHeader));
	U32 * removeBuffer = (U32 *)(data+sizeof(TWLSyncHeader)+(header->numSyncAdd*sizeof(U32)));

	for(U32 countAdd = 0; countAdd < header->numSyncAdd ; ++countAdd)
	{
		for(U32 countRemove = 0; countRemove < header->numSyncRemove ; ++countRemove)
		{
			if(addBuffer[countAdd] == removeBuffer[countRemove])
			{
				addBuffer[countAdd] = 0;
				removeBuffer[countRemove] = 0;
			}
		}
	}

	U32 count;
	for(count = 0; count < header->numSyncAdd; ++count)
	{
		addTargetClient(addBuffer[count]);
	}
	for(count = 0; count < header->numSyncRemove; ++count)
	{
		removeTargetClient(removeBuffer[count]);
	}
}
//---------------------------------------------------------------------------
//
bool TractorWaveLauncher::TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
{
	if ((info.flags & (TERRAIN_PARKED|TERRAIN_MOVING)) != 0 && (info.flags & TERRAIN_DESTINATION)==0)
	{
		if(MGlobals::GetPlayerFromPartID(info.missionID) != 0 && (!MGlobals::AreAllies(MGlobals::GetPlayerFromPartID(info.missionID),MGlobals::GetPlayerFromPartID(owner.Ptr()->GetPartID()))) )
		{
			IBaseObject * obj = OBJLIST->FindObject(info.missionID);
			if (obj)
			{
				if(obj->effectFlags.bTractorWave)
				{
					// this may be an old target, make sure we keep it
					for(U32 count = 0; count < numTestRemove; ++count)
					{
						if(testRemoveList[count] == info.missionID)
						{
							testRemoveList[count] = 0;//no it will not be removed
						}
					}
				}
				else
				{
					//this is a new target
					if(numTestAdd < MAX_TRACTOR_WAVE_TARGETS)
					{
						bool bAlreadInList = false;
						for(U32 count = 0; count < numTestAdd;++count)
						{
							if(testAddList[count] == info.missionID)
							{
								bAlreadInList = true;
								break;
							}
						}
						if(!bAlreadInList)
						{
							testAddList[numTestAdd] = info.missionID;
							++numTestAdd;
						}
					}
				}
			}
		}
	}
	return true;
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::AttackPosition (const struct GRIDVECTOR * position, bool bSpecial)
{
	if(bSpecial)
	{
		bActive = true;
		tractorPos = *position;
		timer = 0;
		waveTimer = 0;
	}
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::AttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
const TRANSFORM & TractorWaveLauncher::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
bool TractorWaveLauncher::checkSupplies()
{
	MPart part = owner.Ptr();

	return pData->supplyCost <= part->supplies;
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::resetTestTargetLists()
{
	CQASSERT(THEMATRIX->IsMaster());
	numTestAdd = 0;
	if(numTargets)
		memcpy(testRemoveList,targets,numTargets*sizeof(U32));
	numTestRemove = numTargets;
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::removeTargetHost(U32 missionID)
{
	CQASSERT(THEMATRIX->IsMaster());
	//we cannot remove him if our sync buffer is full.  We must not lose track of anyone.
	if(numSyncRemove < MAX_TRACTOR_WAVE_TARGETS)
	{
		for(U32 count = 0 ; count < numTargets; ++count)
		{
			if(targets[count] == missionID)
			{
				syncRemoveList[numSyncRemove] = missionID;
				++numSyncRemove;
				if(count != numTargets-1)
					memmove(&(targets[count]),&(targets[count+1]),(numTargets-count-1)*sizeof(U32));
				numTargets--;
				//we will do the real release on sending the sync data.
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::addTargetHost(U32 missionID)
{
	CQASSERT(THEMATRIX->IsMaster());
	if(numSyncAdd < MAX_TRACTOR_WAVE_TARGETS && numTargets+numSyncRemove < MAX_TRACTOR_WAVE_TARGETS)
	{
		//we need to grab him now on the host so no one else can grab him
		targets[numTargets] = missionID;
		VOLPTR(IShipMove) mover = OBJLIST->FindObject(targets[numTargets]);
		if(mover)
		{
			Vector center = owner.Ptr()->GetPosition();
			Vector startPos = mover.Ptr()->GetPosition();
			Vector targetDir = startPos-center;
			targetDir.z = 0;
			if(fabs(targetDir.x) < 0.001 && fabs(targetDir.y) < 0.001)
			{
				Vector target = tractorPos;
				targetDir = target-center;
				targetDir.z = 0;
				if(fabs(targetDir.x) < 0.001 && fabs(targetDir.y) < 0.001)
					targetDir = Vector(0,1,0);
				else
					targetDir.fast_normalize();
			}
			else
			{
				targetDir.fast_normalize();
			}

			Vector targetPoint = center+(targetDir*GRIDSIZE);

			CQASSERT(!(mover.Ptr()->effectFlags.bTractorWave)); //make sure no one is holding this guy
			mover.Ptr()->effectFlags.bTractorWave = true;
			CQTRACE42("Ship Grabed - Ship:%x  owner:%x",targets[numTargets],ownerID); 
			mover->PushShipTo(ownerID,targetPoint,700);
		}
		++numTargets;

		syncAddList[numSyncAdd] = missionID;
		++numSyncAdd;
	}
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::freeAllTargetsHost()
{
	CQASSERT(THEMATRIX->IsMaster());
	while(numTargets)
	{
		removeTargetHost(targets[numTargets-1]);
	}
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::removeTargetClient(U32 missionID)
{
	for(U32 count = 0 ; count < numTargets; ++count)
	{
		if(targets[count] == missionID)
		{
			VOLPTR(IShipMove) mover = OBJLIST->FindObject(targets[count]);
			if(mover)
			{
				CQASSERT(mover.Ptr()->effectFlags.bTractorWave);
				CQTRACE42("Ship Released - Ship:%x  owner:%x",syncRemoveList[count],ownerID); 
				mover->ReleaseShipControl(ownerID);
				mover.Ptr()->effectFlags.bTractorWave = false;
			}

			if(count != numTargets-1)
				memmove(&(targets[count]),&(targets[count+1]),(numTargets-count-1)*sizeof(U32));
			numTargets--;
			//we will do the real release on sending the sync data.
			break;
		}
	}
}
//---------------------------------------------------------------------------
//
void TractorWaveLauncher::addTargetClient(U32 missionID)
{
	CQASSERT(numTargets < MAX_TRACTOR_WAVE_TARGETS);
	if(numTargets < MAX_TRACTOR_WAVE_TARGETS)
	{
		//we need to grab him now on the host so no one else can grab him
		targets[numTargets] = missionID;
		VOLPTR(IShipMove) mover = OBJLIST->FindObject(targets[numTargets]);
		if(mover)
		{
			Vector center = owner.Ptr()->GetPosition();
			Vector startPos = mover.Ptr()->GetPosition();
			Vector targetDir = startPos-center;
			targetDir.z = 0;
			if(fabs(targetDir.x) < 0.001 && fabs(targetDir.y) < 0.001)
			{
				Vector target = tractorPos;
				targetDir = target-center;
				targetDir.z = 0;
				if(fabs(targetDir.x) < 0.001 && fabs(targetDir.y) < 0.001)
					targetDir = Vector(0,1,0);
				else
					targetDir.fast_normalize();
			}
			else
			{
				targetDir.fast_normalize();
			}

			Vector targetPoint = center+(targetDir*GRIDSIZE);

			CQASSERT(!(mover.Ptr()->effectFlags.bTractorWave)); //make sure no one is holding this guy
			mover.Ptr()->effectFlags.bTractorWave = true;
			CQTRACE42("Ship Grabed - Ship:%x  owner:%x",targets[numTargets],ownerID); 
			mover->PushShipTo(ownerID,targetPoint,700);
		}
		++numTargets;

	}
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createTractorWaveLauncher (PARCHETYPE pArchetype,TractorWaveLauncherArchetype * objtype)
{
	TractorWaveLauncher * tractorWaveLauncher = new ObjectImpl<TractorWaveLauncher>;

	tractorWaveLauncher->init(pArchetype,objtype);

	return tractorWaveLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------TractorWaveLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE TractorWaveLauncherFactory : public IObjectFactory
{
	struct OBJTYPE : TractorWaveLauncherArchetype
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

	BEGIN_DACOM_MAP_INBOUND(TractorWaveLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	TractorWaveLauncherFactory (void) { }

	~TractorWaveLauncherFactory (void);

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

	/* TractorWaveLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
TractorWaveLauncherFactory::~TractorWaveLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void TractorWaveLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE TractorWaveLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_TRACTOR_WAVE_LAUNCHER * data = (BT_TRACTOR_WAVE_LAUNCHER *) _data;

		if (data->type == LC_TRACTOR_WAVE_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);

			if(data->spaceWaveType[0])
			{
				result->pWaveType = ARCHLIST->LoadArchetype(data->spaceWaveType);
			}
			else
			{
				result->pWaveType = NULL;
			}
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 TractorWaveLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * TractorWaveLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createTractorWaveLauncher(objtype->pArchetype,objtype);
}
//-------------------------------------------------------------------
//
void TractorWaveLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _tractorWaveLauncher : GlobalComponent
{
	TractorWaveLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<TractorWaveLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _tractorWaveLauncher __tractorWaveLauncher;

//---------------------------------------------------------------------------------------------
//-------------------------------End TractorWaveLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------