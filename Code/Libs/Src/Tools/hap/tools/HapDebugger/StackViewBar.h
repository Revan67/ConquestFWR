//////////////////////////////////////////////////////////////////////////////
//  File:    StackViewBar.h
//
//  Purpose: This is a dockable window which displays the call stack
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

#if !defined(AFX_STACKVIEWBAR_H__AF616C05_63FA_4EEC_AA02_EB376E7AB37B__INCLUDED_)
#define AFX_STACKVIEWBAR_H__AF616C05_63FA_4EEC_AA02_EB376E7AB37B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StackViewBar.h : header file
//

#include "sizecbar.h"
#include "scbarg.h"

/////////////////////////////////////////////////////////////////////////////
// CStackViewBar window

class CStackViewBar : public CSizingControlBarG
{
// Construction
public:
	CStackViewBar();
	virtual ~CStackViewBar();

	void SetCallStack(char_vector& vecCallStack);
	void UpdateStyles();

protected:
	CListBox m_List;
	HBRUSH m_hBkBrush;
	CFont m_Font;
	COLORREF m_ForeColor;
	COLORREF m_BackColor;

	string m_StackStr;
	char_vector m_vecCallStack;

	char* m_pStackBuffer;
	int m_nStackBufferSize;


	static void FormatStackEntry(CString& StackEntry);

	// Generated message map functions
protected:
	//{{AFX_MSG(CStackViewBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnClose();
	//}}AFX_MSG
	afx_msg void OnDblclickStackList(); 
	DECLARE_MESSAGE_MAP()

public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStackViewBar)
	//}}AFX_VIRTUAL
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STACKVIEWBAR_H__AF616C05_63FA_4EEC_AA02_EB376E7AB37B__INCLUDED_)
