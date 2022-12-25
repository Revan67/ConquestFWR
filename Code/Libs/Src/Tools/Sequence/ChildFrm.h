// ChildFrm.h : interface of the ChildFrame class
//
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_CHILDFRM_H__3F629E0B_355B_11D3_BF44_00A0CC25FE00__INCLUDED_)
#define AFX_CHILDFRM_H__3F629E0B_355B_11D3_BF44_00A0CC25FE00__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class ChildFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(ChildFrame)
public:
	ChildFrame();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ChildFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~ChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(ChildFrame)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDFRM_H__3F629E0B_355B_11D3_BF44_00A0CC25FE00__INCLUDED_)
