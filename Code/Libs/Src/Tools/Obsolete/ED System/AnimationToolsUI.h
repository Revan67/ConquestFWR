// Author: Shaival Varma
// --------------------------------------------------------------------------
#if !defined(AFX_ANIMATIONTOOLSUI_H__15E9CF03_48C4_11D2_823E_0000F4A24556__INCLUDED_)
#define AFX_ANIMATIONTOOLSUI_H__15E9CF03_48C4_11D2_823E_0000F4A24556__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AnimationToolsUI.h : header file
//
#include "AnimationToolsController.h"
#include "Links.h"
#include "IdleNotification.h"

class TTrackViewUIForm;
/////////////////////////////////////////////////////////////////////////////
// TAnimationToolsUIForm dialog

class TAnimationToolsUIForm : public CDialog
{
// Construction
public:
	TAnimationToolsUIForm(CWnd* pParent, ROS::SceneModel& sceneModel);   // standard constructor
	~TAnimationToolsUIForm();

// Dialog Data
	//{{AFX_DATA(TAnimationToolsUIForm)
	enum { IDD = IDD_ANIMATION_TOOLS_DIALOG };
	CButton	mPlaySpeedButton;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(TAnimationToolsUIForm)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void OnOK();
	virtual void OnCancel();

	// Generated message map functions
	//{{AFX_MSG(TAnimationToolsUIForm)
	afx_msg void SettingsSpeedButtonClick();
	afx_msg void PlaySpeedButtonClick();
	virtual BOOL OnInitDialog();
	afx_msg void OnMove(int x, int y);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void UpdateGUI(int updateID);

	static void IdleNotify();
	void IdleNotificationFunc();

	/**#: [Cardinalities = "1..1/"]*/
	IdleNotification						mIdleNotification;
	AggAPointer<AnimationToolsController>	mAnimationToolsControllerSP;
	bool                                    mDialogInitialized;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ANIMATIONTOOLSUI_H__15E9CF03_48C4_11D2_823E_0000F4A24556__INCLUDED_)
