/***********************************************************************
* CCommandListView.cpp : Derived ListView class for command list
*
* Chris N. Haddan
* April 1998
*
* (C) 1998 Microsoft Corporation
*
************************************************************************/
#include "CCommandListView.hpp"
#include "textdoc.h"
#include "command.h"
#include "CPrepDoc.hpp"

extern CPrepDoc *g_PrepDoc;

bool CCommandListView::SpanColumns(LPDRAWITEMSTRUCT lpDrawItem)
{
	CCommand *pCmd = (CCommand *) lpDrawItem->itemData;

	switch (pCmd->GetCommandType ())
	{
		case TOK_COMMENT:
				return true;
		default:
				return false;
	}
}


COLORREF CCommandListView::GetRowTextColor (LPDRAWITEMSTRUCT lpDrawItem)
{
	if (g_PrepDoc->IsShowInColor())
	{
		return (RGB (255,255,255));
	}
	else
	{
		return (GetSysColor (COLOR_WINDOWTEXT));
	}

	CCommand *pCmd = (CCommand *) lpDrawItem->itemData;

	switch (pCmd->GetCommandType ())
	{
		case TOK_COMMENT:
			return (RGB (255,255,255));
		case TOK_INSTALLLIST:
			return (RGB (105,15,0));

		case TOK_INIVALUE:
			return (RGB (150,0,0));

		case TOK_INSTICON:
			return (RGB (100,10,0));

		default:
			return (RGB (155,55,55));
	}
}


COLORREF CCommandListView::GetRowBkColor (LPDRAWITEMSTRUCT lpDrawItem)
{	
	COLORREF bkColor;

	if (!g_PrepDoc->IsShowInColor())
	{
		return (GetSysColor (COLOR_WINDOW));
	}

	CCommand *pCmd = (CCommand *) lpDrawItem->itemData;
	switch (pCmd->GetCommandType ())
	{
		case TOK_COMMENT:
			bkColor = RGB (31,85,33);
			break;
		case TOK_MKDIR:
			bkColor = RGB (146, 78, 143);
			break;
		case TOK_INIVALUE:
			bkColor = RGB (209, 185, 50);
			break;
		case TOK_DELETEFILE:
			bkColor = RGB (207,29,47);
			break;
		case TOK_INSTALL:
			bkColor = RGB (243,119,27);
			break;
		case TOK_INSTALLLIST:
			bkColor = RGB (65,41,196);
			break;
		case TOK_INSTFONT:
			bkColor = RGB (27,149,207);
			break;
		case TOK_INSTICON:
			bkColor = RGB (18,197,179);
			break;
		case TOK_PROPERTY:
			bkColor = RGB (150,90,20);
			break;
		case TOK_RULE:
			bkColor = RGB (150,10,10);
			break;
		case TOK_BEGINFILELIST:
		case TOK_ENDFILELIST:

		case TOK_BEGINSTRINGLIST:
		case TOK_ENDSTRINGLIST:

		case TOK_BEGINSTATICSTRINGLIST:
		case TOK_ENDSTATICSTRINGLIST:
			bkColor = RGB (97,97,79);
			//bkColor = RGB (58,97,79);
			break;
		case TOK_STRINGVAR:
			bkColor = RGB (122,122,79);
			break;

		default:
			bkColor = RGB (64,64,64);//GetBkColor();
			break;
	}
	return (RGB (GetRValue (bkColor)-((lpDrawItem->itemID % COLOR_GRAD)*COLOR_VAR), GetGValue (bkColor)-((lpDrawItem->itemID % COLOR_GRAD)*COLOR_VAR), GetBValue (bkColor)-((lpDrawItem->itemID  % COLOR_GRAD)*COLOR_VAR)));
}


LRESULT CALLBACK CCommandListView ::ProcessMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RECT rect;

	switch (msg)
	{
		case WM_COMMAND:
			switch (LOWORD (wParam))
			{
				case ID_POPUP_PROPERTIES:
					DialogBoxParam (g_hAppInst, MAKEINTRESOURCE (IDD_EDITCOMMANDDIALOG), g_hAppWnd, (DLGPROC) EditCommandDialogProc, (LPARAM)GetLvHwnd());
					SetFocus (GetLvHwnd());
					break;

				case ID_AUTOSIZE:
					for (int i=0;i<5;i++)
						AutoSizeColumn (i);
					break;
			}
			return 0;

		case WM_NOTIFY:
			{
				LV_DISPINFO *pLvdi = (LV_DISPINFO *)lParam;
				NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;	

				switch(pLvdi->hdr.code)
				{

					case HDN_ITEMCHANGING:    
					case HDN_ENDTRACK:
						{
							HD_NOTIFY *phdn = (HD_NOTIFY *)pNm;
						    InvalidateRect(m_hWnd, NULL, FALSE);
							break;
						}

					case NM_DBLCLK:
					{
						CCommand *pCmd;
						LV_HITTESTINFO hti;
						LV_ITEM lvi;

						// see if an item is really selected. User might have double clicked in a void area.

						if (ListView_GetSelectedCount (m_hWnd) == 0)
						{
							return false;
						}

						ZeroMemory (&lvi, sizeof (LV_ITEM));

						hti.pt.x = pNm->ptAction.x;
						hti.pt.y = pNm->ptAction.y;

						hti.flags = LVHT_ONITEM;
						lvi.iItem = ListView_HitTest (m_hWnd, &hti);
						
						if (lvi.iItem == -1)
						{
							return false;
						}

						lvi.iSubItem = 0;
						lvi.mask = LVIF_PARAM;

						if (!ListView_GetItem  (m_hWnd, &lvi))
						{
							return false;
						}

						pCmd =  (CCommand *)lvi.lParam;
						DialogBoxParam (g_hAppInst, MAKEINTRESOURCE (IDD_EDITCOMMANDDIALOG), g_hAppWnd, (DLGPROC) EditCommandDialogProc, (LPARAM)GetLvHwnd());
						SetFocus (GetLvHwnd());
						return true;
					}
					break;
					
					case NM_RCLICK:

						GetClientRect (m_hWnd, &rect);
						if (!PtInRect (&rect, pNm->ptAction))
							break;

						HMENU hPrimaryMenu = LoadMenu (m_hInst, MAKEINTRESOURCE (IDM_COMMAND_ITEM_MENU));

						HMENU hMenu = GetSubMenu (hPrimaryMenu,0);

					

						ClientToScreen (m_hWnd, &pNm->ptAction);

						// if there are no commands in the list, disable the properties option
						if (ListView_GetSelectedCount (GetLvHwnd()) == 0)
							EnableMenuItem (hMenu, ID_POPUP_PROPERTIES, MF_GRAYED);
						

						// popup the menu at the location the user right clicked.
						TrackPopupMenu (hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, 
										pNm->ptAction.x, 
										pNm->ptAction.y,
										0,
										hWnd,
										NULL);

						DestroyMenu (hPrimaryMenu);

					break;
				}
			}
			break;

		case WM_DRAWITEM:
			{
				LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT) lParam; 
				switch (lpDrawItem->itemAction)
				{
					case ODA_SELECT:
					case ODA_DRAWENTIRE:
					case ODA_FOCUS:
						DrawListViewItem((LPDRAWITEMSTRUCT)lpDrawItem);
						break;
				}
			}

		case WM_MEASUREITEM:
			{
				LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT) lParam; 
				lpmis->itemHeight = m_nRowHeight;
				return true;
			}

		case WM_ERASEBKGND:
			HDC hdc = GetDC (m_hWnd);
			PaintListViewVoidAreas (hdc);
			ReleaseDC (m_hWnd, hdc);
			InvalidateRect (GetLvHwnd(), NULL, true);
			return 1;
	}
	return (DefWindowProc (hWnd, msg, wParam, lParam));
}


void CCommandListView::AddImageList()
{
	HBITMAP hbm, hbm2;

	m_hImageList = ImageList_Create(23,20, TRUE, 1, 1);  
	hbm  = LoadBitmap (m_hInst, MAKEINTRESOURCE (IDB_OS_ICONS));
	hbm2 = LoadBitmap (m_hInst, MAKEINTRESOURCE (IDB_OS_ICONS_MASK));
	ImageList_Add (m_hImageList, hbm, hbm2);
	ListView_SetImageList(m_hWnd, m_hImageList, LVSIL_SMALL);   
	DeleteObject (hbm);
	DeleteObject (hbm2);
}


void CCommandListView::RemoveImageList()
{
	ImageList_Destroy (ListView_GetImageList (m_hWnd, LVSIL_SMALL));
}

void ExpandTabs (const char *szIn, char *szOut)
{
	while (*(szIn))
	{
		if (*szIn=='\t')
		{
			*(szOut++)=' ';
			*(szOut++)=' ';
			*(szOut++)=' ';
			*(szOut++)=' ';
		}
		else
			*(szOut++)=*szIn;

		++szIn;
	}
	*szOut = '\0';
}

int CCommandListView::GetItemWidth (int iCol, int iRow)
{
	SIZE sizeString;
	char szBuffer[MAX_PATH*4];
	LV_ITEM lvi;

	lvi.iItem = iRow;
	lvi.mask = LVIF_PARAM;
	ListView_GetItem (m_hWnd, &lvi);

	GetText (iCol, lvi.lParam, (char *)&szBuffer);

	HDC hdc = GetDC (m_hWnd);

	CCommand *pCmd = (CCommand *)lvi.lParam;

	HFONT hFontOld;

	if (pCmd->GetCommandType ()==TOK_COMMENT)
	{
		hFontOld = (HFONT) SelectObject (hdc, m_italicfont);
	}
	else
	{
		hFontOld = (HFONT) SelectObject (hdc, m_font);
	}

	GetTextExtentPoint32(hdc, szBuffer, lstrlen (szBuffer), &sizeString);	

	SelectObject (hdc, hFontOld);
	ReleaseDC (m_hWnd, hdc);
	
	return (sizeString.cx);
}

void CCommandListView::GetText (int iCol, LPARAM lParam, char *szString)
{
	CCommand *pCmd;
	pCmd =  (CCommand *)lParam;

	static char buffer[MAX_PATH*2];
	static char szText[MAX_PATH*2];
	switch (iCol)
	{
		case 0:  // ID
			if (pCmd->GetCommandType() == TOK_COMMENT)
			{
				ExpandTabs (pCmd->GetComment(), szText);
				wsprintf (buffer, "***** %s", szText);
				lstrcpy (szString, buffer);
			}
			else
			if (pCmd->IsValidToken())
			{
				wsprintf (buffer, "%d", pCmd->GetCommandID());
				lstrcpy (szString, buffer);
			}
			else
			{
				wsprintf (buffer, " - ");
				lstrcpy (szString, buffer);
			}
			break;
		case 1:   // COMMAND
			wsprintf (buffer, "%s", Keywords[pCmd->GetCommandType()].pszKeyword);
			lstrcpy (szString, buffer);
			break;
		case 2:  // Group
			switch (pCmd->GetCommandType())
			{
				case TOK_MKDIR:
					sprintf (szString, "%I64X ", pCmd->GetDirGroup());
					break;
				case TOK_PROPERTY:
				case TOK_READFILELIST:
				case TOK_INSTALLGO:
				case TOK_GETPID:
				case TOK_GETNAME:
				case TOK_GETGROUP:
				case TOK_MKROOT:
				case TOK_INSTDX:
				case TOK_BEGINFILELIST:
				case TOK_ENDFILELIST:
				case TOK_BEGINSTATICSTRINGLIST:
				case TOK_ENDSTATICSTRINGLIST:
				case TOK_REGWIZ:
				case TOK_DELETEFILE:
				case TOK_STRINGVAR:
				case TOK_BEGINSTRINGLIST:
				case TOK_ENDSTRINGLIST:
				case TOK_ACTION:
					lstrcpy (szString, "<none>");
					break;
				default:					
					sprintf (szString, "%I64X ", pCmd->GetGroup());
					break;
			}
			break;
		case 3:  // OS Flags
			wsprintf (buffer, "%X", pCmd->GetBuildFlags());
			lstrcpy (szString, buffer);
			break;
		case 4:  // param1
			switch (pCmd->GetCommandType())
			{
				case TOK_ACTION:
					wsprintf (szString, "Command='%s'", pCmd->GetActionCommand());
					break;
				case TOK_STRINGVAR:
					wsprintf (szString, "ID='%d'", pCmd->GetStringID());
					break;
				case TOK_DELETEFILE:
					wsprintf (szString, "Src='%s'", pCmd->GetDeleteFileValue());
					break;
				case TOK_SHELLEXECUTE:
					wsprintf (szString, "Filename='%s'", pCmd->GetShellExecuteFileName());
					break;
				case TOK_REGWIZ:
					wsprintf (szString, "'%s'", pCmd->GetRegWizRegName());
					break;
				case TOK_CABGO:
					wsprintf (szString, "'%s'", pCmd->GetCabName());
					break;

				case TOK_CDSPEED:
					wsprintf (szString, "Min='%s'", pCmd->GetCDSpeedMinCDValue());
					break;
				case TOK_INSTICON:
					wsprintf (szString, "Name='%s'", pCmd->GetInstIconNameValue());
					break;
			
				case TOK_INSTDX:
					wsprintf (szString, "Value='%s'",pCmd->GetInstDxValue());
					break;

				case TOK_MKROOT:
					if (pCmd->GetUninstallFileFlag())
					{
						lstrcpy (szString, "[Uninstall] ");
					}
					break;

				case TOK_MKDIR:
					wsprintf (buffer, "Dir='%s'", pCmd->GetDirName());
					lstrcpy (szString, buffer);
					break;
				case TOK_INSTFONT:
				case TOK_INSTALL:
				case TOK_INSTALLLIST:
					wsprintf (buffer, "Src='%s'", pCmd->GetSourceName());
					lstrcpy (szString, buffer);
					break;
				case TOK_PROPERTY:
					wsprintf (buffer, "Property='%s'", pCmd->GetProperty());
					lstrcpy (szString, buffer);
					break;
				case TOK_COMMENT:
					wsprintf (buffer, "%s", pCmd->GetComment());
					lstrcpy (szString, buffer);
					break;
				case TOK_INIVALUE:
					if (pCmd->GetMapFlag())
					{
						wsprintf (buffer, "[Mapped]");
					}
					else
					{
						wsprintf (buffer, "Filename='%s'",pCmd->GetIniFilename()); 
					}
					lstrcpy (szString, buffer);
					break;
				case TOK_RULE:
					wsprintf (buffer, "Pattern='%s'", pCmd->GetRulePattern());
					lstrcpy (szString, buffer);
					break;
				case TOK_READFILELIST:
					wsprintf (buffer, "File='%s'", pCmd->GetReadFileListName());
					lstrcpy (szString, buffer);
					break;
					break;
			}
			break;
		case 5: // param2
			switch (pCmd->GetCommandType())
			{
				case TOK_ACTION:
					if (pCmd->GetActionParam1())
						wsprintf (szString, "Param1='%s'", pCmd->GetActionParam1());
					else
						lstrcpy (szString, "-");
					break;

				case TOK_STRINGVAR:
					wsprintf (szString, "Value='%s'", pCmd->GetStringValue());
					break;

				case TOK_DELETEFILE:
					if (pCmd->GetUninstallFileFlag())
						{
							lstrcpy (szString, "[Silent] ");
						}
					break;
				case TOK_RULE:
					wsprintf (buffer, "Action='%s'", pCmd->GetRuleAction());
					lstrcpy (szString, buffer);
					break;
				case TOK_SHELLEXECUTE:
					wsprintf (szString, "Dir='%s'", pCmd->GetShellExecuteDirectory());
					break;
				case TOK_CDSPEED:
					wsprintf (szString, "Max='%s'", pCmd->GetCDSpeedMaxCPUValue());
					break;
				case TOK_INSTICON:
					wsprintf (szString, "IconName='%s'", pCmd->GetInstIconNameIconValue());
					break;
				case TOK_INSTDX:
					wsprintf (szString, "Name='%s'",pCmd->GetInstDxNameValue());
					break;
				case TOK_MKDIR:
					lstrcpy (szString, "");
					if (pCmd->GetUninstallFileFlag())
					{
						lstrcat (szString, "[Uninstall] ");
					}

					if (pCmd->GetUninstallAllFlag())
					{
						lstrcat (szString, "[Uninstall_All] ");
					}
					break;
				case TOK_INIVALUE:
					wsprintf (buffer, "Section='%s'",pCmd->GetIniSection()); 
					lstrcpy (szString, buffer);
					break;
				case TOK_PROPERTY:
					wsprintf (buffer, "Value='%s'", pCmd->GetPropertyValue());
					lstrcpy (szString, buffer);
					break;
				case TOK_INSTFONT:
				case TOK_INSTALL:
				case TOK_INSTALLLIST:
					wsprintf (buffer, "Dest='%s'", pCmd->GetDestName());
					lstrcpy (szString, buffer);
					break;
			}
			break;
		case 6: // param3
			switch (pCmd->GetCommandType())
			{ 
				case TOK_ACTION:
					if (pCmd->GetActionParam2())
						wsprintf (szString, "Param2='%s'", pCmd->GetActionParam2());
					else
						lstrcpy (szString, "-");
					break;

				case TOK_SHELLEXECUTE:
					wsprintf (szString, "Param='%s'", pCmd->GetShellExecuteParameters());
					break;
				case TOK_CDSPEED:
					wsprintf (szString, "Name='%s'", pCmd->GetCDSpeedFileNameValue());
					break;
				case TOK_INSTICON:
					wsprintf (szString, "Path='%s'", pCmd->GetInstIconDescriptionValue());
					break;
				case TOK_INSTDX:
					wsprintf (szString, "MinVer='%s'",pCmd->GetInstDxMinVersion());
					break;
				case TOK_INIVALUE:
					wsprintf (buffer, "Entry='%s'",pCmd->GetIniEntry()); 
					lstrcpy (szString, buffer);
					break;
				case TOK_INSTFONT:
				case TOK_INSTALL:
				case TOK_INSTALLLIST:
					lstrcpy (szString, "");
					if (pCmd->GetWindowsDirFlag())
					{
						lstrcat (szString, "[WindowsDir] ");
					}
					else
					if (pCmd->GetSystemDirFlag())
					{
						lstrcat (szString, "[SystemDir] ");
					}
					else
					if (pCmd->GetAppDirFlag())
					{
						lstrcat (szString, "[AppDir] ");
					}
					break;
			}
			break;
		case 7: // param4
			switch (pCmd->GetCommandType())
			{
				case TOK_ACTION:
					if (pCmd->GetActionParam3())
						wsprintf (szString, "Param3='%s'", pCmd->GetActionParam3());
					else
						lstrcpy (szString, "-");
					break;
				case TOK_SHELLEXECUTE:
					wsprintf (szString, "Show='%n'", pCmd->GetShellExecuteShow());
					break;
				case TOK_INSTICON:
					wsprintf (szString, "Dest='%s'", pCmd->GetInstIconDestinationValue());
					break;
				case TOK_INSTDX:
					wsprintf (szString, "Flags='%s'",pCmd->GetInstDxFlagsValue());
					break;
				case TOK_INIVALUE:
					wsprintf (buffer, "Type=%s",pCmd->GetIniType()); 
					lstrcpy (szString, buffer);
					break;
				case TOK_INSTFONT:
				case TOK_INSTALL:
				case TOK_INSTALLLIST:
					lstrcpy (szString, "");
					if (pCmd->GetSysFileFlag())
					{
						lstrcat (szString, "[SysFile] ");
					}
					if (pCmd->GetFontFlag())
					{
						lstrcat (szString, "[Font] ");
					}
					if (pCmd->GetSharedFileFlag())
					{
						lstrcat (szString, "[Shared] ");
					}
					if (pCmd->GetDLLRegisterFlag())
					{
						lstrcat (szString, "[DLL Register] ");
					}
					if (pCmd->GetDeleteFileSilentFlag())
					{
						lstrcat (szString, "[Uninstall] ");
					}
					if (pCmd->GetCabFlag())
					{
						lstrcat (szString, "[Cab] ");
					}
					if (pCmd->GetUninstOnlyFlag())
					{
						lstrcat (szString, "[UnistOnly] ");
					}
					break;
			}
			break;
		case 8: // param5
			switch (pCmd->GetCommandType())
			{
				case TOK_ACTION:
					if (pCmd->GetActionParam4())
						wsprintf (szString, "Param4='%s'", pCmd->GetActionParam4());
					else
						lstrcpy (szString, "-");
					break;

				case TOK_SHELLEXECUTE:
					lstrcpy (szString, "");
					if (pCmd->GetShellExecuteWait ())
					{
						lstrcat (szString, "[Wait] ");
					}
					if (pCmd->GetUninstallFileFlag())
					{
						lstrcat (szString, "[Uninstall] ");
					}

					break;
				case TOK_INSTICON:
					wsprintf (szString, "Index='%s'", pCmd->GetInstIconIndexValue());
					break;

				case TOK_INIVALUE:
					wsprintf (buffer, "Value='%s'",pCmd->GetIniValue()); 
					lstrcpy (szString, buffer);
					break;
			}
			break;
		case 9: // param6
			switch (pCmd->GetCommandType())
			{
				case TOK_INSTALLLIST:
					lstrcpy (szString, "");
					
					if (pCmd->GetCabPreCopy())
					{
						lstrcat (szString, "[CabPreCopy] ");
					}
					break;
				case TOK_ACTION:
					if (pCmd->GetActionRecurseFlag())
					{	
						lstrcpy (szString, "[Recursive]");
					}
					if (pCmd->GetCabPreCopy())
					{
						lstrcat (szString, "[CabPreCopy]");
					}
					break;
				case TOK_INIVALUE:
					lstrcpy (szString, "");
					if (pCmd->GetUninstallFileFlag())
					{
						lstrcat (szString, "[Uninstall] ");
					}

					if (pCmd->GetUninstallAllFlag())
					{
						lstrcat (szString, "[Uninstall_All] ");
					}
					break;

			}
			break;
		case 10: // param 7
			if (pCmd->GetDiskId() == -1)
			{
				lstrcpy (szString, "-");
			}
			else
			{
				wsprintf (buffer, "Disk %d", pCmd->GetDiskId()+1);
				lstrcpy (szString, buffer);
			}
			break;
		case 11: // param 8
			break;
		default:
			lstrcpy (szString, "");
	}
}


void CCommandListView::DrawItem (int iItem, HDC hdc, LPRECT prcClip, LPDRAWITEMSTRUCT lpDrawItem)
{
	LV_ITEM lvi;
	HIMAGELIST himl;
	int cxImage, cyImage;
	HBRUSH hbr;

	lvi.mask = LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvi.iItem = lpDrawItem->itemID;
	lvi.iSubItem = 0;
	ListView_GetItem(lpDrawItem->hwndItem, &lvi);
	COLORREF rgbBk;

    if ((lpDrawItem->itemState & ODS_SELECTED) && m_bSelectable)
    {
		hbr = CreateSolidBrush (GetHiLiteBkColor ());
		rgbBk = GetHiLiteBkColor ();
    }
    else
    {
		rgbBk = GetRowBkColor (lpDrawItem);
		hbr = CreateSolidBrush (GetRowBkColor (lpDrawItem));
	}

	FillRect (hdc, prcClip, hbr);

	HRGN hrgn = CreateRectRgnIndirect (prcClip);
	SelectClipRgn (hdc, hrgn);

	switch (iItem)
	{
		case 3:
			CCommand *pCmd = (CCommand *) lvi.lParam;

			himl = ListView_GetImageList(lpDrawItem->hwndItem, LVSIL_SMALL);

			ImageList_GetIconSize(himl, &cxImage, &cyImage);
			if (pCmd->GetCommandType() != TOK_COMMENT)
			{
				
				DWORD dwFlags = pCmd->GetBuildFlags();
				if (dwFlags & OS_WIN95)
				{
					ImageList_DrawEx (himl, 0, hdc,  prcClip->left, prcClip->top, cxImage, cyImage, 
						rgbBk,CLR_NONE,
						ILD_TRANSPARENT);
				}
				prcClip->left += cxImage;
				if (dwFlags & OS_WIN98)
				{
					ImageList_DrawEx (himl, 1, hdc,  prcClip->left, prcClip->top, cxImage, cyImage, RGB (10,20,10),
						RGB (20,10,20), ILD_TRANSPARENT);
				}
				prcClip->left += cxImage;
				if (dwFlags & OS_NT40)
				{
					ImageList_DrawEx (himl, 2, hdc,  prcClip->left, prcClip->top, cxImage, cyImage, RGB (10,20,10),
						RGB (20,10,20), ILD_TRANSPARENT);
				}
				prcClip->left += cxImage;
				if (dwFlags & OS_NT50)
				{
					ImageList_DrawEx (himl, 3, hdc,  prcClip->left, prcClip->top, cxImage, cyImage, RGB (10,20,10),
						RGB (20,10,20), ILD_TRANSPARENT);
				}
			}
			break;
	}
	SelectClipRgn (hdc, NULL);
	DeleteObject (hrgn);
	DeleteObject (hbr);
}


void CCommandListView::DrawListViewItem(LPDRAWITEMSTRUCT lpDrawItem)
{
    LV_ITEM lvi;
    int cxImage = 0, cyImage = 0;
    RECT rcClip;
    int iColumn = 1;
	UINT uiFlags = ILD_TRANSPARENT;
	int iWidth, iCol = 0;
	char buffer[MAX_PATH*4];
	RECT rcClient;

    // Get the item image to be displayed
    
	lvi.mask = LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
    lvi.iItem = lpDrawItem->itemID;
    lvi.iSubItem = 0;
    ListView_GetItem(lpDrawItem->hwndItem, &lvi);

	// Is the item selected?

    if ((lpDrawItem->itemState & ODS_SELECTED) && m_bSelectable)
    {
        SetTextColor(lpDrawItem->hDC, GetHiLiteTextColor());
		SetBkColor(lpDrawItem->hDC, GetHiLiteBkColor ());
    }
    else
    {
		SetTextColor(lpDrawItem->hDC, GetRowTextColor(lpDrawItem)); 
		SetBkColor(lpDrawItem->hDC, GetRowBkColor (lpDrawItem));
	}
	
	// draw each of the columns
	
	rcClip.left		= lpDrawItem->rcItem.left;
	rcClip.right	= lpDrawItem->rcItem.right;
    rcClip.top		= lpDrawItem->rcItem.top;
    rcClip.bottom	= lpDrawItem->rcItem.bottom;

	GetClientRect (GetHwnd(), &rcClient);

	if (SpanColumns(lpDrawItem))
	{
		rcClip.right = lpDrawItem->rcItem.right;

		buffer[0]='\0';
		GetText (iCol, lvi.lParam, (char *)&buffer);

		CCommand *pCmd = (CCommand *) lpDrawItem->itemData;
		if (pCmd->GetCommandType ()==TOK_COMMENT)
			DrawItemColumn(lpDrawItem->hDC, buffer,  &rcClip, m_italicfont);
		else
			DrawItemColumn(lpDrawItem->hDC, buffer,  &rcClip, m_font);

	//	DrawItemColumn(lpDrawItem->hDC, buffer,  &rcClip, 12);

		rcClip.left = rcClip.right;
	}
	else  
	{
		while ((iWidth = ListView_GetActualColumnWidth (iCol)) != -1)
		{
			rcClip.right = rcClip.left + iWidth;
			
			buffer[0]='\0';
	
			GetText (iCol, lvi.lParam, (char *)&buffer);
	
			if (iCol==1)
			{
				DrawItemColumn(lpDrawItem->hDC, buffer,  &rcClip, m_boldfont);
			}
			else
			if (iCol==3)
				DrawItem (iCol, lpDrawItem->hDC, &rcClip, lpDrawItem);
			else
				DrawItemColumn(lpDrawItem->hDC, buffer,  &rcClip, 12);
	
			rcClip.left = rcClip.right;

			++iCol;
		}
		rcClip.right = rcClient.right;
	}

	// reset the colors to default.

    if ((lpDrawItem->itemState & ODS_SELECTED) && m_bSelectable)
    {
		SetTextColor(lpDrawItem->hDC, GetRowTextColor(lpDrawItem)); 
		SetBkColor(lpDrawItem->hDC, GetRowBkColor(lpDrawItem));
    }

	// draw a extra column

	rcClip.right = rcClient.right;
	ExtTextOut(lpDrawItem->hDC, rcClip.left, rcClip.top , ETO_CLIPPED| ETO_OPAQUE,
               &rcClip, "", 0, NULL);


    // If the item is focused, now draw a focus rect around the entire row
    if ((lpDrawItem->itemState & ODS_FOCUS) && m_bSelectable)
    {
		GetClientRect (lpDrawItem->hwndItem, &rcClip);
		rcClip = lpDrawItem->rcItem;
        DrawFocusRect(lpDrawItem->hDC, &rcClip);
    }

	PaintListViewVoidAreas (lpDrawItem->hDC);
    return;
}
