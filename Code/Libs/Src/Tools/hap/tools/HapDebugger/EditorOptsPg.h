//////////////////////////////////////////////////////////////////////////////
//  File:    EditorOptsPg.h
//
//  Purpose: This defines a single page of the options dialog - this page 
//  provides the interface for setting all editor options.
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

#if !defined(AFX_EDITOROPTSPG_H__31E2894E_0A85_4F1C_9129_13E34E07350A__INCLUDED_)
#define AFX_EDITOROPTSPG_H__31E2894E_0A85_4F1C_9129_13E34E07350A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CEditorOptsPg property page class

class CEditorOptsPg : public CPropertyPage
{
	DECLARE_DYNCREATE(CEditorOptsPg)

// Construction
public:
	enum EStyleTypes
	{
		kForecol,
		kBackcol,
		kFont,
		kStyleTypesCount
	};

	CEditorOptsPg();
	~CEditorOptsPg();

// Dialog Data
	//{{AFX_DATA(CEditorOptsPg)
	enum { IDD = IDD_EDITOROPTS_PG };
	UINT	m_TabWidth;
	BOOL	m_bFoldOnOpen;
	BOOL	m_bUseTabs;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CEditorOptsPg)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:

	void UpdateFontDisplay();
	void ShowCurrentStyle();
	void SaveCurrentStyle();
	void SaveDefaultStyle(EStyleTypes changedStyle, const char* description);
	void UpdateViews();
	void UpdateStyleList(const char* cszLanguageName);
	void UpdateLanguageList();
	
// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CEditorOptsPg)
	afx_msg void OnForecolBtn();
	afx_msg void OnFontSelectBtn();
	afx_msg void OnBackcolBtn();
	virtual BOOL OnInitDialog();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnSelchangeStyleLst();
	afx_msg void OnRestoreCurBtn();
	afx_msg void OnRestoreAllBtn();
	afx_msg void OnCaretColorBtn();
	afx_msg void OnChangeTabwidthEdt();
	afx_msg void OnSelchangeCurlangCbo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


	CBrush m_CurForeBrush;
	CBrush m_CurBackBrush;
	CFont  m_CurFont;
	COLORREF m_CaretColor;

	CUserCfg::SEditorStyle m_CurrentStyle;

public:

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITOROPTSPG_H__31E2894E_0A85_4F1C_9129_13E34E07350A__INCLUDED_)
