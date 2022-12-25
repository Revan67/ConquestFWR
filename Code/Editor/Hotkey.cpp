//--------------------------------------------------------------------------//
//                                                                          //
//                                Hotkey.cpp                                //
//                                                                          //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

 
#include "stdafx.h"
#include "globals.h"

#include "Startup.h"

#include <TComponent.h>
#include <TSmartPointer.h>
#include <IConnection.h>
#include <Engine.h>
#include <EventSys.h>
#include <system.h>

#include "HKEvent.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

struct KeyCombo
{
	HOTKEYS_ID hotKeyAction;
	U32        keyOne;
	U32        keyTwo;
	bool       bSignalOnActivate;
	bool       bSignalOnDeactivate;
	bool       bActive;
};

// the order is important (check with Allen before modifying)

static KeyCombo s_KeyComboList[] =
{
	{ IDH_ROTATE_WORLD_LEFT,     VK_LEFT,   VK_CONTROL, false, true, false },
	{ IDH_ROTATE_WORLD_RIGHT,    VK_RIGHT,  VK_CONTROL, false, true, false },
//	{ IDH_ROTATE_WORLD_UP,       VK_UP,     VK_CONTROL, false, true, false },
//	{ IDH_ROTATE_WORLD_DOWN,     VK_DOWN,   VK_CONTROL, false, true, false },
	{ IDH_TOGGLE_ZOOM,           VK_SPACE,  VK_CONTROL, false, true, false },
//	{ IDH_ROTATE_0_WORLD,        VK_HOME,   VK_CONTROL, false, true, false },
//	{ IDH_ROTATE_90_WORLD_LEFT,  VK_DELETE, VK_CONTROL, false, true, false },
//	{ IDH_ROTATE_90_WORLD_RIGHT, VK_END,    VK_CONTROL, false, true, false },

	{ IDH_ZOOM_IN,  VK_UP,   VK_SHIFT, false, true, false },
	{ IDH_ZOOM_OUT, VK_DOWN, VK_SHIFT, false, true, false },

	{ IDH_SCROLL_DOWNLEFT,  VK_DOWN,  VK_LEFT,  true, true, false },
	{ IDH_SCROLL_DOWNRIGHT, VK_DOWN,  VK_RIGHT, true, true, false },
	{ IDH_SCROLL_UPLEFT,    VK_UP,    VK_LEFT,  true, true, false },
	{ IDH_SCROLL_UPRIGHT,   VK_UP,    VK_RIGHT, true, true, false },
	{ IDH_SCROLL_DOWN,		VK_DOWN,  0,        true, true, false },
	{ IDH_SCROLL_LEFT,      VK_LEFT,  0,        true, true, false },
	{ IDH_SCROLL_RIGHT,     VK_RIGHT, 0,        true, true, false },
	{ IDH_SCROLL_UP,        VK_UP,    0,        true, true, false },
};

const unsigned s_NumKeys = sizeof(s_KeyComboList) / sizeof(KeyCombo);

struct DACOM_NO_VTABLE Hotkey : public IHotkeyEvent, IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(Hotkey)
		DACOM_INTERFACE_ENTRY(IHotkeyEvent)
		DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	U32 eventHandle;

	DEFMETHOD(Initialize) (void)
	{
		return GR_OK;
	}

	DEFMETHOD(Enable) (void);

	DEFMETHOD(Disable) (void);

	DEFMETHOD_(BOOL32,GetVkeyState) (U32 vkey);

	DEFMETHOD_(BOOL32,GetHotkeyState) (U32 hotkey);

	virtual void __cdecl SystemMessage ( S32 hwnd, S32 message, S32 wParam, S32 lParam, S32 unused1, S32 unused2)
	{
		int t = 5;
	}

	DEFMETHOD_(U32,GetHotkeyText) (U32 hotkey, C8 *outBuffer, U32 bufferSize);

	DEFMETHOD_(BOOL32,IsAnyHotkeyPressed) (void);

	// IEventCallback methods

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	// local methods

	void key_press_update();
};

//-----------------------------------------------------------------------------------------------------

GENRESULT COMAPI Hotkey::Enable()
{
	return GR_OK;
}

//-----------------------------------------------------------------------------------------------------

GENRESULT COMAPI Hotkey::Disable()
{
	return GR_OK;
}

//-----------------------------------------------------------------------------------------------------

BOOL32 COMAPI Hotkey::GetVkeyState(U32 vkey)
{
	return( (GetAsyncKeyState(vkey) & 0x8001) != 0 );
}
//-----------------------------------------------------------------------------------------------------

BOOL32 COMAPI Hotkey::GetHotkeyState(U32 hotkey)
{
	for( unsigned i = 0; i < s_NumKeys; i++ )
	{
		if( s_KeyComboList[i].hotKeyAction == (HOTKEYS_ID)hotkey )
		{
			return s_KeyComboList[i].bActive;
		}
	}

	return false;
}
//-----------------------------------------------------------------------------------------------------

U32 COMAPI Hotkey::GetHotkeyText(U32 hotkey, C8 *outBuffer, U32 bufferSize)
{
	return 0;
}
//-----------------------------------------------------------------------------------------------------

BOOL32 COMAPI Hotkey::IsAnyHotkeyPressed()
{
	for( unsigned i = 0; i < s_NumKeys; i++ )
	{
		if( s_KeyComboList[i].bActive )
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

GENRESULT Hotkey::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
		case CQE_UPDATE:
			key_press_update();
			break;
	}

	return GR_OK;
}

//-----------------------------------------------------------------------------------------------------

void Hotkey::key_press_update()
{
	for( unsigned i = 0; i < s_NumKeys; i++ )
	{
		bool changedToActive = false;
		bool changedFromAction = false;

		// is a two key combo?
		if( s_KeyComboList[i].keyOne && s_KeyComboList[i].keyTwo )
		{
			BOOL32 key1 = GetVkeyState(s_KeyComboList[i].keyOne);
			BOOL32 key2 = GetVkeyState(s_KeyComboList[i].keyTwo);

			if( s_KeyComboList[i].bActive && (!key1 || !key2) )
			{
				// the key combo is "Key Up"
				changedFromAction = true;
			}
			else if( !s_KeyComboList[i].bActive && (key1 && key2) )
			{
				// the key combo is "Key Down"
				changedToActive = true;
			}
		}
		else if( s_KeyComboList[i].keyOne )
		{
			BOOL32 key1 = GetVkeyState(s_KeyComboList[i].keyOne);

			if( s_KeyComboList[i].bActive && !key1 )
			{
				// the key combo is "Key Up"
				changedFromAction = true;
			}
			else if( !s_KeyComboList[i].bActive && key1 )
			{
				// the key combo is "Key Down"
				changedToActive = true;
			}
		}

		if( changedToActive )
		{
			s_KeyComboList[i].bActive = true;
			if( s_KeyComboList[i].bSignalOnActivate )
			{
				EVENTSYS->Send( CQE_HOTKEY, (void*)s_KeyComboList[i].hotKeyAction );
			}
		}
		else if( changedFromAction )
		{
			s_KeyComboList[i].bActive = false;
			if( s_KeyComboList[i].bSignalOnDeactivate )
			{
				EVENTSYS->Send( CQE_HOTKEY, (void*)s_KeyComboList[i].hotKeyAction );
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

struct _hotkey : GlobalComponent
{
	Hotkey * hotkey;

	virtual void Startup (void)
	{
		HOTKEY = hotkey = new DAComponent<Hotkey>;
		AddToGlobalCleanupList((IDAComponent **) &HOTKEY);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
		if (SYSTEM->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise( static_cast<IHotkeyEvent *>(hotkey), &hotkey->eventHandle);
		}
	}
};
static _hotkey __hotkey;
