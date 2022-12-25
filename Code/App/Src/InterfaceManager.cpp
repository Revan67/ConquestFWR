//--------------------------------------------------------------------------//
//                                                                          //
//                      InterfaceManager.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 2004 Warthog, INC.                           //
//																			//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "IInterfaceManager.h"
#include "BaseHotRect.h"
#include "EventPriority.h"
#include "Startup.h"
#include "ObjWatch.h"
#include "IObject.h"
#include "ObjList.h"

#include "frame.h"
#include "hotkeys.h"
#include "Sfx.h"

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct FlashingHotkey
{
	FlashingHotkey * next;
	U32 hotkeyID;
};
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE InterfaceManager : public IEventCallback, IInterfaceManager
{
	BEGIN_DACOM_MAP_INBOUND(InterfaceManager)
	DACOM_INTERFACE_ENTRY(IInterfaceManager)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	U32 handle;			// connection handle

	FlashingHotkey * keys;

	SINGLE flash;

	InterfaceManager (void);

	~InterfaceManager (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IInterfaceManager methods */
	virtual void StartHotkeyFlash (U32 hotkeyID);

	virtual void StopHotkeyFlash (U32 hotkeyID);
	
	virtual void Flush ();

	virtual bool RenderHotkeyFlashing (U32 hotkeyID);

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* InterfaceManager methods */

	IDAComponent * GetBase (void)
	{
		return (IInterfaceManager *) this;
	}

	void onUpdate (U32 dt)			// dt is milliseconds
	{
		SINGLE sec = dt;
		sec /= 1000;
		flash += sec;
		if(flash > 2.0)
			flash = 0.0;
		else if(flash > 0.5)
			flash = flash-0.5;
	}
};
//--------------------------------------------------------------------------//
//
InterfaceManager::InterfaceManager (void)
{
	flash = 0;
	keys = NULL;
}
//--------------------------------------------------------------------------//
//
InterfaceManager::~InterfaceManager (void)
{
	Flush();
	if (FULLSCREEN)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(handle);
	}
}

//--------------------------------------------------------------------------//
//
void InterfaceManager::StartHotkeyFlash (U32 hotkeyID)
{
	FlashingHotkey * search = keys;
	while(search)
	{
		if(search->hotkeyID == hotkeyID)
			return;//already flashing
		search = search->next;
	}
	search = new FlashingHotkey;
	search->next = keys;
	search->hotkeyID = hotkeyID;
	keys = search;
}
//--------------------------------------------------------------------------//
//
void InterfaceManager::StopHotkeyFlash (U32 hotkeyID)
{
	FlashingHotkey * search = keys;
	FlashingHotkey * prev = NULL;
	while(search)
	{
		if(search->hotkeyID == hotkeyID)
		{
			if(prev)
				prev->next = search->next;
			else
				keys = search->next;
			delete search;
			return;
		}
		prev = search;
		search = search->next;
	}
}
//--------------------------------------------------------------------------//
//
void InterfaceManager::Flush ()
{
	FlashingHotkey * search = keys;
	while(search)
	{
		FlashingHotkey * tmp = search->next;
		delete search;
		search = tmp;
	}
	keys = NULL;
}
//--------------------------------------------------------------------------//
//
bool InterfaceManager::RenderHotkeyFlashing (U32 hotkeyID)
{
	if(flash > 0.25)
	{
		FlashingHotkey * search = keys;
		while(search)
		{
			if(search->hotkeyID == hotkeyID)
				return true;
			search = search->next;
		}
	}
	return false;
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT InterfaceManager::Notify (U32 message, void *param)
{
	static BOOL32 bBrief = FALSE;

	switch (message)
	{
	case CQE_MISSION_CLOSE:
		Flush();
		break;

	case CQE_UPDATE:
		onUpdate(S32(param) >> 10);
		break;
	}

	return GR_OK;
}

//--------------------------------------------------------------------------//
//
struct InterfaceManagerComponent : GlobalComponent
{
	InterfaceManager * IMAN;
	
	virtual void Startup (void)
	{
		INTERMAN = IMAN = new DAComponent<InterfaceManager>;
		AddToGlobalCleanupList((IDAComponent **) &INTERMAN);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
	
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise(IMAN->GetBase(), &IMAN->handle);
			FULLSCREEN->SetCallbackPriority(IMAN, EVENT_PRIORITY_LINEDISPLAY);
		}
	}
};

static InterfaceManagerComponent interfaceManagerComponent;

//--------------------------------------------------------------------------//
//--------------------------End InterfaceManager.cpp-----------------------------//
//--------------------------------------------------------------------------//