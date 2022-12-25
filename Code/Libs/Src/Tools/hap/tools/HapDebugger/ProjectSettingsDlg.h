//////////////////////////////////////////////////////////////////////////////
//  File:    ProjectSettingsDlg.h
//
//  Purpose: A dialog box that allows you to change the name (and in the future)
//  other properties of a project.
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

#if !defined(AFX_PROJECTSETTINGSDLG_H__A6787A36_E4EC_4D2B_AA86_3B00DDDEE58E__INCLUDED_)
#define AFX_PROJECTSETTINGSDLG_H__A6787A36_E4EC_4D2B_AA86_3B00DDDEE58E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProjectSettingsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProjectSettingsDlg dialog

class CProjectSettingsDlg : public CDialog
{
// Construction
public:
	CProjectSettingsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CProjectSettingsDlg)
	enum { IDD = IDD_PROJECTSETTINGS_DLG };
	CString	m_ProjectName;
	CString	m_RunParams;
	CString	m_RemotePath;
	CString	m_WorkingDir;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProjectSettingsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CProjectSettingsDlg)
	afx_msg void OnBrowseDirBtn();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	void PASCAL DDV_WorkingDir(CDataExchange* pDX, CString workingDir);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROJECTSETTINGSDLG_H__A6787A36_E4EC_4D2B_AA86_3B00DDDEE58E__INCLUDED_)
