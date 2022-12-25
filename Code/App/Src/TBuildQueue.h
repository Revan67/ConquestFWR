#ifndef TBUILDQUEUE_H
#define TBUILDQUEUE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                            TBuildQueue.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 2004 Warthog, INC.                        //
//                                                                          //
//--------------------------------------------------------------------------//
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
#define TBuildQueue _Cq

template <class Base>
struct _NO_VTABLE TBuildQueue : public Base, IBuildQueue, BUILDQUEUE_SAVELOAD
{
	typename typedef Base::INITINFO BQINITINFO;
	typename typedef Base::SAVEINFO BQSAVEINFO;

	struct InitNode			initNode;
	struct SaveNode			saveNode;
	struct LoadNode			loadNode;
	struct ResolveNode      resolveNode;

	U32 maxQueueSize;
	U32 nextIndex;

	TBuildQueue();
	~TBuildQueue();

	//
	// rememeber: This template is special. load/save functions must be called explicitly.
	//	  i.e. The load/save methods cannot be registered like the others.
	//
	void saveFab(BQSAVEINFO & saveStruct);
	void loadFab(BQSAVEINFO & saveStruct);
	void initBuildQueue (const BQINITINFO & _data);

	virtual U32 GetNumInQueue (U32 value);

	virtual U32 GetFabJobID (void);

	virtual bool IsInQueue (U32 value);

	virtual U32 GetQueue (U32 * queueCopy,U32 * slotIDs = NULL);

	virtual bool IsUpgradeInQueue ();

	virtual void FailSound(M_RESOURCE_TYPE resType);

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

	U32 getQueueSize();
};

template <class Base>
TBuildQueue< Base >::TBuildQueue() :
							initNode(this, CASTINITPROC(&TBuildQueue::initBuildQueue)),
							saveNode(this, CASTSAVELOADPROC(&TBuildQueue::saveFab)),
							loadNode(this, CASTSAVELOADPROC(&TBuildQueue::loadFab)),
							resolveNode(this, ResolveProc(&TBuildQueue::resolveFab))
{
}
//---------------------------------------------------------------------------
//
template <class Base>
TBuildQueue< Base >::~TBuildQueue()
{
}
template <class Base>
U32 TBuildQueue< Base >::GetNumInQueue (U32 value)
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
U32 TBuildQueue< Base >::GetFabJobID (void)
{
	if(queueSize)
	{
		return buildQueue[queueStart];
	}
	return 0;
}

template <class Base>
bool TBuildQueue< Base >::IsInQueue (U32 value)
{
	return isInQueue(value);
}

template <class Base>
U32 TBuildQueue< Base >::GetQueue (U32 * queueCopy,U32 * slotIDs)
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
bool TBuildQueue< Base >::IsUpgradeInQueue ()
{
	return queueHasUpgrade();
}

template <class Base>
void TBuildQueue< Base >::FailSound(M_RESOURCE_TYPE resType)
{
	failSound(resType);
}
//---------------------------------------------------------------------------
//
template <class Base>
void TBuildQueue< Base >::initBuildQueue (const BQINITINFO & _data)
{
	maxQueueSize = __max(_data.pData->maxQueueSize,FAB_MAX_QUEUE_SIZE);
}

//---------------------------------------------------------------------------
//
template <class Base>
void TBuildQueue< Base >::loadFab(BQSAVEINFO & saveStruct)
{
	BQSAVEINFO * loadInfo = (BQSAVEINFO *) (&saveStruct);
	*static_cast<BUILDQUEUE_SAVELOAD *> (this) = loadInfo->fab_SL;
}
//---------------------------------------------------------------------------
//
template <class Base>
void TBuildQueue< Base >::saveFab(BQSAVEINFO & saveStruct)
{
	BQSAVEINFO * saveInfo = (BQSAVEINFO *) (&saveStruct);

	saveInfo->fab_SL = *static_cast<BUILDQUEUE_SAVELOAD *> (this);
}
//---------------------------------------------------------------------------
//
template <class Base>
SINGLE TBuildQueue< Base >::FabGetProgress(U32 & stallType)
{
	return 0;
}
//---------------------------------------------------------------------------
//
template <class Base>
SINGLE TBuildQueue< Base >::FabGetDisplayProgress(U32 & stallType)
{
	return 0;
}
//---------------------------------------------------------------------------
//
template <class Base>
void TBuildQueue< Base >::addToQueue(U32 dwAchetypeID)
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
void TBuildQueue< Base >::addToQueue(U32 dwAchetypeID, U8 index)
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
void TBuildQueue< Base >::removeIndexFromQueue(U8 index)
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
U8 TBuildQueue< Base >::getNextQueueIndex()
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
U32 TBuildQueue< Base >::findQueueValue(U8 index)
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
U8 TBuildQueue< Base >::getFirstQueueIndex()
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
U32 TBuildQueue< Base >::getQueuePositionFromIndex(U8 index)
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
bool TBuildQueue< Base >::isInQueue(U32 dwArchetypeID)
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
bool TBuildQueue< Base >::buildQueueEmpty (void)
{
	return (queueSize == 0);
}
//---------------------------------------------------------------------------
//
template <class Base>
bool TBuildQueue< Base >::buildQueueFull (void)
{
	return (queueSize == maxQueueSize);
}
//---------------------------------------------------------------------------
//
template <class Base>
U32 TBuildQueue< Base >:: peekFabQueue (void)
{
	CQASSERT(queueSize);
	return buildQueue[queueStart];
}
//---------------------------------------------------------------------------
//
template <class Base>
void TBuildQueue< Base >::popFabQueue (void)
{

	CQASSERT(queueSize);
	queueStart = (queueStart+1)%maxQueueSize;
	CQASSERT(queueSize > 0);
	--queueSize;
}
//---------------------------------------------------------------------------
//
template <class Base>
bool TBuildQueue< Base >::queueHasUpgrade()
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
void TBuildQueue< Base >::resolveFab (void)
{
}
//---------------------------------------------------------------------------
//
template <class Base>
U32 TBuildQueue< Base >::getQueueSize()
{
	return queueSize;
}
//---------------------------------------------------------------------------
//-------------------------End TBuildQueue.h---------------------------------
//---------------------------------------------------------------------------
#endif