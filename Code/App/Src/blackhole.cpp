//--------------------------------------------------------------------------//
//                                                                          //
//                               BlackHole.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/blackhole.cpp 60    10/17/00 9:48a Jasony $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "Resource.h"
#include "Startup.h"
#include "TCPoint.h"
#include "Objlist.h"
#include "IObject.h"
#include "TObject.h"
//#include "BlackHole.h"
#include "camera.h"
#include "UserDefaults.h"
#include "menu.h"
#include "DFog.h"
#include "Camera.h"
#include "BBMesh.h"
#include "TObjSelect.h"
#include "TObjMission.h"
#include "TObjFrame.h"
#include "TObjTrans.h"
#include "DBlackHole.h"
#include "Anim2d.h"
#include "Camera.h"
#include "mission.h"
#include "mesh.h"
#include "IShipMove.h"
#include "ObjMapIterator.h"
#include "SysMap.h"
#include "TerrainMap.h"

//for RadixSort
#include "MyVertex.h"
//temp 
#include "TManager.h"
#include "OpAgent.h"
#include "IWeapon.h"
#include "MPart.h"
#include "CommPacket.h"
#include "ObjSet.h"

#include <DQuickSave.h>
#include <FileSys.h>
#include <TComponent.h>
#include <TSmartPointer.h>
#include <Heapobj.h>
#include <Viewer.h>
#include <EventSys.h>
#include <3dmath.h>
#include <Engine.h>
#include <Physics.h>

#include <malloc.h>

#define BLACKHOLE_RANGE 2.0
#define OUTER_BLACKHOLE_RANGE 3.0

#define TARGET_DELTA_TIME 10.0

struct BLACKHOLE_INIT
{
	BT_BLACKHOLE_DATA *pData;
	S32 archIndex;
	IMeshArchetype * meshArch;

	PARCHETYPE pArchetype;
	ARCHETYPE_INDEX bb_mesh_archID[MAX_BB_MESHES];
	U32 bb_txm[MAX_BB_MESHES];
	AnimArchetype *bb_anim_arch[MAX_BB_MESHES];
	MorphMeshArchetype *mm_arch[MAX_BB_MESHES];
	U32 sysMapIconID;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}


	BLACKHOLE_INIT (void)
	{
		meshArch = NULL;
		archIndex = -1;
		for (int i=0;i<MAX_BB_MESHES;i++)
		{
			bb_txm[i] = 0;
			bb_mesh_archID[i] = INVALID_ARCHETYPE_INDEX;
		}
	}


	~BLACKHOLE_INIT (void)
	{
		ENGINE->release_archetype(archIndex);
		for (int i=0;i<MAX_BB_MESHES;i++)
		{
			if (bb_anim_arch[i])
			{
				delete bb_anim_arch[i];
				bb_anim_arch[i] = 0;
			}

			ENGINE->release_archetype(bb_mesh_archID[i]);
			if (mm_arch[i])
			{
				DestroyMorphMeshArchetype(mm_arch[i]);
				mm_arch[i] = 0;
			}
		}

	}
};

struct SortVertex
{
	Vector p;
	U32 v_idx;
	U32 m_idx;
	Parameters *params;
};

#define BH_START 0
#define BH_END 1

struct BlackholeCommand
{
	U32 dwMissionID;
	U8 command;
	NETGRIDVECTOR jumpPos;
};

struct DoomedNode
{
	U32 dwMissionID;
	struct DoomedNode * next;
};

#define NUM_RINGS 5

struct DACOM_NO_VTABLE BlackHole : ObjectSelection
										<ObjectMission
												<ObjectTransform<ObjectFrame<IBaseObject,BLACKHOLE_SAVELOAD,BLACKHOLE_INIT> >
												>
										>, BASE_BLACKHOLE_SAVELOAD,
									ISaveLoad, IQuickSaveLoad
{
	BEGIN_MAP_INBOUND(BlackHole)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	END_MAP()

	ReceiveOpDataNode	receiveOpDataNode;

	BT_BLACKHOLE_DATA *data;

	// functionality

	//BB Mesh stuff
	BB_Mesh bb[MAX_BB_MESHES];
	SortVertex *s_sort;
	U32 *v_sort;
	U32 v_pos;
	U32 sort_size;
	int numMeshes;

	// rotation
	SINGLE theta;

	//ring timers
	SINGLE ringTimer[NUM_RINGS];

	DoomedNode *doomedList;

	//objmap
	int map_sys,map_square;

	U32 sysMapIconID;

	// methods

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	BlackHole (void);
	~BlackHole (void);
	
	IDAComponent * GetBase (void)
	{
		return (IEventCallback *) this;
	}

	void View ();

	void init(BLACKHOLE_INIT *arch);

//	BOOL32 CheckVisible(INSTANCE_INDEX instanceIndex);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate(SINGLE dt);

	void StashVertices(U32 mesh_id);

	void Billboard3db(BLENDS blendMode);

	virtual void Render (void);

	void RenderPlane(const Transform &trans,U8 alpha);

	virtual void MapRender (bool bPing);

	void warpShip(U32 objMissionID,NETGRIDVECTOR &jumpPos,bool bGeneratePos);

	void enableTerrainFootprint (bool bEnable);

	/* ISaveLoad */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	/* IQuickSaveLoad methods */
	
	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void);

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	//IMissionActor

	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize);

	virtual void OnMasterChange (bool bIsMaster);

	void receiveOpData (U32 agentID, void *buffer, U32 bufferSize);

	/* IPhysicalObject methods */

	virtual void SetSystemID (U32 newSystemID)
	{
		systemID = newSystemID;
	}

	virtual void SetPosition (const Vector & position, U32 newSystemID)
	{
		enableTerrainFootprint(false);

		systemID = newSystemID;
		transform.translation = position;
		enableTerrainFootprint(true);
	}

	virtual void SetTransform (const TRANSFORM & _transform, U32 newSystemID)
	{
		enableTerrainFootprint(false);
		systemID = newSystemID;
		transform = _transform;
		enableTerrainFootprint(true);
	}

	// return true if networking operation is full
	bool isOperationFull (void)
	{
		bool result = false;

		if (workingID)
		{
			const ObjSet * pSet;
			if (THEMATRIX->GetOperationSet(workingID, &pSet))
			{
				if (pSet->numObjects >= MAX_SELECTED_UNITS)
					result = true;
			}
		}

		return result;
	}
};
//------------------------------------------------------------------------
//------------------------------------------------------------------------

BlackHole::BlackHole (void) :
	receiveOpDataNode(this, ReceiveOpDataProc(&BlackHole::receiveOpData))
{
	CQASSERT(MAX_SYSTEMS+1 == MAX_SYSTEMS_PLUS_ONE);
	numTargetSys=1;
	targetSys[0] = 1;
	sysMapIconID = -1;
}

BlackHole::~BlackHole (void)
{
	enableTerrainFootprint(false);

	delete [] v_sort;
	delete [] s_sort;

	DoomedNode *node=doomedList;
	while (doomedList)
	{
		doomedList = node->next;
		delete node;
		node = doomedList;
	}
/*	if (GS)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}*/

//	DEFAULTS->SetDataInRegistry(szRegKey, &holeData, sizeof(holeData));
}
//---------------------------------------------------------------------------
//
void BlackHole::View (void)
{
	BLACKHOLE_DATA data;

	memset(&data, 0, sizeof(data));
	
	//memcpy(data.name,name, sizeof(data.name));
	memset(data.jumpsTo,0,sizeof(data.jumpsTo));
	for (int i=0;i<numTargetSys;i++)
	{
		data.jumpsTo[targetSys[i]] = true;
	}

	if (DEFAULTS->GetUserData("BLACKHOLE_DATA", " ", &data, sizeof(data)))
	{
		numTargetSys=0;
		for (int i=1;i<=MAX_SYSTEMS;i++)
		{
			if (data.jumpsTo[i])
				targetSys[numTargetSys++] = i;
		}
	}
}
//---------------------------------------------------------------------------
//
void BlackHole::init (BLACKHOLE_INIT *arch)
{
	data = arch->pData;

	sysMapIconID = arch->sysMapIconID;

//	hAmbientSound = SFXMANAGER->Open(data->ambience);

	int i;
	for (i=0;i<MAX_BB_MESHES;i++)
	{
		bb[i].bb_txm = arch->bb_txm[i];
		bb[i].animArch = arch->bb_anim_arch[i];
		if (bb[i].animArch)
			bb[i].bb_txm = bb[i].animArch->frames[0].texture;
	}


	sort_size = 0;
	//INIT BILLBOARDS
	for (i=0;i<MAX_BB_MESHES;i++)
	{
		if (arch->bb_mesh_archID[i] != INVALID_ARCHETYPE_INDEX)
		//	bb[i].SetMesh(ENGINE->create_instance(arch->bb_mesh_archID[i]),&arch->pData->billboardMesh[i]);
			bb[i].SetMesh(arch->bb_mesh_archID[i],&arch->pData->billboardMesh[i]);
		if (arch->mm_arch[i])
		{
			bb[i].SetMesh(arch->mm_arch[i],&arch->pData->billboardMesh[i]);
			bb[i].morphMesh->RunAnim(0,TRUE);
		}

		//bb[i].bAdditive = arch->pData->billboardMesh[i].bAdditive;
		bb[i].blendMode = arch->pData->billboardMesh[i].blendMode;

		sort_size += bb[i].numVerts;

		if (bb[i].numVerts)
			numMeshes++;
	}

	s_sort = new SortVertex[sort_size];
	v_sort = new U32[sort_size];

	//transform.rotate_about_j(-PI/2);

	int bubba = 0;
	for (int c=0;c<NUM_RINGS;c++)
	{
		ringTimer[c] = bubba*1.0/NUM_RINGS;
		bubba += 2;
		if (bubba >= NUM_RINGS)
			bubba = 1;
	}

	numTargetSys = SECTOR->GetNumSystems();
	for(U32 sysCount = 0; sysCount < numTargetSys; ++ sysCount)
	{
		targetSys[sysCount] = sysCount+1;
	}
	timer = 0;
	do
	{
		currentTarget = targetSys[rand()%numTargetSys];
	}while(SECTOR->GetAlertState(currentTarget,1) & S_LOCKED);
}

//
// TODO: Make the blackhole NOT warp the ship if the ship's hull points fall below 0.
//    To do this, you need to send a different type of packet so the client knows not to 
//    warp the victim to a random location. (jy)
//

void BlackHole::warpShip(U32 objMissionID,NETGRIDVECTOR &jumpPos,bool bGeneratePos)
{
	waitingID = 0;
	
	DoomedNode *node=doomedList;
	DoomedNode *last=0;
	
	bool bFound=0;
	while (node && (bFound==false))
	{
		if (node->dwMissionID == objMissionID)
			bFound = true;
		else
		{
			last = node;
			node = node->next;
		}
	}
	
	CQASSERT(node);
	//remove me from the list of the doomed
	if (last)
		last->next = node->next;
	else
	{
		doomedList = node->next;
	}
	
	delete node;

	IBaseObject *objectList = OBJLIST->FindObject(objMissionID);
	VOLPTR(IWeaponTarget) target=objectList;
	target->ApplyAOEDamage(0,data->damage);
	MPart part(objectList);
	
	{
		int sys;
		TRANSFORM trans = objectList->GetTransform();
		VOLPTR(IPhysicalObject) obj=objectList;
		if (bGeneratePos)
		{
			sys = currentTarget;//targetSys[rand()%numTargetSys];

			COMPTR<ITerrainMap> map;
			SECTOR->GetTerrainMap(sys,map);
			RECT rect;
			SECTOR->GetSystemRect(sys,&rect);
			U32 width = rect.right-rect.left;
			GRIDVECTOR targ;
			targ = Vector(((rand()%1000)/1000.0f)*width,((rand()%1000)/1000.0f)*width,0);
			while(!(map->IsGridEmpty(targ,0,true)))
			{
				targ = Vector(((rand()%1000)/1000.0f)*width,((rand()%1000)/1000.0f)*width,0);
			}
			trans.translation = targ;
			jumpPos.init(trans.translation,sys);
		}
		else
		{
			sys = jumpPos.systemID;
			trans.translation = jumpPos;
		}
		CQASSERT(sys && sys <= MAX_SYSTEMS);
		obj->SetTransform(trans,sys);
		SECTOR->RevealSystem(sys,obj.Ptr()->GetPlayerID());
		
		UnregisterSystemVolatileWatchersForObject(objectList);
	}
	
	VOLPTR(IShipMove) ship=objectList;
	ship->ReleaseShipControl(dwMissionID);
}
//
//
void BlackHole::enableTerrainFootprint (bool bEnable)
{
	if (bEnable)
	{
		CQASSERT(map_sys==0);
		map_square = OBJMAP->GetMapSquare(systemID,transform.translation);
		map_sys = systemID;
		OBJMAP->AddObjectToMap(this,map_sys,map_square,OM_SYSMAP_FIRSTPASS);
	}
	else
	if (map_sys)
	{
		OBJMAP->RemoveObjectFromMap(this,map_sys,map_square);
		map_sys = map_square = 0;
	}
}
//--------------------------------------------------------------------------//
// Poll objects
//
#define MAX_AFFECTED_UNITS 64
BOOL32 BlackHole::Update (void)
{
	if(!bReady)
		return 1;
	U32 unitIDs[MAX_AFFECTED_UNITS];
	IBaseObject * ptrs[MAX_AFFECTED_UNITS];
	U32 numUnits=0;
	{
		ObjMapIterator it(systemID, transform.translation, OUTER_BLACKHOLE_RANGE*GRIDSIZE);

		while (it)
		{
			if (it->obj->objClass == OC_SPACESHIP)
			{
				if (numUnits >= MAX_AFFECTED_UNITS)
					break;
				ptrs[numUnits] = it->obj;
				unitIDs[numUnits++] = it->dwMissionID;
			}

			++it;
		}
	}
	
	const GRIDVECTOR gridpos = GetGridPosition();
	const bool bIsMaster = THEMATRIX->IsMaster();

	// remove dead people from the doomed list
	{
		DoomedNode *node=doomedList;
		DoomedNode *last=0;

		while (node)
		{
			if (OBJLIST->FindObject(node->dwMissionID) == 0)
			{
				if (last == 0)		// deleting the first element of the list
				{
					doomedList = doomedList->next;
					delete node;
					node = doomedList;
				}
				else
				{
					last->next = node->next;
					delete node;
					node = last->next;
				}
			}
			else
			{
				last = node;
				node = node->next;
			}
		}
	}

	while (numUnits-->0)
	{
		U32 objMissionID = unitIDs[numUnits];
		IBaseObject * obj = ptrs[numUnits];
		bool bDoomed = false;
		DoomedNode *node=doomedList;
		DoomedNode *last=0;
		
		while (node && (bDoomed==false))
		{
			if (node->dwMissionID == objMissionID)
				bDoomed = true;
			else
			{
				last = node;
				node = node->next;
			}
		}
		
		GRIDVECTOR objPos;
		objPos = obj->GetPosition();
		const SINGLE distance =  objPos - gridpos;
		if (distance <= BLACKHOLE_RANGE && bIsMaster && OBJLIST->FindObject(objMissionID))
		{
			// if we're not already on the doomed list, and there is room in the operation
			if (bDoomed == false && isOperationFull()==0)
			{
				//you're doomed

				VOLPTR(IMissionActor) actor=obj;
				actor->PrepareTakeover(objMissionID,0);

				if(!THEMATRIX->GetWorkingOp(actor.Ptr()))
				{
					THEMATRIX->FlushOpQueueForUnit(actor.Ptr());

					DoomedNode *newNode = new DoomedNode;
					newNode->next = doomedList;
					newNode->dwMissionID = objMissionID;
					doomedList = newNode;

					bDoomed = true;


					BlackholeCommand buffer;
					buffer.command = BH_START;
					buffer.dwMissionID = objMissionID;
					buffer.jumpPos.zero();
					if (workingID)
					{
						THEMATRIX->AddObjectToOperation(workingID,objMissionID);
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					}
					else
					{
						ObjSet set;
						set.numObjects = 2;
						set.objectIDs[0] = dwMissionID;
						set.objectIDs[1] = objMissionID;
						workingID = THEMATRIX->CreateOperation(set,&buffer,sizeof(buffer));
						THEMATRIX->SetCancelState(workingID,false);
						THEMATRIX->SetLongTermState(workingID, true);
					}

					VOLPTR(IShipMove) ship=actor;
					ship->PushShipTo(dwMissionID,transform.translation,ship->GetCurrentCruiseVelocity());
				}
			}
		}

		//	
		// are we near the center point ?
		//

		if (bDoomed && distance < 0.3)
		{
			if (bIsMaster)
			{
				NETGRIDVECTOR jumpPos;
				warpShip(objMissionID,jumpPos,true); //generate the destination
				node = 0;	

				BlackholeCommand buffer;
				buffer.command = BH_END;
				buffer.dwMissionID = objMissionID;
				buffer.jumpPos = jumpPos;
				CQASSERT(buffer.jumpPos.systemID && buffer.jumpPos.systemID <= MAX_SYSTEMS);
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
				THEMATRIX->OperationCompleted(workingID,objMissionID);
			}
			else if (node && node->dwMissionID == waitingID)
			{
				waitingID = 0;
				warpShip(objMissionID,waitingJumpPos,false); // use networked position
				THEMATRIX->OperationCompleted(workingID,objMissionID);
			}
		}
		/*
		else
		if (bDoomed==false && distance <= OUTER_BLACKHOLE_RANGE)
		{
			VOLPTR(IPhysicalObject) ship = it->obj;
			if (ship)
			{
				//Tug on the ship, he could still escape
				Vector dir = it->obj->GetPosition() - transform.translation;
				SINGLE mag = dir.magnitude();

				if (mag > 0 && mag <= OUTER_BLACKHOLE_RANGE*GRIDSIZE)
				{
					Vector vel = it->obj->GetVelocity();
					dir /= mag;
					mag = ((OUTER_BLACKHOLE_RANGE*GRIDSIZE) - mag) / (OUTER_BLACKHOLE_RANGE*GRIDSIZE);
					// mag = now a percentage of pull
					mag *= 1000 * ELAPSED_TIME;
					dir *= mag;
					vel += dir;

					ship->SetVelocity(vel);
				}
			}
		}
		*/
	}

	return 1;
}

void BlackHole::PhysicalUpdate(SINGLE dt)
{
	timer += dt;
	if(timer > TARGET_DELTA_TIME)
	{
		do
		{
			currentTarget = targetSys[rand()%numTargetSys];
		}while(SECTOR->GetAlertState(currentTarget,1) & S_LOCKED);
		timer = 0;
	}
	theta += 0.002*dt;
	transform.rotate_about_k(0.2*dt);

	int i;
	for (i=0;i<NUM_RINGS;i++)
	{
	//	ringTimer[i] -= 0.05*dt;
		ringTimer[i] -= 0.05*dt*pow((1.3-ringTimer[i]),1.5);
		while (ringTimer[i] < 0.0f)
			ringTimer[i] += 1.0f;
	}

	for (i=0;i<MAX_BB_MESHES;i++)
		bb[i].Update(dt);
}

void BlackHole::StashVertices(U32 mesh_id)
{
	Vector cpos (CAMERA->GetPosition());

	Vector *p_sort = bb[mesh_id].p_sort;
	Parameters *params = &bb[mesh_id].params;
	S32 numVerts = bb[mesh_id].GetVertices(p_sort,params);
	int v_cnt;
	for (v_cnt=0;v_cnt<numVerts;v_cnt++)
	{

		s_sort[v_pos].v_idx = v_cnt;
		s_sort[v_pos].m_idx = mesh_id;
		s_sort[v_pos].p = transform*(p_sort[v_cnt]+data->billboardMesh[mesh_id].offset);
		s_sort[v_pos].params = params;
	//	v_sort[v_pos] = (s_sort[v_cnt].p-cpos).magnitude();
		v_sort[v_pos] = -s_sort[v_cnt].p.y;
		v_pos++;
	}
}

void BlackHole::Billboard3db(BLENDS blendMode)
{
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);

	BATCH->set_state(RPR_BATCH,FALSE);
	/*if (bb[0].animArch)
	{
		//BATCH->set_texture_stage_texture( 0,bb[0].animArch->frames[0].texture);
		SetupDiffuseBlend(bb[0].animArch->frames[0].texture,TRUE);
	}
	else
		//BATCH->set_texture_stage_texture( 0,bb[0].bb_txm);*/
	//SetupDiffuseBlend(bb[0].bb_txm,TRUE);

	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_DITHERENABLE,FALSE);
	if (blendMode == ONE_ONE || blendMode == ONE_INVSRC)
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	else
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	
	if (blendMode == ONE_ONE || blendMode == SRC_ONE)
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	else
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

	Vector epos = transform.translation;

	Vector cpos (CAMERA->GetPosition());
	
	Vector look (epos - cpos);
	
	Vector i (look.y, -look.x, 0);
	
	if (fabs (i.x) < 1e-5 && fabs (i.y) < 1e-5)
	{
		i.x = 1.0f;
	}
	
	i.normalize ();
	
	Vector k (-look);
	k.normalize ();
	Vector j (cross_product (k, i));
	
	Vector base_i,base_j;
	base_i = i;
	base_j = j;
	
	TRANSFORM trans;
	trans.set_k(k);
	
	CAMERA->SetModelView();
	
	PB.Color4ub(255,255,255,255);
	PB.Begin(PB_QUADS);
	
	RadixSort::sort(v_sort,v_pos);//sort_size);

	SINGLE alpha=1.0;//0.4;
	for (int v_cnt=v_pos-1;v_cnt>=0;v_cnt--)
	{
		U32 idx = RadixSort::index_list[v_cnt];
		BB_Mesh *bbm = &bb[s_sort[idx].m_idx];
		Billboard *b = &bbm->bb_data[s_sort[idx].v_idx];
		Vector p;
		p = s_sort[idx].p;

		trans.set_i(base_i);
		trans.set_j(base_j);
		trans.rotate_about_k(b->rotation);
		i = trans.get_i();
		j = trans.get_j();

		Vector v[4];
		v[0] = p+b->size*s_sort[idx].params->scale*(-i-j);
		v[1] = p+b->size*s_sort[idx].params->scale*(i-j);
		v[2] = p+b->size*s_sort[idx].params->scale*(i+j);
		v[3] = p+b->size*s_sort[idx].params->scale*(-i+j);
		
		if (blendMode == ONE_ONE)
			PB.Color3ub(alpha*255,alpha*255,alpha*255);
		else
			PB.Color4ub(255,255,255,alpha*255);

		if (bbm->animArch)
		{
			AnimFrame * frame = &bbm->animArch->frames[S32(b->frameOffset+bbm->timer*bbm->animArch->capture_rate)%bbm->animArch->frame_cnt];
			PB.TexCoord2f (frame->x0,frame->y0);
			PB.Vertex3f (v[0].x, v[0].y, v[0].z);
			PB.TexCoord2f (frame->x1,frame->y0);
			PB.Vertex3f (v[1].x, v[1].y, v[1].z);
			PB.TexCoord2f (frame->x1,frame->y1);
			PB.Vertex3f (v[2].x, v[2].y, v[2].z);
			PB.TexCoord2f (frame->x0,frame->y1);
			PB.Vertex3f (v[3].x, v[3].y, v[3].z);
		}
		else
		{
			PB.TexCoord2f (0,0);
			PB.Vertex3f (v[0].x, v[0].y, v[0].z);
			PB.TexCoord2f (1,0);
			PB.Vertex3f (v[1].x, v[1].y, v[1].z);
			PB.TexCoord2f (1,1);
			PB.Vertex3f (v[2].x, v[2].y, v[2].z);
			PB.TexCoord2f (0,1);
			PB.Vertex3f (v[3].x, v[3].y, v[3].z);		
		}
	//	alpha+= 0.005;
	}
	PB.End();
}

void BlackHole::Render()
{
	if (bVisible)
	{
		BATCH->set_state(RPR_BATCH,FALSE);
		TRANSFORM trans = transform;
		transform.set_identity();
		transform.translation = trans.translation;
		//	transform.rotate_about_k(theta);
		
#define SCALE 1.1
#define SCALE_MIN 0.4
		
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		//???
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		
		
		//	Mesh *mesh = REND->get_unique_instance_mesh(instanceIndex);
		//	CAMERA->SetModelView();
		Transform scaleTrans;
		int c;
		for (c=0;c<NUM_RINGS;c++)
		{
			//		mesh->material_list->transparency = 255*(min(1.0,(1.0-ringTimer[c])*3));
			scaleTrans.d[0][0] = SCALE*ringTimer[c]+SCALE_MIN;
			scaleTrans.d[1][1] = SCALE*ringTimer[c]+SCALE_MIN;
			scaleTrans.d[2][2] = 1;//SCALE*ringTimer[c]+SCALE_MIN;
			scaleTrans.translation.z = c*200-100*NUM_RINGS;
			transform.set_identity();
			transform.translation = trans.translation;
			//transform.rotate_about_k(pow((1-ringTimer[c]),3)*4*PI);
			transform.rotate_about_k((1-ringTimer[c])*3*PI);
			transform = transform *scaleTrans;
			//ENGINE->render_instance(MAINCAM, instanceIndex, RF_TRANSFORM_LOCAL, &scaleTrans);
			CAMERA->SetModelView(&transform);
			RenderPlane(transform,255*(min(1.0,(1.0-ringTimer[c])*3)));
		}
		
		transform = trans;
		//ENGINE->render_instance(MAINCAM,instanceIndex);
		
		//RENDER BILLBOARD MESHES
		
		int meshesRendered=0;
		
		BLENDS stage;
		U32 nextStartMesh=0;
		U32 currentTex;
		
		static bool bRendered[MAX_BB_MESHES];
		memset(bRendered,0,sizeof(bool)*MAX_BB_MESHES);
		
		while (meshesRendered < numMeshes)
		{
			v_pos = 0;
			
			while (bRendered[nextStartMesh] || bb[nextStartMesh].numVerts==0)
			{
				nextStartMesh++;
				CQASSERT(nextStartMesh < MAX_BB_MESHES);
			}
			
			currentTex = bb[nextStartMesh].bb_txm;
			stage = bb[nextStartMesh].blendMode;
			for (c=nextStartMesh;c<MAX_BB_MESHES;c++)
			{
				if (bb[c].numVerts && bb[c].blendMode == stage && bb[c].bb_txm == currentTex)
				{
					StashVertices(c);
					meshesRendered++;
					bRendered[c] = true;
				}
			}
			
			SetupDiffuseBlend(currentTex,TRUE);
			if (v_pos)
				Billboard3db(stage);
		}
	}
}

void BlackHole::RenderPlane(const Transform &trans,U8 alpha)
{
	Mesh *mesh = REND->get_unique_instance_mesh(instanceIndex);
	FaceGroup *fg = mesh->face_groups;
	SetupDiffuseBlend(mesh->material_list->texture_id,TRUE);
	PIPE->set_render_state(D3DRS_DITHERENABLE,TRUE);

//	Vector a = CAMERA->GetPosition()-trans.translation;
//	a.normalize();
//	a= trans.inverse_rotate(a);

	PB.Begin(PB_TRIANGLES);
	PB.Color4ub(255,255,255,alpha);
	int i;
	for (i=0;i<mesh->face_cnt;i++)
	{
		U32 ref[3];
		U32 tref[3];
	
		ref[0] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3]];
		ref[1] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3+1]];
		ref[2] = mesh->vertex_batch_list[fg->face_vertex_chain[i*3+2]];

		tref[0] = mesh->texture_batch_list[fg->face_vertex_chain[i*3]];
		tref[1] = mesh->texture_batch_list[fg->face_vertex_chain[i*3+1]];
		tref[2] = mesh->texture_batch_list[fg->face_vertex_chain[i*3+2]];

		Vector v[3];
		v[0] = mesh->object_vertex_list[ref[0]];
		v[1] = mesh->object_vertex_list[ref[1]];
		v[2] = mesh->object_vertex_list[ref[2]];

		SINGLE z_bias[3];
		for (int c=0;c<3;c++)
		{
			z_bias[c] = 0.007*(fmod(v[c].x,342)*fmod(v[c].y,265))*cos(theta*500);
		}


		PB.TexCoord2f(mesh->texture_vertex_list[tref[0]].u,mesh->texture_vertex_list[tref[0]].v);
		PB.Vertex3f(v[0].x,v[0].y,v[0].z+z_bias[0]);
		PB.TexCoord2f(mesh->texture_vertex_list[tref[1]].u,mesh->texture_vertex_list[tref[1]].v);
		PB.Vertex3f(v[1].x,v[1].y,v[1].z+z_bias[1]);
		PB.TexCoord2f(mesh->texture_vertex_list[tref[2]].u,mesh->texture_vertex_list[tref[2]].v);
		PB.Vertex3f(v[2].x,v[2].y,v[2].z+z_bias[2]);
		
	}
	PB.End();
}
//---------------------------------------------------------------------------
//
void BlackHole::MapRender (bool bPing)
{
	if(IsVisibleToPlayer(MGlobals::GetThisPlayer()) || DEFAULTS->GetDefaults()->bEditorMode || DEFAULTS->GetDefaults()->bSpectatorModeOn)
	{
		if(sysMapIconID != -1)
			SYSMAP->DrawIcon(transform.translation,GRIDSIZE*2,sysMapIconID);
		else
			SYSMAP->DrawCircle(transform.translation,GRIDSIZE*2,RGB(255,255,255));
		
	}
}
//---------------------------------------------------------------------------
//
BOOL32 BlackHole::Save (struct IFileSystem * outFile)
{
	BOOL32 result = 0;
	U32 dwWritten;
	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc = "BLACKHOLE_SAVELOAD";
	BLACKHOLE_SAVELOAD d;

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	if (outFile->CreateInstance(&fdesc,file) != GR_OK)
		goto Done;

	memset(&d, 0, sizeof(d));

	FRAME_save(d);
	memcpy(&d, static_cast<BASE_BLACKHOLE_SAVELOAD *>(this), sizeof(BASE_BLACKHOLE_SAVELOAD));
		
	if (file->WriteFile(0,&d,sizeof(d),&dwWritten,0) == 0)
		goto Done;

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 BlackHole::Load (struct IFileSystem * inFile)
{
	BOOL32 result = 0;
	U32 dwRead;
	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc = "BLACKHOLE_SAVELOAD";
	U8 buffer[1024];
	BLACKHOLE_SAVELOAD d;

	if (inFile->CreateInstance(&fdesc,file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol(fdesc.lpFileName, buffer, &d);

	CQASSERT(d.mission.dwMissionID != 0);

	memcpy(static_cast<BASE_BLACKHOLE_SAVELOAD *>(this), &d, sizeof(BASE_BLACKHOLE_SAVELOAD));
	FRAME_load(d);
		
	SetPosition(transform.translation, systemID);

	result = 1;

Done:	
	
	return result;
}
//---------------------------------------------------------------------------
//
void BlackHole::ResolveAssociations()
{
	FRAME_resolve();
}
//---------------------------------------------------------------------------
//
void BlackHole::QuickSave (struct IFileSystem * file)
{
	DAFILEDESC fdesc = partName;
	HANDLE hFile;

	file->CreateDirectory("MT_BLACKHOLE_QLOAD");
	if (file->SetCurrentDirectory("MT_BLACKHOLE_QLOAD") == 0)
		CQERROR0("QuickSave failed on directory 'MT_BLACKHOLE_QLOAD'");

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	if ((hFile = file->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR1("QuickSave failed on part '%s'", fdesc.lpFileName);
	}
	else
	{
		MT_BLACKHOLE_QLOAD qload;
		DWORD dwWritten;

		qload.pos.init(GetGridPosition(), systemID);
		qload.numTargetSys = numTargetSys;
		memcpy(qload.targetSys,targetSys,sizeof(targetSys));

		file->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		file->CloseHandle(hFile);
	}
}
//---------------------------------------------------------------------------
//
void BlackHole::QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize)
{
	MT_BLACKHOLE_QLOAD qload;
	MISSION->CorrelateSymbol(szSaveLoadType, buffer, &qload);
	memcpy(targetSys,qload.targetSys,sizeof(targetSys));

	MGlobals::InitMissionData(this, MGlobals::CreateNewPartID(0));
	SetPosition(qload.pos, qload.pos.systemID);
	partName = szInstanceName;
	SetReady(true);

	OBJLIST->AddPartID(this, dwMissionID);
}
//---------------------------------------------------------------------------
//
void BlackHole::QuickResolveAssociations (void)
{
	// nothing to do here!?
}
//--------------------------------------------------------------------------//
// set bVisible if possible for any part of object to appear
//
void BlackHole::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	bVisible = (systemID == currentSystem);
	if (bVisible)
	{
		float depth;
		ViewRect out_rect;

		bVisible = REND->get_instance_projected_bounding_box(instanceIndex, MAINCAM, LODPERCENT, &out_rect, depth);
	}
}
//---------------------------------------------------------------------------
//
void BlackHole::OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
{
	CQASSERT(!workingID);
	//purely client code
	BlackholeCommand * buf = (BlackholeCommand *)buffer;
	
	VOLPTR(IMissionActor) actor=OBJLIST->FindObject(buf->dwMissionID);
//	actor->PrepareTakeover(buf->dwMissionID,0);
	VOLPTR(IShipMove) ship=actor;
	ship->PushShipTo(dwMissionID,transform.translation,ship->GetCurrentCruiseVelocity());

	//you're doomed
	
	DoomedNode *newNode = new DoomedNode;
	newNode->next = doomedList;
	newNode->dwMissionID = buf->dwMissionID;
	doomedList = newNode;

	workingID = agentID;
	THEMATRIX->SetCancelState(agentID, false);
	THEMATRIX->SetLongTermState(agentID, true);
}
//---------------------------------------------------------------------------
//
void BlackHole::OnMasterChange (bool bIsMaster)
{
	if (bIsMaster && waitingID)
	{
		U32 objMissionID = waitingID;
		warpShip(waitingID,waitingJumpPos,false); // use previous master's position
		waitingID = 0;
		THEMATRIX->OperationCompleted(workingID,objMissionID);
	}
}
//---------------------------------------------------------------------------
//
void BlackHole::receiveOpData (U32 agentID, void *buffer, U32 bufferSize)
{
	if (agentID == workingID)
	{
		BlackholeCommand * buf = (BlackholeCommand *)buffer;
		
		//purely client code
		if (buf->command == BH_START)
		{						
			VOLPTR(IMissionActor) actor=OBJLIST->FindObject(buf->dwMissionID);
//			actor->PrepareTakeover(buf->dwMissionID,0);
			VOLPTR(IShipMove) ship=actor;
			ship->PushShipTo(dwMissionID,transform.translation,ship->GetCurrentCruiseVelocity());

			//you're doomed
			DoomedNode *newNode = new DoomedNode;
			newNode->next = doomedList;
			newNode->dwMissionID = buf->dwMissionID;
			doomedList = newNode;
		}
		else
		{
			CQASSERT(buf->command == BH_END);
			CQASSERT(waitingID==0 || OBJLIST->FindObject(waitingID)==0);		// should only have 1 active jumper at a time (in the "end" state)
			CQASSERT(buf->jumpPos.systemID && buf->jumpPos.systemID <= MAX_SYSTEMS);
			waitingID = buf->dwMissionID;
			waitingJumpPos = buf->jumpPos;
		}
	}
}

struct DACOM_NO_VTABLE BlackHoleManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(BlackHoleManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	// structure
//	struct BlackHoleNode *BlackHoleList;
	U32 factoryHandle;
	TRANSFORM editorOrientation;	// initial orientation of objects

	//child object info
	BLACKHOLE_INIT *pArchetype;

	//BlackHoleManager methods

	BlackHoleManager (void) 
	{
		//editorOrientation.rotate_about_k(45*MUL_DEG_TO_RAD);
	//	editorOrientation.rotate_about_j(PI/2);
	}

	~BlackHoleManager();
	
    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	IDAComponent * GetBase (void)
	{
		return static_cast<IObjectFactory *>(this);
	}

	BOOL32 Update();

	void Init();

	//void setCursorMode (CURSOR_MODE newMode);

	//IObjectFactory
	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);
};


//--------------------------------------------------------------------------
// BlackHoleManager methods

BlackHoleManager::~BlackHoleManager()
{
	COMPTR<IDAConnectionPoint> connection;
	if (OBJLIST)
	{
		if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
			connection->Unadvise(factoryHandle);
	}

}

BOOL32 BlackHoleManager::Update()
{
	return 1;
}

void BlackHoleManager::Init()
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------//
//
HANDLE BlackHoleManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_BLACKHOLE)
	{
		BLACKHOLE_INIT *newguy = new BLACKHOLE_INIT;
//		newguy->name = szArchname;
		newguy->pData = (BT_BLACKHOLE_DATA *)data;
		newguy->pArchetype = ARCHLIST->GetArchetype(szArchname);

		COMPTR<IFileSystem> file;
		DAFILEDESC fdesc;

		// LOAD BILLBOARD MESHES
		for (int i=0;i<MAX_BB_MESHES;i++)
		{
			if (newguy->pData->billboardMesh[i].mesh_name[0] && newguy->pData->billboardMesh[i].tex_name[0])
			{
				//Get mesh or animating mesh
				if (strstr(newguy->pData->billboardMesh[i].mesh_name,".3db"))
				{
					fdesc = newguy->pData->billboardMesh[i].mesh_name;
					if (OBJECTDIR->CreateInstance(&fdesc, file) == GR_OK)
						newguy->bb_mesh_archID[i] = ENGINE->create_archetype(fdesc.lpFileName, file);
					else
						CQFILENOTFOUND(fdesc.lpFileName);
				}
				else if (strstr(newguy->pData->billboardMesh[i].mesh_name,".ini"))
				{
					newguy->mm_arch[i] = CreateMorphMeshArchetype(newguy->pData->billboardMesh[i].mesh_name);
				}

				//Get texture or animating texture
				if (strstr(newguy->pData->billboardMesh[i].tex_name,".tga"))
					newguy->bb_txm[i] = TMANAGER->CreateTextureFromFile(newguy->pData->billboardMesh[i].tex_name,TEXTURESDIR, DA::TGA,PF_4CC_DAA8);
				else if (strstr(newguy->pData->billboardMesh[i].tex_name,".anm"))
				{
					fdesc = newguy->pData->billboardMesh[i].tex_name;
					if (OBJECTDIR->CreateInstance(&fdesc, file) == GR_OK)
						newguy->bb_anim_arch[i] = ANIM2D->create_archetype(file);
					else
						CQFILENOTFOUND(fdesc.lpFileName);
				}
				else
					CQERROR1("Don't know how to load %s",newguy->pData->billboardMesh[i].tex_name);
			}
		}

		if (newguy->pData->ringObjectName[0])
		{
			fdesc.lpFileName = newguy->pData->ringObjectName;
			if (OBJECTDIR->CreateInstance(&fdesc, file) == GR_OK)
			{
				newguy->archIndex = ENGINE->create_archetype(fdesc.lpFileName, file);
			}
		}

		if(newguy->pData->sysMapIcon[0])
		{
			newguy->sysMapIconID = SYSMAP->RegisterIcon(newguy->pData->sysMapIcon);
		}
		else
			newguy->sysMapIconID = -1;


		return newguy;
	}
	else
		return 0;
}

BOOL32 BlackHoleManager::DestroyArchetype(HANDLE hArchetype)
{
/*	if (tex)
	{
		delete [] tex;
		tex = 0;
	}*/

	BLACKHOLE_INIT *deadguy = (BLACKHOLE_INIT *)hArchetype;
	EditorStopObjectInsertion(deadguy->pArchetype);
	delete deadguy;

	return 1;
}

IBaseObject * BlackHoleManager::CreateInstance(HANDLE hArchetype)
{
	BLACKHOLE_INIT *pBlackHole = (BLACKHOLE_INIT *)hArchetype;
	BT_BLACKHOLE_DATA *objData = ((BLACKHOLE_INIT *)hArchetype)->pData;
	TRANSFORM & trans = editorOrientation;
	
	if (objData->objClass == OC_BLACKHOLE)
	{
		BlackHole * obj = new ObjectImpl<BlackHole>;
		
		obj->FRAME_init(*pBlackHole);
		
		obj->pArchetype = 0;
		obj->objClass = OC_BLACKHOLE;
		obj->systemID = 0;
		obj->transform = trans;
		obj->init(pBlackHole);
		
		return obj;
	}

	
	
	return 0;
}

void BlackHoleManager::EditorCreateInstance(HANDLE hArchetype,const MISSION_INFO & _info)
{
	BLACKHOLE_INIT * objtype = (BLACKHOLE_INIT *) hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, _info);
}

//----------------------------------------------------------------------------------------------
//
static struct BlackHoleManager *BLACKHOLEMGR;
//----------------------------------------------------------------------------------------------
//
struct _blackhole : GlobalComponent
{
	virtual void Startup (void)
	{
		struct BlackHoleManager *BlackHoleMgr = new DAComponent<BlackHoleManager>;
		BLACKHOLEMGR = BlackHoleMgr;
		AddToGlobalCleanupList((IDAComponent **) &BLACKHOLEMGR);
	}

	virtual void Initialize (void)
	{
		BLACKHOLEMGR->Init();
	}
};

static _blackhole blackhole;

//--------------------------------------------------------------------------//
//----------------------------END BlackHole.cpp------------------------------//
//--------------------------------------------------------------------------//
