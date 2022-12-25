//////////////////////////////////////////////////////////////////////////////
//  File:    FilePropDlg.h
//
//  Purpose: A dialog box that displays file properties
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

#if !defined(AFX_FILEPROPDLG_H__A2E0A86B_03BA_4EE0_90DE_AC58DF13B813__INCLUDED_)
#define AFX_FILEPROPDLG_H__A2E0A86B_03BA_4EE0_90DE_AC58DF13B813__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FilePropDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFilePropDlg dialog

class CFilePropDlg : public CDialog
{
// Construction
public:
	CFilePropDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CFilePropDlg)
	enum { IDD = IDD_FILEPROP_DLG };
	CString	m_Filename;
	CString	m_FileStatus;
	CString	m_FilePath;
	CString	m_ModifiedDate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFilePropDlg)
	public:
	virtual BOOL Create(CWnd* pParentWnd);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFilePropDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	void ShowFileProperties(const char* filename);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILEPROPDLG_H__A2E0A86B_03BA_4EE0_90DE_AC58DF13B813__INCLUDED_)
