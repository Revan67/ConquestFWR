// PropEditView.h : interface of the CPropEditView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROPEDITVIEW_H__F08D530E_4127_11D3_85B6_0000F4A24553__INCLUDED_)
#define AFX_PROPEDITVIEW_H__F08D530E_4127_11D3_85B6_0000F4A24553__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CPropEditView : public CListView
{
protected: // create from serialization only
	CPropEditView();
	DECLARE_DYNCREATE(CPropEditView)

// Attributes
public:
	CPropEditDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropEditView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	void set_property (int index, Property &p);
	void add_property (Property &p);
	void del_property (int which);
	void edit_item (int nItem);

	CMenu mContextMenu;
	int mViewContextPos;
	int mItemContextPos;
	virtual ~CPropEditView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CPropEditView)
	afx_msg void OnEditInsertProperty();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnViewcontextNewLong();
	afx_msg void OnViewcontextNewDouble();
	afx_msg void OnViewcontextNewMatrix();
	afx_msg void OnViewcontextNewSingle();
	afx_msg void OnViewcontextNewString();
	afx_msg void OnViewcontextNewTransform();
	afx_msg void OnViewcontextNewUlong();
	afx_msg void OnViewcontextNewVector();
	afx_msg void OnItemcontextEdit();
	afx_msg void OnItemcontextDelete();
	afx_msg void OnItemdblclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in PropEditView.cpp
inline CPropEditDoc* CPropEditView::GetDocument()
   { return (CPropEditDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPEDITVIEW_H__F08D530E_4127_11D3_85B6_0000F4A24553__INCLUDED_)
