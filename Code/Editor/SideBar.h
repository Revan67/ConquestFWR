#if !defined(__SIDEBAR_H__)
#define __SIDEBAR_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "sizecbar.h"
#include "scbarg.h"

/////////////////////////////////////////////////////////////////////////////
// CSidebar window

class CSidebar : public CSizingControlBarG
{
// Construction
public:
	CSidebar();

// Attributes
public:
	CString name;

// Operations
public:
	virtual bool Reset(){ return true; }
	virtual bool Update(){ return true; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSidebar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSidebar();

public:
	virtual DoPaint( CPaintDC& ) = 0;

	// Generated message map functions
protected:
	//{{AFX_MSG(CSidebar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif