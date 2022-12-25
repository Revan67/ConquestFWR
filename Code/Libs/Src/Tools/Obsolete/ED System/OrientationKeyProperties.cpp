// Author: Shaival Varma
// --------------------------------------------------------------------------
// OrientationKeyProperties.cpp : implementation file
//

#include "PCH.h"
#include "stdafx.h"
#include "resource.h"
#include "OrientationKeyProperties.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// OrientationKeyProperties dialog
OrientationKeyProperties::OrientationKeyProperties(CWnd* pParent /*=NULL*/)
: TKeyPropertiesUIForm(OrientationKeyProperties::IDD, pParent), mInterpolationType(kLinear), mSelectedTarget(NULL)
{
	//{{AFX_DATA_INIT(OrientationKeyProperties)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void OrientationKeyProperties::DoDataExchange(CDataExchange* pDX)
{
	TKeyPropertiesUIForm::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(OrientationKeyProperties)
	DDX_Control(pDX, IDC_TARGET_COMBO_BOX, mTargetComboBox);
	DDX_Control(pDX, IDC_LINEAR_RADIO, mLinearRadio);
	DDX_Control(pDX, IDC_SPLINE_RADIO, mSplineRadio);
	DDX_Control(pDX, IDC_TANGENT_RADIO, mTangentRadio);
	DDX_Control(pDX, IDC_LOOK_AT_RADIO, mLookAtRadio);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(OrientationKeyProperties, TKeyPropertiesUIForm)
	//{{AFX_MSG_MAP(OrientationKeyProperties)
	ON_CBN_SELCHANGE(IDC_TARGET_COMBO_BOX, OnSelchangeTargetComboBox)
	ON_BN_CLICKED(IDC_LINEAR_RADIO, OnLinearRadio)
	ON_BN_CLICKED(IDC_SPLINE_RADIO, OnSplineRadio)
	ON_BN_CLICKED(IDC_TANGENT_RADIO, OnTangentRadio)
	ON_BN_CLICKED(IDC_LOOK_AT_RADIO, OnLookAtRadio)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// OrientationKeyProperties message handlers
//---------------------------------------------------------------------------
void OrientationKeyProperties::SetInterpolationType(InterpolationType type)
{
	mInterpolationType = type;
}
//---------------------------------------------------------------------------
OrientationKeyProperties::InterpolationType OrientationKeyProperties::GetInterpolationType() const
{
	return mInterpolationType;
}
//---------------------------------------------------------------------------
void OrientationKeyProperties::SetTargets(const ROS::SceneEntityCollection& targets, ROS::ASceneEntity* currentTarget)
{
	mTargets = targets;
	mSelectedTarget = currentTarget;
}
//---------------------------------------------------------------------------
ROS::ASceneEntity* OrientationKeyProperties::GetTarget() const
{
	ASSERT(mInterpolationType == kLookAt);

	return mSelectedTarget;
}
//---------------------------------------------------------------------------
void OrientationKeyProperties::OnOK() 
{
	TKeyPropertiesUIForm::OnOK();

	if(mLinearRadio.GetCheck() == 1)
	{
		mInterpolationType = kLinear;
		mSelectedTarget = NULL;
	}
	else if(mSplineRadio.GetCheck() == 1)
	{
		mInterpolationType = kSpline;
		mSelectedTarget = NULL;
	}
	else if(mTangentRadio.GetCheck() == 1)
	{
		mInterpolationType = kTangent;
		mSelectedTarget = NULL;
	}
	else 
	{
		ASSERT(mLookAtRadio.GetCheck() == 1);

		mInterpolationType = kLookAt;
	}          
}
//---------------------------------------------------------------------------
BOOL OrientationKeyProperties::OnInitDialog() 
{
	TKeyPropertiesUIForm::OnInitDialog();
	
	mLinearRadio.SetCheck(mInterpolationType == kLinear ? 1 : 0);
	mSplineRadio.SetCheck(mInterpolationType == kSpline ? 1 : 0);
	mTangentRadio.SetCheck(mInterpolationType == kTangent ? 1 : 0);
	
	// Look at
	mLookAtRadio.SetCheck(mInterpolationType == kLookAt ? 1 : 0);
	mLookAtRadio.EnableWindow(mTargets.size() > 0);
	mTargetComboBox.EnableWindow(mInterpolationType == kLookAt && mTargets.size() > 0);
	
	ROS::SceneEntityCollection::const_iterator			begin = mTargets.begin();
	const ROS::SceneEntityCollection::const_iterator	end = mTargets.end();

	mTargetComboBox.ResetContent();

	while(begin != end)
	{
		ROS::ASceneEntity*	entity = *begin;

		const int idx = mTargetComboBox.AddString((*begin)->GetConstSceneEntityStateAccessor()->GetName().c_str());

		mTargetComboBox.SetItemDataPtr(idx, entity);

		if(entity == mSelectedTarget)
		{
			mTargetComboBox.SetCurSel(idx);
		}

		++begin;
	}

	if(mSelectedTarget == NULL && mTargetComboBox.GetCount() > 0)
	{
		mTargetComboBox.SetCurSel(0);
		
		OnSelchangeTargetComboBox();
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
//---------------------------------------------------------------------------
void OrientationKeyProperties::OnSelchangeTargetComboBox() 
{
	const int	idx = mTargetComboBox.GetCurSel();

	ASSERT(idx >= 0);

	void*	data = mTargetComboBox.GetItemDataPtr(idx);
	ASSERT(data);

	mSelectedTarget = reinterpret_cast<ROS::ASceneEntity*>(data);
}
//---------------------------------------------------------------------------
void OrientationKeyProperties::OnLinearRadio() 
{
	mTargetComboBox.EnableWindow(false);
}
//---------------------------------------------------------------------------
void OrientationKeyProperties::OnSplineRadio() 
{
	mTargetComboBox.EnableWindow(false);
}
//---------------------------------------------------------------------------
void OrientationKeyProperties::OnTangentRadio() 
{
	mTargetComboBox.EnableWindow(false);
}
//---------------------------------------------------------------------------
void OrientationKeyProperties::OnLookAtRadio() 
{
	mTargetComboBox.EnableWindow(true);
}
//---------------------------------------------------------------------------
