//////////////////////////////////////////////////////////////////////////////
//  File:    VariableViewBar.h
//
//  Purpose: This is a docking window that provides a display of global and
//  local python variables.
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

#ifndef _VARIABLEVIEWBAR_H_
#define _VARIABLEVIEWBAR_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "sizecbar.h"
#include "scbarg.h"
#include "VarListCtrl.h"
/////////////////////////////////////////////////////////////////////////////
// CVariableViewBar window

class CVariableViewBar : public CSizingControlBarG
{
// Construction
public:
	CVariableViewBar();
	virtual ~CVariableViewBar();

	void UpdateGlobals(const char* szGlobals);
	void UpdateLocals(const char* szLocals);


	void ShowGlobals();
	void ShowLocals();

	void ModifyGlobalItem(int nItem, int nSubItem, const char* szNewVal);
	void ModifyLocalItem(int nItem, int nSubItem, const char* szNewVal);
	
	virtual void LoadState(LPCTSTR lpszProfileName);
    virtual void SaveState(LPCTSTR lpszProfileName);

protected:
	CVarListCtrl m_GlobalsList;
	CVarListCtrl m_LocalsList;

	CTabCtrl m_Tab;
	HBRUSH m_hBkBrush;
	CFont m_Font;

	int m_nListColWidth;

public:
	//{{AFX_VIRTUAL(CVariableViewBar)
	public:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(CVariableViewBar)
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

#endif // _VARIABLEVIEWBAR_H_
