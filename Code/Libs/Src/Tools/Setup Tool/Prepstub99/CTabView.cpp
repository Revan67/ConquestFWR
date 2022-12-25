/**************************************************************
* CTabView.cpp: A tab view control
*
* Chris N. Haddan
* May 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#include "CTabView.hpp"
#include "commctrl.h"
#include "malloc.h"

#define CTABVIEW_FRAME_CLASS "aas:CTabViewClass"


CTabView::CTabView ()
{
	m_hWnd = NULL;
	m_hWndFrame = NULL;
	m_hFont = NULL;
	m_cTabs = 0;
	m_cMaxTabs = 0;
	m_iTab = 0;
	m_papTabs = NULL;
}

CTabView::~CTabView()
{
	if (m_hFont)
		DeleteObject (m_hFont);

	if (m_hWnd)
		DestroyWindow (m_hWnd);

	if (m_hWndFrame)
		DestroyWindow (m_hWndFrame);

	m_hFont = NULL;
	m_hWnd = NULL;
	m_hWndFrame = NULL;

	for (int i=0; i<m_cTabs; i++)
	{
		free (m_papTabs[i]);
	}

	free (m_papTabs);
}

bool CTabView::Create (HINSTANCE hInst, HWND hwndParent, int x, int y, int width, int height)
{
	InitCommonControls ();
	
	WNDCLASSEX wc;

	// register the CListViewFrame window classes

	ZeroMemory (&wc, sizeof (wc));
	wc.cbSize = sizeof (WNDCLASSEX);

	if (!GetClassInfoEx (hInst, CTABVIEW_FRAME_CLASS, &wc))
	{ 
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = (WNDPROC) CTabViewWndFrameProc; 
		wc.cbClsExtra = 0; 
		wc.cbWndExtra = 0; 
		wc.hInstance = hInst;
		wc.hIcon = NULL;; 
		wc.hCursor = LoadCursor (NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = CTABVIEW_FRAME_CLASS; 

		if (!RegisterClassEx (&wc))
			return false;
	}

	m_iTab = 0;

	m_hWndFrame = CreateWindowEx (0L, CTABVIEW_FRAME_CLASS, "",
			WS_CHILD, 0, 0, 0, 0,
			hwndParent, (HMENU) this, hInst, NULL);

	if (m_hWndFrame == NULL) 
		return false;

	SetWindowLong (m_hWndFrame, GWL_USERDATA, (long)this);

	m_hWnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TABCONTROL, "", 
			WS_CHILD | WS_VISIBLE | WS_BORDER | TCS_BOTTOM,
			x, y, width, height, 
			m_hWndFrame, NULL, hInst, NULL);    

	if (m_hWnd == NULL)
		return false;

	HDC hdc = GetDC (m_hWnd);
	
	m_hFont = CreateFont (-MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH |FF_DONTCARE, "Arial");
	
	if (m_hFont == NULL)
		return false;

	ReleaseDC (m_hWnd, hdc);

	SendMessage (m_hWnd, WM_SETFONT, (WPARAM)m_hFont, MAKELPARAM(1,0));    

	ShowWindow (m_hWnd, SW_SHOW);
	ShowWindow (m_hWndFrame, SW_SHOW);
	
	return true;
}


bool CTabView::AddTab (char *szText)
{
	TC_ITEM tci;

	tci.mask = TCIF_TEXT | TCIF_IMAGE;
	tci.iImage = -1; 
    tci.pszText = szText;

	// insert a tab in the tab control

    if (TabCtrl_InsertItem(m_hWnd, TabCtrl_GetItemCount(m_hWnd), &tci) == -1) 
	{ 
		return false;
	}     

	// add data to private tab list

	if (m_cTabs >= m_cMaxTabs)
	{
		m_cMaxTabs  += TAB_ALLOC;

		if (m_cTabs == 0)
			m_papTabs = (TABDATA **) malloc (sizeof (TABDATA *) * m_cMaxTabs);
		else
			m_papTabs = (TABDATA **) realloc (m_papTabs, sizeof (TABDATA *) * m_cMaxTabs);

		if (m_papTabs == NULL) return false;
	}

	m_papTabs[m_cTabs++] = (TABDATA *) malloc (sizeof (TABDATA));

	return true;
}


bool CTabView::AttachTab (HWND hwnd, int iTabNumber)
{
	// add to list

	if (iTabNumber > m_cTabs)
		return false;
	
	m_papTabs[iTabNumber]->hwnd = hwnd;
	SetParent (hwnd, m_hWnd); 
	ShowWindow (hwnd, SW_HIDE);

	return true;
}

bool CTabView::Resize (int x, int y, int width, int height)
{
	SetWindowPos (m_hWndFrame, HWND_TOP, x, y, width, height, SWP_SHOWWINDOW);
	return true;
}



LRESULT CALLBACK CTabViewWndFrameProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CTabView *pTabView = (CTabView *)GetMenu (hWnd);

	return (pTabView->HandleFrameMessages (hWnd, msg, wParam, lParam));
}


LRESULT CALLBACK CTabView::HandleFrameMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (this == NULL) 
		return (DefWindowProc (hWnd, msg, wParam, lParam));

	switch (msg)
	{
		case WM_NOTIFY:
			{
				LPNMHDR pNm = (LPNMHDR) lParam; 

				switch (pNm->code) 
				{ 
					case TCN_SELCHANGE:
						{ 
							SetTab (TabCtrl_GetCurSel(m_hWnd));
							break;             
						}             
				}
				break;
			}
	
		case WM_ERASEBKGND:
			return 1;

		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				if (m_hWnd)
					MoveWindow (m_hWnd, 0, 0, LOWORD (lParam), HIWORD (lParam), true);
				
				// see which tab (window) has focus, and resize it.
				if (m_papTabs)
				{
					if (m_papTabs[m_iTab]->hwnd)
					{
						RECT rect;
						SetRect (&rect, 0, 0, LOWORD (lParam), HIWORD (lParam));
						TabCtrl_AdjustRect (m_hWnd, false, &rect);
						MoveWindow (m_papTabs[m_iTab]->hwnd, rect.left, rect.top, rect.right-rect.left-8, rect.bottom-rect.top-9, true);
					}
				}
			}
			break;
	}
	return (DefWindowProc (hWnd, msg, wParam, lParam));
}

bool CTabView::SetTab (int iTab)
{
	RECT rect;

	if (iTab >=0 && iTab < m_cMaxTabs)
	{
		TabCtrl_SetCurFocus (m_hWnd, iTab); 

		ShowWindow (m_papTabs[m_iTab]->hwnd, SW_HIDE);
		
		m_iTab = TabCtrl_GetCurSel(m_hWnd); 
			
		GetClientRect (m_hWndFrame, &rect);
		
		TabCtrl_AdjustRect (m_hWnd, false, &rect);

		MoveWindow (m_papTabs[m_iTab]->hwnd, rect.left , rect.top , rect.right-rect.left - 8, rect.bottom-rect.top - 9, true);

		ShowWindow (m_papTabs[m_iTab]->hwnd, SW_SHOW);

		return true;
	}
	else
		return false;
}

void CTabView::PreSize ()
{
	RECT rect;

	int i;

	for (i=0; i<m_cTabs; i++)
	{
		GetClientRect (m_hWndFrame, &rect);
		
		TabCtrl_AdjustRect (m_hWnd, false, &rect);

		MoveWindow (m_papTabs[m_iTab]->hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, true);
	}
}