/**************************************************************
* CListView.cpp: a list view class
*
* Chris N. Haddan
* April 22nd, 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#include "CListView.hpp"
#include "resource.h"
#include "string.h"
#include "math.h"


CListView::CListView ()
{
	m_hWnd					= NULL;
	m_hInst					= NULL;
	m_font					= NULL;
	m_dwStyle				= 0;
	m_dwExStyle				= LVS_EX_FULLROWSELECT;
	m_rgbBkColor			= GetSysColor (COLOR_WINDOW);
	m_rgbTextColor			= GetSysColor (COLOR_WINDOWTEXT);
	m_rgbHiLiteTextColor	= GetSysColor (COLOR_HIGHLIGHTTEXT);
	m_rgbHiLiteBkColor		= GetSysColor (COLOR_HIGHLIGHT);
	m_nRowHeight			= 12;
	m_bSelectable			= true;
	m_cColumns				= 0;
}	


CListView::~CListView ()
{
}


int CListView::GetItemWidth (int iCol, int iRow)
{
	SIZE sizeString;
	char szBuffer[MAX_PATH*4];
	LV_ITEM lvi;

	lvi.iItem = iRow;
	lvi.mask = LVIF_PARAM;
	ListView_GetItem (m_hWnd, &lvi);

	GetText (iCol, lvi.lParam, (char *)&szBuffer);

	HDC hdc = GetDC (m_hWnd);
	HFONT hFontOld = (HFONT)SelectObject (hdc, GetFont());

	GetTextExtentPoint32(hdc, szBuffer, lstrlen (szBuffer), &sizeString);	

	SelectObject (hdc, hFontOld);
	ReleaseDC (m_hWnd, hdc);
	
	return (sizeString.cx);
}

void CListView::AutoSizeColumn (int iCol)
{

	//char szBuffer[MAX_PATH*4];
	int nRows = ListView_GetItemCount (m_hWnd);

	int nWidth = ListView_GetColumnWidth (m_hWnd, iCol);
	int nItemWidth;

	for (int i=0; i<nRows; i++)
	{
		nItemWidth = GetItemWidth (iCol, i);//ListView_GetStringWidth (m_hWnd, szBuffer);

		if (nItemWidth > nWidth) 
		{
			nWidth = nItemWidth;
		}
	}
	
	ListView_SetColumnWidth (m_hWnd, iCol, (int)(double)(nWidth*1.20));
}


int CListView::GetRowHeight()
{
	return m_nRowHeight;
}


HFONT CListView::GetFont ()
{
	return m_font;
}


bool CListView::Create (HINSTANCE hInst, HWND hwndParent)
{
	RECT rect;
	GetClientRect (hwndParent, &rect);

	return (Create (hInst, hwndParent, 0, 0, rect.right-rect.left, rect.bottom-rect.top));
}


bool CListView::Create (HINSTANCE hInst, HWND hwndParent, int x, int y, int width, int height)
{
	InitCommonControls ();
	
	WNDCLASSEX wc;

	// register the CListViewFrame window classes

	ZeroMemory (&wc, sizeof (wc));
	wc.cbSize = sizeof (WNDCLASSEX);

	if (!GetClassInfoEx (hInst, CLISTVIEW_FRAME_CLASS, &wc))
	{ 
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc =  (WNDPROC) CListViewWndFrameProc; 
		wc.cbClsExtra = 0; 
		wc.cbWndExtra = 0; 
		wc.hInstance = hInst;
		wc.hIcon = NULL;; 
		wc.hCursor = LoadCursor (NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = CLISTVIEW_FRAME_CLASS; 

		if (!RegisterClassEx (&wc))
		{
			return false;
		}
	}
	
	m_hWndFrame = CreateWindowEx (0L,CLISTVIEW_FRAME_CLASS, "",
			WS_CHILD, x, y, width, height,
			hwndParent, (HMENU) this, hInst, NULL);

	if (m_hWndFrame == NULL) 
	{
		return false;
	}

	m_hWnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", 
			WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | m_dwStyle, 
			0, 0, width, height, 
			m_hWndFrame, (HMENU) this, hInst, NULL);    

	if (m_hWnd == NULL)
	{
		return false;
	}

	//
	// subclass the list view
	//

	m_lPreviousWndProc = GetWindowLong (m_hWnd, GWL_WNDPROC);

	SetWindowLong (m_hWnd, GWL_WNDPROC, (long)ListViewWndProc);

	ListView_SetExtendedListViewStyle (m_hWnd, m_dwExStyle);
	m_hInst = hInst;

	ShowWindow (m_hWnd, SW_SHOW);
	ShowWindow (m_hWndFrame, SW_SHOW);
	UpdateWindow (m_hWnd);
	UpdateWindow (m_hWndFrame);
	return true;
}


bool CListView::Resize (int x, int y, int width, int height)
{
	SetWindowPos (m_hWnd, HWND_TOP, x, y, width, height, SWP_SHOWWINDOW);
	return true;
}

void CListView::Refresh ()
{
	InvalidateRect (GetHwnd(), NULL, true);
	InvalidateRect (GetLvHwnd(), NULL, true);
}


BOOL CListView::DeleteAllItems ()
{
	ListView_DeleteAllItems (m_hWnd);
	ListView_RedrawItems (m_hWnd, 0, 0);
	UpdateWindow (m_hWnd);
	return TRUE;
}


bool CListView::CreateImageList ()
{

	
	return true;
}


bool CListView::AddColumn (char *szLabel)
{
   	return (AddColumn (szLabel, 100));
} 


bool CListView::AddColumn (char *szLabel, int iWidth)
{
   	LV_COLUMN lvc; 
	
	// Initialize the LV_COLUMN structure. 

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
    lvc.fmt = LVCFMT_LEFT;     
	lvc.cx = iWidth;     
	
	lvc.iSubItem = 1; 

	lvc.pszText = szLabel;

	if (ListView_InsertColumn(m_hWnd, 0, &lvc) == -1) 
	{
	    return false;
	}
	
	++m_cColumns;

	return true;
} 


bool CListView::AddItem (void *pvItem)
{
	LV_ITEM lvi;

	ZeroMemory (&lvi, sizeof (lvi));
    lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM; 
    lvi.state = 0;     
	lvi.stateMask = 0; 
    lvi.iImage = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK; 
	lvi.iItem = ListView_GetItemCount (m_hWnd);
	lvi.iSubItem = 0;         
	lvi.lParam = (LPARAM) pvItem;

	ListView_InsertItem(m_hWnd, &lvi);  

	return true;
}


LRESULT CALLBACK ListViewWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// The pointer to the object instance is stored in the menu handle.
	// Get the instance pointer and then dispatch the message to that
	// instance.

	CListView *pListView = (CListView *)GetMenu (hWnd);
	
	switch (msg)
	{
		case WM_ERASEBKGND:
			return 1;

		case WM_RBUTTONDOWN:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			{
				if (!pListView->GetSelectable())
					pListView->ProcessMessages (hWnd, msg, wParam, lParam);
			}
	}
	return (CallWindowProc ((SCWNDPROC)pListView->GetPreviousWndProcAddr(), hWnd, msg, wParam, lParam));
}


LRESULT CALLBACK CListViewWndFrameProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// The pointer to the object instance is stored in the menu handle.
	// Get the instance pointer and then dispatch the message to that
	// instance.

	CListView *pListView = (CListView *)GetMenu (hWnd);

	switch (msg)
	{
		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				if (pListView->GetLvHwnd())
					MoveWindow (pListView->GetLvHwnd(), 0, 0, LOWORD (lParam), HIWORD (lParam), true);
			}
			return 0;
	}
	return (pListView->ProcessMessages (hWnd, msg, wParam, lParam));
}


void CListView::Delete()
{
	DestroyWindow (m_hWnd);
	DestroyWindow (m_hWndFrame);
	m_hWnd = NULL;
	m_hInst = NULL;

}


CListView *GetObjectPtrFromHwnd (HWND hWnd)
{
	return ((CListView *)GetMenu (hWnd));
}


COLORREF CListView::GetBkColor ()
{
	return m_rgbBkColor;
}


COLORREF CListView::GetTextColor ()
{
	return m_rgbTextColor;
}


COLORREF CListView::GetHiLiteTextColor ()
{
	return m_rgbHiLiteTextColor;
}


COLORREF CListView::GetHiLiteBkColor ()
{
	return m_rgbHiLiteBkColor;
}


void CListView::PaintListViewVoidAreas (HDC hdc)
{
	LV_HITTESTINFO hti;
	int iStart, iTotal, iPotentialItemsOnScreen;
	int iHeaderHeight;
	int iTotalWidth = 0;
	int iCol = 0;
	RECT rect;
	HWND hwndH;
	HBRUSH hbr;
	RECT bRect;

	hbr = CreateSolidBrush (GetBkColor());

	//
	//  We need to determine where the list view is displaying data,
	//  and then using that information figure out where it is not
	//  displaying data so we can paint the background. 
	//

	// figure out the current header height
	hwndH = ListView_GetHeader(GetLvHwnd());
	GetClientRect (hwndH, &rect);
	iHeaderHeight = rect.bottom-rect.top;

	// see which list view item is at the top of the list view display
	hti.pt.x = 1;
	hti.pt.y = iHeaderHeight + 1;
	hti.flags = LVHT_ONITEM;
	iStart = ListView_HitTest (GetLvHwnd(), &hti) + 1;
	
	iTotal = ListView_GetItemCount (GetLvHwnd());

	GetClientRect (GetLvHwnd(), &rect);
	iPotentialItemsOnScreen = (int)ceil ((double)(rect.bottom - rect.top - iHeaderHeight) / (double)GetRowHeight());

	// Check to see if there are less items displayed than possible, which tells
	// us if there is a void space at the bottom.

	if (iStart == 0)
		iStart = 1;
	{
		for (int i=(iTotal - iStart)+1;i<iPotentialItemsOnScreen;i++)
		{
			
			SetRect (&bRect, 0, iHeaderHeight + (i* GetRowHeight()), rect.right, iHeaderHeight + (i* GetRowHeight())+GetRowHeight());
			HBRUSH hbr2 = CreateSolidBrush (RGB (168-((i % 2)*5), 152-((i % 2)*5), 144-((i % 2)*5)));
			FillRect (hdc, &bRect, (HBRUSH)(hbr2));
			DeleteObject (hbr2);
		}
	}
	/*else
	if ((iTotal - iStart) <= iPotentialItemsOnScreen)
	{
		SetRect (&bRect, 0, iHeaderHeight + ((iTotal - iStart + 1) * GetRowHeight()), rect.right, rect.bottom);
		FillRect (hdc, &bRect, (HBRUSH)(hbr));
	}*/
	DeleteObject (hbr);
}


int CListView::ListView_GetActualColumnWidth (int iCol)
{
	LV_COLUMN Col;

	ZeroMemory (&Col, sizeof(Col));
	Col.mask = LVCF_WIDTH;

	if (ListView_GetColumn(GetLvHwnd(), iCol, &Col))
	{
		return Col.cx;
	}
	else
	{
		return -1;
	}
}


void CListView::DrawItem (int iItem, HDC hdc, LPRECT prcClip, LPDRAWITEMSTRUCT lpDrawItem)
{
	LV_ITEM lvi;

	lvi.mask = LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvi.iItem = lpDrawItem->itemID;
	lvi.iSubItem = 0;
	ListView_GetItem(lpDrawItem->hwndItem, &lvi);

	HBRUSH hbr = CreateSolidBrush (GetRowBkColor(lpDrawItem));
	//RGB (GetRValue (GetBkColor())-((lvi.iItem % 2)*5), GetGValue (GetBkColor())-((lvi.iItem % 2)*5), GetBValue (GetBkColor())-((lvi.iItem % 2)*5)));
	FillRect (hdc, prcClip, hbr);
	DeleteObject (hbr);
}


void CListView::DrawListViewItem(LPDRAWITEMSTRUCT lpDrawItem)
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

    if (lpDrawItem->itemState & ODS_SELECTED & m_bSelectable)
    {
        SetTextColor(lpDrawItem->hDC, GetHiLiteTextColor());
		SetBkColor(lpDrawItem->hDC, GetHiLiteBkColor ());
    }
    else
    {
		SetTextColor(lpDrawItem->hDC, GetRowTextColor(lpDrawItem)); 
		SetBkColor(lpDrawItem->hDC, GetRowBkColor (lpDrawItem));
			//RGB (GetRValue (GetBkColor())-((lvi.iItem % 2)*5), GetGValue (GetBkColor())-((lvi.iItem % 2)*5), GetBValue (GetBkColor())-((lvi.iItem % 2)*5)));
	}
	
	// draw each of the columns
	
	rcClip.left		= lpDrawItem->rcItem.left;
	rcClip.right	= lpDrawItem->rcItem.right;
    rcClip.top		= lpDrawItem->rcItem.top;
    rcClip.bottom	= lpDrawItem->rcItem.bottom;

	GetClientRect (GetHwnd(), &rcClient);

	if (SpanColumns(lpDrawItem/*lvi.lParam*/))
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
	
			//if (iCol==5)
				//DrawItem (iCol, lpDrawItem->hDC, &rcClip, lpDrawItem);
			//else
				DrawItemColumn(lpDrawItem->hDC, buffer,  &rcClip, 12);
	
			rcClip.left = rcClip.right;

			++iCol;
		}
		rcClip.right = rcClient.right;
	}

	// reset the colors to default.

    if (lpDrawItem->itemState & ODS_SELECTED & m_bSelectable)
    {
		SetTextColor(lpDrawItem->hDC, GetRowTextColor(lpDrawItem)); 
		SetBkColor(lpDrawItem->hDC, GetRowBkColor(lpDrawItem));
		//RGB (GetRValue (GetBkColor())-((lvi.iItem % 2)*5), GetGValue (GetBkColor())-((lvi.iItem % 2)*5), GetBValue (GetBkColor())-((lvi.iItem % 2)*5)));
    }

	// draw a extra column
	rcClip.right = rcClient.right;

	//DrawItemColumn(lpDrawItem->hDC, "",  &rcClip, 12, pLv);
	DrawItem (10, lpDrawItem->hDC, &rcClip, lpDrawItem);

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


void CListView::DrawItemColumn(HDC hdc, LPTSTR lpsz, LPRECT prcClip, int nFont)
{
	HFONT m_old;
    TCHAR szString[MAX_PATH*4];

    // Check to see if the string fits in the clip rect.  If not, truncate
    // the string and add "...".
    lstrcpy(szString, lpsz);
    CalcStringEllipsis(hdc, szString, 256, prcClip->right - prcClip->left);

	m_old = (HFONT)SelectObject (hdc, GetFont());

    // print the text
    ExtTextOut(hdc, prcClip->left + 2, prcClip->top+1, ETO_CLIPPED| ETO_OPAQUE,
             prcClip, szString, lstrlen(szString), NULL);
	

	SelectObject (hdc, m_old);
}

void CListView::DrawItemColumn(HDC hdc, LPTSTR lpsz, LPRECT prcClip, HFONT hFont)
{
	HFONT m_old;
    TCHAR szString[256];

    // Check to see if the string fits in the clip rect.  If not, truncate
    // the string and add "...".
    lstrcpy(szString, lpsz);
    CalcStringEllipsis(hdc, szString, 256, prcClip->right - prcClip->left);

	m_old = (HFONT)SelectObject (hdc, hFont);

    // print the text
    ExtTextOut(hdc, prcClip->left + 2, prcClip->top+1, ETO_CLIPPED| ETO_OPAQUE,
             prcClip, szString, lstrlen(szString), NULL);
	

	SelectObject (hdc, m_old);
}


BOOL CListView::CalcStringEllipsis(HDC hdc, LPTSTR lpszString, int cchMax, UINT uColWidth)
{
    const TCHAR szEllipsis[] = TEXT("...");
    SIZE   sizeString;
    SIZE   sizeEllipsis;
    int    cbString;
    LPTSTR lpszTemp;
    BOOL   fSuccess = FALSE;
    static BOOL fOnce = TRUE;
    static FARPROC pGetTextExtentPoint;

    // Adjust the column width to take into account the edges
    uColWidth -= 4;

    __try
    {
        // Allocate a string for us to work with.  This way we can mangle the
        // string and still preserve the return value
        lpszTemp = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cchMax);
        if (!lpszTemp)
        {

            __leave;
        }
        lstrcpy(lpszTemp, lpszString);

        // Get the width of the string in pixels
        cbString = lstrlen(lpszTemp);
        if (!GetTextExtentPoint32(hdc, lpszTemp, cbString, &sizeString))
        {

            __leave;
        }

        // If the width of the string is greater than the column width shave
        // the string and add the ellipsis
        if ((ULONG)sizeString.cx > uColWidth)
        {
            if (!GetTextExtentPoint32(hdc, szEllipsis, lstrlen(szEllipsis),
                                       &sizeEllipsis))
            {

                __leave;
            }

            while (cbString > 0)
            {
                lpszTemp[--cbString] = 0;
                if (!GetTextExtentPoint32(hdc, lpszTemp, cbString, &sizeString))
                {
        
                    __leave;
                }

                if ((ULONG)(sizeString.cx + sizeEllipsis.cx) <= uColWidth)
                {
                    // The string with the ellipsis finally fits, now make sure
                    // there is enough room in the string for the ellipsis
                    if (cchMax >= (cbString + lstrlen(szEllipsis)))
                    {
                        // Concatenate the two strings and break out of the loop
                        lstrcat(lpszTemp, szEllipsis);
                        lstrcpy(lpszString, lpszTemp);
                        fSuccess = TRUE;
                        __leave;
                    }
                }
            }
        }
        else
        {
            // No need to do anything, everything fits great.
            fSuccess = TRUE;
        }
    }
    __finally
    {
        // Free the memory
        HeapFree(GetProcessHeap(), 0, (LPVOID)lpszTemp);
        return (fSuccess);
    }
}


COLORREF CListView::GetRowTextColor (LPDRAWITEMSTRUCT lpDrawItem)
{
	return (GetTextColor());
}


COLORREF CListView::GetRowBkColor (LPDRAWITEMSTRUCT lpDrawItem)
{
	return (GetBkColor());
}


bool CListView::SpanColumns (LPDRAWITEMSTRUCT lpDrawItem)
{
	return false;
}
