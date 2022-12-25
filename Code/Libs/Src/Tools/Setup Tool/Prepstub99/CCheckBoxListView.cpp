/***********************************************************************
* CCheckBoxListView.cpp : Derived ListView class that supports Checkboxes.
*
* Chris N. Haddan
* April 1998
*
* (C) 1998 Microsoft Corporation
*
************************************************************************/
#include "CCheckBoxListView.hpp"
#include "stateinfo.h"
#include "resource.h"
extern HINSTANCE g_hAppInst;

bool CCheckBoxListView::SpanColumns(LPDRAWITEMSTRUCT lpDrawItem)
{
	return false;
}


COLORREF CCheckBoxListView::GetRowTextColor (LPDRAWITEMSTRUCT lpDrawItem)
{
	if (IsWindowEnabled (GetParent (m_hWndFrame)))
	{
		return (GetTextColor());
	}
	else
	{
		return (RGB (128,128,128));
	}
}


COLORREF CCheckBoxListView::GetRowBkColor (LPDRAWITEMSTRUCT lpDrawItem)
{	
	COLORREF bkColor;
	bkColor = RGB (31,105,33);
	bkColor = GetBkColor();
	return (RGB (GetRValue (bkColor)-((lpDrawItem->itemID % COLOR_GRAD)*COLOR_VAR), GetGValue (bkColor)-((lpDrawItem->itemID % COLOR_GRAD)*COLOR_VAR), GetBValue (bkColor)-((lpDrawItem->itemID  % COLOR_GRAD)*COLOR_VAR)));
}


LRESULT CALLBACK CCheckBoxListView ::ProcessMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_RBUTTONDOWN:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
				LV_HITTESTINFO hti;
				LV_ITEM lvi;

				ZeroMemory (&lvi, sizeof (LV_ITEM));

				hti.pt.x = LOWORD (lParam);
				hti.pt.y = HIWORD (lParam);

				hti.flags = LVHT_ONITEM;
				lvi.iItem = ListView_HitTest (GetLvHwnd(), &hti);
				
				if (lvi.iItem == -1)
					return false;

				lvi.mask = LVIF_PARAM;
				ListView_GetItem(GetLvHwnd(), &lvi);

				STATEINFO *si;
				si = (STATEINFO *)lvi.lParam;
				
				lvi.iSubItem = 0;
				lvi.mask = LVIF_IMAGE;

				if (si->cStates > 0)
				{
					si->nState = (si->nState + 1) % si->cStates;
				}
							
				lvi.iImage = si->nState;

				ListView_SetItem (GetLvHwnd(), &lvi);
				return true;
			break;

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
				return true;
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


void CCheckBoxListView::GetText (int iCol, LPARAM lParam, char *szString)

{
	static char buffer[255];
	wsprintf (buffer, "Item %d", iCol);
	lstrcpy (szString, buffer);
}


void CCheckBoxListView::DrawItem (int iItem, HDC hdc, LPRECT prcClip, LPDRAWITEMSTRUCT lpDrawItem)
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

	STATEINFO *si;
	si = (STATEINFO *)lvi.lParam;

	switch (iItem)
	{
		case 0:

			if (si->nState == -1)
			{
				break;
			}

			himl = ListView_GetImageList(lpDrawItem->hwndItem, LVSIL_SMALL);

			ImageList_GetIconSize(himl, &cxImage, &cyImage);

			ImageList_DrawEx (	himl, 
								si->nState + (IsWindowEnabled(GetParent(m_hWndFrame))?0:3), 
								hdc,  
								prcClip->left+1, 
								prcClip->top+2, 
								cxImage, 
								cyImage, 
								rgbBk,CLR_NONE,
								ILD_TRANSPARENT);
			break;
		case 1:
			DrawItemColumn(lpDrawItem->hDC, si->szName,  prcClip, 12);
			break;
	}
	SelectClipRgn (hdc, NULL);
	DeleteObject (hbr);
	DeleteObject (hrgn);
}


void CCheckBoxListView::DrawListViewItem(LPDRAWITEMSTRUCT lpDrawItem)
{
    LV_ITEM lvi;
    int cxImage = 0, cyImage = 0;
    RECT rcClip;
    int iColumn = 1;
	UINT uiFlags = ILD_TRANSPARENT;
	int iWidth, iCol = 0;
	char buffer[255];
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

	//	DrawItem (1, lpDrawItem->hDC, &rcClip, lpDrawItem, pLv);
		DrawItemColumn(lpDrawItem->hDC, buffer,  &rcClip, 12);

		rcClip.left = rcClip.right;
	}
	else  
	{
		while ((iWidth = ListView_GetActualColumnWidth (iCol)) != -1)
		{
			rcClip.right = rcClip.left + iWidth;
			
			buffer[0]='\0';
	
			GetText (iCol, lvi.lParam, (char *)&buffer);
			DrawItem (iCol, lpDrawItem->hDC, &rcClip, lpDrawItem);
	
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


void AddCheckBoxImageList(HWND hWnd)
{
    HIMAGELIST himlSmall;   
    
    himlSmall = ImageList_Create (11, 11, TRUE, 1, 1);
	HBITMAP hbm = LoadBitmap (g_hAppInst, MAKEINTRESOURCE (IDB_CHECKBOXES));

	ImageList_Add (himlSmall, hbm, NULL);
   
    ListView_SetImageList(hWnd, himlSmall, LVSIL_SMALL);   
	DeleteObject (hbm);
}

void RemoveCheckBoxImageList(HWND hWnd)
{
	ImageList_Destroy (ListView_GetImageList (hWnd, LVSIL_SMALL));
}

