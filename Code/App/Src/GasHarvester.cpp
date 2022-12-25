//--------------------------------------------------------------------------//
//                                                                          //
//                               GasHarvester.cpp                           //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/GasHarvester.cpp 10    2/25/00 7:00p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TPlatform.h"
#include "Startup.h"
#include "MGlobals.h"
#include "ObjList.h"
#include "GenData.h"
#include "IHardPoint.h"
#include "OpAgent.h"
#include "IMineable.h"
#include "IHarvest.h"

#include <DGasHarvester.h>

#include <Physics.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <HeapObj.h>

#define HOST_CHECK THEMATRIX->IsMaster()

#define COMP_OP(operation) { U32 tempID = workingID; workingID = 0; THEMATRIX->OperationCompleted(tempID,operation);}

#define NUM_LINE_SEGS 40
#define CIRC_TIME 4.0

Vector harvestRangeLines[NUM_LINE_SEGS];

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE GasHarvester :Platform<GASHARVESTER_SAVELOAD, GASHARVESTER_INIT>,
											BASE_GASHARVESTER_SAVELOAD
{
	BEGIN_MAP_INBOUND(GasHarvester)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IPlatform)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IExtent)
	END_MAP()

	SaveNode		saveNode;
	LoadNode		loadNode;
	ResolveNode		resolveNode;
	UpdateNode		updateNode;
	PhysUpdateNode  physUpdateNode;
	RenderNode		renderNode;
	PreTakeoverNode preTakeoverNode;

	OBJPTR<IShuttle> miner[NUM_MINERS];
	OBJPTR<IBaseObject> dockLocker;

	HardpointInfo  shippointinfo;
	INSTANCE_INDEX shipPointIndex;

	U32 maxMinerLoad;
	SINGLE mineNebTime;

	SINGLE mineRadius;

	U32 mapTextID;
	SINGLE circleTime;

	GasHarvester (void);

	virtual ~GasHarvester (void);	// See ObjList.cpp

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);
	
	virtual void Render ();

	virtual void MapRender();

	/* IExplosionOwner methods */

	virtual void OnChildDeath (INSTANCE_INDEX child)
	{
	}


	// IPlatform method

	virtual U32 GetPlanetID ()
	{
		CQASSERT(0 && "GasHarvester do not support this");
		return 0;
	};

	virtual U32 GetSlotID ()
	{
		CQASSERT(0 && "GasHarvester do not support this");
		return 0;
	};

	virtual TRANSFORM GetShipTransform ();

	virtual void SetRallyPoint (const struct NETGRIDVECTOR & point)
	{
		CQBOMB0("What!?");
	}

	virtual bool IsDockLocked()
	{
		return (dockLockerID != 0);
	}

	virtual void LockDock(IBaseObject * locker)
	{
		if(THEMATRIX->IsMaster())
			CQASSERT(!dockLockerID);
		if(!dockLockerID)
		{
			dockLocker = locker;
			dockLockerID = locker->GetPartID();
		}
	}

	virtual void UnlockDock(IBaseObject * locker)
	{
		CQASSERT(dockLockerID);
		if(THEMATRIX->IsMaster())
			CQASSERT(locker->GetPartID() == dockLockerID);
		if(locker->GetPartID() == dockLockerID)
		{
			dockLocker = NULL;
			dockLockerID = 0;
		}
	}

	virtual void FreePlanetSlot()
	{
		CQASSERT(0 && "GasHarvester do not support this");
	}

	virtual void ParkYourself (const TRANSFORM & _trans, U32 planetID, U32 slotID);
	
	virtual U32 GetNumDocking ()
	{
		return numDocking;
	}

	virtual void IncNumDocking ()
	{
		++numDocking;
	}

	virtual void DecNumDocking ()
	{
		CQASSERT(numDocking);
		--numDocking;
	}

	/* Platform methods */


	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "GASHARVESTER_SAVELOAD";
	}

	virtual void * getViewStruct (void)				// must be overriden implemented by derived class
	{
		return static_cast<BASE_GASHARVESTER_SAVELOAD *>(this);
	}

	/* GasHarvester methods */

	bool findMiningTarg(Vector startPos,U32 & nebTarg,U32 & nebIndex,Vector & position);

	bool alreadyTargeted(U32 partID,U32 index);

	bool initGasHarvester (const GASHARVESTER_INIT & data);

	BOOL32 updateGasHarvester ();

	void physUpdateGasHarvester(SINGLE dt);

	void renderGasHarvest ();

	void preTakeover (U32 newMissionID);

	void save (GASHARVESTER_SAVELOAD & save);

	void load (GASHARVESTER_SAVELOAD & load);

	void resolve (void);
};
//---------------------------------------------------------------------------
//
GasHarvester::GasHarvester (void) :
			saveNode(this, CASTSAVELOADPROC(save)),
			loadNode(this, CASTSAVELOADPROC(load)),
			resolveNode(this, ResolveProc(resolve)),
			updateNode(this, UpdateProc(updateGasHarvester)),
			physUpdateNode(this, PhysUpdateProc(physUpdateGasHarvester)),
			renderNode(this,RenderProc(renderGasHarvest)),
			preTakeoverNode(this, PreTakeoverProc(preTakeover))
{
	circleTime = 0;
}
//---------------------------------------------------------------------------
//
GasHarvester::~GasHarvester (void)
{
	for(U32 i = 0; i < NUM_MINERS; ++i)
	{
		if(miner[i])
			delete miner[i].ptr; //not in obj list
	}
}
//---------------------------------------------------------------------------
//
void GasHarvester::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	Platform<GASHARVESTER_SAVELOAD, GASHARVESTER_INIT>::TestVisible(defaults, currentSystem, currentPlayer);

	for(U32 i = 0; i < NUM_MINERS; ++i)
	{
		if(miner[i])
			miner[i].ptr->TestVisible(defaults, currentSystem, currentPlayer);
	}
}
//-------------------------------------------------------------------
//
void GasHarvester::Render (void)
{
	Platform<GASHARVESTER_SAVELOAD, GASHARVESTER_INIT>::Render();

	for(U32 i = 0; i < NUM_MINERS; ++i)
	{
		if(miner[i])
			miner[i].ptr->Render();
	}
}
//---------------------------------------------------------------------------
//
void GasHarvester::ParkYourself (const TRANSFORM & _transform, U32 planetID, U32 slotID)
{
	CQASSERT(0 && "Gas Harvesters don't park");
}
//---------------------------------------------------------------------------
//
TRANSFORM GasHarvester::GetShipTransform()
{
	TRANSFORM trans;
	trans.set_orientation(shippointinfo.orientation);
	trans.set_position(shippointinfo.point);
	trans = transform.multiply(trans);
	return trans;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//

bool GasHarvester::initGasHarvester (const GASHARVESTER_INIT & data)
{
	for(U32 i = 0; i < NUM_MINERS; ++i)
	{
		if(data.pData->drone_release[i].builderType[0])
		{
			IBaseObject *obj = ARCHLIST->CreateInstance(data.pData->drone_release[i].builderType);
			if (obj)
			{
				obj->QueryInterface(IShuttleID,miner[i]);
				CQASSERT(miner[i] != 0);
				miner[i]->InitBuildShip(this);
				miner[i]->SetSystemID(systemID);
				if (data.pData->drone_release[i].hardpoint[0])
				{
					INSTANCE_INDEX engIndex;
					HardpointInfo hpInfo;
					FindHardpoint(data.pData->drone_release[i].hardpoint, engIndex, hpInfo, instanceIndex);
					if(miner[i])
					{
						miner[i]->SetStartHardpoint(hpInfo,engIndex);
					}
				}
				minerInfo[i].state = MINER_PARKED;
				minerInfo[i].nebulaID =0;
				minerInfo[i].load = 0;
			}	
		}
	}
	maxMinerLoad = data.pData->maxMinerLoad;
	mineNebTime = data.pData->mineNebTime;
	mapTextID = data.rangeTexID;
	mineRadius = data.pData->mineRadius;

	dockLockerID = 0;

	if (data.pData->ship_hardpoint[0])
		FindHardpoint(data.pData->ship_hardpoint, shipPointIndex, shippointinfo, instanceIndex);

	return true;
}
//---------------------------------------------------------------------------
//
bool GasHarvester::alreadyTargeted(U32 partID,U32 index)
{
	for(U32 i = 0; i < NUM_MINERS; ++i)
	{
		if(miner[i])
		{
			if(minerInfo[i].nebulaID == partID && minerInfo[i].locationIndex == index)
				return true;
		}
	}
	return false;
}
//---------------------------------------------------------------------------
//
bool GasHarvester::findMiningTarg(Vector startPos,U32 & nebTarg,U32 & nebIndex,Vector & position)
{
	IBaseObject * obj = OBJLIST->GetObjectList();
	OBJPTR<IMineable> mineable;
	SINGLE currentDist = mineRadius*mineRadius;
	bool bFound = false;
	while(obj)
	{
		if((obj->objClass == OC_NEBULA) && (obj->GetSystemID() == systemID))
		{
			obj->QueryInterface(IMineableID,mineable);
			if(mineable)
			{
				U32 maxSquares = mineable->GetMineableSquares();
				for(U32 i = 0; i < maxSquares; ++i)
				{
					if(mineable->GetSquareResources(i))
					{
						if(!(alreadyTargeted(obj->GetPartID(),i)))
						{
							SINGLE newDist = (mineable->GetSquarePosition(i)-GetPosition()).magnitude_squared();
							if(newDist < currentDist)
							{
								currentDist = newDist;
								nebTarg = obj->GetPartID();
								nebIndex = i;
								position = mineable->GetSquarePosition(i);
								bFound = true;
							}
						}
					}
				}
			}
		}
		obj = obj->next;
	}
	return bFound;
}
//---------------------------------------------------------------------------
//
BOOL32 GasHarvester::updateGasHarvester()
{
	for(U32 i = 0; i < NUM_MINERS;++i)
	{
		if(miner[i])
		{
			miner[i].ptr->Update();
			if(minerInfo[i].state == MINER_WORKING)
				minerInfo[i].mineTime += ELAPSED_TIME;
		}
	}
	if(MGlobals::IsUpdateFrame(dwMissionID))
	{
		for(i = 0; i < NUM_MINERS; ++i)
		{
			if(miner[i])
			{
				switch(minerInfo[i].state)
				{
				case MINER_PARKED:
					{
						if(supplies != supplyPointsMax)
						{
							U32 nebTarg, nebIndex;
							Vector position;
							if(findMiningTarg(miner[i].ptr->GetPosition(),nebTarg,nebIndex,position))
							{
								minerInfo[i].state = MINER_MOVING_TO_WORK;
								minerInfo[i].nebulaID = nebTarg;
								minerInfo[i].locationIndex = nebIndex;
								miner[i]->IdleAtPos(position);
							}
						}
					}
					break;
				case MINER_WORKING:
					{
						if(minerInfo[i].mineTime > mineNebTime)
						{
							IBaseObject * obj = OBJLIST->FindObject(minerInfo[i].nebulaID);
							CQASSERT(obj);
							OBJPTR<IMineable> mineable;
							obj->QueryInterface(IMineableID,mineable);
							CQASSERT(mineable);
							U32 resources = mineable->GetSquareResources(minerInfo[i].locationIndex);
							if(resources)
							{
								U32 taken = __min(maxMinerLoad-minerInfo[i].load,resources);
								if(taken)
								{
									minerInfo[i].load += taken;
									mineable->SetSquareResources(minerInfo[i].locationIndex,resources-taken);
								}
							}
							if(minerInfo[i].load == maxMinerLoad)
							{
								minerInfo[i].state = MINER_MOVING_TO_PARK;
								minerInfo[i].nebulaID = 0;
								minerInfo[i].locationIndex = 0;
								miner[i]->Return();
							}
							else
							{
								U32 nebTarg, nebIndex;
								Vector position;
								if(findMiningTarg(miner[i].ptr->GetPosition(),nebTarg,nebIndex,position))
								{
									minerInfo[i].state = MINER_MOVING_TO_WORK;
									minerInfo[i].nebulaID = nebTarg;
									minerInfo[i].locationIndex = nebIndex;
									miner[i]->IdleAtPos(position);
								}
								else
								{
									minerInfo[i].state = MINER_MOVING_TO_PARK;
									minerInfo[i].nebulaID = 0;
									minerInfo[i].locationIndex = 0;
									miner[i]->Return();
								}
							}
						}
					}
					break;
				case MINER_MOVING_TO_WORK:
					{
						if(miner[i]->AtTarget())
						{
							minerInfo[i].state = MINER_WORKING;
							minerInfo[i].mineTime = 0.0;
						}
					}
					break;
				case MINER_MOVING_TO_PARK:
					{
						if(miner[i]->AtTarget())
						{
							minerInfo[i].state = MINER_PARKED;
							if(THEMATRIX->IsMaster())
							{
								supplies += minerInfo[i].load;
								if(supplies > supplyPointsMax)
									supplies = supplyPointsMax;
							}
							minerInfo[i].load = 0;
						}
					}
					break;
				}
			}
		}
	}
	return true;
}
//---------------------------------------------------------------------------
//
void GasHarvester::physUpdateGasHarvester( SINGLE dt)
{
	for(U32 i = 0; i < NUM_MINERS;++i)
		if(miner[i])
			miner[i].ptr->PhysicalUpdate(dt);
	circleTime += dt;
	while(circleTime >CIRC_TIME)
	{
		circleTime -= CIRC_TIME;
	}
}
//---------------------------------------------------------------------------
//
void GasHarvester::renderGasHarvest ()
{
	if((systemID == SECTOR->GetCurrentSystem()) && bHighlight)
	{
		BATCH->set_state(RPR_BATCH,true);
		DisableTextures();
		CAMERA->SetModelView();
		BATCH->set_render_state(D3DRENDERSTATE_ZENABLE,FALSE);
		BATCH->set_render_state(D3DRENDERSTATE_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRENDERSTATE_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRENDERSTATE_DESTBLEND,D3DBLEND_ONE);
		PB.Begin(PB_LINES);
		PB.Color3ub(64,128,64);
		Vector relDir =Vector(cos((2*PI*circleTime)/CIRC_TIME)*mineRadius,
					sin((2*PI*circleTime)/CIRC_TIME)*mineRadius,0);
		Vector oldVect = transform.translation+relDir;
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		oldVect = transform.translation-relDir;
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		oldVect = harvestRangeLines[0]+transform.translation;
		for(U32 i = 1 ; i <= NUM_LINE_SEGS; ++i)
		{
			PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
			oldVect = harvestRangeLines[i%NUM_LINE_SEGS]+transform.translation;
			PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		}
		PB.End();
	}
}
//---------------------------------------------------------------------------
//
void GasHarvester::preTakeover(U32 newMissionID)
{
	if(dockLocker)
	{
		OBJPTR<IHarvest> harvester;
		dockLocker->QueryInterface(IHarvestID,harvester);
		if(harvester)
		{
			harvester->DockTaken();
		}
	}
}
//-------------------------------------------------------------------
//
void GasHarvester::MapRender()
{
	Platform<GASHARVESTER_SAVELOAD, GASHARVESTER_INIT>::MapRender();

	if(mapTextID && bHighlight)
	{
		SINGLE supRange = mineRadius*GRIDSIZE;
		Vector pt[4];
		pt[0] = transform.translation-supRange*Vector(1,0,0)+supRange*Vector(0,1,0);
		pt[1] = transform.translation-supRange*Vector(1,0,0)-supRange*Vector(0,1,0);
		pt[2] = transform.translation+supRange*Vector(1,0,0)-supRange*Vector(0,1,0);
		pt[3] = transform.translation+supRange*Vector(1,0,0)+supRange*Vector(0,1,0);

		BATCH->set_state(RPR_BATCH,true);

		SetupDiffuseBlend(mapTextID,TRUE);
		BATCH->set_state(RPR_STATE_ID,mapTextID);
		BATCH->set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRENDERSTATE_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRENDERSTATE_DESTBLEND,D3DBLEND_ONE);

		PB.Begin(PB_QUADS);
		PB.Color3ub(255,255,255);
		PB.TexCoord2f(0,0); PB.Vertex3f(pt[0].x,pt[0].y,0);
		PB.TexCoord2f(1,0); PB.Vertex3f(pt[1].x,pt[1].y,0);
		PB.TexCoord2f(1,1); PB.Vertex3f(pt[2].x,pt[2].y,0);
		PB.TexCoord2f(0,1); PB.Vertex3f(pt[3].x,pt[3].y,0);
		PB.End();

		BATCH->set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE,FALSE);
		BATCH->set_state(RPR_STATE_ID,0);		
	}
}
//---------------------------------------------------------------------------
//
void GasHarvester::save (GASHARVESTER_SAVELOAD & save)
{
	save.gasHarvesterSaveload = *static_cast<BASE_GASHARVESTER_SAVELOAD *>(this);

	for(U32 i = 0; i < NUM_MINERS; ++i)
	{
		if(miner[i])
			save.minerTransform[i] = miner[i].GetBase()->GetTransform();
	}
}
//---------------------------------------------------------------------------
//
void GasHarvester::load (GASHARVESTER_SAVELOAD & load)
{
 	*static_cast<BASE_GASHARVESTER_SAVELOAD *>(this) = load.gasHarvesterSaveload;
	for(U32 i = 0; i < NUM_MINERS; ++i)
	{
		if(miner[i])
		{
			miner[i]->SetSystemID(systemID);
			miner[i]->SetTransform(load.minerTransform[i]);
		}
	}
}
//---------------------------------------------------------------------------
//
void GasHarvester::resolve (void)
{
	if(dockLockerID)
		dockLocker = OBJLIST->FindObject(dockLockerID);
}
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createGasHarvester (const GASHARVESTER_INIT & data)
{
	GasHarvester * obj = new ObjectImpl<GasHarvester>;

	obj->FRAME_init(data);
	if (obj->initGasHarvester(data) == false)
	{
		delete obj;
		obj = 0;
	}

	return obj;
}
//------------------------------------------------------------------------------------------
//
GASHARVESTER_INIT::~GASHARVESTER_INIT (void)					// free archetype references
{
	TMANAGER->ReleaseTextureRef(rangeTexID);
}
//------------------------------------------------------------------------------------------
//---------------------------GeneralPlat Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE GasHarvesterFactory : public IObjectFactory
{
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(GasHarvesterFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	GasHarvesterFactory (void) { }

	~GasHarvesterFactory (void);

	void init (void);

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "GasHarvesterFactory");
	}

	/* IObjectFactory methods */

	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);

	/* GeneralPlatFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
GasHarvesterFactory::~GasHarvesterFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void GasHarvesterFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE GasHarvesterFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	GASHARVESTER_INIT * result = 0;

	if (objClass == OC_PLATFORM)
	{
		BT_GASHARVESTER_DATA * data = (BT_GASHARVESTER_DATA *) _data;
		
		if (data->type == PC_GASHARVESTER)
		{
			for(int i = 0; i < NUM_LINE_SEGS; ++i)
			{
				harvestRangeLines[i] = Vector(cos((2*PI*i)/NUM_LINE_SEGS)*data->mineRadius,
					sin((2*PI*i)/NUM_LINE_SEGS)*data->mineRadius,0);
			}
			
			result = new GASHARVESTER_INIT;

			if (result->loadPlatformArchetype(data, ARCHLIST->GetArchetype(szArchname)))
			{
				// do something here...
			}
			else
				goto Error;

			result->rangeTexID = TMANAGER->CreateTextureFromFile("SupplyTex.tga", TEXTURESDIR, DA::TGA,PF_4CC_DAA4);		
		}

		goto Done;
	}



Error:
	delete result;
	result = 0;
Done:
	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 GasHarvesterFactory::DestroyArchetype (HANDLE hArchetype)
{
	GASHARVESTER_INIT * objtype = (GASHARVESTER_INIT *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * GasHarvesterFactory::CreateInstance (HANDLE hArchetype)
{
	GASHARVESTER_INIT * objtype = (GASHARVESTER_INIT *)hArchetype;
	return createGasHarvester(*objtype);
}
//-------------------------------------------------------------------
//
void GasHarvesterFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	GASHARVESTER_INIT * objtype = (GASHARVESTER_INIT *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _gasharvestfactory : GlobalComponent
{
	GasHarvesterFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<GasHarvesterFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _gasharvestfactory __plat;

//--------------------------------------------------------------------------//
//----------------------------End GasHarvester.cpp-------------------------------//
//--------------------------------------------------------------------------//
