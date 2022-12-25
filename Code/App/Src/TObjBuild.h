#ifndef TOBJBUILD_H
#define TOBJBUILD_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              TObjBuild.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjBuild.h 138   6/22/01 4:03p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef MESH_H
#include "Mesh.h"
#endif

#ifndef RENDERER_H
#include "Renderer.h"
#endif

//#ifndef MATERIAL_H
//#include "Material.h"
//#endif

#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

#ifndef ENGINE_H
#include "Engine.h"
#endif

#ifndef IBUILD_H
#include "IBuild.h"
#endif

#ifndef FILESYS_H
#include <FileSys.h>
#endif

#ifndef IBUILDSHIP_H
#include "IBuildShip.h"
#endif

#ifndef DSHIPSAVE_H
#include "DShipSave.h"
#endif

#ifndef ARCHHOLDER_H
#include "ArchHolder.h"
#endif
/*
#ifndef EXPLOSION_H
#include "IExplosion.h"
#endif*/

#ifndef OPAGENT_H
#include "OpAgent.h"
#endif

#ifndef IBANKER_H
#include "IBanker.h"
#endif

#ifndef UNITCOMM_H
#include "UnitComm.h"
#endif

#ifndef CQBATCH_H
#include "CQBatch.h"
#endif

#ifndef IACTIVEBUTTON_H
#include "IActiveButton.h"
#endif

#ifndef MSCRIPT_H
#include "MScript.h"
#endif

#ifndef MPART_H
#include "MPart.h"
#endif

#ifndef IJUMPPLAT_H
#include "IJumpplat.h"
#endif

#ifndef UNITCOMM_H
#include "UnitComm.h"
#endif

#define SLICES 10
#define REVERSE_SPEED_FACTOR 5.0f

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//





#define ObjectBuild _Cob



template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectBuild : public Base, IBuild, BUILD_SAVELOAD
{
	struct PreRenderNode	preRenderNode;
	struct PostRenderNode   postRenderNode;
	struct UpdateNode       updateNode;
	struct PhysUpdateNode	physUpdateNode;
	struct SaveNode			saveNode;
	struct LoadNode         loadNode;
	struct ExplodeNode      explodeNode;
	struct ResolveNode		resolveNode;
	struct InitNode			initNode;
	struct PreDestructNode	destructNode;
		
	typename typedef Base::SAVEINFO BUILDSAVEINFO;
	typename typedef Base::INITINFO BUILDINITINFO;
	//----------------------------------	
	OBJPTR<IBuildEffect> buildEffectObj;
	
	SINGLE emitterPause;
	bool emissiveMask:1;

//	U8 numFabWorking;
	U32 fabID;//[MAX_FAB_WORKING];

	ObjectBuild (void);
	~ObjectBuild (void);

	// IBuild methods
	virtual void Build (IFabricator *_fab);
	virtual void AddFabToBuild (IFabricator * _fab);
	virtual void RemoveFabFromBuild (IFabricator * _fab);
	virtual U32 NumFabOnBuild ();
//	virtual U32 GetPrimaryBuilder ();
	virtual void EndBuild (bool bAborting);
	virtual void CancelBuild ();
	virtual void HaltBuild ();
	virtual void EnableCompletion ();
	virtual SINGLE GetBuildProgress (U32 & stallType);
	virtual SINGLE GetBuildDisplayProgress (U32 & stallType);
	virtual bool IsReversing () ;
	virtual bool IsComplete () ;
	virtual bool IsPaused () ;
	virtual void SetProcessID (U32 processID);

	virtual U32 GetProcessID () ;

	virtual void SetBuilderType (U32 builderType);

	virtual S32 GetBuilderType ();

	virtual void BeginDismantle(IBaseObject * _fab);
	
	/* ObjectBuild methods */

	void resolveBuild();
	void saveBuild(BUILDSAVEINFO &);
	void loadBuild(BUILDSAVEINFO &);
	void explodeBuild (bool bExplode);
	void preDestructBuild (void);
	void buildPreRender (void);
	void buildPostRender (void);
	void buildInit(const BUILDINITINFO &);
	BOOL32 updateBuild (void);
	void physUpdateBuild (SINGLE dt);

	virtual void failSound(M_RESOURCE_TYPE resType);

//	void markChild (INSTANCE_INDEX parentIdx);
	//virtual void SetupMesh(SINGLE fractionPerFrame);


};

//---------------------------------------------------------------------------
//
template <class Base> 
ObjectBuild< Base >::ObjectBuild (void) :
					preRenderNode(this, RenderProc(&ObjectBuild::buildPreRender)),
					postRenderNode(this, RenderProc(&ObjectBuild::buildPostRender)),
					updateNode(this, UpdateProc(&ObjectBuild::updateBuild)),
					physUpdateNode(this, PhysUpdateProc(&ObjectBuild::physUpdateBuild)),
					saveNode(this, SaveLoadProc(&ObjectBuild::saveBuild)),
					loadNode(this, SaveLoadProc(&ObjectBuild::loadBuild)),
					explodeNode(this, ExplodeProc(&ObjectBuild::explodeBuild)),
					resolveNode(this, ResolveProc(&ObjectBuild::resolveBuild)),
					initNode(this, InitProc(&ObjectBuild::buildInit)),
					destructNode(this, PreDestructProc(&ObjectBuild::preDestructBuild))

{
	building = 0;
	whole = 1;
//	numFabWorking = 0;
}
//---------------------------------------------------------------------------
//

template <class Base> 
ObjectBuild< Base >::~ObjectBuild (void) 
{
	//fabricator owns build effect
	//delete buildEffectObj;
}

/*

template <class Base>
void ObjectBuild< Base >::markChild (INSTANCE_INDEX parentIdx)
{
	INSTANCE_INDEX child = MODEL->get_child(parentIdx);
	while ((child != INVALID_INSTANCE_INDEX) && (num_childs < MAX_CHILDS))
	{
		mc[num_childs].instanceIndex = child;
		num_childs++;
		markChild(child);
		child = MODEL->get_child(parentIdx, child);
		
	}
}*/

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base >::Build (IFabricator *_fab)
{
	SetColors();
//	CQASSERT(!numFabWorking);
	hullPointsAdded=0;
	hullPointsFinish = hullPointsMax;
	hullPoints = 0;
	if(objClass == OC_PLATFORM)
	{
		OBJPTR<IJumpPlat> plat;
		static_cast<IBaseObject *>(this)->QueryInterface(IJumpPlatID,plat);
		if(plat)
		{
			plat->SetSiblingHull(0);
		}
	}

	SetReady(false); 
	bReverseBuild = false;
	building = 1;
	whole = 0;
	timeSpent = 0;
	OBJPTR<IFabricator> fab;
	_fab->QueryInterface(IFabricatorID,fab);
	fabID = fab.Ptr()->GetPartID();

/*	IBaseObject * obj = _fab->GetBuildEffectObj();
	if (obj && obj->QueryInterface(IBuildEffectID,buildEffectObj,NONSYSVOLATILEPTR))
	{
		if (buildEffectObj)
		{			
			U32 buildTime = (pInitData->buildTime*(1.0-MGlobals::GetAIBonus(playerID)));
			buildEffectObj->SetupMesh(fab.Ptr(),this,mesh_info,instanceIndex,buildTime?buildTime:1.0);
		//	bSpecialRender = true;
			buildEffectObj->AddFabricator(_fab);
			buildEffectObj->SetBuildRate(buildTime?1.0/buildTime:1.0);
		}
	}
*/
	SetVisibilityFlags(fab.Ptr()->GetVisibilityFlags());
	emissiveMask = true;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base >::AddFabToBuild (IFabricator * _fab)
{
	CQASSERT(building);
	
	OBJPTR<IFabricator> fab;
	_fab->QueryInterface(IFabricatorID,fab);
	fabID = fab.Ptr()->GetPartID();
	
	//kind of hacky, this code should only run on load
/*	IBaseObject * obj = _fab->GetBuildEffectObj();
	if (obj && obj->QueryInterface(IBuildEffectID,buildEffectObj,NONSYSVOLATILEPTR))
	{
		if (buildEffectObj)
		{
			U32 buildTime = (pInitData->buildTime*(1.0-MGlobals::GetAIBonus(playerID)));
			buildEffectObj->SetupMesh(fab.Ptr(),this,mesh_info,instanceIndex,buildTime?buildTime:1.0);
			//	bSpecialRender = true;
			buildEffectObj->SetBuildPercent(buildTime?((SINGLE)timeSpent)/buildTime:1.0);
		}
	}
*/	
	if (buildEffectObj)
	{
		buildEffectObj->AddFabricator(_fab);
		U32 buildTime = (pInitData->buildTime*(1.0-MGlobals::GetAIBonus(playerID)));
		buildEffectObj->SetBuildRate(buildTime?(1.0f/buildTime):1.0);
	}

}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base >::RemoveFabFromBuild (IFabricator * _fab)
{
	fabID = 0;
	if (buildEffectObj)
		buildEffectObj->RemoveFabricator(_fab);

}
//---------------------------------------------------------------------------
//
template <class Base>
U32 ObjectBuild< Base >::NumFabOnBuild ()
{
	return (building ? 1 : 0);
}

//---------------------------------------------------------------------------
//
//	obsolete 
/*template <class Base>
U32 ObjectBuild< Base >::GetPrimaryBuilder ()
{
	return fabID[0];
}*/
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base >::EndBuild (bool bAborting)
{
	if (building)
	{
		building = FALSE;
		whole = !bAborting;
		
		if (bAborting == FALSE)
		{
			if((mObjClass != M_HARVEST) 
				&&	(mObjClass != M_GALIOT) &&(mObjClass != M_SIPHON))
				supplies = supplyPointsMax;
			SetReady(true);
			emitterPause = 2.0;
			if(objClass == OC_PLATFORM)
			{
				PLATFORM_ALERT(this, dwMissionID, constructComplete,SUB_CONSTRUCTION_COMP, pInitData->displayName,ALERT_PLATFORM_BUILD);
			}
			else
			{
				SPACESHIP_ALERT(this, dwMissionID, constructComplete,SUB_CONSTRUCTION_COMP, pInitData->displayName,ALERT_UNIT_BUILT);
			}

			MScript::RunProgramsWithEvent(CQPROGFLAG_OBJECTCONSTRUCTED, dwMissionID);

			if (buildEffectObj)
			{
				buildEffectObj->SetBuildPercent(1.0);
				buildEffectObj->Done();
			//	bSpecialRender = false;
				//hang around for unbuild right now
				buildEffectObj = 0;
			//	delete buildEffectObj;
			}
			
		}
		else
		{
			if(THEMATRIX->IsMaster())
			{
				THEMATRIX->ObjectTerminated(GetPartID());
			}
			fabID = 0;
		}

		pause = false;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base >::CancelBuild()
{
	if(building)
	{
		bReverseBuild = true;
		U32 buildTime = (pInitData->buildTime*(1.0-MGlobals::GetAIBonus(playerID)));
		if (buildEffectObj)
			buildEffectObj->SetBuildRate(buildTime?-5.0/buildTime:-5.0);

		pause = false;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base >::HaltBuild ()
{
	fabID = 0;
	building = FALSE;
	whole = false;
	pause = false;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base>::EnableCompletion ()
{
//	buildAgentID = agentID;
	if(THEMATRIX->IsMaster())
	{
		U32 newHull = hullPoints +( hullPointsFinish-hullPointsAdded);
		if(newHull > hullPointsMax)
			newHull = hullPointsMax;
		hullPoints = newHull;
		if(objClass == OC_PLATFORM)
		{
			OBJPTR<IJumpPlat> plat;
			static_cast<IBaseObject *>(this)->QueryInterface(IJumpPlatID,plat);
			if(plat)
			{
				plat->SetSiblingHull(newHull);
			}
		}

	}
	OBJPTR<IFabricator> fab;

	IBaseObject *obj1;
	obj1 = OBJLIST->FindObject(fabID);
	CQASSERT(obj1 && "Fabricator object has disappeared but buildee remains - Abort or suffer");
	obj1->QueryInterface(IFabricatorID,fab);
//	whole=1;
	fab->FabEndBuild(bReverseBuild);
	pause = false;
}
//---------------------------------------------------------------------------
//
template <class Base>
SINGLE ObjectBuild< Base>::GetBuildProgress (U32 & stallType)
{
	stallType = IActiveButton::NO_STALL;

	U32 buildTime = (pInitData->buildTime*(1.0-MGlobals::GetAIBonus(playerID)));
	if(bReverseBuild)
	{
		return 1.0-(buildTime?((SINGLE)timeSpent)/buildTime:0.0);
	}

	if (CQFLAGS.bInstantBuilding)
	{
		return 1.0f;
	}
	return buildTime?((SINGLE)timeSpent)/buildTime:1.0;
}
//---------------------------------------------------------------------------
//
template <class Base>
SINGLE ObjectBuild< Base>::GetBuildDisplayProgress (U32 & stallType)
{
	stallType = IActiveButton::NO_STALL;
	U32 buildTime = (pInitData->buildTime*(1.0-MGlobals::GetAIBonus(playerID)));
	return buildTime?((SINGLE)timeSpent)/buildTime:1.0;
}
//---------------------------------------------------------------------------
//
template <class Base>
bool ObjectBuild< Base>::IsReversing ()
{
	return bReverseBuild;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base>::SetProcessID (U32 processID)
{
	if(processID)
	{
		CQASSERT(!buildProcessID);
		buildProcessID = processID;
	}
	else
	{
		CQASSERT(buildProcessID);
		buildProcessID = 0;
	}

}
//---------------------------------------------------------------------------
//
template <class Base>
U32 ObjectBuild< Base>::GetProcessID ()
{
	return buildProcessID;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base>::SetBuilderType (U32 _builderType)
{
	builderType = _builderType;
}
//---------------------------------------------------------------------------
//
template <class Base>
S32 ObjectBuild< Base>::GetBuilderType ()
{
	return builderType;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base>::BeginDismantle(IBaseObject * _fab)
{
	bDismantle = true;
	building = true;
	SetReady(false);
	if(bSelected)
		OBJLIST->UnselectObject(this);
	bReverseBuild = true;
	whole = false;

	U32 buildTime = (pInitData->buildTime*(1.0-MGlobals::GetAIBonus(playerID)));
	timeSpent = buildTime;

//	percent = GetTrueHullPoints()/hullPointsMax;

	hullPointsAdded = hullPoints;
	hullPointsFinish = hullPointsAdded;

	OBJPTR<IFabricator> fab;
	_fab->QueryInterface(IFabricatorID,fab);

	if (buildEffectObj)
	{
	//	bSpecialRender = true;
		buildEffectObj->AddFabricator(fab);
		buildEffectObj->SetBuildRate(buildTime?-REVERSE_SPEED_FACTOR/buildTime:-1.0);
	}
	else
	{
/*		IBaseObject * obj = fab->GetBuildEffectObj();
		if (obj && obj->QueryInterface(IBuildEffectID,buildEffectObj,NONSYSVOLATILEPTR))
		{
			if (buildEffectObj)
			{
				buildEffectObj->SetupMesh(fab.Ptr(),this,mesh_info,instanceIndex,buildTime?buildTime:1.0);
		//		bSpecialRender = true;
				buildEffectObj->AddFabricator(fab);
				buildEffectObj->SetBuildRate(buildTime?-REVERSE_SPEED_FACTOR/buildTime:-1.0);
				buildEffectObj->SetBuildPercent(1.0);
			}
		}
*/	}
	
//	SetupMesh(buildRate/(pInitData->buildCost*REALTIME_FRAMERATE));
//	ReBuild();

	fabID = fab.Ptr()->GetPartID();

	//useful?
/*	OBJPTR<IBuildShip> drone;
	for(U32 c = 0; c<fab->GetNumDrones() ; c++)
	{
		IBaseObject * obj;
		fab->GetDrone(&obj,c);
		obj->QueryInterface(IBuildShipID,drone);
		Transform trans = GetTransform();
		Vector bob = mc[0].buildMesh->object_vertex_list[mc[0].buildMesh->vertex_batch_list[mc[0].buildMesh->face_groups[mc[0].myFaceArray[0].groupID]
					.face_vertex_chain[mc[0].myFaceArray[0].index*3] ]];
		Vector nBob = mc[0].buildMesh->normal_ABC
			[
				mc[0].buildMesh->vertex_normal
				[
					mc[0].buildMesh->vertex_batch_list
					[
						mc[0].buildMesh->face_groups
						[
							mc[0].myFaceArray[0].groupID
						]
						.face_vertex_chain
						[
							mc[0].myFaceArray[0].index*3
						] 
					]
				]
			];
		if(c)
		{
			bob.y += numFabWorking*300+c*100;
			bob = trans.rotate_translate(bob);
			drone->IdleAtPos(bob);
		}
		else
		{
			bob = trans.rotate_translate(bob);
			nBob = trans.rotate(nBob);
			drone->BuildAtPos(bob,nBob);
		}
	}*/

	emissiveMask = true;

}
//---------------------------------------------------------------------------
//
template <class Base>
bool ObjectBuild< Base>::IsComplete ()
{
	return (!building);
}

template <class Base>
bool ObjectBuild< Base>::IsPaused ()
{
	return pause;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base>::resolveBuild ()
{
/*	if (building)
	{
	
	}*/
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base>::loadBuild(BUILDSAVEINFO &loadInfo)
{
	*static_cast<BUILD_SAVELOAD *> (this) = loadInfo.build_SL;
	/*OBJPTR<IFabricator> fab;
	IBaseObject *obj;
	obj = OBJLIST->FindObject(fabID);*/
//	drone[0] = NULL;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base>::saveBuild(BUILDSAVEINFO &saveInfo)
{
	saveInfo.build_SL = *static_cast<BUILD_SAVELOAD *> (this);
/*	if (rate)
	{
		//should be kept up to date now
//		SINGLE checkVal =  min(1,(perFrame*buildTimer)/rate);
//		CQASSERT(saveInfo.build_SL.percent == checkVal);
//			saveInfo.build_SL.percent = (perFrame*buildTimer)/rate;
	}
	else
		saveInfo.build_SL.percent = 0;*/
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectBuild< Base >::buildPreRender (void)
{
	if(emissiveMask)
	{
		if(!bReady)
		{
		/*	for (int i=0;i<mc.numChildren;i++)
			{
				mc.mi[i]->bSuppressEmissive = true;
			}
			Mesh * mesh = REND->get_instance_mesh(instanceIndex);
			for(S32 count = 0; count < mesh->material_cnt; ++count)
			{
				if(!(mesh->material_list[count].flags & MF_NO_EMITTER_PASS))
				{
					mesh->material_list[count].flags |=MF_NO_EMITTER_PASS;
					COMPTR<ICQBatch> batcher;
					BATCH->QueryInterface("ICQBatch",batcher);
					if (batcher)
						batcher->InvalidateMaterial((U32)(&(mesh->material_list[count])));
				}
			}*/
		}
		else if(emitterPause > 0)
		{
			SINGLE num = rand()%1000;
			num = num/500;
			if(num > emitterPause)
			{
			/*	for (int i=0;i<mc.numChildren;i++)
				{
					mc.mi[i]->bSuppressEmissive = false;
				}
				Mesh * mesh = REND->get_instance_mesh(instanceIndex);
				for(S32 count = 0; count < mesh->material_cnt; ++count)
				{
					if(mesh->material_list[count].flags & MF_NO_EMITTER_PASS)
					{
						mesh->material_list[count].flags &= (~MF_NO_EMITTER_PASS);
						COMPTR<ICQBatch> batcher;
						BATCH->QueryInterface("ICQBatch",batcher);
						if (batcher)
							batcher->InvalidateMaterial((U32)(&(mesh->material_list[count])));
					}
				}*/
			}
			else
			{
			/*	for (int i=0;i<mc.numChildren;i++)
				{
					mc.mi[i]->bSuppressEmissive = true;
				}
				Mesh * mesh = REND->get_instance_mesh(instanceIndex);
				for(S32 count = 0; count < mesh->material_cnt; ++count)
				{
					if(!(mesh->material_list[count].flags & MF_NO_EMITTER_PASS))
					{
						mesh->material_list[count].flags |=MF_NO_EMITTER_PASS;
						COMPTR<ICQBatch> batcher;
						BATCH->QueryInterface("ICQBatch",batcher);
						if (batcher)
							batcher->InvalidateMaterial((U32)(&(mesh->material_list[count])));
					}
				}*/
			}
			
		}
		else
		{
	/*		for (int i=0;i<mc.numChildren;i++)
			{
				mc.mi[i]->bSuppressEmissive = false;
			}
			Mesh * mesh = REND->get_instance_mesh(instanceIndex);
			for(S32 count = 0; count < mesh->material_cnt; ++count)
			{
				if(mesh->material_list[count].flags & MF_NO_EMITTER_PASS)
				{
					mesh->material_list[count].flags &= (~MF_NO_EMITTER_PASS);
					COMPTR<ICQBatch> batcher;
					BATCH->QueryInterface("ICQBatch",batcher);
					if (batcher)
						batcher->InvalidateMaterial((U32)(&(mesh->material_list[count])));
				}
			}*/
			emissiveMask = false;
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectBuild< Base >::buildPostRender (void)
{
	if (!whole && !bExploding)
		if (buildEffectObj)
			buildEffectObj.Ptr()->Render();
/*	if (!whole && !bExploding)
	{
		BATCH->set_state(RPR_BATCH,1);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		//BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		//BATCH->set_render_state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
		renderMeshChunk(buildTex[race],FS_BUILDING);
	}*/
}


template <class Base>
void ObjectBuild< Base >::buildInit(const BUILDINITINFO &arch)
{	
	timeSpent = 0.0;
	emitterPause = 0;
	buildProcessID = 0;

/*	IBaseObject *obj = ARCHLIST->CreateInstance(arch.pBuildEffect);
	if (obj)
	{
		obj->QueryInterface(IBuildEffectID,buildEffectObj);
		CQASSERT(buildEffectObj);
	}*/
}

//---------------------------------------------------------------------------
//

template <class Base>
BOOL32 ObjectBuild< Base >::updateBuild (void)
{
	if (building)
	{
		U32 buildTime = (pInitData->buildTime*(1.0-MGlobals::GetAIBonus(playerID)));
		OBJPTR<IBuildQueue> queue;
		IBaseObject *obj1;
		obj1 = OBJLIST->FindObject(fabID);
		U32 stallType=0;
		if (obj1)
		{
			obj1->QueryInterface(IBuildQueueID,queue);
			if (queue)
				queue->FabGetProgress(stallType);
		}
		if (objClass == OC_PLATFORM || ((SECTOR->SystemInSupply(systemID,playerID) || bReverseBuild) && (stallType!=IActiveButton::NO_MONEY)))
		{
			SINGLE industryBonus = 1.0;
			if(obj1)
				industryBonus = obj1->effectFlags.getIndustrialBoost();
			if (pause)
			{
				pause=false;
				if (buildEffectObj)
					buildEffectObj->PauseBuildEffect(false);
				if(obj1)
				{
					VOLPTR(IFabricator) fab=obj1;
					fab->FabSetBuildPause(false,buildTime?timeSpent/buildTime:1.0);
				}
			}
			
			if (buildEffectObj)
				buildEffectObj->Update();
			
			if(bReverseBuild)
			{
				if(ELAPSED_TIME*REVERSE_SPEED_FACTOR > timeSpent)
					timeSpent = 0;
				else
					timeSpent -= (ELAPSED_TIME*REVERSE_SPEED_FACTOR*industryBonus);
			}
			else
			{
				timeSpent += (ELAPSED_TIME*industryBonus);
				if(buildTime < timeSpent)
					timeSpent = buildTime;
			}
			//		droneMoveTime += REALTIME_FRAMERATE;
			if (buildEffectObj)
				buildEffectObj->SetBuildPercent(buildTime?timeSpent/buildTime:1.0);
			//buildTimer = newTimer;
			if (bReverseBuild || (hullPoints != hullPointsMax))
			{
				S32 oldHull = hullPoints;
				S32 newHull = oldHull +( (buildTime?timeSpent/buildTime:1.0)*hullPointsFinish-hullPointsAdded);
				if(newHull < 0)
					newHull = 0;
				if(newHull > hullPointsMax)
					newHull = hullPointsMax;
				if(THEMATRIX->IsMaster())
				{
					hullPoints = newHull;
					if(objClass == OC_PLATFORM)
					{
						OBJPTR<IJumpPlat> plat;
						static_cast<IBaseObject *>(this)->QueryInterface(IJumpPlatID,plat);
						if(plat)
						{
							plat->SetSiblingHull(newHull);
						}
					}
					
				}
				//			hullPointsAdded = percent*hullPointsMax;
				hullPointsAdded += newHull-oldHull;
			}
		}
		else if (!pause)
		{
			pause=true;
			if (buildEffectObj)
				buildEffectObj->PauseBuildEffect(true);
			if (obj1)
			{
				VOLPTR(IFabricator) fab=obj1;
				fab->FabSetBuildPause(true,buildTime?timeSpent/buildTime:1.0);
			}
		}
	}

	return 1;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base >::physUpdateBuild (SINGLE dt)
{
	if (!bExploding)
	{
		if (buildEffectObj)
			buildEffectObj->PhysicalUpdate(dt);
	
		if(emitterPause)
		{
			if(emitterPause > dt)
				emitterPause -= dt;
			else
				emitterPause = 0;
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base >::failSound(M_RESOURCE_TYPE resType)
{
	if(objClass == OC_PLATFORM)
	{
		switch(resType)
		{
		case M_GAS:
			GENPLATFORMCOMM(notEnoughGas,SUB_NO_GAS);
			break;
		case M_METAL:
			GENPLATFORMCOMM(notEnoughMetal,SUB_NO_METAL);
			break;
		case M_CREW:
			GENPLATFORMCOMM(notEnoughCrew,SUB_NO_CREW);
			break;
		case M_COMMANDPTS:
			GENPLATFORMCOMM(notEnoughCommandPoints, SUB_NO_CP);
			break;
		}
	}
	else
	{
		switch(resType)
		{
		case M_GAS:
			FABRICATORCOMM(notEnoughGas,SUB_NO_GAS);
			break;
		case M_METAL:
			FABRICATORCOMM(notEnoughMetal,SUB_NO_METAL);
			break;
		case M_CREW:
			FABRICATORCOMM(notEnoughCrew,SUB_NO_CREW);
			break;
		case M_COMMANDPTS:
			FABRICATORCOMM(notEnoughCommandPoints, SUB_NO_CP);
			break;
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectBuild< Base >::explodeBuild (bool bExplode)
{
	if (buildEffectObj)
		buildEffectObj->PrepareForExplode();
	EndBuild(true);
}
//---------------------------------------------------------------------------
// we're about to die!!! oh no!
template <class Base>
void ObjectBuild< Base >::preDestructBuild (void)
{
	if(!bReady && fabID && building)
	{
		OBJPTR<IFabricator> fab;
		IBaseObject *obj1;
		obj1 = OBJLIST->FindObject(fabID);
		if(obj1)
		{
			obj1->QueryInterface(IFabricatorID,fab);
		//	whole=1;
			if(fab && fab->GetBuildee() == this)
				fab->FabHaltBuild();
		}
	}
}

#undef START_TIME
#undef ONE_FACE
#undef NEW_CHILD
//---------------------------------------------------------------------------
//---------------------------End TObjBuild.h---------------------------------
//---------------------------------------------------------------------------
#endif