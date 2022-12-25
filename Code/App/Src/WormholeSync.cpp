//--------------------------------------------------------------------------//
//                                                                          //
//                                WormholeSync.cpp                           //
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
#include "IWeapon.h"
#include "DSpaceShip.h"
#include "IWormGenerator.h"
#include "IGotoPos.h"
#include "OpAgent.h"
#include "IShipMove.h"
#include "ObjMapIterator.h"
#include "CommPacket.h"
#include "sector.h"
#include "TObjMission.h"
#include "TObjTrans.h"
#include "TobjFrame.h"
#include "ObjSet.h"
#include "anim2d.h"
#include "Camera.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h> 
#include <MGlobals.h>
#include <DMTechNode.h>

#include <stdlib.h>

#define NUM_FLARES 4
#define FLAIR_CIRC_COUNT 45

static SINGLE flairCircleX[FLAIR_CIRC_COUNT];
static SINGLE flairCircleY[FLAIR_CIRC_COUNT];

struct WORMHOLE_SYNC_INIT
{
	BT_WORMHOLE_SYNC * pData;
	S32 archIndex;
	PARCHETYPE pArchetype;
	PARCHETYPE wormEntryBlast;
	AnimArchetype * wormholeAnm;
};

#define WORMHOLE_ANM_COUNT 30
#define INNER_RING 1500.0


struct WormNetInit
{
	U32 numTargets;
	GRIDVECTOR pos;
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE WormholeSync : ObjectMission<ObjectTransform<ObjectFrame<IBaseObject,struct WORMHOLE_SYNC_SAVELOAD,struct WORMHOLE_SYNC_INIT> > >, ISaveLoad, IWormholeSync, BASE_WORMHOLE_SYNC_SAVELOAD
{
	BEGIN_MAP_INBOUND(WormholeSync)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IWormholeSync)
	END_MAP()

	ReceiveOpDataNode	receiveOpDataNode;

	BT_WORMHOLE_SYNC * pData;

	OBJPTR<IShipMove> targetShips[MAX_WORM_VICTIMS];
	OBJPTR<IBaseObject> ship1,ship2;

	AnimInstance anm[WORMHOLE_ANM_COUNT];

	SINGLE flashScale;

	PARCHETYPE wormEntryBlast;

	//----------------------------------
	//----------------------------------
	
	WormholeSync (void);

	~WormholeSync (void);	

	/* IBaseObject methods */

	virtual BOOL32 Update (void);	// returning FALSE causes destruction

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual U32 GetPlayerID (void) const
	{
		return 0;
	}

	virtual void Render (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void CastVisibleArea();

	/* IWormholeSync */

	virtual void CreateAt(IBaseObject * targ1, IBaseObject * targ2, U32 targID);

	virtual void ResolveWormhole(IBaseObject * owner);

	virtual void InitWormhole(IBaseObject * owner,U32 partID);

	/* IMissionActor */

	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize);

	// called in response to OpAgent::SendOperationData()
	virtual void receiveOperationData (U32 agentID, void *buffer, U32 bufferSize);

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

	/* WormholeSync methods */
	
	void init (WORMHOLE_SYNC_INIT& initData);

	bool checkSupplies();

	void renderAnm();

	void renderFlare();
};

//---------------------------------------------------------------------------
//
WormholeSync::WormholeSync (void) :
		receiveOpDataNode(this, ReceiveOpDataProc(receiveOperationData))
{
}
//---------------------------------------------------------------------------
//
WormholeSync::~WormholeSync (void)
{
	if(systemID)
		OBJMAP->RemoveObjectFromMap(this,systemID,OBJMAP->GetMapSquare(systemID,transform.translation));
}
//---------------------------------------------------------------------------
//
void WormholeSync::CreateAt(IBaseObject * targ1, IBaseObject * targ2, U32 targID)
{
	targ1->QueryInterface(IBaseObjectID,ship1,NONSYSVOLATILEPTR);
	shipID1 = targ1->GetPartID();
	targ2->QueryInterface(IBaseObjectID,ship2,NONSYSVOLATILEPTR);
	shipID2 = targ2->GetPartID();
	targetSystemID = targID;

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
	
	transform.translation = (targ1->GetPosition()+targ2->GetPosition())/2;

	systemID = targ1->GetSystemID();

	inEffect = true;

	OBJLIST->AddObject(this);

	Vector anmPos = transform.translation;
	for(U32 anmCnt = 0; anmCnt < WORMHOLE_ANM_COUNT; ++anmCnt)
	{
		Vector pos(anmPos.x+(((rand()%2000)/1000.0)-1.0)*INNER_RING,
			anmPos.y+(((rand()%2000)/1000.0)-1.0)*INNER_RING,
			anmPos.z+(((rand()%2000)/1000.0)-1.0)*INNER_RING);
		anm[anmCnt].SetPosition(pos);
		anm[anmCnt].Randomize();
	}
	OBJMAP->AddObjectToMap(this,systemID,OBJMAP->GetMapSquare(systemID,transform.translation));
	effectScale = 0;
}
//---------------------------------------------------------------------------
//
void WormholeSync::ResolveWormhole(IBaseObject * owner)
{
}
//---------------------------------------------------------------------------
//
void WormholeSync::InitWormhole(IBaseObject * owner,U32 partID)
{
	inEffect = false;
	dwMissionID = partID;
	playerID = MGlobals::GetPlayerFromPartID(partID);
	OBJLIST->AddPartID(this, dwMissionID);
	flashScale = 0;
}
//---------------------------------------------------------------------------
//
void WormholeSync::OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
{
	workingID = agentID;

	WormNetInit * myBuf = (WormNetInit *) buffer;
	
	numTargets = 0;

	targetPos = myBuf->pos;
			
	numDeleted = myBuf->numTargets;
}
//---------------------------------------------------------------------------
//
void WormholeSync::receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
{
	if(numDeleted > numTargets)
	{
		targetShipIDs[numTargets] = *((U32 *)buffer);
		OBJLIST->FindObject(targetShipIDs[numTargets],NONSYSVOLATILEPTR,targetShips[numTargets],IShipMoveID);
		if(targetShips[numTargets])
		{
			OBJPTR<IMissionActor> actor;
			targetShips[numTargets].ptr->QueryInterface(IMissionActorID,actor);
			actor->PrepareTakeover(targetShipIDs[numTargets],0);
			targetShips[numTargets]->PushShipTo(dwMissionID,GetPosition(),20000);
		}
		++numTargets;
		if(numDeleted == numTargets)
			numDeleted = 0;
	}
	else
	{
		U32 partID = *((U32 *)buffer);
		if(partID == dwMissionID)
		{
			THEMATRIX->OperationCompleted2(workingID,dwMissionID);
//			OBJLIST->DeferredDestruction(dwMissionID);
			inEffect = false;
			deleteEffect = true;
		}
		else
		{
			U32 i;
			for(i = 0; i < numTargets; ++i)
			{
				if(targetShipIDs[i] == partID)
					break;
			}
			CQASSERT(i < numTargets);
			if(targetShips[i])
			{
				OBJPTR<IPhysicalObject> phys;
				targetShips[i].ptr->QueryInterface(IPhysicalObjectID,phys);
				phys->SetPosition(targetPos,targetSystemID);
				targetShips[i]->ReleaseShipControl(dwMissionID);
			}
			THEMATRIX->OperationCompleted(workingID,targetShipIDs[i]);
			targetShipIDs[i] = 0;
			++numDeleted;
			flashScale = 1.0;
			IBaseObject * obj = CreateBlast(wormEntryBlast,targetShips[i].ptr->GetTransform(),targetSystemID,1);
			if(obj)
				OBJLIST->AddObject(obj);
		}
	}
}
//---------------------------------------------------------------------------
//
BOOL32 WormholeSync::Update (void)
{
	if(inEffect)
	{
		if(workingID)
		{
			if(THEMATRIX->IsMaster())
			{
				SINGLE range = GRIDSIZE*GRIDSIZE*0.07;
				for(U32 i = 0; i < numTargets; ++i)
				{
					if(targetShipIDs[i])
					{
						if((targetShips[i].ptr->GetPosition()-GetPosition()).magnitude_squared() < range)
						{
							THEMATRIX->SendOperationData(workingID,dwMissionID,&(targetShipIDs[i]),sizeof(U32));
							OBJPTR<IPhysicalObject> phys;
							targetShips[i].ptr->QueryInterface(IPhysicalObjectID,phys);
							phys->SetPosition(targetPos,targetSystemID);
							THEMATRIX->OperationCompleted(workingID,targetShipIDs[i]);
							targetShips[i]->ReleaseShipControl(dwMissionID);

							USR_PACKET<USRMOVE> packet;
							packet.objectID[0] = targetShipIDs[i];
							packet.position.init(targetPos,targetSystemID);
							packet.init(1);
							NETPACKET->Send(HOSTID,0,&packet);
							targetShipIDs[i] = 0;
							++numDeleted;
							flashScale = 1.0;

							IBaseObject * obj = CreateBlast(wormEntryBlast,targetShips[i].ptr->GetTransform(),targetSystemID,1);
							if(obj)
								OBJLIST->AddObject(obj);
						}
					}
				}
				if(numDeleted == numTargets)
				{
					THEMATRIX->SendOperationData(workingID,dwMissionID,&dwMissionID,sizeof(U32));
					THEMATRIX->OperationCompleted2(workingID,dwMissionID);
//					OBJLIST->DeferredDestruction(dwMissionID);
					THEMATRIX->ObjectTerminated(shipID1,0);
					THEMATRIX->ObjectTerminated(shipID2,0);
					inEffect = false;
					deleteEffect = true;
				}
			}
		}
		else if(THEMATRIX->IsMaster())
		{
			ObjMapIterator iter(systemID,GetPosition(),pData->range*GRIDSIZE);
			numTargets = 0;
			while(iter)
			{
				if(numTargets < MAX_WORM_VICTIMS)
				{
					if(iter->obj != ship1.ptr && iter->obj != ship2.ptr)
					{
						MPart part(iter->obj);
						if(part.isValid() && part->caps.jumpOk && part->bReady)
						{
							targetShipIDs[numTargets] = part->dwMissionID;
							iter->obj->QueryInterface(IShipMoveID,targetShips[numTargets],NONSYSVOLATILEPTR);
							OBJPTR<IMissionActor> actor;
							targetShips[numTargets].ptr->QueryInterface(IMissionActorID,actor);
							actor->PrepareTakeover(part->dwMissionID,0);
							targetShips[numTargets]->PushShipTo(dwMissionID,GetPosition(),700);
							++numTargets;
						}
					}
				}
				++iter;
			}
			
			numDeleted = 0;
			WormNetInit buffer;
			buffer.numTargets = numTargets;
			buffer.pos = targetPos;
			workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(WormNetInit));
			THEMATRIX->SetCancelState(workingID,false);

			for(U32 i = 0; i < numTargets; ++ i)
			{
				THEMATRIX->AddObjectToOperation(workingID,targetShipIDs[i]);
				THEMATRIX->SendOperationData(workingID,dwMissionID,&(targetShipIDs[i]),sizeof(U32));
			}
		}
	}
	return !((deleteEffect) && (effectScale == 0));
}
//----------------------------------------------------------------------------------
//
void WormholeSync::renderFlare()
{
	if(flashScale <= 0)
		return;
	Vector pos = transform.translation;
		
	Vector cpos (CAMERA->GetPosition());
		
	Vector look (pos - cpos);
		
	Vector k = look.normalize();

	Vector tmpUp(pos.x,pos.y,pos.z+50000);

	Vector j (cross_product(k,tmpUp));
	if(j.magnitude())
	{
		j.normalize();

		Vector i (cross_product(j,k));

		i.normalize();

		TRANSFORM trans;
		trans.set_i(i);
		trans.set_j(j);
		trans.set_k(k);

		i = trans.get_i();
		j = trans.get_j();

		BATCH->set_state(RPR_BATCH,false);
		DisableTextures();
		CAMERA->SetModelView();
		BATCH->set_render_state(D3DRENDERSTATE_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRENDERSTATE_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRENDERSTATE_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRENDERSTATE_DESTBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRENDERSTATE_DITHERENABLE,TRUE);

		U8 alpha = 100*flashScale;
		PB.Begin(PB_TRIANGLES);
		for(U32 count = 0; count < NUM_FLARES; ++count)
		{	
			PB.Color3ub(alpha,alpha,alpha);
			PB.Vertex3f(pos.x,pos.y,pos.z);
			PB.Color3ub(0,0,0);
			U32 r1 = rand()%FLAIR_CIRC_COUNT;
			Vector final = pos+i*flairCircleX[r1]+j*flairCircleY[r1];
			PB.Vertex3f(final.x,final.y,final.z);
			r1 = (r1+(rand()%3)+1)%FLAIR_CIRC_COUNT;
			final = pos+i*flairCircleX[r1]+j*flairCircleY[r1];
			PB.Vertex3f(final.x,final.y,final.z);
		}
		PB.End();
	}
	
}
//----------------------------------------------------------------------------------
//
void WormholeSync::renderAnm()
{
	U8 anmAlpha = 255;
	SINGLE anmSize = INNER_RING;
	if(effectScale < 1.0)
	{
		if(effectScale < 0)
		{
			anmSize = 0;
			anmAlpha = 0;
		}
		else
		{
			anmAlpha = 255 * effectScale;
			anmSize = INNER_RING*effectScale;
		}
	}
//	if(time < 1.0)
//	{
//		anmSize = INNER_RING*2*time;
//	}
	BATCH->set_render_state(D3DRENDERSTATE_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRENDERSTATE_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRENDERSTATE_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRENDERSTATE_DESTBLEND,D3DBLEND_ONE);

	if(anmSize > 100)
	{
		for(U32 anmCnt = 0; anmCnt < WORMHOLE_ANM_COUNT; ++anmCnt)
		{
			anm[anmCnt].SetWidth(anmSize);
			anm[anmCnt].SetColor(255,255,255,anmAlpha);
			ANIM2D->render(&(anm[anmCnt]));
		}
	}

}
//---------------------------------------------------------------------------
//
void WormholeSync::Render (void)
{
	if(bVisible)
	{
		renderAnm();
		renderFlare();
	}
}
//----------------------------------------------------------------------------------
//
void WormholeSync::CastVisibleArea()
{
	SetVisibleToPlayer(playerID);
}
//---------------------------------------------------------------------------
//
void WormholeSync::PhysicalUpdate(SINGLE dt)
{
	if(bVisible)
	{
		Vector centerPos = transform.translation;
		for(U32 anmCnt = 0; anmCnt < WORMHOLE_ANM_COUNT; ++anmCnt)
		{
			TRANSFORM trans;
			if(anmCnt%2)
				trans.rotate_about_k(dt);
			else
				trans.rotate_about_k(-dt);
			if((anmCnt%4 > 1))
				trans.rotate_about_i(dt);
			else
				trans.rotate_about_i(-dt);

			if((anmCnt%8 > 3))
				trans.rotate_about_j(dt);
			else
				trans.rotate_about_j(-dt);

			Vector pos = anm[anmCnt].GetPosition() - centerPos;
			pos = trans*pos+centerPos;
			anm[anmCnt].SetPosition(pos);
			anm[anmCnt].update(dt);
		}
		if(flashScale > 0)
			flashScale -= (dt);
	}
	if(inEffect && effectScale < 1.0)
	{
		effectScale += (dt/4);
		if(effectScale > 1.0)
			effectScale = 1.0;
	}else if((!inEffect) && effectScale != 0)
	{
		effectScale -= (dt/4);
		if(effectScale < 0)
			effectScale = 0;
	}
}
//---------------------------------------------------------------------------
//
void WormholeSync::init (WORMHOLE_SYNC_INIT& initData)
{
	FRAME_init(initData);
	pData = (BT_WORMHOLE_SYNC *) ARCHLIST->GetArchetypeData(initData.pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->wpnClass == WPN_WORMHOLE);
	CQASSERT(pData->objClass == OC_WEAPON);

	for(U32 anmCnt = 0; anmCnt < WORMHOLE_ANM_COUNT; ++anmCnt)
	{
		anm[anmCnt].Init(initData.wormholeAnm);
		anm[anmCnt].delay = 0;
		anm[anmCnt].SetRotation(((rand()%1000)/1000.0) * 2*PI);
		anm[anmCnt].SetWidth(INNER_RING*2);
		anm[anmCnt].loop = TRUE;
	}

	pArchetype = initData.pArchetype;
	objClass = OC_WEAPON;
	workingID = 0;
	deleteEffect = false;
	wormEntryBlast = initData.wormEntryBlast;
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createWormholeSync (PARCHETYPE pArchetype)
{
	WormholeSync * wormholeSync = new ObjectImpl<WormholeSync>;

	wormholeSync->init(*((WORMHOLE_SYNC_INIT *)(ARCHLIST->GetArchetypeHandle(pArchetype))));

	return wormholeSync;
}
//------------------------------------------------------------------------------------------
//---------------------------WormholeSync Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE WormholeSyncFactory : public IObjectFactory
{
	struct OBJTYPE : public WORMHOLE_SYNC_INIT
	{
		void * operator new (size_t size)
		{
			return calloc(size,1);
		}

		OBJTYPE (void)
		{
		}

		~OBJTYPE (void)
		{
			if(wormholeAnm)
				delete wormholeAnm;
			if (wormEntryBlast)
				ARCHLIST->Release(wormEntryBlast);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(WormholeSyncFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	WormholeSyncFactory (void) { }

	~WormholeSyncFactory (void);

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

	/* WormholeSyncFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
WormholeSyncFactory::~WormholeSyncFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void WormholeSyncFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE WormholeSyncFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_WEAPON)
	{
		BT_WORMHOLE_SYNC * data = (BT_WORMHOLE_SYNC *) _data;

		if (data->wpnClass == WPN_WORMHOLE)	   
		{
			for(U32 i = 0; i < FLAIR_CIRC_COUNT; ++i)
			{
				flairCircleX[i] = cos(i*((PI*2)/FLAIR_CIRC_COUNT))*data->range*GRIDSIZE;
				flairCircleY[i] = sin(i*((PI*2)/FLAIR_CIRC_COUNT))*data->range*GRIDSIZE;
			}
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);

			DAFILEDESC anmdesc;
			COMPTR<IFileSystem> anmFile;
			anmdesc.lpFileName = "JG18_anim.anm";
			if (OBJECTDIR->CreateInstance(&anmdesc, anmFile) == GR_OK)
			{
				result->wormholeAnm = ANIM2D->create_archetype(anmFile);
			}	
			result->wormEntryBlast = ARCHLIST->LoadArchetype("BLAST!!S_WormEntry");
			if(result->wormEntryBlast)
				ARCHLIST->AddRef(result->wormEntryBlast);
		}
	}
	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 WormholeSyncFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * WormholeSyncFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createWormholeSync(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void WormholeSyncFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _wormholeSync : GlobalComponent
{
	WormholeSyncFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<WormholeSyncFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _wormholeSync __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End WormholeSync.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------