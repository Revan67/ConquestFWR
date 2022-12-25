#ifndef TOBJFRAME_H
#define TOBJFRAME_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               TObjFrame.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjFrame.h 14    9/19/00 9:02p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef CQTRACE_H
#include "CQTrace.h"
#endif

#define ObjectFrame _Cof

//--------------------------------------------------------------------------//
//
template <class Base, class SaveStruct, class InitStruct>
struct _NO_VTABLE ObjectFrame : public Base
{
	typedef SaveStruct SAVEINFO;
	typedef InitStruct INITINFO;
	typedef void   (IBaseObject::*RenderProc) (void);
	typedef BOOL32 (IBaseObject::*UpdateProc) (void);
	typedef void   (IBaseObject::*PhysUpdateProc) (SINGLE dt);
	typedef void   (IBaseObject::*SaveLoadProc) (SAVEINFO & saveStruct);
	typedef void   (IBaseObject::*ResolveProc) (void);
	typedef void   (IBaseObject::*ExplodeProc) (bool bExplode);
	typedef void   (IBaseObject::*InitProc) (const INITINFO & initStruct);
	typedef void   (IBaseObject::*PreDestructProc) (void);
	typedef void   (IBaseObject::*OnOpCancelProc) (U32 agentID);
	typedef void   (IBaseObject::*PreTakeoverProc) (U32 newMissionID, U32 troopID);
	typedef void   (IBaseObject::*ReceiveOpDataProc) (U32 agentID, void *buffer, U32 bufferSize);
	typedef void   (IBaseObject::*UpgradeProc) (const INITINFO & initStruct);
	typedef U32    (IBaseObject::*SyncGetProc) (void * buffer);
	typedef void   (IBaseObject::*SyncPutProc) (void * buffer, U32 bufferSize, bool bLateDelivery);

	struct PreRenderNode
	{
		PreRenderNode * pNext;
		RenderProc   rendProc;

		PreRenderNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, RenderProc _proc)
		{
			rendProc = _proc;
			inst->registerPreRender(this);
		}
	};

	struct RenderNode
	{
		RenderNode * pNext;
		RenderProc   rendProc;

		RenderNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, RenderProc _proc)
		{
			rendProc = _proc;
			inst->registerRender(this);
		}
	};

	struct PostRenderNode
	{
		PostRenderNode * pNext;
		RenderProc		 rendProc;

		PostRenderNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, RenderProc _proc)
		{
			rendProc = _proc;
			inst->registerPostRender(this);
		}
	};

	struct UpdateNode
	{
		UpdateNode * pNext;
		UpdateProc   updateProc;

		UpdateNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, UpdateProc _proc)
		{
			updateProc = _proc;
			inst->registerUpdate(this);
		}
	};

	struct PhysUpdateNode
	{
		PhysUpdateNode * pNext;
		PhysUpdateProc   updateProc;

		PhysUpdateNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, PhysUpdateProc _proc)
		{
			updateProc = _proc;
			inst->registerPhysUpdate(this);
		}
	};

	struct SaveNode
	{
		SaveNode *		pNext;
		SaveLoadProc	saveProc;

		SaveNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, SaveLoadProc _proc)
		{
			saveProc = _proc;
			inst->registerSave(this);
		}
	};

	struct ResolveNode
	{
		ResolveNode * pNext;
		ResolveProc	 resolveProc;

		ResolveNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, ResolveProc _proc)
		{
			resolveProc = _proc;
			inst->registerResolve(this);
		}
	};
	
	struct LoadNode
	{
		LoadNode *		pNext;
		SaveLoadProc	loadProc;

		LoadNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, SaveLoadProc _proc)
		{
			loadProc = _proc;
			inst->registerLoad(this);
		}
	};

	struct ExplodeNode
	{
		ExplodeNode * 	pNext;
		ExplodeProc		explodeProc;

		ExplodeNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, ExplodeProc _proc)
		{
			explodeProc = _proc;
			inst->registerExplode(this);
		}
	};

	struct InitNode
	{
		InitNode * pNext;
		InitProc   initProc;

		InitNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, InitProc _proc)
		{
			initProc = _proc;
			inst->registerInit(this);
		}
	};

	struct PreDestructNode
	{
		PreDestructNode * pNext;
		PreDestructProc   destructProc;

		PreDestructNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, PreDestructProc _proc)
		{
			destructProc = _proc;
			inst->registerPreDestruct(this);
		}
	};

	struct OnOpCancelNode
	{
		OnOpCancelNode * pNext;
		OnOpCancelProc   onOpCancelProc;

		OnOpCancelNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, OnOpCancelProc _proc)
		{
			onOpCancelProc = _proc;
			inst->registerOnOpCancel(this);
		}
	};

	struct PreTakeoverNode
	{
		PreTakeoverNode * pNext;
		PreTakeoverProc   preTakeoverProc;

		PreTakeoverNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, PreTakeoverProc _proc)
		{
			preTakeoverProc = _proc;
			inst->registerPreTakeover(this);
		}
	};

	struct ReceiveOpDataNode
	{
		ReceiveOpDataNode * pNext;
		ReceiveOpDataProc   receiveOpDataProc;

		ReceiveOpDataNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, ReceiveOpDataProc _proc)
		{
			receiveOpDataProc = _proc;
			inst->registerReceiveOpData(this);
		}
	};

	struct UpgradeNode
	{
		UpgradeNode * pNext;
		UpgradeProc   upgradeProc;

		UpgradeNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, UpgradeProc _proc)
		{
			upgradeProc = _proc;
			inst->registerUpgrade(this);
		}
	};

	struct PrioritySyncNode
	{
		PrioritySyncNode * pNext;
		SyncGetProc   getProc;
		SyncPutProc   putProc;
		U8			  count;		// what number am I in the chain?

		PrioritySyncNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, SyncGetProc _getProc, SyncPutProc _putProc)
		{
			getProc = _getProc;
			putProc = _putProc;
			inst->registerPrioritySync(this);
		}
	};

	struct GeneralSyncNode
	{
		GeneralSyncNode * pNext;
		SyncGetProc   getProc;
		SyncPutProc   putProc;
		U8			  count;		// what number am I in the chain?

		GeneralSyncNode (ObjectFrame<Base,SaveStruct,InitStruct> * inst, SyncGetProc _getProc, SyncPutProc _putProc)
		{
			getProc = _getProc;
			putProc = _putProc;
			inst->registerGeneralSync(this);
		}
	};

private:
	PreRenderNode *  preRenderList, *preRenderListEnd;
	RenderNode *	 renderList, *renderListEnd;
	PostRenderNode * postRenderList, *postRenderListEnd;
	UpdateNode *	 updateList, *updateListEnd;
	PhysUpdateNode * physUpdateList, *physUpdateListEnd;
	SaveNode *		 saveList, *saveListEnd;
	LoadNode *		 loadList, *loadListEnd;
	ResolveNode *	 resolveList, *resolveListEnd;
	ExplodeNode *	 explodeList, *explodeListEnd;
	InitNode *		 initList, *initListEnd;
	PreDestructNode *preDestructList, *preDestructListEnd;   
	OnOpCancelNode *onOpCancelList, *onOpCancelListEnd;   
	PreTakeoverNode *preTakeoverList, *preTakeoverListEnd;   
	ReceiveOpDataNode *receiveOpDataList, *receiveOpDataListEnd;   
	UpgradeNode *	 upgradeList, *upgradeListEnd;
	PrioritySyncNode *	 prioritySyncList, *prioritySyncListEnd;
	GeneralSyncNode *	 generalSyncList, *generalSyncListEnd;

public:

	const InitStruct * frameInitInfo;//pointer to the initialization structure for this class

	ObjectFrame (void)
	{
	}

	~ObjectFrame (void)
	{
	}

	//----------------------------------
	//
	void registerPreRender (PreRenderNode *pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=preRenderListEnd);		// can only register once
		if (preRenderListEnd == 0)
			preRenderList = pNode;
		else
			preRenderListEnd->pNext = pNode;
		preRenderListEnd = pNode;
	}
	//----------------------------------
	//
	void registerRender (RenderNode *pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=renderListEnd);		// can only register once
		if (renderListEnd == 0)
			renderList = pNode;
		else
			renderListEnd->pNext = pNode;
		renderListEnd = pNode;
	}
	//----------------------------------
	//
	void registerPostRender (PostRenderNode *pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=postRenderListEnd);		// can only register once
		if (postRenderListEnd == 0)
			postRenderList = pNode;
		else
			postRenderListEnd->pNext = pNode;
		postRenderListEnd = pNode;
	}
	//----------------------------------
	//
	void registerUpdate (UpdateNode *pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=updateListEnd);		// can only register once
		if (updateListEnd == 0)
			updateList = pNode;
		else
			updateListEnd->pNext = pNode;
		updateListEnd = pNode;
	}
	//----------------------------------
	//
	void registerPhysUpdate (PhysUpdateNode *pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=physUpdateListEnd);		// can only register once
		if (physUpdateListEnd == 0)
			physUpdateList = pNode;
		else
			physUpdateListEnd->pNext = pNode;
		physUpdateListEnd = pNode;
	}
	//----------------------------------
	//
	void registerSave (SaveNode * pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=saveListEnd);		// can only register once
		if (saveListEnd == 0)
			saveList = pNode;
		else
			saveListEnd->pNext = pNode;
		saveListEnd = pNode;
	}
	//----------------------------------
	//
	void registerLoad (LoadNode * pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=loadListEnd);		// can only register once
		if (loadListEnd == 0)
			loadList = pNode;
		else
			loadListEnd->pNext = pNode;
		loadListEnd = pNode;
	}
	//----------------------------------
	//
	void registerResolve (ResolveNode *pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=resolveListEnd);		// can only register once
		if (resolveListEnd == 0)
			resolveList = pNode;
		else
			resolveListEnd->pNext = pNode;
		resolveListEnd = pNode;
	}
	//----------------------------------
	//
	void registerExplode (ExplodeNode * pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=explodeListEnd);		// can only register once
		if (explodeListEnd == 0)
			explodeList = pNode;
		else
			explodeListEnd->pNext = pNode;
		explodeListEnd = pNode;
	}
	//----------------------------------
	//
	void registerInit (InitNode * pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=initListEnd);		// can only register once
		if (initListEnd == 0)
			initList = pNode;
		else
			initListEnd->pNext = pNode;
		initListEnd = pNode;
	}
	//----------------------------------
	//
	void registerPreDestruct (PreDestructNode * pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=preDestructListEnd);		// can only register once
		if (preDestructListEnd == 0)
			preDestructList = pNode;
		else
			preDestructListEnd->pNext = pNode;
		preDestructListEnd = pNode;
	}
	//----------------------------------
	//
	void registerOnOpCancel (OnOpCancelNode * pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=onOpCancelListEnd);		// can only register once
		if (onOpCancelListEnd == 0)
			onOpCancelList = pNode;
		else
			onOpCancelListEnd->pNext = pNode;
		onOpCancelListEnd = pNode;
	}
	//----------------------------------
	//
	void registerPreTakeover (PreTakeoverNode * pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=preTakeoverListEnd);		// can only register once
		if (preTakeoverListEnd == 0)
			preTakeoverList = pNode;
		else
			preTakeoverListEnd->pNext = pNode;
		preTakeoverListEnd = pNode;
	}
	//----------------------------------
	//
	void registerReceiveOpData (ReceiveOpDataNode * pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=receiveOpDataListEnd);		// can only register once
		if (receiveOpDataListEnd == 0)
			receiveOpDataList = pNode;
		else
			receiveOpDataListEnd->pNext = pNode;
		receiveOpDataListEnd = pNode;
	}
	//----------------------------------
	//
	void registerUpgrade (UpgradeNode * pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=upgradeListEnd);		// can only register once
		if (upgradeListEnd == 0)
			upgradeList = pNode;
		else
			upgradeListEnd->pNext = pNode;
		upgradeListEnd = pNode;
	}
	//----------------------------------
	//
	void registerPrioritySync (PrioritySyncNode * pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=prioritySyncListEnd);		// can only register once
		if (prioritySyncListEnd == 0)
		{
			prioritySyncList = pNode;
			pNode->count = 0;
		}
		else
		{
			prioritySyncListEnd->pNext = pNode;
			pNode->count = prioritySyncListEnd->count + 1;
			CQASSERT(pNode->count < 8);		// fit in 3 bits please (jy)
		}
		prioritySyncListEnd = pNode;
	}
	//----------------------------------
	//
	void registerGeneralSync (GeneralSyncNode * pNode)
	{
		CQASSERT(pNode->pNext==0 && pNode!=generalSyncListEnd);		// can only register once
		if (generalSyncListEnd == 0)
		{
			generalSyncList = pNode;
			pNode->count = 0;
		}
		else
		{
			generalSyncListEnd->pNext = pNode;
			pNode->count = generalSyncListEnd->count + 1;
			CQASSERT(pNode->count < 8);		// fit in 3 bits please (jy)
		}
		generalSyncListEnd = pNode;
	}
	//----------------------------------
	//
	void FRAME_preRender (void)
	{
		PreRenderNode * pNode = preRenderList;

		while (pNode)
		{
			(this->*(pNode->rendProc))();
			pNode = pNode->pNext;
		}
	}
	//----------------------------------
	//
	void FRAME_render (void)
	{
		RenderNode * pNode = renderList;

		while (pNode)
		{
			(this->*(pNode->rendProc))();
			pNode = pNode->pNext;
		}
	}
	//----------------------------------
	//
	void FRAME_postRender (void)
	{
		PostRenderNode * pNode = postRenderList;

		while (pNode)
		{
			(this->*(pNode->rendProc))();
			pNode = pNode->pNext;
		}
	}
	//----------------------------------
	//
	BOOL32 FRAME_update (void)
	{
		UpdateNode * pNode = updateList;
		BOOL32 result=1;

		while (pNode)
		{
		 	result &= (this->*(pNode->updateProc))();
			pNode = pNode->pNext;
		}

		return result;
	}
	//----------------------------------
	//
	void FRAME_physicalUpdate (SINGLE dt)
	{
		PhysUpdateNode * pNode = physUpdateList;

		while (pNode)
		{
		 	(this->*(pNode->updateProc))(dt);
			pNode = pNode->pNext;
		}
	}
	//----------------------------------
	//
	void FRAME_save (SAVEINFO & saveStruct)
	{
		SaveNode * pNode = saveList;

		while (pNode)
		{
			(this->*(pNode->saveProc))(saveStruct);
			pNode = pNode->pNext;
		}
	}
	//----------------------------------
	//
	void FRAME_load (SAVEINFO & saveStruct)
	{
		LoadNode * pNode = loadList;

		while (pNode)
		{
			(this->*(pNode->loadProc))(saveStruct);
			pNode = pNode->pNext;
		}
	}
	//----------------------------------
	//
	void FRAME_explode (bool bExplode)
	{
		ExplodeNode * pNode = explodeList;

		while (pNode)
		{
			(this->*(pNode->explodeProc))(bExplode);
			pNode = pNode->pNext;
		}
	}
	//----------------------------------
	//
	void FRAME_resolve (void)
	{
		ResolveNode * pNode = resolveList;

		while (pNode)
		{
			(this->*(pNode->resolveProc))();
			pNode = pNode->pNext;
		}
	}
	//----------------------------------
	//
	void FRAME_init (const INITINFO & initStruct)
	{
		frameInitInfo = &initStruct;

		InitNode * pNode = initList;

		while (pNode)
		{
			(this->*(pNode->initProc))(initStruct);
			pNode = pNode->pNext;
		}
	}
	//----------------------------------
	//
	void FRAME_preDestruct (void)
	{
		PreDestructNode * pNode = preDestructList;

		while (pNode)
		{
			(this->*(pNode->destructProc))();
			pNode = pNode->pNext;
		}
	}
	//----------------------------------
	//
	void FRAME_onOpCancel (U32 agentID)
	{
		OnOpCancelNode * pNode = onOpCancelList;

		while (pNode)
		{
			(this->*(pNode->onOpCancelProc))(agentID);
			pNode = pNode->pNext;
		}
	}
	//----------------------------------
	//
	void FRAME_preTakeover (U32 newMissionID, U32 troopID)
	{
		PreTakeoverNode * pNode = preTakeoverList;

		while (pNode)
		{
			(this->*(pNode->preTakeoverProc))(newMissionID, troopID);
			pNode = pNode->pNext;
		}
	}
	//----------------------------------
	//
	void FRAME_receiveOpData (U32 agentID, void *buffer, U32 bufferSize)
	{
		ReceiveOpDataNode * pNode = receiveOpDataList;

		while (pNode)
		{
			(this->*(pNode->receiveOpDataProc))(agentID, buffer, bufferSize);
			pNode = pNode->pNext;
		}
	}
	//----------------------------------
	//
	void FRAME_upgrade (const INITINFO & initStruct)
	{
		UpgradeNode * pNode = upgradeList;

		while (pNode)
		{
			(this->*(pNode->upgradeProc))(initStruct);
			pNode = pNode->pNext;
		}
	}
	//----------------------------------
	//
	U32 FRAME_getPrioritySyncData (void * _buffer)
	{
		PrioritySyncNode * pNode = prioritySyncList;
		U32 result = 0;
		
		if (pNode)
		{
			U8 * buffer = (U8 *) _buffer;

			while (pNode)
			{
				U32 num;
				num = (this->*(pNode->getProc))(buffer+1);
				if (num)
				{
					if (num < 32)								// only room for 5 bit length
						buffer[0] = (num << 3) | (pNode->count);	
					else	// else two bit length
					{
						rmemcpy(buffer+2, buffer+1, num);		// move the block up 1 byte
						buffer[0] = pNode->count;
						buffer[1] = num++;
					}
					result += num + 1;
					buffer += num + 1;
				}
				pNode = pNode->pNext;
			}
		}

		return result;
	}
	//----------------------------------
	//
	U32 FRAME_getGeneralSyncData (void * _buffer)
	{
		GeneralSyncNode * pNode = generalSyncList;
		U32 result = 0;
		
		if (pNode)
		{
			U8 * buffer = (U8 *) _buffer;

			while (pNode)
			{
				U32 num;
				num = (this->*(pNode->getProc))(buffer+1);
				if (num)
				{
					if (num < 32)								// only room for 5 bit length
						buffer[0] = (num << 3) | (pNode->count);	
					else	// else two bit length
					{
						rmemcpy(buffer+2, buffer+1, num);		// move the block up 1 byte
						buffer[0] = pNode->count;
						buffer[1] = num++;
					}
					result += num + 1;
					buffer += num + 1;
				}
				pNode = pNode->pNext;
			}
		}

		return result;
	}
	//----------------------------------
	//
	void FRAME_putPrioritySyncData (void * _buffer, U32 bufferSize, bool bLateDelivery)
	{
		PrioritySyncNode * pNode = prioritySyncList;

		if (pNode)
		{
			U8 * buffer = (U8 *) _buffer;
			U32 count = buffer[0] & 7;
			U32 size;
			U32 esize = 0;		// extra byte
			
			if ((size = (buffer[0] >> 3)) == 0)
			{
				size = buffer[1];
				esize = 1;
			}

			while (pNode)
			{
				if (pNode->count == count)
				{
					CQASSERT(size < bufferSize);
					(this->*(pNode->putProc))(buffer+1+esize, size, bLateDelivery);
					buffer += size + 1 + esize;
					if ((bufferSize -= (size + 1 + esize)) == 0)
						break;		// used all of the data in the buffer
					count = buffer[0] & 7;
					esize = 0;
					if ((size = (buffer[0] >> 3)) == 0)
					{
						size = buffer[1];
						esize = 1;
					}
				}
				pNode = pNode->pNext;
			}
		}
		CQASSERT(bufferSize == 0);
	}
	//----------------------------------
	//
	void FRAME_putGeneralSyncData (void * _buffer, U32 bufferSize, bool bLateDelivery)
	{
		GeneralSyncNode * pNode = generalSyncList;

		if (pNode)
		{
			U8 * buffer = (U8 *) _buffer;
			U32 count = buffer[0] & 7;
			U32 size;
			U32 esize = 0;		// extra byte
			
			if ((size = (buffer[0] >> 3)) == 0)
			{
				size = buffer[1];
				esize = 1;
			}

			while (pNode)
			{
				if (pNode->count == count)
				{
					CQASSERT(size < bufferSize);
					(this->*(pNode->putProc))(buffer+1+esize, size, bLateDelivery);
					buffer += size + 1 + esize;
					if ((bufferSize -= (size + 1 + esize)) == 0)
						break;		// used all of the data in the buffer
					count = buffer[0] & 7;
					esize = 0;
					if ((size = (buffer[0] >> 3)) == 0)
					{
						size = buffer[1];
						esize = 1;
					}
				}
				pNode = pNode->pNext;
			}
		}
		CQASSERT(bufferSize == 0);
	}
};


//--------------------------------------------------------------------------//
//
template <>
struct _NO_VTABLE ObjectFrame<IBaseObject,struct DUMMY_SAVESTRUCT,struct DUMMY_INITSTRUCT> : public IBaseObject
{
	typedef DUMMY_SAVESTRUCT SAVEINFO;
	typedef DUMMY_INITSTRUCT INITINFO;
	typedef void   (IBaseObject::*RenderProc) (void);
	typedef BOOL32 (IBaseObject::*UpdateProc) (void);
	typedef void   (IBaseObject::*PhysUpdateProc) (SINGLE dt);
	typedef void   (IBaseObject::*SaveLoadProc) (SAVEINFO & saveStruct);
	typedef void   (IBaseObject::*ResolveProc) (void);
	typedef void   (IBaseObject::*ExplodeProc) (void);
	typedef void   (IBaseObject::*InitProc) (const INITINFO & initStruct);

	struct PreRenderNode
	{
		PreRenderNode (ObjectFrame<IBaseObject,DUMMY_SAVESTRUCT,DUMMY_INITSTRUCT> * inst, RenderProc _proc)
		{
		}
	};

	struct RenderNode
	{
		RenderNode (ObjectFrame<IBaseObject,DUMMY_SAVESTRUCT,DUMMY_INITSTRUCT> * inst, RenderProc _proc)
		{
		}
	};

	struct PostRenderNode
	{
		PostRenderNode (ObjectFrame<IBaseObject,DUMMY_SAVESTRUCT,DUMMY_INITSTRUCT> * inst, RenderProc _proc)
		{
		}
	};

	struct UpdateNode
	{
		UpdateNode (ObjectFrame<IBaseObject,DUMMY_SAVESTRUCT,DUMMY_INITSTRUCT> * inst, UpdateProc _proc)
		{
		}
	};

	struct PhysUpdateNode
	{
		PhysUpdateNode (ObjectFrame<IBaseObject,DUMMY_SAVESTRUCT,DUMMY_INITSTRUCT> * inst, PhysUpdateProc _proc)
		{
		}
	};

	struct SaveNode
	{
		SaveNode (ObjectFrame<IBaseObject,DUMMY_SAVESTRUCT,DUMMY_INITSTRUCT> * inst, SaveLoadProc _proc)
		{
		}
	};

	struct ResolveNode
	{
		ResolveNode (ObjectFrame<IBaseObject,DUMMY_SAVESTRUCT,DUMMY_INITSTRUCT> * inst, ResolveProc _proc)
		{
		}
	};
	
	struct LoadNode
	{
		LoadNode (ObjectFrame<IBaseObject,DUMMY_SAVESTRUCT,DUMMY_INITSTRUCT> * inst, SaveLoadProc _proc)
		{
		}
	};

	struct ExplodeNode
	{
		ExplodeNode (ObjectFrame<IBaseObject,DUMMY_SAVESTRUCT,DUMMY_INITSTRUCT> * inst, ExplodeProc _proc)
		{
		}
	};

	struct InitNode
	{
		InitNode (ObjectFrame<IBaseObject,DUMMY_SAVESTRUCT,DUMMY_INITSTRUCT> * inst, InitProc _proc)
		{
		}
	};


private:
public:

	ObjectFrame (void)
	{
	}

	~ObjectFrame (void)
	{
	}

};


//---------------------------------------------------------------------------
//------------------------End TObjFrame.h------------------------------------
//---------------------------------------------------------------------------
#endif
