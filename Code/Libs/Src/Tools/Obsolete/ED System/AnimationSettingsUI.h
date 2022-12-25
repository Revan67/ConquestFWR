#if !defined(AFX_ANIMATIONSETTINGSUI_H__8291D683_518D_11D2_968C_9C90F80CD7F5__INCLUDED_)
#define AFX_ANIMATIONSETTINGSUI_H__8291D683_518D_11D2_968C_9C90F80CD7F5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AnimationSettingsUI.h : header file
//
#include "TimeType.h"
/////////////////////////////////////////////////////////////////////////////
// TAnimationSettingsUIForm dialog

class TAnimationSettingsUIForm : public CDialog
{
	// Construction
	public:
		TAnimationSettingsUIForm(CWnd* pParent = NULL);   // standard constructor

		void SetDuration(ROS::Time time);
		ROS::Time GetDuration() const;

	// Dialog Data
		//{{AFX_DATA(TAnimationSettingsUIForm)
		enum { IDD = IDD_ANIMATION_SETTINGS_DIALOG };
		CEdit	mTimeEdit;
		//}}AFX_DATA


	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(TAnimationSettingsUIForm)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	protected:

		// Generated message map functions
		//{{AFX_MSG(TAnimationSettingsUIForm)
		virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	private:
		ROS::Time	mTime;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ANIMATIONSETTINGSUI_H__8291D683_518D_11D2_968C_9C90F80CD7F5__INCLUDED_)
