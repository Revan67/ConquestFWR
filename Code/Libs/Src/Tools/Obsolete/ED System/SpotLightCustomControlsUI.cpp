// SpotLightCustomControlsUI.cpp : implementation file
//

#include "PCH.h"
#include "stdafx.h"
#include "ed.h"
#include "SpotLightCustomControlsUI.h"
#include "ALight.h"
#include "ASpotLight.h"
#include "Vector.h"
#include "FloatEditUtil.h"
#include "ConstSpotLightStateAccessor.h"
#include "SpotLightStateAccessor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//---------------------------------------------------------------------------
const unsigned int	numColorDecimals = 4;
const unsigned int	numRangeDecimals = 2;
const unsigned int	numCutOffDecimals = 2;
/////////////////////////////////////////////////////////////////////////////
// SpotLightCustomControlsUI dialog
SpotLightCustomControlsUI::SpotLightCustomControlsUI(CWnd* pParent, const CallbackOnChange& callback, ROS::ASceneEntity* sceneEntity)
: TSceneEntityForm(SpotLightCustomControlsUI::IDD, pParent, callback, sceneEntity)
{
	Create(SpotLightCustomControlsUI::IDD, pParent);
	ShowWindow(SW_SHOWNORMAL);

	//{{AFX_DATA_INIT(SpotLightCustomControlsUI)
	//}}AFX_DATA_INIT
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::OnPaint() 
{
	BaseClass::OnPaint();

	UpdateColorBitmap(ROS::Color(GetFloatValue(mRedEdit), GetFloatValue(mGreenEdit), GetFloatValue(mBlueEdit), 1.0));
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(SpotLightCustomControlsUI)
	DDX_Control(pDX, IDC_INTENSITY_SPIN, mIntensitySpin);
	DDX_Control(pDX, IDC_RANGE_SPIN, mRangeSpin);
	DDX_Control(pDX, IDC_CUTOFF_SPIN, mCutOffSpin);
	DDX_Control(pDX, IDC_BLUE_SPIN, mBlueSpin);
	DDX_Control(pDX, IDC_GREEN_SPIN, mGreenSpin);
	DDX_Control(pDX, IDC_RED_SPIN, mRedSpin);
	DDX_Control(pDX, IDC_COLOR_PICTURE, mColorBitmap);
	DDX_Control(pDX, IDC_RANGE_EDIT, mRangeEdit);
	DDX_Control(pDX, IDC_CUTOFF_EDIT, mCutOffEdit);
	DDX_Control(pDX, IDC_RED_EDIT, mRedEdit);
	DDX_Control(pDX, IDC_GREEN_EDIT, mGreenEdit);
	DDX_Control(pDX, IDC_BLUE_EDIT, mBlueEdit);
	//}}AFX_DATA_MAP
}
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(SpotLightCustomControlsUI, CDialog)
	//{{AFX_MSG_MAP(SpotLightCustomControlsUI)
	ON_EN_CHANGE(IDC_RANGE_EDIT, OnChangeRangeEdit)
	ON_EN_CHANGE(IDC_CUTOFF_EDIT, OnChangeCutOffEdit)
	ON_EN_CHANGE(IDC_RED_EDIT, OnChangeRedEdit)
	ON_EN_CHANGE(IDC_GREEN_EDIT, OnChangeGreenEdit)
	ON_EN_CHANGE(IDC_BLUE_EDIT, OnChangeBlueEdit)
	ON_NOTIFY(UDN_DELTAPOS, IDC_RED_SPIN, OnDeltaposRedSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_GREEN_SPIN, OnDeltaposGreenSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_BLUE_SPIN, OnDeltaposBlueSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_INTENSITY_SPIN, OnDeltaposIntensitySpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_RANGE_SPIN, OnDeltaposRangeSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_CUTOFF_SPIN, OnDeltaposCutOffSpin)
	ON_BN_CLICKED(IDC_COLOR_PALETTE_BUTTON, OnColorPaletteButton)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// SpotLightCustomControlsUI message handlers
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::UpdateForm()
{
    const ROS::ASpotLight* light = dynamic_cast<const ROS::ASpotLight*>(GetSceneEntity());

	ASSERT(IsNotNull(light));

	std::auto_ptr<ROS::ConstSpotLightStateAccessor> lightStateAccess = light->GetConstSpotLightStateAccessor();

	ROS::Color color = lightStateAccess->GetColor();

	SetFloatValue(mRangeEdit, lightStateAccess->GetRange(), numRangeDecimals);
	SetFloatValue(mCutOffEdit, lightStateAccess->GetCutOff(), numCutOffDecimals);
	SetFloatValue(mRedEdit, color.GetRed(), numColorDecimals);
	SetFloatValue(mGreenEdit, color.GetGreen(), numColorDecimals);
	SetFloatValue(mBlueEdit, color.GetBlue(), numColorDecimals);

	UpdateColorBitmap(color);
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::UpdateColorBitmap(const ROS::Color& color)
{
	// Calculate color bitmap to draw
	CRect	bitmapRect, parentRect, drawRect;

	mColorBitmap.GetWindowRect(bitmapRect);
	this->GetWindowRect(parentRect);

	drawRect = bitmapRect - parentRect.TopLeft();
	drawRect.DeflateRect(1,1);

	CBrush	brush(RGB(color.GetRed() * 255, color.GetGreen() * 255, color.GetBlue() * 255));

	CClientDC	dc(this);

	dc.FillRect(&drawRect, &brush);
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::OnChangeRangeEdit() 
{
    ROS::ASpotLight* light = dynamic_cast<ROS::ASpotLight*>(GetSceneEntity());

	ASSERT(IsNotNull(light));

	float	range = GetFloatValue(mRangeEdit);

	// Clamp to [0, inf]
	if(range < 0.0)
	{	
		range = 0.0;
		SetFloatValue(mRangeEdit, range, numRangeDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeRedEdit()
	}			// Thus, there is no need to continue

	std::auto_ptr<ROS::SpotLightStateAccessor>	lightStateAccess = light->GetSpotLightStateAccessor();

	if(lightStateAccess->GetRange() != range)
	{
		lightStateAccess->SetRange(range);
    	NotifyEntityStateChanged();
	}
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::OnChangeCutOffEdit() 
{
    ROS::ASpotLight* light = dynamic_cast<ROS::ASpotLight*>(GetSceneEntity());

	ASSERT(IsNotNull(light));

	float	cutOff = GetFloatValue(mCutOffEdit);

	// Clamp to [0, 179.99]
	if(cutOff < 0.0)
	{	
		cutOff = 0.0;
		SetFloatValue(mCutOffEdit, cutOff, numCutOffDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeRedEdit()
	}			// Thus, there is no need to continue
	else if(cutOff >= 180.0)
	{	
		cutOff = 179.99;
		SetFloatValue(mCutOffEdit, cutOff, numCutOffDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeRedEdit()
	}			// Thus, there is no need to continue

	std::auto_ptr<ROS::SpotLightStateAccessor>	lightStateAccess = light->GetSpotLightStateAccessor();

	if(lightStateAccess->GetCutOff() != cutOff)
	{
		lightStateAccess->SetCutOff(cutOff);
    	NotifyEntityStateChanged();
	}
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::OnChangeRedEdit() 
{
    ROS::ALight* light = dynamic_cast<ROS::ALight*>(GetSceneEntity());

	ASSERT(IsNotNull(light));

	float	red = GetFloatValue(mRedEdit);

	// Clamp to [0, 1]
	if(red > 1.0)
	{	
		red = 1.0;
		SetFloatValue(mRedEdit, red, numColorDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeRedEdit().
	}			// Thus, there is no need to continue
	else if(red < 0.0)
	{	
		red = 0.0;
		SetFloatValue(mRedEdit, red, numColorDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeRedEdit()
	}			// Thus, there is no need to continue

	std::auto_ptr<ROS::LightStateAccessor>	lightStateAccess = light->GetLightStateAccessor();

	ROS::Color	color = lightStateAccess->GetColor();

	if(color.GetRed() != red)
	{
		color.SetRed(red);

		UpdateColorBitmap(color);

		lightStateAccess->SetColor(color);
    	NotifyEntityStateChanged();
	}
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::OnChangeGreenEdit() 
{
   ROS::ALight* light = dynamic_cast<ROS::ALight*>(GetSceneEntity());

	ASSERT(IsNotNull(light));

	float	green = GetFloatValue(mGreenEdit);

	// Clamp to [0, 1]
	if(green > 1.0)
	{	
		green = 1.0;
		SetFloatValue(mGreenEdit, green, numColorDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeGreenEdit().
	}			// Thus, there is no need to continue
	else if(green < 0.0)
	{	
		green = 0.0;
		SetFloatValue(mGreenEdit, green, numColorDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeGreenEdit().
	}			// Thus, there is no need to continue

	std::auto_ptr<ROS::LightStateAccessor>	lightStateAccess = light->GetLightStateAccessor();

	ROS::Color	color = lightStateAccess->GetColor();

	if(color.GetGreen() != green)
	{
		color.SetGreen(green);

		UpdateColorBitmap(color);

		lightStateAccess->SetColor(color);
    	NotifyEntityStateChanged();
	}
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::OnChangeBlueEdit() 
{
   ROS::ALight* light = dynamic_cast<ROS::ALight*>(GetSceneEntity());

	ASSERT(IsNotNull(light));

	float	blue = GetFloatValue(mBlueEdit);

	// Clamp to [0, 1]
	if(blue > 1.0)
	{	
		blue = 1.0;
		SetFloatValue(mBlueEdit, blue, numColorDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeBlueEdit().
	}			// Thus, there is no need to continue
	else if(blue < 0.0)
	{	
		blue = 0.0;
		SetFloatValue(mBlueEdit, blue, numColorDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeBlueEdit().
	}			// Thus, there is no need to continue

	std::auto_ptr<ROS::LightStateAccessor>	lightStateAccess = light->GetLightStateAccessor();

	ROS::Color	color = lightStateAccess->GetColor();

	if(color.GetBlue() != blue)
	{
		color.SetBlue(blue);

		UpdateColorBitmap(color);

		lightStateAccess->SetColor(color);
    	NotifyEntityStateChanged();
	}
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::OnDeltaposRangeSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* nMUpDown = (NM_UPDOWN*)pNMHDR;

	const float	range = GetFloatValue(mRangeEdit) - 0.01 * nMUpDown->iDelta;

	SetFloatValue(mRangeEdit, range, numRangeDecimals);

	mRangeSpin.SetPos(0);

	*pResult = 0;
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::OnDeltaposCutOffSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* nMUpDown = (NM_UPDOWN*)pNMHDR;

	const float	cutOff = GetFloatValue(mCutOffEdit) - 0.01 * nMUpDown->iDelta;

	SetFloatValue(mCutOffEdit, cutOff, numCutOffDecimals);

	mCutOffSpin.SetPos(0);

	*pResult = 0;
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::OnDeltaposRedSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* nMUpDown = (NM_UPDOWN*)pNMHDR;

	const float	red = GetFloatValue(mRedEdit) - 0.01 * nMUpDown->iDelta;

	SetFloatValue(mRedEdit, red, numColorDecimals);

	mRedSpin.SetPos(0);

	*pResult = 0;
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::OnDeltaposGreenSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* nMUpDown = (NM_UPDOWN*)pNMHDR;

	const float	green = GetFloatValue(mGreenEdit) - 0.01 * nMUpDown->iDelta;

	SetFloatValue(mGreenEdit, green, numColorDecimals);
	
	mGreenSpin.SetPos(0);

	*pResult = 0;
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::OnDeltaposBlueSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* nMUpDown = (NM_UPDOWN*)pNMHDR;

	const float	blue = GetFloatValue(mBlueEdit) - 0.01 * nMUpDown->iDelta;

	SetFloatValue(mBlueEdit, blue, numColorDecimals);

	mBlueSpin.SetPos(0);

	*pResult = 0;
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::OnDeltaposIntensitySpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* nMUpDown = (NM_UPDOWN*)pNMHDR;
	
	const float	changeFactor = 0.01 * (nMUpDown->iDelta > 0 ? 1 : -1);

	float	red = GetFloatValue(mRedEdit);
	float	green = GetFloatValue(mGreenEdit);
	float	blue = GetFloatValue(mBlueEdit);

	Vector	color(red, green, blue);

	color.normalize();
	
	color *= changeFactor;

	red -= color.x;
	green -= color.y;
	blue -= color.z;

	// All components have to be in [0.01, 1]
	if(0.01 <= red && red <= 1.0 && 0.01 <= green && green <= 1.0 && 0.01 <= blue && blue <= 1.0)
	{
		SetFloatValue(mRedEdit, red, numColorDecimals);
		SetFloatValue(mGreenEdit, green, numColorDecimals);
		SetFloatValue(mBlueEdit, blue, numColorDecimals);
	}

	mIntensitySpin.SetPos(0);

	*pResult = 0;
}
//---------------------------------------------------------------------------
void SpotLightCustomControlsUI::OnColorPaletteButton() 
{
	const unsigned int	red = 255 * GetFloatValue(mRedEdit);
	const unsigned int	green = 255 * GetFloatValue(mGreenEdit);
	const unsigned int	blue = 255 * GetFloatValue(mBlueEdit);

	CColorDialog	colorDialog(RGB(red, green, blue), CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT, this);

	if(colorDialog.DoModal() == IDOK)
	{	
		const COLORREF	color = colorDialog.GetColor();

		const float redComponent = GetRValue(color) / 255.0;
		const float greenComponent = GetGValue(color) / 255.0;
		const float blueComponent = GetBValue(color) / 255.0;

		SetFloatValue(mRedEdit, redComponent, numColorDecimals);
		SetFloatValue(mGreenEdit, greenComponent, numColorDecimals);
		SetFloatValue(mBlueEdit, blueComponent, numColorDecimals);
	}
}
//---------------------------------------------------------------------------
