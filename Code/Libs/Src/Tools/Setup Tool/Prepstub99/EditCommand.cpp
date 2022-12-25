/**************************************************************************
* 
* EditCommand.cpp
* 
* Created 3/24/98 by Chris N. Haddan
* 
* (C) 1998 Microsoft Corporation
* 
***************************************************************************/
#include "EditCommand.hpp"
#include "CPrepDoc.hpp"
#include "CCheckBoxListView.hpp"

extern CPrepDoc *g_PrepDoc;


bool SetFlagFromState (DWORD *dwFlags, DWORD dwItemFlag, int nValue)
{
	// if the value is 2, then it is a tri-state value, and we
	// just leave the flags alone. Otherwise we update 
	// the flags based on the new value.  return value denotes if something
	// changed.

	bool bChanged = false;

	if (nValue != 2)
	{
		if (nValue == 1)
		{
			if ((*dwFlags & dwItemFlag) == FALSE) 
				bChanged = true;

			// set the flag
			*dwFlags |= dwItemFlag;
		}
		else
		if (nValue == 0)
		{
			if ((*dwFlags & dwItemFlag) != 0) 
				bChanged = true;

			// clear the flag
			*dwFlags &= ~dwItemFlag;
		}
	}
	return (bChanged);
}


bool SetCommandInfoInListView (HWND hwndLV, STATEINFO *StateInfo, STATEINFO *GroupStateInfo)
{
	int nItem;
	LV_ITEM lvi;
	CCommand *pCmd;
	bool bChanged = false;

	// -1 tells GetNextItem to find the first matching item in the list.
	nItem = -1;

	// Ask the List View control which items are selected.
	while ((nItem = ListView_GetNextItem (hwndLV, nItem, LVNI_SELECTED)) != -1)
	{
		// get the user data from the list view for the given item.
		ZeroMemory (&lvi, sizeof (LV_ITEM));
		lvi.mask = LVIF_PARAM;
		lvi.iItem = nItem;
		lvi.iSubItem = 0;

		ListView_GetItem (hwndLV, &lvi);

		pCmd = (CCommand *)lvi.lParam;

		if (pCmd->IsValidToken())
		{

			// get current flag settings
			DWORD dwFlags = pCmd->GetBuildFlags();
			
			__int64 iGroup;
			
			if (pCmd->GetCommandType() == TOK_MKDIR)
			{
				iGroup = pCmd->GetDirGroup();
			}
			else
			{
				iGroup = pCmd->GetGroup();
			}

			// modify if necessary
			bChanged |= SetBuildFlagsFromTriState (&dwFlags, StateInfo);
			bChanged |= SetGroupFlagsFromTriState (&iGroup, GroupStateInfo);

			// write back the changes
			pCmd->SetBuildFlags (dwFlags);
			if (pCmd->GetCommandType() == TOK_MKDIR)
			{
				pCmd->SetDirGroup (iGroup);
			}
			else
			{
				pCmd->SetGroup (iGroup);
			}
		}
	}
	return (bChanged);
}


void SetCommandInfoFromListView (HWND hwndLV, CMD_INFO *ci, STATEINFO *StateInfo, STATEINFO *GroupStateInfo)
{
	int nItem;
	LV_ITEM lvi;
	CCommand *pCmd;

	nItem = -1;
	
	// Ask the List View control which items are selected.
	while ((nItem = ListView_GetNextItem (hwndLV, nItem, LVNI_SELECTED)) != -1)
	{
		// get the user data from the list view for the given item.
		ZeroMemory (&lvi, sizeof (LV_ITEM));
		lvi.mask = LVIF_PARAM;
		lvi.iItem = nItem;
		lvi.iSubItem = 0;

		ListView_GetItem (hwndLV, &lvi);

		pCmd = (CCommand *)lvi.lParam;
		
		if (pCmd->IsValidToken())
		{

			DWORD dwFlags = pCmd->GetBuildFlags();
			__int64 iGroup;
			if (pCmd->GetCommandType() == TOK_MKDIR)
			{
				iGroup = pCmd->GetDirGroup();
			}
			else
			{
				iGroup = pCmd->GetGroup();
			}

			InitializeStateFromData (dwFlags, StateInfo);
			InitializeGroupStateFromData (iGroup, GroupStateInfo);
		}
	}

	if (ListView_GetSelectedCount (hwndLV) == 1)
		lstrcpy (ci->szToken, Keywords[pCmd->GetCommandType()].pszKeyword );
	else
		lstrcpy (ci->szToken, "Multiple tokens selected...");
}




BOOL CALLBACK EditCommandDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static CMD_INFO ci;
	static STATEINFO StateInfo[NUM_FLAGS];
	static STATEINFO GroupStateInfo[64];

	static HWND hwndLV;
	static CCheckBoxListView *clvOS;     
	static CCheckBoxListView *clvLang; 
	static CCheckBoxListView *clvLoc;	
	static CCheckBoxListView *clvGroups;

	switch (msg)
	{
		case WM_INITDIALOG:
			{
				ClearStateInfo (StateInfo);
				ClearGroupStateInfo (GroupStateInfo);

				hwndLV = (HWND) lParam;  // data from CCommandListView

				clvOS	= new CCheckBoxListView;
				
				if (clvOS->Create (g_hAppInst, GetDlgItem (hWnd, IDC_OS)))
				{
					clvOS->AddColumn ("",80);
					clvOS->AddColumn ("",20);
				}

				clvLang = new CCheckBoxListView;
				if (clvLang->Create (g_hAppInst, GetDlgItem (hWnd, IDC_LANG)))
				{
					clvLang->AddColumn ("",80);
					clvLang->AddColumn ("",20);
				}
	
				clvLoc	= new CCheckBoxListView;
				if (clvLoc->Create (g_hAppInst, GetDlgItem (hWnd, IDC_LOC)))
				{
					clvLoc->AddColumn ("",80);
					clvLoc->AddColumn ("",20);
				}

				clvGroups = new CCheckBoxListView;
				if (clvGroups->Create (g_hAppInst, GetDlgItem (hWnd, IDC_GROUPS)))
				{
					clvGroups->AddColumn ("",100);
					clvGroups->AddColumn ("",20);
				}

				AddCheckBoxImageList(clvOS->GetLvHwnd());
				AddCheckBoxImageList(clvLang->GetLvHwnd());
				AddCheckBoxImageList(clvLoc->GetLvHwnd());
				AddCheckBoxImageList(clvGroups->GetLvHwnd());

				SetCommandInfoFromListView (hwndLV, &ci, StateInfo, GroupStateInfo);
				
				for (int i=0; i<64; i++)
				{
					clvGroups->AddItem (&GroupStateInfo[i]);
					
				}

				clvOS->AddItem (&StateInfo[BF_WIN95]);
				clvOS->AddItem (&StateInfo[BF_WIN98]);
				clvOS->AddItem (&StateInfo[BF_NT40]);
				clvOS->AddItem (&StateInfo[BF_NT50]);

				
				clvLang->AddItem (&StateInfo[BF_JPN]);
				clvLang->AddItem (&StateInfo[BF_GER]);
				clvLang->AddItem (&StateInfo[BF_FRA]);
				clvLang->AddItem (&StateInfo[BF_SPA]);
				clvLang->AddItem (&StateInfo[BF_USA]);


				clvLoc->AddItem (&StateInfo[BF_DBCS]);
				clvLoc->AddItem (&StateInfo[BF_OEM]);
				clvLoc->AddItem (&StateInfo[BF_RTL]);
				clvLoc->AddItem (&StateInfo[BF_APP1]);
				clvLoc->AddItem (&StateInfo[BF_APP2]);
				clvLoc->AddItem (&StateInfo[BF_APP3]);
				clvLoc->AddItem (&StateInfo[BF_IMEON]);
				clvLoc->AddItem (&StateInfo[BF_IMEENABLE]);
				clvLoc->AddItem (&StateInfo[BF_CABPRECOPY]);

				clvOS->Refresh ();
				clvLang->Refresh ();
				clvLoc->Refresh ();
				clvGroups->Refresh ();

				return true;
			}
		case WM_COMMAND:
			switch (wParam) 
			{
				case IDCANCEL:
					RemoveCheckBoxImageList(clvOS->GetLvHwnd());
					RemoveCheckBoxImageList(clvLang->GetLvHwnd());
					RemoveCheckBoxImageList(clvLoc->GetLvHwnd());
					RemoveCheckBoxImageList(clvGroups->GetLvHwnd());
					clvOS->Delete();
					clvLang->Delete();
					clvLoc->Delete();
					clvGroups->Delete();
					delete clvOS;
					delete clvLang;
					delete clvLoc;
					delete clvGroups;

					EndDialog (hWnd, 0);
					break;

				case IDOK:
					// put any changes back into the CCommandListView
					if (SetCommandInfoInListView (hwndLV, StateInfo, GroupStateInfo))
						g_PrepDoc->SetListDirtyState(true);

					UpdateWindowText();
					RemoveCheckBoxImageList(clvOS->GetLvHwnd());
					RemoveCheckBoxImageList(clvLang->GetLvHwnd());
					RemoveCheckBoxImageList(clvLoc->GetLvHwnd());
					RemoveCheckBoxImageList(clvGroups->GetLvHwnd());
					clvOS->Delete();
					clvLang->Delete();
					clvLoc->Delete();
					clvGroups->Delete();
					delete clvOS;
					delete clvLang;
					delete clvLoc;
					delete clvGroups;
					EndDialog (hWnd, 1);
					break;
			}
			return 0;
	}
	return 0;
}




void ClearStateInfo (STATEINFO *si)
{
	for (int i = 0; i < NUM_FLAGS; i++)
	{
		si[i].cStates = 0;
		si[i].nState  = -1;
		SetFlagNames ((BuildFlag) i, si);
	}
}

void ClearGroupStateInfo (STATEINFO *si)
{
	char buffer[35];
	for (int i = 0; i < 64; i++)
	{
		si[i].cStates = 0;
		si[i].nState  = -1;
		wsprintf (buffer, "Install Group #%d", i+1);
		lstrcpy (si[i].szName, buffer);
	}
}


bool Set64FlagFromState (__int64 *dwFlags, __int64 dwItemFlag, int nValue)
{
	bool bChanged = false;
	// if the value is 2, then it is a tri-state value, and we
	// just leave the flags alone. Otherwise we update 
	// the flags based on the new value.
	
	if (nValue != 2)
	{
		if (nValue == 1)
		{
			if ((*dwFlags & dwItemFlag) == FALSE)
				bChanged = true;

			// set the flag
			*dwFlags |= dwItemFlag;
		}
		else
		{
			if ((*dwFlags & dwItemFlag) != 0)
				bChanged = true;

			// clear the flag
			*dwFlags &= ~dwItemFlag;
		}
	}
	return (bChanged);
}


bool SetGroupFlagsFromTriState (__int64	*iGroup, STATEINFO *si)
{
	bool bChanged = false;

	for (int i=0; i< 64; i++)
	{
		bChanged |= Set64FlagFromState (iGroup, ((__int64)1 << (__int64)i), si[i].nState);

	}
	return (bChanged);
}


bool SetBuildFlagsFromTriState (DWORD *dwFlags, STATEINFO *si)
{
	bool bChanged = false;

	bChanged |= SetFlagFromState (dwFlags,	OS_WIN95,		si[BF_WIN95].nState);
	bChanged |= SetFlagFromState (dwFlags,	OS_WIN98,		si[BF_WIN98].nState);
	bChanged |= SetFlagFromState (dwFlags,	OS_NT40,		si[BF_NT40].nState);
	bChanged |= SetFlagFromState (dwFlags,	OS_NT50,		si[BF_NT50].nState);
	bChanged |= SetFlagFromState (dwFlags,	BLD_DBCS,		si[BF_DBCS].nState);
	bChanged |= SetFlagFromState (dwFlags,	BLD_OEM,		si[BF_OEM].nState);
	bChanged |= SetFlagFromState (dwFlags,	BLD_RTL,		si[BF_RTL].nState);
	bChanged |= SetFlagFromState (dwFlags,	BLD_JPN,		si[BF_JPN].nState);
	bChanged |= SetFlagFromState (dwFlags,	BLD_GER,		si[BF_GER].nState);
	bChanged |= SetFlagFromState (dwFlags,	BLD_FRA,		si[BF_FRA].nState);
	bChanged |= SetFlagFromState (dwFlags,	BLD_SPA,		si[BF_SPA].nState);
	bChanged |= SetFlagFromState (dwFlags,	BLD_USA,		si[BF_USA].nState);
	bChanged |= SetFlagFromState (dwFlags,	BLD_APP1,		si[BF_APP1].nState);
	bChanged |= SetFlagFromState (dwFlags,	BLD_APP2,		si[BF_APP2].nState);
	bChanged |= SetFlagFromState (dwFlags,	BLD_APP3,		si[BF_APP3].nState);
	bChanged |= SetFlagFromState (dwFlags,	SCF_IME_ON,		si[BF_IMEON].nState);
	bChanged |= SetFlagFromState (dwFlags,	SCF_IME_ENABLE,	si[BF_IMEENABLE].nState);
	bChanged |= SetFlagFromState (dwFlags,	BLD_CAB_PRECOPY,si[BF_CABPRECOPY].nState);

	
	return (bChanged);
}


BOOL CheckFlagsState (BuildFlag bf, DWORD dwFlags)
{
	switch (bf)
	{
		case BF_WIN95:
			return dwFlags & OS_WIN95;
		case BF_WIN98:
			return dwFlags & OS_WIN98;
		case BF_NT40:
			return dwFlags & OS_NT40;
		case BF_NT50:
			return dwFlags & OS_NT50;
		case BF_DBCS:
			return dwFlags & BLD_DBCS;
		case BF_OEM:
			return dwFlags & BLD_OEM;
		case BF_RTL:
			return dwFlags & BLD_RTL;
		case BF_JPN:
			return dwFlags & BLD_JPN;
		case BF_GER:
			return dwFlags & BLD_GER;
		case BF_FRA:
			return dwFlags & BLD_FRA;
		case BF_SPA:
			return dwFlags & BLD_SPA;
		case BF_USA:
			return dwFlags & BLD_USA;
		case BF_APP1:
			return dwFlags & BLD_APP1;
		case BF_APP2:
			return dwFlags & BLD_APP2;
		case BF_APP3:
			return dwFlags & BLD_APP3;
		case BF_IMEON:
			return dwFlags & SCF_IME_ON;
		case BF_IMEENABLE:
			return dwFlags & SCF_IME_ENABLE;
		case BF_CABPRECOPY:
			return dwFlags & BLD_CAB_PRECOPY;
		default:
			//ASSERT (FALSE);
			return false;
	}
}


void InitializeStateFromData (DWORD dwFlags, STATEINFO *si)
{
	int i;

	for (i=0; i < NUM_FLAGS; i++)
	{
		if (si[i].nState == -1)
		{
			si[i].nState = (CheckFlagsState ((BuildFlag)i, dwFlags) > 0);
			si[i].cStates = 2;
		}
		else
		{
			if (si[i].nState != (CheckFlagsState ((BuildFlag)i, dwFlags) > 0))
			{
				si[i].nState  = 2;
				si[i].cStates = 3;
			}
		}
	}
}


void InitializeGroupStateFromData (__int64 iGroup, STATEINFO *si)
{
	int i;

	for (i=0; i < 64; i++)
	{
		if (si[i].nState == -1)
		{
			si[i].nState = ((((__int64)1 << (__int64)i) & iGroup) != 0);
			si[i].cStates = 2;
		}
		else
		{
			if (si[i].nState != ((((__int64)1 << (__int64)i) & iGroup) != 0))
			{
				si[i].nState  = 2;
				si[i].cStates = 3;
			}
		}
	}
}


void SetFlagNames (BuildFlag bf, STATEINFO *si)
{
	// bugbug:: these strings should be moved to the string table.
	//

	switch (bf)
	{
		case BF_WIN95:
			lstrcpy (si[bf].szName, "Windows 95");
			return;
		case BF_WIN98:
			lstrcpy (si[bf].szName, "Windows 98");
			return;
		case BF_NT40:
			lstrcpy (si[bf].szName, "WinNT 4.0");
			return;
		case BF_NT50:
			lstrcpy (si[bf].szName, "WinNT 5.0");
			return;
		case BF_DBCS:
			lstrcpy (si[bf].szName, "DBCS Enabled.");
			return;
		case BF_OEM:
			lstrcpy (si[bf].szName, "OEM");
			return;
		case BF_RTL:
			lstrcpy (si[bf].szName, "Retail");
			return;
		case BF_JPN:
			lstrcpy (si[bf].szName, "Japan");
			return;
		case BF_GER:
			lstrcpy (si[bf].szName, "Germany");
			return;
		case BF_FRA:
			lstrcpy (si[bf].szName, "France");
			return;
		case BF_SPA:
			lstrcpy (si[bf].szName, "Spain");
			return;
		case BF_USA:
			lstrcpy (si[bf].szName, "USA");
			return;
		case BF_APP1:
			lstrcpy (si[bf].szName, "App Flag #1");
			return;
		case BF_APP2:
			lstrcpy (si[bf].szName, "App Flag #2");
			return;
		case BF_APP3:
			lstrcpy (si[bf].szName, "App Flag #3");
			return;
		case BF_IMEON:
			lstrcpy (si[bf].szName, "IME On");
			return;
		case BF_IMEENABLE:
			lstrcpy (si[bf].szName, "IME Enabled");
			return;
		case BF_CABPRECOPY:
			lstrcpy (si[bf].szName, "Cab (Precopy)");
			return;
		default:
			//ASSERT (FALSE);
			return;
	}
}


