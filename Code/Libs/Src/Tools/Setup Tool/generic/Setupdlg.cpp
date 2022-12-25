#include "windows.h"		    /* required for all Windows applications*/
#include "stdio.h"
#include "string.h"
#include "resource.h"		    /* specific to this program		    */
#include "hotsetuprc.h"
#include "hotsetup.h"
#include "widclass.h"
#include "setupdlg.h"
#include <shlobj.h>
#include "io.h"
#include "gauge.h"
#include "commctrl.h"
#include "checklist.h"

#define NUM_COLUMNS 3

BOOL bDXSkipped=FALSE;
BOOL bInstDX = TRUE;
BOOL bInstalledDX = FALSE;
BOOL g_fDeleteEm = FALSE;
BOOL g_fAskToDelete = TRUE;

extern CGlobals g_Globals;
extern EBURETCODE WINAPI GetGrpFn(LPGETGROUPDATA group);
extern EBURETCODE WINAPI MyMkRootFn(LPMKROOTDATA mk);

using namespace NGLOBALS;

struct restoreAppWindow
{
	public:
		restoreAppWindow(HWND window = GetWndParent()){appWindow = window; restoreWindow = false;};
		~restoreAppWindow() {if (restoreWindow) restoreNow();};
		restoreNow();
		void setState(bool restore) {restoreWindow = restore;};
	protected:
		HWND appWindow;
		bool restoreWindow;
};

restoreAppWindow::restoreNow()
{
	if (IsIconic(appWindow))
		ShowWindow(appWindow, SW_RESTORE);
	
	// Bring the window to the foreground
	SetForegroundWindow(appWindow);
	restoreWindow = false;
}

_declspec(dllexport) EBURETCODE WINAPI MasterCallback(void *cbd);

_declspec(dllexport) EBURETCODE WINAPI MasterCallback(void *cbd)
{
	restoreAppWindow restore;

	switch (((PCALLBACKDATA)cbd)->nID)
	{

	case SS_BEGINUNINSTALL:
		g_fDeleteEm = FALSE;
		g_fAskToDelete = TRUE;
		return EBU_OK;
	break;

	case SC_DELETEFILE:
	{	
		LPDELETEFILEDATA data;
		data = (LPDELETEFILEDATA) cbd;
		if (!data->fPromptToDelete)
		{
			return EBU_OK;
		}
		else
		if (g_fAskToDelete)
		{
			char str[200];
			EBULoadString(GetResourceInst(),STR_DELETE_SAVED_GAMES,str,200);

			g_fDeleteEm = IDYES == MessageBox(GetWndParent(),str,GetAppTitle(), MB_YESNO);
			g_fAskToDelete = FALSE;
		}
		return g_fDeleteEm ? EBU_OK : EBU_CANCEL;
	}	break;

	case SC_INSTALLLIST:
		return EBU_OK; 

//	case SC_INSTALLFONT:
//		return EBU_OK;

	case SC_MKDIR:
		return EBU_OK;

	case SC_MKROOT:
		restore.restoreNow();
		return MyMkRootFn((LPMKROOTDATA) cbd);

	case SC_GETGROUP:
		restore.restoreNow();
		return GetGrpFn((LPGETGROUPDATA) cbd);

   case SC_INSTDX:
		{
			LPDIRECTXDATA dxd;
			
			dxd = (LPDIRECTXDATA) cbd;
			
			switch (dxd->nStatus)
			{
				case QUERYDPLAYINSTALL:
					return InstallDPlay();
				break;
					
				case QUERYDIRECTXINSTALL:
					return EBU_OK;
//					return InstallDirectX(dxd->gUpdates); // this allows user to select specific drivers to keep or update
				break;
					
				case DETECTDIRECTX:
				   return DetectDirectX(&dxd->uExistingVersion);
				break;
				default:
					if (bInstalledDX)
					{
						bInstalledDX = false;
						if ( (DSETUPERR_SUCCESS_RESTART == GetDXReturnCode()) || ( DSETUPERR_SUCCESS == GetDXReturnCode()) )
						{
						   char str[200];
						   EBULoadString(GetResourceInst(),STR_DIRECTX_INSTALLED,str,200);
						   MessageBox(GetWndParent(),str,GetAppTitle(), MB_OK);
						}
					}
				break;
			}
			return EBU_OK;
		}break;

	case SC_INSTICON:
		restore.restoreNow();
		return InstIconFn((LPINSTICONDATA) cbd);

   case SC_REGWIZ:
		return EBU_OK;

   case SC_GETNAME:
		restore.restoreNow();
		return GetNameFn((LPGETNAMEDATA) cbd);

   case SC_GETPID:
		restore.restoreNow();
		{
			LPGETPIDDATA
				tPid = (LPGETPIDDATA) cbd;

			if (!tPid->pszPID)
				// return GetPidFn(tPid);
				return EBU_OK;
			else
				return GetPidNumFn(tPid);
		}break;
	
	case SC_ADDINIVALUE:
		{
			LPADDINIVALUEDATA tIniDat = (LPADDINIVALUEDATA) cbd;
			return EBU_OK;
		}

	case SC_READFILELIST:
		return EBU_OK;

	case SC_INSTALLGO:
	case SC_CABGO:
		if(!((PCALLBACKDATA)cbd)->fUninstall)
		{	
			return CopyFileFn((LPFILECOPYSTATUS) cbd);
		}
		else
		{
			return EBU_OK;
		}

	case SC_SHELLEXECUTE:
		return EBU_OK;

	default:
		return EBU_OK;
	}
}

EBURETCODE WINAPI GetGroupFn(LPGETGROUPDATA group)
{
	EBURETCODE retc= EBU_CANCEL;
	GetGroupDlg *gg = new GetGroupDlg();
	if(gg)
	{
       retc = gg->start((LPARAM)group);
	   delete gg;
	}
	return retc;
}
EBURETCODE WINAPI MkRootFn(LPMKROOTDATA mk)
{
	EBURETCODE retc=EBU_CANCEL;
	MkRootDlg *mkd = new MkRootDlg();
	if(mkd)
	{
       retc = mkd->start((LPARAM)mk);
	   delete mkd;
	}
	return retc;
}


WORD ReallyQuit(HWND hDlg)
{
   char str[200];
   EBULoadString(GetResourceInst(),STR_QUIT_SETUP,str,200);
   return MessageBox(hDlg,str,GetAppTitle(),MB_ICONEXCLAMATION | MB_OKCANCEL);
}

BOOL ForwardMyMessages()
{
	MSG   msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
	{
		if( msg.message == WM_QUIT ||
			msg.message == WM_CLOSE ||
			msg.message == WM_SYSCOMMAND ||
			msg.message == WM_DESTROY )
		{
			// Put the message back on the queue and get out of here.
		    PostMessage( msg.hwnd, msg.message, msg.wParam, msg.lParam );
			return FALSE;
		}
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	return TRUE;
}

EBURETCODE WINAPI CopyFileFn(LPFILECOPYSTATUS lpfs)
{
    static BOOL CopyStatus = IDOK;
	static BOOL Created = FALSE;
	static HWND hWnd;
	static int deltatotal = 0;
	if(!Created)
	{
		SendMessage(GetWndParent(),WM_START_BILLBOARDS,0,0);
		fUserQuit = FALSE;
		ProOpen(GetWndParent());
		ProSetBarRange(10000);
		Created = TRUE;
	}
	int delta = (int)((float)((float)lpfs->dwLastFile/(float)lpfs->dwTotalSize) * 10000.00);
	if(delta < 1)
		delta = 1;
	deltatotal+= delta;
	if(deltatotal > 10000)
		deltatotal = 10000;
	ForwardMyMessages();
//	don't need to display the source file
//	ProSetText(ID_STATUS1,lpfs->szSource);

	// just display the filename
	char * filenameOnly = strrchr( lpfs->szDest, '\\' )+1;
	if (NULL == filenameOnly)
		filenameOnly = lpfs->szDest;
	ProSetText(ID_STATUS2,filenameOnly);
	ProDeltaPos(delta);
	if(fUserQuit)
	{
		SendMessage(GetWndParent(),WM_STOP_BILLBOARDS,0,0);
		ProClose(GetWndParent());
		UpdateWindow(GetWndParent());
		ForwardMyMessages();
		Created = FALSE;
		fUserQuit = FALSE;
		return EBU_ABORT;
	}
	if(lpfs->fDone == TRUE)
	{
		if(deltatotal != 10000)
		{
			ProSetBarPos(10000);
			ForwardMyMessages();
			Sleep(500U);
		}

		SendMessage(GetWndParent(),WM_STOP_BILLBOARDS,0,0);
		ProClose(GetWndParent());
		UpdateWindow(GetWndParent());
		ForwardMyMessages();
		Created = FALSE;
	}
	return EBU_OK;
}
EBURETCODE WINAPI GetNameFn(LPGETNAMEDATA UserName)
{
	EBURETCODE retc;
	GetNameDlg *gn = new GetNameDlg();
	if(gn)
	{
       retc = gn->start((LPARAM)UserName->pszPlayerName);
	   delete gn;
	}
	return retc;

}
EBURETCODE WINAPI InstIconFn(LPINSTICONDATA lpi)
{
	static EBURETCODE Retcode;
	if(lpi->fUninstall)
		return EBU_OK;

	if(lpi->icontype & ICON_DESKTOP)
	{
		char msg[_MAX_PATH];
		if(Told)
			return Retcode;
		EBULoadString(GetResourceInst(),STR_INSTALLDESKTOP,msg,_MAX_PATH);
		Told = TRUE;
		if(MessageBox(GetWndParent(), msg,GetAppTitle(),MB_YESNO) == IDNO)
		{
			Retcode = EBU_CANCEL;
			return EBU_CANCEL;
		}
		Retcode = EBU_OK;
		return EBU_OK;
	}
	return EBU_OK;
}

BOOL DXSetupDlg::Init(LPARAM lParam)
{
	int                  nIndex;
	LPUPDATEARRAY        lpdu;
	BOOL                 fRecommend = FALSE;
	BOOL                 fAutomatic = FALSE;
	UINT
		nRecommendResID,
		nWhatHappenID;


	char                 szMessage[512];
	bDXSkipped = FALSE;
	
	Updates = (LPUPDATEARRAY)lParam;
	lpdu = Updates;
	HWND hWndList = GetDlgItem(hDlg,ID_CHECKLIST);

	Checklist_Init();

	Checklist_OnInitDialog(hWndList);

    RECT rc;
    LV_COLUMN col;

	char szText[100];

	// add the columns to the list view.
    GetClientRect(hWndList, &rc);
    col.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
    col.cx = 100;
	col.fmt = LVCFMT_LEFT;  // left-align column
	col.pszText = szText;

	GetWindowText(hDlg,szMessage,150);
	ReplaceStringTokens(szMessage,150);
	SetWindowText(hDlg,szMessage);

	EBULoadString(GetResourceInst(), STR_DIRECTX_INSTALLTEXT, szMessage, sizeof(szMessage));
	SetWindowText(GetDlgItem(hDlg,IDOK),szMessage);

	EBULoadString(GetResourceInst(), STR_DIRECTX_DONTINSTALLTEXT, szMessage, sizeof(szMessage));
	SetWindowText(GetDlgItem(hDlg,IDCANCEL),szMessage);

	EBULoadString(GetResourceInst(), STR_DIRECTX_INSTALLDXDRIVERS, szMessage, sizeof(szMessage));
	SetDlgItemText(hDlg,IDC_DX_STATIC,szMessage);

	// Add the columns.
	EBULoadString(GetResourceInst(), STR_DIRECTX_DRIVERACTION, szText, sizeof(szText));
	ListView_InsertColumn(hWndList, 0, &col);

	EBULoadString(GetResourceInst(), STR_DIRECTX_RECOMMENDATION, szText, sizeof(szText));
	ListView_InsertColumn(hWndList, 1, &col);

	EBULoadString(GetResourceInst(), STR_DIRECTX_DRIVERNAME, szText, sizeof(szText));
	ListView_InsertColumn(hWndList, 2, &col);

	// Finally, add the actual items to the control.
	// Fill out the LV_ITEM structure for each of the items to add to the list.
	// The mask specifies the the pszText  and state members of the LV_ITEM structure are valid.
	LV_ITEM lvI;
	memset(&lvI, 0, sizeof(LV_ITEM));
	
	
	lvI.mask = LVIF_TEXT | LVIF_STATE;
    lvI.state = INDEXTOSTATEIMAGEMASK(1);	// unchecked
    lvI.stateMask = LVIS_STATEIMAGEMASK;


	int	iStateMaskIndex = 0;

	//For each DirectX driver update array element
	for (nIndex = 0; nIndex < lpdu->numUpdates; nIndex++)
	{
		iStateMaskIndex = 1;	// unchecked

		
		
		//Determine whether DirectX setup recommends that this driver be installed
		switch (GetDefaultButtonResponse(lpdu->UpdateArray[nIndex]->dwMsgType))
		{
		case IDYES:
		case IDOK:
			fRecommend = TRUE;
			iStateMaskIndex = 2;	// checked

			break;
		}
		
		//If DirectX determined that the files absolutely *should* or *should not*
		//be installed, then don't set flag to not give the user a choice.
		if (MB_OK == (lpdu->UpdateArray[nIndex]->dwMsgType & 0x0000000F) ||
			DX_FORCE == lpdu->UpdateArray[nIndex]->eStatus ||
			DX_KEEP == lpdu->UpdateArray[nIndex]->eStatus)
		{
			fAutomatic = TRUE;
			iStateMaskIndex += 2; // bump into the automatic images
		}
	

		
		
		// add the driver line item
		lvI.iItem = nIndex;
		lvI.state = INDEXTOSTATEIMAGEMASK(iStateMaskIndex);
		lvI.pszText = "";		// we are adding the text by sub item
		lvI.cchTextMax = sizeof(szMessage);

		if (ListView_InsertItem(hWndList, &lvI) == -1)
		{
//			MYASSERT(!"ListView_InsertItem failed.");
			return FALSE;
		}

		// now fill out the subitems with the descriptive parts
		
		// add the what will happen text
		nWhatHappenID = fRecommend ? STR_DIRECTX_KEEP : STR_DIRECTX_REPLACE;
		EBULoadString(GetResourceInst(), nWhatHappenID, szMessage, sizeof(szMessage));
		ListView_SetItemText( hWndList, nIndex, 0, szMessage);



		// add the recommendation txt
		if (FALSE == fAutomatic)
		{
			nRecommendResID = (fRecommend) ? (STR_DIRECTX_RECOMMENDED) : (STR_DIRECTX_NOTRECOMMENDED);
		}else{
			nRecommendResID = STR_DIRECTX_AUTOMATIC;
		}

		
		EBULoadString(GetResourceInst(), nRecommendResID, szMessage, sizeof(szMessage));
		ListView_SetItemText( hWndList, nIndex, 1, szMessage);
		
		// add the driver description
		ListView_SetItemText( hWndList, nIndex, 2, lpdu->UpdateArray[nIndex]->szName);
			
	}


	// have all the columns resize as needed
	int
		iColWidth = 0,
		iHeaderWidth = 0;
	for (int index = 0; index < NUM_COLUMNS; index++)
	{
		ListView_SetColumnWidth(hWndList, index, LVSCW_AUTOSIZE);
		iColWidth = ListView_GetColumnWidth(hWndList, index);

		ListView_SetColumnWidth(hWndList, index, LVSCW_AUTOSIZE_USEHEADER);
		iHeaderWidth = ListView_GetColumnWidth(hWndList, index);

		ListView_SetColumnWidth(hWndList, index, max(iColWidth, iHeaderWidth));
	}
	return TRUE;
}
BOOL DXSetupDlg::Command(WORD nID, WPARAM wParam,LPARAM lParam)
{
	switch (nID)
	{
   case IDABORT:
   case EBU_ABORT:
		{
			EndDialog(hDlg,EBU_ABORT);
		}
	}
    return(0);
}
BOOL DXSetupDlg::Cancel()
{
	Checklist_Term();
	EndDialog(hDlg,EBU_CANCEL);
	bDXSkipped = TRUE;
	return TRUE;
}
BOOL DXSetupDlg::Ok()
{
	LPDRIVERUPDATE       lpdu;
	int                 nIndex;
	HWND hWndList = GetDlgItem(hDlg,ID_CHECKLIST);
	
	//If no DirectX driver update info passed in via hotsetup engine
	//callback, then just flag that we want to install and get out
	//of here...
	if (NULL == Updates)
	{
		bDXSkipped = FALSE;
		return TRUE;
	}
	//For each DirectX driver update array element
	for (nIndex = 0; nIndex < Updates->numUpdates; nIndex++)
	{
		//Get pointer to the update array info for this element
		lpdu = Updates->UpdateArray[nIndex];
		lpdu->UserResponse = (Checklist_IsChecked(hWndList, nIndex)) ? IDYES : IDNO;
	}
	Checklist_Term();
	bDXSkipped = FALSE;
	EndDialog(hDlg,EBU_OK);
	return TRUE;
}
BOOL DXSetupDlg::Notify(WPARAM wParam, LPARAM lParam)
{
			NM_LISTVIEW
				*pNm = (NM_LISTVIEW *)lParam;
			HWND hWndList = GetDlgItem(hDlg,ID_CHECKLIST);
			switch(pNm->hdr.code)
			{
			case LVN_BEGINLABELEDIT:
				{
					// we want to abort label editing.  This is a hack for the way we are doing
					// the DX options page.
//					TRACE("LVN_BEGINLABELEDIT.\n");
					return TRUE;
				}break;
			case NM_CLICK:
				{
					DWORD dwpos;
					LV_HITTESTINFO tvhti;
					POINT point;
					
					// Find out where the cursor was
					dwpos = GetMessagePos();
					point.x = LOWORD(dwpos);
					point.y = HIWORD(dwpos);
					
					MapWindowPoints(HWND_DESKTOP, hWndList, &point, 1);
					
					tvhti.pt = point;
					
					int
						iItemClicked = ListView_HitTest(hWndList, &tvhti);
					char
						szMessage[256];
					
					// If the state image was clicked, lets get the state from the item and toggle it.
					if (tvhti.flags & LVHT_ONITEM)
					{
						if (!Checklist_IsAutomatic(hWndList, iItemClicked)) // not in automatic
						{
							if (Checklist_IsChecked(hWndList, iItemClicked))
							{
								// uncheck
								Checklist_SetState(hWndList, iItemClicked, FALSE);
								
								// update happens text
								EBULoadString(GetResourceInst(), STR_DIRECTX_KEEP, szMessage, sizeof(szMessage));
								ListView_SetItemText( hWndList, iItemClicked, 0, szMessage);
							}else{
								// check
								Checklist_SetState(hWndList, iItemClicked, TRUE);
								
								// update happens text
								EBULoadString(GetResourceInst(), STR_DIRECTX_REPLACE, szMessage, sizeof(szMessage));
								ListView_SetItemText( hWndList, iItemClicked, 0, szMessage);
							}
						}
						
					}
				}
				break;
				
			case LVN_KEYDOWN:
				{
					LV_KEYDOWN
						*pnkd = (LV_KEYDOWN *) lParam;
					
					if (VK_SPACE == pnkd->wVKey)
					{
						int
							iItemPressed = -1;
						char
							szMessage[256];
						
						iItemPressed = ListView_GetNextItem(hWndList, -1, LVNI_ALL | LVNI_SELECTED);
						
						if (-1 != iItemPressed)
						{
							if (!Checklist_IsAutomatic(hWndList, iItemPressed))
							{
								// not an automatic
								if (Checklist_IsChecked(hWndList, iItemPressed))
								{
									// uncheck
									Checklist_SetState(hWndList, iItemPressed, FALSE);
									
									// update happens text
									EBULoadString(GetResourceInst(), STR_DIRECTX_KEEP, szMessage, sizeof(szMessage));
									ListView_SetItemText( hWndList, iItemPressed, 0, szMessage);
								}else{
									// check
									Checklist_SetState(hWndList, iItemPressed, TRUE);
									
									// update happens text
									EBULoadString(GetResourceInst(), STR_DIRECTX_REPLACE, szMessage, sizeof(szMessage));
									ListView_SetItemText( hWndList, iItemPressed, 0, szMessage);
								}
							}
						}
					}
				}
				break;
			default:
				return FALSE;
			}
	return TRUE;
}
WORD DXSetupDlg::GetDefaultButtonResponse(DWORD dwMsgType)
{
	switch (dwMsgType & 0x0000000F)
	{
		case MB_OKCANCEL:
			if(dwMsgType & MB_DEFBUTTON2)
				return IDCANCEL;
			else
				return IDOK;
		case MB_OK:
			return IDOK;
		case  MB_RETRYCANCEL:
			if(dwMsgType & MB_DEFBUTTON2)
				return IDCANCEL;
			else
				return IDRETRY;
		case MB_ABORTRETRYIGNORE:
			if(dwMsgType & MB_DEFBUTTON3)
				return IDIGNORE;
			if(dwMsgType & MB_DEFBUTTON2)
				return IDRETRY;
			else
				return IDABORT;
		case MB_YESNOCANCEL:
			if(dwMsgType & MB_DEFBUTTON2)
				return IDNO;
			if(dwMsgType & MB_DEFBUTTON3)
				return IDCANCEL;
			else
				return IDYES;
		case MB_YESNO:
			if(dwMsgType & MB_DEFBUTTON2)
				return IDNO;
			else
				return IDYES;
	}
	return IDOK;
}
void DXSetupDlg::UpdateButton(HWND hDlg,int Id, char *szName,BOOL bRecommend)
{
	HWND btnHwd = GetDlgItem(hDlg,Id);
	SetWindowText(btnHwd,szName);
	ShowWindow(btnHwd,SW_NORMAL);
	SendMessage(btnHwd,BM_SETCHECK,(WPARAM)bRecommend,0);
}

EBURETCODE WINAPI DetectDirectX(DIRECT_X_VERSION * pver)
{	
	char szMessage[256];
	int message_id = STR_DIRECTX_CHECK;
	EBULoadString(GetResourceInst(), message_id, szMessage, sizeof(szMessage));
	MessageBox(GetWndParent(), szMessage, GetAppTitle(), MB_OK);

	switch (*pver)
	{
		case EV_EXISTING_OLDER:
			message_id = STR_DIRECTX_INSTALL_OLDER;
		break;
		case EV_NOT_INSTALLED:
			message_id = STR_DIRECTX_INSTALL_NONE;
		break;
		case EV_EXISTING_SAME:
		case EV_EXISTING_NEWER:
			message_id = STR_DIRECTX_OK;
			bInstDX = FALSE;
		break;
		default:
		   return EBU_CANCEL;
	}
	EBULoadString(GetResourceInst(), message_id, szMessage, sizeof(szMessage));
	if (bInstDX)
	{
		if (IDOK == MessageBox(GetWndParent(), szMessage, GetAppTitle(), MB_OKCANCEL))
		{
			bInstalledDX = true;
			return EBU_OK;
		}
		else
		{
			char str[200];
			EBULoadString(GetResourceInst(),STR_DIRECTXNOTINSTALLED,str,200);
			MessageBox(GetWndParent(),str,GetAppTitle(),MB_ICONEXCLAMATION | MB_OK);
			return EBU_ABORT;
		}
	}
	else
	{
		MessageBox(GetWndParent(), szMessage, GetAppTitle(), MB_OK);
		return EBU_CANCEL;
	}
}

EBURETCODE WINAPI InstallDirectX(LPUPDATEARRAY Updates)
{

	EBURETCODE retc;
									
	if(Updates != NULL)
	{
	   bInstDX = TRUE;
	   DXSetupDlg *d = new DXSetupDlg();
	   if(d)
	   {
		   retc = d->start((LPARAM)Updates);
		   delete d;
	   }
	   return retc;
	}
	return EBU_CANCEL;
}
EBURETCODE WINAPI InstallDPlay()
{
	
	TCHAR szTemp[_MAX_PATH] = "%TEMPDIR\\ixp000.tmp";
	Sleep(0);
	NukeDPlayRemants(szTemp,5000);
	if(bInstDX)
	{
	   bInstDX= FALSE;
	   return EBU_OK;
	}
	else
	   return EBU_CANCEL;
}
void NukeDPlayRemants(TCHAR *szTemp,ULONG uSleep)
{
	TCHAR szFile[_MAX_PATH];
//	TCHAR szTemp[_MAX_PATH] = "%TEMPDIR\\ixp000.tmp";
	SetCursor(LoadCursor(NULL,IDC_WAIT));
	Sleep(0);
	ForwardMyMessages();
	if(uSleep)
       Sleep(uSleep);
	Sleep(0);

	//
	//Get path to DPlay temp directory...
	//
	ReplaceStringTokens(szTemp, _MAX_PATH);

	//
	//If the DPlay temp directory exists, delete the files and then
	//remove the directory...
	//
	if (0xFFFFFFFF != GetFileAttributes(szTemp))
	{
		lstrcpy(szFile, szTemp);
		lstrcat(szFile, "\\ADVPACK.DLL");
		DeleteDirectPlayFile(szFile);

		lstrcpy(szFile, szTemp);
		lstrcat(szFile, "\\W95INF32.DLL");
		DeleteDirectPlayFile(szFile);

		lstrcpy(szFile, szTemp);
		lstrcat(szFile, "\\W95INF16.DLL");
		DeleteDirectPlayFile(szFile);

		RemoveDirectory(szTemp);
	}
	SetCursor(LoadCursor(NULL,IDC_ARROW));
}
VOID DeleteDirectPlayFile(TCHAR *szFile)
{
	HANDLE hFile;

	//
	//If the file specified does exist...
	//
	if (0xFFFFFFFF != GetFileAttributes(szFile))
	{
		//
		//If DeleteFile API fails for whatever reason...
		//
		if (0 == DeleteFile(szFile))
		{
			//
			//Create the file with DELETE_ON_CLOSE flag so that the
			//operating system can delete it once the last handle
			//is closed...
			//
			hFile = CreateFile(szFile,
							   GENERIC_READ,
							   FILE_SHARE_READ,
							   NULL,
							   OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
							   NULL);

			//
			//If we can't open the file with DELETE_ON_CLOSE for
			//any reason, make one last attempt to delete the file
			//
			if (INVALID_HANDLE_VALUE == hFile)
			{
				DeleteFile(szFile);
			}
			else
			{
				//
				//Close the handle to the file so the O.S. can delete
				//it.  The actual delete will occur when all other
				//processes accessing the file (if any) release their 
				//hold on it too...
				//
				CloseHandle(hFile);
			}
		}
	}
}

EBURETCODE WINAPI GetPidFn(LPGETPIDDATA pid)
{
	EBURETCODE retc=EBU_CANCEL;

	GetPIDDlg *d = new GetPIDDlg();
	if(d)
	{
       retc = d->start((LPARAM)pid);
	   delete d;
	}
	return retc;
}
EBURETCODE WINAPI GetPidNumFn(LPGETPIDDATA pid)
{
	EBURETCODE retc=EBU_CANCEL;
	ShowPIDDlg *d = new ShowPIDDlg();
	if(d)
	{
       retc = d->start((LPARAM)pid);
	   delete d;
	}
	return retc;
}

EBURETCODE WINAPI CDFailedFn(LPCDSPEEDDATA cd)
{
	if(g_hCDWnd)
	{
		DestroyWindow(g_hCDWnd);
		ForwardMyMessages();
		g_hCDWnd = NULL;
	}
	EBURETCODE retc=EBU_CANCEL;
	CDFailedDlg *d = new CDFailedDlg();
	if(d)
	{
       retc = d->start((LPARAM)cd);
	   delete d;
	}
	return retc;
}

BOOL CDFailedDlg::Init(LPARAM lParam)
{
	char text[1024];
	char text2[1024];
	cd = (LPCDSPEEDDATA)lParam;
	if(cd->cdstatus == CDCDROMFAIL)
	{
		EBULoadString(GetResourceInst(),STR_ERROR_CDTOOSLOW,text,1024);
		sprintf(text2,text,(int)cd->avg_speed);
		SetDlgItemText(hDlg,ID_PERFTEXT,text2);
	}
	else if(cd->cdstatus == CDCPUFAIL)
	{
		EBULoadString(GetResourceInst(),STR_ERROR_CPUTOOHIGH,text,1024);
		sprintf(text2,text,cd->mincpu);
		SetDlgItemText(hDlg,ID_PERFTEXT,text2);
	}
   return FALSE;
}
EBURETCODE WINAPI StartCDFn()
{
	EBURETCODE retc;
	StartCDDlg *d = new StartCDDlg();
	if(d)
	{
       retc = d->start();
	   delete d;
	}
	return retc;
}
EBURETCODE WINAPI CDDoneFn(LPCDSPEEDDATA cd)
{
	if(g_hCDWnd)
	{
		DestroyWindow(g_hCDWnd);
		ForwardMyMessages();
		g_hCDWnd = NULL;
	}
	EBURETCODE retc=EBU_CANCEL;
	CDDoneDlg *d = new CDDoneDlg();
	if(d)
	{
       retc = d->start((LPARAM)cd);
	   delete d;
	}
	return retc;
}
BOOL CDDoneDlg::Init(LPARAM lParam)
{
	LPCDSPEEDDATA cd = (LPCDSPEEDDATA)lParam;
	char msg[256];
	char msg2[300];
	if(cd->mincpu <=0.0)
	{
		EBULoadString(GetResourceInst(),STR_CDTESTOKRESULTS,msg,256);
		sprintf(msg2,msg,cd->avg_speed);
	}
	else
	{
	   EBULoadString(GetResourceInst(),STR_CDTESTRESULTS,msg,256);
	   sprintf(msg2,msg,cd->avg_speed,cd->mincpu);
	}
	   SetDlgItemText(hDlg,IDC_CDRESULTS,msg2);
	   SetFocus(GetDlgItem(hDlg,IDOK));
	return(1);
}
EBURETCODE WINAPI CreateCDSpeedWindow(LPCDSPEEDDATA cd)
{
    g_hCDWnd = CreateDialogParam(GetResourceInst(),"CDTEST",GetWndParent(),(DLGPROC)CDWndProc,(LPARAM)cd);
	cd->hWnd = g_hCDWnd;
	if(g_hCDWnd)
       ShowWindow(g_hCDWnd,SW_SHOW);
    else
		return EBU_CANCEL;
	return EBU_OK;
}
_declspec(dllexport) BOOL CALLBACK CDWndProc(HWND hDlg, WORD message, WPARAM wParam, LPARAM lParam)
{
	static LPCDSPEEDDATA cd;
    switch(message)
    {
		case WM_INITDIALOG:
		cd = (LPCDSPEEDDATA)lParam;
        SetCursor(LoadCursor(NULL,IDC_APPSTARTING));
        SetFocus(GetDlgItem(hDlg,IDOK));

        break;

    case WM_COMMAND:
        if(LOWORD(wParam) == IDCANCEL)
        {
               char str[200];
               EBULoadString(GetResourceInst(),STR_QUIT_SETUP,str,200);
               if(MessageBox(hDlg,str,GetAppTitle(),MB_ICONEXCLAMATION | MB_OKCANCEL) == IDOK)
               {
                 DestroyWindow(hDlg);
                 cd->nAbortCode = EBU_ABORT;
               }
        }
        else if(LOWORD(wParam) == IDOK)
        {
            DestroyWindow(hDlg);
            cd->nAbortCode = EBU_CANCEL;
        }
        break;
    case WM_ACTIVATE:
        {
            if(LOWORD(wParam) == WA_INACTIVE)
            {
                if((HWND)lParam == GetParent(hDlg))
                {
                    SetForegroundWindow(hDlg);	
                }
            }
        }
        break;
    case WM_DESTROY:
        SetCursor(LoadCursor(NULL,IDC_ARROW));
        break;

    }
    return(0);
}
BOOL StartCDDlg::Init(LPARAM lParam)
{
	SetFocus(GetDlgItem(hDlg,IDOK));
	return TRUE;
}
BOOL StartCDDlg::Cancel()
{
	EndDialog(hDlg,EBU_CANCEL);
	return TRUE;
}

BOOL GetGroupDlg :: Init(LPARAM lParam)
{
	group = (LPGETGROUPDATA)lParam;
	group->group = 0;
	return TRUE;
}

BOOL GetGroupDlg::Command(WORD nId,WORD nNotify, LPARAM lParam)
{
	switch(nId)
	{
		case ID_FULL:
			group->group = 2;
			break;
		case ID_TYPICAL:
			group->group = 4;
			break;
		case ID_MINIMUM:
			group->group = 8;
			break;
		default:
			return FALSE;
	}
	EndDialog(hDlg,EBU_OK);
	return TRUE;
}

BOOL MkRootDlg::Init(LPARAM lParam)
{
	mk = (LPMKROOTDATA)lParam;
	SetDlgItemText(hDlg,ID_DLGEDITCONTROL,mk->szAppDir);
	SendDlgItemMessage(hDlg,ID_DLGEDITCONTROL,EM_SETSEL,0,-1);
	SendDlgItemMessage(hDlg,ID_DLGEDITCONTROL,EM_LIMITTEXT,_MAX_PATH/2,0);
	SetFocus(GetDlgItem(hDlg,ID_DLGEDITCONTROL));
	return TRUE;
}

BOOL MkRootDlg::Browse()
{
	char Buffer[MAX_PATH];
	LPITEMIDLIST pidlBrowse;

	BROWSEINFO bi;
	bi.hwndOwner=hDlg;
	bi.pidlRoot=NULL;
	bi.pszDisplayName = Buffer;
	bi.lpszTitle = GetAppTitle();
	bi.ulFlags=BIF_RETURNONLYFSDIRS;
	bi.lpfn=NULL;
	bi.lParam=0;
	bi.iImage = NULL;
	pidlBrowse = SHBrowseForFolder(&bi);
	if(pidlBrowse != NULL)
	{
		if(SHGetPathFromIDList(pidlBrowse,Buffer))
			SetDlgItemText(hDlg,ID_DLGEDITCONTROL,Buffer);
		LPMALLOC lpMalloc;

		if (SUCCEEDED(SHGetMalloc(&lpMalloc)))
		{
			lpMalloc->Free(pidlBrowse);
			lpMalloc->Release();
		}
	}

    return TRUE;
}
BOOL MkRootDlg::Command(WORD nId,WORD nNotify, LPARAM lParam)
{
	switch(nId)
	{
		case ID_BROWSE:
			return Browse();
		default:
			return FALSE;
	}
}


BOOL MkRootDlg::Ok()
{
	GetDlgItemText(hDlg,ID_DLGEDITCONTROL,mk->UserRootEntry,sizeof(mk->UserRootEntry));
	if(mk->lpfnValidateEntry)
	{
	   int retc;
	   if((retc = (*mk->lpfnValidateEntry)(mk->UserRootEntry)) == EBU_OK)
	   {
		  EndDialog(hDlg,EBU_OK);
		  return(TRUE);
	   }
	   else if(retc == EBU_ABORT)
		  {
			  EndDialog(hDlg,EBU_ABORT);
			  return(TRUE);
		  }
		   return (TRUE);
	}
	else
	{
		 EndDialog(hDlg,EBU_OK);
		 return(TRUE);
	}
    return (FALSE);
}

BOOL GetPIDDlg::Init(LPARAM lParam)
{
	pid = (LPGETPIDDATA)lParam;
#ifdef SETUPOEM		
     SendDlgItemMessage(hDlg,ID_DLGEDITCONTROL,EM_LIMITTEXT,5,0);
     SendDlgItemMessage(hDlg,ID_DLGEDITCONTROL3,EM_LIMITTEXT,5,0);
     SendDlgItemMessage(hDlg,ID_DLGEDITCONTROL2,EM_LIMITTEXT,7,0);
#else
     SendDlgItemMessage(hDlg,ID_DLGEDITCONTROL2,EM_LIMITTEXT,7,0);
     SendDlgItemMessage(hDlg,ID_DLGEDITCONTROL,EM_LIMITTEXT,3,0);
#endif
     SetFocus(GetDlgItem(hDlg,ID_DLGEDITCONTROL));
     return(TRUE);
}

BOOL GetPIDDlg::Command(WORD nID, WORD nNotify,LPARAM lParam)
{
    switch(nID)
    {
#ifndef SETUPOEM
	case ID_DLGEDITCONTROL:
			if(nNotify == EN_CHANGE)
			{
				char edit[10];
				if(GetDlgItemText(hDlg,ID_DLGEDITCONTROL,edit,10)>=3)
				   SetFocus(GetDlgItem(hDlg,ID_DLGEDITCONTROL2));
				break;
			}
		break;
#else
	case ID_DLGEDITCONTROL:
			if(nNotify == EN_CHANGE)
			{
				char edit[10];
				if(GetDlgItemText(hDlg,ID_DLGEDITCONTROL,edit,10)>=5)
				   SetFocus(GetDlgItem(hDlg,ID_DLGEDITCONTROL2));
				break;
			}
		break;
	case ID_DLGEDITCONTROL2:
			if(nNotify == EN_CHANGE)
			{
				char edit[10];
				if(GetDlgItemText(hDlg,ID_DLGEDITCONTROL2,edit,10)>=7)
				   SetFocus(GetDlgItem(hDlg,ID_DLGEDITCONTROL3));
				break;
			}
		break;
		default:
			return FALSE;
#endif
	}
	return TRUE;
}
BOOL GetPIDDlg::Ok()
{
#ifdef SETUPOEM
     GetDlgItemText(hDlg,ID_DLGEDITCONTROL3,pid->SerialNumber,25);
	 g_fOEM = TRUE;
#endif
     GetDlgItemText(hDlg,ID_DLGEDITCONTROL,pid->SiteCode,25);
     GetDlgItemText(hDlg,ID_DLGEDITCONTROL2,pid->ProductID,25);
     if(pid->lpfnValidateEntry)
     {
         int retc;
	  	 if((retc = (*pid->lpfnValidateEntry)((char *)pid)) == IDOK)
         {
             EndDialog(hDlg,EBU_OK);
             return(TRUE);
         }
         else if(retc == IDABORT)
         {
             EndDialog(hDlg,EBU_ABORT);
             return(TRUE);
         }
	     else
		   	 return (TRUE);
     }
	 else
     {
         EndDialog(hDlg,EBU_OK);
         return(TRUE);
     }
     return(0);
}
BOOL GetNameDlg::Init(LPARAM lParam)
{
	char regkey[256];
	char player[100];
	char szValue[50];
	char Control[256];
	HKEY hkResult;
	LONG lBufSize = sizeof(Control);
	DWORD dwResult;
	UserName = (LPSTR)lParam;

	memset(Control,0,sizeof(Control));
	EBULoadString(GetResourceInst(),STR_REGKEY_PLAYERNAME,regkey,sizeof(regkey));
	EBULoadString(GetResourceInst(),STR_DEFPLAYERNAME,player,sizeof(player));
	EBULoadString(GetResourceInst(),STR_REGKEY_DEFNAME,szValue,sizeof(szValue));
	dwResult = RegOpenKeyEx(HKEY_CURRENT_USER,
					 (LPSTR)regkey,
					 NULL,
					 KEY_QUERY_VALUE,
					 (HKEY FAR *)&hkResult);
	if( ERROR_SUCCESS == dwResult )
	{

	   dwResult = RegQueryValueEx(hkResult,
						  (char *)szValue,
						  NULL,
						  NULL,
						  (LPBYTE)Control,
						  (LPDWORD)&lBufSize);
	   if( ERROR_SUCCESS != dwResult )
	   {
		  lstrcpy( Control, player);
	   }

	   RegCloseKey(hkResult);

	}
	else
	{
	   lstrcpy( Control, player );
	}

	 SetDlgItemText(hDlg,ID_DLGEDITCONTROL,Control);
	 SendDlgItemMessage(hDlg,ID_DLGEDITCONTROL,EM_SETSEL,0,-1);
	 SetFocus(GetDlgItem(hDlg,ID_DLGEDITCONTROL));
	 SendDlgItemMessage(hDlg,ID_DLGEDITCONTROL,EM_LIMITTEXT,30,0);
     return(TRUE);
}
BOOL GetNameDlg::Ok()
{
    GetDlgItemText(hDlg,ID_DLGEDITCONTROL,UserName,256);
    char *ptr=UserName;
    while(*ptr != '\0')
    {
   	  if(*ptr == '\\')
   	  {
   		  char str[256];
   		  EBULoadString(GetResourceInst(),STR_BADCHARINNAME,str,256);
             MessageBox(hDlg,str,GetAppTitle(),MB_ICONEXCLAMATION | MB_OK);
   		  return (TRUE);
   	  }
   	  ptr++;
    }
    EndDialog(hDlg,EBU_OK);
    return(TRUE);
}
BOOL ShowPIDDlg::Init(LPARAM lParam)
{
	char Buffer[150];
	// replace string tokens in window title
	GetWindowText(hDlg,Buffer,150);
	ReplaceStringTokens(Buffer,150);
	SetWindowText(hDlg,Buffer);

	// replace string tokens in text
	GetDlgItemText(hDlg,IDC_STATIC1,Buffer,150);
	ReplaceStringTokens(Buffer,150);
	SetDlgItemText(hDlg,IDC_STATIC1,Buffer);

     SetDlgItemText(hDlg,ID_DLGSTATICCONTROL,(LPSTR)((LPGETPIDDATA)lParam)->pszPID);
	 return(TRUE);
}

BOOL Restart::Init(LPARAM lParam)
{
	char msg[_MAX_PATH];
	EBULoadString(GetResourceInst(),STR_MUSTRESTART,msg,_MAX_PATH);
	SetDlgItemText(hDlg,ID_DLGSTATICCONTROL,msg);
    return TRUE;
}
BOOL Restart::Cancel()
{
	EndDialog(hDlg,EBU_CANCEL);
	return TRUE;
}
