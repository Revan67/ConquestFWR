// DeformableKeyPropertiesUI.cpp : implementation file
//
#include "PCH.h"
#include "stdafx.h"
#include "ed.h"
#include "DeformableKeyPropertiesUI.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//---------------------------------------------------------------------------
static float GetFloatValue(const CEdit& edit)
{
	CString	str;
	
	edit.GetWindowText(str);

	return atof(str);
}
//---------------------------------------------------------------------------
static void SetFloatValue(CEdit& edit, float value)
{
	int	dec, sign;
	CString	str = _fcvt(value, 2, &dec, &sign);

	str = str.Left(dec) + "." + str.Right(str.GetLength() - dec);

	if(dec == 0)
	{	str = "0" + str;
	}

	if(sign != 0)
	{	str = "-" + str;
	}

	edit.SetWindowText(str);
}
/////////////////////////////////////////////////////////////////////////////
// TDeformableKeyPropertiesUIForm dialog
TDeformableKeyPropertiesUIForm::TDeformableKeyPropertiesUIForm(CWnd* pParent /*=NULL*/)
: CDialog(TDeformableKeyPropertiesUIForm::IDD, pParent), mTrackTime(0), mStartTime(0), mTransitionTime(0)
, mUseDamping(false), mDampingFactor(1.0), mAxis(kZAxis), mUpAxis(kYAxis), mUseAxis(false), mUseIK(false)
,mMoveTo(false), mPointAt(false)
{
	//{{AFX_DATA_INIT(TDeformableKeyPropertiesUIForm)
	//}}AFX_DATA_INIT
}
//---------------------------------------------------------------------------
void TDeformableKeyPropertiesUIForm::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TDeformableKeyPropertiesUIForm)
	DDX_Control(pDX, IDC_NEGATE_UP, mNegateUp);
	DDX_Control(pDX, IDC_NEGATE_FRONT, mNegateFront);
	DDX_Control(pDX, IDC_X_UP_AXIS_RADIO, mXUpAxisRadio);
	DDX_Control(pDX, IDC_Y_UP_AXIS_RADIO, mYUpAxisRadio);
	DDX_Control(pDX, IDC_Z_UP_AXIS_RADIO, mZUpAxisRadio);
	DDX_Control(pDX, IDC_UP_AXIS_STATIC, mUpAxisStatic);
	DDX_Control(pDX, IDC_DAMPING_STATIC, mDampingStatic);
	DDX_Control(pDX, IDC_AXIS_STATIC, mAxisStatic);
	DDX_Control(pDX, IDC_POINT_AT_TARGET, mPointAtBox);
	DDX_Control(pDX, IDC_MOVE_TO_TARGET, mMoveToBox);
	DDX_Control(pDX, IDC_X_AXIS_RADIO, mXAxisRadio);
	DDX_Control(pDX, IDC_Y_AXIS_RADIO, mYAxisRadio);
	DDX_Control(pDX, IDC_Z_AXIS_RADIO, mZAxisRadio);
	DDX_Control(pDX, IDC_DAMPING_EDIT, mDampingFactorEdit);
	DDX_Control(pDX, IDC_TIME_EDIT, mTrackTimeEdit);
	DDX_Control(pDX, IDC_START_EDIT, mStartTimeEdit);
	DDX_Control(pDX, IDC_TRANSITION_EDIT, mTransitionTimeEdit);
	//}}AFX_DATA_MAP
}
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(TDeformableKeyPropertiesUIForm, CDialog)
	//{{AFX_MSG_MAP(TDeformableKeyPropertiesUIForm)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TDeformableKeyPropertiesUIForm message handlers
void TDeformableKeyPropertiesUIForm::OnOK() 
{
	bool	success1 = false;
    bool	success2 = false;
	bool	success3 = false;
	bool	success4 = false;
	bool	success5 = false;
	bool    success6 = false;

    // Verify values
	try
    {
		float trackTime = GetFloatValue(mTrackTimeEdit);
    	if(trackTime >= 0)
        {
			success1 = true;
        }

		float startTime = GetFloatValue(mStartTimeEdit);
    	if(startTime >= 0)
        {
			success2 = true;
        }

    	float transitionTime = GetFloatValue(mTransitionTimeEdit);
    	if(transitionTime >= 0)
        {
			success3 = true;
        }

		if(mUseDamping)
		{
			float dampingFactor = GetFloatValue(mDampingFactorEdit);
			if(dampingFactor >= 0.0)
			{
				success4 = true;
			}
		}
		else
		{
			success4 = true;
		}

		if(mUseAxis)
		{
			if(mXAxisRadio.GetCheck() == 1)
			{
				if (mNegateFront.GetCheck() == 1)
					mAxis = kNXAxis;
				else
					mAxis = kXAxis;
			}
			else if(mYAxisRadio.GetCheck() == 1)
			{
				if (mNegateFront.GetCheck() == 1)
					mAxis = kNYAxis;
				else
					mAxis = kYAxis;
			}
			else
			{
				ASSERT(mZAxisRadio.GetCheck() == 1);

				if (mNegateFront.GetCheck() == 1)
					mAxis = kNZAxis;
				else
					mAxis = kZAxis;
			}

			// There is some wierd logic here to ensure that the user doesn't select the
			// positive and negative versions of an axis for up and front.
			mUpAxis = mAxis;
			if(mXUpAxisRadio.GetCheck() == 1)
			{
				if (mAxis != kXAxis)
				{
					if (mNegateUp.GetCheck() == 1)
						mUpAxis = kNXAxis;
					else
						mUpAxis = kXAxis;
				}
			}
			else if(mYUpAxisRadio.GetCheck() == 1)
			{
				if (mAxis != kYAxis)
				{
					if (mNegateUp.GetCheck() == 1)
						mUpAxis = kNYAxis;
					else
						mUpAxis = kYAxis;
				}
			}
			else
			{
				ASSERT(mZUpAxisRadio.GetCheck() == 1);

				if (mAxis != kZAxis)
				{
					if (mNegateUp.GetCheck() == 1)
						mUpAxis = kNZAxis;
					else
						mUpAxis = kZAxis;
				}
			}

			if (mUpAxis != mAxis)
			{
				success6 = true;
			}
		}
		else
		{
			success6 = true;
		}

		if(mUseIK)
		{
			bool moveTo = (mMoveToBox.GetCheck() == 1);
			bool pointAt = (mPointAtBox.GetCheck() == 1);

			if (moveTo || pointAt)
			{
				success5 = true;
			}
		}
		else
		{
			success5 = true;
		}
    }
    catch(...)
    {
    }

    if(!success1)
    {
		CString	message("Please enter a value of 0 or greater");

    	MessageBox(message, "Invalid Track Time", MB_OK);
    }

    if(!success2)
    {
		CString	message("Please enter a value of 0 or greater");

    	MessageBox(message, "Invalid Start Time", MB_OK);
    }

    if(!success3)
    {
		CString	message("Please enter a value of 0 or greater");

    	MessageBox(message, "Invalid Transition Time", MB_OK);
    }

	if (!success4)
	{
		CString	message("Please enter a value of 0 or greater");

    	MessageBox(message, "Invalid Damping Factor", MB_OK);
	}

	if (!success5)
	{
		CString	message("You have not selected to move to or point at the target.");

    	MessageBox(message, "Invalid IK Operation", MB_OK);
	}

	if (!success6)
	{
		CString	message("The up and front axis must be different.");

    	MessageBox(message, "Invalid IK Axis", MB_OK);
	}

	if(success1 && success2 && success3 && success4 && success5 && success6)
    {
		mTrackTime = ROS::Time(GetFloatValue(mTrackTimeEdit));
		mStartTime = ROS::Time(GetFloatValue(mStartTimeEdit));
		mTransitionTime = ROS::Time(GetFloatValue(mTransitionTimeEdit));
		mDampingFactor = GetFloatValue(mDampingFactorEdit);
		mMoveTo = (mMoveToBox.GetCheck() == 1);
		mPointAt = (mPointAtBox.GetCheck() == 1);

		CDialog::OnOK();
    }
}
//---------------------------------------------------------------------------
BOOL TDeformableKeyPropertiesUIForm::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	SetFloatValue(mTrackTimeEdit, mTrackTime.GetTime());
	SetFloatValue(mStartTimeEdit, mStartTime.GetTime());
	SetFloatValue(mTransitionTimeEdit, mTransitionTime.GetTime());
	
	if(mUseDamping)
	{
		mDampingStatic.EnableWindow(TRUE);
		mDampingFactorEdit.EnableWindow (TRUE);
		SetFloatValue (mDampingFactorEdit, mDampingFactor);
	}
	else
	{
		mDampingStatic.EnableWindow(FALSE);
		mDampingFactorEdit.EnableWindow (FALSE);
		mDampingFactorEdit.SetWindowText ("N/A");
	}
	
	if(mUseAxis)
	{
		mXAxisRadio.EnableWindow(TRUE);
		mYAxisRadio.EnableWindow(TRUE);
		mZAxisRadio.EnableWindow(TRUE);
		mAxisStatic.EnableWindow(TRUE);
		mXUpAxisRadio.EnableWindow(TRUE);
		mYUpAxisRadio.EnableWindow(TRUE);
		mZUpAxisRadio.EnableWindow(TRUE);
		mUpAxisStatic.EnableWindow(TRUE);
		mNegateUp.EnableWindow(TRUE);
		mNegateFront.EnableWindow(TRUE);

		mNegateUp.SetCheck(0);
		mNegateFront.SetCheck(0);

		switch (mAxis)
		{
		case kNXAxis:
			mNegateFront.SetCheck(1);
			// fall through on purpose

		case kXAxis:
			mXAxisRadio.SetCheck(1);
			break;

		case kNYAxis:
			mNegateFront.SetCheck(1);
			// fall through on purpose

		case kYAxis:
			mYAxisRadio.SetCheck(1);
			break;

		case kNZAxis:
			mNegateFront.SetCheck(1);
			// fall through on purpose

		case kZAxis:
			mZAxisRadio.SetCheck(1);
			break;

		default:
			ASSERT(0 && "Invalid axis selection");
			break;
		}

		switch (mUpAxis)
		{
		case kNXAxis:
			mNegateUp.SetCheck(1);
			// fall through on purpose

		case kXAxis:
			mXUpAxisRadio.SetCheck(1);
			break;

		case kNYAxis:
			mNegateUp.SetCheck(1);
			// fall through on purpose

		case kYAxis:
			mYUpAxisRadio.SetCheck(1);
			break;

		case kNZAxis:
			mNegateUp.SetCheck(1);
			// fall through on purpose

		case kZAxis:
			mZUpAxisRadio.SetCheck(1);
			break;

		default:
			ASSERT(0 && "Invalid axis selection");
			break;
		}
	}
	else
	{
		mAxisStatic.EnableWindow(FALSE);
		mXAxisRadio.EnableWindow(FALSE);
		mYAxisRadio.EnableWindow(FALSE);
		mZAxisRadio.EnableWindow(FALSE);
		mXUpAxisRadio.EnableWindow(FALSE);
		mYUpAxisRadio.EnableWindow(FALSE);
		mZUpAxisRadio.EnableWindow(FALSE);
		mUpAxisStatic.EnableWindow(FALSE);
		mNegateUp.EnableWindow(FALSE);
		mNegateFront.EnableWindow(FALSE);
	}

	if(mUseIK)
	{
		mMoveToBox.EnableWindow(TRUE);
		mPointAtBox.EnableWindow(TRUE);

		mMoveToBox.SetCheck(mMoveTo ? 1 : 0);
		mPointAtBox.SetCheck(mPointAt ? 1 : 0);
	}
	else
	{
		mMoveToBox.EnableWindow(FALSE);
		mPointAtBox.EnableWindow(FALSE);
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
//---------------------------------------------------------------------------
ROS::Time TDeformableKeyPropertiesUIForm::GetTrackTime() const
{
	return mTrackTime;
}
//---------------------------------------------------------------------------
ROS::Time TDeformableKeyPropertiesUIForm::GetStartTime() const
{
	return mStartTime;
}
//---------------------------------------------------------------------------
ROS::Time TDeformableKeyPropertiesUIForm::GetTransitionTime() const
{
	return mTransitionTime;
}
//---------------------------------------------------------------------------
void TDeformableKeyPropertiesUIForm::SetTrackTime(ROS::Time time)
{
	mTrackTime = time;
}
//---------------------------------------------------------------------------
void TDeformableKeyPropertiesUIForm::SetStartTime(ROS::Time time)
{
	mStartTime = time;
}
//---------------------------------------------------------------------------
void TDeformableKeyPropertiesUIForm::SetTransitionTime(ROS::Time time)
{
	mTransitionTime = time;
}
//---------------------------------------------------------------------------
void TDeformableKeyPropertiesUIForm::EnableDampingFactor (bool enable)
{
	mUseDamping = enable;
}
//---------------------------------------------------------------------------
float TDeformableKeyPropertiesUIForm::GetDampingFactor() const
{
	return mDampingFactor;
}
//---------------------------------------------------------------------------
void TDeformableKeyPropertiesUIForm::SetDampingFactor (float factor)
{
	mDampingFactor = factor;
}
//---------------------------------------------------------------------------
TDeformableKeyPropertiesUIForm::Axis TDeformableKeyPropertiesUIForm::GetAxis() const
{
	return mAxis;
}
//---------------------------------------------------------------------------
TDeformableKeyPropertiesUIForm::Axis  TDeformableKeyPropertiesUIForm::GetUpAxis() const
{
	return mUpAxis;
}
//---------------------------------------------------------------------------
void TDeformableKeyPropertiesUIForm::SetAxis(TDeformableKeyPropertiesUIForm::Axis axis)
{
	mAxis = axis;
}
//---------------------------------------------------------------------------
void TDeformableKeyPropertiesUIForm::SetUpAxis(TDeformableKeyPropertiesUIForm::Axis axis)
{
	mUpAxis = axis;
}
//---------------------------------------------------------------------------
void TDeformableKeyPropertiesUIForm::EnableAxis(bool enable)
{
	mUseAxis = enable;
}
//---------------------------------------------------------------------------
void TDeformableKeyPropertiesUIForm::EnableIKProperties (bool enable)
{
	mUseIK = enable;
	EnableAxis(enable);
	EnableDampingFactor(enable);
}
//---------------------------------------------------------------------------
bool TDeformableKeyPropertiesUIForm::GetPointAtFlag() const
{
	return mPointAt;
}
//---------------------------------------------------------------------------
bool TDeformableKeyPropertiesUIForm::GetMoveToFlag() const
{
	return mMoveTo;
}
//---------------------------------------------------------------------------
void TDeformableKeyPropertiesUIForm::SetPointAtFlag (bool pointAt)
{
	mPointAt = pointAt;
}
//---------------------------------------------------------------------------
void TDeformableKeyPropertiesUIForm::SetMoveToFlag(bool moveTo)
{
	mMoveTo = moveTo;
}
