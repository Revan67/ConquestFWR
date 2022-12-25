#if !defined(AFX_CAMERACUSTOMCONTROLSUI_H__5B13F041_B437_11D2_968C_0040333267EF__INCLUDED_)
#define AFX_CAMERACUSTOMCONTROLSUI_H__5B13F041_B437_11D2_968C_0040333267EF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CameraCustomControlsUI.h : header file
//
//---------------------------------------------------------------------------
#include "SceneEntityControls.h"
/////////////////////////////////////////////////////////////////////////////
// CameraCustomControlsUI dialog

class CameraCustomControlsUI: public TSceneEntityForm
{
	// Construction
	public:
		CameraCustomControlsUI(CWnd* pParent, const CallbackOnChange& callback, ROS::ASceneEntity* sceneEntity);

		virtual void UpdateForm();

	// Dialog Data
		//{{AFX_DATA(CameraCustomControlsUI)
		enum { IDD = IDD_CAMERA_CUSTOM_DIALOG };
		CSpinButtonCtrl	mHorizontalFOVSpin;
		CSpinButtonCtrl	mVerticalFOVSpin;
		CEdit	mHorizontalFOVEdit;
		CEdit	mVerticalFOVEdit;
		//}}AFX_DATA


	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CameraCustomControlsUI)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	protected:

		// Generated message map functions
		//{{AFX_MSG(CameraCustomControlsUI)
			afx_msg void OnChangeHorizontalFOVEdit();
			afx_msg void OnChangeVerticalFOVEdit();
			afx_msg void OnDeltaposHorizontalFOVSpin(NMHDR* pNMHDR, LRESULT* pResult);
			afx_msg void OnDeltaposVerticalFOVSpin(NMHDR* pNMHDR, LRESULT* pResult);
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CAMERACUSTOMCONTROLSUI_H__5B13F041_B437_11D2_968C_0040333267EF__INCLUDED_)
