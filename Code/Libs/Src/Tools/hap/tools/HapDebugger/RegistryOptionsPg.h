//////////////////////////////////////////////////////////////////////////////
//  File:    RegistryOptionsPg.h
//
//  Purpose: This is a page of the settings dialog - it allows you to set a 
//  number of registry related options (file associations etc)
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

#if !defined(AFX_REGISTRYOPTIONSPG_H__47459AE6_43C2_4100_9333_39C9EAF37B5A__INCLUDED_)
#define AFX_REGISTRYOPTIONSPG_H__47459AE6_43C2_4100_9333_39C9EAF37B5A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CRegistryOptionsPg dialog

class CRegistryOptionsPg : public CPropertyPage
{
	DECLARE_DYNCREATE(CRegistryOptionsPg)

// Construction
public:
	CRegistryOptionsPg();
	~CRegistryOptionsPg();

// Dialog Data
	//{{AFX_DATA(CRegistryOptionsPg)
	enum { IDD = IDD_REGISTRYOPTS_PG };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRegistryOptionsPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	bool AssurePythonEntries();
	bool AssureDbgEntries();
	bool OpenWithSet();
	bool EditorSet();

	// Generated message map functions
	//{{AFX_MSG(CRegistryOptionsPg)
	afx_msg void OnOpenWithBtn();
	afx_msg void OnSetEditorBtn();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REGISTRYOPTIONSPG_H__47459AE6_43C2_4100_9333_39C9EAF37B5A__INCLUDED_)
