#if !defined(AFX_UIARTFILELISTVIEW_H__98D3CD41_5A1B_11D2_85B3_0000F4A24553__INCLUDED_)
#define AFX_UIARTFILELISTVIEW_H__98D3CD41_5A1B_11D2_85B3_0000F4A24553__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UIArtFileListView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUIArtFileListView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

#include "UIEditDoc.h"

class CUIArtFileListView : public CFormView
{
protected:
	CUIArtFileListView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CUIArtFileListView)

// Form Data
public:
	//{{AFX_DATA(CUIArtFileListView)
	enum { IDD = IDD_ART_FORM };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:
	CUIEditDoc* GetDocument();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUIArtFileListView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void syncToCurrentArt(CUIEditDoc* pDoc);
	virtual ~CUIArtFileListView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CUIArtFileListView)
	afx_msg void OnTestArtFile();
	afx_msg void OnItemchangedArtfileList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in UIEditView.cpp
inline CUIEditDoc* CUIArtFileListView::GetDocument()
   { return (CUIEditDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UIARTFILELISTVIEW_H__98D3CD41_5A1B_11D2_85B3_0000F4A24553__INCLUDED_)
