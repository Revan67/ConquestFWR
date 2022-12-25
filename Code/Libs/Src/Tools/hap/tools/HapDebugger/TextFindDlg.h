//////////////////////////////////////////////////////////////////////////////
//  File:    TextFindDlg.h
//
//  Purpose: This is a dialog box which allows you to define parameters for
//  searching through an open python file - the actual searching is done by
//  the scintilla control - this dialog simply allows you to define what and 
//  how.
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

#if !defined(AFX_TEXTFINDDLG_H__9013D1C4_85C7_4EBD_B0C2_DBF4A8D55C94__INCLUDED_)
#define AFX_TEXTFINDDLG_H__9013D1C4_85C7_4EBD_B0C2_DBF4A8D55C94__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CTextFindDlg dialog


class CTextFindDlg : public CDialog
{
// Construction
public:
	CTextFindDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTextFindDlg)
	enum { IDD = IDD_TEXTFIND_DLG };
	CString	m_FindText;
	BOOL	m_MatchCase;
	BOOL	m_MatchStart;
	BOOL	m_MatchWord;
	BOOL	m_UseRegExpr;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTextFindDlg)
	public:
	virtual BOOL Create(CWnd* pParentWnd);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:



	void InsertFindText(const char* cszInsert, int nSelect=-1);
	// Generated message map functions
	//{{AFX_MSG(CTextFindDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnRegexBtn();
	afx_msg void OnFindpopAnychar();
	afx_msg void OnFindpopCharinset();
	afx_msg void OnFindpopCharnotinset();
	afx_msg void OnFindpopEndofline();
	afx_msg void OnFindpopEndword();
	afx_msg void OnFindpopOnemore();
	afx_msg void OnFindpopStartofline();
	afx_msg void OnFindpopStartword();
	afx_msg void OnFindpopTabchar();
	afx_msg void OnFindpopTagpart();
	afx_msg void OnFindpopZeromore();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEXTFINDDLG_H__9013D1C4_85C7_4EBD_B0C2_DBF4A8D55C94__INCLUDED_)
