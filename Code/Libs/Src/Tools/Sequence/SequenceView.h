// SequenceView.h : interface of the SequenceView class
//
/////////////////////////////////////////////////////////////////////////////
#include "sequencedoc.h"

#if !defined(AFX_SEQUENCEVIEW_H__3F629E0F_355B_11D3_BF44_00A0CC25FE00__INCLUDED_)
#define AFX_SEQUENCEVIEW_H__3F629E0F_355B_11D3_BF44_00A0CC25FE00__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000




class SequenceView : public CScrollView
{
protected: // create from serialization only
	SequenceView();
	DECLARE_DYNCREATE(SequenceView)

	int		ClickTarget, DropTarget, TransTo, TransFrom;
	BOOL	Dragging, DrawnOnce, PickingTransitionTarget;
	CPoint	DragPos;	//to keep rects synched (makes wire trail a bit)
	HWND	ActWnd;	//preview window

// Attributes
public:
	SequenceDoc	*GetDocument(void);
	void	DrawPreview(void);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SequenceView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	virtual BOOL OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll = TRUE);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~SequenceView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void	DrawCluster(CDC *pDC, SequenceDoc *doc, CRect *rect, int idx);

// Generated message map functions
protected:
	//{{AFX_MSG(SequenceView)
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in SequenceView.cpp
inline SequenceDoc* SequenceView::GetDocument()
   { return (SequenceDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SEQUENCEVIEW_H__3F629E0F_355B_11D3_BF44_00A0CC25FE00__INCLUDED_)
