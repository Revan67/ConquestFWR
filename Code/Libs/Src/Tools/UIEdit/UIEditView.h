// UIEditView.h : interface of the CUIEditView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_UIEDITVIEW_H__9FFDA60C_4836_11D2_89DA_00400521015D__INCLUDED_)
#define AFX_UIEDITVIEW_H__9FFDA60C_4836_11D2_89DA_00400521015D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "UIEditTypes.h"

class CUIEditView : public CScrollView
{
protected: // create from serialization only
	CUIEditView();
	DECLARE_DYNCREATE(CUIEditView)

// Attributes
public:
	CUIEditDoc* GetDocument();

// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUIEditView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	UIEditMode  m_EditMode;

	virtual ~CUIEditView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void onEditRectDraw   (CDC* pDC);
	void onEditScreenDraw (CDC* pDC);
// Generated message map functions
protected:
	//{{AFX_MSG(CUIEditView)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnUpdateEditButtons(CCmdUI* pCmdUI);
	afx_msg void OnEditButtons();
	afx_msg void OnEditRects();
	afx_msg void OnUpdateEditRects(CCmdUI* pCmdUI);
	afx_msg void OnArtAddnew();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in UIEditView.cpp
inline CUIEditDoc* CUIEditView::GetDocument()
   { return (CUIEditDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UIEDITVIEW_H__9FFDA60C_4836_11D2_89DA_00400521015D__INCLUDED_)
