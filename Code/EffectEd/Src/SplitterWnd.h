/*------------------------------------------------------------------------
 Module:        SPLITTERWND.H
 Author:        Daniel Guerrero Miralles
 Project:       TreeSize
 State:         alpha
 Creation Date: 17/10/98
 Description:   Implementation of a simple splitter
                window type (like MFC static CSplitterWnd
                but with only two panes). Pane
                disposition can be changed at run-time.
------------------------------------------------------------------------*/
#ifndef _SPLITTERWND_H_
#define _SPLITTERWND_H_

#include <windows.h>

/* --- Types --- */

typedef enum {
	SWS_HORIZONTAL,
    SWS_VERTICAL
} SPLITTERWNDSTYLE;

/* --- Prototypes --- */

BOOL SplitterWnd_RegisterClass (HINSTANCE hiInst );
HWND SplitterWnd_Create (HWND hwParent, HINSTANCE hiInst, UINT uChildId ,
	SPLITTERWNDSTYLE swsStyle, HCURSOR hCursor, LPRECT lprRect);

void SplitterWnd_GetPanes (HWND hwnd ,LPHANDLE lphPane1 ,LPHANDLE lphPane2 );
int SplitterWnd_GetSplitPos (HWND hwnd);
SPLITTERWNDSTYLE SplitterWnd_GetStyle (HWND hwnd);
HCURSOR SplitterWnd_GetCursor (HWND hwnd);

BOOL SplitterWnd_IsBarMoving (HWND hwnd);

void SplitterWnd_SetPanes (HWND hwnd ,HWND hwPane1 ,HWND hwPane2 );
void SplitterWnd_SetSplitPos (HWND hwnd ,int nPos );
void SplitterWnd_SetPercentSplitPos (HWND hwnd ,int nPercentPos );
void SplitterWnd_SetStyle (HWND hwnd, SPLITTERWNDSTYLE swsStyle);
void SplitterWnd_SetCursor (HWND hwnd, HCURSOR hCursor);

void SplitterWnd_Redraw (HWND hwnd);

#endif
