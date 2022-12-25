#ifndef TFABRICATOR_H
#define TFABRICATOR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                            TFabricator.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TFabricator.h 87    10/19/00 6:15p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef RENDERER_H
#include "Renderer.h"
#endif

#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

#ifndef ENGINE_H
#include "Engine.h"
#endif

#ifndef DFABRICATOR_H
#include "DFabricator.h"
#endif

#ifndef DFABSAVE_H
#include "DFabSave.h"
#endif

#ifndef IFABRICATOR_H
#include "IFabricator.h"
#endif

#ifndef _IHARDPOINT_H_
#include <IHardPoint.h>
#endif

#ifndef ARCHHOLDER_H
#include "ArchHolder.h"
#endif

#ifndef DRESEARCH_H
#include "DResearch.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#define NUM_DRONES 12
#define TFabricator _Cf

template <class Base>
struct _NO_VTABLE TFabricator : public Base, IFabricator, IBuildQueue, FAB_SAVELOAD
{
	typename typedef Base::INITINFO FABINITINFO;
	typename typedef Base::SAVEINFO FABSAVEINFO;

	struct UpdateNode       updateNode;
	struct PhysUpdateNode	physUpdateNode;
	struct PostRenderNode	postRenderNode;
	struct InitNode			initNode;
	struct SaveNode			saveNode;
	struct LoadNode			loadNode;
	struct ResolveNode      resolveNode;
	struct UpgradeNode		upgradeNode;

	//----------------------------------
	// animation index
	//----------------------------------
	S32 animIndex,loopingAnimIndex;

	//----------------------------------
	// hardpoint data
	//----------------------------------
	HardpointInfo  hardpointinfo;
	INSTANCE_INDEX bayDoorIndex;
	//should these be OBJPTRS?  I think not.
	//OBJPTR<IBuildShip,false> drone[NUM_DRONES];
	IBuildShip * drone[NUM_DRONES];
	U8 numDrones;

	OBJPTR<IBuild> buildee;
	OBJPTR<IBuild> repairee;
	OBJPTR<IBuild> sellee;
	U32 returnPause;
	SINGLE timePassed,updateTime;

	U32 maxQueueSize;
	U32 nextIndex;

	PARCHETYPE pBuildEffect;
	PARCHETYPE pBuilderType;
	BUILDSHIP_INIT *bs_init;

	//DEBUG VAR ONLY
#if 0
	U32 buildCnt;
#endif

	// sticky visible state
	bool bWasVisible:1;

	//----------------------------------	
	OBJPTR<IBuildEffect> fabBuildEffectObj;

	TFabricator();
	~TFabricator();

	//
	// rememeber: This template is special. load/save functions must be called explicitly.
	//	  i.e. The load/save methods cannot be registered like the others.
	//
	void saveFab(FABSAVEINFO & saveStruct);
	void loadFab(FABSAVEINFO & saveStruct);

	BOOL32 updateFab ();

	void physUpdateFab (SINGLE dt);

	virtual TRANSFORM GetDroneTransform ();

	void initFabricator (const FABINITINFO & data);

	virtual void FabStartBuild (IBuild *newguy);

	virtual void FabStartAssistBuild (IBuild *newguy);

	virtual void FabAssistBuildDone ();

	virtual void FabStopAssistBuild ();

	virtual void FabStartDismantle (IBuild * oldguy);
	
	virtual void FabEndBuild (bool bAborting);

	virtual void FabHaltBuild ();

	virtual void FabEmergencyStop();

	virtual void FabSetBuildPause (bool bPause,SINGLE percent);

	virtual void FabEnableCompletion ();

	virtual IBaseObject * GetBuildee();

	virtual void OpenBayDoors (SINGLE dp);

	virtual void CloseBayDoors (SINGLE delay = 0);

	virtual U32 GetNumDrones ();

	virtual IBaseObject *GetBuildEffectObj ();

	virtual void GetDrone (struct IBuildShip **obj,U8 which);

	virtual U32 GetNumInQueue (U32 value);

	virtual U32 GetFabJobID (void);

	virtual bool IsInQueue (U32 value);

	virtual U32 GetQueue (U32 * queueCopy,U32 * slotIDs = NULL);

	virtual bool IsUpgradeInQueue ();

	virtual void FailSound(M_RESOURCE_TYPE resType);

	virtual void fabPostRender();

	virtual SINGLE FabGetProgress(U32 & stallType);

	virtual SINGLE FabGetDisplayProgress(U32 & stallType);

	virtual void addToQueue(U32 dwAchetypeID);

	virtual void addToQueue(U32 dwAchetypeID, U8 index);

	virtual bool isInQueue(U32 dwArchetype);

	virtual bool buildQueueFull (void);

	virtual bool buildQueueEmpty (void);

	virtual U32 peekFabQueue (void);

	virtual void popFabQueue (void);

	virtual bool queueHasUpgrade();

	virtual U8 getNextQueueIndex();

	virtual U32 findQueueValue(U8 index);

	virtual void removeIndexFromQueue(U8 index);

	virtual U8 getFirstQueueIndex();

	virtual U32 getQueuePositionFromIndex(U8 index);

	void resolveFab (void);

	void upgradeFab (const FABINITINFO & data);

	void fabStartAnims ();

	//virtual BOOL32 fabUpdate();

//	virtual void RecallDrones ();

//	S32 findChild (const char * pathname, INSTANCE_INDEX parent);

//	BOOL32 findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent);
};

template <class Base>
TFabricator< Base >::TFabricator() :
							postRenderNode(this,RenderProc(&TFabricator::fabPostRender)), 
							updateNode(this, UpdateProc(&TFabricator::updateFab)),
							physUpdateNode(this, PhysUpdateProc(&TFabricator::physUpdateFab)),
							initNode(this, CASTINITPROC(&TFabricator::initFabricator)),
							saveNode(this, CASTSAVELOADPROC(&TFabricator::saveFab)),
							loadNode(this, CASTSAVELOADPROC(&TFabricator::loadFab)),
							resolveNode(this, ResolveProc(&TFabricator::resolveFab)),
							upgradeNode(this, UpgradeProc(CASTINITPROC(&TFabricator::upgradeFab)))
{
	bayDoorIndex = animIndex = loopingAnimIndex = -1;
	hardpointinfo.orientation.set_identity();
	hardpointinfo.point.zero();
}
//---------------------------------------------------------------------------
//
template <class Base>
TFabricator< Base >::~TFabricator()
{
	ANIM->release_script_inst(animIndex);
	ANIM->release_script_inst(loopingAnimIndex);
	bayDoorIndex = animIndex = loopingAnimIndex = -1;

	//!!!TODO : Make fabricators release drones here
/*	if (drone[0])
	{
		for (U8 c=0;c<numDrones;c++)
		{
			delete drone[c].ptr;	// objects are not on the object list!
		}
	}*/

	delete fabBuildEffectObj.Ptr();
}
//---------------------------------------------------------------------------
//
template <class Base>
BOOL32 TFabricator< Base >::updateFab()
{
	if (bDoorsOpen)
	{
		if (--doorPause == 0)
		{
			CloseBayDoors();
			if (bReturning)
			{
				bReturning = FALSE;
				bDrones = FALSE;
			}
		}

		if (bReturning)
			if (--returnPause == 0)
			{
			}
	}

	//!!!
/*	if(drone[0])
	{
		for (U8 c=0;c<numDrones;c++)
		{
			drone[c].ptr->Update();
		}
	}*/

	//check if we are falling behind too far on physical update
	if (updateTime-timePassed > ELAPSED_TIME)
	{
		physUpdateFab(ELAPSED_TIME);
	}
	updateTime += ELAPSED_TIME;

	if(!bVisible || !bReady || !bBuilding)
	{
		if (bWasVisible)
		{
			if(bs_init && bs_init->factory && drone[0])
				bs_init->factory->ReleaseShips(drone[0]);
			memset(drone,0,sizeof(void *)*numDrones);
		}

		bWasVisible = false;
	}

	return 1;
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::physUpdateFab(SINGLE dt)
{
	timePassed += dt;

	if (buildee && !buildee->IsPaused())
	{
		if (drone[0])
		{
			for (U8 c=0;c<numDrones;c++)
			{
				drone[c]->PhysicalUpdate(dt);
			}
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
TRANSFORM TFabricator< Base >::GetDroneTransform ()
{
	TRANSFORM trans;
//	trans.set_orientation(hardpointinfo.orientation);
	trans.set_position(hardpointinfo.point);
	trans = transform.multiply(trans);
	return trans;
}
//---------------------------------------------------------------------------
//
template <class Base>
IBaseObject * TFabricator< Base >::GetBuildEffectObj()
{
	return fabBuildEffectObj;
}

template <class Base>
void TFabricator< Base >::fabStartAnims()
{
	if (animIndex != -1)
		ANIM->script_start(animIndex, Animation::FORWARD, Animation::BEGIN);

	if (loopingAnimIndex != -1)
		ANIM->script_start(loopingAnimIndex, Animation::LOOP, Animation::BEGIN);
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::FabStartBuild(IBuild *newguy)
{
#if 0
	CQASSERT(buildCnt==0);
	buildCnt++;
#endif

	/*if (animIndex != -1)
		ANIM->script_start(animIndex, Animation::FORWARD, Animation::BEGIN);

	if (loopingAnimIndex != -1)
		ANIM->script_start(loopingAnimIndex, Animation::LOOP, Animation::BEGIN);*/

	bBuilding = true;
	LocalStartBuild();

	CQASSERT(fabBuildEffectObj==0);
	IBaseObject *obj = ARCHLIST->CreateInstance(pBuildEffect);
	if (obj)
	{
		delete fabBuildEffectObj.Ptr();
		obj->QueryInterface(IBuildEffectID,fabBuildEffectObj,NONSYSVOLATILEPTR);
		CQASSERT(fabBuildEffectObj);
	}

	newguy->QueryInterface(IBuildID,buildee,NONSYSVOLATILEPTR);
	newguy->SetBuilderType(mObjClass);
	newguy->Build(this);
	//pointless?
	/*if(drone[0])
	{
		for (U8 c=0;c<numDrones;c++)
		{
			drone[c]->SetSystemID(systemID);
		}
	}*/
	
	//this is correct for bay doors
//	OpenBayDoors(3);

	//this is correct for cranes
	//RaiseCrane(1/(fractionPerFrame*REALTIME_FRAMERATE));

	bDrones = TRUE;
	bReturning = FALSE;
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::FabStartDismantle (IBuild * oldguy)
{
#if 0
	CQASSERT(buildCnt==0);
	buildCnt++;
#endif

/*
	if (animIndex != -1)
		ANIM->script_start(animIndex, Animation::FORWARD, Animation::BEGIN);

	if (loopingAnimIndex != -1)
		ANIM->script_start(loopingAnimIndex, Animation::LOOP, Animation::BEGIN);
*/

	bBuilding = true;
	LocalStartBuild();

	CQASSERT(fabBuildEffectObj==0);
	IBaseObject *obj = ARCHLIST->CreateInstance(pBuildEffect);
	if (obj)
	{
		delete fabBuildEffectObj.Ptr();
		obj->QueryInterface(IBuildEffectID,fabBuildEffectObj,NONSYSVOLATILEPTR);
		CQASSERT(fabBuildEffectObj);
	}

	oldguy->QueryInterface(IBuildID,buildee,NONSYSVOLATILEPTR);
	oldguy->SetBuilderType(mObjClass);
	oldguy->BeginDismantle(this);
	
	//pointless?
	/*if(drone[0])
	{
		for (U8 c=0;c<numDrones;c++)
		{
			drone[c]->SetSystemID(systemID);
		}
	}*/
	
	//this is correct for bay doors
//	OpenBayDoors(3);

	//this is correct for cranes
	//RaiseCrane(1/(fractionPerFrame*REALTIME_FRAMERATE));

	bDrones = TRUE;
	bReturning = FALSE;
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::FabStartAssistBuild (IBuild *newguy)
{
#if 0
	CQASSERT(buildCnt==0);
	buildCnt++;
#endif

	/*if (animIndex != -1)
		ANIM->script_start(animIndex, Animation::FORWARD, Animation::BEGIN);

	if (loopingAnimIndex != -1)
		ANIM->script_start(loopingAnimIndex, Animation::LOOP, Animation::BEGIN);*/

	bBuilding = true;
	LocalStartBuild();

	newguy->QueryInterface(IBuildID,buildee,NONSYSVOLATILEPTR);
	newguy->AddFabToBuild(this);
	
	//this is correct for bay doors
	//OpenBayDoors(3);

	//this is correct for cranes
	//RaiseCrane(1/(fractionPerFrame*REALTIME_FRAMERATE));

	bDrones = TRUE;
	bReturning = FALSE;
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::FabAssistBuildDone ()
{
#if 0
	CQASSERT(buildCnt==1);
	buildCnt--;
#endif

	bBuilding = false;
	LocalEndBuild();

	if (!buildee)
	{
		OBJLIST->FindObject(buildeeID, NONSYSVOLATILEPTR, buildee, IBuildID);
	}
	CQASSERT(buildee);
	if (buildee)
		buildee->RemoveFabFromBuild(this);

	buildee = 0;
	buildeeID = 0;

	//OpenBayDoors(3);

	returnPause = 90;
	bReturning = TRUE;
	if(drone[0])
	{
		for (U8 c=0;c<numDrones;c++)
		{
			drone[c]->Return();
		}
	}

	if (animIndex != -1)
		ANIM->script_start(animIndex, Animation::BACKWARDS, Animation::END);

	if (loopingAnimIndex != -1)
		ANIM->script_stop(loopingAnimIndex);

//	if(bAborting && buildee.ptr)
//	{
//		OBJPTR<IBaseObject> baseObj;
//		buildee->QueryInterface(IBaseObjectID,baseObj);
//		FabStartDisassembly(baseObj.ptr);
//	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::FabStopAssistBuild ()
{
#if 0
	CQASSERT(buildCnt==1);
	buildCnt--;
#endif

	if (animIndex != -1)
		ANIM->script_start(animIndex, Animation::BACKWARDS, Animation::END);

	if (loopingAnimIndex != -1)
		ANIM->script_stop(loopingAnimIndex);

	bBuilding = false;
	if (!buildee)
	{
		OBJLIST->FindObject(buildeeID, NONSYSVOLATILEPTR, buildee, IBuildID);
	}
	CQASSERT(buildee);
	if (buildee)
		buildee->RemoveFabFromBuild(this);

	buildee = 0;
	buildeeID = 0;

	OpenBayDoors(3);

	returnPause = 90;
	bReturning = TRUE;
	if(drone[0])
	{
		for (U8 c=0;c<numDrones;c++)
		{
			drone[c]->Return();
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::FabEndBuild(bool bAborting)
{
#if 0
	CQASSERT(buildCnt==1);
	buildCnt--;
#endif

	ANIM->script_start(animIndex, Animation::BACKWARDS, Animation::END);

	if (loopingAnimIndex != -1)
		ANIM->script_stop(loopingAnimIndex);

	bBuilding = false;
	LocalEndBuild();

	if (!buildee)
	{
		OBJLIST->FindObject(buildeeID, NONSYSVOLATILEPTR, buildee, IBuildID);
	}
	CQASSERT(buildee);
	if (buildee)
		buildee->EndBuild(bAborting);

	OpenBayDoors(3);

	returnPause = 90;
	bReturning = TRUE;
	if(drone[0])
	{
		for (U8 c=0;c<numDrones;c++)
		{
			drone[c]->Return();
		}
	}

	if(bAborting && buildee.Ptr())
	{
//		FabStartDisassembly(buildee.ptr);
	}

	buildee = 0;
	buildeeID = 0;

	delete fabBuildEffectObj.Ptr();
	fabBuildEffectObj = 0;
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::FabHaltBuild ()
{
#if 0
	CQASSERT(buildCnt==1);
	buildCnt--;
#endif

	if (animIndex != -1)
		ANIM->script_start(animIndex, Animation::BACKWARDS, Animation::END);

	if (loopingAnimIndex != -1)
		ANIM->script_stop(loopingAnimIndex);

	bBuilding = false;
	LocalEndBuild(true);

	if (!buildee)
	{
		OBJLIST->FindObject(buildeeID, NONSYSVOLATILEPTR, buildee, IBuildID);
	}
	bReturning = TRUE;
	if(drone[0])
	{
		for (U8 c=0;c<numDrones;c++)
		{
			drone[c]->Return();
		}
	}
	if (buildee)
		buildee->HaltBuild();

	buildee = 0;
	buildeeID = 0;
	delete fabBuildEffectObj.Ptr();
	fabBuildEffectObj = 0;
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::FabEmergencyStop()
{
	if(bBuilding)
	{
		if (animIndex != -1)
			ANIM->script_start(animIndex, Animation::BACKWARDS, Animation::END);

		if (loopingAnimIndex != -1)
			ANIM->script_stop(loopingAnimIndex);

		bBuilding = false;
		LocalEndBuild(true);

		if (!buildee)
		{
			OBJLIST->FindObject(buildeeID, NONSYSVOLATILEPTR, buildee, IBuildID);
		}
		bReturning = TRUE;
		if(drone[0])
		{
			for (U8 c=0;c<numDrones;c++)
			{
				drone[c]->Return();
			}
		}
		if (buildee)
			buildee->HaltBuild();

		buildee = 0;
		buildeeID = 0;
		delete fabBuildEffectObj.Ptr();
		fabBuildEffectObj = 0;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::FabSetBuildPause (bool bPause,SINGLE percent)
{
	if (bPause)
	{
		if (loopingAnimIndex != -1)
			ANIM->script_stop(loopingAnimIndex);
	}
	else
	{
		if (loopingAnimIndex != -1)
			ANIM->script_start(loopingAnimIndex, Animation::LOOP, Animation::BEGIN);
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::FabEnableCompletion ()
{
	if (!buildee)
	{
		OBJLIST->FindObject(buildeeID, NONSYSVOLATILEPTR, buildee, IBuildID);
	}
	if (buildee)
		buildee->EnableCompletion();
}
//---------------------------------------------------------------------------
//
template <class Base>
IBaseObject * TFabricator< Base >::GetBuildee()
{
	return buildee.Ptr();
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::OpenBayDoors(SINGLE dp)
{
/*	U32 duration = dp*REALTIME_FRAMERATE;
	if (doorPause < duration)
		doorPause = duration;
	if (animIndex != -1 && !bDoorsOpen)
	{
		ANIM->script_start(animIndex, Animation::FORWARD, Animation::BEGIN);
	}
	bDoorsOpen = TRUE;*/
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::CloseBayDoors(SINGLE delay)
{
	/*if (animIndex != -1 && bDoorsOpen)
	{
		if (delay == 0)
		{
			ANIM->script_start(animIndex, Animation::BACKWARDS, Animation::END);
			bDoorsOpen = FALSE;
		}
		else
			doorPause = delay*REALTIME_FRAMERATE;
	}else
	{
		bDoorsOpen = FALSE;
	}*/
}
//---------------------------------------------------------------------------
//
template <class Base>
U32 TFabricator< Base >::GetNumDrones ()
{
	return numDrones;
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::GetDrone (struct IBuildShip **obj,U8 which)
{
	*obj = NULL;

	if (which < numDrones)
	{
		*obj = drone[which];
	}
}

template <class Base>
U32 TFabricator< Base >::GetNumInQueue (U32 value)
{
	U32 retVal = 0;
	for(U32 index = 0; index < queueSize; ++index)
	{
		if(buildQueue[(queueStart+index)%maxQueueSize] == value)
			++retVal;
	}
	return retVal;
}

template <class Base>
U32 TFabricator< Base >::GetFabJobID (void)
{
	if(queueSize)
	{
		return buildQueue[queueStart];
	}
	return 0;
}

template <class Base>
bool TFabricator< Base >::IsInQueue (U32 value)
{
	return isInQueue(value);
}

template <class Base>
U32 TFabricator< Base >::GetQueue (U32 * queueCopy,U32 * slotIDs)
{
	for(U32 index = 0; index < queueSize; ++index)
	{
		queueCopy[index] = buildQueue[(queueStart+index)%maxQueueSize];
		if(slotIDs)
			slotIDs[index] = buildQueueIndex[(queueStart+index)%maxQueueSize];
	}
	return queueSize;
}

template <class Base>
bool TFabricator< Base >::IsUpgradeInQueue ()
{
	return queueHasUpgrade();
}

template <class Base>
void TFabricator< Base >::FailSound(M_RESOURCE_TYPE resType)
{
	failSound(resType);
}
/*template <class Base>
void TFabricator< Base >::RecallDrones()
{
	if (drone[0])
	{
		for (U8 c=0;c<NUM_DRONES;c++)
		{
			drone[c]->Return();
		}
	}
}*/
//---------------------------------------------------------------------------
//
/*template <class Base>
S32 TFabricator< Base >::findChild (const char * pathname, INSTANCE_INDEX parent)
{
	S32 index = -1;
	char buffer[MAX_PATH];
	const char *ptr=pathname, *ptr2;
	INSTANCE_INDEX child=-1;

	if (ptr[0] == '\\')
		ptr++;

	if ((ptr2 = strchr(ptr, '\\')) == 0)
	{
		strcpy(buffer, ptr);
	}
	else
	{
		memcpy(buffer, ptr, ptr2-ptr);
		buffer[ptr2-ptr] = 0;		// null terminating
	}

	while ((child = MODEL->get_child(parent, child)) != -1)
	{
		if (MODEL->is_named(child, buffer))
		{
			if (ptr2)
			{
				// found the child, go deeper if needed
				parent = child;
				child = -1;
				ptr = ptr2+1;
				if ((ptr2 = strchr(ptr, '\\')) == 0)
				{
					strcpy(buffer, ptr);
				}
				else
				{
					memcpy(buffer, ptr, ptr2-ptr);
					buffer[ptr2-ptr] = 0;		// null terminating
				}
			}
			else
			{
				index = child;
				break;
			}
		}
	}

	return index;
}
//----------------------------------------------------------------------------------------
//
template <class Base>
BOOL32 TFabricator< Base >::findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent)
{
	BOOL32 result=0;
	char buffer[MAX_PATH];
	char *ptr=buffer;

	strcpy(buffer, pathname);
	if ((ptr = strrchr(ptr, '\\')) == 0)
		goto Done;

	*ptr++ = 0;
	if (buffer[0])
		index = findChild(buffer, parent);
	else
		index = parent;

Done:	
	if (index != -1)
	{
		HARCH arch = index;

		if ((result = HARDPOINT->retrieve_hardpoint_info(arch, ptr, hardpointinfo)) == false)
			index = -1;		// invalidate result
	}

	return result;
}*/
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::initFabricator (const FABINITINFO & _data)
{

	maxQueueSize = __max(_data.pData->maxQueueSize,FAB_MAX_QUEUE_SIZE);
/*	for(int i = 0; i < NUM_DRONES; ++i)
	{
		if(_data.pBuilderType[i])
		{
			++numDrones;
			IBaseObject *obj = ARCHLIST->CreateInstance(_data.pBuilderType[i]);
			CQASSERT(obj);
			if (obj)
			{
				obj->QueryInterface(IBuildShipID,drone[numDrones-1]);
				CQASSERT(drone[numDrones-1] != 0);
				drone[numDrones-1]->InitBuildShip(this);
			}
		}

	}*/


	bBuilding = false;
	pBuildEffect = _data.pBuildEffect;
	pBuilderType = _data.builderInfo[0].pBuilderType;
	if (pBuilderType)
	{
		bs_init = static_cast<BUILDSHIP_INIT *>(ARCHLIST->GetArchetypeHandle(pBuilderType));
		numDrones = _data.builderInfo[0].numDrones;
	}

	animIndex = ANIM->create_script_inst(_data.animArchetype, instanceIndex, "Sc_build");
	loopingAnimIndex = ANIM->create_script_inst(_data.animArchetype, instanceIndex, "Sc_construct");
}

//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::loadFab(FABSAVEINFO & saveStruct)
{
	FABSAVEINFO * loadInfo = (FABSAVEINFO *) (&saveStruct);

	//this isn't even used!
//	Transform startTrans = GetDroneTransform();

	/*for (int c=0;c<numDrones;c++)
	{
		if(drone[c])
		{
			//what could this possibly be good for?
		//	drone[c]->SetSystemID(systemID);

		// !!!!!!!!!!!drone position will have to be set through some other mechanism
		//	drone[c]->SetTransform(loadInfo->fab_SL.droneTrans[c]);

		}
	}*/

	*static_cast<FAB_SAVELOAD *> (this) = loadInfo->fab_SL;

	//if (loadInfo->fab_SL.bDoorsOpen)
	//	ANIM->script_start(animIndex, Animation::FORWARD, Animation::END);

	//this wouldn't seem to be necessary as nothing is building in this case
	/*for (c=0;c<numDrones;c++)
	{
		if (bReturning)
		{
			if(drone[c])
				drone[c]->Return();
		}
		else if(!bDrones)
		{
			if(drone[c])
				drone[c]->Return();
		}
	}*/
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::saveFab(FABSAVEINFO & saveStruct)
{
	FABSAVEINFO * saveInfo = (FABSAVEINFO *) (&saveStruct);

	saveInfo->fab_SL = *static_cast<FAB_SAVELOAD *> (this);
/*	for (U8 c=0;c<3;c++)
	{
		if(drone[c])
			saveInfo->fab_SL.droneTrans[c] = drone[c].ptr->GetTransform();
	}*/
	if (buildee)
		saveInfo->fab_SL.buildeeID = buildee.Ptr()->GetPartID();
	if (repairee)
		saveInfo->fab_SL.repaireeID = repairee.Ptr()->GetPartID();
	if (sellee)
		saveInfo->fab_SL.selleeID = sellee.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::fabPostRender()
{
	if (bBuilding)
	{
		if (bWasVisible == false)
		{
			if (numDrones)
			{
				if(bs_init->factory)
					bs_init->factory->GetBuilderShips(drone,numDrones,pBuilderType,this);
				if (fabBuildEffectObj)
					fabBuildEffectObj->SynchBuilderShips();
			}
		}
		
		bWasVisible = true;
		
		for (U8 c=0;c<numDrones;c++)
		{
			if(drone[c])
				drone[c]->Render();
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
SINGLE TFabricator< Base >::FabGetProgress(U32 & stallType)
{
	if (buildee == 0)
		return 0;
	
	return buildee->GetBuildProgress(stallType);
}
//---------------------------------------------------------------------------
//
template <class Base>
SINGLE TFabricator< Base >::FabGetDisplayProgress(U32 & stallType)
{
	if (buildee == 0)
		return 0;
	
	return buildee->GetBuildDisplayProgress(stallType);
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::addToQueue(U32 dwAchetypeID)
{
	if(queueSize != maxQueueSize)
	{
		buildQueue[(queueStart+queueSize)%maxQueueSize] = dwAchetypeID;
		lastIndex = getNextQueueIndex();
		buildQueueIndex[(queueStart+queueSize)%maxQueueSize] = lastIndex;
		++queueSize;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::addToQueue(U32 dwAchetypeID, U8 index)
{
	if(queueSize != maxQueueSize)
	{
		buildQueue[(queueStart+queueSize)%maxQueueSize] = dwAchetypeID;
		lastIndex = index;
		buildQueueIndex[(queueStart+queueSize)%maxQueueSize] = index;
		++queueSize;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::removeIndexFromQueue(U8 index)
{
	CQASSERT(queueSize > 0);
	U32 position = 0;
	while(position < queueSize)
	{
		if(buildQueueIndex[(queueStart+position)%maxQueueSize] == index)
		{
			while(position < queueSize)
			{
				buildQueue[(queueStart+position)%maxQueueSize] = buildQueue[(queueStart+position+1)%maxQueueSize];
				++position;
			}
			--queueSize;
			return;
		}
		++position;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
U8 TFabricator< Base >::getNextQueueIndex()
{
	if(queueSize)
	{
		U8 index = lastIndex+1;
		while(findQueueValue(index) != -1)
		{
			++index;
		}
		return index;
	}
	return 0;
}
//---------------------------------------------------------------------------
//
template <class Base>
U32 TFabricator< Base >::findQueueValue(U8 index)
{
	for(U32 position = 0; position < queueSize; ++position)
	{
		if(buildQueueIndex[(queueStart+position)%maxQueueSize] == index)
			return buildQueue[(queueStart+position)%maxQueueSize];
	}
	return -1;
}
//---------------------------------------------------------------------------
//
template <class Base>
U8 TFabricator< Base >::getFirstQueueIndex()
{
	if(queueSize)
	{
		return buildQueueIndex[(queueStart)%maxQueueSize];		
	}
	return 0;
}
//---------------------------------------------------------------------------
//
template <class Base>
U32 TFabricator< Base >::getQueuePositionFromIndex(U8 index)
{
	for(U32 position = 0; position < queueSize; ++position)
	{
		if(buildQueueIndex[(queueStart+position)%maxQueueSize] == index)
			return position;
	}
	return -1;
}
//---------------------------------------------------------------------------
//
template <class Base>
bool TFabricator< Base >::isInQueue(U32 dwArchetypeID)
{
	for(U32 position = 0; position < queueSize; ++position)
	{
		if(buildQueue[(queueStart+position)%maxQueueSize] == dwArchetypeID)
			return true;
	}
	return false;
}
//---------------------------------------------------------------------------
template <class Base>
bool TFabricator< Base >::buildQueueEmpty (void)
{
	return (queueSize == 0);
}
//---------------------------------------------------------------------------
//
template <class Base>
bool TFabricator< Base >::buildQueueFull (void)
{
	return (queueSize == maxQueueSize);
}
//---------------------------------------------------------------------------
//
template <class Base>
U32 TFabricator< Base >:: peekFabQueue (void)
{
	CQASSERT(queueSize);
	return buildQueue[queueStart];
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::popFabQueue (void)
{

	CQASSERT(queueSize);
	queueStart = (queueStart+1)%maxQueueSize;
	CQASSERT(queueSize > 0);
	--queueSize;
}
//---------------------------------------------------------------------------
//
template <class Base>
bool TFabricator< Base >::queueHasUpgrade()
{
	if(queueSize)
	{
		for(S32 index = queueSize-1; index >= 0; --index)
		{
			BASIC_DATA * data = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(buildQueue[(queueStart+index)%maxQueueSize]));
			if(data->objClass == OC_RESEARCH)
			{
				BASE_RESEARCH_DATA * resData = (BASE_RESEARCH_DATA *) data;
				if(resData->type == RESEARCH_UPGRADE)
					return true;
			}
		}
	}
	return false;
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::resolveFab (void)
{
	if(buildeeID)
	{
		IBaseObject *obj = ARCHLIST->CreateInstance(pBuildEffect);
		if (obj)
		{
			obj->QueryInterface(IBuildEffectID,fabBuildEffectObj,NONSYSVOLATILEPTR);
			CQASSERT(fabBuildEffectObj);
		}
		
		OBJLIST->FindObject(buildeeID, NONSYSVOLATILEPTR, buildee, IBuildID);
		if (buildee==0)
			CQBOMB1("Can't Find Object 0x%X",buildeeID);
		CQASSERT(buildee);
		buildee->AddFabToBuild(this);

		if (animIndex != -1)
			ANIM->script_start(animIndex, Animation::FORWARD, Animation::END);
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void TFabricator< Base >::upgradeFab (const FABINITINFO & _data)
{

	maxQueueSize = __max(_data.pData->maxQueueSize,FAB_MAX_QUEUE_SIZE);

	//!!!TODO - upgrade drones with platform
/*	for(U32 i = 0; i < NUM_DRONES; ++i)
	{
		if(drone[i])
		{
			delete drone[i].ptr;
			drone[i] = NULL;
		}
	}
	numDrones = 0;
	for(i = 0; i < NUM_DRONES; ++i)
	{
		if(_data.pBuilderType[i])
		{
			++numDrones;
			IBaseObject *obj = ARCHLIST->CreateInstance(_data.pBuilderType[i]);
			CQASSERT(obj);
			if (obj)
			{
				obj->QueryInterface(IBuildShipID,drone[numDrones-1]);
				CQASSERT(drone[numDrones-1] != 0);
				drone[numDrones-1]->InitBuildShip(this);
			}
		}
	}*/
	pBuildEffect = _data.pBuildEffect;
}
//---------------------------------------------------------------------------
//-------------------------End TFabricator.h---------------------------------
//---------------------------------------------------------------------------
#endif