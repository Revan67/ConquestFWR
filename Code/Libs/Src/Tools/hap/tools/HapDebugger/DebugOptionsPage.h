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
#if !defined(AFX_DEBUGOPTIONSPAGE_H__9AFC6807_ECE1_40B5_BFA7_7E658988C278__INCLUDED_)
#define AFX_DEBUGOPTIONSPAGE_H__9AFC6807_ECE1_40B5_BFA7_7E658988C278__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DebugOptionsPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDebugOptionsPage dialog

class CDebugOptionsPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CDebugOptionsPage)

// Construction
public:
	CDebugOptionsPage();
	~CDebugOptionsPage();

// Dialog Data
	//{{AFX_DATA(CDebugOptionsPage)
	enum { IDD = IDD_DEBUGOPTIONS_PG };
	BOOL	m_EnableAdvancedDebug;
	BOOL	m_BreakOnFirstLine;
	//}}AFX_DATA
	CUserCfg::EExceptionMode m_eExceptionMode;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDebugOptionsPage)
	public:
	virtual BOOL OnApply();
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDebugOptionsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnBrakUnhandledRdo();
	afx_msg void OnBreakAlwaysRdo();
	afx_msg void OnIgnoreExcepRdo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	BOOL m_bClearOutput;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEBUGOPTIONSPAGE_H__9AFC6807_ECE1_40B5_BFA7_7E658988C278__INCLUDED_)
