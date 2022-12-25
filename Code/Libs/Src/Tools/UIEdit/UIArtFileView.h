#if !defined(AFX_UIARTFILEVIEW_H__D3B5CEE2_58B8_11D2_89DA_00400521015D__INCLUDED_)
#define AFX_UIARTFILEVIEW_H__D3B5CEE2_58B8_11D2_89DA_00400521015D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// UIArtFileView.h : header file
//

#include "UIData.h"
#include "UIEditDoc.h"

/////////////////////////////////////////////////////////////////////////////
// CUIArtFileView view

class CUIArtFileView : public CScrollView
{
protected:
	CUIArtFileView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CUIArtFileView)

// Attributes
public:

// Operations
public:
	CUIEditDoc* GetDocument();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUIArtFileView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnInitialUpdate();     // first time after construct
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	UIHandle m_CurrentArtHandle;
	virtual ~CUIArtFileView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CUIArtFileView)
	afx_msg void OnTestArtFile();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
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

#endif // !defined(AFX_UIARTFILEVIEW_H__D3B5CEE2_58B8_11D2_89DA_00400521015D__INCLUDED_)
