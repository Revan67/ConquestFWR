// Author: Shaival Varma
// --------------------------------------------------------------------------
// ScenePaletteUI.cpp : implementation file
//

#include "PCH.h"
#include "stdafx.h"

#include <afxpriv.h>	// For WM_KICKIDLE
#include <fstream>
#include "ED.h"
#include "ScenePaletteUI.h"
#include "ModelNS.h"
#include "Utils.h"
#include "StringUtils.h"
#include "SceneModel.h"
#include "Type.h"
#include "ASceneEntity.h"
#include "AStaticSceneEntity.h"
#include "ADynamicCamera.h"
#include "ALight.h"
#include "Article.h"
#include "Actor.h"
#include "AmbientLight.h"
#include "CompoundSceneEntity.h"
#include "DeformableSceneEntity.h"
#include "MissingControlsUI.h"
#include "AnimatedSceneEntityControlsUI.h"
#include "AmbientLightCustomControlsUI.h"
#include "CameraCustomControlsUI.h"
#include "DirectionalLightCustomControlsUI.h"
#include "PointLightCustomControlsUI.h"
#include "SpotLightCustomControlsUI.h"
#include "AnimationToolsUI.h"
#include "DAAudioObject.h"
#include "SelectDBEntityDialog.h"
#include "StringList.h"
#include "TrackViewUI.h"
#include "DBExtension.h"
#include "DACompoundObject.h"
#include "DADeformableObject.h"
#include "DABaseCamera.h"
#include "EventIterator.h"
#include "ConstSceneEntityStateAccessor.h"
#include "SceneEntityStateAccessor.h"
#include "ConstSpotLightStateAccessor.h"
#include "ASpotLight.h"
#include "DAAudioObject.h"
#include "edutil.h"


//
// Registry values
//

const char *REG_SCENE_X_POS = "SceneXPos";
const char *REG_SCENE_Y_POS = "SceneYPos";

//#define SWITCH_TO_NICKNAMES

#ifdef SWITCH_TO_NICKNAMES
#include "AudioRole.h"
#include "StringUtils.h"

ROS::ROSString GetAudioNickname(const ROS::ROSString& fileName)
{
	const ROS::ROSString	fileNameOnly = GetFileName(fileName);

	const unsigned int	entityCount = DBE::get_entity_count();

	for(unsigned int entityIdx = 0; entityIdx < entityCount; ++entityIdx)
	{
		const U32	category = DBE::get_entity_category(entityIdx);

		if(DBE::kAudio == DBE::get_category_type(category))
		{
			ROS::StringList	stringList;

			stringList.Resize(DBE::kAudioStringsPerEntity);

			DBE::get_entity_strings(entityIdx, stringList);

			const ROS::ROSString	dbFileName = stringList.GetString(DBE::kAudioFilename);

			if(fileNameOnly == GetFileName(dbFileName))
			{
				return DBE::get_entity_name(entityIdx);
			}
		}
	}

	ASSERT(0 && "Sound not found!");

	return "";
}
#endif


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//---------------------------------------------------------------------------
const CString kCaptionStr("Scene Palette");
const CString kSeparatorStr(" - ");
const CString kPropertiesCollapsedStr("+ ");
const CString kPropertiesExpandedStr("- ");
const CString kPropertiesStr("Properties");

enum PopupIndex
{
	kSceneEntityPopupMenuIndex,
	kDeformableSceneEntityPopupMenuIndex
};

const unsigned int kWindowOffset = 10; // pixels
/////////////////////////////////////////////////////////////////////////////
// CWndListDlg control bar data 
static UINT gToolBarButtonCommandID[] = {
											ID_SEPARATOR,
											IDC_NEW_SCENE_BUTTON,
											IDC_OPEN_SCENE_BUTTON,
											IDC_SAVE_SCENE_BUTTON, 
											IDC_SAVE_AND_CONVERT_SCENE_BUTTON, 
											ID_SEPARATOR,
											IDC_ADD_OBJECT_BUTTON,
											IDC_ADD_POSITION_MARKER_BUTTON,
											IDC_ADD_CAMERA_BUTTON,
											IDC_ADD_LIVE_CAMERA_BUTTON,
											IDC_ADD_AMBIENT_LIGHT_BUTTON,
											IDC_ADD_DIRECTIONAL_LIGHT_BUTTON,
											IDC_ADD_POINT_LIGHT_BUTTON,
											IDC_ADD_SPOT_LIGHT_BUTTON,
											ID_SEPARATOR,
											IDC_DELETE_OBJECT_BUTTON,
											ID_SEPARATOR,
											IDC_EDIT_OBJECTS_BUTTON,
											IDC_EDIT_VIEWS_BUTTON,
											ID_SEPARATOR,
											IDC_UNDO_OPERATION,
											IDC_REDO_OPERATION,
											ID_SEPARATOR,
											IDC_CREATE_VIEW_BUTTON,
											IDC_CREATE_TRACK_VIEW_BUTTON,
											ID_SEPARATOR,
											IDC_EXIT_BUTTON
										};
// --------------------------------------------------------------------------
void __cdecl EventHandlerFunction(unsigned int channel_id, const EventIterator& event_iter)
{
    for (int idx = 0; idx < event_iter.get_event_count (); ++idx)
    {
        float time = event_iter.get_event_time (idx);
        unsigned int type = event_iter.get_event_type (idx);
        int tag = *((int*)event_iter.get_event_data (idx));

		// Commented this out because it is annoying. -TNB
		// *** Should there be a message box here to indicate that the event was received.
        // Beep(100, 100);
    }
}
// --------------------------------------------------------------------------
void TScenePaletteUIForm::SceneCallback(ROS::SceneModel& sceneModel, ROS::SceneEvent sceneEvent, const ROS::ROSString& sceneEntityName, const ROS::ROSString& entityCategory, const ROS::StringList& descriptionStrings, const void** entity, void** entityUserData, ROS::SceneEventFlag* flags)
{

	TScenePaletteUIForm*	palette = reinterpret_cast<TScenePaletteUIForm*>(sceneModel.GetUserData());

	*flags = ROS::kSEFNone;

	ASSERT(palette);

    switch(sceneEvent)
    {
		case ROS::kSEAudioObjectConstruct:
			*entity = palette->CreateAudioObject(sceneEntityName, reinterpret_cast<const ROS::AStaticSceneEntity*>(*entity));
			*flags |= (ROS::kSEFUseStateInScriptAsInitialState | ROS::kSEFUseTransitionInScriptTheFirstTime | ROS::kSEFUseFloorHeightInScript);
            break;
    	case ROS::kSEAudioObjectDelete:
			AudioObjectDestroy(reinterpret_cast<const ROS::DAAudioObject*>(*entity));
            break;
    	case ROS::kSEAudioObjectPlay:
			palette->PlayAudioObject(reinterpret_cast<const ROS::DAAudioObject*>(*entity), *reinterpret_cast<float*>(entityUserData));
            break;
    	case ROS::kSEAudioObjectStop:
			AudioObjectStop(reinterpret_cast<const ROS::DAAudioObject*>(*entity));
            break;
		case ROS::kSECompoundSceneEntityConstruct:
			*entity = palette->CreateCompoundObject(sceneEntityName, descriptionStrings);
			*flags |= (ROS::kSEFUseStateInScriptAsInitialState | ROS::kSEFUseTransitionInScriptTheFirstTime | ROS::kSEFUseFloorHeightInScript);
            break;
    	case ROS::kSECompoundSceneEntityDelete:
			CompoundObjectDestroy(reinterpret_cast<const ROS::DACompoundObject*>(*entity));
            break;
    	case ROS::kSEDeformableSceneEntityConstruct:
            *entity = palette->CreateDeformableObject(sceneEntityName, descriptionStrings);
			*flags |= (ROS::kSEFUseStateInScriptAsInitialState | ROS::kSEFUseTransitionInScriptTheFirstTime | ROS::kSEFUseFloorHeightInScript);
            break;
    	case ROS::kSEDeformableSceneEntityDelete:
    		palette->DestroyDeformableObject(reinterpret_cast<const ROS::DADeformableObject*>(*entity), descriptionStrings);
            break;
        case ROS::kSEBaseCameraConstruct:
            *entity = CameraCreate(640, 480);
			*flags |= (ROS::kSEFUseStateInScriptAsInitialState | ROS::kSEFUseTransitionInScriptTheFirstTime | ROS::kSEFUseFloorHeightInScript);
            break;
        case ROS::kSEBaseCameraDelete:
            CameraDestroy(reinterpret_cast<const ROS::DABaseCamera*>(*entity));
            break;
        case ROS::kSEInternalBaseCameraConstruct:
            *entity = CameraCreate(640, 480);
			*flags |= (ROS::kSEFUseStateInScriptAsInitialState | ROS::kSEFUseTransitionInScriptTheFirstTime | ROS::kSEFUseFloorHeightInScript);
            break;
        case ROS::kSEInternalBaseCameraDelete:
            CameraDestroy(reinterpret_cast<const ROS::DABaseCamera*>(*entity));
            break;
		case ROS::kSECompoundSceneEntityUpdateDescription:
		case ROS::kSEDeformableSceneEntityUpdateDescription:
		case ROS::kSEAudioEntityUpdateDescription:
			DBE::get_entity_strings(sceneEntityName, const_cast<ROS::ROSString&>(entityCategory), const_cast<ROS::StringList&>(descriptionStrings));
			break;
        default:
        	ASSERT(0);	// Unknown case
	}
}
/////////////////////////////////////////////////////////////////////////////
// TScenePaletteUIForm dialog
//---------------------------------------------------------------------------
TScenePaletteUIForm::TScenePaletteUIForm(CWnd* pParent, ISystemContainer* system, IEngine* engine)
: CDialog(TScenePaletteUIForm::IDD, pParent), mScenePaletteControllerSP(NULL), 
  mPropertyControlsFormSP(NULL), mPropertiesAreExpanded(false),
  mTrackView(NULL), mSystem(system), mEngine(engine)
{
    mDialogInitialized = false;
	ASSERT(system && engine);
	//{{AFX_DATA_INIT(TScenePaletteUIForm)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	mIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}
//---------------------------------------------------------------------------
TScenePaletteUIForm::~TScenePaletteUIForm()
{
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TScenePaletteUIForm)
	DDX_Control(pDX, IDC_OBJECTS_GROUP_BOX, mEntitiesGroupBoxP);
	DDX_Control(pDX, IDC_RENDER_SELECTION_CHECK, mRenderSelectionCheckBox);
	DDX_Control(pDX, IDC_PROPERTIES_GROUP_BOX, mPropertiesGroupBoxP);
	DDX_Control(pDX, IDC_PROPERTIES_BUTTON, mPropertiesButtonP);
	DDX_Control(pDX, IDC_SCENE_FILE_EDIT, mCurrentSceneEditP);
	DDX_Control(pDX, IDC_OBJECT_LIST, mEntityListP);
	DDX_Control(pDX, IDC_LOCK_SELECTION_CHECK, mLockSelectionCheckBox);
	DDX_Control(pDX, IDC_ACTORS_RADIO, mActorsRadioP);
	DDX_Control(pDX, IDC_DEFORMABLES_RADIO, mDeformablesRadioP);
	DDX_Control(pDX, IDC_COMPOUNDS_RADIO, mCompoundsRadioP);
	DDX_Control(pDX, IDC_ARTICLES_RADIO, mArticlesRadioP);
	DDX_Control(pDX, IDC_LIGHTS_RADIO, mLightsRadioP);
	DDX_Control(pDX, IDC_CAMERAS_RADIO, mCamerasRadioP);
	DDX_Control(pDX, IDC_ALL_RADIO, mAllRadioP);
	//}}AFX_DATA_MAP
}
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(TScenePaletteUIForm, CDialog)
	//{{AFX_MSG_MAP(TScenePaletteUIForm)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(IDC_NEW_SCENE_BUTTON, OnNewSceneButton)
	ON_COMMAND(IDC_OPEN_SCENE_BUTTON, SceneBrowseBitBtnClick)
	ON_COMMAND(IDC_SAVE_SCENE_BUTTON, OnSaveSceneButton)
	ON_COMMAND(IDC_SAVE_AND_CONVERT_SCENE_BUTTON, OnSaveAndConvertSceneButton)
	ON_BN_CLICKED(IDC_ALL_RADIO, OnClickAllRadio)
	ON_BN_CLICKED(IDC_ACTORS_RADIO, OnClickActorsRadio)
	ON_BN_CLICKED(IDC_ARTICLES_RADIO, OnClickArticlesRadio)
	ON_BN_CLICKED(IDC_CAMERAS_RADIO, OnClickCamerasRadio)
	ON_BN_CLICKED(IDC_COMPOUNDS_RADIO, OnClickCompoundsRadio)
	ON_BN_CLICKED(IDC_DEFORMABLES_RADIO, OnClickDeformablesRadio)
	ON_BN_CLICKED(IDC_LIGHTS_RADIO, OnClickLightsRadio)
	ON_COMMAND(IDC_ADD_OBJECT_BUTTON, AddSceneObjectBitBtnClick)
	ON_COMMAND(IDC_ADD_POSITION_MARKER_BUTTON, OnAddPositionMarkerButton)
	ON_COMMAND(IDC_ADD_CAMERA_BUTTON, OnAddCameraButton)
	ON_COMMAND(IDC_ADD_LIVE_CAMERA_BUTTON, OnAddLiveCameraButton)
	ON_COMMAND(IDC_ADD_AMBIENT_LIGHT_BUTTON, OnAddAmbientLightButton)
	ON_COMMAND(IDC_ADD_DIRECTIONAL_LIGHT_BUTTON, OnAddDirectionalLightButton)
	ON_COMMAND(IDC_ADD_POINT_LIGHT_BUTTON, OnAddPointLightButton)
	ON_COMMAND(IDC_ADD_SPOT_LIGHT_BUTTON, OnAddSpotLightButton)
	ON_COMMAND(IDC_UNDO_OPERATION, OnUndo)
	ON_COMMAND(IDC_REDO_OPERATION, OnRedo)
	ON_COMMAND(ID_WRITE_ENTITY_POSITIONS_FILE, OnWriteEntityPositionsFile)
	ON_LBN_SELCHANGE(IDC_OBJECT_LIST, OnClickEntityList)
	ON_BN_CLICKED(IDC_PROPERTIES_BUTTON, OnClickPropertiesButton)
	ON_BN_CLICKED(IDC_LOCK_SELECTION_CHECK, OnLockSelectionCheckBoxClick)
	ON_COMMAND(IDC_CREATE_VIEW_BUTTON, OnCreateViewButton)
	ON_COMMAND(IDC_CREATE_TRACK_VIEW_BUTTON, OnTrackViewButton)
	ON_COMMAND(IDC_EDIT_OBJECTS_BUTTON, OnEditEntitiesButton)
	ON_COMMAND(IDC_EDIT_VIEWS_BUTTON, OnEditViewsButton)
	ON_COMMAND(IDC_EXIT_BUTTON, OnExitButton)
	ON_BN_CLICKED(IDC_RENDER_SELECTION_CHECK, OnRenderSelectionCheck)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_SCENE_PALETTE_DELETE, OnDeleteClick)
	ON_COMMAND(ID_SCENE_PALETTE_REPLACE, OnReplaceClick)
	ON_WM_MOVE()
	ON_COMMAND(ID_UNDO, OnUndo)
	ON_COMMAND(IDC_DELETE_OBJECT_BUTTON, OnDeleteClick)
	ON_COMMAND(ID_REDO, OnRedo)
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TScenePaletteUIForm message handlers
//---------------------------------------------------------------------------
BOOL TScenePaletteUIForm::OnInitDialog()
{
	// Read in all the user preferences
	const int	size = 255;
	char		moduleFileName[size];
	
	GetModuleFileName(NULL, moduleFileName, size);

	const char USERINI[] = "User.ini";
	const ROS::ROSString	iniName = GetFilePath(moduleFileName) + USERINI;
	// I had to make this work with reconstructing a stream instead of attempting to open
	// after a failure. Don't know why.  -TNB

	std::ifstream *iFStream = NULL;

	iFStream = new std::ifstream(iniName.c_str(), std::ios::in);

	if(!iFStream->is_open())
	{
		delete iFStream;
		// If it could not be found where the executable resides, look in the current
		// directory.

		iFStream = new std::ifstream(USERINI, std::ios::in);

		if (!iFStream->is_open())
		{
			delete iFStream;
			const ROS::ROSString	message = ROS::ROSString("Unable to open user preferences file: ") + iniName;

			MessageBox(message.c_str(), "Error");

			EndDialog(IDCANCEL);

			return TRUE;
		}
	}

	mUserPreferences.Read(*iFStream);
	delete iFStream;
	
	if(!mUserPreferences.IsValid())
	{
		const ROS::ROSString	message = ROS::ROSString("Invalid user preferences file: ") + iniName;

		MessageBox(message.c_str(), "Error");

		EndDialog(IDCANCEL);

		return TRUE;
	}

	// User preferences read successfully

	// Initialized the extension system
	const bool	started = DBE::startup(mUserPreferences.GetDBExtensionFilename(), mUserPreferences.GetDataPath());
	
	// Initialize dialog controls
	CDialog::OnInitDialog();
	
	EnableToolTips(TRUE);
	mEntitiesGroupBoxP.EnableToolTips(TRUE);

	// Create toolbar at the top of the dialog window
	if (mToolBar.Create(this))
	{
		mToolBar.LoadBitmap(IDR_MAIN);
	    mToolBar.SetButtons(gToolBarButtonCommandID, sizeof(gToolBarButtonCommandID)/sizeof(UINT));
	}             

	mToolBar.SetBarStyle(mToolBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	mToolBar.BringWindowToTop();

    // We need to resize the dialog to make room for tool bar.
	// First, figure out how big the control bars are.
	CRect	clientRect;
	CRect	clientWithBarRect;

	GetClientRect(clientRect);
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0, reposQuery, clientWithBarRect);
	
	// Move down all the controls already in the dialog
	const CPoint	offset(clientWithBarRect.left - clientRect.left, clientWithBarRect.top - clientRect.top); 

	CRect	childRect;					
	CWnd*	childWnd = GetWindow(GW_CHILD);
	
	while(childWnd)
	{
		if(childWnd->GetDlgCtrlID() != IDC_TOOLBAR_BITMAP)
		{
			childWnd->GetWindowRect(childRect);

			ScreenToClient(childRect);
			childRect.OffsetRect(offset);
			
			childWnd->MoveWindow(childRect, FALSE);
		}

		childWnd = childWnd->GetNextWindow();
	}

	// Adjust the dialog window dimensions to accomodate the tool bar
	CRect windowRect;
	
	GetWindowRect(windowRect);
	windowRect.right += clientRect.Width() - clientWithBarRect.Width();
	windowRect.bottom += clientRect.Height() - clientWithBarRect.Height();
	
	MoveWindow(windowRect, FALSE);
	
	// Finally, position the control bars
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);

	// Position this Scene Palette in the upper left corner of the screen
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	mPopupMenus.LoadMenu(IDR_SCENE_PALETTE_POPUP_MENU);

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(mIcon, TRUE);			// Set big icon
	SetIcon(mIcon, FALSE);		// Set small icon
	
	const bool	audioStarted = AudioObjectSystemStartup(m_hWnd, mSystem, mEngine);

	if(!audioStarted)
	{
		MessageBox("Audio system could not be started!", "Error");

		EndDialog(IDCANCEL);

		return TRUE;
	}
	
	CString commandLine = AfxGetApp()->m_lpCmdLine;

    commandLine.TrimLeft();
		
	const char* cmdLine = commandLine;

	CString	extension = GetFileExtension(cmdLine).c_str();
    if(extension == ".sce" || extension == ".scl" || commandLine.IsEmpty())
    {
		LoadScene(commandLine);
	}

	CreateView();

    UpdateGUI(ModelNS::kAll);

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
			if (ERROR_SUCCESS == RegQueryValueEx (hEdKey, REG_SCENE_X_POS, NULL, &valType, (BYTE *) &xPos, &valSize))
			{
				if (valSize != sizeof(int) || valType != REG_DWORD)
				{
					xPos = 0;
				}
			}
			valSize = sizeof(int);
			if (ERROR_SUCCESS == RegQueryValueEx (hEdKey, REG_SCENE_Y_POS, NULL, &valType, (BYTE *) &yPos, &valSize))
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

	return TRUE;  // return TRUE  unless you set the focus to a control
}
//---------------------------------------------------------------------------
// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void TScenePaletteUIForm::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, mIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}
//---------------------------------------------------------------------------
// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR TScenePaletteUIForm::OnQueryDragIcon()
{
	return (HCURSOR) mIcon;
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::LoadScene(const CString& kFileNameR)
{
	CWaitCursor	waitCursor;

	try
	{	
		CString	currentSceneFileName;

		if(IsNotNull(mScenePaletteControllerSP))
		{
			currentSceneFileName = mScenePaletteControllerSP->GetSceneFileName().c_str();
		}

		if(currentSceneFileName != kFileNameR || kFileNameR.IsEmpty())
		{
			// clean up old scene                                                   
			ScenePaletteController::UpdateCB updateCB = makeFunctor((ScenePaletteController::UpdateCB*)0, *this, &TScenePaletteUIForm::UpdateGUI);

			const char*	cFileName = kFileNameR;

			std::auto_ptr<ROS::SceneModel> sceneModelSP(new ROS::SceneModel(SceneCallback, this));
			sceneModelSP->SetUseInitialEntityState(true);					// While using the editor, we always want to see the entities at their scene file specified positions
			sceneModelSP->SetPerformanceStyle(ROS::SceneModel::kRepeat);	// While using the editor, we always want to have the scene repeat from Time(0)
			sceneModelSP->Read(cFileName);
			sceneModelSP->SetGameEngineUpdatingMode(true);        

			if(IsNull(mScenePaletteControllerSP))
			{
				// Create a new scene palette controller
				mScenePaletteControllerSP = AggAPointer<ScenePaletteController>(new ScenePaletteController(*sceneModelSP, updateCB));

				// Setup new animation tools and position next to this Scene Palette
				TAnimationToolsUIForm*	animToolsUIForm = new TAnimationToolsUIForm(this, *sceneModelSP);
				mAnimToolsUIFormSP = AggPointer<TAnimationToolsUIForm>(animToolsUIForm);

#if 0
				CRect	scenePaletteRect;

				GetWindowRect(&scenePaletteRect);
				animToolsUIForm->SetWindowPos(NULL, scenePaletteRect.Width() + kWindowOffset, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
#endif
			}
			else
			{
				// Already have a controller
				mScenePaletteControllerSP->PauseScene();
				ROS::SceneModel& oldSceneModel = mScenePaletteControllerSP->GetSceneModel();
				mScenePaletteControllerSP->ReplaceSceneModel(sceneModelSP.get());
				delete &oldSceneModel;
				mScenePaletteControllerSP->SetCurrentSceneTime(ROS::Time(0));
			}

	#ifdef PORTED
			mAnimToolsUIFormSP->Show();
	#endif
			sceneModelSP.release();

			UpdateGUI(ModelNS::kAll);
	//      SetSceneObjectTypeFilter(GetSceneObjectTypeFilter());
		}
	}
	catch(...)
	{
		CString	message = CString("File: ") + kFileNameR;

		MessageBox(message, "Failed to open file!", MB_OK);
	}

#ifdef SWITCH_TO_NICKNAMES
	/*********** TEMPORARY CODE TO CONVERT AUDIO FILE NAMES TO NICKNAMES ***********/
	ROS::SceneEntityCollection   sceneEntityColl;

	mScenePaletteControllerSP->GetSceneEntities(sceneEntityColl);

    ROS::SceneEntityCollection::iterator		begin = sceneEntityColl.begin();
	const ROS::SceneEntityCollection::iterator	end = sceneEntityColl.end();

	while(begin != end)
	{
		ROS::ASceneEntity*			sceneEntity = *begin;
		ROS::DeformableSceneEntity*	deformable = dynamic_cast<ROS::DeformableSceneEntity*>(sceneEntity);

		if(deformable)
		{
			ROS::ARole*		aRole = &deformable->GetSceneEntityStateAccessor()->GetRole(3);
			ROS::AudioRole*	audioRole = dynamic_cast<ROS::AudioRole*>(aRole);
			ASSERT(audioRole);

			const unsigned int	audioCount = audioRole->CountTimePoints();

			for(unsigned int audioIdx = 0; audioIdx < audioCount; ++audioIdx)
			{
				ROS::AudioState			state = audioRole->GetState(audioIdx);
				ROS::ROSString			fileName = state.GetAudioName();
				const ROS::ROSString	nickname = GetAudioNickname(fileName);

				state.SetAudioName(nickname);

				audioRole->StateUpdated(state, audioIdx);
			}
		}

		++begin;
	}
#endif
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnNewSceneButton()
{
	LoadScene("");
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::SceneBrowseBitBtnClick() 
{
    CString			filters("Scene (*.sce)|*.sce|Scene List (*.scl)|*.scl||");
	CFileDialog		fileDlg(TRUE, NULL, NULL, 0, filters, this);
	CString			fileName;
	
	mCurrentSceneEditP.GetWindowText(fileName);

#if 0
	/**********NOTE: FIGURE OUT HOW TO SET INITIAL PATH**********/
	CString	filePath = ExtractFilePath(fileName);

    if(!filePath.IsEmpty())
    {
		mOpenSceneDialogP->InitialDir = filePath.c_str();
    }
#endif

    if(fileDlg.DoModal() == IDOK)
    {
		CString  fileName = fileDlg.GetPathName();

        LoadScene(fileName);
    }	
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnTrackViewButton() 
{
	if(IsNull(mTrackView))
    {
		CWaitCursor	waitCursor;

		mTrackView.reset(new TTrackViewUIForm(this, mScenePaletteControllerSP->GetSceneModel()));
    }
    
    mTrackView->ShowWindow(SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnEditEntitiesButton()
{
	ASSERT(IsNotNull(mScenePaletteControllerSP));

	ROS::SceneEntityCollection   sceneEntityColl;

	mScenePaletteControllerSP->GetSceneEntities(sceneEntityColl);
	ASSERT(!sceneEntityColl.empty());	// This function should never be called if there are no entities

    ROS::SceneEntityCollection::const_iterator   begin = sceneEntityColl.begin();
	
	ROS::ASceneEntity*	entity = *begin;

	mScenePaletteControllerSP->LockSceneEntitySelection(false);
	SetSelectedSceneEntity(entity);
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnEditViewsButton()
{
	ASSERT(IsNotNull(mScenePaletteControllerSP));

	mScenePaletteControllerSP->LockSceneEntitySelection(false);
	SetSelectedSceneEntity(NULL);
	mScenePaletteControllerSP->LockSceneEntitySelection(true);
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnExitButton()
{
	CWaitCursor	waitCursor;

	ROS::SceneModel& sceneModel = mScenePaletteControllerSP->GetSceneModel();
	
	mScenePaletteControllerSP->ShutdownSystem();

	delete &sceneModel;

	if(DBE::has_started_up())
	{
		DBE::shutdown();
	}

	AudioObjectSystemShutdown();

	CDialog::OnOK();
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnSaveSceneButton()
{
	CWaitCursor	waitCursor;

    if(IsNotNull(mScenePaletteControllerSP))
    {
		const CString	filters("Scene (*.sce)|*.sce||");
		CFileDialog		fileDlg(FALSE, NULL, NULL, 0, filters, this);
		
		if(fileDlg.DoModal() == IDOK)
		{
			CString  fileName = fileDlg.GetPathName();
			
			const char* cFileName = fileName;

			// Ensure a .sce extension
			if(GetFileExtension(cFileName) != ".sce")
			{
				fileName = (GetFilePath(cFileName) + GetFileName(cFileName) + ".sce").c_str();
				cFileName = fileName;
			}

			waitCursor.Restore();

			mScenePaletteControllerSP->SaveScene(cFileName);
			
			mCurrentSceneEditP.SetWindowText(mScenePaletteControllerSP->GetSceneFileName().c_str());
		}	
    }
}//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnSaveAndConvertSceneButton()
{
	CWaitCursor	waitCursor;

    if(IsNotNull(mScenePaletteControllerSP))
    {
		const CString	filters("Scene (*.sce)|*.sce||");
		CFileDialog		fileDlg(FALSE, NULL, NULL, 0, filters, this);
		
		if(fileDlg.DoModal() == IDOK)
		{
			CString  fileName = fileDlg.GetPathName();
			
			const char* cFileName = fileName;

			// Ensure a .sce extension
			if(GetFileExtension(cFileName) != ".sce")
			{
				fileName = (GetFilePath(cFileName) + GetFileName(cFileName) + ".sce").c_str();
				cFileName = fileName;
			}

			waitCursor.Restore();

			mScenePaletteControllerSP->SaveScene(cFileName, true);
			
			mCurrentSceneEditP.SetWindowText(mScenePaletteControllerSP->GetSceneFileName().c_str());
		}	
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::UpdateGUI(int updateID)
{
    if(updateID == ModelNS::kAll || updateID == ModelNS::kEntitySelectionChanged || updateID == ModelNS::kEntityAdded || updateID == ModelNS::kSelectedEntityRemoved)
    {
		bool isScenePresent = false;

        if(IsNotNull(mScenePaletteControllerSP))
        {
			isScenePresent = mScenePaletteControllerSP->IsScenePresent();
        }

        AType<ROS::ASceneEntity>* sceneEntityTypeP;
		UINT	checkedCommandID, unCheckedCommandID;
		
        if(isScenePresent)
        {
			if(GetSelectedSceneEntity())
			{
				checkedCommandID = IDC_EDIT_OBJECTS_BUTTON;
				unCheckedCommandID = IDC_EDIT_VIEWS_BUTTON;				
			}
			else
			{
				checkedCommandID = IDC_EDIT_VIEWS_BUTTON;
				unCheckedCommandID = IDC_EDIT_OBJECTS_BUTTON;				
			}
			
			if(mAllRadioP.GetCheck())
            {
				sceneEntityTypeP = new Type<ROS::ASceneEntity, ROS::ASceneEntity>;
            }
            else if(mCamerasRadioP.GetCheck())
            {
				sceneEntityTypeP = new Type<ROS::ASceneEntity, ROS::ADynamicCamera>;
            }
            else if(mLightsRadioP.GetCheck())
            {
				sceneEntityTypeP = new Type<ROS::ASceneEntity, ROS::ALight>;
            }
            else if(mArticlesRadioP.GetCheck())
            {
				sceneEntityTypeP = new Type<ROS::ASceneEntity, ROS::Article>;
            }
            else if(mActorsRadioP.GetCheck())
            {
				sceneEntityTypeP = new Type<ROS::ASceneEntity, ROS::Actor>;
            }
            else
            {
				sceneEntityTypeP = new Type<ROS::ASceneEntity, ROS::ASceneEntity>;
            }
        }
        else
        {
			// No scene. Setup defaults.
			checkedCommandID = IDC_EDIT_VIEWS_BUTTON;
			unCheckedCommandID = IDC_EDIT_OBJECTS_BUTTON;
		
			mAllRadioP.SetCheck(1);
            sceneEntityTypeP = new Type<ROS::ASceneEntity, ROS::ASceneEntity>;
        }

		// Check the appropriate button an uncheck the other
		UINT	style;
		int		buttonIndex, imageIndex;
		
		buttonIndex = mToolBar.CommandToIndex(checkedCommandID);
		ASSERT(buttonIndex != -1);
	    mToolBar.GetButtonInfo(buttonIndex, checkedCommandID, style, imageIndex);
	    mToolBar.SetButtonInfo(buttonIndex, checkedCommandID, MAKELONG(LOWORD(style) | TBSTYLE_CHECKGROUP, HIWORD(style) | TBSTATE_CHECKED), imageIndex);

		buttonIndex = mToolBar.CommandToIndex(unCheckedCommandID);
		ASSERT(buttonIndex != -1);
	    mToolBar.GetButtonInfo(buttonIndex, unCheckedCommandID, style, imageIndex);
	    mToolBar.SetButtonInfo(buttonIndex, unCheckedCommandID, MAKELONG(LOWORD(style) | TBSTYLE_CHECKGROUP, HIWORD(style) & ~TBSTATE_CHECKED), imageIndex);

		// Enable the Edit Entities button only if there is at least one entity in the scene. Disable otherwise.
		bool	enable = false;

		if(isScenePresent)
		{
			ROS::SceneEntityCollection   sceneEntityColl;

			mScenePaletteControllerSP->GetSceneEntities(sceneEntityColl);

			enable = !sceneEntityColl.empty();
		}

		UINT	commandID;
		buttonIndex = mToolBar.CommandToIndex(IDC_EDIT_OBJECTS_BUTTON);
		ASSERT(buttonIndex != -1);
	    mToolBar.GetButtonInfo(buttonIndex, commandID, style, imageIndex);
		if(enable)
		{
			mToolBar.SetButtonInfo(buttonIndex, commandID, MAKELONG(LOWORD(style) | TBSTYLE_CHECKGROUP, HIWORD(style) & ~TBSTATE_ENABLED), imageIndex);
		}
		else
		{
			mToolBar.SetButtonInfo(buttonIndex, commandID, MAKELONG(LOWORD(style) | TBSTYLE_CHECKGROUP, HIWORD(style) | TBSTATE_ENABLED), imageIndex);
		}

        std::auto_ptr< AType<ROS::ASceneEntity> >    typeSP(sceneEntityTypeP);

        if(updateID == ModelNS::kAll || updateID == ModelNS::kEntityAdded || updateID == ModelNS::kSelectedEntityRemoved)
        {
			if(isScenePresent)
            {
				CString caption = kCaptionStr + kSeparatorStr + CString(mScenePaletteControllerSP->GetSceneName().c_str());

				// *** TODO: Add current data path to scene model caption, or in a text field in the
				// *** dialog itself.
				SetWindowText(caption);
            }
			else
			{
				SetWindowText(kCaptionStr);
			}

            UpdateEntityList(*typeSP);

			BOOL enable = isScenePresent ? TRUE : FALSE;

            mAllRadioP.EnableWindow(enable);
            mCamerasRadioP.EnableWindow(enable);
            mLightsRadioP.EnableWindow(enable);
            mArticlesRadioP.EnableWindow(enable);
            mActorsRadioP.EnableWindow(enable);
            mCompoundsRadioP.EnableWindow(enable);
            mDeformablesRadioP.EnableWindow(enable);

            if(isScenePresent)
            {
				ROS::ASceneEntity* selectedEntity = GetSelectedSceneEntity();

				mLockSelectionCheckBox.EnableWindow(selectedEntity ? TRUE : FALSE);
				mLockSelectionCheckBox.SetCheck((selectedEntity && mScenePaletteControllerSP->IsSceneEntitySelectionLocked()) ? 1 : 0);
            }

			mRenderSelectionCheckBox.EnableWindow(GetSelectedSceneEntity() ? TRUE : FALSE);
			
            mEntityListP.EnableWindow(enable);

            if(isScenePresent)
			{
				mCurrentSceneEditP.SetWindowText(mScenePaletteControllerSP->GetSceneFileName().c_str());
			}
			else
			{
				mCurrentSceneEditP.SetWindowText("");
			}
        }
        else if(updateID == ModelNS::kEntitySelectionChanged)
		{
			const ROS::ASceneEntity*   sceneEntity = GetSelectedSceneEntity();
            SetSelectionInEntityList(sceneEntity);

			if(sceneEntity)
			{
				mLockSelectionCheckBox.EnableWindow(TRUE);
				
				mRenderSelectionCheckBox.EnableWindow(TRUE);
				mRenderSelectionCheckBox.SetCheck(sceneEntity->GetConstSceneEntityStateAccessor()->IsVisible() ? 1 : 0);
			}
			else
			{
				mLockSelectionCheckBox.EnableWindow(FALSE);

				mRenderSelectionCheckBox.SetCheck(1);
				mRenderSelectionCheckBox.EnableWindow(FALSE);				
			}
		}
    }
    else if(updateID == ModelNS::kSelectedEntityUpdated)
    {
		const ROS::ASceneEntity*    sceneEntity = GetSelectedSceneEntity();

		ASSERT(sceneEntity);

        int itemIdx = mEntityListP.GetCurSel();
		
		void*	itemData = mEntityListP.GetItemDataPtr(itemIdx);
		ASSERT(((const void*)itemData) == sceneEntity);

		CString	nameInList;
		const CString sceneEntityName = sceneEntity->GetConstSceneEntityStateAccessor()->GetName().c_str();

		mEntityListP.GetText(itemIdx, nameInList);

		if(nameInList != sceneEntityName)
		{
			// Scene Entity's name has changed
			mEntityListP.DeleteString(itemIdx);

			itemIdx = mEntityListP.InsertString(itemIdx, sceneEntityName);
			mEntityListP.SetItemDataPtr(itemIdx, itemData);

			mEntityListP.SetCurSel(itemIdx);
		}
    }
	else if(updateID == ModelNS::kSecondaryEntityUpdated)
	{
		const ROS::ASceneEntity*		entity = GetSecondarySceneEntity();
		const ROS::AStaticSceneEntity*	sEntity = dynamic_cast<const ROS::AStaticSceneEntity*>(entity);

		if(sEntity)
		{
			if(sEntity->GetConstStaticsStateAccessor()->IsStaticsPathVisible())
			{
				UpdateGUI(ModelNS::kAll);
			}
		}

		return;
	}
	else if(updateID == ModelNS::kEntitySelectionLockUpdated)
	{
		ROS::ASceneEntity* selectedEntity = GetSelectedSceneEntity();

		mLockSelectionCheckBox.EnableWindow(selectedEntity ? TRUE : FALSE);
		mLockSelectionCheckBox.SetCheck((selectedEntity && mScenePaletteControllerSP->IsSceneEntitySelectionLocked()) ? 1 : 0);
    }

    UpdatePropertyEditor();
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnClickAllRadio() 
{
    if(IsNotNull(mScenePaletteControllerSP))
    {
		AType<ROS::ASceneEntity>*	sceneEntityTypeP = new Type<ROS::ASceneEntity, ROS::ASceneEntity>;
        
        std::auto_ptr< AType<ROS::ASceneEntity> >    typeSP(sceneEntityTypeP);

        UpdateEntityList(*typeSP);
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnClickActorsRadio() 
{
    if(IsNotNull(mScenePaletteControllerSP))
    {
		AType<ROS::ASceneEntity>*	sceneEntityTypeP = new Type<ROS::ASceneEntity, ROS::Actor>;
        
        std::auto_ptr< AType<ROS::ASceneEntity> >    typeSP(sceneEntityTypeP);

        UpdateEntityList(*typeSP);
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnClickArticlesRadio() 
{
    if(IsNotNull(mScenePaletteControllerSP))
    {
		AType<ROS::ASceneEntity>*	sceneEntityTypeP = new Type<ROS::ASceneEntity, ROS::Article>;
        
        std::auto_ptr< AType<ROS::ASceneEntity> >    typeSP(sceneEntityTypeP);

        UpdateEntityList(*typeSP);
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnClickCamerasRadio() 
{
    if(IsNotNull(mScenePaletteControllerSP))
    {
		AType<ROS::ASceneEntity>*	sceneEntityTypeP = new Type<ROS::ASceneEntity, ROS::ADynamicCamera>;
        
        std::auto_ptr< AType<ROS::ASceneEntity> >    typeSP(sceneEntityTypeP);

        UpdateEntityList(*typeSP);
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnClickCompoundsRadio() 
{
    if(IsNotNull(mScenePaletteControllerSP))
    {
		AType<ROS::ASceneEntity>*	sceneEntityTypeP = new Type<ROS::ASceneEntity, ROS::CompoundSceneEntity>;
        
        std::auto_ptr< AType<ROS::ASceneEntity> >    typeSP(sceneEntityTypeP);

        UpdateEntityList(*typeSP);
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnClickDeformablesRadio() 
{
    if(IsNotNull(mScenePaletteControllerSP))
    {
		AType<ROS::ASceneEntity>*	sceneEntityTypeP = new Type<ROS::ASceneEntity, ROS::DeformableSceneEntity>;
        
        std::auto_ptr< AType<ROS::ASceneEntity> >    typeSP(sceneEntityTypeP);

        UpdateEntityList(*typeSP);
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnClickLightsRadio() 
{
    if(IsNotNull(mScenePaletteControllerSP))
    {
		AType<ROS::ASceneEntity>*	sceneEntityTypeP = new Type<ROS::ASceneEntity, ROS::ALight>;
        
        std::auto_ptr< AType<ROS::ASceneEntity> >    typeSP(sceneEntityTypeP);

        UpdateEntityList(*typeSP);
    }
}
//---------------------------------------------------------------------------
void __fastcall TScenePaletteUIForm::UpdateEntityList(const AType<ROS::ASceneEntity>& kTypeR)
{
	mEntityListP.ResetContent();
	
	const ROS::ASceneEntity*	selectedEntity = GetSelectedSceneEntity();

	int	idx = -1;

	if(IsNotNull(mScenePaletteControllerSP))
    {
		ROS::SceneEntityCollection   sceneEntityColl;

        mScenePaletteControllerSP->GetSceneEntities(sceneEntityColl);

        ROS::SceneEntityCollection::const_iterator   begin = sceneEntityColl.begin();
        const ROS::SceneEntityCollection::const_iterator kEnd = sceneEntityColl.end();

		while(begin != kEnd)
        {
			if(kTypeR.Matches(**begin))
            {
				int	itemIdx = mEntityListP.AddString((*begin)->GetConstSceneEntityStateAccessor()->GetName().c_str());
				if(itemIdx != LB_ERR && itemIdx != LB_ERRSPACE)
				{
					mEntityListP.SetItemDataPtr(itemIdx, *begin);
					
					if(*begin == selectedEntity)
					{
						idx = itemIdx;
					}
				}
				else
				{
					return;
				}
            }

            ++begin;
        }
    }

	mEntityListP.SetCurSel(idx);
 }
//---------------------------------------------------------------------------
void __fastcall TScenePaletteUIForm::UpdatePropertyEditor()
{
    bool isScenePresent = false;

    if(IsNotNull(mScenePaletteControllerSP))
    {
		isScenePresent = mScenePaletteControllerSP->IsScenePresent();
    }

    BOOL enabled = (isScenePresent && GetSelectedSceneEntity()) ? TRUE : FALSE;

	mPropertiesButtonP.EnableWindow(enabled);

	CString	propertyString;

    if(mPropertiesAreExpanded)
    {
		propertyString = kPropertiesExpandedStr;

        DisplayCustomControls(GetSelectedSceneEntity() != NULL);
    }
    else
    {
		propertyString = kPropertiesCollapsedStr;
    }

	propertyString += kPropertiesStr;

    mPropertiesButtonP.SetWindowText(propertyString);

//    UpdateCustomControls();
}
//---------------------------------------------------------------------------
ROS::ASceneEntity* __fastcall TScenePaletteUIForm::GetSelectedSceneEntity() const
{
    if(IsNotNull(mScenePaletteControllerSP))
    {
		return mScenePaletteControllerSP->GetSelectedSceneEntity();
    }
    else
    {
		return NULL;
    }
}
//---------------------------------------------------------------------------
ROS::ASceneEntity* __fastcall TScenePaletteUIForm::GetSecondarySceneEntity() const
{
    if(IsNotNull(mScenePaletteControllerSP))
    {
		return mScenePaletteControllerSP->GetSecondarySceneEntity();
    }
    else
    {
		return NULL;
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::DisplayCustomControls(bool display)
{
	ROS::ASceneEntity*  sceneEntity = GetSelectedSceneEntity();

    if(display && sceneEntity != NULL)
    {
		std::auto_ptr<ASceneEntityFormFactory<TSceneEntityForm> >   formTypeSP(GetCompatibleFormType(sceneEntity));

        if(IsNotNull(mPropertyControlsFormSP))
        {
			// A form is present. Check if it is of the right type
            if(formTypeSP->Matches(*(mPropertyControlsFormSP.get())))
            {
				mPropertyControlsFormSP->SetSceneEntity(sceneEntity);
            }
            else
            {
				DisplayCustomControls(false);
            }
        }

        if(IsNull(mPropertyControlsFormSP))
        {
			TSceneEntityForm::CallbackOnChange  callback = makeFunctor((TSceneEntityForm::CallbackOnChange*)0, *this, &TScenePaletteUIForm::SceneEntityStateChanged);

            mPropertyControlsFormSP = SceneEntitySPtr(formTypeSP->Manufacture(mPropertiesButtonP.GetOwner(), callback, sceneEntity));

		
			ASSERT(IsNotNull(mPropertyControlsFormSP));

			mPropertyControlsFormSP->SetParent(this);
			mPropertyControlsFormSP->SetOwner(this);
			mPropertyControlsFormSP->UpdateForm();

			CRect	winRect, clientWinRect, embeddedWinRect, propertiesButtonRect;
			CPoint	clientOrigin(0, 0);

			ClientToScreen(&clientOrigin);
			GetWindowRect(&winRect);	
			GetClientRect(&clientWinRect);
			mPropertiesButtonP.GetWindowRect(&propertiesButtonRect);

			CPoint	propertiesButtonBottomLeft(propertiesButtonRect.left, propertiesButtonRect.bottom);

			// Set the embedded dialog's position
			mPropertyControlsFormSP->GetWindowRect(&embeddedWinRect);

			embeddedWinRect.OffsetRect(-(embeddedWinRect.TopLeft()));
			embeddedWinRect.OffsetRect(propertiesButtonBottomLeft - clientOrigin);

			mPropertyControlsFormSP->MoveWindow(&embeddedWinRect);

			// Resize the host dialog. This time we need to work with the window rect
			winRect.bottom += embeddedWinRect.Height();
			MoveWindow(&winRect);

			//mMainPanelP->Height += mPropertyControlsFormSP->Height;

			// Resize the group box for the property controls
			CRect	groupBoxWinRect;
			mPropertiesGroupBoxP.GetWindowRect(&groupBoxWinRect);
			// Since the rect is relative to the screen origin, we need to make it relative
			// to the parent before calling MoveWindow().
			CPoint	groupBoxOffset(groupBoxWinRect.TopLeft() - clientOrigin);

			groupBoxWinRect.OffsetRect(-(groupBoxWinRect.TopLeft()));
			groupBoxWinRect.OffsetRect(groupBoxOffset);

			// Now we are ready to expand it by the height of the embedded control window
			groupBoxWinRect.bottom += embeddedWinRect.Height();
            mPropertiesGroupBoxP.MoveWindow(groupBoxWinRect);
 
			mPropertyControlsFormSP->ShowWindow(SW_SHOW);
		}
    }
    else
    {
		if(IsNotNull(mPropertyControlsFormSP))
        {
			CRect	embeddedWinRect, winRect;

			GetWindowRect(&winRect);

			mPropertyControlsFormSP->GetWindowRect(&embeddedWinRect);

			// Resize the host dialog
			winRect.bottom -= embeddedWinRect.Height();
			
			mPropertyControlsFormSP->ShowWindow(SW_HIDE);

			MoveWindow(&winRect);
			
			// Resize the group box for the property controls
			CRect	groupBoxWinRect;
			mPropertiesGroupBoxP.GetWindowRect(&groupBoxWinRect);
			// Since the rect is relative to the screen origin, we need to make it relative
			// to the parent before calling MoveWindow().
			CPoint	clientOrigin(0, 0);

			ClientToScreen(&clientOrigin);

			CPoint	groupBoxOffset(groupBoxWinRect.TopLeft() - clientOrigin);

			groupBoxWinRect.OffsetRect(-(groupBoxWinRect.TopLeft()));
			groupBoxWinRect.OffsetRect(groupBoxOffset);

			// Now we are ready to expand it by the height of the embedded control window
			groupBoxWinRect.bottom -= embeddedWinRect.Height();
            mPropertiesGroupBoxP.MoveWindow(groupBoxWinRect);

            //mPropertiesGroupBoxP->Height -= mPropertyControlsFormSP->Height;
            
			delete mPropertyControlsFormSP.release();

			mPropertyControlsFormSP = SceneEntitySPtr(NULL);
		}

    }
}
//---------------------------------------------------------------------------
void __fastcall TScenePaletteUIForm::SetSelectedSceneEntity(ROS::ASceneEntity* entityP)
{
    if(IsNotNull(mScenePaletteControllerSP))
    {
		mScenePaletteControllerSP->SetSelectedSceneEntity(entityP);
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnClickEntityList()
{
    int itemIdx = mEntityListP.GetCurSel();

	AttemptEntityListSelection(itemIdx);
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::AttemptEntityListSelection(int itemIdx)
{
	ASSERT(itemIdx >= 0);
	ASSERT(IsNotNull(mScenePaletteControllerSP));

	ROS::ASceneEntity*    sceneEntityP = dynamic_cast<ROS::ASceneEntity*>(reinterpret_cast<ROS::ASceneEntity*>(mEntityListP.GetItemDataPtr(itemIdx)));

	if(!GetSelectedSceneEntity())
	{
		// Switching from View manipulation mode
		mScenePaletteControllerSP->LockSceneEntitySelection(false);
		SetSelectedSceneEntity(sceneEntityP);
	}
	else
	{
		// Within Entity manipulation mode. Switching from another entity. Preserve entity selection lock status
		const bool	locked = mScenePaletteControllerSP->IsSceneEntitySelectionLocked();

		if(locked)
		{
			mScenePaletteControllerSP->LockSceneEntitySelection(false);

			SetSelectedSceneEntity(sceneEntityP);
				
			mScenePaletteControllerSP->LockSceneEntitySelection(true);
		}
		else
		{
			SetSelectedSceneEntity(sceneEntityP);
		}
	}

	const ROS::ASceneEntity*	selectedSceneEntityP = GetSelectedSceneEntity();

	if(selectedSceneEntityP != sceneEntityP)
	{
		// Selection did not change! Revert selection in GUI
		SetSelectionInEntityList(selectedSceneEntityP);
	}
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnClickPropertiesButton()
{
    mPropertiesAreExpanded = !mPropertiesAreExpanded;
    DisplayCustomControls(mPropertiesAreExpanded);
    UpdatePropertyEditor();
}
//---------------------------------------------------------------------------
ASceneEntityFormFactory<TSceneEntityForm>* TScenePaletteUIForm::GetCompatibleFormType(const ROS::ASceneEntity* kSceneEntityP)
{
    ASceneEntityFormFactory<TSceneEntityForm>*  formFactoryP = NULL;

#ifdef PORTED
    if(dynamic_cast<const ADynamicCamera*>(kSceneEntityP))
    {
		formFactoryP = new SceneEntityFormFactory<TSceneEntityForm, TCameraControlsUIForm>;
    }
    else if(dynamic_cast<const Article*>(kSceneEntityP))
    {
		formFactoryP = new SceneEntityFormFactory<TSceneEntityForm, TArticleControlsUIForm>;
    }
    else if(dynamic_cast<const Actor*>(kSceneEntityP))
    {
		formFactoryP = new SceneEntityFormFactory<TSceneEntityForm, TActorControlsUIForm>;
    }
    else
#endif
	if(dynamic_cast<const ROS::ACompoundSceneEntity*>(kSceneEntityP))
    {
		formFactoryP = new SceneEntityFormFactory<TSceneEntityForm, TAnimatedSceneEntityControlsUIForm>;
    }
	else if(dynamic_cast<const ROS::ACamera*>(kSceneEntityP))
    {
		formFactoryP = new SceneEntityFormFactory<TSceneEntityForm, CameraCustomControlsUI>;
    }
	else if(dynamic_cast<const ROS::ALight*>(kSceneEntityP))
    {
		const ROS::ALight* aLight = dynamic_cast<const ROS::ALight*>(kSceneEntityP);

		if(dynamic_cast<const ROS::AmbientLight*>(aLight))
		{
			formFactoryP = new SceneEntityFormFactory<TSceneEntityForm, AmbientLightCustomControlsUI>;
		}
		else
		{	
			// Distinguish between different versions of ASpotLight

			const ROS::ASpotLight* aSpotLight = dynamic_cast<const ROS::ASpotLight*>(aLight);

			if(aSpotLight)
			{
				if(aSpotLight->GetConstSpotLightStateAccessor()->IsInfinite())
				{
					formFactoryP = new SceneEntityFormFactory<TSceneEntityForm, DirectionalLightCustomControlsUI>;
				}
				else if(aSpotLight->GetConstSpotLightStateAccessor()->GetCutOff() == 180)
				{
					formFactoryP = new SceneEntityFormFactory<TSceneEntityForm, PointLightCustomControlsUI>;
				}
				else
				{
					formFactoryP = new SceneEntityFormFactory<TSceneEntityForm, SpotLightCustomControlsUI>;
				}
			}
		}
    }
#ifdef PORTED
    else if(dynamic_cast<const CompoundSceneEntity*>(kSceneEntityP))
    {
		formFactoryP = new SceneEntityFormFactory<TSceneEntityForm, TCompoundSceneEntityControlsUIForm>;
    }
#endif

    if(IsNull(formFactoryP))
    {
		formFactoryP = new SceneEntityFormFactory<TSceneEntityForm, TMissingControlsUIForm>;
    }

    return formFactoryP;
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::SceneEntityStateChanged()
{
    mScenePaletteControllerSP->SelectedSceneEntityUpdated();
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::SetSelectionInEntityList(const ROS::ASceneEntity* sceneEntity)
{
	const int	count = mEntityListP.GetCount();
	int			idx = -1;
	
	for(int itemIdx = 0; itemIdx < count; ++itemIdx)
	{
		const void*	data = mEntityListP.GetItemDataPtr(itemIdx);
		if(data == sceneEntity)
		{
			idx = itemIdx;
			break;
		}
	}
	
    mEntityListP.SetCurSel(idx);
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::AddSceneObjectBitBtnClick()
{
	CWaitCursor	waitCursor;

	CSelectDBEntityDialog	dbEntityDialog(NULL);

	const int	modalReturn = dbEntityDialog.DoModal();
	
	if(modalReturn == IDCANCEL || modalReturn == -1)
	{
		return;
	}

	waitCursor.Restore();

	const CSelectDBEntityDialog::EntityType	entityType = dbEntityDialog.GetEntityType();
	const ROS::ROSString					entityName = dbEntityDialog.GetEntityName();
	const ROS::StringList					entityDescriptionStrings = dbEntityDialog.GetEntityDescriptionStrings();
	const ROS::ROSString					entityCategory = dbEntityDialog.GetCategoryName();

	try
	{
		switch(entityType)
		{
			case CSelectDBEntityDialog::kDeformable:
				mScenePaletteControllerSP->AddDeformableSceneEntity(entityName, entityCategory, entityDescriptionStrings);
				break;
			case CSelectDBEntityDialog::kCompound:
				mScenePaletteControllerSP->AddCompoundSceneEntity(entityName, entityCategory, entityDescriptionStrings);
				break;
			default:
				MessageBox("The selected entity is of unrecognized type.");
				break;
		}
	}
	catch(std::exception& ex)
	{
		MessageBox(ex.what(), "Error");
	}
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnAddPositionMarkerButton()
{
	CWaitCursor	waitCursor;

    if(IsNotNull(mScenePaletteControllerSP))
    {
		mScenePaletteControllerSP->AddNewPositionMarker();
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnAddCameraButton()
{
	CWaitCursor	waitCursor;

    if(IsNotNull(mScenePaletteControllerSP))
    {
		mScenePaletteControllerSP->AddNewCamera();
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnAddLiveCameraButton()
{
	CWaitCursor	waitCursor;

    if(IsNotNull(mScenePaletteControllerSP))
    {
		mScenePaletteControllerSP->AddNewLiveCamera();
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnAddAmbientLightButton()
{
	CWaitCursor	waitCursor;

    if(IsNotNull(mScenePaletteControllerSP))
    {
		mScenePaletteControllerSP->AddNewAmbientLight();
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnAddDirectionalLightButton()
{
	CWaitCursor	waitCursor;

    if(IsNotNull(mScenePaletteControllerSP))
    {
		mScenePaletteControllerSP->AddNewDirectionalLight();
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnAddPointLightButton()
{
	CWaitCursor	waitCursor;

    if(IsNotNull(mScenePaletteControllerSP))
    {
		mScenePaletteControllerSP->AddNewPointLight();
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnAddSpotLightButton()
{
	CWaitCursor	waitCursor;

    if(IsNotNull(mScenePaletteControllerSP))
    {
		mScenePaletteControllerSP->AddNewSpotLight();
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnLockSelectionCheckBoxClick()
{
	ASSERT(GetSelectedSceneEntity());

    if(IsNotNull(mScenePaletteControllerSP))
    {
		mScenePaletteControllerSP->LockSceneEntitySelection(mLockSelectionCheckBox.GetCheck() == 1);
    }
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnCreateViewButton() 
{	
	CWaitCursor	waitCursor;

	CreateView();
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::CreateView() 
{
	if(IsNotNull(mScenePaletteControllerSP))
    {
		// Calculate the position of the new scene view, making sure it does not overlap this Scene Palette
		CRect	rect, scenePaletteRect;

		GetWindowRect(&scenePaletteRect);

		try
		{
			mScenePaletteControllerSP->AddSceneView(scenePaletteRect.Width() + kWindowOffset, scenePaletteRect.Height() + kWindowOffset, m_hWnd);
		}
		catch(const std::exception& ex)
		{	
			::MessageBox(NULL, ex.what(), "Error", MB_OK);
		}
    }
}
//---------------------------------------------------------------------------
LRESULT TScenePaletteUIForm::OnKickIdle(WPARAM, LPARAM lCount)
{
	IdleNotification::Notify();
	
	return 1;
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnRenderSelectionCheck() 
{
	ROS::ASceneEntity*   sceneEntity = GetSelectedSceneEntity();

	ASSERT(sceneEntity);

	sceneEntity->GetSceneEntityStateAccessor()->SetVisible(mRenderSelectionCheckBox.GetCheck() == 1);

    mScenePaletteControllerSP->SelectedSceneEntityUpdated();
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	if(&mEntityListP == pWnd)
	{
		CPoint	listOrigin(0, 0);
	
		mEntityListP.ClientToScreen(&listOrigin);

		const CPoint	listPoint(point - listOrigin);
		BOOL			outside;

		const int	itemIdx = mEntityListP.ItemFromPoint(listPoint, outside);

		if(!outside)
		{
			AttemptEntityListSelection(itemIdx);
			
			if(itemIdx >= 0)
			{
				const ROS::ASceneEntity*			sceneEntity = GetSelectedSceneEntity();
				const ROS::DeformableSceneEntity*	deformable = dynamic_cast<const ROS::DeformableSceneEntity*>(sceneEntity);
				const int							popupMenuIndex = deformable ? kDeformableSceneEntityPopupMenuIndex : kSceneEntityPopupMenuIndex;
				CMenu*								popupMenu = mPopupMenus.GetSubMenu(popupMenuIndex);

				if(popupMenu)
				{
					::TrackPopupMenu(popupMenu->m_hMenu, TPM_LEFTALIGN, point.x, point.y, 0, m_hWnd, NULL);	
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnDeleteClick() 
{
	CWaitCursor	waitCursor;

	ROS::ASceneEntity*	sceneEntityP = GetSelectedSceneEntity();

	if(sceneEntityP)
	{
		mScenePaletteControllerSP->RemoveSceneEntity(*sceneEntityP);

		sceneEntityP->Delete();

		mScenePaletteControllerSP->SceneUpdated();
	}
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnReplaceClick()
{
	CWaitCursor	waitCursor;

	ROS::ASceneEntity*			sceneEntity = GetSelectedSceneEntity();
	ROS::DeformableSceneEntity*	deformableEntity = dynamic_cast<ROS::DeformableSceneEntity*>(sceneEntity);

	if(deformableEntity)
	{
		// Select new entity and ask existing entity to replace.
		CSelectDBEntityDialog					dbEntityDialog(NULL);
		CSelectDBEntityDialog::EntityTypeList	entityTypes;

		entityTypes.push_back(CSelectDBEntityDialog::kDeformable);

		dbEntityDialog.SetEntityTypes(entityTypes);
		
		const int	modalReturn = dbEntityDialog.DoModal();
		
		if(modalReturn == IDOK)
		{
			waitCursor.Restore();

			const CSelectDBEntityDialog::EntityType	entityType = dbEntityDialog.GetEntityType();
			ASSERT(entityType == CSelectDBEntityDialog::kDeformable);
			const ROS::ROSString					entityName = dbEntityDialog.GetEntityName();
			const ROS::StringList					entityDescriptionStrings = dbEntityDialog.GetEntityDescriptionStrings();
			const ROS::ROSString					entityCategory = dbEntityDialog.GetCategoryName();

			try
			{
				deformableEntity->Replace(entityName, entityCategory, entityDescriptionStrings);

				mScenePaletteControllerSP->SelectedSceneEntityUpdated();
			}
			catch(std::exception& ex)
			{
				MessageBox(ex.what(), "Error");			
			}
		}
	}
}
//---------------------------------------------------------------------------
BOOL TScenePaletteUIForm::OnToolTipTextNotify(UINT id, NMHDR * pTTTStruct, LRESULT * pResult)
{	
	// *** I removed this because there was a problem with the tool tips.
	// *** I will find the problem later. -TNB
	return FALSE;

	BOOL			tipFound = FALSE;
	TOOLTIPTEXT*	toolTipText = (TOOLTIPTEXT*)pTTTStruct;

	int	controlID;

	if(toolTipText->uFlags & TTF_IDISHWND)
	{
		// idFrom is the HWND of the tool
		HWND	ctrlWndH = (HWND)pTTTStruct->idFrom;
		
		controlID = ::GetDlgCtrlID(ctrlWndH);
	}
	else
	{
		// idFrom is the id of the tool
		controlID = pTTTStruct->idFrom;
	}
	
	LoadString(AfxGetInstanceHandle(), controlID, toolTipText->szText, 80);
	toolTipText->lpszText = toolTipText->szText;
  	toolTipText->hinst = NULL;

	if(controlID == IDC_UNDO_OPERATION)
	{
		const ROS::ROSString name = mScenePaletteControllerSP->GetUndoOperationName();

		strcat(toolTipText->szText, " ");
		strcat(toolTipText->szText, name.c_str());
	}
	else if(controlID == IDC_REDO_OPERATION)
	{
		const ROS::ROSString name = mScenePaletteControllerSP->GetRedoOperationName();

		strcat(toolTipText->szText, " ");
		strcat(toolTipText->szText, name.c_str());
	}

	return TRUE;
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnCancel()
{
}
//---------------------------------------------------------------------------
void TScenePaletteUIForm::OnOK()
{
}
//---------------------------------------------------------------------------
const ROS::DACompoundObject* TScenePaletteUIForm::CreateCompoundObject(const ROS::ROSString& sceneEntityName, const ROS::StringList& descriptionStrings) const
{
	ASSERT(descriptionStrings.GetStringCount() == 1);

	ROS::StringList	fullDescription;
		
	fullDescription.Add(mUserPreferences.GetDataPath() + descriptionStrings.GetString(DBE::kCompoundFilename));
    
	return CompoundObjectCreate(fullDescription);
}
// --------------------------------------------------------------------------
const ROS::DADeformableObject* TScenePaletteUIForm::CreateDeformableObject(const ROS::ROSString& sceneEntityName, const ROS::StringList& descriptionStrings) const
{
	const unsigned int	stringCount = descriptionStrings.GetStringCount();

	ASSERT((stringCount % 4) == 0);

	ROS::StringList			fullDescription;
	const ROS::ROSString	dataPath = mUserPreferences.GetDataPath();
		
	for(unsigned int stringIdx = 0; stringIdx < stringCount; stringIdx += 4)
	{
		fullDescription.Add(descriptionStrings.GetString(stringIdx + DBE::kDeformableEntityName));					// Part name
		fullDescription.Add(dataPath + descriptionStrings.GetString(stringIdx + DBE::kDeformableMeshFilename));	// Mesh filename
		fullDescription.Add(dataPath + descriptionStrings.GetString(stringIdx + DBE::kDeformableSkeletonPath));	// Skeleton path
		fullDescription.Add(dataPath + descriptionStrings.GetString(stringIdx + DBE::kDeformableAnimationFilename));	// Animation filename
	}
    
	return DeformableObjectCreate(fullDescription, EventHandlerFunction);
}
// --------------------------------------------------------------------------
void TScenePaletteUIForm::DestroyDeformableObject(const ROS::DADeformableObject* entity, const ROS::StringList& descriptionStrings) const
{
	const unsigned int	stringCount = descriptionStrings.GetStringCount();

	ASSERT((stringCount % 4) == 0);

	ROS::StringList			fullDescription;
	const ROS::ROSString	dataPath = mUserPreferences.GetDataPath();

	for(unsigned int stringIdx = 0; stringIdx < stringCount; stringIdx += 4)
	{
		fullDescription.Add(descriptionStrings.GetString(stringIdx + DBE::kDeformableEntityName));					// Part name
		fullDescription.Add(dataPath + descriptionStrings.GetString(stringIdx + DBE::kDeformableMeshFilename));	// Mesh filename
		fullDescription.Add(dataPath + descriptionStrings.GetString(stringIdx + DBE::kDeformableSkeletonPath));	// Skeleton path
		fullDescription.Add(dataPath + descriptionStrings.GetString(stringIdx + DBE::kDeformableAnimationFilename));	// Animation filename
	}
    
	DeformableObjectDestroy(entity, fullDescription);
}
// --------------------------------------------------------------------------
const ROS::DAAudioObject* TScenePaletteUIForm::CreateAudioObject(const ROS::ROSString& soundName, const ROS::AStaticSceneEntity* sourceEntity) const
{
	ROS::ROSString		entityCategory;
	ROS::StringList		fullDescription;

	fullDescription.Resize(DBE::kAudioStringsPerEntity);

	DBE::get_entity_strings(soundName, entityCategory, fullDescription);

	const ROS::ROSString	dataPath = mUserPreferences.GetDataPath();
	const ROS::ROSString	fileNameStr = dataPath + fullDescription.GetString(DBE::kAudioFilename);
	const ROS::ROSString	attenuationStr = fullDescription.GetString(DBE::kAudioAttenuation);
	const ROS::ROSString	minDistanceStr = fullDescription.GetString(DBE::kAudioMinDistance);
	const ROS::ROSString	maxDistanceStr = fullDescription.GetString(DBE::kAudioMaxDistance);

	float	attenuation, minDistance, maxDistance;

	sscanf(attenuationStr.c_str(), "%f", &attenuation);
	sscanf(minDistanceStr.c_str(), "%f", &minDistance);
	sscanf(maxDistanceStr.c_str(), "%f", &maxDistance);

	return AudioObjectCreate(fileNameStr.c_str(), attenuation, minDistance, maxDistance, sourceEntity);
}
// --------------------------------------------------------------------------
void TScenePaletteUIForm::PlayAudioObject(const ROS::DAAudioObject* audioObj, float startTimePoint) const
{
	ASSERT(audioObj);

	AudioObjectPlay(audioObj, startTimePoint);
}
// --------------------------------------------------------------------------
void TScenePaletteUIForm::OnUndo()
{
	CWaitCursor	waitCursor;

	mScenePaletteControllerSP->UndoLastOperation();
}
// --------------------------------------------------------------------------
void TScenePaletteUIForm::OnRedo() 
{
	CWaitCursor	waitCursor;

	mScenePaletteControllerSP->RedoLastOperation();
}
// --------------------------------------------------------------------------
void TScenePaletteUIForm::OnWriteEntityPositionsFile()
{
	try
	{
		CWaitCursor	waitCursor;

		ROS::ROSString	fileName = mScenePaletteControllerSP->GetSceneFileName() + ROS::ROSString(" - Entity Positions.txt");

		std::ofstream	oFStream(fileName.c_str(), std::ios_base::out);

		oFStream << "Scene Filename: " << mScenePaletteControllerSP->GetSceneFileName().c_str() << std::endl;
		oFStream << "Scene Name: " << mScenePaletteControllerSP->GetSceneName().c_str() << std::endl;
		oFStream << "Current Scene Time: " << mScenePaletteControllerSP->GetCurrentSceneTime().GetTime();

		oFStream << std::endl;

		ROS::SceneEntityCollection	sceneEntityColl;

		mScenePaletteControllerSP->GetSceneEntities(sceneEntityColl);

		ROS::SceneEntityCollection::const_iterator   begin = sceneEntityColl.begin();
		const ROS::SceneEntityCollection::const_iterator kEnd = sceneEntityColl.end();

		while(begin != kEnd)
		{
			const ROS::ASceneEntity*	entity = (*begin);

			if(entity->IsPersistent())
			{
				const ROS::AStaticSceneEntity*	sEntity = dynamic_cast<const ROS::AStaticSceneEntity*>(entity);

				if(sEntity)
				{
					oFStream << std::endl << std::endl << sEntity->GetConstSceneEntityStateAccessor()->GetName().c_str();

					// Write location
					const ROS::Location	location = sEntity->GetConstStaticsStateAccessor()->GetLocation();
					
					oFStream << std::endl << "Location";
					oFStream << std::endl << location.GetX() << ", " << location.GetY() << ", " << location.GetZ();

					// Write location
					const ROS::Orientation	orientation = sEntity->GetConstStaticsStateAccessor()->GetOrientation();
					
					oFStream << std::endl << "Orientation";

					for(unsigned int i = 0; i < 3; ++i)
					{
						oFStream << std::endl;

						for(unsigned int j = 0; j < 2; ++j)
						{						 
							oFStream << orientation.Get(i, j)  << ", ";
						}

						oFStream << orientation.Get(i, j);
					}
				}
			}

			++begin;
		}
	}
	catch(...)
	{
		MessageBox("Failed to write Entity position listing", "Error");
	}
}
// --------------------------------------------------------------------------
void TScenePaletteUIForm::OnMove(int x, int y) 
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
			RegSetValueEx (hEdKey, REG_SCENE_X_POS, NULL, REG_DWORD, (BYTE *) &xPos, valSize);
			RegSetValueEx (hEdKey, REG_SCENE_Y_POS, NULL, REG_DWORD, (BYTE *) &yPos, valSize);
			RegCloseKey (hEdKey);
		}
	}
}
