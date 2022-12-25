#if !defined(AFX_SELECTENTITYPART_H__41CC7441_0496_11D3_BF43_00A0CC25FE00__INCLUDED_)
#define AFX_SELECTENTITYPART_H__41CC7441_0496_11D3_BF43_00A0CC25FE00__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SelectEntityPart.h : header file
//
#include "EntityTree.h"

/////////////////////////////////////////////////////////////////////////////
// CSelectEntityPart dialog

class CSelectEntityPart : public CDialog
{
// Construction
public:
	CSelectEntityPart(ROS::ASceneEntity &entity, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectEntityPart)
	enum { IDD = IDD_SELECT_ENTITY_PART };
	CEntityTree	mPartTree;
	//}}AFX_DATA

	CEntityTree::Part selectedPart;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectEntityPart)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSelectEntityPart)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	ROS::ASceneEntity &bound_entity;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTENTITYPART_H__41CC7441_0496_11D3_BF43_00A0CC25FE00__INCLUDED_)
