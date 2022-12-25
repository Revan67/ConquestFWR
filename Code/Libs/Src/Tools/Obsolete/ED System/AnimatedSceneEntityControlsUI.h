#if !defined(AFX_ANIMATEDSCENEENTITYCONTROLSUI_H__5B930763_51F7_11D2_823E_0000F4A24556__INCLUDED_)
#define AFX_ANIMATEDSCENEENTITYCONTROLSUI_H__5B930763_51F7_11D2_823E_0000F4A24556__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AnimatedSceneEntityControlsUI.h : header file
//
//---------------------------------------------------------------------------
#include "SceneEntityControls.h"
/////////////////////////////////////////////////////////////////////////////
// TAnimatedSceneEntityControlsUIForm dialog

class TAnimatedSceneEntityControlsUIForm : public TSceneEntityForm
{
// Construction
public:
	TAnimatedSceneEntityControlsUIForm(CWnd* pParent, const CallbackOnChange& callback, ROS::ASceneEntity* sceneEntity);   // standard constructor

	virtual void UpdateForm();
// Dialog Data
	//{{AFX_DATA(TAnimatedSceneEntityControlsUIForm)
	enum { IDD = IDD_DEFORMABLE_ENTITY_CUSTOM_DIALOG };
	CComboBox	mMotionNameComboBox;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(TAnimatedSceneEntityControlsUIForm)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(TAnimatedSceneEntityControlsUIForm)
	afx_msg void OnSelchangeMotionComboBox();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ANIMATEDSCENEENTITYCONTROLSUI_H__5B930763_51F7_11D2_823E_0000F4A24556__INCLUDED_)
