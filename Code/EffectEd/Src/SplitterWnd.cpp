/*------------------------------------------------------------------------
 Module:        SPLITTERWND.C
 Author:        Daniel Guerrero Miralles
 Project:       TreeSize
 State:         alpha
 Creation Date: 17/10/98
 Description:   Implementation of a simple splitter
                window type (like MFC static CSplitterWnd
                but with only two panes). Pane
                disposition can be changed at run-time.
------------------------------------------------------------------------*/

#include "stdafx.h"
#include <windows.h>
#include <windowsx.h>

#ifndef _SPLITTERWND_H_
	#include "splitterwnd.h" // import definition of SPLITTERWNDSTYLE and prototypes
#endif


/* +++ Internal constants +++ */

#define SPLITBAR_SIZE 4
#define MIN_PANESIZE 4


/* --- Private types --- */

typedef struct _SPLITTERINFO_TAG {
	SPLITTERWNDSTYLE swsStyle; // style of the splitter (SWS_HORIZONTAL or SWS_VERTICAL)
	HCURSOR hCursor;
	int nSplitPos;// Position of the split bar
	HWND hwPane1, hwPane2; // handles of the pane (view) windows
	BOOL fMovingBar;
} SPLITTERINFO, * LPSPLITTERINFO;


/* --- Private functions --- */

BOOL SplitterWnd_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	// WARNING !!!!
	// This is a hack! If the size of a HGLOBAL (the size of a pointer) is greater than 32 bits it
	// won't work! (may be in a future 64 bit version of Windows or in other plataforms?)
	SetWindowLong (hwnd, GWL_USERDATA,(LONG)lpCreateStruct->lpCreateParams);

	return TRUE;
}

void SplitterWnd_OnDestroy (HWND hwnd)
{
	LPSPLITTERINFO lpsiInfo;

	lpsiInfo = (LPSPLITTERINFO)GetWindowLong (hwnd, GWL_USERDATA);
	GlobalFree (lpsiInfo);
}

void SplitterWnd_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	if (state != SIZE_MINIMIZED)
	{
		// this will force a split pos recalculation
		SplitterWnd_SetSplitPos (hwnd, SplitterWnd_GetSplitPos(hwnd));
	}
	FORWARD_WM_SIZE (hwnd, state, cx, cy, DefWindowProc);
}

void SplitterWnd_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	LPSPLITTERINFO lpsiInfo;

	lpsiInfo = (LPSPLITTERINFO)GetWindowLong (hwnd, GWL_USERDATA);
	lpsiInfo->fMovingBar = TRUE;
	SetCapture (hwnd);
}

void SplitterWnd_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
	LPSPLITTERINFO lpsiInfo;

	lpsiInfo = (LPSPLITTERINFO)GetWindowLong (hwnd, GWL_USERDATA);
	if (lpsiInfo->fMovingBar)
	{
		lpsiInfo->fMovingBar = FALSE;
		ReleaseCapture ();
	}
}


void SplitterWnd_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
	// the class has no cursor, so we need to set it every time the mouse moves
	SetCursor(SplitterWnd_GetCursor(hwnd));

	if (SplitterWnd_IsBarMoving(hwnd))
	{
		if (SplitterWnd_GetStyle(hwnd) == SWS_HORIZONTAL)
		{
			SplitterWnd_SetSplitPos(hwnd, y);
		}
		else // SWS_VERTICAL
		{
			SplitterWnd_SetSplitPos(hwnd, x);
		}
	}
	FORWARD_WM_MOUSEMOVE(hwnd, x, y, keyFlags, DefWindowProc);
}

void SplitterWnd_OnPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd,&ps);
	EndPaint(hwnd,&ps);
};

LRESULT CALLBACK SplitterWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		HANDLE_MSG (hwnd, WM_CREATE, SplitterWnd_OnCreate);
		HANDLE_MSG (hwnd, WM_DESTROY, SplitterWnd_OnDestroy);
		HANDLE_MSG (hwnd, WM_SIZE, SplitterWnd_OnSize);
		HANDLE_MSG (hwnd, WM_LBUTTONDOWN, SplitterWnd_OnLButtonDown);
		HANDLE_MSG (hwnd, WM_LBUTTONUP, SplitterWnd_OnLButtonUp);
		HANDLE_MSG (hwnd, WM_MOUSEMOVE, SplitterWnd_OnMouseMove);
		HANDLE_MSG (hwnd, WM_PAINT, SplitterWnd_OnPaint);
	}
	return (DefWindowProc (hwnd, msg, wParam, lParam));
}


/* --- Public functions --- */

BOOL SplitterWnd_RegisterClass (HINSTANCE hiInst)
{
	WNDCLASS wc;

	wc.style = CS_HREDRAW|CS_VREDRAW|CS_NOCLOSE;
	wc.lpfnWndProc = (WNDPROC) SplitterWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 4; // for the pointer to the struct.
	wc.hInstance = hiInst;
	wc.hIcon = (HICON) NULL;
	wc.hCursor = (HCURSOR) NULL;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "SplitterWndClass";

	return (RegisterClass(&wc)!=(ATOM)NULL);
}

HWND SplitterWnd_Create (HWND hwParent, HINSTANCE hiInst, UINT uChildId, SPLITTERWNDSTYLE swsStyle, HCURSOR hCursor, LPRECT lprRect)
{
	HWND hwTheWindow;
	LPSPLITTERINFO lpsiWndInfo;
	RECT rParentRect;
	int x, y, nWidth, nHeight;

	lpsiWndInfo = (LPSPLITTERINFO)GlobalAlloc(GPTR, sizeof (SPLITTERINFO));
	if (lpsiWndInfo == NULL)
	{
		return ((HWND)NULL);
	}

	lpsiWndInfo->swsStyle = swsStyle;
	lpsiWndInfo->hCursor = hCursor;
	lpsiWndInfo->hwPane1 = (HWND)NULL;
	lpsiWndInfo->hwPane2 = (HWND)NULL;
	lpsiWndInfo->fMovingBar = FALSE;
	lpsiWndInfo->nSplitPos = 0;

	// test the rectangle. If NULL, the window will have the size of the parent client area.
	if (lprRect == NULL)
	{
		GetClientRect (hwParent, &rParentRect);
		x = 0;
		y = 0;
		nWidth = rParentRect.right;
		nHeight = rParentRect.bottom;
	}
	else
	{
		x = lprRect->left;
		y = lprRect->top;
		nWidth = lprRect->right - x;
		nHeight = lprRect->bottom - y;
	}

	hwTheWindow = CreateWindow("SplitterWndClass", "Splitter"
		, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN, x, y, nWidth, nHeight
		, hwParent, (HMENU)uChildId, hiInst, lpsiWndInfo);

	return hwTheWindow;
}

void SplitterWnd_SetPanes (HWND hwnd, HWND hwPane1, HWND hwPane2)
{
	LPSPLITTERINFO lpsiInfo;

	lpsiInfo = (LPSPLITTERINFO)GetWindowLong (hwnd, GWL_USERDATA);
	lpsiInfo->hwPane1 = hwPane1;
	lpsiInfo->hwPane2 = hwPane2;
	if (GetParent(hwPane1) != hwnd) SetParent (hwPane1, hwnd);
	if (GetParent(hwPane2) != hwnd) SetParent (hwPane2, hwnd);
}

void SplitterWnd_GetPanes (HWND hwnd, LPHANDLE lphPane1, LPHANDLE lphPane2)
{
	LPSPLITTERINFO lpsiInfo;

	lpsiInfo = (LPSPLITTERINFO)GetWindowLong (hwnd, GWL_USERDATA);
	*lphPane1 = (HANDLE)lpsiInfo->hwPane1;
	*lphPane2 = (HANDLE)lpsiInfo->hwPane2;
}

void SplitterWnd_SetSplitPos (HWND hwnd, int nNewPos)
{
	LPSPLITTERINFO lpsiInfo;
	RECT rcClient;
	int nPosMax;

	lpsiInfo = (LPSPLITTERINFO)GetWindowLong (hwnd, GWL_USERDATA);

	if (nNewPos < MIN_PANESIZE) nNewPos = MIN_PANESIZE;
	else
	{
		GetClientRect (hwnd, &rcClient);
		if (lpsiInfo->swsStyle == SWS_HORIZONTAL) nPosMax = rcClient.bottom - (MIN_PANESIZE + SPLITBAR_SIZE);
		else nPosMax = rcClient.right - (MIN_PANESIZE + SPLITBAR_SIZE);

		if (nNewPos > nPosMax)	nNewPos = nPosMax;
	}
	lpsiInfo->nSplitPos = nNewPos;
	SplitterWnd_Redraw (hwnd);
}

void SplitterWnd_SetPercentSplitPos (HWND hwnd, int nPercentPos)
{
	RECT rcClient;

	GetClientRect (hwnd, &rcClient);
	if (SplitterWnd_GetStyle(hwnd) == SWS_HORIZONTAL)
	{
		SplitterWnd_SetSplitPos (hwnd, (rcClient.bottom * nPercentPos) / 100);
	}
	else // SWS_VERTICAL
	{
		SplitterWnd_SetSplitPos (hwnd, (rcClient.right * nPercentPos) / 100);
	}
}

int SplitterWnd_GetSplitPos (HWND hwnd)
{
	LPSPLITTERINFO lpsiInfo;

	lpsiInfo = (LPSPLITTERINFO)GetWindowLong (hwnd, GWL_USERDATA);
	return lpsiInfo->nSplitPos;
}

BOOL SplitterWnd_IsBarMoving (HWND hwnd)
{
	LPSPLITTERINFO lpsiInfo;

	lpsiInfo = (LPSPLITTERINFO)GetWindowLong (hwnd, GWL_USERDATA);
	return lpsiInfo->fMovingBar;
}

void SplitterWindow_SetStyle (HWND hwnd, SPLITTERWNDSTYLE swsStyle)
{
	LPSPLITTERINFO lpsiInfo;

	lpsiInfo = (LPSPLITTERINFO)GetWindowLong (hwnd, GWL_USERDATA);
	lpsiInfo->swsStyle = swsStyle;
}

SPLITTERWNDSTYLE SplitterWnd_GetStyle (HWND hwnd)
{
	LPSPLITTERINFO lpsiInfo;

	lpsiInfo = (LPSPLITTERINFO)GetWindowLong (hwnd, GWL_USERDATA);
	return lpsiInfo->swsStyle;
}

void SplitterWnd_SetCursor (HWND hwnd, HCURSOR hCursor)
{
	LPSPLITTERINFO lpsiInfo;

	lpsiInfo = (LPSPLITTERINFO)GetWindowLong (hwnd, GWL_USERDATA);
	lpsiInfo->hCursor = hCursor;
}

HCURSOR SplitterWnd_GetCursor (HWND hwnd)
{
	LPSPLITTERINFO lpsiInfo;

	lpsiInfo = (LPSPLITTERINFO)GetWindowLong (hwnd, GWL_USERDATA);
	return lpsiInfo->hCursor;
}

void SplitterWnd_Redraw (HWND hwnd)
{
	int nPos;
	HWND hwPane1, hwPane2;
	RECT rcClient;

	nPos = SplitterWnd_GetSplitPos (hwnd);
	SplitterWnd_GetPanes (hwnd, (LPHANDLE)&hwPane1, (LPHANDLE)&hwPane2);
	GetClientRect (hwnd, &rcClient);

	if (SplitterWnd_GetStyle(hwnd) == SWS_HORIZONTAL)
	{
		MoveWindow (hwPane1, 0, 0, rcClient.right, nPos, TRUE);
		MoveWindow (hwPane2, 0, nPos + SPLITBAR_SIZE, rcClient.right, rcClient.bottom - (nPos + SPLITBAR_SIZE), TRUE);
	}
	else // SWS_VERTICAL
	{
		MoveWindow (hwPane1, 0, 0, nPos, rcClient.bottom, TRUE);
		MoveWindow (hwPane2, nPos + SPLITBAR_SIZE, 0, rcClient.right - (nPos + SPLITBAR_SIZE), rcClient.bottom, TRUE);
	}
}
