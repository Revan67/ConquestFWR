//--------------------------------------------------------------------------//
//                                                                          //
//                             UnbornMeshList.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/UnbornMeshList.cpp 26    10/31/00 9:05a Jasony $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "IUnbornMeshList.h"
#include "Startup.h"
#include "MeshRender.h"
#include "ObjList.h"
#include "OpAgent.h"
#include "IJumpGate.h"
#include "IPlanet.h"
#include "Sector.h"
#include "Objwatch.h"
#include "TerrainMap.h"
#include "CQLight.h"
#include "IFabricator.h"
#include "ObjSet.h"
#include "WindowManager.h"
#include "Camera.h"
#include "Cursor.h"
#include <IConnection.h>
#include <TSmartPointer.h>
#include <DPlatform.h>
#include <renderer.h>

#define MAX_MATS 30

void GetAllChildren (INSTANCE_INDEX instanceIndex,INSTANCE_INDEX *array,S32 &last,S32 arraySize);

struct MeshEntry
{
	U32 archID;
	PARCHETYPE pArchetype;
	U8 lastPlayer;

	U32 instanceIndex;
//	U32 children;
	IMeshInfoTree * mesh_info;
//	MeshInfo * mi[MAX_CHILDS];
	MeshChain mc;

	MeshEntry * next;

	MeshEntry()
	{};

	~MeshEntry()
	{
		ENGINE->destroy_instance(instanceIndex);
		DestroyMeshInfoTree(mesh_info);
		ARCHLIST->Release(pArchetype, OBJREFNAME);
	}
};

struct FabQueueStruct
{
	U32 opID;
	U32 archID;
	U32 systemID;
	TRANSFORM trans;
	
	U32 altSystemID;
	TRANSFORM altTrans;

	U32 targetID;  //planet or jumpgate
	U32 slotID; //planet only;
	OBJPTR<IFabricator> fab;

	FabQueueStruct * next;
};

//--------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE UnbornMeshList : public IUnbornMeshList,IAgentEnumerator
{
	BEGIN_DACOM_MAP_INBOUND(UnbornMeshList)
	DACOM_INTERFACE_ENTRY(IUnbornMeshList)
	END_DACOM_MAP()

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
	
	MeshEntry * firstMesh;
	U32 currentPlayerID;

	FabQueueStruct * firstQueue[MAX_PLAYERS];
	FabQueueStruct * currentNode;
	FabQueueStruct * prevNode;

	UnbornMeshList();

	~UnbornMeshList();

	/* IAgentEnumerator */

	virtual bool EnumerateAgent (const NODE & node);

	/* IUnbornMeshList */

	virtual void RenderMeshAt(TRANSFORM trans,U32 archID,U32 playerID,U8 red = 255,U8 green = 255, U8 blue = 255, U8 alpha = 255);

	virtual void Render();

	virtual void Unload();

	virtual bool IsFabBuilding(U32 dwMissionfID,U32 archID);

	virtual void GetBoundingSphere(TRANSFORM trans,U32 archID,float & obj_rad,Vector &wcenter);

	//virtual INSTANCE_INDEX GetInstanceIndex(const TRANSFORM &trans,U32 archID);

	virtual MeshChain *GetMeshChain(const TRANSFORM &trans,U32 archID);

	virtual bool IsFullGrid (U32 archID);

	void init();

	MeshEntry * findMesh(U32 archID);

	MeshEntry * createMesh(U32 archID);

	void setMeshColor(MeshEntry * mesh,U32 playerID,U8 red, U8 green, U8 blue, U8 alpha);

	void setMeshTransform(MeshEntry * mesh, TRANSFORM trans);


	void clearQueue();

	void addToQueue(U32 fabID,U32 opID, U32 archID, Vector position, U32 systemID, U32 targetID, U32 slotID,FabQueueStruct * prev);

	void removeFromQueue(FabQueueStruct * node,FabQueueStruct * prev,bool bClosing = false);

	void renderQueue();
};
//--------------------------------------------------------------------------//
//
UnbornMeshList::UnbornMeshList()
{
};
//--------------------------------------------------------------------------//
//

UnbornMeshList::~UnbornMeshList()
{
	clearQueue();
	while(firstMesh)
	{
		MeshEntry * delMesh = firstMesh;
		firstMesh = firstMesh->next;
		delete delMesh;
	}
};
//--------------------------------------------------------------------------//
//
void UnbornMeshList::Unload()
{
	clearQueue();
	while(firstMesh)
	{
		MeshEntry * delMesh = firstMesh;
		firstMesh = firstMesh->next;
		delete delMesh;
	}
}
//--------------------------------------------------------------------------//
//
bool UnbornMeshList::IsFabBuilding(U32 dwMissionID,U32 archID)
{
	U32 playerIndex = MGlobals::GetPlayerFromPartID(dwMissionID);
	if(playerIndex)
	{
		--playerIndex;
		FabQueueStruct * search = firstQueue[playerIndex];
		while(search)
		{
			if(search->fab && search->fab.Ptr()->GetPartID() == dwMissionID && search->archID == archID)
			{
				return true;
			}
			search = search->next;
		}
	}
	return false;
}
//--------------------------------------------------------------------------//
//
void UnbornMeshList::RenderMeshAt(TRANSFORM trans,U32 archID,U32 playerID,U8 red ,U8 green , U8 blue ,U8 alpha)
{
	LIGHT->deactivate_all_lights();
	LIGHTS->ActivateBestLights(trans.translation,8,4000);
	
	MeshEntry * mesh = findMesh(archID);
	if(!mesh)
	{
		mesh = createMesh(archID);
	}
	CQASSERT(mesh);
	if(mesh->lastPlayer != playerID)
	{
		setMeshColor(mesh,playerID,red,green,blue,alpha);
	}
	setMeshTransform(mesh,trans);
	TreeRender(mesh->mc);
}
//--------------------------------------------------------------------------//
//
MeshEntry * UnbornMeshList::findMesh(U32 archID)
{
	MeshEntry * mesh = firstMesh;
	while(mesh)
	{
		if(mesh->archID == archID)
			return mesh;
		mesh = mesh->next;
	}
	return NULL;
}

typedef PLATFORM_INIT<BASE_PLATFORM_DATA> BASEPLATINIT;

//--------------------------------------------------------------------------//
//
MeshEntry * UnbornMeshList::createMesh(U32 archID)
{
	MeshEntry * mesh = new MeshEntry;
	mesh->pArchetype = ARCHLIST->LoadArchetype(archID);
	ARCHLIST->AddRef(mesh->pArchetype, OBJREFNAME);
	mesh->archID = archID;
	mesh->lastPlayer = 0;

	IMeshRender **mr;
	
	BASIC_DATA *data = (BASIC_DATA *)ARCHLIST->GetArchetypeData(mesh->pArchetype);
	if (data->objClass == OC_PLATFORM)
	{
		BASEPLATINIT * initSt = ((BASEPLATINIT *) (ARCHLIST->GetArchetypeHandle(mesh->pArchetype)));
		mesh->instanceIndex = ENGINE->create_instance2(initSt->archIndex,NULL);
		mr = initSt->mr;
		
		
		mesh->mesh_info = CreateMeshInfoTree(mesh->instanceIndex);
		mesh->mc.numChildren = mesh->mesh_info->ListChildren(mesh->mc.mi);
		
		if (mr == 0)
		{
			
			typedef IMeshRender * booga;
			mr = new booga[mesh->mc.numChildren];
			for (U32 i=0;i<mesh->mc.numChildren;i++)
			{
				mr[i] = CreateMeshRender();
				mr[i]->AddRef();
				mr[i]->Init(mesh->mc.mi[i]);
			}
			
			//hack to bust into the const struct
			IMeshRender ***pmr = (IMeshRender ***)(void *)&(initSt->mr);
			int *nm = (int *)(void *)&initSt->numChildren;
			*nm = mesh->mc.numChildren;
			*pmr = mr;
		}
	}
	else
	{
		SPACESHIP_INIT<BASE_SPACESHIP_DATA> * initSt = ((SPACESHIP_INIT<BASE_SPACESHIP_DATA> *) (ARCHLIST->GetArchetypeHandle(mesh->pArchetype)));
		mesh->instanceIndex = ENGINE->create_instance2(initSt->archIndex,NULL);
		mr = initSt->mr;
		
		
		mesh->mesh_info = CreateMeshInfoTree(mesh->instanceIndex);
		mesh->mc.numChildren = mesh->mesh_info->ListChildren(mesh->mc.mi);
		
		if (mr == 0)
		{
			
			//mr = new MeshRender[mesh->mc.numChildren];
			typedef IMeshRender * booga;
			mr = new booga[mesh->mc.numChildren];
			//AllocateMeshRenders(mr,mesh->mc.numChildren);
			for (U32 i=0;i<mesh->mc.numChildren;i++)
			{
				mr[i] = CreateMeshRender();
				mr[i]->AddRef();
				mr[i]->Init(mesh->mc.mi[i]);
			}
			
			//hack to bust into the const struct
			IMeshRender ***pmr = (IMeshRender ***)(void *)&(initSt->mr);
			int *nm = (int *)(void *)&initSt->numChildren;
			*nm = mesh->mc.numChildren;
			*pmr = mr;
		}
	}

	
	for (U32 i=0;i<mesh->mc.numChildren;i++)
	{
		mr[i]->SetupMeshInfo(mesh->mc.mi[i]);
	}

	mesh->next = firstMesh;
	firstMesh = mesh;
	return mesh;
}
//--------------------------------------------------------------------------//
//
void UnbornMeshList::setMeshColor(MeshEntry * meshE,U32 playerID,U8 red,U8 green, U8 blue, U8 alpha)
{
	S32 children[30];
	S32 num_children=0;

	Material *mat[MAX_MATS];

	memset(mat, 0, sizeof(mat));

	GetAllChildren(meshE->instanceIndex,children,num_children,30);
	COLORREF color = COLORTABLE[MGlobals::GetColorID(playerID)];

	for (int c=0;c<num_children;c++)
	{	
		Mesh *mesh = REND->get_instance_mesh(children[c]);

		if (mesh)
		{
			for (int fg_cnt=0;fg_cnt<mesh->face_group_cnt;fg_cnt++)
			{
				Material *tmat = &mesh->material_list[mesh->face_groups[fg_cnt].material];
							
				if (strstr(tmat->name, "markings") != 0)
				{
					color = COLORTABLE[MGlobals::GetColorID(playerID)];

					tmat->diffuse.r = GetRValue(color);
					tmat->diffuse.g = GetGValue(color);
					tmat->diffuse.b = GetBValue(color);

					tmat->emission.r = 0.3*tmat->diffuse.r;
					tmat->emission.g = 0.3*tmat->diffuse.g;
					tmat->emission.b = 0.3*tmat->diffuse.b;
				}

				FaceGroupInfo *fgi = &(meshE->mc.mi[c]->fgi[fg_cnt]);
				
				fgi->a = alpha;

				SINGLE redShift = (((SINGLE)(red))/255);
				SINGLE greenShift = (((SINGLE)(green))/255);
				SINGLE blueShift = (((SINGLE)(blue))/255);
				fgi->diffuse.r = tmat->diffuse.r*redShift;
				fgi->diffuse.g = tmat->diffuse.g*greenShift;
				fgi->diffuse.b = tmat->diffuse.b*blueShift;

				fgi->emissive.r = tmat->emission.r*redShift;
				fgi->emissive.g = tmat->emission.g*greenShift;
				fgi->emissive.b = tmat->emission.b*blueShift;
			}
		}
	}

}
//--------------------------------------------------------------------------//
//
void UnbornMeshList::setMeshTransform(MeshEntry * mesh, TRANSFORM trans)
{
	ENGINE->set_transform(mesh->instanceIndex,trans);
	ENGINE->update_instance(mesh->instanceIndex,0,0);
}
//--------------------------------------------------------------------------//
//
void UnbornMeshList::init()
{
	firstMesh = NULL;
	for(U32 i = 0; i < MAX_PLAYERS; ++i)
		firstQueue[i]= NULL;
}
//---------------------------------------------------------------------------
//
bool UnbornMeshList::EnumerateAgent (const NODE & node)
{
	while(currentNode)
	{
		if(currentNode->opID == node.opID)
		{
			if(node.pSet->numObjects)
			{
				prevNode = currentNode;
				currentNode = currentNode->next;
				return true;
			}
			else
			{
				removeFromQueue(currentNode,prevNode);
				if(prevNode)
					currentNode = prevNode->next;
				else
					currentNode = firstQueue[currentPlayerID];
			}
		}
		else if(currentNode->opID <= node.opID)
		{
			removeFromQueue(currentNode,prevNode);
			if(prevNode)
				currentNode = prevNode->next;
			else
				currentNode = firstQueue[currentPlayerID];
		}
		else
			break;
	}
	if(node.pSet->numObjects)
	{
		addToQueue(node.pSet->objectIDs[0],node.opID,node.dwFabArchetype,node.targetPosition,node.targetSystemID,node.fabTargetID,node.fabSlotID,prevNode);
		if(prevNode)
			prevNode = prevNode->next;
		else
			prevNode = firstQueue[currentPlayerID];
		currentNode = prevNode->next;
	}
	return true;
}

struct JumpCallback : public ITerrainSegCallback
{
	OBJPTR<IJumpGate> jumpgate;

	JumpCallback(){jumpgate = NULL;};

	virtual bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
	{
		if((info.flags & (TERRAIN_FULLSQUARE | TERRAIN_PARKED)) == (TERRAIN_FULLSQUARE | TERRAIN_PARKED))
		{
			OBJLIST->FindObject(info.missionID,NONSYSVOLATILEPTR,jumpgate,IJumpGateID);
			if(jumpgate)
				return false;
		}
		return true;
	}
};

struct PlanetCallback : public ITerrainSegCallback
{
	OBJPTR<IPlanet> planet;

	PlanetCallback(){planet = NULL;};

	virtual bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
	{
		if((info.flags & (TERRAIN_FULLSQUARE | TERRAIN_PARKED | TERRAIN_BLOCKLOS | TERRAIN_IMPASSIBLE)) == 
			(TERRAIN_FULLSQUARE | TERRAIN_PARKED | TERRAIN_BLOCKLOS | TERRAIN_IMPASSIBLE))
		{
			OBJLIST->FindObject(info.missionID,NONSYSVOLATILEPTR,planet,IPlanetID);
			if(planet)
				return false;
		}
		return true;
	}
};

struct DeepSpaceCallback : public ITerrainSegCallback
{
	bool empty;
	GRIDVECTOR targPos;

	DeepSpaceCallback(){empty = true;};

	virtual bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
	{
		if((info.flags & TERRAIN_IMPASSIBLE) ||
			((info.flags & (TERRAIN_FULLSQUARE |TERRAIN_PARKED)) == (TERRAIN_FULLSQUARE |TERRAIN_PARKED)) ||
			(((info.flags & (TERRAIN_HALFSQUARE |TERRAIN_PARKED)) == (TERRAIN_HALFSQUARE |TERRAIN_PARKED)) && pos == targPos) ||
			(((info.flags & (TERRAIN_HALFSQUARE |TERRAIN_WILLBEPLAT)) == (TERRAIN_HALFSQUARE |TERRAIN_WILLBEPLAT)) && pos == targPos))
		{
			empty = false;
			return false;
		}
		return true;
	}
};

//---------------------------------------------------------------------------
//
void UnbornMeshList::Render()
{
	for(U32 i = 0; i < MAX_PLAYERS; ++i)
	{
		currentPlayerID = i;
		prevNode = NULL;
		currentNode = firstQueue[i];
		THEMATRIX->EnumerateFabricateAgents(i+1,this);
		while(currentNode)
		{
			FabQueueStruct * node = currentNode->next;
			removeFromQueue(currentNode,prevNode);	
			currentNode = node;
		}
	}
	renderQueue();
	if(BUILDARCHEID && CURSOR->GetCursor() == IDC_CURSOR_BUILD)
	{
		S32 x, y;
		WM->GetCursorPos(x, y);

		if (y <= CAMERA->GetPane()->y1)
		{
			Vector vec;

			vec.x = x;
			vec.y = y;
			vec.z = 0;

			if(CAMERA->ScreenToPoint(vec.x, vec.y, vec.z) != 0)
			{
				GRIDVECTOR pos;
				pos = vec;
				BASIC_DATA * data = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(BUILDARCHEID));
				if(data->objClass == OC_PLATFORM)
				{
					BASE_PLATFORM_DATA * platData = (BASE_PLATFORM_DATA *) data;
					bool defaultRender = true;
					if(platData->type == PC_JUMPPLAT)
					{
						COMPTR<ITerrainMap> map;
						SECTOR->GetTerrainMap(SECTOR->GetCurrentSystem(),map);
						if(map)
						{
							JumpCallback jumpCallback;
							map->TestSegment(pos,pos,&jumpCallback);
							if(jumpCallback.jumpgate && jumpCallback.jumpgate->IsHighlightingBuild())
							{
								defaultRender = false;
							}
						}
					}
					else
					{
						if(platData->slotsNeeded == 0)//deep space
						{
							COMPTR<ITerrainMap> map;
							SECTOR->GetTerrainMap(SECTOR->GetCurrentSystem(),map);
							if(map)
							{
								if(IsFullGrid(BUILDARCHEID))
									pos.centerpos();
								else
									pos.quarterpos();
								if(map->IsGridInSystem(pos))
								{
									DeepSpaceCallback deepCallback;
									deepCallback.targPos = pos;
									map->TestSegment(pos,pos,&deepCallback);
									if(deepCallback.empty && map->IsGridInSystem(pos))
									{
										defaultRender = false;
										TRANSFORM trans;
										trans.rotate_about_i(PI/2);
										trans.translation = pos;
										RenderMeshAt(trans,BUILDARCHEID,MGlobals::GetThisPlayer(),0,255,0,128);
									}
								}
							}
							
						}
						else
						{
							COMPTR<ITerrainMap> map;
							SECTOR->GetTerrainMap(SECTOR->GetCurrentSystem(),map);
							if(map)
							{
								PlanetCallback planetCallback;
								map->TestSegment(pos,pos,&planetCallback);
								if(planetCallback.planet && planetCallback.planet->IsHighlightingBuild())
								{
									defaultRender = false;
								}
							}
						}
					}
					if(defaultRender)
					{
						TRANSFORM trans;
						trans.rotate_about_i(PI/2);
						trans.translation = vec;
						RenderMeshAt(trans,BUILDARCHEID,MGlobals::GetThisPlayer(),255,0,0,128);
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void UnbornMeshList::GetBoundingSphere(TRANSFORM trans,U32 archID,float & obj_rad,Vector &wcenter)
{
	MeshEntry * mesh = findMesh(archID);
	if(!mesh)
	{
		mesh = createMesh(archID);
	}
	CQASSERT(mesh);
	setMeshTransform(mesh,trans);
	CQASSERT(mesh->instanceIndex != INVALID_INSTANCE_INDEX);

	ENGINE->get_instance_bounding_sphere(mesh->instanceIndex,0,&obj_rad,&wcenter);
}
//---------------------------------------------------------------------------
//
MeshChain * UnbornMeshList::GetMeshChain(const TRANSFORM &trans,U32 archID)
{
	MeshEntry * mesh = findMesh(archID);
	if(!mesh)
	{
		mesh = createMesh(archID);
	}
	CQASSERT(mesh);
	setMeshTransform(mesh,trans);
	CQASSERT(mesh->instanceIndex != INVALID_INSTANCE_INDEX);
	return &mesh->mc;
}
//---------------------------------------------------------------------------
//
bool UnbornMeshList::IsFullGrid (U32 archID)
{
	MeshEntry * mesh = findMesh(BUILDARCHEID);
	if(!mesh)
	{
		mesh = createMesh(BUILDARCHEID);
	}

	PLATFORM_INIT<BASE_PLATFORM_DATA> * initData = (PLATFORM_INIT<BASE_PLATFORM_DATA> *) (ARCHLIST->GetArchetypeHandle(mesh->pArchetype));
	return (initData->fp_radius > GRIDSIZE/2);
}
//---------------------------------------------------------------------------
//
void UnbornMeshList::clearQueue()
{
	for(U32 i = 0; i < MAX_PLAYERS; ++i)
	{
		currentPlayerID = i;
		while(firstQueue[i])
		{
			removeFromQueue(firstQueue[i],NULL,true);
		}
	}
}
//---------------------------------------------------------------------------
//
void UnbornMeshList::addToQueue(U32 fabID,U32 opID, U32 archID, Vector position, U32 systemID, U32 targetID, U32 slotID,FabQueueStruct * prev)
{
	FabQueueStruct * node = new FabQueueStruct;
	node->opID = opID;
	node->archID = archID;
	node->systemID = systemID;
	node->altSystemID = 0;
	node->targetID = targetID;
	node->slotID = slotID;
	OBJLIST->FindObject(fabID,NONSYSVOLATILEPTR,node->fab,IFabricatorID);
	if(targetID)
	{
		IBaseObject * ptr = OBJLIST->FindObject(targetID);
		if(ptr->objClass == OC_PLANETOID)
		{
			OBJPTR<IPlanet> planet;
			ptr->QueryInterface(IPlanetID,planet);

			node->trans = planet->GetSlotTransform(slotID);
			planet->MarkSlot(currentPlayerID+1,slotID,archID);
		}
		else//jumpgate
		{
			TRANSFORM trans;
			trans.rotate_about_i(PI/2);

			OBJPTR<IJumpGate> gate;
			ptr->QueryInterface(IJumpGateID,gate);
			gate->Mark(currentPlayerID+1);
			trans.translation = ptr->GetPosition();
			node->trans = trans;

			ptr = SECTOR->GetJumpgateDestination(ptr);
			ptr->QueryInterface(IJumpGateID,gate);
			gate->Mark(currentPlayerID+1);
			node->altSystemID = ptr->GetSystemID();
			trans.set_identity();
			trans.rotate_about_i(PI/2);
			trans.translation = ptr->GetPosition();
			node->altTrans = trans;
		}
	}
	else//deep space;
	{
		GRIDVECTOR vect;
		vect = position;
		node->trans.set_identity();
		node->trans.rotate_about_i(PI/2.0);
		node->trans.translation = vect;

		COMPTR<ITerrainMap> map;
		SECTOR->GetTerrainMap(node->systemID, map);

		FootprintInfo fpi;
		fpi.flags = TERRAIN_HALFSQUARE | TERRAIN_DESTINATION | TERRAIN_WILLBEPLAT;
		fpi.height = 0;
		fpi.missionID = ((node->opID) << 4)+10;
		map->SetFootprint(&vect, 1, fpi); 
	}
	if(prev)
	{
		node->next = prev->next;
		prev->next = node;
	}
	else
	{
		node->next = firstQueue[currentPlayerID];
		firstQueue[currentPlayerID] = node;
	}
}
//---------------------------------------------------------------------------
//
void UnbornMeshList::removeFromQueue(FabQueueStruct * node,FabQueueStruct * prev,bool bClosing)
{
	if(!bClosing)
	{
		if(node->targetID)
		{
			if(node->slotID)//standard plat
			{
				OBJPTR<IPlanet> planet;
				OBJLIST->FindObject(node->targetID,TOTALLYVOLATILEPTR,planet,IPlanetID);
			
				planet->UnmarkSlot(currentPlayerID+1,node->slotID);
			}
			else//jumpplat
			{
				OBJPTR<IJumpGate> gate;
				OBJLIST->FindObject(node->targetID,TOTALLYVOLATILEPTR,gate,IJumpGateID);
				gate->Unmark(currentPlayerID+1);
				SECTOR->GetJumpgateDestination(gate.Ptr())->QueryInterface(IJumpGateID,gate);
				gate->Unmark(currentPlayerID+1);
			}
		}
		else //deep space platform
		{
			COMPTR<ITerrainMap> map;
			SECTOR->GetTerrainMap(node->systemID, map);

			GRIDVECTOR vect;
			vect = node->trans.translation;

			FootprintInfo fpi;
			fpi.flags = TERRAIN_HALFSQUARE | TERRAIN_DESTINATION | TERRAIN_WILLBEPLAT;
			fpi.height = 0;
			fpi.missionID = ((node->opID) << 4)+10;
			map->UndoFootprint(&vect, 1, fpi); 
		}
	}
	if(prev)
		prev->next = node->next;
	else
		firstQueue[currentPlayerID] = node->next;
	delete node;
}
//---------------------------------------------------------------------------
//
void UnbornMeshList::renderQueue()
{
	currentPlayerID = MGlobals::GetThisPlayer()-1;
	FabQueueStruct * node = firstQueue[currentPlayerID];
	while(node)
	{
		if(node->systemID == SECTOR->GetCurrentSystem())
		{
			if(node->fab && (!(node->fab->IsBuildingAgent(node->opID))))
				RenderMeshAt(node->trans,node->archID,MGlobals::GetThisPlayer(),0,255,0,128);
		}
		else if(node->altSystemID == SECTOR->GetCurrentSystem())
		{
			if(node->fab && (!(node->fab->IsBuildingAgent(node->opID))))
				RenderMeshAt(node->altTrans,node->archID,MGlobals::GetThisPlayer(),0,255,0,128);
		}
		node = node->next;
	}
}
//--------------------------------------------------------------------------//
//
struct _unbornMang : GlobalComponent
{
	UnbornMeshList * unbornMeshList;

	virtual void Startup (void)
	{
		UNBORNMANAGER = unbornMeshList = new DAComponent<UnbornMeshList>;
		AddToGlobalCleanupList(&unbornMeshList);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		unbornMeshList->init();
	}
};

static _unbornMang globalUnbornList;


//---------------------------------------------------------------------------
//------------------------End UnbornMeshList.cpp----------------------------------------
//---------------------------------------------------------------------------


