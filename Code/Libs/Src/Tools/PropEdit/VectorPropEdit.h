#if !defined(AFX_VECTORPROPEDIT_H__A8C7D861_453C_11D3_A92B_00104B07D00B__INCLUDED_)
#define AFX_VECTORPROPEDIT_H__A8C7D861_453C_11D3_A92B_00104B07D00B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VectorPropEdit.h : header file
//

#include "property.h"

/////////////////////////////////////////////////////////////////////////////
// CVectorPropEdit dialog

class CVectorPropEdit : public CDialog
{
// Construction
public:
	CVectorPropEdit(const Property &_p, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CVectorPropEdit)
	enum { IDD = IDD_VECTOREDIT };
	CString	m_PropertyName;
	CString	m_TypeName;
	float	m_VectorX;
	float	m_VectorY;
	float	m_VectorZ;
	//}}AFX_DATA

	// Local editable property
	Property p;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVectorPropEdit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void sync_property ();

	// Generated message map functions
	//{{AFX_MSG(CVectorPropEdit)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VECTORPROPEDIT_H__A8C7D861_453C_11D3_A92B_00104B07D00B__INCLUDED_)
