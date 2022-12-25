//////////////////////////////////////////////////////////////////////////////
//  File:    AppOptionsMainPg.h
//
//  Purpose: This file defines the app options page of the options tabbed
//  dialog box.
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

#if !defined(AFX_APPOPTIONSMAINPG_H__FF9B7D59_6C1B_40CA_B331_3E32C3771015__INCLUDED_)
#define AFX_APPOPTIONSMAINPG_H__FF9B7D59_6C1B_40CA_B331_3E32C3771015__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AppOptionsMainPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAppOptionsMainPg dialog

class CAppOptionsMainPg : public CPropertyPage
{
	DECLARE_DYNCREATE(CAppOptionsMainPg)

// Construction
public:
	CAppOptionsMainPg();
	~CAppOptionsMainPg();

// Dialog Data
	//{{AFX_DATA(CAppOptionsMainPg)
	enum { IDD = IDD_APPOPTIONS_MAIN_PG };
	BOOL	m_bAutoOpenProject;
	CString	m_PythonHelpFile;
	BOOL	m_bOpenXmlExtern;
	BOOL	m_bAutowatchPopup;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAppOptionsMainPg)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAppOptionsMainPg)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelpBrowseBtn();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_APPOPTIONSMAINPG_H__FF9B7D59_6C1B_40CA_B331_3E32C3771015__INCLUDED_)
