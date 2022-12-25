//////////////////////////////////////////////////////////////////////////////
//  File:    ConnectStatusBar.h
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

#if !defined(AFX_CONNECTSTATUSBAR_H__DF3FE7DF_D507_4D0B_A381_FE4E9C879AF9__INCLUDED_)
#define AFX_CONNECTSTATUSBAR_H__DF3FE7DF_D507_4D0B_A381_FE4E9C879AF9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "sizecbar.h"
#include "scbarg.h"
/////////////////////////////////////////////////////////////////////////////
// CConnectStatusBar window

class CConnectStatusBar : public CSizingControlBarG
{
// Construction
public:
	CConnectStatusBar();
	virtual ~CConnectStatusBar();

	void SetStatus(EDebugState state, const char* szMsg);
	void SetClientInfo(const char* szMsg);
public:
    virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

	

protected:


protected:
	CEdit m_Edit;
	HBRUSH m_hBkBrush;
	CFont m_Font;

	CString m_StatusStr;
	CString m_ClientStr;

public:
	//{{AFX_VIRTUAL(CConnectStatusBar)
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(CConnectStatusBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONNECTSTATUSBAR_H__DF3FE7DF_D507_4D0B_A381_FE4E9C879AF9__INCLUDED_)
