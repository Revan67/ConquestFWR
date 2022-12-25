//////////////////////////////////////////////////////////////////////////////
//  File:    MainFrm.h
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

#if !defined(AFX_MAINFRM_H__87012C82_CF86_4E5A_A468_5F6DB95CC3DE__INCLUDED_)
#define AFX_MAINFRM_H__87012C82_CF86_4E5A_A468_5F6DB95CC3DE__INCLUDED_

#include "ConnectStatusBar.h"
#include "StackViewBar.h"
#include "TraceViewBar.h"
#include "ProjectViewBar.h"
#include "WatchBar.h"
#include "VariableViewBar.h"


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

enum EBarState
{
	eBARSTATE_INVALID = -1,
	eBARSTATE_EDIT,
	eBARSTATE_DEBUG,
};

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();
	virtual ~CMainFrame();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


	void StartConnectCheckTimer();
	void EndConnectCheckTimer();

	void StartReadTimer();
	void EndReadTimer();

	void SetBarState(EBarState eState);

	CTraceViewBar* pGetTraceViewBar() {return &m_wndTraceViewBar;}
	CStackViewBar* pGetStackViewBar() {return &m_wndStackViewBar;}
	CConnectStatusBar* pGetConnectStatusBar() {return &m_wndConnectStatusBar;}
	CProjectViewBar* pGetProjectViewBar() {return &m_wndProjectViewBar;}
	CWatchBar* pGetWatchBar() {return &m_wndWatchBar;}
	CVariableViewBar* pGetVariableBar() {return &m_wndVarBar;}

protected:
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
	CToolBar    m_wndDebugToolBar;
	CToolBar    m_wndEditToolBar;


	UINT m_ConnectCheckTimer;
	UINT m_ReadTimer;

	CConnectStatusBar  m_wndConnectStatusBar;
	CStackViewBar      m_wndStackViewBar;
	CTraceViewBar      m_wndTraceViewBar;
	CProjectViewBar    m_wndProjectViewBar;
	CWatchBar          m_wndWatchBar;
	CVariableViewBar   m_wndVarBar;

	EBarState m_eCurBarState;

public:
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL DestroyWindow();
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnClose();
	afx_msg void OnViewWatch();
	afx_msg void OnUpdateViewWatch(CCmdUI* pCmdUI);
	afx_msg void OnViewCallstack();
	afx_msg void OnUpdateViewCallstack(CCmdUI* pCmdUI);
	afx_msg void OnViewDebugoutput();
	afx_msg void OnUpdateViewDebugoutput(CCmdUI* pCmdUI);
	afx_msg void OnViewLocals();
	afx_msg void OnUpdateViewLocals(CCmdUI* pCmdUI);
	afx_msg void OnViewProject();
	afx_msg void OnUpdateViewProject(CCmdUI* pCmdUI);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	afx_msg void OnUpdateReadOnly(CCmdUI *pCmdUI);
	afx_msg void OnUpdateLineNum(CCmdUI *pCmdUI);
	afx_msg LRESULT OnSccMsgInfo(WPARAM, LPARAM);
	afx_msg LRESULT OnSccMsgErr(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__87012C82_CF86_4E5A_A468_5F6DB95CC3DE__INCLUDED_)
