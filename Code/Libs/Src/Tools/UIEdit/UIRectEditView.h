#if !defined(AFX_UIRECTEDITVIEW_H__19B0E222_5F0A_11D2_89DA_00400521015D__INCLUDED_)
#define AFX_UIRECTEDITVIEW_H__19B0E222_5F0A_11D2_89DA_00400521015D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// UIRectEditView.h : header file
//

#include "UIData.h"
#include "UIEditDoc.h"

/////////////////////////////////////////////////////////////////////////////
// UIRectEditView view

class CUIRectEditView : public CScrollView
{
protected:
	CUIRectEditView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CUIRectEditView)

// Attributes
public:

// Operations
public:
	CUIEditDoc* GetDocument();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUIRectEditView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnInitialUpdate();     // first time after construct
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CUIRectEditView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CUIRectEditView)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in UIEditView.cpp
inline CUIEditDoc* CUIRectEditView::GetDocument()
   { return (CUIEditDoc*)m_pDocument; }
#endif
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UIRECTEDITVIEW_H__19B0E222_5F0A_11D2_89DA_00400521015D__INCLUDED_)
