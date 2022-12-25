#if !defined(AFX_SIMPLEPROPEDIT_H__33467A70_44E6_11D3_85B6_0000F4A24553__INCLUDED_)
#define AFX_SIMPLEPROPEDIT_H__33467A70_44E6_11D3_85B6_0000F4A24553__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SimplePropEdit.h : header file
//

#include "property.h"

/////////////////////////////////////////////////////////////////////////////
// CSimplePropEdit dialog

class CSimplePropEdit : public CDialog
{
// Construction
public:
	CSimplePropEdit(const Property &_p, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSimplePropEdit)
	enum { IDD = IDD_SIMPLEPROPEDIT };
	CString	m_PropertyName;
	CString	m_TypeName;
	CString	m_PropertyValue;
	//}}AFX_DATA

	// Local editable property
	Property p;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSimplePropEdit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void sync_property ();

	// Generated message map functions
	//{{AFX_MSG(CSimplePropEdit)
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIMPLEPROPEDIT_H__33467A70_44E6_11D3_85B6_0000F4A24553__INCLUDED_)
