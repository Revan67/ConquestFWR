// Author: Shaival Varma
// --------------------------------------------------------------------------
// AnimatedSceneEntityControlsUI.cpp : implementation file
//
//---------------------------------------------------------------------------
#include "PCH.h"
#include "stdafx.h"
#include "ed.h"
#include "AnimatedSceneEntityControlsUI.h"
#include "ACompoundSceneEntity.h"
#include "Utils.h"
#include "ConstMotionStateAccessor.h"
#include "MotionStateAccessor.h"
//---------------------------------------------------------------------------
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// TAnimatedSceneEntityControlsUIForm dialog
TAnimatedSceneEntityControlsUIForm::TAnimatedSceneEntityControlsUIForm(CWnd* pParent, const CallbackOnChange& callback, ROS::ASceneEntity* sceneEntity)
: TSceneEntityForm(TAnimatedSceneEntityControlsUIForm::IDD, pParent, callback, sceneEntity)
{
	Create(TAnimatedSceneEntityControlsUIForm::IDD, pParent);
	ShowWindow(SW_SHOWNORMAL);

	//{{AFX_DATA_INIT(TAnimatedSceneEntityControlsUIForm)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}
//---------------------------------------------------------------------------
void TAnimatedSceneEntityControlsUIForm::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TAnimatedSceneEntityControlsUIForm)
	DDX_Control(pDX, IDC_MOTION_COMBO_BOX, mMotionNameComboBox);
	//}}AFX_DATA_MAP
}
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(TAnimatedSceneEntityControlsUIForm, CDialog)
	//{{AFX_MSG_MAP(TAnimatedSceneEntityControlsUIForm)
	ON_CBN_SELCHANGE(IDC_MOTION_COMBO_BOX, OnSelchangeMotionComboBox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// TAnimatedSceneEntityControlsUIForm message handlers
//---------------------------------------------------------------------------
void TAnimatedSceneEntityControlsUIForm::UpdateForm()
{
    const ROS::ACompoundSceneEntity* aCompoundSE = dynamic_cast<const ROS::ACompoundSceneEntity*>(GetSceneEntity());

	ASSERT(IsNotNull(aCompoundSE));

    if(aCompoundSE)
    {   std::auto_ptr<ROS::ConstStaticsStateAccessor> staticsStateAccess = aCompoundSE->GetConstStaticsStateAccessor();

#ifdef PORTED
		ROS::Location loc = staticsStateAccess->GetLocation();
        SetFloatValue(mLocationXEdit, loc.GetX());
        SetFloatValue(mLocationYEdit, loc.GetY());
        SetFloatValue(mLocationZEdit, loc.GetZ());
#endif
        mMotionNameComboBox.ResetContent();

        std::auto_ptr<ROS::ConstMotionStateAccessor>	motionAccessor = aCompoundSE->GetConstMotionStateAccessor();

        int	motionCount = motionAccessor->GetMotionCount();

        if(motionCount > 0)
        {   for(int motionIdx = 0; motionIdx < motionCount; ++motionIdx)
            {	mMotionNameComboBox.AddString(motionAccessor->GetMotionName(motionIdx).c_str());
            }

            mMotionNameComboBox.EnableWindow(true);
            mMotionNameComboBox.SetCurSel(mMotionNameComboBox.FindStringExact(0, motionAccessor->GetCurrentMotionName().c_str()));
#ifdef PORTED
			EventsButton->Enabled = true;
#endif
        }
        else
        {   mMotionNameComboBox.EnableWindow(false);
#ifdef PORTED
	        EventsButton->Enabled = false;
#endif
        }
    }
}
//---------------------------------------------------------------------------
void TAnimatedSceneEntityControlsUIForm::OnSelchangeMotionComboBox() 
{
    ROS::ACompoundSceneEntity* aCompoundSE = dynamic_cast<ROS::ACompoundSceneEntity*>(GetSceneEntity());

	ASSERT(IsNotNull(aCompoundSE));

    if(aCompoundSE)
    {   std::auto_ptr<ROS::MotionStateAccessor>	motionAccessor = aCompoundSE->GetMotionStateAccessor();

		const int	selIdx = mMotionNameComboBox.GetCurSel();
		
		if(selIdx >= 0)
		{	CString	name;

			mMotionNameComboBox.GetLBText(selIdx, name);

			const char*	charName = name;

			motionAccessor->SetCurrentMotionName(charName);
		}
    }
}
//---------------------------------------------------------------------------
