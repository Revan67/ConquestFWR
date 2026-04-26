//--------------------------------------------------------------------------//
//                                                                          //
//                               EventSys.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Pbleisch $
*/			    
//--------------------------------------------------------------------------//


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#include "EventSys.h"

#include "TComponent.h"
#include "TConnContainer.h"
#include "TConnPoint.h"
#include "System.h"
#include "da_heap_utility.h"

//--------------------------------------------------------------------------//
//---------------------------Local structures-------------------------------//
//--------------------------------------------------------------------------//

struct MESSAGE_NODE
{
	struct MESSAGE_NODE *	pNext;
	U32						message;
	void *					param;
};

static char interface_name[] = "IEventSystem";


//--------------------------------------------------------------------------//
//--------------------------EventSystem Class definition--------------------//
//--------------------------------------------------------------------------//


struct DACOM_NO_VTABLE EventSystem : public ISystemComponent, 
											IEventSystem, 
											IEventCallback,
											ConnectionPointContainer<EventSystem>
{
	BEGIN_DACOM_MAP_INBOUND(EventSystem)
	DACOM_INTERFACE_ENTRY(IEventSystem)
	DACOM_INTERFACE_ENTRY(ISystemComponent)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IEventSystem,IEventSystem)
	DACOM_INTERFACE_ENTRY2(IID_ISystemComponent,ISystemComponent)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IEventCallback,IEventCallback)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer,IDAConnectionPointContainer)
	END_DACOM_MAP()

	BEGIN_DACOM_MAP_OUTBOUND(EventSystem)
	DACOM_INTERFACE_ENTRY_AGGREGATE("IEventCallback", eventCallback)
	END_DACOM_MAP()


	GENRESULT init (AGGDESC * info);

	DA_HEAP_DEFINE_NEW_OPERATOR(EventSystem)

	MESSAGE_NODE *pFreeList;
	MESSAGE_NODE *pShadowList, *pShadowListEnd;
	MESSAGE_NODE *pMessageList, *pMessageListEnd;
	DWORD dwLastPeek;
	MESSAGE_NODE *pPeekNode;
	BOOL32 bInitialized;
	ConnectionPoint<EventSystem, IEventCallback> eventCallback;

	EventSystem (void) : eventCallback(0)
	{
	}

	~EventSystem (void);
	
	/* IEventSystem members */

	DEFMETHOD(Initialize) (U32 flags);

	DEFMETHOD(Post) (U32 message, void *param = 0);

	DEFMETHOD(Send) (U32 message, void *param = 0);

	DEFMETHOD(Peek) (U32 index, U32 *message = 0, void **param = 0);

	/* IAggregateComponent members */

    DEFMETHOD(Initialize) (void);
	
	/* ISystemComponent members */

    DEFMETHOD_(void,Update) (void);

	/* IEventCallback members */
	
	DEFMETHOD(Notify) (U32 message, void *param = 0)
	{
		return Post(message, param);
	}

	/* EventSystem members */

	IDAComponent * GetBase(void)
	{
		return (ISystemComponent *) this;
	}
};

//--------------------------------------------------------------------------//
//
GENRESULT EventSystem::init (AGGDESC * info)
{
	if (info->description && strcmp(info->description, "EventSystem"))
		return GR_INTERFACE_UNSUPPORTED;
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
EventSystem::~EventSystem (void)
{
	MESSAGE_NODE *pTmp;

	while (pFreeList)
	{
		pTmp = pFreeList->pNext;
		delete pFreeList;
		pFreeList = pTmp;
	}
	while (pShadowList)
	{
		pTmp = pShadowList->pNext;
		delete pShadowList;
		pShadowList = pTmp;
	}
	while (pMessageList)
	{
		pTmp = pMessageList->pNext;
		delete pMessageList;
		pMessageList = pTmp;
	}
}
//--------------------------------------------------------------------------//
//
GENRESULT EventSystem::Initialize (U32 flags)
{
	if (flags)
		return GR_GENERIC;
	return GR_OK;	// no flags implemented yet
}
//--------------------------------------------------------------------------//
//
GENRESULT EventSystem::Post (U32 message, void *param)
{
	GENRESULT result = GR_OK;
	MESSAGE_NODE *pNode;

	if ((pNode = pFreeList) != 0)
		pFreeList = pNode->pNext;
	else
	if ((pNode = (MESSAGE_NODE *) DA_HEAP_MALLOC( HEAP, sizeof(MESSAGE_NODE), "IEventSystem::Message cell" ) ) == 0)
	{
		result = GR_OUT_OF_MEMORY;
		goto Done;
	}

	pNode->pNext = 0;
	pNode->message = message;
	pNode->param = param;

	if (pShadowListEnd)
	{
		pShadowListEnd->pNext = pNode;
		pShadowListEnd = pNode;
	}
	else	// empty list
	{
		pShadowList = pShadowListEnd = pNode;
	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
GENRESULT EventSystem::Send (U32 message, void *param)
{
	CONNECTION_NODE<IEventCallback> *pList = eventCallback.pClientList, *pNext;

	static FILE *_evf = nullptr;
	if (!_evf) _evf = fopen("eventsys_diag.txt", "w");

	if (_evf) { fprintf(_evf, "Send msg=0x%04X pList=%p\n", message, (void*)pList); fflush(_evf); }

	//------------------------------
	// service all of the callbacks
	//------------------------------

	int idx = 0;
	while (pList)
	{
		pNext = pList->pNext;
		if (_evf) { DWORD _vtbl = pList->client ? *(DWORD*)(void*)pList->client : 0; fprintf(_evf, "  [%d] client=%p vtbl=0x%08X pNext=%p\n", idx, (void*)pList->client, _vtbl, (void*)pNext); fflush(_evf); }
		pList->client->Notify(message, param);
		if (_evf) { fprintf(_evf, "  [%d] ok\n", idx); fflush(_evf); }
		idx++;
		pList = pNext;
	}

	if (_evf) { fprintf(_evf, "Send done\n"); fflush(_evf); }

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
GENRESULT EventSystem::Peek (U32 index, U32 *message, void **param)
{
	GENRESULT result = GR_GENERIC;
	MESSAGE_NODE *pNode;
	U32 lastPeek = dwLastPeek;

	if ((pNode = pPeekNode) == 0 || index < lastPeek)
	{
		if ((pNode = pMessageList) == 0)
			goto Done;
		lastPeek = 0;
	}

	while (lastPeek < index)
	{
		if ((pNode = pNode->pNext) == 0)
			goto Done;
		lastPeek++;
	}
	
	if (message)
		*message = pNode->message;
	if (param)
		*param = pNode->param;
	// save these values for next time
	pPeekNode = pNode;
	dwLastPeek = lastPeek;

	result = GR_OK;
Done:
	return result;
}
//--------------------------------------------------------------------------//
//
GENRESULT EventSystem::Initialize (void)
{
	bInitialized = 1;
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void EventSystem::Update (void)
{
	MESSAGE_NODE *pTmp = pShadowList;
	CONNECTION_NODE<IEventCallback> *pList, *pNext;
	
	//------------------------------
	// service all of the callbacks
	//------------------------------
	
	if (eventCallback.pClientList)
	while (pTmp)
	{
		pList = eventCallback.pClientList;
		do
		{
			pNext = pList->pNext;
			pList->client->Notify(pTmp->message, pTmp->param);

		} while ((pList = pNext) != 0);

		pTmp = pTmp->pNext;
	}

	// put everyone in the message list into the free list

	if (pMessageListEnd)
	{
		pMessageListEnd->pNext = pFreeList;
		pFreeList = pMessageList;
	}

	pMessageList = pShadowList;
	pMessageListEnd = pShadowListEnd;

	pShadowList = pShadowListEnd = 0;
	dwLastPeek = 0;
	pPeekNode = 0;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
void RegisterTheEventSystem (ICOManager *DACOM)
{
	IComponentFactory *pServer = new DAComponentFactory2<DAComponentAggregate<EventSystem>, AGGDESC> (interface_name);

	if (pServer)
	{
		DACOM->RegisterComponent(pServer, interface_name);
		pServer->Release();
	}
}


//--------------------------------------------------------------------------//
//---------------------------------EventSys.cpp-----------------------------//
//--------------------------------------------------------------------------//
