//////////////////////////////////////////////////////////////////////////////
//  File:    ToolConfigDlg.h
//
//  Purpose: This file defines a dialog box class that is used for configuring
//  the tools on the tools menu.  
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
#if !defined(AFX_TOOLCONFIGDLG_H__E5D83576_BC22_4D04_8BD0_93C5F0FB1F8D__INCLUDED_)
#define AFX_TOOLCONFIGDLG_H__E5D83576_BC22_4D04_8BD0_93C5F0FB1F8D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CToolConfigDlg dialog

class CToolConfigDlg : public CDialog
{
// Construction
public:
	CToolConfigDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CToolConfigDlg)
	enum { IDD = IDD_CONFIGTOOL_DLG };
	CString	m_CurToolName;
	CString	m_CurToolCmd;
	CString	m_CurToolDir;
	CString	m_CurToolParams;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CToolConfigDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateToolsList();

	// Generated message map functions
	//{{AFX_MSG(CToolConfigDlg)
	afx_msg void OnBrowseCmdBtn();
	afx_msg void OnBrowseDirBtn();
	afx_msg void OnAddBtn();
	afx_msg void OnRemoveBtn();
	afx_msg void OnSelchangeToolsLst();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TOOLCONFIGDLG_H__E5D83576_BC22_4D04_8BD0_93C5F0FB1F8D__INCLUDED_)
