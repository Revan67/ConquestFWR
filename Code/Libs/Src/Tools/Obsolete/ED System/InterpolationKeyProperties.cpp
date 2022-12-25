// InterpolationKeyProperties.cpp : implementation file
//

#include "PCH.h"
#include "stdafx.h"
#include "resource.h"
#include "InterpolationKeyProperties.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// InterpolationKeyProperties dialog
InterpolationKeyProperties::InterpolationKeyProperties(CWnd* pParent /*=NULL*/)
: TKeyPropertiesUIForm(InterpolationKeyProperties::IDD, pParent), mInterpolationType(kLinearFixed)
{
	//{{AFX_DATA_INIT(InterpolationKeyProperties)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}
//---------------------------------------------------------------------------
void InterpolationKeyProperties::DoDataExchange(CDataExchange* pDX)
{
	TKeyPropertiesUIForm::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(InterpolationKeyProperties)
	DDX_Control(pDX, IDC_LINEAR_RADIO, mLinearRadio);
	DDX_Control(pDX, IDC_SPLINE_RADIO, mSplineRadio);
	DDX_Control(pDX, IDC_FIXED_RADIO, mFixedRadio);
	DDX_Control(pDX, IDC_BLEND_RADIO, mBlendRadio);
	//}}AFX_DATA_MAP
}
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(InterpolationKeyProperties, TKeyPropertiesUIForm)
	//{{AFX_MSG_MAP(InterpolationKeyProperties)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// InterpolationKeyProperties message handlers
//---------------------------------------------------------------------------
void InterpolationKeyProperties::SetInterpolationType(InterpolationType type)
{
	mInterpolationType = type;
}
//---------------------------------------------------------------------------
InterpolationKeyProperties::InterpolationType InterpolationKeyProperties::GetInterpolationType() const
{
	return mInterpolationType;
}
//---------------------------------------------------------------------------
void InterpolationKeyProperties::OnOK() 
{
	TKeyPropertiesUIForm::OnOK();

	if(mLinearRadio.GetCheck() == 1)
	{
		if(mFixedRadio.GetCheck() == 1)
		{
			mInterpolationType = kLinearFixed;
		}
		else
		{
			ASSERT(mBlendRadio.GetCheck() == 1);

			mInterpolationType = kLinearBlend;
		}
	}
	else
	{
		ASSERT(mSplineRadio.GetCheck() == 1);

		if(mFixedRadio.GetCheck() == 1)
		{
			mInterpolationType = kSplineFixed;
		}
		else
		{
			ASSERT(mBlendRadio.GetCheck() == 1);

			mInterpolationType = kSplineBlend;
		}
	}
}
//---------------------------------------------------------------------------
BOOL InterpolationKeyProperties::OnInitDialog() 
{
	TKeyPropertiesUIForm::OnInitDialog();
	
	mLinearRadio.SetCheck(mInterpolationType == kLinearFixed || mInterpolationType == kLinearBlend ? 1 : 0);
	mSplineRadio.SetCheck(mInterpolationType == kSplineFixed || mInterpolationType == kSplineBlend ? 1 : 0);
	mFixedRadio.SetCheck(mInterpolationType == kLinearFixed || mInterpolationType == kSplineFixed ? 1 : 0);
	mBlendRadio.SetCheck(mInterpolationType == kLinearBlend || mInterpolationType == kSplineBlend ? 1 : 0);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
//---------------------------------------------------------------------------
