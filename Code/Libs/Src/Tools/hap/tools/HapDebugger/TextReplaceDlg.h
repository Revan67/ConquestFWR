//////////////////////////////////////////////////////////////////////////////
//  File:    TextReplaceDlg.h
//
//  Purpose: This is a dialog box that allows you to do a search and replace on 
//  an opened python file.  The actual searching is done by the scintilla control.
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

#if !defined(AFX_TEXTREPLACEDLG_H__DB8C56A8_3371_43D9_B598_130253CB7C74__INCLUDED_)
#define AFX_TEXTREPLACEDLG_H__DB8C56A8_3371_43D9_B598_130253CB7C74__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CTextReplaceDlg dialog

class CTextReplaceDlg : public CDialog
{
// Construction
public:
	CTextReplaceDlg(CWnd* pParent = NULL);   // standard constructor

	void EnableReplaceSelected(BOOL bEnable = TRUE);
	void SetReplaceSelected(BOOL bReplaceSel = TRUE);
// Dialog Data
	//{{AFX_DATA(CTextReplaceDlg)
	enum { IDD = IDD_TEXTREPLACE_DLG };
	CString	m_strReplaceWhat;
	CString	m_strReplaceWith;
	BOOL	m_bMatchCase;
	BOOL	m_bMatchStart;
	BOOL	m_bMatchWord;
	BOOL	m_bRegExpression;
	//}}AFX_DATA
	BOOL    m_bReplaceSel;
	BOOL    m_bEnableReplaceSel;
	HWND	m_hScintillaHwnd;

	int     m_nAbsEndPos;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTextReplaceDlg)
	public:
	virtual BOOL Create(CWnd* pParentWnd);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	bool FindNext(bool bNotifyOnFail = true);
	bool ReplaceNext();
	void ReplaceAll();
	void SetSearchFlags();
	void InsertFindText(const char* cszInsert, int nSelect=-1);
	void UpdateHistory();

	// Generated message map functions
	//{{AFX_MSG(CTextReplaceDlg)
	afx_msg void OnReplaceSelectedRdo();
	afx_msg void OnFindNextBtn();
	afx_msg void OnReplaceBtn();
	afx_msg void OnReplaceAllBtn();
	afx_msg void OnReplaceInWholeFileRdo();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL OnInitDialog();
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
	afx_msg void OnRegexpChk();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEXTREPLACEDLG_H__DB8C56A8_3371_43D9_B598_130253CB7C74__INCLUDED_)
