// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "stdafx.h"
#include "ed.h"
#include "AnimationToolsUI.h"
#include "Utils.h"
#include "AnimationSettingsUI.h"
#include "edutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const char *REG_ANIM_X_POS = "AnimXPos";
const char *REG_ANIM_Y_POS = "AnimYPos";

TAnimationToolsUIForm*	gAnimTools = NULL;

void TAnimationToolsUIForm::IdleNotify()
{
	if(gAnimTools)
	{
		gAnimTools->IdleNotificationFunc();
	}
}
/////////////////////////////////////////////////////////////////////////////
// TAnimationToolsUIForm dialog
TAnimationToolsUIForm::TAnimationToolsUIForm(CWnd* pParent, ROS::SceneModel& sceneModel)
: CDialog(TAnimationToolsUIForm::IDD, pParent), mIdleNotification(IdleNotify)
{
    mDialogInitialized = false;
	AnimationToolsController::UpdateCB	updateCB = makeFunctor((AnimationToolsController::UpdateCB*)0, *this, &TAnimationToolsUIForm::UpdateGUI); 

	mAnimationToolsControllerSP = AggAPointer<AnimationToolsController>(new AnimationToolsController(sceneModel, updateCB));

	ASSERT(gAnimTools == NULL);
	gAnimTools = this;

	Create(TAnimationToolsUIForm::IDD, pParent);
	ShowWindow(SW_SHOWNORMAL);

	CString	fileName = mAnimationToolsControllerSP->GetSceneFileName().c_str();

	//{{AFX_DATA_INIT(TAnimationToolsUIForm)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}
// --------------------------------------------------------------------------
TAnimationToolsUIForm::~TAnimationToolsUIForm()
{
	DestroyWindow();
}
// --------------------------------------------------------------------------
void TAnimationToolsUIForm::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TAnimationToolsUIForm)
	DDX_Control(pDX, IDC_PLAY_BUTTON, mPlaySpeedButton);
	//}}AFX_DATA_MAP
}
// --------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(TAnimationToolsUIForm, CDialog)
	//{{AFX_MSG_MAP(TAnimationToolsUIForm)
	ON_BN_CLICKED(IDC_SET_BUTTON, SettingsSpeedButtonClick)
	ON_BN_CLICKED(IDC_PLAY_BUTTON, PlaySpeedButtonClick)
	ON_WM_MOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// TAnimationToolsUIForm message handlers
// --------------------------------------------------------------------------
void TAnimationToolsUIForm::SettingsSpeedButtonClick() 
{
	TAnimationSettingsUIForm	settingsUI;

    ROS::Time   duration = mAnimationToolsControllerSP->GetSceneDuration();
    settingsUI.SetDuration(duration);

    if(settingsUI.DoModal() == IDOK)
    {
		ROS::Time   newDuration = settingsUI.GetDuration();

        if(newDuration >= ROS::Time(0) && newDuration != duration)
        {
			mAnimationToolsControllerSP->SetSceneDuration(newDuration);
        }
    }
}
//---------------------------------------------------------------------------
void TAnimationToolsUIForm::UpdateGUI(int updateID)
{
	if(updateID == ModelNS::kScenePaused)
	{
		mPlaySpeedButton.SetWindowText("Play");
		mPlaySpeedButton.SetCheck(0);
#ifdef PORTED
		mPlaySpeedButton->Hint = "Play Scene";
        mPlaySpeedButton->Font->Color = clBlack;
        TFontStyles  fontStyles;
        mPlaySpeedButton->Font->Style = fontStyles;
#endif	
	}
}
// --------------------------------------------------------------------------
void TAnimationToolsUIForm::PlaySpeedButtonClick() 
{
    if(mPlaySpeedButton.GetCheck() != 0)
    {
		mPlaySpeedButton.SetWindowText("Stop");
#ifdef PORTED
		mPlaySpeedButton->Hint = "Stop Scene";
        mPlaySpeedButton->Font->Color = clRed;
        TFontStyles  fontStyles;
        fontStyles << fsBold;
        mPlaySpeedButton->Font->Style = fontStyles;
#endif
        mAnimationToolsControllerSP->PlayScene();
    }
    else
    {
        mAnimationToolsControllerSP->PauseScene();
    }
}
// --------------------------------------------------------------------------
void TAnimationToolsUIForm::OnOK() 
{
}
// --------------------------------------------------------------------------
void TAnimationToolsUIForm::OnCancel()
{
}
// --------------------------------------------------------------------------
void TAnimationToolsUIForm::IdleNotificationFunc()
{
    mAnimationToolsControllerSP->UpdateScene();
}
// --------------------------------------------------------------------------
void TAnimationToolsUIForm::PostNcDestroy() 
{
	delete this;

	CDialog::PostNcDestroy();
}
// --------------------------------------------------------------------------

BOOL TAnimationToolsUIForm::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	// Retreive the position of this window from the registry, if possible.
	{
		HKEY hEdKey;
		CString regPath (REGPATH_TOOLS);
		regPath += CString (REGPATH_APPNAME);
		if (ERROR_SUCCESS == RegOpenKeyEx (HKEY_CURRENT_USER, (LPCSTR) regPath, 0, KEY_READ, &hEdKey))
		{
			// Get the positions from the registry
			int xPos, yPos;
			DWORD valType, valSize;
			xPos = 0;
			yPos = 0;

			valSize = sizeof(int);
			if (ERROR_SUCCESS == RegQueryValueEx (hEdKey, REG_ANIM_X_POS, NULL, &valType, (BYTE *) &xPos, &valSize))
			{
				if (valSize != sizeof(int) || valType != REG_DWORD)
				{
					xPos = 0;
				}
			}
			valSize = sizeof(int);
			if (ERROR_SUCCESS == RegQueryValueEx (hEdKey, REG_ANIM_Y_POS, NULL, &valType, (BYTE *) &yPos, &valSize))
			{
				if (valSize != sizeof(int) || valType != REG_DWORD)
				{
					yPos = 0;
				}
			}

			// Move this window to the given position.
			SetWindowPos (NULL, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

			RegCloseKey (hEdKey);
		}
	}

	// Indicate that the dialog has been initialized.
    mDialogInitialized = true;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void TAnimationToolsUIForm::OnMove(int x, int y) 
{
	CDialog::OnMove(x, y);
	
	// Store the position of this window into the registry
	if (mDialogInitialized)
	{
		RECT r;
		GetWindowRect (&r);

		HKEY hEdKey;
		CString regPath (REGPATH_TOOLS);
		regPath += CString (REGPATH_APPNAME);

		DWORD disp;
		if
		(
			ERROR_SUCCESS ==
				RegCreateKeyEx
				(
					HKEY_CURRENT_USER,
					(LPCSTR) regPath,
					0, "",
					REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
					&hEdKey, &disp
				)
		)
		{
			// Store the position
			int xPos, yPos;
			DWORD valSize;
			xPos = r.left;
			yPos = r.top;

			valSize = sizeof(int);
			RegSetValueEx (hEdKey, REG_ANIM_X_POS, NULL, REG_DWORD, (BYTE *) &xPos, valSize);
			RegSetValueEx (hEdKey, REG_ANIM_Y_POS, NULL, REG_DWORD, (BYTE *) &yPos, valSize);
			RegCloseKey (hEdKey);
		}
	}
}
