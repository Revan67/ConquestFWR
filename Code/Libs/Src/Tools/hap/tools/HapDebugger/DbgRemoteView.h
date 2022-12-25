//////////////////////////////////////////////////////////////////////////////
//  File:    DbgRemoteView.h
//
//  Purpose: interface of the CDbgRemoteView class which provides a single
//  view of a single python source file
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

#if !defined(AFX_DBGREMOTEVIEW_H__56886CD2_DEA7_4DE9_AA3E_AE1CD3A9CFDC__INCLUDED_)
#define AFX_DBGREMOTEVIEW_H__56886CD2_DEA7_4DE9_AA3E_AE1CD3A9CFDC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "scintilla\scintilla.h"


//scintilla control typedefs
typedef unsigned long uptr_t;
typedef long sptr_t;
//scintilla control function typedef
typedef sptr_t (*SciFnDirect)(sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam);

class CDbgRemoteDoc;


class CDbgRemoteView : public CView
{
protected: // create from serialization only
	CDbgRemoteView();
	DECLARE_DYNCREATE(CDbgRemoteView)

public:
	virtual ~CDbgRemoteView();


#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


public:
	CDbgRemoteDoc* GetDocument();

	void SetCurrentLine(int nLineNumber);
	int  GetCurrentLine()const;
	int  GetCurrentCol()const;
	void ShowLine(int nLineNumber);
	void SetBreakpoint(int nLineNumber, BOOL bSet = TRUE);
	void ClearBreakpoints();

	void InitializeEditor();
	void GetEditorText(char_vector& editorData);

	void DisplayAutoWatch(const char* cszWatchStr);

protected:
	void SetAStyle(int style, COLORREF fore, COLORREF back = RGB(255,255,255), int size=-1, const char *face=0);
	void DefineMarker(int marker, int markerType, COLORREF fore, COLORREF back);
	LRESULT SendEditor(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0) const;
	

	void AutoIndent();
	void AutoHint();
	void AutoComplete();
	void StartAutoWatch();
	void EndAutoWatch();

	void FoldLine(int nLine, int Modifiers = 0);
	void Expand(int &line, bool doExpand, bool force = false,
	            int visLevels = 0, int level = -1);
	void FoldAll(bool bFold = false);
	void MatchBraces();
	void GetCurrentWordPos(int& CurStart, int& CurEnd);
	string GetCurrentText();
	void SelectLexer(const CString& filename);
	void OnAttemptModify();

protected:

	HWND		m_hwndScintilla;
	SciFnDirect m_fnEditor;
	long		m_ptrEditor;
	
	BOOL m_bShowLinenumbers;
	int  m_nLineNumberMarginWidth;
	int  m_nMarkerMarginWidth;
	BOOL m_bShowSelectiveDisplay;

	int  m_nAutoWatchPos;
	std::string m_autoWatchExpression;
	std::string m_docType;

	struct SFindParams
	{
		int nFlags;
		TextToFind ttf;
		CString FindStr;
	};
	SFindParams m_FindParams;

public:
	//{{AFX_DATA(CDbgRemoteView)
	enum{ IDD = IDD_DBGREMOTE_FORM };
	//}}AFX_DATA
	
	//{{AFX_VIRTUAL(CDbgRemoteView)
	public:
	
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrepareDC( CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnDraw(CDC* pDC);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(CDbgRemoteView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBookmarksToggle();
	afx_msg void OnBookmarksGotonext();
	afx_msg void OnUpdateBookmarksGotonext(CCmdUI* pCmdUI);
	afx_msg void OnBookmarksGotoprevious();
	afx_msg void OnUpdateBookmarksGotoprevious(CCmdUI* pCmdUI);
	afx_msg void OnBookmarksClearall();
	afx_msg void OnUpdateBookmarksClearall(CCmdUI* pCmdUI);
	afx_msg void OnDebugBreakpointsToggle();
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnEditCut();
	afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditFind();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnViewWhitespace();
	afx_msg void OnUpdateViewWhitespace(CCmdUI* pCmdUI);
	afx_msg void OnViewSelectivedisplay();
	afx_msg void OnUpdateViewSelectivedisplay(CCmdUI* pCmdUI);
	afx_msg void OnViewLinenumbers();
	afx_msg void OnUpdateViewLinenumbers(CCmdUI* pCmdUI);
	afx_msg void OnEditFindnext();
	afx_msg void OnEditGotoline();
	afx_msg void OnViewViewlineendings();
	afx_msg void OnUpdateViewViewlineendings(CCmdUI* pCmdUI);
	afx_msg void OnDebugAddtoWatch();
	afx_msg void OnSpacesToTabs();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnEditReplace();
	afx_msg void OnViewEOLTypeCRLF();
	afx_msg void OnUpdateViewEOLTypeCRLF(CCmdUI* pCmdUI);
	afx_msg void OnViewEOLTypeCR();
	afx_msg void OnUpdateViewEOLTypeCR(CCmdUI* pCmdUI);
	afx_msg void OnViewEOLTypeLF();
	afx_msg void OnUpdateViewEOLTypeLF(CCmdUI* pCmdUI);
	afx_msg void OnEditUncommentline();
	afx_msg void OnUpdateEditUncommentline(CCmdUI* pCmdUI);
	afx_msg void OnEditCommentline();
	afx_msg void OnUpdateEditCommentline(CCmdUI* pCmdUI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
	afx_msg void OnViewExpandAll();
	afx_msg void OnUpdateViewExpandAll(CCmdUI* pCmdUI);
	afx_msg void OnViewCollapseAll();
	afx_msg void OnUpdateViewCollapseAll(CCmdUI* pCmdUI);
	afx_msg void OnEditFindinFiles();
	afx_msg void OnTabsToSpaces();
	afx_msg void OnIndent();
	afx_msg void OnUnindent();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEditGototagdefinition();
	afx_msg void OnUpdateEditGototagdefinition(CCmdUI *pCmdUI);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnFileProperties();
	afx_msg void OnViewSplitwindow();
	afx_msg void OnUpdateViewSplitwindow(CCmdUI *pCmdUI);
};

#ifndef _DEBUG  // debug version in DbgRemoteView.cpp
inline CDbgRemoteDoc* CDbgRemoteView::GetDocument()
   { return (CDbgRemoteDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DBGREMOTEVIEW_H__56886CD2_DEA7_4DE9_AA3E_AE1CD3A9CFDC__INCLUDED_)
