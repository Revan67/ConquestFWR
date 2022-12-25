//--------------------------------------------------------------------------//
//                                                                          //
//                                  Menu.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/menu.cpp 133   5/04/01 2:59p Tmauer $

		                 Resource manager for the menu

*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>

#include "Menu.h"
#include "UserDefaults.h"
#include "StatusBar.h"
#include "resource.h"
#include "Hotkeys.h"
#include "DBHotkeys.h"
#include "EventPriority.h"
#include "NetBuffer.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "Frame.h"
#include "Camera.h"
#include "NetPacket.h"
#include "ScrollingText.h"
#include "Mission.h"

#include <SearchPath.h>
#include <IConnection.h>
#include <HeapObj.h>
#include <EventSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <TSmartPointer.h>
#include <FileSys.h>
#include <WindowManager.h>
#include <RendPipeline.h>
#include <IMaterialManager.h>

#include <stdio.h>
#include <commctrl.h>
#include <ZMouse.h>
#include <AfxRes.h>
#include <math.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#define MENU_PRIORITY RES_PRIORITY_HIGH
#define MAX_GAME_SPEED 20
#define DATA_PATH "DataPath"
#define DELAY_TIME  500				// in msecs

Frame * __stdcall CreateMenuPause (int pauseState);		// Menu_pause.cpp 0=local, 1 = net_congestion, 2 = remote pause
void __stdcall SetGamma (bool b3DEnabled);
IFileSystem * __stdcall CreateLogOfFile (IFileSystem * pFile, const char *logName);
U32 __stdcall CreateOptionsMenu (const bool bFocusing = true);


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE MenuResource : public IMenuResource, 
								  IEventCallback,
								  IResourceClient
{
	HMENU hMenu;	
	U32 eventHandle, statusHandle;		// handles to callbacks
	U32 bVisible:1;
	U32 bDefaultRecorded:1;
	U32 bRemotePaused:2;	// can be remote pause or blocked state
	U32 bLocalPaused:1;
	U32 bHasFocus:1;
	DWORD dwMenuHeight;
	U32 wheelMsg;
	HWND hPartsDlg;
	U32 delayCounter;		// for visibility test
	bool bMissionEnding;

	Frame * pPauseDlg;

	// path preference
	C8 dataPath[MAX_PATH];
	COMPTR<ISearchPath> searchPath;


	BEGIN_DACOM_MAP_INBOUND(MenuResource)
	DACOM_INTERFACE_ENTRY(IMenuResource)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	END_DACOM_MAP()

	MenuResource (void);

	~MenuResource (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* IMenuResource methods */
	
	virtual void __cdecl Redraw (void);

	DEFMETHOD_(HMENU,GetSubMenu) (U32 nPos);

	DEFMETHOD_(void,ForceDraw) (void);

	DEFMETHOD_(BOOL32,EnableMenuItem) (U32 nPos, U32 flags);

	DEFMETHOD_(void,InitPreferences) (void);		// reset the menu state

	DEFMETHOD_(HMENU,GetMainMenu) (void) const;

	DEFMETHOD_(void,SetMainMenu) (HMENU hMenu);

	virtual void __stdcall AddToPartMenu (const char *szPartName, U32 identifier);

	virtual void __stdcall RemoveFromPartMenu (U32 identifier);

	virtual void __stdcall RemoveAllFromPartMenu (void);

	/* IResourceClient methods */

	DEFMETHOD_(BOOL32,Notify) (struct IResource *res, U32 message, void *parm=0);

	/* MenuResource methods */

	void OnNoOwner (void)
	{
	}

	void TogglePreferences (U32 id);

	void quickLoad();

	void quickSave();

	bool updateLoadPath (void);

	void enableVolatileItems (bool bEnable);

	void enableCheatItems (bool bEnable);

	void toggleNetPause (int bPause);

	void toggleLocalPause (void);

	void updateMenuVisibility (U32 dt);

	void deletePauseMenu (void)
	{
		if (pPauseDlg)
		{
			pPauseDlg->parent->PostMessage(CQE_DELETE_HOTRECT, static_cast<BaseHotRect *>(pPauseDlg));
			pPauseDlg = 0;
		}
	}

	inline bool testToggleMode (void)
	{
		if (PLAYERID && NETPACKET->GetNumHosts() > 1)
		{
#ifdef _DEBUG
			CQERROR0("You must terminate the network session first.");
#endif
			return false;
		}
		return true;
	}

	void sendPartNotification (void)
	{
		HWND hList = GetDlgItem(hPartsDlg, IDC_LIST1);

		S32 sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
		if (sel >= 0)
		{
			S32 id = SendMessage(hList, LB_GETITEMDATA, sel, 0);

			if (EVENTSYS)
				EVENTSYS->Send(CQE_PART_SELECTION, (void*)id);
		}
	}

	void gotoPart (void)
	{
		HWND hList = GetDlgItem(hPartsDlg, IDC_LIST1);

		S32 sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
		if (sel >= 0)
		{
			S32 id = SendMessage(hList, LB_GETITEMDATA, sel, 0);

			if (EVENTSYS)
				EVENTSYS->Send(CQE_GOTO_PART, (void*)id);
		}
	}

	void initPreferences (void);

	void missingDirDialog (const char * name, bool bFatal, const char *prependName=0);

	static BOOL CALLBACK EditPathDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
	static BOOL CALLBACK NetStressDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
	static BOOL CALLBACK GameSpeedDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
	static BOOL CALLBACK partDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
};
//--------------------------------------------------------------------------//
//
MenuResource::MenuResource (void) 
{
	bHasFocus = 1;
}
//--------------------------------------------------------------------------//
//
MenuResource::~MenuResource (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (FULLSCREEN && FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);
	if (STATUS && STATUS->QueryOutgoingInterface("IResourceClient", connection) == GR_OK)
		connection->Unadvise(statusHandle);

	pPauseDlg = 0;	// has deleted itself by this point
	SendMessage(hPartsDlg, WM_CLOSE, 0, -1);
	DestroyWindow(hPartsDlg);
	hPartsDlg = 0;
	SetMenu(hMainWindow, 0);
	DestroyMenu(hMenu);
	hMenu = 0;
}
//--------------------------------------------------------------------------//
//
void MenuResource::Redraw (void)
{
}
//--------------------------------------------------------------------------//
//
void MenuResource::updateMenuVisibility (U32 dt)
{
	if (CQFLAGS.bNoGDI == 0)
	{
		if (GetAsyncKeyState(VK_MENU) & 0x8000)
		{
			if (delayCounter < DELAY_TIME)
				delayCounter+=dt;
		}
		else
			delayCounter = 0;

		if (CQFLAGS.bFullScreen == 0)
		{
			// enable the menu
			if (bVisible == 0 && delayCounter >= DELAY_TIME)
			{
				SetMenu(hMainWindow, hMenu);
				bVisible=1;
				WM->SetWindowPos(SCREENRESX, SCREENRESY, 0);
				DBHOTKEY->SystemMessage((S32)hMainWindow,    
                                         WM_SYSKEYUP,    
                                         VK_MENU,
										 (1 << 24) | (1 << 31));		// right alt, release event
				DBHOTKEY->SystemMessage((S32)hMainWindow,    
                                         WM_SYSKEYUP,    
                                         VK_MENU,
										 (1 << 31));		// left alt, release event
			}
			else // disable the menu
			if (bVisible && delayCounter < DELAY_TIME)
			{
				SetMenu(hMainWindow, 0);
				bVisible=0;
				WM->SetWindowPos(SCREENRESX, SCREENRESY, 0);
			}
		}
		else // full screen mode
		{
			// enable the menu
			if (bVisible == 0 && delayCounter >= DELAY_TIME)
			{
				SetMenu(hMainWindow, hMenu);
				bVisible=1;
				DBHOTKEY->SystemMessage((S32)hMainWindow,    
                                         WM_SYSKEYUP,    
                                         VK_MENU,
										 (1 << 24) | (1 << 31));		// right alt, release event
				DBHOTKEY->SystemMessage((S32)hMainWindow,    
                                         WM_SYSKEYUP,    
                                         VK_MENU,
										 (1 << 31));		// left alt, release event
			}
			else // disable the menu
			if (bVisible && delayCounter < DELAY_TIME)
			{
				SetMenu(hMainWindow, 0);
				bVisible=0;
			}
		}
	}
	else
	{
		if (bVisible)
		{
			SetMenu(hMainWindow, 0);
			bVisible=0;
		}
	}
}
//--------------------------------------------------------------------------//
//
HMENU MenuResource::GetSubMenu (U32 nPos)
{
	return ::GetSubMenu(hMenu, nPos);
}
//--------------------------------------------------------------------------//
//
void MenuResource::ForceDraw (void)
{
	if (bVisible)
		DrawMenuBar(hMainWindow);
}
//--------------------------------------------------------------------------//
//
BOOL32 MenuResource::EnableMenuItem (U32 nPos, U32 flags)
{
	return ::EnableMenuItem(hMenu, nPos, flags);
}
//--------------------------------------------------------------------------//
//
HMENU MenuResource::GetMainMenu (void) const
{
	return hMenu;
}
//--------------------------------------------------------------------------//
//
void MenuResource::SetMainMenu (HMENU _hMenu)
{
	hMenu = _hMenu;
	if (bVisible)
	{
		SetMenu(hMainWindow, hMenu);
		DrawMenuBar(hMainWindow);
	}
}
//--------------------------------------------------------------------------//
//
void MenuResource::AddToPartMenu (const char *szPartName, U32 identifier)
{
	HWND hList = GetDlgItem(hPartsDlg, IDC_LIST1);

#ifdef _DEBUG
	// make sure we don't get a duplicate
	if (SendMessage(hList, LB_FINDSTRINGEXACT, -1, (LPARAM) szPartName) != LB_ERR)
		CQERROR1("Duplicate part '%s' added to part menu.", szPartName);
#endif

	S32 index = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM) szPartName);
	CQASSERT(index >= 0);

	SendMessage(hList, LB_SETITEMDATA, index, (LPARAM) identifier);

	if (SendMessage(hList, LB_GETCOUNT, 0, 0) == 1)		// first entry
	{
		EnableWindow(GetDlgItem(hPartsDlg, IDC_BUTTON1), 1);
		EnableWindow(GetDlgItem(hPartsDlg, IDC_BUTTON2), 1);
		SendMessage(hList, LB_SETCURSEL, 0, 0);
	}
}
//--------------------------------------------------------------------------//
//
void MenuResource::RemoveFromPartMenu (U32 identifier)
{
	HWND hList = GetDlgItem(hPartsDlg, IDC_LIST1);
	int count = SendMessage(hList, LB_GETCOUNT, 0, 0);
	bool bSingleItem = (count==1);

	while (count-- > 0)
	{
		U32 id = SendMessage(hList, LB_GETITEMDATA, count, 0);

		if (id == identifier)
		{
			SendMessage(hList, LB_DELETESTRING, count, 0);
			if (bSingleItem)
			{
				EnableWindow(GetDlgItem(hPartsDlg, IDC_BUTTON1), 0);
				EnableWindow(GetDlgItem(hPartsDlg, IDC_BUTTON2), 0);
			}
			break;
		}
	}
}
//--------------------------------------------------------------------------//
//
void MenuResource::RemoveAllFromPartMenu (void)
{
	SendMessage(GetDlgItem(hPartsDlg, IDC_LIST1), LB_RESETCONTENT, 0, 0);
	EnableWindow(GetDlgItem(hPartsDlg, IDC_BUTTON1), 0);
	EnableWindow(GetDlgItem(hPartsDlg, IDC_BUTTON2), 0);
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT MenuResource::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	if (message && message == wheelMsg)
	{
		msg->wParam = msg->wParam<<16;
		EVENTSYS->Send(WM_MOUSEWHEEL, param);
	}

	switch (message)
	{
	case WM_SYSKEYUP:
		delayCounter=0;
		break;

	case CQE_SET_FOCUS:
		bHasFocus = 1;
		break;
	case CQE_KILL_FOCUS:
		bHasFocus = 0;
		break;
	case CQE_MISSION_ENDING:		// note: this assumes that we will be breaking the net connection
		bMissionEnding = true;
		CQFLAGS.bGamePaused = 0;
		EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
		bLocalPaused = 0;
		deletePauseMenu();
		break;

	case CQE_GAME_ACTIVE:
		bMissionEnding = false;
		break;

	case CQE_ENTERING_INGAMEMODE:
		if (DEFAULTS->GetDefaults()->bEditorMode)
			SendMessage(hMainWindow, WM_COMMAND, IDM_EDITOR_MODE, 0);
		if (DEFAULTS->GetDefaults()->bEditorMode)
			CQBOMB0("Editor Mode must be off.");
		DEFAULTS->GetDefaults()->bEditorMode = 0;
		EnableMenuItem(MENUPOS_FILE, MF_ENABLED|MF_BYPOSITION);
		break;

	case CQE_LEAVING_INGAMEMODE:
		EnableMenuItem(MENUPOS_FILE, MF_GRAYED|MF_BYPOSITION);
		break;

	case CQE_PAUSE_WARNING:
		if (bLocalPaused)
		{
			SCROLLTEXT->SetText(IDS_MAX_PAUSE_EXCEEDED);
		}
		break;
		
	case CQE_NETPAUSED:
		if (bMissionEnding==false)				// don't bring up pause menu while in that nebulas shutdown time
			toggleNetPause(int(param));
		break;
	case CQE_NETSTARTUP:
		enableVolatileItems(false);
		break;
	case CQE_NETSHUTDOWN:
		bLocalPaused = 0;
		enableVolatileItems(true);
		break;

	case CQE_HOTKEY:
		switch ((U32) param)
		{
		case IDH_TOGGLE_SCREENMODE:
			//if (CQFLAGS.bWindowModeAllowed)
			if (true)
			{
				if (DEFAULTS->GetDefaults()->bWindowMode)
				{
					DEVMODE dm;
					dm.dmSize = sizeof(dm);
					dm.dmPelsHeight = SCREENRESY;
					dm.dmPelsWidth = SCREENRESX;
					dm.dmFields = DM_PELSHEIGHT | DM_PELSWIDTH;
					ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
					WM->SetWindowPos(SCREENRESX, SCREENRESY, WMF_FULL_SCREEN);	
					PIPE->destroy_buffers();
				}
				else
				{
					ChangeDisplaySettings(NULL, 0);
					WM->SetWindowPos(SCREENRESX, SCREENRESY, WMF_CENTER);	
					PIPE->destroy_buffers();
				}
				DEFAULTS->GetDefaults()->bWindowMode = !DEFAULTS->GetDefaults()->bWindowMode;
			}
			else
			{
				if (CQFLAGS.bFullScreen)
				{
					PIPE->set_pipeline_state(RP_BUFFERS_FULLSCREEN,0);
					if (PIPE->create_buffers(hMainWindow,SCREENRESX,SCREENRESY) == GR_OK)
					{
						SetMenu(hMainWindow, hMenu);
						bVisible=1;
						WM->SetWindowPos(SCREENRESX, SCREENRESY, WMF_CENTER);	
						CQFLAGS.bFullScreen = 0;
					}
					else
					{
						SetMenu(hMainWindow, 0);
						bVisible=0;
						WM->SetWindowPos(SCREENRESX, SCREENRESY, WMF_FULL_SCREEN);
						PIPE->set_pipeline_state(RP_BUFFERS_FULLSCREEN,1);
						PIPE->create_buffers(hMainWindow,SCREENRESX,SCREENRESY);
						CQFLAGS.bFullScreen = 1;
					}
				}
				else
				{
					SetMenu(hMainWindow, 0);
					bVisible=0;
					WM->SetWindowPos(SCREENRESX, SCREENRESY, WMF_FULL_SCREEN);
					PIPE->set_pipeline_state(RP_BUFFERS_FULLSCREEN,1);
					if (PIPE->create_buffers(hMainWindow,SCREENRESX,SCREENRESY) == GR_OK)
					{
						CQFLAGS.bFullScreen = 1;
					}
					else
					{
						PIPE->set_pipeline_state(RP_BUFFERS_FULLSCREEN,0);
						PIPE->create_buffers(hMainWindow,SCREENRESX,SCREENRESY);
						SetMenu(hMainWindow, hMenu);
						bVisible=1;
						WM->SetWindowPos(SCREENRESX, SCREENRESY, 0);	
						CQFLAGS.bFullScreen = 0;
					}
				}
				SetGamma(CQFLAGS.bFullScreen != 0);
				RestoreAllSurfaces();
			}
			break;

		case IDH_LOAD_MENU:
			if (bHasFocus && CQFLAGS.bGameActive && PLAYERID==0)
			{
				CreateMenuLoadSave(true, MISSION->IsSinglePlayerGame());
			}
			break;

		case IDH_SAVE_MENU:
			if (bHasFocus && CQFLAGS.bGameActive && (PLAYERID==0 || CQFLAGS.bLimitMapSettings==0))
			{
				CreateMenuLoadSave(false, MISSION->IsSinglePlayerGame());
			}
			break;
		
		case IDH_QUICK_LOAD:
			if (bHasFocus && CQFLAGS.bGameActive && PLAYERID==0)
				quickLoad();
			break;

		case IDH_QUICK_SAVE:
			if (bHasFocus && CQFLAGS.bGameActive && (PLAYERID==0 || CQFLAGS.bLimitMapSettings==0))
				quickSave();
			break;

		case IDH_PAUSE_GAME:
			if (bHasFocus && bMissionEnding == false && (bLocalPaused || DEFAULTS->GetDefaults()->bSpectatorModeOn == false))
			{
				toggleLocalPause();
			}
			break;

		case IDH_GAME_OPTIONS:
			{
				if (bHasFocus && CQFLAGS.bGameActive)
				{
					U32 igResult = CreateMenu_igoptions();
					if (igResult)
					{
						// player wants to resign
						if(igResult == 1)
							EVENTSYS->Send(CQE_PLAYER_RESIGN, 0);
						else
							EVENTSYS->Send(CQE_PLAYER_QUIT, 0);
					}
				}
				break;
			}
		} // end switch (param)
		break; // end case CQE_HOTKEY
	
	case CQE_DEBUG_HOTKEY:
		if (CQFLAGS.bNoGDI==0)
		switch ((U32) param)
		{
		case IDH_EDITOR_MODE:
			SendMessage(hMainWindow, WM_COMMAND, IDM_EDITOR_MODE, 0);
			break;

		case IDH_GAME_SPEED:
			SendMessage(hMainWindow, WM_COMMAND, IDM_GAME_SPEED, 0);
			break;

		case IDH_SHOWPARTS:
			SendMessage(hMainWindow, WM_COMMAND, IDM_SHOWPARTS, 0);
			break;

		} // end switch (param)
		break; // end case CQE_DEBUG_HOTKEY

		
	case CQE_UPDATE:
		updateMenuVisibility(U32(param)>>10);
		break;
	
	case WM_CLOSE:
		if (bDefaultRecorded==0)
		{
//			USER_DEFAULTS *defaults = DEFAULTS->GetDefaults();

			//defaults->bWindowMode = (CQFLAGS.bFullScreen==0);
			bDefaultRecorded=1;
		}
		break;

	case WM_MENUSELECT:
		if (HIWORD(msg->wParam) == 0xFFFF)
			STATUS->ReleaseOwnership(this);
		else
		if (HIWORD(msg->wParam) & MF_HILITE)
		{
			UINT uItem;

			uItem = LOWORD(msg->wParam);

			if (HIWORD(msg->wParam) & MF_POPUP)
			switch (uItem)
			{
			case MENUPOS_FILE:
				uItem = IDS_MENUPOS_FILE;
				break;
			case MENUPOS_PREF:
				uItem = IDS_MENUPOS_PREF;
				break;
			case MENUPOS_VIEW:
				uItem = IDS_MENUPOS_VIEW;
				break;
			case MENUPOS_EDITOR:
				uItem = IDM_EDITOR_MODE;
				break;
			case MENUPOS_MRU_MISSION:
				uItem = IDS_MENUPOS_MRU_MISSION;
				break;
			default:
				uItem = 0;
				break;
			}

			if (uItem && STATUS->GetOwnership(this, MENU_PRIORITY))
				STATUS->SetText(uItem);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(msg->wParam))
		{
		case IDM_EDIT_PATH:
			if (DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG3), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) EditPathDlgProc, (LPARAM) this))
			{
				DEFAULTS->SetStringInRegistry(DATA_PATH, dataPath);
				while (updateLoadPath() == 0)
					;
			}
			break;

		case IDM_GAME_SPEED:
			{
				int result = DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG6), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) GameSpeedDlgProc, (LPARAM) this);

				if (result >= 0)
				{
					DEFAULTS->GetDefaults()->gameSpeed = result-MAX_GAME_SPEED;
				}
			}
			break;

		case IDM_NET_STRESS:
			{
				DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG1), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) NetStressDlgProc, (LPARAM) this);
			}
			break;
			
		case ID_APP_EXIT:
			SendMessage(msg->hwnd, WM_CLOSE, 0, 0);
			break;
			
		case IDM_EDITOR_MODE:
			if (CQFLAGS.bNoGDI==0)
			{
				if (CQFLAGS.bGameActive==0 && DEFAULTS->GetDefaults()->bEditorMode==0)
				{
#ifdef _DEBUG
					if (CQFLAGS.bRuseActive == 0)
						CQERROR0("Editor mode not allowed here.");
#endif
				}
				else
				if (testToggleMode())
				{
					TogglePreferences(LOWORD(msg->wParam));
					DEFAULTS->StoreDefaults();		// store the current defaults
				}
			}
			break;

		case ID_EDITOR_MATERIALMANAGER:
			{
				if(MATMAN)
					MATMAN->OpenEditWindow();
			}
			break;

		case IDH_SAVE_SYS_KIT:
			if (bHasFocus && CQFLAGS.bGameActive)
				CreateMenuSystemKitSaveLoad(false);
			break;

		case IDH_LOAD_SYS_KIT:
			if (bHasFocus && CQFLAGS.bGameActive)
 				CreateMenuSystemKitSaveLoad(true);
			break;

		case IDM_SHOWPARTS:
			if (CQFLAGS.bGameActive)
			{
				TogglePreferences(LOWORD(msg->wParam));
				DEFAULTS->StoreDefaults();		// store the current defaults
			}
			break;

		case IDM_NO_AUTO_SAVE:
		case IDM_POSTOOL_WORLD:
		case IDM_DRAW_HOTRECTS:
		case IDM_NO_WINNING_CONDITIONS:
		case IDM_INFO_HIGHLIGHT:
		case IDM_SHOW_GRIDS:
		case IDM_CHEAP_MOVEMENT:
		case IDM_CONST_UPDATE:
		case IDM_HARDWARE_CURSOR:
		case IDM_NO_FRAME_LIMIT:
			TogglePreferences(LOWORD(msg->wParam));
			DEFAULTS->StoreDefaults();		// store the current defaults
			break;
		
		case IDM_SPECTATOR_MODE:
		case IDM_ENABLE_CHEATS:
		case IDM_VISIBILITY_RULES_OFF:
			if (testToggleMode())
			{
				TogglePreferences(LOWORD(msg->wParam));
				enableCheatItems(DEFAULTS->GetDefaults()->bCheatsEnabled != 0);
				DEFAULTS->StoreDefaults();		// store the current defaults
			}
			break;

		case IDM_NOAUTOTARGET:
		case IDM_NO_SUPPLIES:
		case IDM_NO_DAMAGE:
			if (DEFAULTS->GetDefaults()->bCheatsEnabled)
			{
				TogglePreferences(LOWORD(msg->wParam));
				DEFAULTS->StoreDefaults();		// store the current defaults
			}
			break;
		}
		break;

	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
// receive notifications from the status bar
//
BOOL32 MenuResource::Notify (struct IResource *res, U32 message, void *parm)
{

	
	return 1;
}
//--------------------------------------------------------------------------//
//
void MenuResource::InitPreferences (void)
{
	U32 flags;
	USER_DEFAULTS *defaults = DEFAULTS->GetDefaults();

	flags = (defaults->bNoAutoSave) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_NO_AUTO_SAVE, flags); 
	flags = (defaults->bNoFrameLimit) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_NO_FRAME_LIMIT, flags); 
	flags = (defaults->bCheatsEnabled) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_ENABLE_CHEATS, flags); 
	flags = (defaults->bPosToolWorld) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_POSTOOL_WORLD, flags); 
	flags = (defaults->bEditorMode) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_EDITOR_MODE, flags); 
	flags = (defaults->bDrawHotrects) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_DRAW_HOTRECTS, flags); 
	flags = (defaults->bVisibilityRulesOff) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_VISIBILITY_RULES_OFF, flags); 
	flags = (defaults->bNoAutoTarget) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_NOAUTOTARGET, flags); 
	flags = (defaults->bNoWinningConditions) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_NO_WINNING_CONDITIONS, flags); 
	flags = (defaults->bPartDlgVisible) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_SHOWPARTS, flags); 
	flags = (defaults->bInfoHighlights) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_INFO_HIGHLIGHT, flags); 
	flags = (defaults->bNoSupplies) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_NO_SUPPLIES, flags); 
	flags = (defaults->bNoDamage) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_NO_DAMAGE, flags); 
	flags = (defaults->bShowGrids) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_SHOW_GRIDS, flags); 
	flags = (defaults->bCheapMovement) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_CHEAP_MOVEMENT, flags); 
	flags = (defaults->bConstUpdateRate) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_CONST_UPDATE, flags); 
	flags = (defaults->bHardwareCursor) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_HARDWARE_CURSOR, flags); 
	flags = (defaults->bSpectatorModeOn) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_BYCOMMAND);
	CheckMenuItem(hMenu, IDM_SPECTATOR_MODE, flags); 

	enableCheatItems(defaults->bCheatsEnabled != 0);
}
//--------------------------------------------------------------------------//
//
void MenuResource::initPreferences (void)
{
	InitPreferences();
	while (updateLoadPath() == 0)
		;
}
//--------------------------------------------------------------------------//
//
void MenuResource::TogglePreferences (U32 id)
{
	U32 flags;
	USER_DEFAULTS *defaults = DEFAULTS->GetDefaults();

	flags = ((GetMenuState(hMenu, id, MF_BYCOMMAND) & MF_CHECKED) ^ MF_CHECKED);

	CheckMenuItem(hMenu, id, MF_BYCOMMAND | flags); 

	switch (id)
	{
	case IDM_NO_AUTO_SAVE:
		defaults->bNoAutoSave = (flags!=0);
		break;
	case IDM_NO_FRAME_LIMIT:
		defaults->bNoFrameLimit = (flags!=0);
		break;
	case IDM_ENABLE_CHEATS:
		defaults->bCheatsEnabled = (flags!=0);
		break;
	case IDM_NO_SUPPLIES:
		defaults->bNoSupplies = (flags!=0);
		break;
	case IDM_NO_DAMAGE:
		defaults->bNoDamage = (flags!=0);
		break;
	case IDM_POSTOOL_WORLD:
		defaults->bPosToolWorld = (flags!=0);
		break;
	case IDM_EDITOR_MODE:
		defaults->bEditorMode = (flags!=0);
		EVENTSYS->Send(CQE_EDITOR_MODE, (void *)(defaults->bEditorMode));
		break;
	case IDM_DRAW_HOTRECTS:
		defaults->bDrawHotrects = (flags!=0);
		break;
	case IDM_VISIBILITY_RULES_OFF:
		defaults->bVisibilityRulesOff = (flags!=0);
		break;
	case IDM_NOAUTOTARGET:
		defaults->bNoAutoTarget = (flags!=0);
		break;
	case IDM_NO_WINNING_CONDITIONS:
		defaults->bNoWinningConditions = (flags!=0);
		break;
	case IDM_SHOWPARTS:
		defaults->bPartDlgVisible = (flags!=0);
		ShowWindow(hPartsDlg, (flags!=0) ? SW_SHOW : SW_HIDE);
		break;
	case IDM_INFO_HIGHLIGHT:
		defaults->bInfoHighlights = (flags!=0);
		break;
	case IDM_SHOW_GRIDS:
		defaults->bShowGrids = (flags!=0);
		break;
	case IDM_CHEAP_MOVEMENT:
		defaults->bCheapMovement = (flags!=0);
		break;
	case IDM_CONST_UPDATE:
		defaults->bConstUpdateRate = (flags!=0);
		break;
	case IDM_HARDWARE_CURSOR:
		defaults->bHardwareCursor = (flags!=0);
		break;
	case IDM_SPECTATOR_MODE:
		defaults->bSpectatorModeOn = (flags!=0);
		TogglePreferences(IDM_VISIBILITY_RULES_OFF);
		break;
	}
}
//-------------------------------------------------------------------
//
BOOL MenuResource::EditPathDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;
	MenuResource * menu = (MenuResource *) GetWindowLong(hwnd, DWL_USER);

	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND hEdit = GetDlgItem(hwnd, IDC_EDIT1);
			SetWindowLong(hwnd, DWL_USER, lParam);
			menu = (MenuResource *) GetWindowLong(hwnd, DWL_USER);

			SendMessage(hEdit, EM_SETLIMITTEXT, MAX_PATH-1, 0);
			SetWindowText(hEdit, menu->dataPath);
			SetFocus(hEdit);
		}
		break;
	
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				HWND hEdit = GetDlgItem(hwnd, IDC_EDIT1);
				GetWindowText(hEdit, menu->dataPath, sizeof(menu->dataPath));
				EndDialog(hwnd, 1);
			}
			break;
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL MenuResource::NetStressDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;
	MenuResource * menu = (MenuResource *) GetWindowLong(hwnd, DWL_USER);
	NM_UPDOWN * pnmud;

	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND hItem;
			SetWindowLong(hwnd, DWL_USER, lParam);
			menu = (MenuResource *) GetWindowLong(hwnd, DWL_USER);

			SetDlgItemInt(hwnd, IDC_EDIT1, DEFAULTS->GetDefaults()->minLatency, 0);
			hItem = GetDlgItem(hwnd, IDC_SPIN1);
			SendMessage(hItem, UDM_SETRANGE, 0, MAKELONG(2000,0));			// from 0 to 2000

			SetDlgItemInt(hwnd, IDC_EDIT2, DEFAULTS->GetDefaults()->packetLossPercent, 0);
			hItem = GetDlgItem(hwnd, IDC_SPIN2);
			SendMessage(hItem, UDM_SETRANGE, 0, MAKELONG(10,0));			// from 0 to 10

			SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
		}
		break;
	
	case WM_NOTIFY:	
		pnmud = (NM_UPDOWN *) lParam;
		if (pnmud->hdr.code == UDN_DELTAPOS && pnmud->hdr.idFrom==IDC_SPIN1)
		{
			if (pnmud->iPos < 100 && pnmud->iDelta<0)
				SetWindowLong(hwnd, DWL_MSGRESULT, 1);		// ignore this user click
			else
			if (pnmud->iDelta < 0)
				pnmud->iDelta = -100;
			else
			if (pnmud->iDelta > 0)
				pnmud->iDelta = 100;
		}
		else
		if (pnmud->hdr.code == UDN_DELTAPOS && pnmud->hdr.idFrom==IDC_SPIN2)
		{
			if (pnmud->iPos < 1 && pnmud->iDelta<0)
				SetWindowLong(hwnd, DWL_MSGRESULT, 1);		// ignore this user click
			else
			if (pnmud->iDelta < 0)
				pnmud->iDelta = -1;
			else
			if (pnmud->iDelta > 0)
				pnmud->iDelta = 1;
		}
		else
		if (pnmud->hdr.code == UDN_DELTAPOS && pnmud->hdr.idFrom==IDC_SPIN3)
		{
			if (pnmud->iPos <= 1000 && pnmud->iDelta<0)
				SetWindowLong(hwnd, DWL_MSGRESULT, 1);		// ignore this user click
			else
			if (pnmud->iDelta < 0)
				pnmud->iDelta = -1000;
			else
			if (pnmud->iDelta > 0)
				pnmud->iDelta = 1000;
		}
		result = 1;
		break;
		
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
		case IDOK:
			{
				int result;

				result = GetDlgItemInt(hwnd, IDC_EDIT1, 0, 0);
				NETBUFFER->SetMinLatency(result);

				result = GetDlgItemInt(hwnd, IDC_EDIT2, 0, 0);
				NETBUFFER->SetPacketLoss(result);
			}
			EndDialog(hwnd, 1);
			break;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL MenuResource::GameSpeedDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;
	MenuResource * menu = (MenuResource *) GetWindowLong(hwnd, DWL_USER);
	NM_UPDOWN * pnmud;

	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND hItem;
			SetWindowLong(hwnd, DWL_USER, lParam);
			menu = (MenuResource *) GetWindowLong(hwnd, DWL_USER);

			SetDlgItemInt(hwnd, IDC_EDIT1, DEFAULTS->GetDefaults()->gameSpeed, 1);
			hItem = GetDlgItem(hwnd, IDC_SPIN1);
			SendMessage(hItem, UDM_SETRANGE, 0, MAKELONG(MAX_GAME_SPEED, -MAX_GAME_SPEED));			// from -10 to 10

			if (DEFAULTS->GetDefaults()->bNoFrameLimit == 0)
			{
				EnableWindow(hItem, 1);
				EnableWindow(GetDlgItem(hwnd, IDC_EDIT1),1);
				CheckDlgButton(hwnd, IDC_CHECK1, BST_CHECKED);
				SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
			}
			else
			{
				SetFocus(GetDlgItem(hwnd, IDC_CHECK1));
			}

			if (PLAYERID)	// if a network session is active
			{
				SetFocus(hwnd);
				EnableWindow(hItem, 0);
				EnableWindow(GetDlgItem(hwnd, IDC_EDIT1),0);
				EnableWindow(GetDlgItem(hwnd, IDC_CHECK1),0);
				result = TRUE;
			}
		}
		break;
	
	case WM_NOTIFY:	
		pnmud = (NM_UPDOWN *) lParam;
		if (pnmud->hdr.code == UDN_DELTAPOS && pnmud->hdr.idFrom==IDC_SPIN1)
		{
			if (pnmud->iPos <= -MAX_GAME_SPEED && pnmud->iDelta<0)
				SetWindowLong(hwnd, DWL_MSGRESULT, 1);		// ignore this user click
			else
			if (pnmud->iDelta < 0)
				pnmud->iDelta = -1;
			else
			if (pnmud->iDelta > 0)
				pnmud->iDelta = 1;
		}
		result = 1;
		break;
		
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CHECK1:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				SendMessage(hMainWindow,WM_COMMAND,IDM_NO_FRAME_LIMIT, 0);
				if (DEFAULTS->GetDefaults()->bNoFrameLimit == 0)	// frame limiting is on
				{
					EnableWindow(GetDlgItem(hwnd, IDC_SPIN1),1);
					EnableWindow(GetDlgItem(hwnd, IDC_EDIT1),1);
				}
				else
				{
					EnableWindow(GetDlgItem(hwnd, IDC_SPIN1),0);
					EnableWindow(GetDlgItem(hwnd, IDC_EDIT1),0);
				}
			}
			break;

		case IDCANCEL:
		case IDOK:
			if (abs(int(GetDlgItemInt(hwnd, IDC_EDIT1, 0, 1))) <= MAX_GAME_SPEED)
			{
				EndDialog(hwnd, GetDlgItemInt(hwnd, IDC_EDIT1, 0, 1) + MAX_GAME_SPEED);
				break;
			}
			EndDialog(hwnd, -1);
			break;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void MenuResource::enableVolatileItems (bool bEnable)
{
	UINT flags = (bEnable) ? (MF_BYCOMMAND | MF_ENABLED) : (MF_BYCOMMAND | MF_GRAYED);

	EnableMenuItem(IDM_VISIBILITY_RULES_OFF, flags); 		
	EnableMenuItem(IDM_SPECTATOR_MODE, flags);
	EnableMenuItem(IDM_ENABLE_CHEATS, flags);
	EnableMenuItem(IDM_OPEN_MISSION, flags);
	EnableMenuItem(IDM_NEW_MISSION, flags);

	enableCheatItems(DEFAULTS->GetDefaults()->bCheatsEnabled != 0);

	//
	// gray out the popup menus
	// 
	::EnableMenuItem(GetSubMenu(MENUPOS_FILE), MENUPOS_MRU_MISSION, flags|MF_BYPOSITION);		// MF_BYCOMMAND==0
	EnableMenuItem(MENUPOS_EDITOR, flags|MF_BYPOSITION);		// MF_BYCOMMAND==0
	ForceDraw();
}
//--------------------------------------------------------------------------//
//
void MenuResource::enableCheatItems (bool bEnable)
{
	UINT flags = (bEnable) ? (MF_BYCOMMAND | MF_ENABLED) : (MF_BYCOMMAND | MF_GRAYED);

	EnableMenuItem(IDM_NO_SUPPLIES, flags); 		
	EnableMenuItem(IDM_NO_DAMAGE, flags); 		
	EnableMenuItem(IDM_NOAUTOTARGET, flags); 		
}
//--------------------------------------------------------------------------//
//
void MenuResource::quickLoad()
{	
//	MISSION->Load("f0000000.mission",SAVEDIR);
	MISSION->QuickLoadFile();
}
//--------------------------------------------------------------------------//
//
void MenuResource::quickSave()
{
	MISSION->QuickSaveFile();
//	MISSION->SetFileDescription(L"QuickSave");
//	MISSION->Save("f0000000.mission",SAVEDIR);
}
//--------------------------------------------------------------------------//
//
void MenuResource::missingDirDialog (const char * name, bool bFatal, const char *prependName)
{
	wchar_t appname[64];
	wchar_t buffer[256];
	U32 flags = MB_OK;

	if (bFatal)
		flags |= MB_ICONSTOP;
	else
		flags |= MB_ICONWARNING;
	wcsncpy(appname, _localLoadStringW(IDS_APP_NAME), sizeof(appname)/sizeof(wchar_t));

	swprintf(buffer, _localLoadStringW(IDS_HELP_DIR_MISSING), ((prependName)?prependName : ""), name);
	
	MessageBoxW(hMainWindow, buffer, appname, flags);

	if (bFatal)
	{
		PostQuitMessage(-1);
		if (WM)
			WM->ServeMessageQueue();
	}
}
//--------------------------------------------------------------------------//
//
bool MenuResource::updateLoadPath (void)
{
	char buffer[MAX_PATH+4];
	DAFILEDESC fdesc = buffer;
	SEARCHPATHDESC sdesc;
	bool result = false;

	if (searchPath==0)
	{
		if (DACOM->CreateInstance(&sdesc, searchPath) != GR_OK)
			CQBOMB0("Could not create searchPath object");
	}

	if (DEFAULTS->GetStringFromRegistry(DATA_PATH, dataPath, sizeof(dataPath)) == 0)
	{
		strcpy(dataPath, "Data");
		DEFAULTS->SetStringInRegistry(DATA_PATH, dataPath);
	}

	// concat CD install directory onto path
	{
		int i;
		strcpy(buffer, dataPath);
		i = strlen(buffer);
		if (i)
		{
			// force a ';' at the end of the string
			buffer[i++] = ';';
			buffer[i] = 0;
		}

		if (DEFAULTS->GetInstallStringFromRegistry("CDPath", buffer+i, sizeof(buffer)-i))
		{
			i = strlen(buffer);
			if (i)
			{
				if (buffer[i-1] != '\\')
				{
					buffer[i++] = '\\';
					buffer[i] = 0;
				}
				strcpy(buffer+i, "game\\data");
			}

			searchPath->SetPath(buffer);
		}
		else
			searchPath->SetPath(dataPath);
	}

	//
	// now create directory shortcuts
	//
	fdesc.lpFileName = buffer;

	strcpy(buffer, "Objects");
	if (OBJECTDIR)
		OBJECTDIR->Release();
	if (searchPath->CreateInstance(&fdesc, (void **)&OBJECTDIR) != GR_OK)
	{
#ifndef FINAL_RELEASE
		missingDirDialog(buffer, false);
		if (DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG3), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) EditPathDlgProc, (LPARAM) this))
		{
			DEFAULTS->SetStringInRegistry(DATA_PATH, dataPath);
			goto Done;
		}
#endif	// !FINAL_RELEASE
		missingDirDialog(buffer, true);
	}
#if defined(_DEMO_) && defined(_DEBUG)
	OBJECTDIR = CreateLogOfFile(OBJECTDIR, "r:\\projects\\conquest\\demo\\ObjectsLog.txt");
#endif
	ENGINE->set_search_path2(OBJECTDIR);

	strcpy(buffer, "Interface");
	if (INTERFACEDIR)
		INTERFACEDIR->Release();
	if (searchPath->CreateInstance(&fdesc, (void **)&INTERFACEDIR) != GR_OK)
	{
#ifndef FINAL_RELEASE
		missingDirDialog(buffer, false);
		if (DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG3), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) EditPathDlgProc, (LPARAM) this))
		{
			DEFAULTS->SetStringInRegistry(DATA_PATH, dataPath);
			goto Done;
		}
#endif	// !FINAL_RELEASE
		missingDirDialog(buffer, true);
	}
#if defined(_DEMO_) && defined(_DEBUG)
	INTERFACEDIR = CreateLogOfFile(INTERFACEDIR, "r:\\projects\\conquest\\demo\\InterfaceLog.txt");
#endif

	strcpy(buffer, "Textures");
	if (TEXTURESDIR)
		TEXTURESDIR->Release();
	if (searchPath->CreateInstance(&fdesc, (void **)&TEXTURESDIR) != GR_OK)
	{
#ifndef FINAL_RELEASE
		missingDirDialog(buffer, false);
		if (DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG3), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) EditPathDlgProc, (LPARAM) this))
		{
			DEFAULTS->SetStringInRegistry(DATA_PATH, dataPath);
			goto Done;
		}
#endif	// !FINAL_RELEASE
		missingDirDialog(buffer, true);
	}
#if defined(_DEMO_) && defined(_DEBUG)
	TEXTURESDIR = CreateLogOfFile(TEXTURESDIR, "r:\\projects\\conquest\\demo\\TexturesLog.txt");
#endif

	strcpy(buffer, "Mat");
	if (MATERIALDIR)
		MATERIALDIR->Release();
	if (searchPath->CreateInstance(&fdesc, (void **)&MATERIALDIR) != GR_OK)
	{
#ifndef FINAL_RELEASE
		missingDirDialog(buffer, false);
		if (DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG3), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) EditPathDlgProc, (LPARAM) this))
		{
			DEFAULTS->SetStringInRegistry(DATA_PATH, dataPath);
			goto Done;
		}
#endif	// !FINAL_RELEASE
		missingDirDialog(buffer, true);
	}
#if defined(_DEMO_) && defined(_DEBUG)
	MATERIALDIR = CreateLogOfFile(MATERIALDIR, "r:\\projects\\conquest\\demo\\TexturesLog.txt");
#endif

	strcpy(buffer, "Profiles");
	if (PROFILEDIR)
		PROFILEDIR->Release();
	if (searchPath->CreateInstance(&fdesc, (void **)&PROFILEDIR) != GR_OK)
	{
#ifndef FINAL_RELEASE
		missingDirDialog(buffer, false);
		if (DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG3), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) EditPathDlgProc, (LPARAM) this))
		{
			DEFAULTS->SetStringInRegistry(DATA_PATH, dataPath);
			goto Done;
		}
#endif	// !FINAL_RELEASE
		missingDirDialog(buffer, true);
	}
#if defined(_DEMO_) && defined(_DEBUG)
	PROFILEDIR = CreateLogOfFile(PROFILEDIR, "r:\\projects\\conquest\\demo\\ProfileLog.txt");
#endif

	{
		COMPTR<IFileSystem> mapsDir;
	
		strcpy(buffer, "Maps");

		if (searchPath->CreateInstance(&fdesc, mapsDir) != GR_OK)
		{
#ifndef FINAL_RELEASE
			missingDirDialog(buffer, false);
			if (DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG3), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) EditPathDlgProc, (LPARAM) this))
			{
				DEFAULTS->SetStringInRegistry(DATA_PATH, dataPath);
				goto Done;
			}
#endif	// !FINAL_RELEASE
			missingDirDialog(buffer, true);
		}
		
		strcpy(buffer, "Campaign");
		if (SPMAPDIR)
			SPMAPDIR->Release();
		if (mapsDir->CreateInstance(&fdesc, (void **)&SPMAPDIR) != GR_OK)
		{
#ifndef FINAL_RELEASE
			missingDirDialog(buffer, false, "Maps\\");
			if (DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG3), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) EditPathDlgProc, (LPARAM) this))
			{
				DEFAULTS->SetStringInRegistry(DATA_PATH, dataPath);
				goto Done;
			}
#endif	// !FINAL_RELEASE
			missingDirDialog(buffer, true, "Maps\\");
		}

		strcpy(buffer, "Multiplayer");
		if (MPMAPDIR)
			MPMAPDIR->Release();
		if (mapsDir->CreateInstance(&fdesc, (void **)&MPMAPDIR) != GR_OK)
		{
#ifndef FINAL_RELEASE
			missingDirDialog(buffer, false, "Maps\\");
			if (DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG3), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) EditPathDlgProc, (LPARAM) this))
			{
				DEFAULTS->SetStringInRegistry(DATA_PATH, dataPath);
				goto Done;
			}
#endif	// !FINAL_RELEASE
			missingDirDialog(buffer, true, "Maps\\");
		}
	}

	strcpy(buffer, "Movies");
	if (MOVIEDIR)
		MOVIEDIR->Release();
	if (searchPath->CreateInstance(&fdesc, (void **)&MOVIEDIR) != GR_OK)
		missingDirDialog(buffer, false);

	strcpy(buffer, "Music");
	if (MUSICDIR)
		MUSICDIR->Release();
	if (searchPath->CreateInstance(&fdesc, (void **)&MUSICDIR) != GR_OK)
		missingDirDialog(buffer, false);

	strcpy(buffer, "SFX");
	if (SFXDIR)
		SFXDIR->Release();
	if (searchPath->CreateInstance(&fdesc, (void **)&SFXDIR) != GR_OK)
		missingDirDialog(buffer, false);
#if defined(_DEMO_) && defined(_DEBUG)
	SFXDIR = CreateLogOfFile(SFXDIR, "r:\\projects\\conquest\\demo\\SFXLog.txt");
#endif

	strcpy(buffer, "Speech");
	if (SPEECHDIR)
		SPEECHDIR->Release();
	if (searchPath->CreateInstance(&fdesc, (void **)&SPEECHDIR) != GR_OK)
		missingDirDialog(buffer, false);
#if defined(_DEMO_) && defined(_DEBUG)
	SPEECHDIR = CreateLogOfFile(SPEECHDIR, "r:\\projects\\conquest\\demo\\SpeechLog.txt");
#endif

#ifndef _DEMO_
	strcpy(buffer, "MSpeech");
	if (MSPEECHDIR)
		MSPEECHDIR->Release();
	if (searchPath->CreateInstance(&fdesc, (void **)&MSPEECHDIR) != GR_OK)
		missingDirDialog(buffer, false);

	strcpy(buffer, "Scripts");
	if (SCRIPTSDIR)
		SCRIPTSDIR->Release();
	if (DACOM->CreateInstance(&fdesc, (void **)&SCRIPTSDIR) != GR_OK)
		missingDirDialog(buffer, true);
#endif // end !_DEMO_

	result = true;

#ifndef FINAL_RELEASE
Done:
#endif //!FINAL_RELEASE
	return result;
}
//-------------------------------------------------------------------------------------------------//
//
void MenuResource::toggleLocalPause (void)
{
	if (CQFLAGS.bGameActive)
	{
		if (bLocalPaused==0 && NETPACKET->HasPauseWarningBeenReceived())
		{
			SCROLLTEXT->SetText(IDS_FORMAT_PAUSE_DENIED);
		}
		else
		{
			bLocalPaused = !bLocalPaused;
			if ((CQFLAGS.bGamePaused = bLocalPaused||bRemotePaused) != 0)
			{
				deletePauseMenu();
				pPauseDlg = CreateMenuPause((bLocalPaused)? 0 : bRemotePaused);
			}
			else
			{
				deletePauseMenu();
			}
			EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
			EVENTSYS->Send(CQE_LOCALPAUSED, (void*)bLocalPaused);
		}
	}
}
//--------------------------------------------------------------------------//
//
void MenuResource::toggleNetPause (int bPause)
{
	bRemotePaused = bPause;
	if ((CQFLAGS.bGamePaused = bLocalPaused||bRemotePaused) != 0)
	{
		deletePauseMenu();
		pPauseDlg = CreateMenuPause((bLocalPaused)? 0 : bRemotePaused);
	}
	else
	{
		deletePauseMenu();
	}
	EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
}
//
//----------------------------------------------------------------------------
//
static LONG CALLBACK listControlProcedure(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
 	WNDPROC oldProc = (WNDPROC) GetWindowLong(hwnd, GWL_USERDATA);

	switch (message)
	{
		case WM_KEYDOWN:
			switch (LOWORD(wParam))
			{
			case VK_TAB:
			case VK_RETURN:
				PostMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDC_BUTTON1, 0), (LONG)hwnd);
				return 0;
			}
			break;
	}

	return CallWindowProc((long (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long))oldProc, hwnd, message, wParam, lParam);
}
//--------------------------------------------------------------------------//
//
struct partDlgSaveStruct : WINDOWPLACEMENT
{
	partDlgSaveStruct (void)
	{
		length = sizeof(*this);
	}
};
//--------------------------------------------------------------------------//
//
BOOL MenuResource::partDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;
	MenuResource * menu = (MenuResource *) GetWindowLong(hwnd, DWL_USER);

	switch (message)
	{
	case WM_INITDIALOG:
		{
			SetWindowLong(hwnd, DWL_USER, lParam);
			menu = (MenuResource *) GetWindowLong(hwnd, DWL_USER);
			
			HWND hList = GetDlgItem(hwnd, IDC_LIST1);
			WNDPROC oldProc;
			partDlgSaveStruct loadStruct;

			if ((oldProc = (WNDPROC) GetWindowLong(hList, GWL_WNDPROC)) != 0)
			{
				SetWindowLong(hList, GWL_USERDATA, (LONG) oldProc);
				SetWindowLong(hList, GWL_WNDPROC, (LONG) listControlProcedure);
			}

			if (DEFAULTS->GetDataFromRegistry("partsDialog", &loadStruct, sizeof(loadStruct)) == sizeof(loadStruct))
			{
				SetWindowPos(hwnd, HWND_TOPMOST,
							 loadStruct.rcNormalPosition.left,
							 loadStruct.rcNormalPosition.top,
							 loadStruct.rcNormalPosition.right - loadStruct.rcNormalPosition.left,
							 loadStruct.rcNormalPosition.bottom - loadStruct.rcNormalPosition.top,
							 SWP_NOZORDER|SWP_NOSIZE);
				if (DEFAULTS->GetDefaults()->bPartDlgVisible)
					ShowWindow(hwnd, loadStruct.showCmd);
			}
		}
		break;

	case WM_INITMENU:
		{
			HMENU hMenu = (HMENU) wParam;
			::EnableMenuItem(hMenu, SC_MAXIMIZE, MF_BYCOMMAND|MF_GRAYED); 
			::EnableMenuItem(hMenu, SC_SIZE, MF_BYCOMMAND|MF_GRAYED); 
			SetWindowLong(hwnd, DWL_MSGRESULT, 0);
			result = 1;
		}
		break;

	case WM_CLOSE:
		if (DEFAULTS->GetDefaults()->bPartDlgVisible)
		{
			partDlgSaveStruct loadStruct;

			if (GetWindowPlacement(hwnd, &loadStruct))
				DEFAULTS->SetDataInRegistry("partsDialog", &loadStruct, sizeof(loadStruct));
			
			if (lParam != -1)
				menu->TogglePreferences(IDM_SHOWPARTS);
		}
		result = 1;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON1:
			menu->sendPartNotification();
			break;
		case IDC_BUTTON2:
			menu->gotoPart();
			break;
	
		case IDC_LIST1:
			switch (HIWORD(wParam))
			{
			case LBN_DBLCLK:
				menu->sendPartNotification();
				break;
			}
			break;	// end case IDC_LIST1
		}
		break; // end case WM_COMMAND
	}

	return result;
}
//--------------------------------------------------------------------------//
//
struct _menu : GlobalComponent
{
	MenuResource * menu;

	virtual void Startup (void)
	{
		HMENU hMenu = GetMenu(hMainWindow);
		if (hMenu == 0)
			hMenu = LoadMenu(hResource, MAKEINTRESOURCE(IDR_MENU1));

//		HMENU hPopup = CreatePopupMenu();
//		AppendMenu(hMenu, MF_POPUP|MF_STRING, (U32)hPopup, "&View");  

		HMENU hPopup = ::GetSubMenu(hMenu, MENUPOS_VIEW);
		::DeleteMenu(hPopup, ID_VIEW_EMPTY, MF_BYCOMMAND);
		
		MENU = menu = new DAComponent<MenuResource>;
		AddToGlobalCleanupList((IDAComponent **) &MENU);

		menu->hMenu = hMenu;
		menu->dwMenuHeight = GetSystemMetrics(SM_CYMENU);

		menu->wheelMsg = RegisterWindowMessage(MSH_MOUSEWHEEL);
		menu->hPartsDlg = CreateDialogParam(hResource, MAKEINTRESOURCE(IDD_DIALOG4), hMainWindow, MenuResource::partDlgProc, (LPARAM) menu);
		menu->initPreferences();

		AddToGlobalCleanupList(&OBJECTDIR);
		AddToGlobalCleanupList(&MUSICDIR);
		AddToGlobalCleanupList(&INTERFACEDIR);
		AddToGlobalCleanupList(&SFXDIR);
		AddToGlobalCleanupList(&SPEECHDIR);
		AddToGlobalCleanupList(&TEXTURESDIR);
		AddToGlobalCleanupList(&SPMAPDIR);
		AddToGlobalCleanupList(&MPMAPDIR);
		AddToGlobalCleanupList(&SAVEDIR);
		AddToGlobalCleanupList(&PROFILEDIR);
		AddToGlobalCleanupList(&SCRIPTSDIR);
		AddToGlobalCleanupList(&MOVIEDIR);
		AddToGlobalCleanupList(&MSPEECHDIR);
		AddToGlobalCleanupList(&MATERIALDIR);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise(MENU, &menu->eventHandle);
			FULLSCREEN->SetCallbackPriority(menu, EVENT_PRIORITY_MENU);
		}

		if (STATUS->QueryOutgoingInterface("IResourceClient", connection) == GR_OK)
			connection->Advise(MENU, &menu->statusHandle);
	}
};

static _menu startup;

//--------------------------------------------------------------------------//
//-----------------------------End Menu.cpp---------------------------------//
//--------------------------------------------------------------------------//
