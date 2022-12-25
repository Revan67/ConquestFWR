// AmbientLightCustomControlsUI.cpp : implementation file
//

#include "PCH.h"
#include "stdafx.h"
#include "AmbientLightCustomControlsUI.h"
#include "ALight.h"
#include "Vector.h"
#include "FloatEditUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//---------------------------------------------------------------------------
const unsigned int	numDecimals = 4;
/////////////////////////////////////////////////////////////////////////////
// AmbientLightCustomControlsUI dialog
AmbientLightCustomControlsUI::AmbientLightCustomControlsUI(CWnd* pParent, const CallbackOnChange& callback, ROS::ASceneEntity* sceneEntity)
: TSceneEntityForm(AmbientLightCustomControlsUI::IDD, pParent, callback, sceneEntity)
{
	Create(AmbientLightCustomControlsUI::IDD, pParent);
	ShowWindow(SW_SHOWNORMAL);

	//{{AFX_DATA_INIT(AmbientLightCustomControlsUI)
	//}}AFX_DATA_INIT
}
//---------------------------------------------------------------------------
void AmbientLightCustomControlsUI::OnPaint() 
{
	BaseClass::OnPaint();

	UpdateColorBitmap(ROS::Color(GetFloatValue(mRedEdit), GetFloatValue(mGreenEdit), GetFloatValue(mBlueEdit), 1.0));
}
//---------------------------------------------------------------------------
void AmbientLightCustomControlsUI::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(AmbientLightCustomControlsUI)
	DDX_Control(pDX, IDC_INTENSITY_SPIN, mIntensitySpin);
	DDX_Control(pDX, IDC_BLUE_SPIN, mBlueSpin);
	DDX_Control(pDX, IDC_GREEN_SPIN, mGreenSpin);
	DDX_Control(pDX, IDC_RED_SPIN, mRedSpin);
	DDX_Control(pDX, IDC_COLOR_PICTURE, mColorBitmap);
	DDX_Control(pDX, IDC_RED_EDIT, mRedEdit);
	DDX_Control(pDX, IDC_GREEN_EDIT, mGreenEdit);
	DDX_Control(pDX, IDC_BLUE_EDIT, mBlueEdit);
	//}}AFX_DATA_MAP
}
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(AmbientLightCustomControlsUI, CDialog)
	//{{AFX_MSG_MAP(AmbientLightCustomControlsUI)
	ON_EN_CHANGE(IDC_RED_EDIT, OnChangeRedEdit)
	ON_EN_CHANGE(IDC_GREEN_EDIT, OnChangeGreenEdit)
	ON_EN_CHANGE(IDC_BLUE_EDIT, OnChangeBlueEdit)
	ON_NOTIFY(UDN_DELTAPOS, IDC_RED_SPIN, OnDeltaposRedSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_GREEN_SPIN, OnDeltaposGreenSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_BLUE_SPIN, OnDeltaposBlueSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_INTENSITY_SPIN, OnDeltaposIntensitySpin)
	ON_BN_CLICKED(IDC_COLOR_PALETTE_BUTTON, OnColorPaletteButton)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// AmbientLightCustomControlsUI message handlers
//---------------------------------------------------------------------------
void AmbientLightCustomControlsUI::UpdateForm()
{
    const ROS::ALight* light = dynamic_cast<const ROS::ALight*>(GetSceneEntity());

	ASSERT(IsNotNull(light));

	std::auto_ptr<ROS::ConstLightStateAccessor> lightStateAccess = light->GetConstLightStateAccessor();

	ROS::Color color = lightStateAccess->GetColor();

	SetFloatValue(mRedEdit, color.GetRed(), numDecimals);
	SetFloatValue(mGreenEdit, color.GetGreen(), numDecimals);
	SetFloatValue(mBlueEdit, color.GetBlue(), numDecimals);

	UpdateColorBitmap(color);
}
//---------------------------------------------------------------------------
void AmbientLightCustomControlsUI::UpdateColorBitmap(const ROS::Color& color)
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
void AmbientLightCustomControlsUI::OnChangeRedEdit() 
{
    ROS::ALight* light = dynamic_cast<ROS::ALight*>(GetSceneEntity());

	ASSERT(IsNotNull(light));

	float	red = GetFloatValue(mRedEdit);

	// Clamp to [0, 1]
	if(red > 1.0)
	{	
		red = 1.0;
		SetFloatValue(mRedEdit, red, numDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeRedEdit().
	}			// Thus, there is no need to continue
	else if(red < 0.0)
	{	
		red = 0.0;
		SetFloatValue(mRedEdit, red, numDecimals);

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
void AmbientLightCustomControlsUI::OnChangeGreenEdit() 
{
   ROS::ALight* light = dynamic_cast<ROS::ALight*>(GetSceneEntity());

	ASSERT(IsNotNull(light));

	float	green = GetFloatValue(mGreenEdit);

	// Clamp to [0, 1]
	if(green > 1.0)
	{	
		green = 1.0;
		SetFloatValue(mGreenEdit, green, numDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeGreenEdit().
	}			// Thus, there is no need to continue
	else if(green < 0.0)
	{	
		green = 0.0;
		SetFloatValue(mGreenEdit, green, numDecimals);

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
void AmbientLightCustomControlsUI::OnChangeBlueEdit() 
{
   ROS::ALight* light = dynamic_cast<ROS::ALight*>(GetSceneEntity());

	ASSERT(IsNotNull(light));

	float	blue = GetFloatValue(mBlueEdit);

	// Clamp to [0, 1]
	if(blue > 1.0)
	{	
		blue = 1.0;
		SetFloatValue(mBlueEdit, blue, numDecimals);

		return;	// The call to SetFloatValue caused a recursive call to OnChangeBlueEdit().
	}			// Thus, there is no need to continue
	else if(blue < 0.0)
	{	
		blue = 0.0;
		SetFloatValue(mBlueEdit, blue, numDecimals);

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
void AmbientLightCustomControlsUI::OnDeltaposRedSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* nMUpDown = (NM_UPDOWN*)pNMHDR;

	const float	red = GetFloatValue(mRedEdit) - 0.01 * nMUpDown->iDelta;

	SetFloatValue(mRedEdit, red, numDecimals);

	mRedSpin.SetPos(0);

	*pResult = 0;
}
//---------------------------------------------------------------------------
void AmbientLightCustomControlsUI::OnDeltaposGreenSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* nMUpDown = (NM_UPDOWN*)pNMHDR;

	const float	green = GetFloatValue(mGreenEdit) - 0.01 * nMUpDown->iDelta;

	SetFloatValue(mGreenEdit, green, numDecimals);
	
	mGreenSpin.SetPos(0);

	*pResult = 0;
}
//---------------------------------------------------------------------------
void AmbientLightCustomControlsUI::OnDeltaposBlueSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* nMUpDown = (NM_UPDOWN*)pNMHDR;

	const float	blue = GetFloatValue(mBlueEdit) - 0.01 * nMUpDown->iDelta;

	SetFloatValue(mBlueEdit, blue, numDecimals);

	mBlueSpin.SetPos(0);

	*pResult = 0;
}
//---------------------------------------------------------------------------
void AmbientLightCustomControlsUI::OnDeltaposIntensitySpin(NMHDR* pNMHDR, LRESULT* pResult) 
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
		SetFloatValue(mRedEdit, red, numDecimals);
		SetFloatValue(mGreenEdit, green, numDecimals);
		SetFloatValue(mBlueEdit, blue, numDecimals);
	}

	mIntensitySpin.SetPos(0);

	*pResult = 0;
}
//---------------------------------------------------------------------------
void AmbientLightCustomControlsUI::OnColorPaletteButton() 
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

		SetFloatValue(mRedEdit, redComponent, numDecimals);
		SetFloatValue(mGreenEdit, greenComponent, numDecimals);
		SetFloatValue(mBlueEdit, blueComponent, numDecimals);
	}
}
//---------------------------------------------------------------------------
