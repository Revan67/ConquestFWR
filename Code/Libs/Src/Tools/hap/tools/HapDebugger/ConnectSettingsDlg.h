//////////////////////////////////////////////////////////////////////////////
//  File:    ConnectSettingsDlg.h
//
//  Purpose: A dialog box that allows the user to specify connection settings
//  for the connection to the remote debug engine.
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

#if !defined(AFX_CONNECTSETTINGSDLG_H__E9882943_A833_42B5_A2F0_7C4EA7FE3095__INCLUDED_)
#define AFX_CONNECTSETTINGSDLG_H__E9882943_A833_42B5_A2F0_7C4EA7FE3095__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConnectSettingsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConnectSettingsDlg dialog
#define CONNECT_REMOTE	1
#define CONNECT_LOCAL   0


class CConnectSettingsDlg : public CDialog
{
// Construction
public:
	CConnectSettingsDlg(CWnd* pParent = NULL);   // standard constructor



	const vector<string>* m_pConnectNames;
	int m_nConnectType;

// Dialog Data
	//{{AFX_DATA(CConnectSettingsDlg)
	enum { IDD = IDD_CONNECTOPTS_DLG };
	CString	m_RemoteName;
	CString	m_DebugScript;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConnectSettingsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConnectSettingsDlg)
	afx_msg void OnLocalRdo();
	afx_msg void OnRemoteRdo();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnSetdbgscriptBtn();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONNECTSETTINGSDLG_H__E9882943_A833_42B5_A2F0_7C4EA7FE3095__INCLUDED_)
