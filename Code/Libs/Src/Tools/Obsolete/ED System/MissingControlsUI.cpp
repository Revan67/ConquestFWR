// TMissingControlsUI.cpp : implementation file
//
//---------------------------------------------------------------------------
#include "PCH.h"
#include "stdafx.h"
#include "resource.h"
#include "MissingControlsUI.h"
//---------------------------------------------------------------------------
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// TMissingControlsUIForm dialog
//---------------------------------------------------------------------------
TMissingControlsUIForm::TMissingControlsUIForm(CWnd* parent, const CallbackOnChange& callback, ROS::ASceneEntity* sceneEntity)
: TSceneEntityForm(TMissingControlsUIForm::IDD, parent, callback, sceneEntity)
{
	Create(TMissingControlsUIForm::IDD, parent);
	ShowWindow(SW_SHOWNORMAL);

	//{{AFX_DATA_INIT(TMissingControlsUIForm)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}
//---------------------------------------------------------------------------
void TMissingControlsUIForm::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TMissingControlsUIForm)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(TMissingControlsUIForm, CDialog)
	//{{AFX_MSG_MAP(TMissingControlsUIForm)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// TMissingControlsUIForm message handlers
//---------------------------------------------------------------------------
void TMissingControlsUIForm::UpdateForm()
{
}
//---------------------------------------------------------------------------
