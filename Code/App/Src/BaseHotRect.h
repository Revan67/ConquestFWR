#ifndef BASEHOTRECT_H
#define BASEHOTRECT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              BaseHotRect.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/BaseHotRect.h 30    8/10/00 1:15p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#ifndef TCPOINT_H 
#include "TCPoint.h"
#endif

#ifndef TCONNCONTAINER_H
#include <TConnContainer.h>
#endif

#ifndef TCONNPOINT_H
#include <TConnPoint.h>
#endif

#ifndef EVENTSYS_H
#include <EventSys.h>
#endif

#ifndef IRESOURCE_H
#include "IResource.h"
#endif

#ifndef CURSOR_H
#include "Cursor.h"
#endif

#ifndef STATUSBAR_H
#include "StatusBar.h"
#endif

#ifndef TSMARTPOINTER_H
#include <TSmartPointer.h>
#endif

#ifndef HEAPOBJ_H
#include <HeapObj.h>
#endif

#ifndef EVENTPRIORITY_H
#include "EventPriority.h"
#endif

#ifndef HKEVENT_H
#include <hkevent.h>
#endif

#ifndef TRESCLIENT_H
#include "TResClient.h"
#endif

#ifndef USERDEFAULTS_H
#include "UserDefaults.h"
#endif

#ifndef WINDOWMANAGER_H
#include <WindowManager.h>
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#define HOTRECT_PRIORITY  0x80000000

#undef BASEHOTRECTEXTERN
#ifdef BUILD_TRIM
#define BASEHOTRECTEXTERN __declspec(dllexport)
#else
#define BASEHOTRECTEXTERN __declspec(dllimport)
#endif

//---------------------------------------------------------------------------------
//
struct IKeyboardFocus : IDAComponent
{
	virtual bool SetKeyboardFocus (bool bEnable) = 0;		// return true if can accept keyboard focus

	virtual U32 GetControlID (void) = 0;
};
//--------------------------------------------------------------------------//
// outgoing interface to get notified about button presses
//
struct DACOM_NO_VTABLE IHotControlEvent : IDAComponent
{
	virtual void OnLeftButtonEvent (U32 menuID, U32 controlID) = 0;

	virtual void OnRightButtonEvent (U32 menuID, U32 controlID) = 0;

	virtual void OnLeftDblButtonEvent (U32 menuID, U32 controlID) = 0;		// double-click
};
//---------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE __dummyBaseHotRect : IEventCallback, ResourceClient<>, ConnectionPointContainer<struct BaseHotRect>
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(__dummyBaseHotRect)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()
	//
	// outgoing interface map
	//
	BEGIN_DACOM_MAP_OUTBOUND(__dummyBaseHotRect)
	DACOM_INTERFACE_ENTRY_AGGREGATE("IEventCallback", point)
	DACOM_INTERFACE_ENTRY_AGGREGATE("IKeyboardFocus", point2)
	DACOM_INTERFACE_ENTRY_AGGREGATE("IHotControlEvent", point3)
	END_DACOM_MAP()

	ConnectionPoint2<struct BaseHotRect,IEventCallback> point;
	ConnectionPoint<struct BaseHotRect,IKeyboardFocus> point2;
	ConnectionPoint<struct BaseHotRect,IHotControlEvent> point3;

	__dummyBaseHotRect (void) : point(0), point2(1), point3(2)
	{
	}
};
//---------------------------------------------------------------------------------
//
struct BaseHotRect : public DAComponent<__dummyBaseHotRect>
{
	COMPTR<IDAConnectionPoint> parentEventConnection;
	COMPTR<IDAConnectionPoint> parentKeyboardFocusConnection;
	BaseHotRect *	parent;			// parent 

	U32 eventHandle, keyboardFocusHandle;
	bool bHasFocus:1;
	bool bAlert:1;			// mouse is over rect
	bool bInvisible:1;		// control show act invisible
	
	RECT  screenRect;	// inclusive screen coordinates

	//------------------------
	// private data and methods to support PostMessage
	//------------------------
private:

	struct MESSAGE
	{	
		struct MESSAGE *	pNext;
		U32					message;
		void *				param;
	};

	MESSAGE *pMessageList;
	
	GENRESULT ReverseNotifyClients (CONNECTION_NODE2<IEventCallback> * node, U32 message, void * param)
	{
		if (node)
		{
			ReverseNotifyClients(node->pNext, message, param);
			node->client->Notify(message, param);
		}
		return GR_OK;
	}

	BASEHOTRECTEXTERN void sendPostedMessages (MESSAGE * pNode);		// system.cpp
	
	GENRESULT NotifyClients (U32 message, void * param)
	{
		CONNECTION_NODE2<IEventCallback> * node = point.pClientList;

		if (message == CQE_ENDFRAME)
			return ReverseNotifyClients(node, message, param);
		else
		while (node)
		{
			if (node->client->Notify(message, param) != GR_OK)
				return GR_GENERIC;
			node = node->pNext;		// if this crashes, previous guy deleted himself!
		}
		return GR_OK;
	}

	void removeClient (BaseHotRect *client)
	{
#ifdef _DEBUG
		// verify that client is connected to us
		CONNECTION_NODE2<IEventCallback> * node = point.pClientList;
		
		while (node)
		{
			if (node->client == client)
				break;
			node = node->pNext;
		}

		CQASSERT(node && "BaseHotrect::Cannot remove unknown client");
#endif
		client->Release();
	}
	
	void initializeBaseHotRect (void)
	{
		bHasFocus = 1;
		resPriority = HOTRECT_PRIORITY;

		parentEventConnection->Advise(GetBase(), &eventHandle);
		if (parent)
			parent->SetCallbackPriority(this, EVENT_PRIORITY_HOTRECT);
		if (parentKeyboardFocusConnection)
			parentKeyboardFocusConnection->Advise(GetBase(), &keyboardFocusHandle);
		initializeResources();
	}

public:
	//------------------------
	// public methods
	//------------------------

	BaseHotRect (IDAComponent * systemComponent)
	{
		if(systemComponent)
		{
			systemComponent->QueryOutgoingInterface("IEventCallback", parentEventConnection);
			CQASSERT(parentEventConnection!=0);
		 	systemComponent->QueryOutgoingInterface("IKeyboardFocus", parentKeyboardFocusConnection);
			initializeBaseHotRect();
		}
	}

	BaseHotRect (BaseHotRect * _parent)
	{
		if(_parent)
		{
			parent = _parent;
			CQASSERT(parent);
			parent->GetBase()->QueryOutgoingInterface("IEventCallback", parentEventConnection);
			CQASSERT(parentEventConnection!=0);
	 		parent->QueryOutgoingInterface("IKeyboardFocus", parentKeyboardFocusConnection);
			initializeBaseHotRect();
		}
	}

	// use this if cannot initialize resources at construction time
	//
	void lateInitialize (IDAComponent * systemComponent)
	{
	 	CQASSERT(parentEventConnection==0);	// cannot init twice!
	 	systemComponent->QueryOutgoingInterface("IEventCallback", parentEventConnection);
	 	CQASSERT(parentEventConnection!=0);
	 	systemComponent->QueryOutgoingInterface("IKeyboardFocus", parentKeyboardFocusConnection);
	 	initializeBaseHotRect();
	}

	BASEHOTRECTEXTERN virtual ~BaseHotRect (void);		// system.cpp

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "BaseHotRect");
	}

	GENRESULT QueryOutgoingInterface (const C8 *connectionName, struct IDAConnectionPoint **connection)
	{
		return FindConnectionPoint(connectionName, connection);
	}

	IDAComponent * GetBase (void)
	{
		return static_cast<IEventCallback *>(this);
	}

	BOOL32 CheckRect (S32 x, S32 y)
	{
		bAlert = 0;

		if (bHasFocus && bInvisible==false && x >= screenRect.left && x <= screenRect.right && y >= screenRect.top && y <= screenRect.bottom)
			bAlert = 1;

		return bAlert;
	}

	void SetRect (struct tagRECT * rect)
	{
		screenRect = *rect;
	}

	void GetRect (struct tagRECT * rect) const
	{
		*rect = screenRect;
	}

	BOOL32 IsAlert (void) const
	{
		return bAlert;
	}

	GENRESULT SetCallbackPriority (struct IEventCallback * client, U32 priority)
	{
		if (client && MoveToFront(client, priority))
			return GR_OK;
		else
			return GR_INVALID_PARMS;
	}

	BASEHOTRECTEXTERN void PostMessage (U32 message, void * param);		// system.cpp

	virtual void DrawRect (void)
	{
		U32 color = (U32)-1;
		//
		// draw the box
		//
		DA::LineDraw(0, screenRect.left, screenRect.top, screenRect.right, screenRect.top, color);
		DA::LineDraw(0, screenRect.right, screenRect.top, screenRect.right, screenRect.bottom, color);
		DA::LineDraw(0, screenRect.right, screenRect.bottom, screenRect.left, screenRect.bottom, color);
		DA::LineDraw(0, screenRect.left, screenRect.bottom, screenRect.left, screenRect.top, color);
	}

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param)
	{
		MSG *msg = (MSG *) param;
		GENRESULT genresult = GR_OK;

		switch (message)
		{
		case CQE_DELETE_HOTRECT:
			removeClient((BaseHotRect *)param);
			break;

		case CQE_SLIDER:
		case CQE_LIST_SELECTION:
		case CQE_LIST_CARET_MOVED:
		case CQE_BUTTON:
		case CQE_LHOTBUTTON:
		case CQE_RHOTBUTTON:
			break;
		case CQE_KILL_FOCUS:
		case CQE_SET_FOCUS:
			if (param == this)
				break;
			// fall through intentional
		default:
			genresult = NotifyClients(message, param);	// send the message to clients first
		} // end switch (message)

		switch (message)
		{
		case CQE_UNALERT:
			bAlert = 0;
			break;

		case CQE_KILL_FOCUS:
			if (param != this)
				onSetFocus(false);
			break;

		case CQE_SET_FOCUS:
			if (param != this)
				onSetFocus(true);
			break;

		case CQE_ENDFRAME:
			if (bInvisible==false && DEFAULTS->GetDefaults()->bDrawHotrects)
				DrawRect();
			break;

		case WM_MOUSEMOVE:
			CheckRect(S16(LOWORD(msg->lParam)), S16(HIWORD(msg->lParam)));
			break;

		case WM_LBUTTONDOWN:
			onRequestKeyboardFocus(S16(LOWORD(msg->lParam)), S16(HIWORD(msg->lParam)));
			break;

		case WM_CLOSE:
		case CQE_FLUSHMESSAGES:
			{
				MESSAGE *pNode;

				if ((pNode = pMessageList) != 0)
				{
					pMessageList = 0;
					sendPostedMessages(pNode);
				}
			}
			break;  // end case CQE_UPDATE

		} // end switch (message)

		return genresult;
	}

	/* BaseHotRect methods */

	BOOL32 MoveToFront (struct IEventCallback * res, U32 priority)
	{
		CONNECTION_NODE2<IEventCallback> *node = point.pClientList, *prev=0;

		if (node)
		{
			if (node->client != res)
			{
				prev = node;
				while ((node = node->pNext) != 0)
				{
					if (node->client == res)
					{
						prev->pNext = node->pNext;
						node->pNext = point.pClientList;
						point.pClientList = node;
						break;
					}
					prev = node;
				}
				if (node == 0)
					return 0;
			}
			//
			// node is now at the front of the list
			//
			node->priority = priority;
			//
			// now we must sort the entries in decending order
			// 
			CONNECTION_NODE2<IEventCallback> *next = node->pNext;
			
			prev = 0;
			while (next && next->priority > priority)
			{	
				prev = next;
				next = next->pNext;
			}
			if (prev)		// if sorting is needed
			{
				point.pClientList = node->pNext;
				node->pNext = prev->pNext;
				prev->pNext = node;
			}
		}

		return 1;
	}

	// override this if you want to
	virtual void onSetFocus (bool bFocus)
	{
		if (bFocus == false)
		{
			bAlert = 0;
			bHasFocus = 0;
			desiredOwnedFlags = 0;
			releaseResources();
		}
		else
		{
			bHasFocus = 1;
		}
	}

	// override this if you want to
	virtual void onRequestKeyboardFocus (int x, int y)
	{
		if (bAlert && bInvisible==false && parent && keyboardFocusHandle)
			parent->PostMessage(CQE_KEYBOARD_FOCUS, static_cast<IEventCallback *>(this));
	}
};

//--------------------------------------------------------------------------//
//---------------------------End BaseHotRect.h------------------------------//
//--------------------------------------------------------------------------//
#endif
