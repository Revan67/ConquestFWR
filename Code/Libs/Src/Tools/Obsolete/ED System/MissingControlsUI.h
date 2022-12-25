#if !defined(AFX_MISSINGCONTROLSUI_H__371B5383_4688_11D2_968C_804B01767957__INCLUDED_)
#define AFX_MISSINGCONTROLSUI_H__371B5383_4688_11D2_968C_804B01767957__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TMissingControlsUIForm.h : header file
//
//---------------------------------------------------------------------------
#include "SceneEntityControls.h"
/////////////////////////////////////////////////////////////////////////////
// TMissingControlsUIForm dialog

class TMissingControlsUIForm : public TSceneEntityForm
{
// Construction
public:
	TMissingControlsUIForm(CWnd* parent, const CallbackOnChange& callback, ROS::ASceneEntity* sceneEntity);   // standard constructor

    virtual void UpdateForm();
// Dialog Data
	//{{AFX_DATA(TMissingControlsUIForm)
	enum { IDD = IDD_MISSING_CONTROLS_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(TMissingControlsUIForm)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(TMissingControlsUIForm)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MISSINGCONTROLSUI_H__371B5383_4688_11D2_968C_804B01767957__INCLUDED_)
