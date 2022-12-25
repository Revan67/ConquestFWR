//////////////////////////////////////////////////////////////////////////////
//  File:    ConnectStatusBar.cpp
//
//  Purpose: This is a docking control bar that displays information related
//  to the socket connection with the debug engine
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001 by Humongous Entertainment
//  
//  This library is free software; you can redistribute it and/or modify it 
//  under the terms of the GNU Lesser General Public License as published by 
//  the Free Software Foundation; either version 2.1 of the License, or  any 
//  later version.
//  
//  This library is distributed in the hope that it will be useful, but 
//  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
//  License for more details.
//  
//  You should have received a copy of the GNU Lesser General Public License 
//  along with this library (see the file license.txt); if not, write to the 
//  Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//////////////////////////////////////////////////////////////////////////////	

// ConnectStatusBar.cpp : implementation file
//

#include "stdafx.h"
#include "DbgRemote.h"
#include "ConnectStatusBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConnectStatusBar

CConnectStatusBar::CConnectStatusBar()
{
	m_StatusStr = "";
	m_ClientStr = "";
}

CConnectStatusBar::~CConnectStatusBar()
{
}


BEGIN_MESSAGE_MAP(CConnectStatusBar, CSizingControlBar)
	//{{AFX_MSG_MAP(CConnectStatusBar)
	ON_WM_CREATE()
    ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CConnectStatusBar message handlers

int CConnectStatusBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CSizingControlBar::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (! m_Edit.Create(WS_CHILD|WS_VISIBLE|ES_MULTILINE/*|ES_READONLY|WS_BORDER|WS_VSCROLL*/|WS_HSCROLL, CRect(0,0,10,10), this, 1010) )
		return -1;

	if (!m_Font.CreatePointFont(100, "Courier"))
        return -1;

    m_Edit.SetFont(&m_Font);


	m_hBkBrush = ::CreateSolidBrush(RGB(0,0,0));
    return 0;
}

void CConnectStatusBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
    CSizingControlBar::OnUpdateCmdUI(pTarget, bDisableIfNoHndler);

    UpdateDialogControls(pTarget, bDisableIfNoHndler);
}

void CConnectStatusBar::OnSize(UINT nType, int cx, int cy) 
{
    CSizingControlBar::OnSize(nType, cx, cy);

    // TODO: Add your message handler code here
    CRect rc;
    GetClientRect(rc);

    m_Edit.MoveWindow(rc);
}


void  CConnectStatusBar::SetStatus(EDebugState state, const char* szMsg)
{
	m_StatusStr = szMsg;
	CString Temp = m_StatusStr;
	Temp += "\r\n";
	Temp += m_ClientStr;
	m_Edit.SetWindowText(Temp);
}

void CConnectStatusBar::SetClientInfo(const char* szMsg)
{
	m_ClientStr = szMsg;

	CString Temp = m_StatusStr;
	Temp += "\r\n";
	Temp += m_ClientStr;
	m_Edit.SetWindowText(Temp);
}

HBRUSH CConnectStatusBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	if (nCtlColor == CTLCOLOR_EDIT)
	{
		pDC->SetBkColor(RGB(0, 0, 0)); // change the background color
		pDC->SetTextColor(RGB(255,255,255)); // change the text color

		return m_hBkBrush;
	}
	return CSizingControlBar::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CConnectStatusBar::OnClose() 
{
	DeleteObject(m_hBkBrush);
	CSizingControlBar::OnClose();
}
