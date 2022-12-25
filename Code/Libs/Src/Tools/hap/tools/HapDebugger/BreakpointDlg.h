//////////////////////////////////////////////////////////////////////////////
//  File:    BreakpointDlg.h
//
//  Purpose: This is a dialog which displays the current set of breakpoints
//  (currently unimplemented)
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

#if !defined(AFX_BREAKPOINTDLG_H__E9C528DF_AECD_4844_BD1B_2A7922994884__INCLUDED_)
#define AFX_BREAKPOINTDLG_H__E9C528DF_AECD_4844_BD1B_2A7922994884__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BreakpointDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBreakpointDlg dialog

class CBreakpointDlg : public CDialog
{
// Construction
public:
	CBreakpointDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CBreakpointDlg)
	enum { IDD = IDD_EDITBREAKPT_DLG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
	


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBreakpointDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBreakpointDlg)
	afx_msg void OnDblclkBreakptList(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BREAKPOINTDLG_H__E9C528DF_AECD_4844_BD1B_2A7922994884__INCLUDED_)
