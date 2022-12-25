// CameraCustomControlsUI.cpp : implementation file
//---------------------------------------------------------------------------
#include "PCH.h"
#include "stdafx.h"
#include "ed.h"
#include "CameraCustomControlsUI.h"
#include "FloatEditUtil.h"
#include "ACamera.h"
#include "ConstCameraStateAccessor.h"
#include "CameraStateAccessor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//---------------------------------------------------------------------------
const unsigned int	numDecimals = 4;
/////////////////////////////////////////////////////////////////////////////
// CameraCustomControlsUI dialog
CameraCustomControlsUI::CameraCustomControlsUI(CWnd* pParent, const CallbackOnChange& callback, ROS::ASceneEntity* sceneEntity)
: TSceneEntityForm(CameraCustomControlsUI::IDD, pParent, callback, sceneEntity)
{
	Create(CameraCustomControlsUI::IDD, pParent);
	ShowWindow(SW_SHOWNORMAL);

	//{{AFX_DATA_INIT(CameraCustomControlsUI)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}
//---------------------------------------------------------------------------
void CameraCustomControlsUI::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CameraCustomControlsUI)
	DDX_Control(pDX, IDC_HORIZONTAL_FOV_SPIN, mHorizontalFOVSpin);
	DDX_Control(pDX, IDC_VERTICAL_FOV_SPIN, mVerticalFOVSpin);
	DDX_Control(pDX, IDC_HORIZONTAL_FOV_EDIT, mHorizontalFOVEdit);
	DDX_Control(pDX, IDC_VERTICAL_FOV_EDIT, mVerticalFOVEdit);
	//}}AFX_DATA_MAP
}
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CameraCustomControlsUI, CDialog)
	//{{AFX_MSG_MAP(CameraCustomControlsUI)
	ON_EN_CHANGE(IDC_HORIZONTAL_FOV_EDIT, OnChangeHorizontalFOVEdit)
	ON_EN_CHANGE(IDC_VERTICAL_FOV_EDIT, OnChangeVerticalFOVEdit)
	ON_NOTIFY(UDN_DELTAPOS, IDC_HORIZONTAL_FOV_SPIN, OnDeltaposHorizontalFOVSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_VERTICAL_FOV_SPIN, OnDeltaposVerticalFOVSpin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CameraCustomControlsUI message handlers
//---------------------------------------------------------------------------
void CameraCustomControlsUI::UpdateForm()
{
    const ROS::ACamera* camera = dynamic_cast<const ROS::ACamera*>(GetSceneEntity());

	ASSERT(IsNotNull(camera));

	std::auto_ptr<ROS::ConstCameraStateAccessor> cameraStateAccess = camera->GetConstCameraStateAccessor();

	SetFloatValue(mHorizontalFOVEdit, cameraStateAccess->GetHorizontalFOV(), numDecimals);
	SetFloatValue(mVerticalFOVEdit, cameraStateAccess->GetVerticalFOV(), numDecimals);
}
//---------------------------------------------------------------------------
void CameraCustomControlsUI::OnChangeHorizontalFOVEdit() 
{
    ROS::ACamera* camera = dynamic_cast<ROS::ACamera*>(GetSceneEntity());

	ASSERT(IsNotNull(camera));

	float	hFOV = GetFloatValue(mHorizontalFOVEdit);

	// Clamp to [0, 1]
	if(hFOV > 90.0)
	{	
		hFOV = 90.0;
		SetFloatValue(mHorizontalFOVEdit, hFOV, numDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeGreenEdit().
	}			// Thus, there is no need to continue
	else if(hFOV < 0.0)
	{	
		hFOV = 0.0;
		SetFloatValue(mHorizontalFOVEdit, hFOV, numDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeGreenEdit().
	}			// Thus, there is no need to continue

	std::auto_ptr<ROS::CameraStateAccessor>	cameraStateAccess = camera->GetCameraStateAccessor();

	const float	currentHFOV = cameraStateAccess->GetHorizontalFOV();

	if(hFOV != currentHFOV)
	{
		cameraStateAccess->SetHorizontalFOV(hFOV);
    	NotifyEntityStateChanged();
	}
}
//---------------------------------------------------------------------------
void CameraCustomControlsUI::OnChangeVerticalFOVEdit() 
{
    ROS::ACamera* camera = dynamic_cast<ROS::ACamera*>(GetSceneEntity());

	ASSERT(IsNotNull(camera));

	float	vFOV = GetFloatValue(mVerticalFOVEdit);

	// Clamp to [0, 1]
	if(vFOV > 90.0)
	{	
		vFOV = 90.0;
		SetFloatValue(mVerticalFOVEdit, vFOV, numDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeGreenEdit().
	}			// Thus, there is no need to continue
	else if(vFOV < 0.0)
	{	
		vFOV = 0.0;
		SetFloatValue(mVerticalFOVEdit, vFOV, numDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeGreenEdit().
	}			// Thus, there is no need to continue

	std::auto_ptr<ROS::CameraStateAccessor>	cameraStateAccess = camera->GetCameraStateAccessor();

	const float	currentVFOV = cameraStateAccess->GetVerticalFOV();

	if(vFOV != currentVFOV)
	{
		cameraStateAccess->SetVerticalFOV(vFOV);
    	NotifyEntityStateChanged();
	}
}
//---------------------------------------------------------------------------
void CameraCustomControlsUI::OnDeltaposHorizontalFOVSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* nMUpDown = (NM_UPDOWN*)pNMHDR;

	const float	red = GetFloatValue(mHorizontalFOVEdit) - 0.01 * nMUpDown->iDelta;

	SetFloatValue(mHorizontalFOVEdit, red, numDecimals);

	mHorizontalFOVSpin.SetPos(0);

	*pResult = 0;
}
//---------------------------------------------------------------------------
void CameraCustomControlsUI::OnDeltaposVerticalFOVSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* nMUpDown = (NM_UPDOWN*)pNMHDR;

	const float	green = GetFloatValue(mVerticalFOVEdit) - 0.01 * nMUpDown->iDelta;

	SetFloatValue(mVerticalFOVEdit, green, numDecimals);
	
	mVerticalFOVSpin.SetPos(0);

	*pResult = 0;
}
//---------------------------------------------------------------------------
