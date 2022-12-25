//////////////////////////////////////////////////////////////////////////////
//  File:    MainFrm.cpp
//
//  Purpose: The main MDI frame window - this window controls the toolbar and
//  status bar for the app.
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
#include "stdafx.h"
#include "DbgRemote.h"

#include "MainFrm.h"
#include "DbgRemoteDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_COMMAND(ID_VIEW_WATCH, OnViewWatch)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WATCH, OnUpdateViewWatch)
	ON_COMMAND(ID_VIEW_CALLSTACK, OnViewCallstack)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CALLSTACK, OnUpdateViewCallstack)
	ON_COMMAND(ID_VIEW_DEBUGOUTPUT, OnViewDebugoutput)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DEBUGOUTPUT, OnUpdateViewDebugoutput)
	ON_COMMAND(ID_VIEW_LOCALS, OnViewLocals)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LOCALS, OnUpdateViewLocals)
	ON_COMMAND(ID_VIEW_PROJECT, OnViewProject)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PROJECT, OnUpdateViewProject)
	ON_WM_DROPFILES()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_READ, OnUpdateReadOnly)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_LINE, OnUpdateLineNum)

	ON_MESSAGE(NM_SENDINFO, OnSccMsgInfo)
	ON_MESSAGE(NM_SENDERROR, OnSccMsgErr)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_LINE,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
	ID_INDICATOR_READ,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_ConnectCheckTimer = -1;
	m_ReadTimer = -1;
	m_eCurBarState = eBARSTATE_INVALID;
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	DWORD dwTbStyle = WS_CHILD | WS_VISIBLE | CBRS_TOP | 
		              CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC;
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, dwTbStyle) || 
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

/*
	if (!m_wndDebugToolBar.CreateEx(this, TBSTYLE_FLAT, dwTbStyle, CRect(0, 0, 0, 0), 1511) ||
		!m_wndDebugToolBar.LoadToolBar(IDR_TOOLBAR_DBG))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndEditToolBar.CreateEx(this, TBSTYLE_FLAT, dwTbStyle, CRect(0, 0, 0, 0), 1512) ||
		!m_wndEditToolBar.LoadToolBar(IDR_TOOLBAR_EDIT))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
*/

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	if (!m_wndConnectStatusBar.Create(_T("Connection Status"), this, CSize(80, 80),TRUE, 123))
    {
        TRACE0("Failed to create ConnectStatusBar\n");
        return -1;      // fail to create
	}

	if (!m_wndStackViewBar.Create(_T("Call Stack"), this, CSize(120, 80),TRUE, 124))
    {
        TRACE0("Failed to create Stack Bar\n");
        return -1;      // fail to create
	}

	if (!m_wndTraceViewBar.Create(_T("Debug"), this, CSize(120, 120),TRUE, 125))
    {
        TRACE0("Failed to create Trace Bar\n");
        return -1;      // fail to create
	}
	if (!m_wndProjectViewBar.Create(_T("Project"), this, CSize(185, 80),TRUE, 126, WS_CHILD | WS_VISIBLE | CBRS_TOP))
    {
        TRACE0("Failed to create Project Bar\n");
        return -1;      // fail to create
	}
	if (!m_wndWatchBar.Create(_T("Watch"), this, CSize(140, 80),TRUE, 127))
    {
        TRACE0("Failed to create Project Bar\n");
        return -1;      // fail to create
	}
	if (!m_wndVarBar.Create(_T("Variables"), this, CSize(120, 80),TRUE, 128))
    {
        TRACE0("Failed to create Variable Bar\n");
        return -1;      // fail to create
	}

	// connection window
	m_wndConnectStatusBar.SetBarStyle(m_wndConnectStatusBar.GetBarStyle() |
            CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	
	// stack debug window
	m_wndStackViewBar.SetBarStyle(m_wndConnectStatusBar.GetBarStyle() |
            CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndStackViewBar.ShowWindow(FALSE);

	// this is the output window
	m_wndTraceViewBar.SetBarStyle(m_wndTraceViewBar.GetBarStyle() |
            CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	
	// the project workspace
	m_wndProjectViewBar.SetBarStyle(m_wndProjectViewBar.GetBarStyle() |
            CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	
	// watch debug window
	m_wndWatchBar.SetBarStyle(m_wndWatchBar.GetBarStyle() |
            CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndWatchBar.ShowWindow(FALSE);

	// variable debug window
	m_wndVarBar.SetBarStyle(m_wndVarBar.GetBarStyle() |
            CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndVarBar.ShowWindow(FALSE);

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndConnectStatusBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndStackViewBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndTraceViewBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndProjectViewBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndWatchBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndVarBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);	

	CRect rc;    
	
	//top
	DockControlBar(&m_wndToolBar);

	//bottom
	DockControlBar(&m_wndTraceViewBar, AFX_IDW_DOCKBAR_BOTTOM);
	RecalcLayout(TRUE);	
	m_wndStackViewBar.GetWindowRect(rc);
	DockControlBar(&m_wndWatchBar, AFX_IDW_DOCKBAR_BOTTOM, &rc);
	DockControlBar(&m_wndVarBar, AFX_IDW_DOCKBAR_BOTTOM, &rc);
	DockControlBar(&m_wndStackViewBar, AFX_IDW_DOCKBAR_BOTTOM);
	RecalcLayout(TRUE);
	
	//right	
	m_wndTraceViewBar.GetWindowRect(rc);
	DockControlBar(&m_wndConnectStatusBar, AFX_IDW_DOCKBAR_RIGHT, &rc);
	
	//left
	DockControlBar(&m_wndProjectViewBar, AFX_IDW_DOCKBAR_LEFT);

	SetBarState(eBARSTATE_EDIT);
	
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	CWinApp* pApp = AfxGetApp();
	cs.x = (int)pApp->GetProfileInt(_T("Settings"),_T("LeftWinCoord"),cs.x);
	cs.y = (int)pApp->GetProfileInt(_T("Settings"),_T("TopWinCoord"),cs.y);
	cs.cx = (int)pApp->GetProfileInt(_T("Settings"),_T("WinWidth"),cs.cx);
	cs.cy = (int)pApp->GetProfileInt(_T("Settings"),_T("WinHeight"),cs.cy);
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


void CMainFrame::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent == m_ConnectCheckTimer)
	{
		CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
		if (pApp)
			pApp->CheckConnect();
	}
	else if (nIDEvent == m_ReadTimer)
	{
		CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
		if (pApp)
			pApp->CheckRead();
	}
	
	CMDIFrameWnd::OnTimer(nIDEvent);
}

void CMainFrame::OnClose() 
{
	EndConnectCheckTimer();
	EndReadTimer();
	CMDIFrameWnd::OnClose();
}
#define CONNECT_TIMER_TIME 1000
#define CONNECT_TIMERID   11111

void CMainFrame::StartConnectCheckTimer()
{
	if (m_ConnectCheckTimer == -1)
		m_ConnectCheckTimer = SetTimer(CONNECT_TIMERID, CONNECT_TIMER_TIME, NULL);
}

void CMainFrame::EndConnectCheckTimer()
{
	if (m_ConnectCheckTimer != -1)
		KillTimer(m_ConnectCheckTimer);
	m_ConnectCheckTimer = -1;
}

#define READ_TIMERID		11112  
#define READ_TIMER_TIME		  100

void CMainFrame::StartReadTimer()
{
	if (m_ReadTimer == -1)
		m_ReadTimer = SetTimer(READ_TIMERID, READ_TIMER_TIME, NULL);
}

void CMainFrame::EndReadTimer()
{
	if (m_ReadTimer != -1)
		KillTimer(m_ReadTimer);
	m_ReadTimer = -1;
}


BOOL CMainFrame::DestroyWindow() 
{
	SetBarState(eBARSTATE_INVALID);

	// Record the window position for later.
	WINDOWPLACEMENT placement;
	GetWindowPlacement(&placement);
	RECT normalPosition = placement.rcNormalPosition;
	CWinApp* pApp = AfxGetApp();
	pApp->WriteProfileInt(_T("Settings"),_T("LeftWinCoord"), normalPosition.left);
	pApp->WriteProfileInt(_T("Settings"),_T("TopWinCoord"), normalPosition.top);
	pApp->WriteProfileInt(_T("Settings"),_T("WinWidth"), normalPosition.right - normalPosition.left);
	pApp->WriteProfileInt(_T("Settings"),_T("WinHeight"), normalPosition.bottom - normalPosition.top);
	pApp->WriteProfileInt(_T("Settings"),_T("ShowCmd"), placement.showCmd);

	return CMDIFrameWnd::DestroyWindow();
}
void CMainFrame::OnViewWatch() 
{
	BOOL bShow = m_wndWatchBar.IsVisible();
	ShowControlBar(&m_wndWatchBar, !bShow, FALSE);
}

void CMainFrame::OnUpdateViewWatch(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_wndWatchBar.IsVisible());
}

void CMainFrame::OnViewCallstack() 
{
	BOOL bShow = m_wndStackViewBar.IsVisible();
	ShowControlBar(&m_wndStackViewBar, !bShow, FALSE);	
}

void CMainFrame::OnUpdateViewCallstack(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_wndStackViewBar.IsVisible());
}

void CMainFrame::OnViewDebugoutput() 
{
	BOOL bShow = m_wndTraceViewBar.IsVisible();
	ShowControlBar(&m_wndTraceViewBar, !bShow, FALSE);
}

void CMainFrame::OnUpdateViewDebugoutput(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_wndTraceViewBar.IsVisible());
}

void CMainFrame::OnViewLocals() 
{
	BOOL bShow = m_wndVarBar.IsVisible();
	ShowControlBar(&m_wndVarBar, !bShow, FALSE);
}

void CMainFrame::OnUpdateViewLocals(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_wndVarBar.IsVisible());
}

void CMainFrame::OnViewProject() 
{
	BOOL bShow = m_wndProjectViewBar.IsVisible();
	ShowControlBar(&m_wndProjectViewBar, !bShow, FALSE);	
}

void CMainFrame::OnUpdateViewProject(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_wndProjectViewBar.IsVisible());
}

void CMainFrame::OnDropFiles(HDROP hDropInfo) 
{
	if (hDropInfo)
		CMDIFrameWnd::OnDropFiles(hDropInfo);
}


void CMainFrame::OnUpdateReadOnly(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(false);
}

void CMainFrame::OnUpdateLineNum(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(false);
}

const char* cszBarStates[] = 
{
	"Control Bars\\Edit\\BarState",
	"Control Bars\\Debug\\BarState",
};

void CMainFrame::SetBarState(EBarState eState)
{
	if ((eState != m_eCurBarState) && (m_eCurBarState != eBARSTATE_INVALID))
	{
		//save the current bar state
		m_wndConnectStatusBar.SaveState(cszBarStates[m_eCurBarState]);
		m_wndStackViewBar.SaveState(cszBarStates[m_eCurBarState]);
		m_wndTraceViewBar.SaveState(cszBarStates[m_eCurBarState]);
		m_wndProjectViewBar.SaveState(cszBarStates[m_eCurBarState]);
		m_wndWatchBar.SaveState(cszBarStates[m_eCurBarState]);
		m_wndVarBar.SaveState(cszBarStates[m_eCurBarState]);

		SaveBarState(cszBarStates[m_eCurBarState]);
	}

	if ((eState != m_eCurBarState) && (eState != eBARSTATE_INVALID))
	{
		//load the new bar state
		m_wndConnectStatusBar.LoadState(cszBarStates[eState]);
		m_wndStackViewBar.LoadState(cszBarStates[eState]);
		m_wndTraceViewBar.LoadState(cszBarStates[eState]);
		m_wndProjectViewBar.LoadState(cszBarStates[eState]);
		m_wndWatchBar.LoadState(cszBarStates[eState]);
		m_wndVarBar.LoadState(cszBarStates[eState]);

		LoadBarState(cszBarStates[eState]);
		m_eCurBarState = eState;
	}
	
}


void CMainFrame::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIFrameWnd::OnSetFocus(pOldWnd);

	CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
	pApp->OnFileModified();	
	
}


LRESULT CMainFrame::OnSccMsgInfo(WPARAM w, LPARAM l)
{
	if (w && strlen((const char*)w))
	{
		m_wndTraceViewBar.AddOutputMessage("SCC INFO:  ");
		m_wndTraceViewBar.AddOutputMessage((const char*)w);
		m_wndTraceViewBar.AddOutputMessage("\n");
	}
	return 1;
}

LRESULT CMainFrame::OnSccMsgErr(WPARAM w, LPARAM l)
{
	if (w && strlen((const char*)w))
	{
		m_wndTraceViewBar.AddOutputMessage("SCC ERROR: ");
		m_wndTraceViewBar.AddOutputMessage((const char*)w);
	}
	return 1;
}

BOOL CMainFrame::OnCommand(WPARAM wParam, LPARAM lParam)
{
	//this is something of a hack to route accelerated messages to the trace bar when
	//it has the focus.
	HWND hFocus = ::GetFocus();
	if (hFocus)
	{
		HWND hPar = ::GetParent(hFocus);
		if (hPar && (hPar == m_wndTraceViewBar))
		{
			if (m_wndTraceViewBar.OnCommand(wParam, lParam) )
			{
				return TRUE;
			}
		}
	}
	return CMDIFrameWnd::OnCommand(wParam, lParam);
}
