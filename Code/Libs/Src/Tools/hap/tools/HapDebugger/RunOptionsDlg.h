//////////////////////////////////////////////////////////////////////////////
//  File:    RunOptionsDlg.h
//
//  Purpose: This dialog allows you to specify options related to debugging (
//  and running) the python program.
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

#if !defined(AFX_RunOptionsDlg_H__BB8EA509_9F35_4F40_841F_BC56B91518F4__INCLUDED_)
#define AFX_RunOptionsDlg_H__BB8EA509_9F35_4F40_841F_BC56B91518F4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CRunOptionsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRunOptionsDlg dialog

class CRunOptionsDlg : public CDialog
{
// Construction
public:
	CRunOptionsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRunOptionsDlg)
	enum { IDD = IDD_RUNOPTIONS_DLG };
	CString	m_RunParameters;
	CString	m_RunScript;
	CString	m_WorkingDir;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRunOptionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRunOptionsDlg)
	afx_msg void OnScriptBrowseBtn();
	afx_msg void OnWorkdirBrowseBtn();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void PASCAL DDV_WorkingDir(CDataExchange* pDX, CString workingDir);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RunOptionsDlg_H__BB8EA509_9F35_4F40_841F_BC56B91518F4__INCLUDED_)
