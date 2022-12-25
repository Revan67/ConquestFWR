#if !defined(AFX_SELECTENTITYTYPEDIALOG_H__A61451E3_6052_11D2_968C_A29C872EB874__INCLUDED_)
#define AFX_SELECTENTITYTYPEDIALOG_H__A61451E3_6052_11D2_968C_A29C872EB874__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SelectEntityTypeDialog.h : header file
//
#include "resource.h"
/////////////////////////////////////////////////////////////////////////////
// CSelectEntityDialog dialog

class CSelectEntityTypeDialog: public CDialog
{
	// Construction
	public:

		enum EntityType
		{	kDeformable,
			kCompound
		};

		CSelectEntityTypeDialog(CWnd* pParent = NULL);   // standard constructor

		EntityType GetSelection() const;

	// Dialog Data
		//{{AFX_DATA(CSelectEntityDialog)
		enum { IDD = IDD_SELECT_ENTITY_TYPE_DIALOG };
		CButton	mDeformableEntityRadio;
		CButton	mCompoundEntityRadio;
		//}}AFX_DATA


	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CSelectEntityDialog)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	protected:

		// Generated message map functions
		//{{AFX_MSG(CSelectEntityDialog)
		virtual BOOL OnInitDialog();
	afx_msg void OnCompoundEntityRadio();
	afx_msg void OnDeformableEntityRadio();
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	
	private:
		EntityType	mEntityType;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTENTITYTYPEDIALOG_H__A61451E3_6052_11D2_968C_A29C872EB874__INCLUDED_)
