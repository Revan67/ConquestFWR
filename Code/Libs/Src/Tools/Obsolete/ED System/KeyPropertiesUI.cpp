// KeyPropertiesUI.cpp : implementation file
//

#include "PCH.h"
#include "stdafx.h"
#include "resource.h"
#include "KeyPropertiesUI.h"

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
	{
		str = "0" + str;
	}

	if(sign != 0)
	{	
		str = "-" + str;
	}

	edit.SetWindowText(str);
}
/////////////////////////////////////////////////////////////////////////////
// TKeyPropertiesUIForm dialog
TKeyPropertiesUIForm::TKeyPropertiesUIForm(CWnd* pParent /*=NULL*/)
: CDialog(TKeyPropertiesUIForm::IDD, pParent), mTime(0)
{
	//{{AFX_DATA_INIT(TKeyPropertiesUIForm)
	//}}AFX_DATA_INIT
}
//---------------------------------------------------------------------------
TKeyPropertiesUIForm::TKeyPropertiesUIForm(UINT nIDTemplate, CWnd* pParent /*=NULL*/)
: CDialog(nIDTemplate, pParent), mTime(0)
{
	//{{AFX_DATA_INIT(TKeyPropertiesUIForm)
	//}}AFX_DATA_INIT
}
//---------------------------------------------------------------------------
void TKeyPropertiesUIForm::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TKeyPropertiesUIForm)
	DDX_Control(pDX, IDC_TIME_EDIT, mTimeEdit);
	//}}AFX_DATA_MAP
}
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(TKeyPropertiesUIForm, CDialog)
	//{{AFX_MSG_MAP(TKeyPropertiesUIForm)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// TKeyPropertiesUIForm message handlers
//---------------------------------------------------------------------------
void TKeyPropertiesUIForm::SetTime(ROS::Time time)
{
	mTime = time;
}
//---------------------------------------------------------------------------
ROS::Time TKeyPropertiesUIForm::GetTime() const
{
	return mTime;
}
//---------------------------------------------------------------------------
void TKeyPropertiesUIForm::OnOK() 
{
	bool	success1 = false;

    // Verify time value
	try
    {	float timePoint = GetFloatValue(mTimeEdit);
    	if(timePoint >= 0)
        {
			success1 = true;
        }
    }
    catch(...)
    {
    }

    if(!success1)
    {
		MessageBox("Please enter a value of 0 or greater", "Invalid Time", MB_ICONHAND);
    }

	if(success1)
    {	
		mTime = ROS::Time(GetFloatValue(mTimeEdit));

		CDialog::OnOK();
    }
}
//---------------------------------------------------------------------------
BOOL TKeyPropertiesUIForm::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	SetFloatValue(mTimeEdit, mTime.GetTime());
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
//---------------------------------------------------------------------------
