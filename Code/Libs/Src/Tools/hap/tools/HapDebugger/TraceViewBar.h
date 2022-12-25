//////////////////////////////////////////////////////////////////////////////
//  File:    TraceViewBar.h
//
//  Purpose: This is a dockable window which displays debug messages from
//  the python program (stderr).
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

#if !defined(AFX_TRACEVIEWBAR_H__300697D7_5059_4A09_9929_5D58D9A33648__INCLUDED_)
#define AFX_TRACEVIEWBAR_H__300697D7_5059_4A09_9929_5D58D9A33648__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TraceViewBar.h : header file
//
#include "sizecbar.h"
#include "scbarg.h"
#include "DbgRemoteView.h"
#include "scintilla\scintilla.h"

/////////////////////////////////////////////////////////////////////////////
// CTraceViewBar window

class CTraceViewBar : public CSizingControlBarG
{
// Construction
public:
	CTraceViewBar();
	virtual ~CTraceViewBar();
	
	void AddDebugMessage(const char* szMessage);
	void AddOutputMessage(const char* szMessage);
	void SetReadOnly(bool bRead = true);
	void DisplayAutoComplete(const char* cszMessage);
	void UpdateStyles();
	void Clear();
// Attributes
protected:
	static void AddMessage(const char* szMessage, vector<char>& vecMsg);
	void UpdateBuffer(vector<char>& vecMsg);
	
	LRESULT SendEditor(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0) const;
	void InitializeScintilla();
	void SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face);
	void OnEditorDoubleClick();
	void AutoComplete();
// Operations

public:
	//COutputEdit m_Edit;
	//HBRUSH m_hBkBrushErr;
	//HBRUSH m_hBkBrushOut;
	//HBRUSH m_hBkBrushMrg;


	CFont m_Font;
	int m_MaxLineCount;

	CTabCtrl m_Tab;
	
	vector<char> m_vecErrBuffer;
	vector<char> m_vecOutBuffer;
	vector<char> m_vecMrgBuffer;


	HWND		m_hwndScintilla;
	SciFnDirect m_fnEditor;
	long		m_ptrEditor;

	
	struct SFindParams
	{
		int nFlags;
		TextToFind ttf;
		CString FindStr;
	};
	SFindParams  m_FindParams;


public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTraceViewBar)
	protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CTraceViewBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	afx_msg LRESULT OnLineEntered(WPARAM w, LPARAM l);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnEditCut();
	afx_msg void OnEditPaste();
	afx_msg void OnEditCopy();
	afx_msg void OnEditUndo();
	afx_msg void OnTracvwpopClearall();	
public:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnEditFind();
	afx_msg void OnEditFindnext();
	afx_msg void OnTracvwpopClearrun();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRACEVIEWBAR_H__300697D7_5059_4A09_9929_5D58D9A33648__INCLUDED_)
