//////////////////////////////////////////////////////////////////////////////
//  File:    GroupPropertiesDlg.h
//
//  Purpose: A dialog box that allows you to change the name of a file group
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

#if !defined(AFX_GROUPPROPERTIESDLG_H__75F87C8C_7792_45BB_9AA8_806DADD5994B__INCLUDED_)
#define AFX_GROUPPROPERTIESDLG_H__75F87C8C_7792_45BB_9AA8_806DADD5994B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GroupPropertiesDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGroupPropertiesDlg dialog

class CGroupPropertiesDlg : public CDialog
{
// Construction
public:
	CGroupPropertiesDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGroupPropertiesDlg)
	enum { IDD = IDD_GRPPROP_DLG };
	CString	m_GroupName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroupPropertiesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGroupPropertiesDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GROUPPROPERTIESDLG_H__75F87C8C_7792_45BB_9AA8_806DADD5994B__INCLUDED_)
