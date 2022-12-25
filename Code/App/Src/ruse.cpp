//--------------------------------------------------------------------------//
//                                                                          //
//                               Ruse.cpp                                   //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ruse.cpp 21    11/15/00 1:03p Jasony $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <Globals.h>

#include "Resource.h"
#include "Rusekeys.h"
#include "RuseMap.h"
#include "DrawAgent.h"
#include "CQTrace.h"
#include "Menu.h"
#include "BaseHotRect.h"
#include "Objlist.h"
#include "TManager.h"

#include <memfile.h>
#include <HeapObj.h>
#include <EventSys.h>
#include <FileSys.h>
#include <HKEvent.h>
#include <IConnection.h>
#include <TComponent.h>
#include <stdlib.h>
#include <TSmartPointer.h>

//--------------------------------------------------------------------------
//-----------------------Game Global instances------------------------------
//--------------------------------------------------------------------------

BOOL32 bMapChanged;
BOOL32 DRAWING=0;
RECT drawRect;
PANE pane;
HWND hDlgBox;

S32 mouseX =0 ,mouseY=0;
S32 fps;
HMENU hMenu, hOldMenu;
IHotkeyEvent * RHOTKEY;
struct EventCallback * EVENT;

IFileSystem * g_OutFile;
static U32 eventHandle;

BOOL32 bDroppingGates;

U32 sysTextureID;

void setCursor (U32 cursorID)
{
	if (cursorID)
		CURSOR->SetCursor(cursorID);
	else
		CURSOR->SetDefaultState();
}

BOOL32 bRuseEditorMode;

struct RUSEArchEnum : IArchetypeEnum
{
	HWND hListBox;

	virtual	BOOL32 __stdcall ArchetypeEnum (const char * name, void *data, U32 size);
};

BOOL32 __stdcall RUSEArchEnum::ArchetypeEnum (const char * name, void *data, U32 size)
{
	BASIC_DATA *objData = (BASIC_DATA *)data;
	if (objData->objClass == OC_JUMPGATE)
		SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM) name);

	return 1;
}

RUSEArchEnum enumer;
//--------------------------------------------------------------------------//
//
void initializeArchListbox (HWND hListbox)
{
	enumer.hListBox = hListbox;
	ARCHLIST->EnumerateArchetypeData(&enumer);
}
//-------------------------------------------------------------------
//
static BOOL32 GuardedEnableWindow (HWND hwnd, BOOL bEnable)
{
	BOOL bCurrent = IsWindowEnabled(hwnd);

	if (bCurrent != bEnable)
	{
		EnableWindow(hwnd, bEnable);
		return 1;
	}

	return 0;
}
//----------------------------------------------------------------------------
//
static LONG CALLBACK editControlProcedure(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
 	WNDPROC oldProc = (WNDPROC) GetWindowLong(hwnd, GWL_USERDATA);

	switch (message)
	{
		case WM_KEYDOWN:
			switch (LOWORD(wParam))
			{
			case VK_TAB:
				SetFocus(GetDlgItem(GetParent(hwnd), IDC_LIST1));
				return 0;
			case VK_RETURN:
				PostMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDOK, 0), (LONG)hwnd);
				return 0;
			}
			break;
	}

	return CallWindowProc(oldProc, hwnd, message, wParam, lParam);
}
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
				PostMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDOK, 0), (LONG)hwnd);
				return 0;
			}
			break;
	}

	return CallWindowProc((long (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long))oldProc, hwnd, message, wParam, lParam);
}
//-------------------------------------------------------------------
//
void setTextForSelection (HWND hwnd)
{
 	HWND hPartName = GetDlgItem(hwnd, IDC_PART_NAME);

	SetWindowText(hPartName, "Part*");
}
//-------------------------------------------------------------------
//
struct loadDlgSaveStruct : WINDOWPLACEMENT
{
	loadDlgSaveStruct (void)
	{
		length = sizeof(*this);
	}
};
//-------------------------------------------------------------------
//
static BOOL CALLBACK loadArchDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;

	switch (message)
	{
	case WM_INITDIALOG:
		{
			initializeArchListbox(GetDlgItem(hwnd, IDC_LIST1));
			SetFocus(GetDlgItem(hwnd, IDC_LIST1));
			HMENU hMenu = GetSystemMenu(hwnd, 0);
			EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND|MF_DISABLED); 
			SendMessage(GetDlgItem(hwnd, IDM_ADD_PART), BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadCursor(hResource, MAKEINTRESOURCE(IDC_CURSOR_PLACEOBJECT))); 
			HWND hList = GetDlgItem(hwnd, IDC_LIST1);
			WNDPROC oldProc;
			loadDlgSaveStruct loadStruct;

			if ((oldProc = (WNDPROC) GetWindowLong(hList, GWL_WNDPROC)) != 0)
			{
				SetWindowLong(hList, GWL_USERDATA, (LONG) oldProc);
				SetWindowLong(hList, GWL_WNDPROC, (LONG) listControlProcedure);
			}
			HWND hPartName = GetDlgItem(hwnd, IDC_PART_NAME);
			if ((oldProc = (WNDPROC) GetWindowLong(hPartName, GWL_WNDPROC)) != 0)
			{
				SetWindowText(hPartName, "");
				SetWindowLong(hPartName, GWL_USERDATA, (LONG) oldProc);
				SetWindowLong(hPartName, GWL_WNDPROC, (LONG) editControlProcedure);
			}
			setTextForSelection(hwnd);

			if (DEFAULTS->GetDataFromRegistry("loadDialog", &loadStruct, sizeof(loadStruct)) == sizeof(loadStruct))
			{
				SetWindowPos(hwnd, HWND_TOPMOST,
							 loadStruct.rcNormalPosition.left,
							 loadStruct.rcNormalPosition.top,
							 loadStruct.rcNormalPosition.right - loadStruct.rcNormalPosition.left,
							 loadStruct.rcNormalPosition.bottom - loadStruct.rcNormalPosition.top,
							 SWP_NOZORDER|SWP_NOSIZE);
				if (bRuseEditorMode)
					ShowWindow(hwnd, loadStruct.showCmd);
			}
		}
		break;

	case WM_INITMENU:
		{
			HMENU hMenu = (HMENU) wParam;
			EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND|MF_GRAYED); 
			EnableMenuItem(hMenu, SC_MAXIMIZE, MF_BYCOMMAND|MF_GRAYED); 
			EnableMenuItem(hMenu, SC_SIZE, MF_BYCOMMAND|MF_GRAYED); 
			SetWindowLong(hwnd, DWL_MSGRESULT, 0);
			result = 1;
		}
		break;

	case WM_CLOSE:
		{
			loadDlgSaveStruct loadStruct;

			if (GetWindowPlacement(hwnd, &loadStruct))
				DEFAULTS->SetDataInRegistry("loadDialog", &loadStruct, sizeof(loadStruct));
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (GetFocus() == GetDlgItem(hwnd, IDC_PART_NAME) && IsWindowEnabled(GetDlgItem(hwnd, IDM_ADD_PART)))
				SendMessage(hwnd, WM_COMMAND, IDM_ADD_PART, 0);
			else
				SetFocus(GetDlgItem(hwnd, IDC_PART_NAME));
			break;
	
		case IDC_LIST1:
			switch (HIWORD(wParam))
			{
			case LBN_DBLCLK:
				if (IsWindowEnabled(GetDlgItem(hwnd, IDM_ADD_PART)))
					SendMessage(hwnd, WM_COMMAND, IDM_ADD_PART, 0);
				else
					SetFocus(GetDlgItem(hwnd, IDC_PART_NAME));
				break;

			case LBN_SELCHANGE:
				setTextForSelection(hwnd);
				char archetype[32];
				HWND hListbox = GetDlgItem(hwnd, IDC_LIST1);
				S32 index = SendMessage(hListbox, LB_GETCARETINDEX, 0, 0);
				SendMessage(hListbox, LB_GETTEXT, index, (LPARAM) archetype);
				current_arch = ARCHLIST->GetArchetypeDataID(archetype);
//				EVENTSYS->Send(CQE_NEW_SELECTION, static_cast<ObjectList *>(this));
//				if (GetWindowTextLength(GetDlgItem(hwnd, IDC_PART_NAME)))
					GuardedEnableWindow(GetDlgItem(hwnd, IDM_ADD_PART), 1);	// enable button
				break;
			}
			break;

		case IDC_PART_NAME:
			switch (HIWORD(wParam))
			{
			case EN_SETFOCUS:
				SendMessage((HWND)lParam, EM_SETSEL, 0, -1);	// select all text
				break;
			case EN_CHANGE:
//				EVENTSYS->Send(CQE_NEW_SELECTION, static_cast<ObjectList *>(this));
				if (GetWindowTextLength((HWND)lParam) == 0)
				{
					GuardedEnableWindow(GetDlgItem(hwnd, IDM_ADD_PART), 0);	// disable button
				}
				else
				{
					GuardedEnableWindow(GetDlgItem(hwnd, IDM_ADD_PART), 
						(SendMessage(GetDlgItem(hwnd, IDC_LIST1), LB_GETCURSEL, 0, 0) >= 0));
				}
				break;
			}
			break;

		case IDM_ADD_PART:
			{
				HWND hListbox = GetDlgItem(hwnd, IDC_LIST1);
				HWND hPartName = GetDlgItem(hwnd, IDC_PART_NAME);

				S32 index = SendMessage(hListbox, LB_GETCARETINDEX, 0, 0);

				char archetype[32];
				char partName[32];

				if (index >= 0 && GetWindowTextLength(hPartName))
				{
					SendMessage(hListbox, LB_GETTEXT, index, (LPARAM) archetype);

					GetWindowText(hPartName, partName, sizeof(partName));

					current_arch = ARCHLIST->GetArchetypeDataID(archetype);

					setCursor(IDC_CURSOR_PLACEOBJECT);
					bDroppingGates = TRUE;

/*					if (createPart(archetype, partName))
					{
//						setTextForSelection(hwnd);
						SetFocus(hMainWindow);
					}*/
				}

			}
			break;
		}
	}

	return result;
}
//--------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE EventCallback : public IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(EventCallback)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()


	DEFMETHOD(Notify) (U32 message, void *param = 0);
};
//--------------------------------------------------------------------------
//
static BOOL32 LoadHotkeys (void)
{
	COMPTR<IFileSystem> pFile;
	COMPTR<IDAConnectionPoint> connection;
	HKEVENTDESC hkdesc;

	HRSRC hRes;

	if ((hRes = FindResource(hResource, MAKEINTRESOURCE(IDR_RUSE_HOTKEY), "DAHOTKEY")) != 0)
	{
		HGLOBAL hGlobal;

		if ((hGlobal = LoadResource(hResource, hRes)) != 0)
		{
			LPVOID pData;

			if ((pData = LockResource(hGlobal)) != 0)
			{
				MEMFILEDESC mdesc = "hotkey file";
 				mdesc.lpBuffer = pData;
				mdesc.dwBufferSize = SizeofResource(hResource, hRes);
				mdesc.dwFlags = CMF_DONT_COPY_MEMORY;

				CreateUTFMemoryFile(mdesc, pFile);
			}
		}
	}

	if (pFile == 0)
	{
		CQBOMB0("Could not open hotkey file.");
	 	return 0;
	}

	hkdesc.file = pFile;
	hkdesc.hotkeyMessage = CQE_RUSE_HOTKEY;
	hkdesc.joyMessage = 0;
	
	if (DACOM->CreateInstance(&hkdesc, (void **) &RHOTKEY) != GR_OK)
	{
		CQBOMB0("Could not start hotkey system");
		return 0;
	}

	if (RHOTKEY->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
	{
		U32 handle;
		connection->Advise(EVENT, &handle);
	}

	return 1;
}
//--------------------------------------------------------------------------
//
static void draw (void)
{
	CQASSERT(HEAP->EnumerateBlocks());

   	DrawMap();

	if (DRAWING)
	{
		DrawRect(drawRect.left,drawRect.top,drawRect.right,drawRect.bottom, RGB(255,0,255));
	}

	if (linking)
	{
		DA::LineDraw(0, linkX, linkY, mouseX, mouseY, RGB(255,255,0), 0);
	}

	CQASSERT(HEAP->EnumerateBlocks());
}
//--------------------------------------------------------------------------//
//
GENRESULT EventCallback::Notify (U32 message, void *param)
{
	MSG * msg = (MSG *)param;

	DRAGGER *target;

	if (message < WM_USER)
		RHOTKEY->SystemMessage((S32)msg->hwnd,(S32)message, (S32)msg->wParam,(S32)msg->lParam);
	
	switch (message)
	{
	case CQE_RENDER_LAST3D:
		draw();
		break;

	case CQE_ENDFRAME:
		{
			System *systemPos = firstSystem;
			
			while (systemPos)
			{
				systemPos->Draw2D();
				systemPos = systemPos->next;
			}
		}
		break;

	case CQE_UPDATE:
		PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT,0);
		break;

	case CQE_RUSE_HOTKEY:
		switch ((U32)param)
		{
		case IDH_ZOOM_IN:
			ZOOM += 2;
			break;
		case IDH_ZOOM_OUT:
			ZOOM -= 2;
			if (ZOOM == 0)
				ZOOM = 2;
			break;
		case IDH_NEW_SYSTEM:
			if (availableSystemIDs == FULL_MAP)
			{
				CQERROR1("Map Is Full! Max systems=%d", MAX_SYSTEMS);
			}
			else
			{
				S32 worldX,worldY;
				GetWorldCoords(&worldX,&worldY,mouseX,mouseY);
				CreateSystem(worldX,worldY)->AssignID();
				dragging = lastSystem;
				dragging->SetAnchor(mouseX,mouseY);
				baseX = INVALID_BASE;
				baseY = INVALID_BASE;
			}
			break;
		case IDH_DELETE:
			target = ObjectClicked();

			DRAGGER *which;
			which = target;
			dragging = 0;
			if (which && which->IsSystem())
			{
				if (which->bIndestructable)
				{
					CQERROR0("Cannot delete a system that was loaded from disk!");
				}
				else
					delete which;
			}
			else
			{
				GateLink *whichLink;
				if (which)
				{
					//dangerous cast
					Jump *jump = static_cast<Jump *>(which);
					whichLink = jump->link;
				}
				else
					whichLink = LinkClicked();

				if (whichLink)
				{
					if (whichLink->bIndestructable)
					{
						CQERROR0("Cannot delete a link that was loaded from disk!");
					}
					else
						delete whichLink;
				}
			}
			break;
		case IDH_LINK:
			{
				if (current_arch)
				{
					target = ObjectClicked();
			
					if (target && target->IsSystem())
					{
						linking = (System *)target;
						linkX = mouseX;
						linkY = mouseY;
						gateA_arch = current_arch;
						bDroppingGates = FALSE;
						setCursor(0);
					}
				}
				else
					CQERROR0("You must select a jumpgate from the archetype list");
			}
			break;
	/*	case IDH_GROW_System:
			if (dragging)
				dragging->Grow(GROW_FACTOR);
			break;
		case IDH_SHRINK_System:
			if (dragging)
				dragging->Grow(-GROW_FACTOR);
			break;							 */
		}
		break;
	case WM_LBUTTONDOWN:
		if (linking)
		{
		}
		else
		{
			if (bDroppingGates)
			{
				target = ObjectClicked();
			
				if (target && target->IsSystem())
				{
					linking = (System *)target;
					linkX = mouseX;
					linkY = mouseY;
					gateA_arch = current_arch;
					bDroppingGates = FALSE;
					setCursor(0);
				}
			}
			else
			{
				dragging = ObjectClicked();
				if (dragging)
				{
					dragging->GetXY(&baseX,&baseY);
					dragging->SetAnchor(mouseX,mouseY);
					SIZING = ((System *)dragging)->GotEdge();
				}
				else
				{
					DRAWING = TRUE;
					drawRect.left = mouseX;
					drawRect.top = mouseY;
					drawRect.right = mouseX;
					drawRect.bottom = mouseY;
					
				}
			}
		}
		break;
	case WM_LBUTTONUP:
		if (linking)
		{
			target = ObjectClicked();

			if (target != linking)
			{
				if (target)
				{
					gateB_arch = current_arch;
					LinkAB(linking,(System *)target);
				}
				linking = NULL;
			}
		}
		if (DRAWING)
		{
			DRAWING = FALSE;
			if (availableSystemIDs == FULL_MAP)
			{
				CQERROR1("Map Is Full! Max systems=%d", MAX_SYSTEMS);
			}
			else
			{
				S32 wl,wt,wr,wb;
				GetWorldCoords(&wl,&wt,drawRect.left,drawRect.top);
				GetWorldCoords(&wr,&wb,drawRect.right,drawRect.bottom);
				CreateSystem(wl,wt,wr,wb)->AssignID();
			}
			dragging = lastSystem;
			baseX = INVALID_BASE;
			baseY = INVALID_BASE;
		}
		if (SIZING)
			((System *)dragging)->ClearSizeBuf();
		dragging = NULL;
		SIZING = FALSE;
		break;
	case WM_RBUTTONUP:
		if (linking == NULL && dragging == NULL)
		{
			target = ObjectClicked();

			if (target)
				if (target->IsSystem())
					((System* )target)->DoDialog();
			
		//	GateLink *whichLink;
			
		//	whichLink = LinkClicked();
			//	if (whichLink)
			//	whichLink->GetMultiplicity();


		}
		if (dragging)
		{
			if (baseX != INVALID_BASE)
			{
				dragging->SetXY(baseX,baseY);
			}
			else
			{
				delete dragging;
			}
			dragging = NULL;
			SIZING = FALSE;
		}
		break;
	case WM_MOUSEMOVE:
		S32 oldX,oldY;//,diffX,diffY;
		
		oldX = mouseX;
		oldY = mouseY;

		mouseX = LOWORD(msg->lParam);  // horizontal position of cursor 
		mouseY = HIWORD(msg->lParam);  // vertical position of cursor

		if (DRAWING)
		{
			S32 diffX = (mouseX - drawRect.left);
			S32 diffY = (mouseY - drawRect.top);
			S32 newSize;

			if (abs(diffX) < abs(diffY))
				newSize = abs(diffY);
			else
				newSize = abs(diffX);

			if (diffX)
				diffX = diffX / abs(diffX);
			if (diffY)
				diffY = diffY / abs(diffY);
			
			drawRect.right = drawRect.left+diffX*newSize;
			drawRect.bottom = drawRect.top+diffY*newSize;
			
		}

	//	diffX = mouseX-oldX;
	//	diffY = mouseY-oldY;
		if (dragging)
		{
			if (SIZING)
				((System *)dragging)->Size(oldX,oldY,mouseX,mouseY);
			else
				dragging->Move(mouseX,mouseY);
		}

		if (bDroppingGates)
			setCursor(IDC_CURSOR_PLACEOBJECT);
		break;
	case WM_COMMAND:
		switch (msg->wParam)
		{
		case IDM_RUSE_RESET:
			if (!LoadMap())
				CQBOMB0("Error loading map");
			break;
		case IDM_RUSE_SAVE:
			SaveMap();
			break;	  
		case IDM_EDITOR_MODE:
			bRuseEditorMode = !bRuseEditorMode;
			ShowWindow(hDlgBox, (bRuseEditorMode)?SW_SHOW:SW_HIDE);
			break;
/*
		case IDM_RUSE_EXITMAPEDITOR:
			if (bMapChanged)
				SaveMap();
			DeactivateRUSE();
			break;
*/
		}
	}
	
	return GR_OK;
}
//--------------------------------------------------------------------------
//
//  jy testing!!! 
//  NOTE: set cursor to windows cursor on activate
// 			SetClassLong(hMainWindow, GCL_HCURSOR,(LONG)LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW)));
// 	SetMenu(hMainWindow, hMenu);

//--------------------------------------------------------------------------
//  
void __stdcall ActivateRUSE (IFileSystem * outFile)
{
	CQASSERT(HEAP->EnumerateBlocks());

	if (hMenu==0)
		hMenu = LoadMenu(hResource, MAKEINTRESOURCE(IDR_MENU2));
	if (hOldMenu==0)
		hOldMenu = MENU->GetMainMenu();
	MENU->SetMainMenu(hMenu);

	DEFAULTS->GetDefaults()->bEditorMode = 0;
	EVENTSYS->Send(CQE_EDITOR_MODE, 0);
	CheckMenuItem(hOldMenu, IDM_EDITOR_MODE, MF_BYCOMMAND);

	g_OutFile = outFile;
	outFile->AddRef();
	HOTKEY->Disable();

	if (EVENT==0)
	{
		EVENT = new DAComponent<EventCallback>;
		AddToGlobalCleanupList(&EVENT);
		AddToGlobalCleanupList(&RHOTKEY);
	}

	if (RHOTKEY==0)
		LoadHotkeys();

	COMPTR<IDAConnectionPoint> connection;

	if (FULLSCREEN && FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Advise(EVENT, &eventHandle);

	if (!LoadMap())
		CQBOMB0("Error loading map");

	CQFLAGS.bRuseActive = 1;

	bRuseEditorMode = 0;

	sysTextureID = TMANAGER->CreateTextureFromFile("sys_circle.tga",TEXTURESDIR,DA::TGA,PF_4CC_DAA4);

	hDlgBox = CreateDialogParam(hResource, MAKEINTRESOURCE(IDD_DIALOG12), hMainWindow, loadArchDlgProc, NULL);//(LPARAM) this);

	CQASSERT(HEAP->EnumerateBlocks());

}
//--------------------------------------------------------------------------
//  
BOOL32 __stdcall DeactivateRUSE (void)
{
	CQASSERT(HEAP->EnumerateBlocks());

	if (bMapChanged)
	switch (MessageBox(hMainWindow, "Save map data?", "Unsaved Data", MB_YESNOCANCEL|MB_ICONEXCLAMATION))
	{
	case IDYES:
		SaveMap();
		break;
	case IDCANCEL:
		return 0;
	}
	
	MENU->SetMainMenu(hOldMenu);
	hOldMenu = 0;
	KillMap();
	if (g_OutFile)
	{
		g_OutFile->Release();
		g_OutFile = 0;
	}
	HOTKEY->Enable();

	COMPTR<IDAConnectionPoint> connection;
	if (FULLSCREEN && FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);
	eventHandle = 0;
	bMapChanged = 0;

	CQFLAGS.bRuseActive = 0;

	if (hDlgBox)
	{
		ShowWindow(hDlgBox, SW_HIDE);

		SendMessage(hDlgBox, WM_CLOSE, 0, 0);
		hDlgBox = 0;
	}

	TMANAGER->ReleaseTextureRef(sysTextureID);
	sysTextureID=0;

	CQASSERT(HEAP->EnumerateBlocks());

	return 1;
}


//----------------------------------------------------------------------------------
//------------------------END Ruse.cpp----------------------------------------------
//----------------------------------------------------------------------------------
