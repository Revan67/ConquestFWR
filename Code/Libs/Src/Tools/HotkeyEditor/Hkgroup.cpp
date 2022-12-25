//--------------------------------------------------------------------------//
//                                                                          //
//                               HKGroup.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1996 BY ORIGIN SYSTEMS, INC.              //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author:   JYENAWINE  $
*/			    
//--------------------------------------------------------------------------//
//------------------------------- #INCLUDES --------------------------------//

#include <windows.h>
#include <commctrl.h>

#include "HKeyRec.h"
#include "HKGroup.h"
#include "resource.h"
#include "ferror.h"

static HKGROUP * lpCurrentHKG=0;		// pointer to HKGROUP being created
BOOL CALLBACK HotKeyDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
BOOL ModifyHKRecord (HKRECORD *record, DWORD dwNumber, BOOL bUpdateUndo=1);
BOOL InsertHKRecord (HKRECORD *record, DWORD dwNumber, BOOL bUpdateUndo=1);
BOOL AddSymbolsToListBox (HWND hListbox);

DWORD dwRawHotkeySize=0;

BOOL bGlobalRecording=0;
int HKRECORD::iSortColumn=0;
extern ICOManager * DACOM;

//--------------------------------------------------------------------------//
// returns:
//   < 0 if we are less than other guy
//   > 0 if we are greater than other guy
//   ==0 if we are equal to other guy
//
int HKRECORD::compare (HKRECORD & rec)
{
	switch (iSortColumn)
	{
		case 1:		// key combo
			return strcmp(szKeyCombo, rec.szKeyCombo);
		case 2:		// description
			return strcmp(szDescription, rec.szDescription);
	}

	return strcmp(szName, rec.szName);
}
//--------------------------------------------------------------------------//
//---------------------------HKGroup methods--------------------------------//
//--------------------------------------------------------------------------//
//
HKGROUP::HKGROUP (void)
{
	memset(this, 0, sizeof(*this));
}
//--------------------------------------------------------------------------//
//
HKGROUP::~HKGROUP (void)
{
	HKGROUP *ptr = lpCurrentHKG;
	
	if (ptr == this)
	{
		lpCurrentHKG = next;
	}
	else
	while (ptr && ptr->next)
	{
		if (ptr->next == this)
		{
			ptr->next = next;
			break;
		}
	}

	if (hkmanager)
	{
		if (hHKey)
			hkmanager->DestroyHotkey(hHKey);
		hkmanager->Release();
	}

	if (hRecWnd)
		DestroyWindow(hRecWnd);

	if (hDlg)
		DestroyWindow(hDlg);
}
//--------------------------------------------------------------------------//
//
BOOL HKGROUP::init (HWND hParent,  HKRECORD *lpRec, DWORD dwItem, ACTION _action)
{
	next = lpCurrentHKG;
	lpCurrentHKG = this;
	hkrecord = *lpRec;
	dwEditItem = dwItem;
	action = _action;

	if ((hDlg = CreateDialog (hInstance, MAKEINTRESOURCE(IDD_HOTKEY), hParent, HotKeyDlgProc)) == 0)
		Fatal("Could not load HotKey dialog.");

	return 1;
}
static BOOL bRecurse=0;

/*
//--------------------------------------------------------------------------//
//
static void enableIMC (HWND hwnd, bool bEnable)
{
	static HIMC hIMC;

	if (bEnable)
	{
		if (hIMC)
		{
			ImmAssociateContext(hwnd, hIMC);
	        ImmNotifyIME( hIMC, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
			hIMC = 0;
		}
	}
	else
	{
		HIMC _hIMC = ImmAssociateContext(hwnd, 0);

		if (hIMC==0)
			hIMC = _hIMC;
	}
}
*/
//--------------------------------------------------------------------------//
//
BOOL HKGROUP::update (void)
{
	HWND hItem;
	DAHOTKEY hTemp;
	byte buffer[256];

	switch (bStatus)
	{
		case 1:			// OK pressed
			// pull info out of Dialog
			hItem = GetDlgItem(hDlg, IDC_SYMBOLNAME);
			GetWindowText(hItem, hkrecord.szName, sizeof(hkrecord.szName)-1);
			hItem = GetDlgItem(hDlg, IDC_HOTKEYTEXT);
			GetWindowText(hItem, hkrecord.szKeyCombo, sizeof(hkrecord.szKeyCombo)-1);
			hItem = GetDlgItem(hDlg, IDC_ACTION);
			GetWindowText(hItem, hkrecord.szDescription, sizeof(hkrecord.szDescription)-1);
			hkmanager->SetTriggerType(hHKey, (IsDlgButtonChecked(hDlg, IDC_PRESS_EVENT))? HKR_PRESSED  : HKR_RELEASED );
			dwRawHotkeySize = hkmanager->GetData(hHKey, (byte *)hkrecord.dwKeyCombo, sizeof(hkrecord.dwKeyCombo));
			switch (action)
			{
				case Insert:
					InsertHKRecord(&hkrecord, dwEditItem);
					break;
				case Replace:
					ModifyHKRecord(&hkrecord, dwEditItem);
					break;
			}
			return 0;

		case 2:			// CANCEL pressed
			return 0;
	}

	if (bRecording)
	{
		bGlobalRecording = 1;
		hItem = GetDlgItem(hDlg, IDC_HOTKEYTEXT);

		memset(buffer, 0, sizeof(buffer));
		hkmanager->GetData(hHKey, buffer, sizeof(buffer));
		hTemp = hkmanager->CreateHotkey(buffer);
		hkmanager->ReadEvents(hTemp, (IsDlgButtonChecked(hDlg, IDC_SEQUENCE))?HKRF_FORCE_VKEY:0);
	
		if (hkmanager->GetNumKeys(hTemp) < hkmanager->GetNumKeys(hHKey))	// user released something
		{
		 	bRecording = 0;
		 	hkmanager->DestroyHotkey(hTemp);
			if (hkmanager->ConvertToString(hHKey, szText, sizeof(szText)))
			{
				bRecurse++;
				SetWindowText(hItem, szText);
				bRecurse--;
			}
			CheckDlgButton(hDlg, IDC_SEQUENCE, (hkmanager->GetVirtualKey(hHKey) != 0));
			SetFocus(GetDlgItem(hDlg, IDC_RECORD));
		}
		else
		{
			if (hkmanager->IsEqual(hHKey, hTemp)==0)
			{
				if (hkmanager->ConvertToString(hTemp, szText, sizeof(szText)))
				{
					bRecurse++;
					SetWindowText(hItem, szText);
					bRecurse--;
				}
			}
		 	hkmanager->DestroyHotkey(hHKey);
			hHKey = hTemp;
		}
	}

	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL HKGROUP::Update (void)
{
	HKGROUP *ptr = lpCurrentHKG, *tmp;
	bGlobalRecording = 0;

	while (ptr)
	{
		if (ptr->update()==0)
		{
			tmp = ptr->next;
		 	delete ptr;
			ptr = tmp;
		}
		else
			ptr = ptr->next;
	}
	return 1;
}
//--------------------------------------------------------------------------//
// return TRUE if message handled
//
BOOL HKGROUP::Translate (MSG & msg)
{
	HKGROUP *ptr = lpCurrentHKG;

	while (ptr)
	{
		if (IsDialogMessage(ptr->hDlg, &msg))
			return 1;
		ptr = ptr->next;
	}

	return 0;
}
//--------------------------------------------------------------------------//
// bring box to the surface if true
//
BOOL HKGROUP::IsBeingEdited (DWORD dwItem)
{
	HKGROUP *ptr = lpCurrentHKG;

	while (ptr)
	{
		if (ptr->action==Replace && dwItem == ptr->dwEditItem)
		{
			return SetForegroundWindow(ptr->hDlg);
		}
		ptr = ptr->next;
	}
	
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL HKGROUP::AnyEditUnderway (void)
{
	if (lpCurrentHKG)
	{
		WarningBox(hMainWindow, IDS_STRING11);
		return 1;
	}
	return 0;
}
//--------------------------------------------------------------------------//
// bring box to the surface if true
//
BOOL HKGROUP::InsertUnderway (void)
{
	HKGROUP *ptr = lpCurrentHKG;

	while (ptr)
	{
		if (ptr->action == Insert)
			return SetForegroundWindow(ptr->hDlg);
		ptr = ptr->next;
	}
	
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL HKGROUP::NewRecordWasInserted (DWORD dwNum)
{
	HKGROUP *ptr = lpCurrentHKG;
	BOOL result=0;

	while (ptr)
	{
		if (ptr->dwEditItem >= dwNum)
		{
			result=1;
			ptr->dwEditItem++;
		}
		ptr = ptr->next;
	}
	
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL HKGROUP::RecordWasDeleted (DWORD dwNum)
{
	HKGROUP *ptr = lpCurrentHKG, *tmp;
	BOOL result=0;

	while (ptr)
	{
		if (ptr->dwEditItem >= dwNum)
		{
			if (ptr->action==Replace && ptr->dwEditItem == dwNum)
			{
				tmp = ptr->next;
				delete ptr;
				ptr = tmp;
			}
			else
			{
				ptr->dwEditItem--;
				ptr = ptr->next;
			}
			result=1;
		}
		else
			ptr = ptr->next;
	}
	
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL CALLBACK HotKeyDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	HWND hItem;
	HKGROUP *lpGroup;

	switch (message)
	{
		case WM_INITDIALOG:
			SetWindowLong(hwnd, GWL_USERDATA, (LONG) lpCurrentHKG); 
			// setup the instance stuff here
			if (lpCurrentHKG)
			{
				HKRECDESC desc;
				RECT editRect;
				//RECT wndRect;

				GetWindowRect(GetDlgItem(hwnd, IDC_HOTKEYTEXT), &editRect);
				//GetWindowRect(hwnd, &wndRect);
				ScreenToClient(hwnd, (POINT *)&editRect.left);
				ScreenToClient(hwnd, (POINT *)&editRect.right);
//				editRect.left -= wndRect.left;
//				editRect.right -= wndRect.left;
//				editRect.top -= wndRect.top;
//				editRect.bottom -= wndRect.top;

				lpCurrentHKG->hRecWnd = CreateWindow(szRecName, szRecName, WS_CHILD,
									editRect.left, editRect.top, editRect.right-editRect.left, editRect.bottom-editRect.top, 
									hwnd, 0, hInstance, hwnd);
				desc.hWindow = lpCurrentHKG->hRecWnd;
				DACOM->CreateInstance(&desc, (void **) &lpCurrentHKG->hkmanager);
				hItem = GetDlgItem(hwnd, IDC_SYMBOLNAME);
				SetWindowText(hItem, lpCurrentHKG->hkrecord.szName);
				AddSymbolsToListBox(hItem);
				HFONT hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
				hItem = GetDlgItem(hwnd, IDC_HOTKEYTEXT);
				SendMessage(hItem, WM_SETFONT, (UINT)hFont, 0);
				bRecurse++;
				SetWindowText(hItem, lpCurrentHKG->hkrecord.szKeyCombo);
				bRecurse--;
				hItem = GetDlgItem(hwnd, IDC_ACTION);
				SendMessage(hItem, WM_SETFONT, (UINT)hFont, 0);
				SetWindowText(hItem, lpCurrentHKG->hkrecord.szDescription);
				lpCurrentHKG->hHKey = lpCurrentHKG->hkmanager->CreateHotkey((byte*)lpCurrentHKG->hkrecord.dwKeyCombo);
				CheckDlgButton(hwnd, IDC_SEQUENCE, (lpCurrentHKG->hkmanager->GetVirtualKey(lpCurrentHKG->hHKey) != 0));
				if (lpCurrentHKG->hkmanager->GetTriggerType(lpCurrentHKG->hHKey) == HKR_PRESSED)
					CheckRadioButton(hwnd, IDC_PRESS_EVENT, IDC_RELEASE_EVENT, IDC_PRESS_EVENT);
				else
					CheckRadioButton(hwnd, IDC_PRESS_EVENT, IDC_RELEASE_EVENT, IDC_RELEASE_EVENT);
				switch (lpCurrentHKG->hkmanager->GetVirtualKey(lpCurrentHKG->hHKey))
				{
				 	case HKR_VKEY_ALPHA:
						CheckRadioButton(hwnd, IDC_NOTHING, IDC_SINGLE_KEY, IDC_ANY_ALPHA);
						break;
					case HKR_VKEY_NUMBER:
						CheckRadioButton(hwnd, IDC_NOTHING, IDC_SINGLE_KEY, IDC_ANY_NUMBER);
						break;
					case HKR_VKEY_OTHER:
						CheckRadioButton(hwnd, IDC_NOTHING, IDC_SINGLE_KEY, IDC_OTHER_KEYS);
						break;
					case HKR_VKEY_SINGLE:
						CheckRadioButton(hwnd, IDC_NOTHING, IDC_SINGLE_KEY, IDC_SINGLE_KEY);
						break;
					default:
						CheckRadioButton(hwnd, IDC_NOTHING, IDC_SINGLE_KEY, IDC_NOTHING);
						break;
				}


				if (lpCurrentHKG->hkrecord.szName[0])
					SetFocus(GetDlgItem(hwnd, IDC_SYMBOLNAME));
				else
					SetFocus(GetDlgItem(hwnd, IDC_RECORD));
			}
			return 1;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_NOTHING:
				case IDC_ANY_ALPHA:
				case IDC_ANY_NUMBER:
				case IDC_OTHER_KEYS:
				case IDC_SINGLE_KEY:
					if ((lpGroup = (HKGROUP *) GetWindowLong(hwnd, GWL_USERDATA)) != 0)
					{
						lpGroup->hkmanager->SetEmpty(lpGroup->hHKey);
						if (LOWORD(wParam) != IDC_NOTHING)
							lpGroup->hkmanager->SetVirtualKey(lpGroup->hHKey, HKR_VKEY_ALPHA + LOWORD(wParam) - IDC_ANY_ALPHA);
						lpGroup->szText[0] = 0;
						bRecurse++;
						SetWindowText(GetDlgItem(hwnd, IDC_HOTKEYTEXT), lpGroup->szText);
						bRecurse--;
					}
					break;

/*
				case IDC_SYMBOLNAME:
					if ((lpGroup = (HKGROUP *) GetWindowLong(hwnd, GWL_USERDATA)) != 0)
					{
						switch (HIWORD(wParam))
						{
							case CBN_EDITUPDATE:
								if (bRecurse==0)
								{
									int i;
									bRecurse++;
									i = GetWindowText((HWND)lParam, lpGroup->hkrecord.szName, sizeof(lpGroup->hkrecord.szName)-1);
									lpGroup->hkrecord.szName[i] = 0;
									char *ptr;
									if ((ptr = strchr(lpGroup->hkrecord.szName, ' ')) != 0)
									{
									 	*ptr = 0;
										SetWindowText((HWND)lParam, lpGroup->hkrecord.szName);
									}
									bRecurse--;
								}
								break;
						}
					}
					break;
*/

				case IDC_RECORD:
					if ((lpGroup = (HKGROUP *) GetWindowLong(hwnd, GWL_USERDATA)) != 0)
					{
						ShowWindow(lpGroup->hRecWnd, SW_SHOW);
						SetFocus(lpGroup->hRecWnd);
					}
					break;

				case IDC_SEQUENCE:
					CheckDlgButton(hwnd, IDC_SEQUENCE, !IsDlgButtonChecked(hwnd, IDC_SEQUENCE));
					SendMessage(hwnd, WM_COMMAND, IDC_RECORD, 0);
					break;

				case IDCANCEL:
					bGlobalRecording = 1;
					if ((lpGroup = (HKGROUP *) GetWindowLong(hwnd, GWL_USERDATA)) != 0)
					{
						if (lpGroup->bRecording==0)
						 	lpGroup->bStatus = 2;
					}
					return 1;

				case IDOK:
					bGlobalRecording = 1;
					if ((lpGroup = (HKGROUP *) GetWindowLong(hwnd, GWL_USERDATA)) != 0)
					{
						if (lpGroup->bRecording==0)
					 		lpGroup->bStatus = 1;
					}
					return 1;
			}
			break;

		case MSG_BEGIN_REC:
			if ((lpGroup = (HKGROUP *) GetWindowLong(hwnd, GWL_USERDATA)) != 0)
			{
				if (lpGroup->bRecording==0)
				{
					lpGroup->bRecording = 1;			// signal waiting for all keys up
					bGlobalRecording = 1;
					hItem = GetDlgItem(hwnd, IDC_HOTKEYTEXT);
					if (lpGroup->hHKey==0)
						lpGroup->hHKey = lpGroup->hkmanager->CreateHotkey();
					else
						lpGroup->hkmanager->SetEmpty(lpGroup->hHKey);
					lpGroup->szText[0] = 0;
					bRecurse++;
					SetWindowText(hItem, lpGroup->szText);
					bRecurse--;
					CheckRadioButton(hwnd, IDC_NOTHING, IDC_SINGLE_KEY, IDC_NOTHING);

					ShowWindow(lpGroup->hRecWnd, SW_SHOW);
				}
			}
			break;

		case MSG_END_REC:
			if ((lpGroup = (HKGROUP *) GetWindowLong(hwnd, GWL_USERDATA)) != 0)
			{
				lpGroup->bRecording = 0;
				ShowWindow(lpGroup->hRecWnd, SW_HIDE);
			}
			break;
	}
	return 0;
}
//---------------------------------------------------------------------------------
//----------------------------End HKGroup.cpp--------------------------------------
//---------------------------------------------------------------------------------
