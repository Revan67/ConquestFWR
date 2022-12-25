#if !defined(AFX_KEYPROPERTIESUI_H__7A23CFC7_4C8E_11D2_823E_0000F4A24556__INCLUDED_)
#define AFX_KEYPROPERTIESUI_H__7A23CFC7_4C8E_11D2_823E_0000F4A24556__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// KeyPropertiesUI.h : header file
//
#include "TimeType.h"
/////////////////////////////////////////////////////////////////////////////
// TKeyPropertiesUIForm dialog

class TKeyPropertiesUIForm : public CDialog
{
	// Construction
	public:
		TKeyPropertiesUIForm(CWnd* pParent = NULL);   // standard constructor

		void SetTime(ROS::Time time);
		ROS::Time GetTime() const;

	// Dialog Data
		//{{AFX_DATA(TKeyPropertiesUIForm)
		enum { IDD = IDD_KEY_PROPERTIES_DIALOG };
		CEdit	mTimeEdit;
	//}}AFX_DATA


	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(TKeyPropertiesUIForm)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	protected:
		TKeyPropertiesUIForm(UINT nIDTemplate, CWnd* pParent = NULL);   // standard constructor

		// Generated message map functions
		//{{AFX_MSG(TKeyPropertiesUIForm)
		virtual void OnOK();
		virtual BOOL OnInitDialog();
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	private:
		ROS::Time	mTime;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_KEYPROPERTIESUI_H__7A23CFC7_4C8E_11D2_823E_0000F4A24556__INCLUDED_)
