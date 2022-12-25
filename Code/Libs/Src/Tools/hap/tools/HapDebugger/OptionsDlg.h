//////////////////////////////////////////////////////////////////////////////
//  File:    OptionsDlg.h
//
//  Purpose: The options dialog is a property sheet containing a number of 
//  property pages - each one of which is implemented in a seperate class.
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

#if !defined(AFX_OPTIONSDLG_H__DC3C31B8_3D1E_493D_BB67_955C7C371BA9__INCLUDED_)
#define AFX_OPTIONSDLG_H__DC3C31B8_3D1E_493D_BB67_955C7C371BA9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionsDlg.h : header file
//
#include "AppOptionsMainPg.h"
#include "RegistryOptionsPg.h"
#include "EditorOptsPg.h"
#include "DebugOptionsPage.h"

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg

class COptionsDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(COptionsDlg)

// Construction
public:
	COptionsDlg(CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COptionsDlg)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~COptionsDlg();

	// Generated message map functions
protected:

	CAppOptionsMainPg m_MainPage;
	CDebugOptionsPage m_DebugPage;
	CRegistryOptionsPg m_RegPage;
	CEditorOptsPg m_EditPage;


	//{{AFX_MSG(COptionsDlg)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONSDLG_H__DC3C31B8_3D1E_493D_BB67_955C7C371BA9__INCLUDED_)
