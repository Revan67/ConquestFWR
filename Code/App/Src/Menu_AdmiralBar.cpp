//--------------------------------------------------------------------------//
//                                                                          //
//                             Menu_AdmiralBar.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_AdmiralBar.cpp 124   9/19/01 9:54a Tmauer $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include <wchar.h>

#include "Frame.h"
#include "IHotButton.h"
#include "IToolbar.h"
#include "IStatic.h"
#include "IHotStatic.h"
#include "ObjList.h"
#include "MPart.h"
#include "Startup.h"
#include "IButton2.h"
#include "IAttack.h"
#include "ILauncher.h"
#include "Hotkeys.h"
#include "DBHotkeys.h"
#include "Sfx.h"
#include "CommPacket.h"
#include "Camera.h"
#include "UnitComm.h"
#include "IAdmiral.h"
#include "ITroopship.h"
#include "ITabControl.h"
#include "IShapeLoader.h"
#include "IImageReader.h"
#include "Sector.h"
#include "IFabricator.h"
#include "SuperTrans.h"
#include "ICloak.h"

#include <DStatic.h>
#include <DHotButton.h>
#include <DHotStatic.h>
#include <DAdmiralBar.h>
#include <DSounds.h>

#include <DataParser.h>
#include <ViewCnst.h>

#include <DButton.h>

#define MOVE_DISTANCEX IDEAL2REALX(20)
#define MOVE_DISTANCEY IDEAL2REALY(20)
#define MAX_UI_PACKET_SIZE 256
#define MAX_SELECTED_UNITS 22		

#define HOTBUTTON_TYPE		"HotButton!!Default"
#define TAB_TYPE			"Tab!!Test"

// ONLY HAVE 9 WORKING SO FAR
// this includes the assault attack
/*
#define NUM_SPECIAL_ATTACKS 17

#define TERRAN_BEGIN   1
#define TERRAN_END     6
#define MANTIS_BEGIN   6
#define MANTIS_END     11
#define SOLARIAN_BEGIN 11
#define SOLARIAN_END   17

#define NUM_TERRAN		TERRAN_END - TERRAN_BEGIN
#define NUM_MANTIS		MANTIS_END - MANTIS_BEGIN
#define NUM_SOLARIAN	SOLARIAN_END - SOLARIAN_BEGIN

enum SpecialAttack
{
	SA_ASSAULT = 0,		// 0
	SA_AEGIS,			// 1
	SA_PROBE,			// 2
	SA_CLOAK,			// 3
	SA_VAMPIRE,			// 4
	SA_TEMPEST,			// 5
// mantis
	SA_STASIS,			// 6
	SA_FURYRAM,			// 7
	SA_REPEL,			// 8
	SA_REPULSOR,		// 9
	SA_MIMIC,			// 10
// solarian
	SA_SYNTHESIS,		// 11
	SA_MASSDISRUPT,		// 12
	SA_DESTABILIZER,	// 13	
	SA_SOLARIANCLOAK,	// 14
	SA_SHROUD,			// 15
	SA_AUGER			// 16
};
*/
//--------------------------------------------------------------------------//
//
struct Menu_AdmiralBar : public Frame, IToolbar
{
	//
	// interface map
	//

	BEGIN_DACOM_MAP_INBOUND(Menu_AdmiralBar)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	DACOM_INTERFACE_ENTRY_REF("IViewer", viewer)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IToolbar)

	// the following are for BaseHotRect
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	enum CURSOR_MODE
	{
		CM_NOT_OWNED=0,
		CM_INTERMEDIATE_CURSOR,
		CM_CAPTURE_DENIED,
		CM_DEFAULT_CURSOR,
		CM_ATTACK,
		CM_SPECIAL_ATTACK,
		CM_SPECIAL_ATTACKGATE,
		CM_CAPTURE,
		CM_DOCK_FLAGSHIP,
		CM_INTERMEDIATE_DOCK,
		CM_PROBE,
		CM_PROBEJUMP,
		CM_PROBEJUMPUNK,
		CM_MIMIC,
		CM_RAM
	};

	enum TARGET_MODE
	{
		TARGET_NONE,
		TARGET_OBJECT,
		TARGET_ANYWHERE		
	};

	//
	// data items
	//
	GT_ADMIRALBAR data;

	CURSOR_MODE cursorMode, specialCursor;
	CURSOR_MODE oldCursorMode;
	TARGET_MODE targetMode;

	S32 oldMouseX, oldMouseY;

	bool bLButtonDown;

	// how many times do you want to launch the same attack?
	U32 oldButtonID;
	U32 numAttacks;
	UNIT_SPECIAL_ABILITY unitAbility;

	U32 toolbarID;
	S32 timer;

	U32 admiralKey;
	OBJPTR<IBaseObject> admiral;
	U32 admiralID;

	//
	// instance methods
	//

	Menu_AdmiralBar (void)
	{
	}

	~Menu_AdmiralBar (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message, void *parm);

	/* IToolbar methods */

	virtual GENRESULT __stdcall GetControl (const char *buttonName, void ** ppControl);

	virtual GENRESULT __stdcall GetToolbar (const char *menuName, struct IToolbar ** ppMenu, enum M_RACE race);

	virtual void SetVisible (bool bVisible);

	virtual bool GetVisible (void);

	virtual bool GetFocus (void)
	{
		return bHasFocus;
	}

	virtual void SetToolbarID (U32 id)
	{
		toolbarID = id;
	}

	virtual U32 GetToolbarID (void)
	{
		return toolbarID;
	}

	virtual void GetSystemMapRect (S32 & left, S32 & top, S32 & right, S32 & bottom)
	{
	};

	virtual void GetSectorMapRect (S32 & left, S32 & top, S32 & right, S32 & bottom)
	{
	};

	/* BaseHotRect methods */

	virtual void DrawRect (void)
	{
		BaseHotRect::DrawRect();
	}


	/* Menu_AdmiralBar methods */

	void localSendPacket (const BASE_PACKET * packet)
	{
		CQASSERT(packet->dwSize <= MAX_UI_PACKET_SIZE);
		NETPACKET->Send(HOSTID, 0, packet);
	}

	void playSound (SFX::ID id)
	{
		HSOUND hSound;
		
		if ((hSound = SFXMANAGER->Open(id)) != 0)
		{
			SFXMANAGER->Play(hSound);
			SFXMANAGER->CloseHandle(hSound);
		}
	}

	void update (S32 dt);

	virtual void setStateInfo (void);

	void init (void);

	void handleHotkeyPressed (U32 hotkeyID);

	void handleSpecialOrder(U32 orderButtonID);

	void setCursorMode (CURSOR_MODE newMode);

	void updateCursor (void);

	void arrangeResources (U32 leftStatusTextID, U32 rightStatusTextID, U32 uCursorID);

	bool onLButtonDown (S32 x, S32 y);

	bool onLButtonUp (S32 x, S32 y, U32 wParam);

	bool onRButtonUp (S32 x, S32 y, U32 wParam);

	void insertSortObj (U32 objectID[MAX_SELECTED_UNITS], int size, MPart & part);

	bool ScreenToPoint(S32 x, S32 y, Vector& vec);

	void onHandleCloak (void);

	void onHandleSpecialAbility (void);

	void onHandleSpecialAbilityAll (void);

	void handleMassShroud (void);

	void onCreateFleet (void);

	void onDisbandFleet (void);

	void onRepairFleet (void);

	void onResupplyFleet (void);

	void onSelectAllFleet(void);

	bool handleSpecialAttackTarget (S32 x, S32 y, bool bQueued); 

	bool handleSpecialAttackArea (S32 x, S32 y, bool bQueued); 

	bool handleProbe (S32 x, S32 y, bool bQueued); 

	bool handleCapture (void);

	bool handleMimic (void);

	bool handleTransfer (void);

	void handleCommandKitRes(U32 index);

	IBaseObject * findClosestPlatform (const MPart & part, const MISSION_DATA::M_CAPS & platCaps);

	U32 getNumJumps (U32 startSystem, U32 endSystem, U32 playerID);

	void testCancel (void)
	{
		setCursorMode(CM_NOT_OWNED);

		targetMode = TARGET_NONE;
		specialCursor = CM_DEFAULT_CURSOR;
		numAttacks = 0;
	}

	bool testSpecialAbilitySelected (void)
	{
		if (unitAbility == USA_NONE)
		{
			if (ownsResources())
			{
				setCursorMode(CM_NOT_OWNED);
			}
			return false;
		}
		return true;
	}

	bool testOverAttackable (MPart hTarget);

	void findLowestAdmiral (void);

	bool hasTroopship (void);
	
	bool hasDockable (void);

	bool canTraverseWarpgate (UNIT_SPECIAL_ABILITY usa)
	{
		if (usa == USA_TEMPEST || usa == USA_PROBE || usa == USA_DESTABILIZER || usa == USA_STASIS)
		{
			return true;
		}
		return false;
	}

	bool handleSpecialAttackWarpgate (bool bQueued); 

	GENRESULT __stdcall Notify (U32 message, void *param);
};
//----------------------------------------------------------------------------------//
//
Menu_AdmiralBar::~Menu_AdmiralBar (void)
{
	ADMIRALBAR = NULL;
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_AdmiralBar::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, (void*)&data, sizeof(GT_ADMIRALBAR), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::arrangeResources (U32 leftStatusTextID, U32 rightStatusTextID, U32 uCursorID)
{
	U32 uStatusTextID;

	uStatusTextID = leftStatusTextID;

	desiredOwnedFlags = RF_CURSOR | RF_STATUS;

	if (statusTextID == uStatusTextID && cursorID == uCursorID)
	{
		if (ownsResources() == 0)
		{
			grabAllResources();
		}
	}
	else
	{
		statusTextID = uStatusTextID;
		cursorID = uCursorID;
		if (ownsResources())
			setResources();
		else
		{
			grabAllResources();
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_AdmiralBar::setCursorMode (CURSOR_MODE newMode)
{
	switch (newMode)
	{
	case CM_ATTACK:
		arrangeResources(IDS_ATTACKOBJECT, 0, IDC_CURSOR_ATTACK);
		break;

	case CM_CAPTURE:
		arrangeResources(IDS_CAPTUREOBJECT, 0, IDC_CURSOR_CAPTURE);
		break;

	case CM_CAPTURE_DENIED:
		arrangeResources(IDS_CAPTUREDENIED, 0, IDC_CURSOR_BAN);
		break;

	case CM_PROBEJUMPUNK:
	case CM_PROBEJUMP:
	case CM_PROBE:
	case CM_SPECIAL_ATTACK:
		arrangeResources(IDS_SPECIALATTACKOBJECT, 0, IDC_CURSOR_SPECIAL_ATTACK);
		break;

	case CM_SPECIAL_ATTACKGATE:
		arrangeResources(IDS_SPECIALATTACKOBJECT, 0, IDC_CURSOR_WORM_ATTACK);
		break;	

	case CM_MIMIC:
		arrangeResources(IDS_MIMIC, 0, IDC_CURSOR_MIMIC);
		break;

	case CM_RAM:
		arrangeResources(IDS_RAMOBJECT, 0, IDC_CURSOR_RAM);
		break;

	case CM_INTERMEDIATE_CURSOR:
		arrangeResources(IDS_SPECIALATTACKOBJECT, 0, IDC_CURSOR_INTERMEDIATE);
		break;

	case CM_INTERMEDIATE_DOCK:
		arrangeResources(IDS_INTERMEDIATE_DOCK, 0, IDC_CURSOR_INTERMEDIATE);
		break;

	case CM_DOCK_FLAGSHIP:
		arrangeResources(IDS_ATTACH_ADMIRAL, 0, IDC_CURSOR_MOVE_ADMIRAL);
		break;

	case CM_NOT_OWNED:
		if (ownsResources())
		{
			desiredOwnedFlags = 0;
			releaseResources();
		}
		break;
	}

	if (newMode > CM_DEFAULT_CURSOR || targetMode == TARGET_OBJECT)
	{
		cursorMode = newMode;
	}
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_AdmiralBar::GetControl (const char *buttonName, void ** ppControl)
{
	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_AdmiralBar::GetToolbar (const char *menuName, struct IToolbar ** ppMenu, enum M_RACE race)
{
	GENRESULT result = GR_GENERIC;
	return result;
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;
}
//----------------------------------------------------------------------------------//
//
bool Menu_AdmiralBar::GetVisible (void)
{
	return !bInvisible;
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_AdmiralBar::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_LOAD_TOOLBAR:
		screenRect = FULLSCREEN->screenRect;
		break;
	case CQE_GAME_ACTIVE:
		if (param==0)
		{
			setCursorMode(CM_NOT_OWNED);
		}
		break;
	}

	// we want to update as long as the game is active and we are visible, except if the cursor state is not CM_NOT_OWNED
	// in that case we always want to update the menu so that we don't trap the cursor
	if (message == CQE_UPDATE)
	{
		if (cursorMode != CM_NOT_OWNED || (CQFLAGS.bGameActive && bInvisible == false))
		{
			if (!CQFLAGS.bGamePaused)
			{
				update(S32(param) >> 10);
				return Frame::Notify(message, param);
			}
		}
	}


	if (CQFLAGS.bGameActive && bInvisible == false)
	switch (message)
	{
		// handle hotkeys
	case CQE_HOTKEY:
		if (bHasFocus) 
		{
			switch ((U32)param)
			{
			case IDH_FLEET_SPORDER_0:
			case IDH_FLEET_SPORDER_1:
			case IDH_FLEET_SPORDER_2:
			case IDH_FLEET_SPORDER_3:
			case IDH_FLEET_SPORDER_4:
			case IDH_FLEET_SPORDER_5:
			case IDH_FLEET_SPORDER_6:
			case IDH_FLEET_SPORDER_7:
			case IDH_FLEET_SPORDER_8:
			case IDH_FLEET_SPORDER_9:
			case IDH_FLEET_SPORDER_10:
			case IDH_FLEET_SPORDER_11:
			case IDH_SPECIAL_ASSAULT:
			case IDH_SPECIAL_AEGIS:
			case IDH_SPECIAL_PROBE:
			case IDH_SPECIAL_CLOAK:
			case IDH_SPECIAL_VAMPIRE:
			case IDH_SPECIAL_TEMPEST:
			case IDH_SPECIAL_STASIS:
			case IDH_SPECIAL_FURYRAM:
			case IDH_SPECIAL_REPEL:
			case IDH_SPECIAL_REPULSOR:
			case IDH_SPECIAL_MIMIC:
			case IDH_SPECIAL_SYNTHESIS:
			case IDH_SPECIAL_MASSDISRUPT:
			case IDH_SPECIAL_DESTABILIZER:
			case IDH_SPECIAL_AUGER:
			case IDH_SPECIAL_SHROUD:
			case IDH_FORM_FLEET:
			case IDH_DISBAND_FLEET:
			case IDH_REPAIR_FLEET:
			case IDH_RESUPPLY_FLEET:
			case IDH_TRANSFER_FLAGSHIP:
			case IDH_SELECT_ALL_FLEET:
			case IDH_RES_KIT_0:
			case IDH_RES_KIT_1:
			case IDH_RES_KIT_2:
			case IDH_RES_KIT_3:
			case IDH_RES_KIT_4:
			case IDH_RES_KIT_5:
			case IDH_RES_KIT_6:
			case IDH_RES_KIT_7:
			case IDH_RES_KIT_8:
			case IDH_RES_KIT_9:
			case IDH_RES_KIT_10:
			case IDH_RES_KIT_11:
			case IDH_RES_KIT_12:
			case IDH_RES_KIT_13:
				handleHotkeyPressed((U32)param);
			}
		}
		break;

	case WM_LBUTTONDOWN:
		if (ownsResources())
		{
			if (onLButtonDown(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam))) == false)
			{
				// eat the message
				return GR_GENERIC;
			}
		}
		break;

	case WM_RBUTTONUP:
		if (ownsResources())
		{
			if (onRButtonUp(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)), msg->wParam) == false)
			{
				// eat the message
				return GR_GENERIC;
			}
		}
		break;

	case WM_LBUTTONUP:
		if (ownsResources())
		{
			if (onLButtonUp(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)), msg->wParam) == false)
			{
				// eat the message
				return GR_GENERIC;
			}
		}
		bLButtonDown = false;
		break;

		/*
	case CQE_ENDFRAME:
		// simply draw our screenrect
		DA::LineDraw(0, screenRect.left, screenRect.top, screenRect.right, screenRect.top, RGB(255,255,0));
		DA::LineDraw(0, screenRect.left, screenRect.top, screenRect.left, screenRect.bottom, RGB(255,255,0));
		DA::LineDraw(0, screenRect.right, screenRect.top, screenRect.right, screenRect.bottom, RGB(255,255,0));
		DA::LineDraw(0, screenRect.left, screenRect.bottom, screenRect.right, screenRect.bottom, RGB(255,255,0));
		break;
		*/
	}

	return Frame::Notify(message, param);
}
//----------------------------------------------------------------------------------//
//
bool Menu_AdmiralBar::onLButtonDown (S32 x, S32 y)
{
	bLButtonDown = true;
	oldMouseX = x; 
	oldMouseY = y;

	return true;
}
//----------------------------------------------------------------------------------//
//
bool Menu_AdmiralBar::onLButtonUp (S32 x, S32 y, U32 wParam)
{
	bLButtonDown = false;

	// do the special weapon thingy, if we can
	bool bQueue = (wParam & MK_SHIFT) != 0;
	if (targetMode == TARGET_ANYWHERE)
	{
		handleSpecialAttackArea(x, y, bQueue);
		return true;
	}
	else if (targetMode == TARGET_OBJECT)
	{
		handleSpecialAttackTarget(x, y, bQueue);
		return true;
	}

	bLButtonDown = false;
	return true;
}
//----------------------------------------------------------------------------------//
//
bool Menu_AdmiralBar::onRButtonUp (S32 x, S32 y, U32 wParam)
{
	// if we're invisible, pass the r button along
	if (bInvisible)
	{
		return true;
	}
	
	// if we don't own the cursor, then pass the message along
	if (ownsResources() == false || cursorMode == CM_NOT_OWNED)
	{
		return true;
	}

	// if we don't have a special ability selected, then pass the message along
	if (testSpecialAbilitySelected() == false)
	{
		return true;
	}

	testCancel();
	return false;

}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::handleHotkeyPressed (U32 buttonID)
{
	CURSOR_MODE newCursorMode = CM_NOT_OWNED;
	targetMode = TARGET_NONE;

	switch (buttonID)
	{
	case IDH_FLEET_SPORDER_0:
		handleSpecialOrder(0);
		break;
	case IDH_FLEET_SPORDER_1:
		handleSpecialOrder(1);
		break;
	case IDH_FLEET_SPORDER_2:
		handleSpecialOrder(2);
		break;
	case IDH_FLEET_SPORDER_3:
		handleSpecialOrder(3);
		break;
	case IDH_FLEET_SPORDER_4:
		handleSpecialOrder(4);
		break;
	case IDH_FLEET_SPORDER_5:
		handleSpecialOrder(5);
		break;
	case IDH_FLEET_SPORDER_6:
		handleSpecialOrder(6);
		break;
	case IDH_FLEET_SPORDER_7:
		handleSpecialOrder(7);
		break;
	case IDH_FLEET_SPORDER_8:
		handleSpecialOrder(8);
		break;
	case IDH_FLEET_SPORDER_9:
		handleSpecialOrder(9);
		break;
	case IDH_FLEET_SPORDER_10:
		handleSpecialOrder(10);
		break;
	case IDH_FLEET_SPORDER_11:
		handleSpecialOrder(11);
		break;
	case IDH_SPECIAL_ASSAULT:
		if (hasTroopship())
		{
			// doing the troopship thing
			targetMode = TARGET_OBJECT;
			unitAbility = USA_ASSAULT;
			newCursorMode = CM_CAPTURE;
		}
		break;

	case IDH_SPECIAL_AEGIS:
		targetMode = TARGET_NONE;
		unitAbility = USA_AEGIS;
		onHandleSpecialAbilityAll();
		return;

	case IDH_SPECIAL_PROBE:
		targetMode = TARGET_ANYWHERE;
		unitAbility = USA_PROBE;
		newCursorMode = CM_PROBE;
		break;

	case IDH_SPECIAL_CLOAK:
		// cloak/uncloak any ships capable
		onHandleCloak();						
		return; 

	case IDH_SPECIAL_VAMPIRE:
		// vampire arc
		targetMode = TARGET_OBJECT;
		unitAbility = USA_VAMPIRE;
		newCursorMode = CM_SPECIAL_ATTACK;
		break;

	case IDH_SPECIAL_TEMPEST:
		targetMode = TARGET_ANYWHERE;
		unitAbility = USA_TEMPEST;
		newCursorMode = CM_SPECIAL_ATTACK;
		break;

	case IDH_SPECIAL_STASIS:
		targetMode = TARGET_ANYWHERE;
		unitAbility = USA_STASIS;
		newCursorMode = CM_SPECIAL_ATTACK;
		break;

	case IDH_SPECIAL_FURYRAM:
		targetMode = TARGET_OBJECT;
		unitAbility = USA_FURYRAM;
		newCursorMode = CM_SPECIAL_ATTACK;
		break;

	case IDH_SPECIAL_REPEL:
		targetMode = TARGET_ANYWHERE;
		unitAbility = USA_REPEL;
		newCursorMode = CM_SPECIAL_ATTACK;
		break;

	case IDH_SPECIAL_REPULSOR:
		targetMode = TARGET_NONE;
		unitAbility = USA_REPULSOR;
		onHandleSpecialAbility();
		return;

	case IDH_SPECIAL_MIMIC:
		unitAbility = USA_MIMIC;
		onHandleSpecialAbilityAll();	// to unmimic anything that is mimic'ed
		targetMode = TARGET_OBJECT;
		unitAbility = USA_MIMIC;
		newCursorMode = CM_MIMIC;
		break;

	case IDH_SPECIAL_SYNTHESIS:
		unitAbility = USA_SYNTHESIS;
		targetMode = TARGET_OBJECT;
		newCursorMode = CM_SPECIAL_ATTACK;
		break;

	case IDH_SPECIAL_DESTABILIZER:
		unitAbility = USA_DESTABILIZER;
		targetMode = TARGET_ANYWHERE;
		newCursorMode = CM_SPECIAL_ATTACK;
		break;

	case IDH_SPECIAL_MASSDISRUPT:
		unitAbility = USA_MASS_DISRUPTOR;
		targetMode = TARGET_OBJECT;
		newCursorMode = CM_SPECIAL_ATTACK;
		break;

	case IDH_SPECIAL_AUGER:
		unitAbility = USA_TRACTOR;
		targetMode = TARGET_OBJECT;
		newCursorMode = CM_SPECIAL_ATTACK;
		break;

	case IDH_SPECIAL_SHROUD:
		if(oldButtonID == IDH_SPECIAL_SHROUD)
		{
			handleMassShroud();
			return;
		}
		else
		{
			unitAbility = USA_MULTICLOAK;
			targetMode = TARGET_OBJECT;
			newCursorMode = CM_SPECIAL_ATTACK;
		}
		break;

	case IDH_FORM_FLEET:
		onCreateFleet();
		break;

	case IDH_DISBAND_FLEET:
		onDisbandFleet();
		setCursorMode(CM_NOT_OWNED);
		break;

	case IDH_REPAIR_FLEET:
		onRepairFleet();
		break;

	case IDH_RESUPPLY_FLEET:
		onResupplyFleet();
		break;

	case IDH_TRANSFER_FLAGSHIP:
		if (hasDockable())
		{
			unitAbility = USA_DOCK;
			targetMode = TARGET_OBJECT;
			newCursorMode = CM_DOCK_FLAGSHIP;
		}
		break;

	case IDH_SELECT_ALL_FLEET:
		onSelectAllFleet();
		break;

	case IDH_RES_KIT_0:
		handleCommandKitRes(0);
		break;
	case IDH_RES_KIT_1:
		handleCommandKitRes(1);
		break;
	case IDH_RES_KIT_2:
		handleCommandKitRes(2);
		break;
	case IDH_RES_KIT_3:
		handleCommandKitRes(3);
		break;
	case IDH_RES_KIT_4:
		handleCommandKitRes(4);
		break;
	case IDH_RES_KIT_5:
		handleCommandKitRes(5);
		break;
	case IDH_RES_KIT_6:
		handleCommandKitRes(6);
		break;
	case IDH_RES_KIT_7:
		handleCommandKitRes(7);
		break;
	case IDH_RES_KIT_8:
		handleCommandKitRes(8);
		break;
	case IDH_RES_KIT_9:
		handleCommandKitRes(9);
		break;
	case IDH_RES_KIT_10:
		handleCommandKitRes(10);
		break;
	case IDH_RES_KIT_11:
		handleCommandKitRes(11);
		break;
	case IDH_RES_KIT_12:
		handleCommandKitRes(12);
		break;
	case IDH_RES_KIT_13:
		handleCommandKitRes(13);
		break;

	default:
//		CQBOMB1("Which special weapon or button?  ID == %d", buttonID);
		break;
	}

	if (targetMode != TARGET_NONE)
	{
		setCursorMode(newCursorMode);
		specialCursor = newCursorMode;
	}

	if (buttonID == oldButtonID)
	{
		numAttacks++;
	}
	else
	{
		numAttacks = 1;
	}
	oldButtonID = buttonID;

	// make sure no button is highlighted
	setFocus(NULL);
}
//-------------------------------------------------------------------
//
void Menu_AdmiralBar::handleSpecialOrder(U32 orderButtonID)
{
	//translate the order into the proper hotkey
	SpecialAttack type = FLEET_MENU->GetSpecialAttackType(orderButtonID);
	switch(type)
	{
	case SA_ASSAULT:
		handleHotkeyPressed(IDH_SPECIAL_ASSAULT);
		break;
	case SA_AEGIS:
		handleHotkeyPressed(IDH_SPECIAL_AEGIS);
		break;
	case SA_PROBE:
		handleHotkeyPressed(IDH_SPECIAL_PROBE);
		break;
	case SA_CLOAK:
		handleHotkeyPressed(IDH_SPECIAL_CLOAK);
		break;
	case SA_VAMPIRE:
		handleHotkeyPressed(IDH_SPECIAL_VAMPIRE);
		break;
	case SA_TEMPEST:
		handleHotkeyPressed(IDH_SPECIAL_TEMPEST);
		break;
	case SA_STASIS:
		handleHotkeyPressed(IDH_SPECIAL_STASIS);
		break;
	case SA_FURYRAM:
		handleHotkeyPressed(IDH_SPECIAL_FURYRAM);
		break;
	case SA_REPEL:
		handleHotkeyPressed(IDH_SPECIAL_REPEL);
		break;
	case SA_REPULSOR:
		handleHotkeyPressed(IDH_SPECIAL_REPULSOR);
		break;
	case SA_MIMIC:
		handleHotkeyPressed(IDH_SPECIAL_MIMIC);
		break;
	case SA_SYNTHESIS:
		handleHotkeyPressed(IDH_SPECIAL_SYNTHESIS);
		break;
	case SA_MASSDISRUPT:
		handleHotkeyPressed(IDH_SPECIAL_DESTABILIZER);
		break;
	case SA_DESTABILIZER:
		handleHotkeyPressed(IDH_SPECIAL_MASSDISRUPT);
		break;
	case SA_SOLARIANCLOAK:
		handleHotkeyPressed(IDH_SPECIAL_CLOAK);
		break;
	case SA_SHROUD:
		handleHotkeyPressed(IDH_SPECIAL_SHROUD);
		break;
	case SA_AUGER:
		handleHotkeyPressed(IDH_SPECIAL_AUGER);
		break;
	}
}
//-------------------------------------------------------------------
//
void Menu_AdmiralBar::insertSortObj (U32 objectID[MAX_SELECTED_UNITS], int size, MPart & part)
{
	int i;
	MPart cmpObj;

	if (part.isValid() == false)
		return;

	for (i = 0; i < MAX_SELECTED_UNITS; i++)
	{
		if (i >= size)
		{
			objectID[i] = part->dwMissionID;
			break;
		}

		cmpObj = OBJLIST->FindObject(objectID[i]);
		if (part->mObjClass > cmpObj->mObjClass)
		{
			// push everyone else down
			int j;
			for (j = size; j > i ; j--)
			{
				objectID[j] = objectID[j-1];
			}

			objectID[i] = part->dwMissionID;
			break;
		}
	}
}
//-------------------------------------------------------------------
//
bool Menu_AdmiralBar::ScreenToPoint(S32 x, S32 y, Vector& vec)
{
	if (y > CAMERA->GetPane()->y1)
		return false;

	vec.x = x;
	vec.y = y;
	vec.z = 0;

	return (CAMERA->ScreenToPoint(vec.x, vec.y, vec.z) != 0);
}
//----------------------------------------------------------------------------------//
//
bool Menu_AdmiralBar::handleSpecialAttackTarget(S32 x, S32 y, bool bQueued)
{
	// is this the assault attack
	if (unitAbility == USA_ASSAULT)
	{
		return handleCapture();
	}
	else if (unitAbility == USA_MIMIC)
	{
		return handleMimic();
	}
	else if (unitAbility == USA_DOCK)
	{
		return handleTransfer();
	}

	// if we don't have the proper cursor state set, then we can't do a special attack
	if (cursorMode <= CM_DEFAULT_CURSOR)
	{
		return false;
	}

	IBaseObject * target = OBJLIST->GetHighlightedList();
	if (target == NULL)
	{
		// no target
		return false;
	}

	// do the special ability, man
	// go through every object selected, see if it matches our special ability
	IBaseObject * obj = OBJLIST->GetSelectedList();
	IBaseObject * sel = NULL;
	MPart part;

	UNIT_SPECIAL_ABILITY usa;
	U32 num = 0;
	bool bSpecialEnabled;
	bool bSendPacket = false;

	OBJPTR<IAttack> attack;
	USR_PACKET<USRSPATTACK> packet;
	Vector vec;

	int i = 0;  

	packet.userBits = bQueued;

	while (obj && num < numAttacks)
	{
		if (obj->QueryInterface(IAttackID, attack))
		{
			attack->GetSpecialAbility(usa, bSpecialEnabled);

			if (usa == unitAbility && bSpecialEnabled)
			{
				// add the ship to the list to use it's special ability
				part = obj;
				insertSortObj(packet.objectID, i++, part);
				bSendPacket = true;
				num++;
			}
		}
		obj = obj->nextSelected;
	}

	if (bSendPacket)
	{
		if ((sel = OBJLIST->FindGroupObject(packet.objectID, i)) != 0)
		{
			packet.objectID[0] = sel->GetPartID();
			i = 1;
		}

		packet.init(i, false);
		packet.targetID = target->GetPartID();
		localSendPacket(&packet);

		ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, vec);

		// find the right sound to play
		if (part.isValid())
		{
			if (MGlobals::IsFlagship(part->mObjClass))
			{
				FLAGSHIPCOMM2(part.obj, part->dwMissionID, attacking, SUB_ATTACK,part.pInit->displayName);
			}
			else if (MGlobals::IsGunboat(part->mObjClass))
			{
				GUNBOATCOMM2(part.obj, part->dwMissionID, specialAttack,SUB_SPECIAL_ATTACK,part.pInit->displayName);
			}
		}

		specialCursor = CM_NOT_OWNED;
		unitAbility = USA_NONE;
		targetMode = TARGET_NONE;
		numAttacks = 0;
		return true;
	}
	return false;
}
//----------------------------------------------------------------------------------//
//
bool Menu_AdmiralBar::handleTransfer (void)
{
	CQASSERT(unitAbility == USA_DOCK);
	CQASSERT(admiral && admiralID);

	IBaseObject * target = OBJLIST->GetHighlightedList();
	if (target == NULL || testOverAttackable(target) == false)
	{
		// no target
		return false;
	}

	MPart flagship = admiral.Ptr();
	VOLPTR(IAdmiral) admiralInterface = admiral.Ptr();
	
	if (admiralInterface == 0)
	{
		return false;
	}

	IBaseObject * dockship = admiralInterface->GetDockship();

	// if the admiral is currently docked, then we must send an undock packet first 
	if (dockship/* && admiralInterface->IsDocked()*/)
	{
		USR_PACKET<USRUNDOCKFLAGSHIP> packet;
		packet.objectID[0] = admiralID;
		packet.objectID[1] = dockship->GetPartID();
		packet.userBits = 0;	// clear out the queue - important
		packet.position.init(admiral.Ptr()->GetGridPosition(), admiral.Ptr()->GetSystemID());
		packet.init(2, false);		// don't track this command
		NETPACKET->Send(HOSTID, 0, &packet);
	}

	// put the packet together and send it
	USR_PACKET<USRDOCKFLAGSHIP> packet;
	packet.objectID[0] = flagship->dwMissionID;
	packet.objectID[1] = target->GetPartID();
	packet.userBits = (dockship) ? 1 : 0;   // very important, don't want to clear the queue if we sent an undock!
	packet.init(2, false);
	
	localSendPacket(&packet);
	ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, target->GetPosition());
	
	if (dockship)
	{
		FLAGSHIPCOMM2(flagship.obj, flagship->dwMissionID, shipleaving,SUB_ADMIRAL_OFF_DECK,flagship.pInit->displayName);
	}
	else
	{
		SHIPCOMM2(flagship.obj, flagship->dwMissionID, move,SUB_MOVE,flagship.pInit->displayName);
	}

	specialCursor = CM_NOT_OWNED;
	unitAbility = USA_NONE;
	targetMode = TARGET_NONE;
	numAttacks = 0;

	return true;
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::handleCommandKitRes(U32 index)
{
	if(admiral)
	{
		VOLPTR(IAdmiral) admiralPtr = admiral;
		if(admiralPtr)
		{
			U32 archId = admiralPtr->GetAvailibleCommandKitID(index);
			if(archId)
			{
				USR_PACKET<USRBUILD> packet;
				packet.cmd = USRBUILD::ADD;
				packet.dwArchetypeID = archId;
				packet.objectID[0] = admiral.Ptr()->GetPartID();
				packet.init(1, false);
				NETPACKET->Send(HOSTID,0,&packet);
			}
		}
	}
}
//----------------------------------------------------------------------------------//
//
bool Menu_AdmiralBar::handleMimic (void)
{
	CQASSERT(unitAbility == USA_MIMIC);

	// if we don't have the proper cursor state set, then we can't do a special attack
	if (cursorMode <= CM_DEFAULT_CURSOR)
	{
		return false;
	}

	IBaseObject * target = OBJLIST->GetHighlightedList();
	if (target == NULL)
	{
		// no target
		return false;
	}

	// do the special ability, man
	// go through every object selected, see if it matches our special ability
	IBaseObject * obj = OBJLIST->GetSelectedList();
	IBaseObject * sel = NULL;
	MPart part;

	UNIT_SPECIAL_ABILITY usa;
	U32 num = 0;
	bool bSpecialEnabled;
	bool bSendPacket = false;

	OBJPTR<IAttack> attack;
	USR_PACKET<USRMIMIC> packet;
	Vector vec;

	int i = 0;  

	while (obj)
	{
		if (obj->QueryInterface(IAttackID, attack))
		{
			attack->GetSpecialAbility(usa, bSpecialEnabled);

			if (usa == unitAbility && bSpecialEnabled)
			{
				// add the ship to the list to use it's special ability
				part = obj;
				insertSortObj(packet.objectID, i++, part);
				bSendPacket = true;
				num++;
			}
		}
		obj = obj->nextSelected;
	}

	if (bSendPacket)
	{
		if ((sel = OBJLIST->FindGroupObject(packet.objectID, i)) != 0)
		{
			packet.objectID[0] = sel->GetPartID();
			i = 1;
		}

		packet.init(i, false);
		packet.targetID = target->GetPartID();
		packet.userBits = 1;
		localSendPacket(&packet);

		specialCursor = CM_NOT_OWNED;
		unitAbility = USA_NONE;
		targetMode = TARGET_NONE;
		numAttacks = 0;
		return true;
	}

	return false;
}
//----------------------------------------------------------------------------------//
//
bool Menu_AdmiralBar::hasDockable (void)
{
	// go through everything in the fleet set and make sure we have at least one gunboat
	if (admiral)
	{
		OBJPTR<IAdmiral> flagship;
		admiral->QueryInterface(IAdmiralID, flagship);

		if (flagship)
		{
			U32 objectIDs[MAX_SELECTED_UNITS];

			int nObjects = flagship->GetFleetMembers(objectIDs);
			for (int i = 0; i < nObjects; i++)
			{
				MPart part = OBJLIST->FindObject(objectIDs[i]);
				if (part.isValid() && MGlobals::IsGunboat(part->mObjClass))
				{
					return true;
				}
			}
		}
	}

	return false;
}
//----------------------------------------------------------------------------------//
//
bool Menu_AdmiralBar::hasTroopship (void)
{
	IBaseObject * obj = OBJLIST->GetSelectedList();
	MPart part;
	
	while (obj)
	{
		part = obj;
		if (part.isValid() && MGlobals::IsTroopship(part->mObjClass))
		{
			return true;
		}

		obj = obj->nextSelected;
	}

	return false;
}
//----------------------------------------------------------------------------------//
//
bool Menu_AdmiralBar::handleCapture (void)
{
	CQASSERT(unitAbility == USA_ASSAULT);

	// if we don't have the proper cursor state set, then we can't do a special attack
	if (cursorMode <= CM_DEFAULT_CURSOR)
	{
		return false;
	}

	IBaseObject * target = OBJLIST->GetHighlightedList();
	if (target == NULL)
	{
		// no target
		return false;
	}

	// do the capture
	IBaseObject * obj = OBJLIST->GetSelectedList();
	MPart part;

	bool bSendPacket = false;

	USR_PACKET<USRCAPTURE> packet;

	while (obj)
	{
		part = obj;
		if (part.isValid())
		{
			if (MGlobals::IsTroopship(part->mObjClass))
			{
				// we know the special ability of a troopship is assault
				part = obj;
				bSendPacket = true;
				break;
			}
		}
		obj = obj->nextSelected;
	}

	if (bSendPacket)
	{
		packet.objectID[0] = part->dwMissionID;

		packet.init(1, false);
		packet.targetID = target->GetPartID();
		localSendPacket(&packet);

		Vector vec = target->GetPosition();
		ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, vec);

		// find the right sound to play
		GUNBOATCOMM2(part.obj, part->dwMissionID, attacking,SUB_ATTACK,part.pInit->displayName);

		specialCursor = CM_NOT_OWNED;
		unitAbility = USA_NONE;
		targetMode = TARGET_NONE;
		numAttacks = 0;
		return true;
	}

	return false;
}
//----------------------------------------------------------------------------------//
//
bool Menu_AdmiralBar::handleProbe (S32 x, S32 y, bool bQueued)
{
	// do the special ability, man
	// go through every object selected, see if it is to be cloaked (or uncloaked)
	IBaseObject * obj = OBJLIST->GetSelectedList();
	IBaseObject * sel = NULL;
	MPart part;

	UNIT_SPECIAL_ABILITY usa;
	U32 num = 0;
	bool bSpecialEnabled;
	bool bSendPacket = false;

	OBJPTR<IAttack> attack;
	USR_PACKET<USRPROBE> packet;
	Vector vec;

	int i = 0;  

	packet.userBits = bQueued;

	const U32 systemID = SECTOR->GetCurrentSystem();
	if (ScreenToPoint(x, y, vec) && (systemID && systemID <= MAX_SYSTEMS))
	{
		packet.position.init(vec, systemID);
	}
	else
	{
		return false;
	}

	while (obj && num < numAttacks)
	{
		if (obj->QueryInterface(IAttackID, attack))
		{
			attack->GetSpecialAbility(usa, bSpecialEnabled);

			if (usa == unitAbility && bSpecialEnabled)
			{
				// add the ship to the list to use it's special ability
				part = obj;
				insertSortObj(packet.objectID, i++, part);
				bSendPacket = true;
				num++;
			}
		}
		obj = obj->nextSelected;
	}

	if (bSendPacket)
	{
		if ((sel = OBJLIST->FindGroupObject(packet.objectID, i)) != 0)
		{
			packet.objectID[0] = sel->GetPartID();
			i = 1;
		}

		packet.init(i, false);
		localSendPacket(&packet);

		ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK,vec);
		playSound(SFXMANAGER->GetGlobalSounds().moveConfirm);

		unitAbility = USA_NONE;
		targetMode = TARGET_NONE;
		numAttacks = 0;
		return true;
	}

	return false;
}
//----------------------------------------------------------------------------------//
//
bool Menu_AdmiralBar::handleSpecialAttackWarpgate (bool bQueued)
{
	// do the special ability, man
	// go through every object selected, see if it is to be cloaked (or uncloaked)
	IBaseObject * target = OBJLIST->GetHighlightedList();
	IBaseObject * obj = OBJLIST->GetSelectedList();
	IBaseObject * sel = NULL;
	MPart part;

	if (target == NULL || obj == NULL)
	{
		return false;
	}

	UNIT_SPECIAL_ABILITY usa;
	U32 num = 0;
	bool bSpecialEnabled;
	bool bSendPacket = false;

	OBJPTR<IAttack> attack;
	USR_PACKET<USRWORMATTACK> packet;
	Vector vec = target->GetPosition();

	int i = 0;  

	packet.userBits = bQueued;
	packet.targetID = target->GetPartID();


	while (obj && num < numAttacks)
	{
		if (obj->QueryInterface(IAttackID, attack))
		{
			attack->GetSpecialAbility(usa, bSpecialEnabled);

			if (usa == unitAbility && bSpecialEnabled)
			{
				// add the ship to the list to use it's special ability
				part = obj;
				insertSortObj(packet.objectID, i++, part);
				bSendPacket = true;
				num++;
			}
		}
		obj = obj->nextSelected;
	}

	if (bSendPacket)
	{
		if ((sel = OBJLIST->FindGroupObject(packet.objectID, i)) != 0)
		{
			packet.objectID[0] = sel->GetPartID();
			i = 1;
		}

		packet.init(i, false);
		localSendPacket(&packet);

		ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, vec);
		playSound(SFXMANAGER->GetGlobalSounds().moveConfirm);

		unitAbility = USA_NONE;
		targetMode = TARGET_NONE;
		numAttacks = 0;
		return true;
	}

	return false;
}
//----------------------------------------------------------------------------------//
//
bool Menu_AdmiralBar::handleSpecialAttackArea (S32 x, S32 y, bool bQueued)
{
	// if we don't have the proper cursor state set, then we can't do a special attack
	if (cursorMode <= CM_DEFAULT_CURSOR)
	{
		return false;
	}

	// are we sending our attack through a jumpgate?
	IBaseObject * highlite = OBJLIST->GetHighlightedList();
	if (highlite && highlite->objClass == OC_JUMPGATE)
	{
		// can we attack through a jumpgate?
		if (canTraverseWarpgate(unitAbility))
		{
			return handleSpecialAttackWarpgate(bQueued);
		}
	}

	// may want to use handleprobe for probe.  talk to Thomas about it if it every becomes an issue


	// do the special ability, man
	// go through every object selected, see if it is to be cloaked (or uncloaked)
	IBaseObject * obj = OBJLIST->GetSelectedList();
	IBaseObject * sel = NULL;
	MPart part;

	UNIT_SPECIAL_ABILITY usa;
	U32 num = 0;
	bool bSpecialEnabled;
	bool bSendPacket = false;

	OBJPTR<IAttack> attack;
	USR_PACKET<USRAOEATTACK> packet;
	Vector vec;

	int i = 0;  

	packet.userBits = bQueued;

	const U32 systemID = SECTOR->GetCurrentSystem();
	if (ScreenToPoint(x, y, vec) && (systemID && systemID <= MAX_SYSTEMS))
	{
		packet.position.init(vec, systemID);
	}
	else
	{
		return false;
	}

	while (obj && num < numAttacks)
	{
		if (obj->QueryInterface(IAttackID, attack))
		{
			attack->GetSpecialAbility(usa, bSpecialEnabled);

			if (usa == unitAbility && bSpecialEnabled)
			{
				// add the ship to the list to use it's special ability
				part = obj;
				insertSortObj(packet.objectID, i++, part);
				bSendPacket = true;
				num++;
			}
		}
		obj = obj->nextSelected;
	}

	if (bSendPacket)
	{
		if ((sel = OBJLIST->FindGroupObject(packet.objectID, i)) != 0)
		{
			packet.objectID[0] = sel->GetPartID();
			i = 1;
		}

		packet.init(i, false);
		localSendPacket(&packet);

		ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK,vec);
		playSound(SFXMANAGER->GetGlobalSounds().moveConfirm);

		unitAbility = USA_NONE;
		targetMode = TARGET_NONE;
		numAttacks = 0;
		return true;
	}

	return false;
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::onHandleSpecialAbility (void)
{
	// go through every object selected, see if it has the special Ability
	IBaseObject * obj = OBJLIST->GetSelectedList();

	UNIT_SPECIAL_ABILITY usa;
	bool bSpecialEnabled;

	OBJPTR<IAttack> attack;
	USR_PACKET<USRSPABILITY> packet;

	while (obj)
	{
		if (obj->QueryInterface(IAttackID, attack))
		{
			attack->GetSpecialAbility(usa, bSpecialEnabled);

			if (usa == unitAbility && bSpecialEnabled)
			{
				// found a ship that can do the special ability, send the packet
				packet.objectID[0] = obj->GetPartID();
				packet.init(1, false);
				packet.userBits = 1;
				localSendPacket(&packet);
				playSound(SFXMANAGER->GetGlobalSounds().moveConfirm);
				break;
			}
		}
		obj = obj->nextSelected;
	}

	targetMode = TARGET_NONE;
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::onHandleSpecialAbilityAll (void)
{
	// go through every object selected, see if it has the special Ability
	IBaseObject * obj = OBJLIST->GetSelectedList();
	IBaseObject * sel = NULL;
	MPart part;

	UNIT_SPECIAL_ABILITY usa;
	bool bSpecialEnabled;
	bool bSendPacket = false;

	OBJPTR<IAttack> attack;
	USR_PACKET<USRSPABILITY> packet;
	int i = 0;

	while (obj)
	{
		if (obj->QueryInterface(IAttackID, attack))
		{
			attack->GetSpecialAbility(usa, bSpecialEnabled);

			if (usa == unitAbility && bSpecialEnabled)
			{
				part = obj;
				insertSortObj(packet.objectID, i++, part);
				bSendPacket = true;
			}
		}
		obj = obj->nextSelected;
	}

	if (bSendPacket)
	{
		if ((sel = OBJLIST->FindGroupObject(packet.objectID, i)) != 0)
		{
			packet.objectID[0] = sel->GetPartID();
			i = 1;
		}

		packet.init(i, false);
		packet.userBits = 1;
		localSendPacket(&packet);
		playSound(SFXMANAGER->GetGlobalSounds().moveConfirm);
	}

	targetMode = TARGET_NONE;
}
//---------------------------------------------------------------------------//
//

U32 shroudPriority(M_OBJCLASS objClass)
{
	if(MGlobals::IsFlagship(objClass))
		return 1;
	else if(MGlobals::IsSupplyShip(objClass))
		return 2;
	else if(MGlobals::IsTroopship(objClass))
		return 3;
	return 10;
};

void Menu_AdmiralBar::handleMassShroud()
{
	IBaseObject * targets[MAX_SELECTED_UNITS];
	U32 numTargets = 0;
	IBaseObject * cloakers[MAX_SELECTED_UNITS];
	U32 numCloakers = 0;
	IBaseObject * obj = OBJLIST->GetSelectedList();
	USR_PACKET<USRCLOAK> cloakPacket;
	U32 numExtraCloakers = 0;

	while(obj && obj->objClass == OC_SPACESHIP)
	{
		OBJPTR<IAttack> attack;
		if (obj->QueryInterface(IAttackID, attack))
		{
			UNIT_SPECIAL_ABILITY usa;
			bool bSpecialEnabled;
			attack->GetSpecialAbility(usa, bSpecialEnabled);
			VOLPTR(ICloak) cloakPtr = obj;
			if(usa == USA_MULTICLOAK && bSpecialEnabled)
			{
				cloakers[numCloakers] = obj;
				++numCloakers;
				//we have a good cloaker
			}
			else if(cloakPtr && cloakPtr->CanCloak())
			{
				if(!(obj->bCloaked))
				{
					cloakPacket.objectID[numExtraCloakers] = obj->GetPartID();
					numExtraCloakers++;
				}
			}
			else
			{
				IBaseObject * insert = obj;
				MPart insPart(insert);
				U32 insPri = shroudPriority(insPart->mObjClass);
				for(U32 i = 0; i< numTargets; ++i)
				{
					MPart repPart(targets[i]);
					U32 repPri = shroudPriority(repPart->mObjClass);
					if(insPri < repPri)
					{
						IBaseObject * tmp = insert;
						insert = targets[i];
						targets[i] = tmp;
						insPri = repPri;
						insPart = insert;
					}
				}
				targets[numTargets] = insert;
				++numTargets;
			}
		}
		else
		{
			IBaseObject * insert = obj;
			MPart insPart(insert);
			U32 insPri = shroudPriority(insPart->mObjClass);
			for(U32 i = 0; i< numTargets; ++i)
			{
				MPart repPart(targets[i]);
				U32 repPri = shroudPriority(repPart->mObjClass);
				if(insPri < repPri)
				{
					IBaseObject * tmp = insert;
					insert = targets[i];
					targets[i] = tmp;
					insPri = repPri;
					insPart = insert;
				}
			}
			targets[numTargets] = insert;
			++numTargets;
		}		obj = obj->nextSelected;
	}

	for(U32 i = 0; i < numCloakers; ++i)
	{
		if(i < numTargets)
		{
			USR_PACKET<USRSPATTACK> packet;
			packet.targetID = targets[i]->GetPartID();
			packet.userBits = 0;
			packet.ctrlBits = 0;
			packet.objectID[0] = cloakers[i]->GetPartID();
			packet.init(1,false);
			localSendPacket(&packet);
		}
		else if(!cloakers[i]->bCloaked)
		{
			cloakPacket.objectID[numExtraCloakers] = cloakers[i]->GetPartID();
			numExtraCloakers++;
		}
	}
	if(numExtraCloakers)
	{
		cloakPacket.init(numExtraCloakers,false);
		localSendPacket(&cloakPacket);
	}
	if(numCloakers)
	{
		// find the right sound to play
		if (admiral)
		{
			MPart part(admiral);
			FLAGSHIPCOMM2(admiral, admiral->GetPartID(), move, SUB_ATTACK,part.pInit->displayName);
		}
	}
	specialCursor = CM_NOT_OWNED;
	unitAbility = USA_NONE;
	targetMode = TARGET_NONE;
	numAttacks = 0;
	oldButtonID = 0;
}
//---------------------------------------------------------------------------//
//
U32 Menu_AdmiralBar::getNumJumps (U32 startSystem, U32 endSystem, U32 playerID)
{
	U32 list[MAX_SYSTEMS];
	U32 pathLength = (startSystem==endSystem) ? 0 : (SECTOR->GetShortestPath(startSystem, endSystem, list, playerID) - 1);

	if (pathLength > MAX_SYSTEMS)
		pathLength = MAX_SYSTEMS;

	return pathLength;
}
//------------------------------------------------------------------------------------------------------//
//
IBaseObject * Menu_AdmiralBar::findClosestPlatform (const MPart & partFrom, const MISSION_DATA::M_CAPS & platCaps)
{
	const U32 systemID = partFrom->systemID;
	if (systemID == 0 || systemID > MAX_SYSTEMS)
	{
		// the admiral is in a bad system, can't find any platforms now
		return NULL;
	}

	IBaseObject * obj = OBJLIST->GetTargetList();
	IBaseObject * closest = NULL;
	U32 closeSector = 0;
	SINGLE closeDist2 = 0;
	bool closeInSystem = false;
	bool haveCloseDist = false;
	U32 closeList[16];
	
	const U32 * caps = (U32 *) & platCaps;
	const U32 playerID = MGlobals::GetPlayerFromPartID(OBJLIST->GetSelectedList()->GetPartID());
	const U32 allyMask = MGlobals::GetAllyMask(playerID);

	// get the transform structure of the admiral
	const TRANSFORM & transform = partFrom.obj->GetTransform();

	while (obj)
	{
		const U32 hisPlayerID = obj->GetPlayerID();

		if ((obj->objClass == OC_PLATFORM) && (((1 << (hisPlayerID-1)) & allyMask) != 0))
		{
			MPart part(obj);

			// do the caps match what we're looking for?
			if (((((U32 *)&part->caps)[0] & caps[0]) != 0) && (part->bReady) && (obj->GetSystemID() <= MAX_SYSTEMS) && (obj->GetSystemID()))
			{
				// we've found a proper plat
				if (closest)
				{
					if (systemID != obj->GetSystemID())
					{
						if (!closeInSystem)
						{
							U32 list[16];
							U32 numJumps = SECTOR->GetShortestPath(systemID, obj->GetSystemID(), list, playerID);
							
							if (numJumps != -1)
							{
								if (numJumps < closeSector) 
								{
									closeDist2 = 0;
									haveCloseDist = false;
									closest = obj;
									closeSector = numJumps;
								}
								else if (numJumps == closeSector)
								{
									OBJPTR<IPlatform> platform;
									IBaseObject * jump1;
									IBaseObject * jump2;
									Vector pos1,pos2;
									U32 jumpPos;
									if (!haveCloseDist)//do I have the distance to my current closest
									{
										//if not compute it
										haveCloseDist = true;
										closest->QueryInterface(IPlatformID,platform);
										CQASSERT(platform);
										
										//find the amount of work the platfrom has left to do and convert it to distance
										jumpPos = 0;
										jump1 = SECTOR->GetJumpgateTo(closeList[jumpPos],closeList[jumpPos+1],transform.translation);
										closeDist2 = (transform.translation-jump1->GetPosition()).magnitude_squared();
										++jumpPos;
										
										while (jumpPos < closeSector-1)
										{
											jump1 = SECTOR->GetJumpgateDestination(jump1);
											jump2 = SECTOR->GetJumpgateTo(closeList[jumpPos],closeList[jumpPos+1],jump1->GetPosition());
											closeDist2 += (jump1->GetPosition() - jump2->GetPosition()).magnitude_squared();
											jump1 = jump2;
											++jumpPos;
										}
										jump1 = SECTOR->GetJumpgateDestination(jump1);
										closeDist2 += (jump1->GetPosition() - obj->GetPosition()).magnitude_squared();
										platform =0;
									}
									obj->QueryInterface(IPlatformID,platform);
									CQASSERT(platform);
							
									jumpPos = 0;
									jump1 = SECTOR->GetJumpgateTo(list[jumpPos],list[jumpPos+1],transform.translation);
									U32 newDist2 = (transform.translation-jump1->GetPosition()).magnitude_squared();
									++jumpPos;

									while(jumpPos < numJumps-1)
									{
										jump1 = SECTOR->GetJumpgateDestination(jump1);
										jump2 = SECTOR->GetJumpgateTo(list[jumpPos],list[jumpPos+1],jump1->GetPosition());
										newDist2 += (jump1->GetPosition() - jump2->GetPosition()).magnitude_squared();
										jump1 = jump2;
										++jumpPos;
									}
									
									jump1 = SECTOR->GetJumpgateDestination(jump1);
									newDist2 += (jump1->GetPosition() - obj->GetPosition()).magnitude_squared();
									if(newDist2 < closeDist2)
									{
										closeDist2 = newDist2;
										haveCloseDist = true;
										closest = obj;
										closeSector = numJumps;
										memcpy(closeList,list,sizeof(list));
									}
								}
							}
						}
					}
					else
					{
						OBJPTR<IPlatform> platform;
						obj->QueryInterface(IPlatformID,platform);
						CQASSERT(platform);
						if (!closeInSystem)
						{
							closeInSystem = true;
							Vector dist = (transform.translation-obj->GetPosition());
							closeDist2 = (dist.x*dist.x)+(dist.y*dist.y)+(dist.z*dist.z);
							closest = obj;
						}
						else
						{
							Vector dist = (transform.translation-obj->GetPosition());
							SINGLE testDist = (dist.x*dist.x)+(dist.y*dist.y)+(dist.z*dist.z);
							if (testDist < closeDist2)
							{
								closeDist2 = testDist;
								closest = obj;
							}
						}
					}

				} 
				else
				{
					OBJPTR<IPlatform> platform;
					obj->QueryInterface(IPlatformID,platform);
					CQASSERT(platform);
					U32 list[16];
					closeSector = 0; 
					closeDist2 = 0;
					if (systemID != obj->GetSystemID())
					{
						U32 testSect = SECTOR->GetShortestPath(systemID, obj->GetSystemID(), list, playerID);
						if (testSect != -1)
						{
							closest = obj;
							closeSector = testSect;
							memcpy(closeList,list,sizeof(list));
							haveCloseDist = false;
						}
					}
					else
					{
						closest = obj;
						closeInSystem = true;
						Vector dist = (transform.translation-obj->GetPosition());
						closeDist2 = (dist.x*dist.x)+(dist.y*dist.y)+(dist.z*dist.z);
					}
				}
			}
		}
		obj = obj->nextTarget;
	}
	
	return closest;
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::onSelectAllFleet(void)
{
	if (admiral)
	{
		OBJLIST->FlushHighlightedList();
		OBJLIST->FlushSelectedList();
		OBJLIST->HighlightObject(admiral.Ptr());
		OBJLIST->SelectHighlightedObjects();
		OBJLIST->FlushHighlightedList();
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::onResupplyFleet (void)
{
	// find the closest supply platform and send the fleet out to it
	MISSION_DATA::M_CAPS caps;
	memset(&caps, 0, sizeof(caps));
	caps.supplyOk = true;

	IBaseObject * plat = NULL;
	MPart part = admiral;

	if (part.isValid())
	{
		plat = findClosestPlatform(part, caps);
	}

	if (plat)
	{
		// now go through everything in the fleet set and tell it to go to the repair platform
		OBJPTR<IAdmiral> flagship;
		admiral->QueryInterface(IAdmiralID, flagship);

		if (flagship)
		{
			USR_PACKET<USRSHIPREPAIR> packet;
			packet.targetID = plat->GetPartID();
			packet.userBits = 0;

			U32 objectIDs[MAX_SELECTED_UNITS];

			int nObjects = flagship->GetFleetMembers(objectIDs);

			if (nObjects > 0)
			{
				for (int i = 0; i < nObjects; i++)
				{
					packet.objectID[i] = objectIDs[i];
				}
				packet.init(nObjects, false);
			}
			else
			{
				// we are only sending the admiral to get supplies/repaired
				packet.objectID[0] = flagship.Ptr()->GetPartID();
				packet.init(1, false);
			}

			NETPACKET->Send(HOSTID, 0, &packet);
		}
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::onRepairFleet (void)
{
	// find the closest repair platform and send the fleet out to it
	MISSION_DATA::M_CAPS caps;
	memset(&caps, 0, sizeof(caps));
	caps.repairOk = true;

	IBaseObject * plat = NULL;
	MPart part = admiral;

	if (part.isValid())
	{
		plat = findClosestPlatform(part, caps);
	}

	if (plat)
	{
		// now go through everything in the fleet set and tell it to go to the repair platform
		OBJPTR<IAdmiral> flagship;
		admiral->QueryInterface(IAdmiralID, flagship);

		if (flagship)
		{
			USR_PACKET<USRSHIPREPAIR> packet;
			packet.targetID = plat->GetPartID();
			packet.userBits = 0;

			U32 objectIDs[MAX_SELECTED_UNITS];

			int nObjects = flagship->GetFleetMembers(objectIDs);

			if (nObjects > 0)
			{
				for (int i = 0; i < nObjects; i++)
				{
					packet.objectID[i] = objectIDs[i];
				}
				packet.init(nObjects, false);
			}
			else
			{
				// we are only sending the admiral to get supplies/repaired
				packet.objectID[0] = flagship.Ptr()->GetPartID();
				packet.init(1, false);
			}

			NETPACKET->Send(HOSTID, 0, &packet);
		}
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::onDisbandFleet (void)
{
	// disband a fleet by sending a fleet define packet with only the admiral's ID
	CQASSERT(admiral && admiralID);
	
	USR_PACKET<FLEETDEF_PACKET> packet;
	
	packet.objectID[0] = admiralID;
	packet.userBits = 0;
	packet.init(1, false);
	localSendPacket(&packet);
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::onCreateFleet (void)
{
	// if the admiral pointer is bad, don't continue
	if (admiral.Ptr() == 0 || admiralID == 0)
	{
		return;
	}

	// find an admiral and set it's fleet
	IBaseObject * obj = OBJLIST->GetSelectedList();
	MPart part;

	USR_PACKET<FLEETDEF_PACKET> packet;
	U32 numUnits = 0;

	// add the admiral and the admiral's dockship to the packet first
	packet.objectID[numUnits++] = admiralID;
	
	U32 dockshipID = 0;
	
	VOLPTR(IAdmiral) flagship = admiral;
	if (flagship)
	{
		IBaseObject * dockship = flagship->GetDockship();
		if (dockship)
		{
			dockshipID = dockship->GetPartID();
			packet.objectID[numUnits++] = dockshipID;
		}
	}

	// now go through every other selected object and add it to our list
	while (obj && numUnits < MAX_SELECTED_UNITS)
	{
		if (obj->objClass == OC_SPACESHIP)
		{
			part = obj;
			if (MGlobals::IsFlagship(part->mObjClass) == 0)
			{
				if (part->dwMissionID != dockshipID)
				{
					packet.objectID[numUnits++] = part->dwMissionID;
				}
			}
		}

		obj = obj->nextSelected;
	}

	packet.userBits = 0;
	packet.init(numUnits, false);
	localSendPacket(&packet);
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::onHandleCloak (void)
{
	bool bCloakStatusSet = false;
	bool bCloakState = false;

	// go through every object selected, see if it is to be cloaked (or uncloaked)
	IBaseObject * obj = OBJLIST->GetSelectedList();
	IBaseObject * sel = NULL;
	MPart part;

	bool bSendPacket = false;

	OBJPTR<IAttack> attack;
	USR_PACKET<USRCLOAK> packet;
	int i = 0;  

	while (obj)
	{
		part = obj;

		if (part->caps.cloakOk)
		{
			if (bCloakStatusSet == false)
			{
				bCloakState = (obj->bCloaked != 0);
				bCloakStatusSet = true;
			}

			if ((obj->bCloaked != 0) == (bCloakState != 0))
			{
				// add the ship to the list to be cloaked/uncloaked
				part = obj;
				insertSortObj(packet.objectID, i++, part);
				bSendPacket = true;
			}
		}

		obj = obj->nextSelected;
	}

	if (bSendPacket)
	{
		if ((sel = OBJLIST->FindGroupObject(packet.objectID, i)) != 0)
		{
			packet.objectID[0] = sel->GetPartID();
			i = 1;
		}

		packet.init(i, false);
		packet.userBits = 1;
		localSendPacket(&packet);
		playSound(SFXMANAGER->GetGlobalSounds().moveConfirm);

		unitAbility = USA_NONE;
		targetMode = TARGET_NONE;
		numAttacks = 0;
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::findLowestAdmiral (void)
{
	IBaseObject * obj = OBJLIST->GetSelectedList();
	MPart flagship = NULL;
	OBJPTR<IAdmiral> admiralObj;
	MPart part;

	admiralKey = 10000000; // much greater than highest value
	admiralID = 0;
	admiral = NULL;

	while (obj)
	{
		part = obj;
		flagship = NULL;

		if (part.isValid())
		{
			if (MGlobals::IsFlagship(part->mObjClass))
			{
				// this object is the admiral
				flagship = part;
			}
			else if (part->admiralID)
			{
				// this object contains the admiral
				 flagship = OBJLIST->FindObject(part->admiralID);
			}

			if (flagship && flagship.isValid())
			{
				flagship.obj->QueryInterface(IAdmiralID, admiralObj);
				CQASSERT(admiralObj);

				U32 tmpKey = admiralObj->GetAdmiralHotkey();
				if (tmpKey < admiralKey)
				{
					admiralKey = tmpKey;
					admiralObj->QueryInterface(IAdmiralID, admiral, NONSYSVOLATILEPTR);
					admiralID = flagship->dwMissionID;
				}
			}
		}
		obj = obj->nextSelected;
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::update (S32 dt)
{
	if (!bInvisible)
	{
		updateCursor();

		// what is the lowest key admiral you have selected?
		findLowestAdmiral();
	}
//	else if (ownsResources() && cursorMode == CM_NOT_OWNED)
//	{
//		setCursorMode(CM_NOT_OWNED);
//	}
	else if (cursorMode != CM_NOT_OWNED)
	{
		// we don't have the cursor anymore, give it up
		setCursorMode(CM_NOT_OWNED);
	}
}
//----------------------------------------------------------------------------------//
//
bool Menu_AdmiralBar::testOverAttackable (MPart hTarget)
{
	CQASSERT(hTarget.isValid());


	const bool overrideMod = (HOTKEY->GetVkeyState(VK_LCONTROL)||HOTKEY->GetVkeyState(VK_RCONTROL));
	const U32 playerID = MGlobals::GetThisPlayer();

	// are we going to attach an admiral?
	if (unitAbility == USA_DOCK)
	{
		// can only transfer to a ship that is within your own fleet
		if (hTarget->fleetID == admiralID)
		{
			// make sure the target is the same player as us
			return hTarget->caps.moveOk &&  MGlobals::IsGunboat(hTarget->mObjClass) && hTarget->playerID == playerID;
		}
		else
		{
			return false;
		}
	}

	// are we looking for any friendly spaceship?
	if (unitAbility == USA_SYNTHESIS || unitAbility == USA_MULTICLOAK)
	{
		return hTarget.obj->objClass == OC_SPACESHIP && MGlobals::AreAllies(hTarget->playerID, playerID);
	}

	// we are looking for any gunboat
	if (unitAbility == USA_MIMIC)
	{
		return hTarget.obj->objClass == OC_SPACESHIP;
	}

	if ((hTarget.obj->objClass & CF_PLAYERALIGNED) != 0 && 
		(hTarget->playerID==0 || overrideMod || MGlobals::AreAllies(hTarget->playerID,playerID)==0))
	{
		// if we are in assault mode, then check if we are able to capture the target
		if (unitAbility == USA_ASSAULT)
		{
			// get one of the troopships selected and see if we can take over the object
			IBaseObject * obj = OBJLIST->GetSelectedList();
			MPart part;

			while (obj)
			{
				part = obj;
				if (part.isValid())
				{
					if (MGlobals::IsTroopship(part->mObjClass))
					{
						return MGlobals::CanTroopship(playerID, part->dwMissionID, hTarget->dwMissionID);
					}
				}
				obj = obj->nextSelected;
			}
		}

		return true;
	}

	return false;
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::updateCursor (void)
{
	if (cursorMode != CM_NOT_OWNED)
	{
		// if we can fire anywhere, than set the cursor
		// if we need a target, check if we are over an enemy, then set the cursor
		if (targetMode == TARGET_ANYWHERE)
		{
			// are we over a jumpgate?  We have a special attack cursor for that
			IBaseObject * highlite = OBJLIST->GetHighlightedList();
			if (highlite && highlite->objClass == OC_JUMPGATE)
			{
				// can we attack through a jumpgate?
				if (canTraverseWarpgate(unitAbility))
				{
					setCursorMode(CM_SPECIAL_ATTACKGATE);
					return;
				}
			}
		
			setCursorMode(CM_SPECIAL_ATTACK);
		}
		else if (targetMode == TARGET_OBJECT)
		{
			// make sure we are over a targetable enemy object
			MPart hTarget = OBJLIST->GetHighlightedList();
			if (hTarget.isValid())
			{
				// is target attackable?
				if (testOverAttackable(hTarget))
				{
					setCursorMode(specialCursor);
					return;
				}
				else if (unitAbility == USA_ASSAULT)
				{
					setCursorMode(CM_CAPTURE_DENIED);
					return;
				}
			}

			// if we've gotten here, than we are not over a viable target so display the intermediate cursor
			if (unitAbility == USA_DOCK)
			{
				setCursorMode(CM_INTERMEDIATE_DOCK);
			}
			else
			{
				setCursorMode(CM_INTERMEDIATE_CURSOR);
			}
		}
		else if (targetMode == TARGET_NONE)
		{
			setCursorMode(CM_NOT_OWNED);
		}
	}
	else if (ownsResources())
	{
		// why would we possibly be owning the resources here?
		setCursorMode(CM_NOT_OWNED);
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_AdmiralBar::setStateInfo (void)
{
	if (childFrame)
		childFrame->setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_AdmiralBar::init (void)
{
	parent = FULLSCREEN;
	lateInitialize(FULLSCREEN->GetBase());
	parent->SetCallbackPriority(this, eventPriority);
	initializeFrame(NULL);

	// want the res priority to be one higher than ObjComm
	resPriority = RES_PRIORITY_MEDIUM + 1;
	cursorID = IDC_CURSOR_DEFAULT;
}
//--------------------------------------------------------------------------//
//
struct _admiralBar : GlobalComponent
{
	Menu_AdmiralBar * menu;

	virtual void Startup (void)
	{
		ADMIRALBAR  = menu = new DAComponent<Menu_AdmiralBar>; 
		AddToGlobalCleanupList(&ADMIRALBAR);
	}

	virtual void Initialize (void)
	{
		menu->init();
	}
};
//--------------------------------------------------------------------------//
//
static _admiralBar admiralBar;

//--------------------------------------------------------------------------//
//-----------------------------End Menu_Pause.cpp---------------------------//
//--------------------------------------------------------------------------//