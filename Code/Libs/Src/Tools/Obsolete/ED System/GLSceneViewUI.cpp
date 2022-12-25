// Author: Shaival Varma
// --------------------------------------------------------------------------
#pragma warning( disable : 4786 )

#include "PCH.h"
#include <memory>
#include <afxdlgs.h>
#include <vfw.h>

#include "GLSceneViewUI.h"
#include "DARenderPipeline.h"
#include "CodeMsg.h"
#include "DABaseCamera.h"
#include "Vector.h"
#include "GLUtils.h"
#include "Utils.h"
#include "AStaticSceneEntity.h"
#include "SceneView.h"
#include "Matrix4x4.h"
#include "CompoundSceneEntity.h"
#include "DeformableSceneEntity.h"
#include "PositionMarker.h"
#include "AudioStateAccessor.h"
#include "MotionStateAccessor.h"
#include "Camera.h"
#include "ConstSceneEntityStateAccessor.h"
#include "ConstStaticsStateAccessor.h"
#include "StaticsStateAccessor.h"
#include "ConstCompoundStateAccessor.h"
#include "CompoundStateAccessor.h"
#include "ConstMotionStateAccessor.h"
#include "AudioStateAccessor.h"
#include "resource.h"
#include "IntersectInfo.h"
#include "Char.h"
#include "BaseCam.h"
#include "StaticEntityOperations.h"
#include "Matrix4.h"
#include "FDump.h"
#include "DBExtension.h"
#include "SelectDBEntityDialog.h"
#include "ASoundListener.h"
#include "DAAudioObject.h"
#include "HardPoint.h"
#include "EntityBrowser.h"
#include "edutil.h"

//
// Registry values
//

const char *REG_VIEW_X_POS = "ViewXPos";
const char *REG_VIEW_Y_POS = "ViewYPos";

//---------------------------------------------------------------------------
class SceneViewSoundListener: public ASoundListener
{
	public:
		SceneViewSoundListener(const TGLSceneViewUIForm& sceneView)
		: mSceneView(sceneView)
		{
		}

		virtual ROS::Location GetLocation() const
		{
			Vector	location;
			
			CameraGetPosition(mSceneView.mCamera, location);

			return ROS::Location(location.x, location.y, location.z);
		}

		virtual ROS::Orientation GetOrientation() const
		{
    		ROS::Orientation	orientation;

			CameraGetOrientation(mSceneView.mCamera, orientation);

			return ROS::Orientation(orientation.GetI(), orientation.GetJ(), orientation.GetK());
		}

	private:
		const TGLSceneViewUIForm&	mSceneView;
};
//---------------------------------------------------------------------------
const float gClearRed = 0.0;
const float gClearGreen = 0.5;
const float gClearBlue = 0.5;

const ROS::ROSString	gWindowTitle("Scene View");
const ROS::ROSString	gConnected("Connected to");
const ROS::ROSString	gSeparator(": ");
const ROS::ROSString	gSpace(" ");

const float gRotationFactor = 0.25;

const unsigned int		gMaxCommandID = 0xDFFF;

const int PIPE_WIDTH = 640;
const int PIPE_HEIGHT = 480;

const UINT XLAT_TIMER = 16;  // An homage to TZ's magic number

// Main popups
enum PopupIndex
{
	kSceneViewPopupMenuIndex,
	kSceneEntityPopupMenuIndex,
	kCompoundPopupMenuIndex,
	kDeformablePopupMenuIndex,
	kPositionMarkerPopupMenuIndex
};

// Scene View popup menu items
const int kSceneViewSelectCameraIndex = 3;	// This should correspond to the index of the position of the 
											// "Select Camera" menu item in the "SceneView" menu in the resource file.

// Compound Entity popup menu items
const int kCompoundEntitySelectHardpointToRememberIndex = 10;	// This should correspond to the index of the position of the 
																// "Remember Hardpoint" menu item in the "Compound" menu in the resource file.
const int kCompoundEntitySelectHardpointToMoveIndex = 12;	// This should correspond to the index of the position of the 
															// "Move To Remembered Hardpoint" menu item in the "Compound" menu in the resource file.
const int kCompoundEntitySelectHardpointToMakeChildIndex = 14;	// This should correspond to the index of the position of the 
															// "Make Child Of Remembered Hardpoint" menu item in the "Compound" menu in the resource file.
const int kCompoundEntitySetCameraPositionIndex = 16;	// This should correspond to the index of the position of the 
														// "Set View Position" menu item in the "Compound" menu in the resource file.

// Deformable Entity popup menu items
const int kDeformableEntitySelectHardpointToRememberIndex = 11;	// This should correspond to the index of the position of the 
																// "Remember Hardpoint" menu item in the "Deformable" menu in the resource file.
const int kDeformableEntitySelectHardpointToMoveIndex = 13;	// This should correspond to the index of the position of the 
															// "Move To Remembered Hardpoint" menu item in the "Deformable" menu in the resource file.
const int kDeformableEntitySelectHardpointToMakeChildIndex = 15;	// This should correspond to the index of the position of the 
																	// "Make Child Of Remembered Hardpoint" menu item in the "Deformable" menu in the resource file.

//
// Static data members
//

BOOL TGLSceneViewUIForm::mPipeInitialized = FALSE;
int  TGLSceneViewUIForm::mPipeWidth = PIPE_WIDTH;
int  TGLSceneViewUIForm::mPipeHeight = PIPE_HEIGHT;
TGLSceneViewUIForm * TGLSceneViewUIForm::mBufferOwner = NULL;

//
// Constants
//

// Custom System Menu Messages
// NOTE: These must all be less than 0xF000
const WPARAM CSC_PROPERTIES = 0x0100;

//
// Local routines
//
static DA_ERROR_HANDLER	gOriginalErrorHandler = NULL;

int __cdecl TemporaryErrorHandler(ErrorCode code, const C8 *fmt, ...)
{
	// Report the error
	// WARNING: This uses a fixed size buffer.
	char buffer[4096];
	va_list args;
	va_start (args, fmt);
	wvsprintf (buffer, fmt, args);
	va_end (args);
	OutputDebugString (buffer);

	// NOTE: Newlines are already added to trace severity.
	if (code.severity < SEV_TRACE_1)
	{
		OutputDebugString ("\n");
	}

	return 0;
}

static Transform removeRoll (Transform &tr, Vector &trans)
{
	// Remove any roll component from the transformation by transforming the
	// Z vector from new view to world space, crossing with the up vector to get an X axis,
	// then crossing those two to get a Y axis.
	Vector jWorldAxis(0,1,0);
	Vector kWorldAxis = tr * Vector(0,0,1) - tr * Vector(0,0,0);
	Vector iWorldAxis = cross_product (jWorldAxis, kWorldAxis);
	if (iWorldAxis.magnitude() < 0.00001)
	{
		// To prevent badness as k approaches j, cross k and i
		jWorldAxis = cross_product(kWorldAxis, Vector(1,0,0));
		iWorldAxis = cross_product(jWorldAxis, kWorldAxis);
	}
	else
	{
		// Now calculate the new j;
		jWorldAxis = cross_product(kWorldAxis, iWorldAxis);
	}
	// Normalize the vectors and use them to build the transform
	iWorldAxis.normalize();
	jWorldAxis.normalize();
	kWorldAxis.normalize();
	return Transform(Matrix(iWorldAxis, jWorldAxis, kWorldAxis), trans);
}

//
// Routines
//

// Static methods
void TGLSceneViewUIForm::InitPipeline(HWND hMainWnd)
{
	if (!mPipeInitialized)
	{
		GENRESULT genResult;

		// Make the buffer the size of the display.
		mPipeWidth = GetSystemMetrics (SM_CXSCREEN);
		mPipeHeight = GetSystemMetrics (SM_CYSCREEN);

		gOriginalErrorHandler = FDUMP;
 		FDUMP = TemporaryErrorHandler;

		genResult = PIPE->create_buffers(hMainWnd, mPipeWidth, mPipeHeight);

		if(genResult != GR_OK)
		{
			// Retry with default settings
			mPipeWidth = PIPE_WIDTH;
			mPipeHeight = PIPE_HEIGHT;

			genResult = PIPE->create_buffers(hMainWnd, mPipeWidth, mPipeHeight);

			FDUMP = gOriginalErrorHandler;

			if(genResult != GR_OK)
			{
				throw ExRenderingBufferCreationFailed();
			}
		}
		else
		{
			FDUMP = gOriginalErrorHandler;
		}

		genResult = PIPE->set_pipeline_state(RP_CLEAR_COLOR, D3DRGB(gClearRed, gClearGreen, gClearBlue));
		ASSERT(genResult == GR_OK);

		genResult = PIPE->set_pipeline_state(RP_TEXTURE, TRUE);
		ASSERT(genResult == GR_OK);

		PIPE->set_pipeline_state(RP_BATCH_LAZY_STATE, FALSE);
		ASSERT(genResult == GR_OK);
		//genResult = PIPE->set_pipeline_state(RP_BATCH_TRANSLUCENT_POOL, XXXX );
		//ASSERT(genResult == GR_OK);
		genResult = PIPE->set_pipeline_state(RP_BATCH_TRANSLUCENT_MODE, RP_TRANSLUCENT_DEPTH_SORTED);
		ASSERT(genResult == GR_OK);
		genResult = PIPE->set_pipeline_state(RP_BATCH_POOLS, RP_TRANSLUCENT_DEPTH_SORTED);
		ASSERT(genResult == GR_OK);

		genResult = PIPE->set_pipeline_state(RP_BATCH, TRUE);
		ASSERT(genResult == GR_OK);
		
		mPipeInitialized = TRUE;
	}
}

// Methods

const float	gkCameraTranslationFactor = (float)0.1;
//---------------------------------------------------------------------------
TGLSceneViewUIForm::TGLSceneViewUIForm(SceneView& associatedSceneView, int top, int left, int height, int width, HWND parentWndH)
: mAssociatedSceneView(associatedSceneView), mSoundListener(NULL), mHeight(height), mWidth(width), mCurrentMode(neutralMode), mLastMouseX(0), mLastMouseY(0),
  mWndH(NULL), mCamera(NULL), mAssociatedCamera(NULL), mShowMesh(false), mShowAxes(true), mRenderTextured(true), mMinCommandID(gMaxCommandID),
  mCommandType(kCamera), mIsControlKeyDown(false)
{
	mShowLetterbox = false;
	mLetterboxHToV = 1.85;
	mSoundListener = std::auto_ptr<ASoundListener>(new SceneViewSoundListener(*this));

	mPopupMenus.LoadMenu(IDR_SCENE_VIEW_POPUP_MENU);
	// Scene View popup menu
	ASSERT(ID_SCENE_VIEW_SELECT_CAMERA == mPopupMenus.GetSubMenu(kSceneViewPopupMenuIndex)->GetMenuItemID(kSceneViewSelectCameraIndex));	// The rest of the code assumes this index
	// Compound Entity popup menu items
	ASSERT(ID_COMPOUND_ENTITY_COPY_HARDPOINT_POSITION == mPopupMenus.GetSubMenu(kCompoundPopupMenuIndex)->GetMenuItemID(kCompoundEntitySelectHardpointToRememberIndex));	// The rest of the code assumes this index
	ASSERT(ID_COMPOUND_ENTITY_PASTE_POSITION == mPopupMenus.GetSubMenu(kCompoundPopupMenuIndex)->GetMenuItemID(kCompoundEntitySelectHardpointToMoveIndex));	// The rest of the code assumes this index
	ASSERT(ID_COMPOUND_ENTITY_MAKE_CHILD == mPopupMenus.GetSubMenu(kCompoundPopupMenuIndex)->GetMenuItemID(kCompoundEntitySelectHardpointToMakeChildIndex));	// The rest of the code assumes this index	
	ASSERT(ID_COMPOUND_ENTITY_SET_CAMERA == mPopupMenus.GetSubMenu(kCompoundPopupMenuIndex)->GetMenuItemID(kCompoundEntitySetCameraPositionIndex));	// The rest of the code assumes this index
	// Deformable Entity popup menu items
	ASSERT(ID_DEFORMABLE_ENTITY_COPY_HARDPOINT_POSITION == mPopupMenus.GetSubMenu(kDeformablePopupMenuIndex)->GetMenuItemID(kDeformableEntitySelectHardpointToRememberIndex));	// The rest of the code assumes this index	
	ASSERT(ID_DEFORMABLE_ENTITY_PASTE_POSITION == mPopupMenus.GetSubMenu(kDeformablePopupMenuIndex)->GetMenuItemID(kDeformableEntitySelectHardpointToMoveIndex));	// The rest of the code assumes this index	
	ASSERT(ID_DEFORMABLE_ENTITY_MAKE_CHILD == mPopupMenus.GetSubMenu(kDeformablePopupMenuIndex)->GetMenuItemID(kDeformableEntitySelectHardpointToMakeChildIndex));	// The rest of the code assumes this index	

	CreateOpenGLPopupMenu();	/*******NOTE: THIS COULD FAIL**********/
	CreateSceneEntityPopupMenu();	/*******NOTE: THIS COULD FAIL**********/
	CreateCompoundSceneEntityPopupMenu();	/*******NOTE: THIS COULD FAIL**********/
	CreateDeformableSceneEntityPopupMenu();	/*******NOTE: THIS COULD FAIL**********/
	CreatePositionMarkerPopupMenu();	/*******NOTE: THIS COULD FAIL**********/
#ifdef PORTED
    CreateHint();
#endif

	mIsActive = FALSE;

    // Create Camera
    mCamera = CameraCreate(mWidth, mHeight);

    if(mCamera)
    {
		Vector	position(0, 0, 40);
		CameraSetPosition(mCamera, position);
    }
    else
    {
		if(mBufferOwner == this)
		{
			PIPE->destroy_buffers();
			mBufferOwner = NULL;
		}

		throw ExCameraCreationFailed();
    }

	SetAssociatedCameraToInternalCamera();

	CreateAppWindow(AfxGetInstanceHandle(), gWindowTitle.c_str(), parentWndH);

	SetStyleInWindow(left, top, mWidth, mHeight);

	// Create a copy of the standard system menu and attach it to our window.
	// Then add commands to it as needed.
	mSysMenu = GetSystemMenu (mWndH, FALSE);
	MENUITEMINFO mi;
	memset(&mi, 0, sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_TYPE;
	mi.fType = MFT_SEPARATOR;
	InsertMenuItem (mSysMenu, -1, TRUE, &mi);
	memset(&mi, 0, sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_ID | MIIM_TYPE;
	mi.dwTypeData = "Properties...";
	mi.fType = MFT_STRING;
	mi.wID = CSC_PROPERTIES;
	InsertMenuItem (mSysMenu, -1, TRUE, &mi);

	// Initialize the pipeline if it has not already been initialized
	try
	{
		InitPipeline(NULL);
	}
	catch(...)
	{
		DestroyWindow(mWndH);
		CameraDestroy(mCamera);

		throw;
	}

	// Set up some general rendering stuff.
	ShowWindow(mWndH, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
TGLSceneViewUIForm::~TGLSceneViewUIForm()
{
	mAssociatedSceneView.UIClosed();

	ASSERT(PIPE);

	if (mBufferOwner == this)
	{
		PIPE->destroy_buffers();
		mBufferOwner = NULL;
	}

    if(mCamera)
    {
		CameraDestroy(mCamera);
    }
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::AssociateCamera(ROS::ADynamicCamera* camera)
{
	if(camera != mAssociatedCamera)
    {
		// We have a new camera
		mAssociatedCamera = camera;

		if(IsNotNull(mAssociatedCamera))			
		{
			SetInternalCameraToAssociatedCamera();

			const ROS::ROSString	title = gWindowTitle + gSeparator + gConnected + gSpace + mAssociatedCamera->GetConstSceneEntityStateAccessor()->GetName();

			SetWindowText(mWndH, title.c_str());

			Repaint();
		}
		else
		{
			SetWindowText(mWndH, gWindowTitle.c_str());
		}
    }
}
//---------------------------------------------------------------------------
const ROS::ADynamicCamera* TGLSceneViewUIForm::GetAssociatedCamera() const
{
	return mAssociatedCamera;
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::AssociatedCameraUpdated()
{
    SetInternalCameraToAssociatedCamera();

    Repaint();
}
//---------------------------------------------------------------------------
void StoreForm(HWND wndH, TGLSceneViewUIForm* view)
{
	SetWindowLong(wndH, GWL_USERDATA, reinterpret_cast<LONG>(view));
}
//---------------------------------------------------------------------------
TGLSceneViewUIForm* RetrieveForm(HWND wndH)
{
	LONG data = GetWindowLong(wndH, GWL_USERDATA);

    return reinterpret_cast<TGLSceneViewUIForm*>(data);
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::StoreForm()
{
	::StoreForm(mWndH, this);
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::ProcessCommand(int notifyCode, int iD, HWND ctrlWndH, TGLSceneViewUIForm* sceneView)
{
	switch(iD)
	{
		case ID_SCENE_VIEW_MESH:
			sceneView->MeshClick();
			break;

		case ID_SCENE_VIEW_AXES:
			sceneView->AxesClick();
			break;

		case ID_SCENEVIEW_SHOWLETTERBOX:
			sceneView->ShowLetterboxClick ();
			break;

		case ID_SCENE_VIEW_RENDER_TEXTURED:
			sceneView->RenderTexturedClick();
			break;

		case ID_SCENEVIEW_SELECTION_XY_PLANE:
			sceneView->SelectionXYPlaneClick();
			break;

		case ID_SCENEVIEW_SELECTION_YZ_PLANE:
			sceneView->SelectionYZPlaneClick();
			break;

		case ID_SCENEVIEW_SELECTION_ZX_PLANE:
			sceneView->SelectionZXPlaneClick();
			break;

		case ID_SCENEVIEW_XY_PLANE:
			sceneView->XYPlaneClick();
			break;

		case ID_SCENEVIEW_YZ_PLANE:
			sceneView->YZPlaneClick();
			break;

		case ID_SCENEVIEW_ZX_PLANE:
			sceneView->ZXPlaneClick();
			break;

		case ID_SCENE_ENTITY_PROPERTIES:
			break;

		case ID_COMPOUND_ENTITY_SHOW_HARDPOINTS:
		case ID_DEFORMABLE_ENTITY_SHOW_HARDPOINTS:
			sceneView->HardPointsClick();
			break;

		case ID_DEFORMABLE_ENTITY_SHOW_SKELETON:
			sceneView->SkeletonClick();
			break;

		case ID_COMPOUND_ENTITY_SHOW_MOTION_PATH:
		case ID_SCENE_ENTITY_SHOW_MOTION_PATH:
			sceneView->MotionPathClick();
			break;

		case ID_COMPOUND_ENTITY_PROPERTIES:
			break;

		case ID_DEFORMABLE_ENTITY_START_MOTION:
			sceneView->StartClick();
			break;

		case ID_DEFORMABLE_ENTITY_LOOP_MOTION:
			sceneView->LoopClick();
			break;

		case ID_DEFORMABLE_ENTITY_PAUSE_MOTION:
			sceneView->PauseClick();
			break;

		case ID_DEFORMABLE_ENTITY_STOP_MOTION:
			sceneView->StopClick();
			break;

		case ID_COMPOUND_ENTITY_PLAY_SOUND:
		case ID_DEFORMABLE_ENTITY_PLAY_SOUND:
		case ID_POSITION_MARKER_ENTITY_PLAY_SOUND:
			sceneView->PlaySoundClick();
			break;

		case ID_COMPOUND_ENTITY_PASTE_POSITION:
		case ID_DEFORMABLE_ENTITY_PASTE_POSITION:
			sceneView->SetToRememberedPositionClick();
			break;

		case ID_DEFORMABLE_ENTITY_PROPERTIES:
			break;

		case ID_SELECT_IK_TARGET:
			sceneView->SetIKTarget();
			break;

		default:
			// Check for camera commands
			if(sceneView->mMinCommandID <= iD && iD <= gMaxCommandID)
			{
				if(sceneView->mCommandType == kCamera)
				{
					sceneView->AssociateCameraClick(gMaxCommandID - iD);
				}
				else if(sceneView->mCommandType == kCompound || sceneView->mCommandType == kDeformable)
				{
					sceneView->SelectCompoundPopupClick(gMaxCommandID - iD);
				}

				break;
			}
			
			ASSERT(0); // Unknown id
	}


}
//---------------------------------------------------------------------------
long WINAPI TGLSceneViewUIForm::WndProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    TMouseButton button;
    TShiftState shift = 0;
	TGLSceneViewUIForm* form = RetrieveForm(hWindow);	// Will be NULL for WM_CREATE
    TRACKMOUSEEVENT trackMouseEvent;
	WORD notifyCode, iD;
	HWND ctrlWndH;

#if 0
    form->OpenGLUpdateHint(message);
#endif

	switch (message)
	{	
		case WM_ACTIVATE:
			{
				WORD fActive = LOWORD(wParam);           // activation flag 
				BOOL fMinimized = (BOOL) HIWORD(wParam); // minimized flag 
				HWND hwndPrevious = (HWND) lParam;       // window handle

				if (form)
				{
					if (fActive == WA_INACTIVE)
					{
						form->mIsActive = FALSE;
					}
					else
					{
						form->mIsActive = TRUE;

						AudioObjectListenerSet(form->mSoundListener.get());
					}
		
					// Don't indicate that we processed this message, so that we
					// get the default behaviour
				}
			}
			break;

		case WM_SIZE:
			if (form && form->mIsActive)
			{
				RECT r;
				GetClientRect (hWindow, &r);
				form->Resize(r.right, r.bottom);
			}
			break;

		case WM_PAINT:
			{
				RECT r;
				if (form)
				{
					if (form->mIsActive || GetUpdateRect(hWindow, &r, FALSE))
					{
						ASSERT(form && PIPE);

						RECT rc;
						GetClientRect (hWindow, &rc);

						// Set this as the render window, and render into the entire client rectangle
						PIPE->set_window (hWindow, 0, 0, rc.right, rc.bottom);
						if (rc.bottom <= mPipeHeight && rc.right <= mPipeWidth)
						{
							PIPE->set_viewport (0, 0, rc.right, rc.bottom);
						}
						else
						{
							PIPE->set_viewport (0, 0, mPipeWidth, mPipeHeight);
						}

						PIPE->begin_scene();

						PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL);

						form->PaintScene();

						PIPE->flush(RP_TRANSLUCENT_DEPTH_SORTED_ONLY);

						PIPE->end_scene();
					}
					else
					{
						// No paint this time.
						break;
					}
					
					// Do the right BeginPaint/EndPaint stuff to prevent infinite redrawing, swapping
					// buffers in the middle to transfer the contents of the back buffer to the
					// primary surface.
					{
						PAINTSTRUCT ps;
						HDC paintDC = BeginPaint(hWindow, &ps);

						//
						// Slap the 3D scene into the view window
						//
						PIPE->swap_buffers();

						//
						// Now perform 2D painting operations on top of the view.
						//

						form->OverlayPaint (paintDC);

						// We are done painting.
						EndPaint (hWindow, &ps);
					}
				}
			}
			break;

    	case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            ASSERT(form);
			
			if(wParam & MK_LBUTTON)
            {
				button = mbLeft;
                shift |= ssLeft;
            }
            else if(wParam & MK_RBUTTON)
            {
				button = mbRight;
                shift |= ssRight;
            }
            if(wParam & MK_SHIFT)
            {
				shift |= ssShift;
            }
            if(wParam & MK_CONTROL)
            {
				shift |= ssCtrl;
            }
			if (GetAsyncKeyState (VK_MENU) & 0x8000)
			{
				shift |= ssAlt;
			}

		    form->OpenGLMouseDown(button, shift, LOWORD(lParam), HIWORD(lParam));

			break;

		case WM_MOUSEMOVE:
            ASSERT(form);

			if(wParam & MK_LBUTTON)
            {
				shift |= ssLeft;
            }
            else if(wParam & MK_RBUTTON)
            {
				shift |= ssRight;
            }
            if(wParam & MK_SHIFT)
            {
				shift |= ssShift;
            }
            if(wParam & MK_CONTROL)
            {
				shift |= ssCtrl;
            }
			if (GetAsyncKeyState (VK_MENU) & 0x8000)
			{
				shift |= ssAlt;
			}

            form->OpenGLMouseMove(shift, (lParam << 16) >> 16, lParam >> 16);   // Not using LOWORD and HIWORD because the sign needs to be preserved

            trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
            trackMouseEvent.dwFlags = TME_LEAVE;
            trackMouseEvent.hwndTrack = hWindow;
            trackMouseEvent.dwHoverTime = HOVER_DEFAULT;

#if 0
// will be available under win'98
            TrackMouseEvent(&trackMouseEvent);
#endif

            break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            ASSERT(form);

            if(wParam & MK_LBUTTON)
            {
				button = mbLeft;
                shift |= ssLeft;
            }
            else if(wParam & MK_RBUTTON)
            {
				button = mbRight;
                shift |= ssRight;
            }
            if(wParam & MK_SHIFT)
            {
				shift |= ssShift;
            }
            if(wParam & MK_CONTROL)
            {
				shift |= ssCtrl;
            }
			if (GetAsyncKeyState (VK_MENU) & 0x8000)
			{
				shift |= ssAlt;
			}

	       	form->OpenGLMouseUp(button, shift, LOWORD(lParam), HIWORD(lParam));
            
			break;
#if 0
        case WM_MOUSELEAVE:
            ASSERT(form);

            form->OpenGLMouseLeave();
            
			break;
#endif
		case WM_COMMAND:
			notifyCode = HIWORD(wParam); // notification code 
			iD = LOWORD(wParam);         // item, control, or accelerator identifier 
			ctrlWndH = (HWND) lParam;    // handle of control

			ProcessCommand(notifyCode, iD, ctrlWndH, form);
			
			return 0;
			
			break;

        case WM_CLOSE:
            ASSERT(form);
            
			if(AudioObjectListenerGet() == form->mSoundListener.get())
			{
				AudioObjectListenerSet(NULL);
			}

			delete form;
            
			::StoreForm(hWindow, NULL);

            break;

		case WM_INITMENU:
			if (form->mSysMenu == (HMENU) wParam)
			{
				// Enable our menu item before performing the default behavior
				MENUITEMINFO mi;
				memset(&mi, 0, sizeof(mi));
				mi.cbSize = sizeof(mi);
				mi.fMask = MIIM_STATE;
				mi.fState = MFS_ENABLED;
				SetMenuItemInfo (form->mSysMenu, CSC_PROPERTIES, FALSE, &mi);
			}
			break;

		case WM_SYSCOMMAND:
			{
				WPARAM uCmdType = wParam; // type of system command requested 
				WORD xPos = LOWORD(lParam);    // horizontal postion, in screen coordinates 
				WORD yPos = HIWORD(lParam);    // vertical postion, in screen coordinates 			}

				switch (uCmdType)
				{
					case CSC_PROPERTIES:
					// Pop up the properties menu.
					{
					    CMenu*	popupMenu = form->GetSceneViewPopupMenu();
				        if(popupMenu)
						{
							::TrackPopupMenu(popupMenu->m_hMenu, TPM_LEFTALIGN, xPos, yPos, 0, hWindow, NULL);
							return 0;
						}
					}
					break;
				}
			}
			break;

		case WM_KEYDOWN:
			form->OpenGLKeyDown((int)wParam);    // virtual-key code 
			break;

		case WM_KEYUP:
			form->OpenGLKeyUp((int)wParam);    // virtual-key code 
			break;

		case WM_TIMER:
			if (form)
			{
				if (form->OnTimer ((UINT) wParam, (TIMERPROC *) lParam))
				{
					// Don't perform default processing.
					return 0;
				}
			}
			break;
    }

	return DefWindowProc(hWindow, message, wParam, lParam);
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::CreateAppWindow (HINSTANCE hAppInstance, const char* title, HWND parentWndH)
{
	WNDCLASS wc;

	if (mWndH)
    {
		return;
    }

	//
	// Set up and register application window class
	//
	
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hAppInstance;
	wc.hIcon         = 0; // LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = 0; // GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = title;
	
	RegisterClass(&wc);
	
	//
	// Create application's main window
	//
	
	mWndH = CreateWindowEx(
		0,
		title,
		title,
		WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW, 
		0,
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		parentWndH,
		NULL,
		hAppInstance,
		NULL);

	StoreForm();

	//UpdateWindow (mWndH);
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::SetStyleInWindow(unsigned int left, unsigned int top, unsigned int width, unsigned int height)
{
	// Enable caption menu and user preferences
	SetWindowLong(mWndH, GWL_STYLE, GetWindowLong(mWndH, GWL_STYLE) & ~WS_POPUP);
	SetWindowLong(mWndH, GWL_STYLE, GetWindowLong(mWndH, GWL_STYLE) | (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX));
	SetWindowLong(mWndH, GWL_EXSTYLE, GetWindowLong(mWndH, GWL_EXSTYLE) & ~WS_EX_TOPMOST);

	// Set window size and position	
	RECT rect;

	rect.left = left;
	rect.top = top;
	rect.right = left + width - 1;
	rect.bottom = top + height - 1;

	AdjustWindowRectEx(&rect, GetWindowLong(mWndH, GWL_STYLE), GetMenu (mWndH) != NULL, GetWindowLong (mWndH, GWL_EXSTYLE));

	SetWindowPos(mWndH, HWND_TOP, rect.left, rect.top, rect.right - rect.left + 1, rect.bottom - rect.top + 1, SWP_NOCOPYBITS | SWP_NOZORDER);
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::Repaint()
{
	RedrawWindow(mWndH, NULL, NULL, RDW_INVALIDATE);
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::PaintScene()
{
	ASSERT(PIPE);

	Matrix4	projection;	// Identity by default

	projection.set_identity();

	PIPE->set_projection(projection);

	LightUpdateLighting(mCamera);

    Render(mCamera);
}
#if 0
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::OpenGLPanelGLPaint()
{
    wglMakeCurrent(OpenGLPanel->GetDC(), OpenGLPanel->GetGLContext());

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

    Render(mCamera);
}
#endif
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::Render(const ROS::DABaseCamera* camera) const
{
 	SetGLRenderVolume();

	ASSERT(PIPE);

	if(mRenderTextured)
	{
		PIPE->set_pipeline_state(RP_TEXTURE, TRUE);
		PIPE->set_render_state(D3DRS_FILLMODE, D3DFILL_SOLID);
	}
	else
	{
		PIPE->set_pipeline_state(RP_TEXTURE, FALSE);
		PIPE->set_render_state(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	}

	Vector	position;
	CameraGetPosition(camera, position);

    if(mShowAxes)
	{
		GL::Draw3dAxes(22.0, 0.5);

        const ROS::AStaticSceneEntity* staticSceneEntity = dynamic_cast<const ROS::AStaticSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

        if(staticSceneEntity)
        {
			ASSERT(PIPE);

			Transform	oldModelView;
			
			PIPE->get_modelview(oldModelView);

			const std::auto_ptr<ROS::ConstStaticsStateAccessor> access = staticSceneEntity->GetConstStaticsStateAccessor();

            ROS::Location location = access->GetLocationInWorld();
            ROS::Orientation orientation = access->GetOrientationInWorld();

			Vector	loc(location.GetX(), location.GetY(), location.GetZ());
			Matrix	orient(orientation.GetI(), orientation.GetJ(), orientation.GetK());

			Transform	tr(orient, loc);
			tr = oldModelView * tr;

			PIPE->set_modelview(tr);
            
            GL::Draw3dAxes(10.0, 1.0);

            PIPE->set_modelview(oldModelView);
        }
    }

   	if(mShowMesh)
	{
		GL::DrawXZMesh(20.0, 20.0, 1, 1);
    }


    DrawScene(camera);
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::DisplayStartup()
{
	ASSERT(PIPE);
    
	PIPE->set_pipeline_state(RP_TEXTURE, TRUE);

	PIPE->set_pipeline_state(RP_BATCH_LAZY_STATE, FALSE);
	//PIPE->set_pipeline_state(RP_BATCH_TRANSLUCENT_POOL, XXXX );
	PIPE->set_pipeline_state(RP_BATCH_TRANSLUCENT_MODE, RP_TRANSLUCENT_DEPTH_SORTED);
	PIPE->set_pipeline_state(RP_BATCH_POOLS, RP_TRANSLUCENT_DEPTH_SORTED);
	PIPE->set_pipeline_state(RP_BATCH, TRUE);

    SetGLRenderVolume();
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::SetGLRenderVolume() const
{
	ASSERT(PIPE);

	// NOTE: These are forced to be <= the pipeline buffer dimensions.
	PIPE->set_viewport (0, 0, mWidth, mHeight);
	PIPE->set_perspective(CameraGetVerticalFOV(mCamera), CameraGetAspectRatio(mCamera), CameraGetZNear(mCamera), CameraGetZFar(mCamera));

	Transform	camTransform = CameraGetTransform(mCamera);

    Transform	inv = camTransform.get_inverse();

	PIPE->set_modelview(inv);
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::DrawScene(const ROS::DABaseCamera* camera) const
{
	ASSERT(PIPE);

	ROS::SceneEntityCollection   sceneEntityColl;

    mAssociatedSceneView.GetSceneEntities(sceneEntityColl);

    ROS::SceneEntityCollection::const_iterator   begin = sceneEntityColl.begin();
    const ROS::SceneEntityCollection::const_iterator kEnd = sceneEntityColl.end();

    // NOTE:Can remove the saving of the modelview since draw is safe
	Transform	oldModelView;

    PIPE->get_modelview(oldModelView);

    while(begin != kEnd)
    {
		(*begin)->GetConstSceneEntityStateAccessor()->Draw(camera);
        ++begin;
    }

    PIPE->set_modelview(oldModelView);
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::OpenGLMouseDown(TMouseButton Button, TShiftState Shift, int X, int Y)
{
	SetCapture(mWndH);

    CMenu*	popupMenu = NULL;

	ROS::ASceneEntity* sceneEntity = mAssociatedSceneView.GetSelectedSceneEntity();

	if(sceneEntity != NULL)
	{
		// We are in entity edit mode
		ROS::ASceneEntity*	selectedEntity = GetEntity(X, Y);

		if(selectedEntity != NULL)
		{
			if(selectedEntity != sceneEntity)
			{
				sceneEntity = selectedEntity;
			
				mAssociatedSceneView.SetSelectedSceneEntity(sceneEntity);
			}
		}
	}

    sceneEntity = mAssociatedSceneView.GetSelectedSceneEntity();	// Can't assume that the sceneEntity is the current selection

    if(Button == mbLeft)
    {
		if(sceneEntity)
        {
			if(Shift & ssCtrl)
            {
				if(Shift & ssAlt)
				{
					mCurrentMode = rollEntityMode;
				}
				else
				{
					mCurrentMode = rotateEntityWithoutRollMode;
				}

				ROS::AStaticSceneEntity*	staticEntity = dynamic_cast<ROS::AStaticSceneEntity*>(sceneEntity);

				if(staticEntity)
				{
					mAssociatedSceneView.AddUndoableOperation(new StaticEntityOrientationChange(*staticEntity, mAssociatedSceneView));
				}
            }
			else if (Shift & ssAlt)
			{
				mCurrentMode = rotateViewAboutEntityMode;
				mStartTr = CameraGetTransform(mCamera);
			}
            else
            {
				mCurrentMode = translateEntityMode;

				ROS::AStaticSceneEntity*	staticEntity = dynamic_cast<ROS::AStaticSceneEntity*>(sceneEntity);

				if(staticEntity)
				{
					mAssociatedSceneView.AddUndoableOperation(new StaticEntityLocationChange(*staticEntity, mAssociatedSceneView));
				}
            }
        }
        else
        {
			if(Shift & ssCtrl)
            {
				if(Shift & ssAlt)
				{
					mCurrentMode = rollViewMode;
				}
				else
				{
					mCurrentMode = rotateViewWithoutRollMode;
				}

				mStartTr = CameraGetTransform(mCamera);
            }
            else if(Shift & ssShift)
            {
				mCurrentMode = dollyViewMode;
            }
			else if(Shift & ssAlt)
			{
				// Select a rectangle within the view for quick zooming.
				mCurrentMode = viewRectSelectMode;
			}
            else
            {
				mCurrentMode = translateViewMode;

				// Install the timer.
				SetTimer (mWndH, XLAT_TIMER, 0, NULL);
            }
        }
    }
    else if(Button == mbRight)
    {
		if(sceneEntity)
    	{
			ROS::CompoundSceneEntity*	compSceneEntity;
        	ROS::DeformableSceneEntity*	defSceneEntity;
			ROS::AStaticSceneEntity*	staticSceneEntity;
			ROS::PositionMarker*		markerSceneEntity;

            if(defSceneEntity = dynamic_cast<ROS::DeformableSceneEntity*>(sceneEntity))
            {	
//****          ROS::Time time = defSceneEntity->GetMotionStateAccessor()->GetCurrentMotionTime();
//****          DeformableSceneEntityPopupMenu->Items->Items[5]->Caption = AnsiString("Time: ") + FloatToStrF(time.GetTime(), ffFixed, 6, 2);

				popupMenu = GetDeformableEntityPopupMenu(*defSceneEntity);

				ASSERT(popupMenu);
            }
            else if(compSceneEntity = dynamic_cast<ROS::CompoundSceneEntity*>(sceneEntity))
            {
				// Clear all entries in submenu

                popupMenu = GetCompoundEntityPopupMenu(*compSceneEntity);
				
				ASSERT(popupMenu);
            }
			else if(markerSceneEntity= dynamic_cast<ROS::PositionMarker*>(sceneEntity))
            {
				popupMenu = GetPositionMarkerPopupMenu(*markerSceneEntity);

				ASSERT(popupMenu);
            }
            else if(staticSceneEntity = dynamic_cast<ROS::AStaticSceneEntity*>(sceneEntity))
            {
				popupMenu = GetStaticSceneEntityPopupMenu(*staticSceneEntity);

				ASSERT(popupMenu);
            }
        }
        else
        {
			popupMenu = GetSceneViewPopupMenu();

			ASSERT(popupMenu);
        }

        if(popupMenu)
        {
			POINT	point;
        	point.x = X;
            point.y = Y;

        	if(ClientToScreen(mWndH, &point))
            {
				::TrackPopupMenu(popupMenu->m_hMenu, TPM_LEFTALIGN, point.x, point.y, 0, mWndH, NULL);	
            }
        }
    }

    mLastMouseX = X;
    mLastMouseY = Y;
	mStartMouseX = X;
	mStartMouseY = Y;
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::CreateHardpointPopupMenu(CMenu& popupMenu, const std::auto_ptr<ROS::ConstMotionStateAccessor>& motAccessor)
{
	popupMenu.CreateMenu();
	// Put in entries for <Root> and separator
	popupMenu.AppendMenu(MF_STRING, mMinCommandID, "<Root>");
	--mMinCommandID;

	popupMenu.AppendMenu(MF_SEPARATOR, 0);

    const unsigned int hardPointCount = motAccessor->GetHardPointCount();

	// Put in entries for all the hardpoints in the compound
	for(unsigned int hardPointIdx = 0; hardPointIdx < hardPointCount; ++hardPointIdx)
	{
		UINT	flags = MF_STRING;
		
		if((hardPointIdx + 2) % 25 == 0)
		{
			flags |= MF_MENUBARBREAK;
		}

		popupMenu.AppendMenu(flags, mMinCommandID, motAccessor->GetHardPointName(hardPointIdx).c_str());
		--mMinCommandID;
	}
}
//---------------------------------------------------------------------------
CMenu* TGLSceneViewUIForm::GetDeformableEntityPopupMenu(const ROS::DeformableSceneEntity& deformableEntity)
{
    const std::auto_ptr<ROS::ConstMotionStateAccessor>	motAccessor = deformableEntity.GetConstMotionStateAccessor();
    const std::auto_ptr<ROS::ConstStaticsStateAccessor>	sAccessor = deformableEntity.GetConstStaticsStateAccessor();

	mMinCommandID = gMaxCommandID;	// Start at the highest available command to avoid conflicts with other commands
	mCommandType = kDeformable;

	//First tackle the hardpoints, then the cameras
	CMenu* popupMenu = mPopupMenus.GetSubMenu(kDeformablePopupMenuIndex);
	ASSERT(popupMenu);
	
	// Set the state of the show hardpoints menu item
    popupMenu->EnableMenuItem(ID_DEFORMABLE_ENTITY_SHOW_HARDPOINTS, MF_ENABLED);
	popupMenu->CheckMenuItem(ID_DEFORMABLE_ENTITY_SHOW_HARDPOINTS, motAccessor->AreHardPointsShowing() ? MF_CHECKED : MF_UNCHECKED);
	popupMenu->CheckMenuItem(ID_DEFORMABLE_ENTITY_SHOW_SKELETON, motAccessor->IsSkeletonShowing() ? MF_CHECKED : MF_UNCHECKED);

	// Add a seperator and a menu item for starting an IK motion on the selected entity.
	// If the menu has not been added, add it.
	if (popupMenu->GetMenuState(ID_SELECT_IK_TARGET, MF_BYCOMMAND) == 0xFFFFFFFF)
	{
		popupMenu->AppendMenu (MF_SEPARATOR);
		popupMenu->AppendMenu (MF_STRING, ID_SELECT_IK_TARGET, "Start IK Motion...");
	}
	popupMenu->EnableMenuItem (ID_SELECT_IK_TARGET, MF_ENABLED);

#if 0
	// Set the state of the show motion path menu item
    popupMenu->EnableMenuItem(ID_DEFORMABLE_ENTITY_SHOW_MOTION_PATH, MF_ENABLED);
	popupMenu->CheckMenuItem(ID_DEFORMABLE_ENTITY_SHOW_MOTION_PATH, sAccessor->IsStaticsPathVisible() ? MF_CHECKED : MF_UNCHECKED);
#endif

	// Construct the hardpointToRememberListPopupMenu list popup menu
	CString	label;
	CMenu	hardpointToRememberListPopupMenu;	// The Windows menu associated with this instance of CMenu will be passed to a parent menu, so its alright to let the instance destruct at the end of the scope

	CreateHardpointPopupMenu(hardpointToRememberListPopupMenu, motAccessor);

	popupMenu->GetMenuString(kDeformableEntitySelectHardpointToRememberIndex, label, MF_BYPOSITION);
	popupMenu->ModifyMenu(kDeformableEntitySelectHardpointToRememberIndex, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)(hardpointToRememberListPopupMenu.m_hMenu), label);

	hardpointToRememberListPopupMenu.Detach();	// The popupMenu has ownership of the Windows menu

	// Construct the hardpointToMoveListPopupMenu list popup menu
	CMenu	hardpointToMoveListPopupMenu;	// The Windows menu associated with this instance of CMenu will be passed to a parent menu, so its alright to let the instance destruct at the end of the scope

	CreateHardpointPopupMenu(hardpointToMoveListPopupMenu, motAccessor);

	popupMenu->GetMenuString(kDeformableEntitySelectHardpointToMoveIndex, label, MF_BYPOSITION);
	popupMenu->ModifyMenu(kDeformableEntitySelectHardpointToMoveIndex, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)(hardpointToMoveListPopupMenu.m_hMenu), label);

	hardpointToMoveListPopupMenu.Detach();	// The popupMenu has ownership of the Windows menu

	// Construct the hardpointToMakeChildListPopupMenu list popup menu
	CMenu	hardpointToMakeChildListPopupMenu;	// The Windows menu associated with this instance of CMenu will be passed to a parent menu, so its alright to let the instance destruct at the end of the scope

	CreateHardpointPopupMenu(hardpointToMakeChildListPopupMenu, motAccessor);

	popupMenu->GetMenuString(kDeformableEntitySelectHardpointToMakeChildIndex, label, MF_BYPOSITION);
	popupMenu->ModifyMenu(kDeformableEntitySelectHardpointToMakeChildIndex, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)(hardpointToMakeChildListPopupMenu.m_hMenu), label);

	hardpointToMakeChildListPopupMenu.Detach();	// The popupMenu has ownership of the Windows menu

#if 0
	//Now to handle the cameras
    const std::auto_ptr<ROS::ConstCompoundStateAccessor> cAccessor = compoundEntity.GetConstCompoundStateAccessor();

    const unsigned int cameraCount = cAccessor->GetCameraCount();

	// Construct the cameraListPopupMenu list popup menu
	CMenu	cameraListPopupMenu;	// The Windows menu associated with this instance of CMenu will be passed to a parent menu, so its alright to let the instance destruct at the end of the scope
	cameraListPopupMenu.CreateMenu();

	// Note: No entry for <Root> and separator this time

	// Put in entries for all the cameras in the deformable
    for(unsigned int cameraIdx = 0; cameraIdx < cameraCount; ++cameraIdx)
    {
		UINT	flags = MF_STRING;
		
		if((cameraIdx % 25 == 0) && cameraIdx > 0)
		{
			flags |= MF_MENUBARBREAK;
		}

		cameraListPopupMenu.AppendMenu(flags, mMinCommandID, cAccessor->GetCameraName(cameraIdx).c_str());
		--mMinCommandID;
    }

	popupMenu->GetMenuString(kDeformableEntitySetCameraPositionIndex, label, MF_BYPOSITION);
	popupMenu->ModifyMenu(kDeformableEntitySetCameraPositionIndex, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)(cameraListPopupMenu.m_hMenu), label);

	cameraListPopupMenu.Detach();	// The popupMenu has ownership of the Windows menu
#endif
	// Done!
	return popupMenu;
}
//---------------------------------------------------------------------------
CMenu* TGLSceneViewUIForm::GetCompoundEntityPopupMenu(const ROS::CompoundSceneEntity& compoundEntity)
{
    const std::auto_ptr<ROS::ConstMotionStateAccessor>	motAccessor = compoundEntity.GetConstMotionStateAccessor();
    const std::auto_ptr<ROS::ConstStaticsStateAccessor>	sAccessor = compoundEntity.GetConstStaticsStateAccessor();

	mMinCommandID = gMaxCommandID;	// Start at the highest available command to avoid conflicts with other commands
	mCommandType = kCompound;

	//First tackle the hardpoints, then the cameras
	CMenu* popupMenu = mPopupMenus.GetSubMenu(kCompoundPopupMenuIndex);
	ASSERT(popupMenu);
	
	// Set the state of the show hardpoints menu item
    popupMenu->EnableMenuItem(ID_COMPOUND_ENTITY_SHOW_HARDPOINTS, MF_ENABLED);
	popupMenu->CheckMenuItem(ID_COMPOUND_ENTITY_SHOW_HARDPOINTS, motAccessor->AreHardPointsShowing() ? MF_CHECKED : MF_UNCHECKED);

	// Set the state of the show motion path menu item
    popupMenu->EnableMenuItem(ID_COMPOUND_ENTITY_SHOW_MOTION_PATH, MF_ENABLED);
	popupMenu->CheckMenuItem(ID_COMPOUND_ENTITY_SHOW_MOTION_PATH, sAccessor->IsStaticsPathVisible() ? MF_CHECKED : MF_UNCHECKED);

	// Construct the hardpointToRememberListPopupMenu list popup menu
	CString	label;
	CMenu	hardpointToRememberListPopupMenu;	// The Windows menu associated with this instance of CMenu will be passed to a parent menu, so its alright to let the instance destruct at the end of the scope

	CreateHardpointPopupMenu(hardpointToRememberListPopupMenu, motAccessor);

	popupMenu->GetMenuString(kCompoundEntitySelectHardpointToRememberIndex, label, MF_BYPOSITION);
	popupMenu->ModifyMenu(kCompoundEntitySelectHardpointToRememberIndex, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)(hardpointToRememberListPopupMenu.m_hMenu), label);

	hardpointToRememberListPopupMenu.Detach();	// The popupMenu has ownership of the Windows menu

	// Construct the hardpointToMoveListPopupMenu list popup menu
	CMenu	hardpointToMoveListPopupMenu;	// The Windows menu associated with this instance of CMenu will be passed to a parent menu, so its alright to let the instance destruct at the end of the scope

	CreateHardpointPopupMenu(hardpointToMoveListPopupMenu, motAccessor);

	popupMenu->GetMenuString(kCompoundEntitySelectHardpointToMoveIndex, label, MF_BYPOSITION);
	popupMenu->ModifyMenu(kCompoundEntitySelectHardpointToMoveIndex, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)(hardpointToMoveListPopupMenu.m_hMenu), label);

	hardpointToMoveListPopupMenu.Detach();	// The popupMenu has ownership of the Windows menu

	//Now to handle the cameras
    const std::auto_ptr<ROS::ConstCompoundStateAccessor> cAccessor = compoundEntity.GetConstCompoundStateAccessor();

    const unsigned int cameraCount = cAccessor->GetCameraCount();

	// Construct the hardpointToMakeChildListPopupMenu list popup menu
	CMenu	hardpointToMakeChildListPopupMenu;	// The Windows menu associated with this instance of CMenu will be passed to a parent menu, so its alright to let the instance destruct at the end of the scope

	CreateHardpointPopupMenu(hardpointToMakeChildListPopupMenu, motAccessor);

	popupMenu->GetMenuString(kCompoundEntitySelectHardpointToMakeChildIndex, label, MF_BYPOSITION);
	popupMenu->ModifyMenu(kCompoundEntitySelectHardpointToMakeChildIndex, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)(hardpointToMakeChildListPopupMenu.m_hMenu), label);

	hardpointToMakeChildListPopupMenu.Detach();	// The popupMenu has ownership of the Windows menu

	// Construct the cameraListPopupMenu list popup menu
	CMenu	cameraListPopupMenu;	// The Windows menu associated with this instance of CMenu will be passed to a parent menu, so its alright to let the instance destruct at the end of the scope
	cameraListPopupMenu.CreateMenu();

	// Note: No entry for <Root> and separator this time

	// Put in entries for all the cameras in the compound
    for(unsigned int cameraIdx = 0; cameraIdx < cameraCount; ++cameraIdx)
    {
		UINT	flags = MF_STRING;
		
		if((cameraIdx % 25 == 0) && cameraIdx > 0)
		{
			flags |= MF_MENUBARBREAK;
		}

		cameraListPopupMenu.AppendMenu(flags, mMinCommandID, cAccessor->GetCameraName(cameraIdx).c_str());
		--mMinCommandID;
    }

	popupMenu->GetMenuString(kCompoundEntitySetCameraPositionIndex, label, MF_BYPOSITION);
	popupMenu->ModifyMenu(kCompoundEntitySetCameraPositionIndex, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)(cameraListPopupMenu.m_hMenu), label);

	cameraListPopupMenu.Detach();	// The popupMenu has ownership of the Windows menu

	// Done!
	return popupMenu;
}
//---------------------------------------------------------------------------
CMenu* TGLSceneViewUIForm::GetStaticSceneEntityPopupMenu(const ROS::AStaticSceneEntity& staticEntity)
{
    const std::auto_ptr<ROS::ConstStaticsStateAccessor> sAccessor = staticEntity.GetConstStaticsStateAccessor();

	//First tackle the hardpoints, then the cameras
    CMenu* popupMenu = mPopupMenus.GetSubMenu(kSceneEntityPopupMenuIndex);
	ASSERT(popupMenu);
	
	// Set the state of the show motion path menu item
    popupMenu->EnableMenuItem(ID_SCENE_ENTITY_SHOW_MOTION_PATH, MF_ENABLED);
	popupMenu->CheckMenuItem(ID_SCENE_ENTITY_SHOW_MOTION_PATH, sAccessor->IsStaticsPathVisible() ? MF_CHECKED : MF_UNCHECKED);

	// Done!
	return popupMenu;
}
//---------------------------------------------------------------------------
CMenu* TGLSceneViewUIForm::GetPositionMarkerPopupMenu(const ROS::PositionMarker& positionMarker)
{
    const std::auto_ptr<ROS::ConstStaticsStateAccessor> sAccessor = positionMarker.GetConstStaticsStateAccessor();

	//First tackle the hardpoints, then the cameras
    CMenu* popupMenu = mPopupMenus.GetSubMenu(kPositionMarkerPopupMenuIndex);
	ASSERT(popupMenu);
	
	// Set the state of the show motion path menu item
    popupMenu->EnableMenuItem(ID_SCENE_ENTITY_SHOW_MOTION_PATH, MF_ENABLED);
	popupMenu->CheckMenuItem(ID_SCENE_ENTITY_SHOW_MOTION_PATH, sAccessor->IsStaticsPathVisible() ? MF_CHECKED : MF_UNCHECKED);

	// Done!
	return popupMenu;
}
//---------------------------------------------------------------------------
CMenu* TGLSceneViewUIForm::GetSceneViewPopupMenu()
{
	CMenu*	popupMenu = mPopupMenus.GetSubMenu(kSceneViewPopupMenuIndex);
	ASSERT(popupMenu);
	
	// Construct the camera list popup menu
	CMenu	cameraListPopupMenu;	// The Windows menu associated with this instance of CMenu will be passed to a parent menu, so its alright to let the instance destruct at the end of the scope
	cameraListPopupMenu.CreateMenu();

	mMinCommandID = gMaxCommandID;	// Start at the highest available command to avoid conflicts with other commands
	mCommandType = kCamera;

	// Put in entries for <None> and separator
	UINT	flags = MF_STRING;
	if(IsNull(mAssociatedCamera))
	{
		flags |= MF_CHECKED;
	}

	cameraListPopupMenu.AppendMenu(flags, mMinCommandID, "<None>");
	--mMinCommandID;

	cameraListPopupMenu.AppendMenu(MF_SEPARATOR, 0);

	// Put in entries for all the cameras in the scene
	ROS::SceneEntityCollection   sceneEntityColl;

    mAssociatedSceneView.GetSceneEntities(sceneEntityColl);

    ROS::SceneEntityCollection::const_iterator			begin = sceneEntityColl.begin();
    const ROS::SceneEntityCollection::const_iterator	kEnd = sceneEntityColl.end();
	unsigned int										cameraCount = 0;

    while(begin != kEnd)
    {
		ROS::ADynamicCamera*	camera = dynamic_cast<ROS::ADynamicCamera*>(*begin);

		if(camera)
		{
			flags = MF_STRING;
			
			if(camera == mAssociatedCamera)
			{
				flags |= MF_CHECKED;
			}

			if((cameraCount + 2) % 25 == 0)
			{
				flags |= MF_MENUBARBREAK;
			}
			++cameraCount;

			cameraListPopupMenu.AppendMenu(flags, mMinCommandID, camera->GetConstSceneEntityStateAccessor()->GetName().c_str());
			--mMinCommandID;
		}
        ++begin;
    }

	CString	label;

	popupMenu->GetMenuString(kSceneViewSelectCameraIndex, label, MF_BYPOSITION);
	popupMenu->ModifyMenu(kSceneViewSelectCameraIndex, MF_BYPOSITION | MF_POPUP | MF_STRING, (UINT)(cameraListPopupMenu.m_hMenu), label);

	cameraListPopupMenu.Detach();	// The popupMenu has ownership of the Windows menu

	return popupMenu;
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::OpenGLMouseMove(TShiftState Shift, int X, int Y)
{
    if(mCurrentMode == rotateViewWithoutRollMode)
    {
		int rotateAboutY = X - mLastMouseX;
        int rotateAboutX = Y - mLastMouseY;

        Transform   yRotTr, xRotTr, originalTr, newTr, oldToNewTr;
		// Build a transform from old view space to new view space.
        yRotTr.compose_rotation(Y_AXIS, gRotationFactor * rotateAboutY);
        xRotTr.compose_rotation(X_AXIS, gRotationFactor * rotateAboutX);
        oldToNewTr = xRotTr * yRotTr;
		// The new transformation chain is therefore:
		// inv(oldToNewTr) * mStartTr.
		originalTr = CameraGetTransform(mCamera);
//		newTr = mStartTr * oldToNewTr.get_inverse();
		newTr = originalTr * oldToNewTr.get_inverse();
		
		const Transform newCamTr = removeRoll(newTr, originalTr.translation);

		// Make it so.
        CameraSetTransform(mCamera, newCamTr);
        SetAssociatedCameraToInternalCamera();

        mLastMouseX = X;
        mLastMouseY = Y;

        Repaint();
    }
    else if(mCurrentMode == rollViewMode)
    {
        int rotateAboutZ = Y - mLastMouseY;

        Transform   zRotTr, originalTr, newTr;
		// Build a transform from old view space to new view space.
        zRotTr.compose_rotation(Z_AXIS, gRotationFactor * rotateAboutZ);

		originalTr = CameraGetTransform(mCamera);

		newTr = originalTr * zRotTr.get_inverse();
		
		// Make it so.
        CameraSetTransform(mCamera, newTr);
        SetAssociatedCameraToInternalCamera();

        mLastMouseX = X;
        mLastMouseY = Y;

        Repaint();
    }
    else if(mCurrentMode == rotateViewAboutEntityMode)
    {
		ROS::AStaticSceneEntity* staticSceneEntity = dynamic_cast<ROS::AStaticSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

        if(staticSceneEntity)
        {
			// update the view by rotating about the scene entity's location.
            std::auto_ptr<ROS::StaticsStateAccessor> stateAccess = staticSceneEntity->GetStaticsStateAccessor();

			// This is really a rotation of the view's position about the given entity's position, then a
			// recalculation of the orientation to look at the object, with roll removed.

			Transform   yRotTr, xRotTr, originalTr, newTr;
			originalTr = CameraGetTransform(mCamera);

			// First, translate the wacky ROS stuff into DA stuff.
			Vector centerPos;
			{
				ROS::Location l = stateAccess->GetLocation();
				centerPos = Vector(l.GetX(), l.GetY(), l.GetZ());
			}

			// Get the camera position relative to the center position.
			Vector pos = originalTr.translation - centerPos; 

			// Create a transformation matrix by treating the x movement as yaw and y movement as pitch
			// NOTE: These rotations are about the i and j axis of the current camera transform, not the world
			// axis.
			const SINGLE M_PI = 3.14159;

			SINGLE rotateAboutY = (X - mLastMouseX) * M_PI / 180;
			SINGLE rotateAboutX = (Y - mLastMouseY) * M_PI / 180;
			Quaternion xRotQ(originalTr.get_i(), gRotationFactor * rotateAboutX);
			Quaternion yRotQ(originalTr.get_j(), gRotationFactor * rotateAboutY);

			newTr = Matrix(xRotQ * yRotQ);

			// Transform to the new position and convert back to world coordinates.
			Vector newPos = (newTr * pos) + centerPos;

			// Calculate a new camera orientation, looking at the entity.
			Vector kWorldAxis = newPos - centerPos;
			Vector jWorldAxis(0,1,0);
			Vector iWorldAxis = cross_product (jWorldAxis, kWorldAxis);
			if (iWorldAxis.magnitude() < 0.00001)
			{
				// To prevent badness as k approaches j, cross k and i
				jWorldAxis = cross_product(kWorldAxis, Vector(1,0,0));
				iWorldAxis = cross_product(jWorldAxis, kWorldAxis);
			}
			else
			{
				// Now calculate the new j;
				jWorldAxis = cross_product(kWorldAxis, iWorldAxis);
			}
			// Normalize the vectors and use them to build the transform
			iWorldAxis.normalize();
			jWorldAxis.normalize();
			kWorldAxis.normalize();
			Transform newCamTr(Matrix(iWorldAxis, jWorldAxis, kWorldAxis), newPos);

			CameraSetTransform(mCamera, newCamTr);
			SetAssociatedCameraToInternalCamera();

			mLastMouseX = X;
			mLastMouseY = Y;

			Repaint();
		}
	}
    else if(mCurrentMode == rotateEntityWithoutRollMode)
    {
		ROS::AStaticSceneEntity* staticSceneEntity = dynamic_cast<ROS::AStaticSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

        if(staticSceneEntity)
        {
			// update the scene entity
            std::auto_ptr<ROS::StaticsStateAccessor> stateAccess = staticSceneEntity->GetStaticsStateAccessor();

			const bool			isAtKeyOrientation = stateAccess->IsAtKeyOrientation();
            ROS::Orientation	orient = stateAccess->GetOrientation();
            ROS::Orientation	rotationX, rotationY, result;

            rotationX.MakeRotationMatrix(Vector(1, 0, 0), gRotationFactor * (mLastMouseY - Y));

            rotationY.MakeRotationMatrix(Vector(0, 1, 0), gRotationFactor * (mLastMouseX - X));

            orient = orient * rotationX;
            orient = orient * rotationY;
    
			Transform tr = removeRoll(Transform(orient.GetI(), orient.GetJ(), orient.GetK()), Vector(0,0,0));
			stateAccess->SetOrientation(ROS::Matrix(tr.get_i(), tr.get_j(), tr.get_k()));

            // inform the scene model
			if(stateAccess->IsStaticsPathVisible())
			{
				if(isAtKeyOrientation)
				{
					mAssociatedSceneView.SelectedSceneEntityUpdated();
				}
				else
				{
					mAssociatedSceneView.SceneUpdated();
				}
			}
			else
			{
				mAssociatedSceneView.SelectedSceneEntityUpdated();
			}

            mLastMouseX = X;
            mLastMouseY = Y;
        }
    }
    else if(mCurrentMode == rollEntityMode)
    {
		ROS::AStaticSceneEntity* staticSceneEntity = dynamic_cast<ROS::AStaticSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

        if(staticSceneEntity)
        {
			// update the scene entity
            std::auto_ptr<ROS::StaticsStateAccessor> stateAccess = staticSceneEntity->GetStaticsStateAccessor();

			const bool			isAtKeyOrientation = stateAccess->IsAtKeyOrientation();
            ROS::Orientation	orient = stateAccess->GetOrientation();
            ROS::Orientation	rotationZ, result;

            rotationZ.MakeRotationMatrix(Vector(0, 0, 1), gRotationFactor * (mLastMouseY - Y));

            orient = orient * rotationZ;
    
			stateAccess->SetOrientation(orient);

            // inform the scene model
			if(stateAccess->IsStaticsPathVisible())
			{
				if(isAtKeyOrientation)
				{
					mAssociatedSceneView.SelectedSceneEntityUpdated();
				}
				else
				{
					mAssociatedSceneView.SceneUpdated();
				}
			}
			else
			{
				mAssociatedSceneView.SelectedSceneEntityUpdated();
			}

            mLastMouseX = X;
            mLastMouseY = Y;
        }
    }
    else if(mCurrentMode == translateEntityMode)
	{
		ROS::AStaticSceneEntity* staticSceneEntity = dynamic_cast<ROS::AStaticSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

        if(staticSceneEntity)
	    {
			Vector	startRay, endRay;

			// Compute starting point on near clipping plane in world, relative to the camera
			CameraGetScreenToPoint(mCamera, mLastMouseX, mLastMouseY, startRay);

			const float	startRayMagnitude = startRay.magnitude();

			if(startRayMagnitude != 0)
			{
				// Compute current point on near clipping plane in world, relative to the camera
				CameraGetScreenToPoint(mCamera, X, Y, endRay);
				
				// Compute translation vector on near clipping plane in world
				const Vector displacement = endRay - startRay;

				// Now compute what this translation means to the static scene entity
				// First get entity's location and it's distance from the camera
				Vector	cameraLocation;
				
				CameraGetPosition(mCamera, cameraLocation);

				std::auto_ptr<ROS::StaticsStateAccessor> stateAccess = staticSceneEntity->GetStaticsStateAccessor();
				
				const bool			isAtKeyLocation = stateAccess->IsAtKeyLocation();
				const ROS::Location	location = stateAccess->GetLocation();
				const float			entityDistance = (Vector(location.GetX(), location.GetY(), location.GetZ()) - cameraLocation).magnitude();

				// Next compute scaled displacement vector for static entity
				const Vector entityDisplacement = displacement * (entityDistance / startRayMagnitude);

				// Finally, apply the displacement to the entity.
				stateAccess->SetLocation(ROS::Location(	location.GetX() + entityDisplacement.x,
														location.GetY() + entityDisplacement.y,
														location.GetZ() + entityDisplacement.z));

				// inform the scene model
				if(stateAccess->IsStaticsPathVisible())
				{
					if(isAtKeyLocation)
					{
						mAssociatedSceneView.SelectedSceneEntityUpdated();
					}
					else
					{
						mAssociatedSceneView.SceneUpdated();
					}
				}
				else
				{
					mAssociatedSceneView.SelectedSceneEntityUpdated();
				}
			}

            mLastMouseX = X;
            mLastMouseY = Y;
        }
    }
    else if(mCurrentMode == translateViewMode)
	{
#if 0
		// This is the old way. User 1:1 pixel movement on the near clipping plane.
		Vector	startRay, endRay;

		// Compute starting point on near clipping plane in world
		CameraGetScreenToPoint(mCamera, mLastMouseX, mLastMouseY, startRay);
		
		// Compute current point on near clipping plane in world
		CameraGetScreenToPoint(mCamera, X, Y, endRay);
		
		// Compute translation vector on near clipping plane in world
		const Vector displacement = endRay - startRay;

		// Now move the camera by this displacement
		Vector	cameraLocation;
		CameraGetPosition(mCamera, cameraLocation);
        CameraSetPosition(mCamera, cameraLocation - displacement);
		SetAssociatedCameraToInternalCamera();

        mLastMouseX = X;
        mLastMouseY = Y;

		Repaint();
#else
		// This is the new way. The translation actually happens during a timer message, so just store the
		// last value here.
        mLastMouseX = X;
        mLastMouseY = Y;

		// Fake a timer event, causing view movement to happen.
		// *** The view calculation should be in its own function, callable from here, instead
		// *** of in the OnTimer() function.

		OnTimer (XLAT_TIMER, NULL);
#endif
    }
    else if(mCurrentMode == viewRectSelectMode)
	{
		// We are just selecting a view rectangle for now, so just store the coordinates and force a repaint.
        mLastMouseX = X;
        mLastMouseY = Y;

		Repaint();
    }
    else if(mCurrentMode == dollyViewMode)
	{
		Transform	transform;
        transform = CameraGetTransform(mCamera);

        float	translationInc = gkCameraTranslationFactor * (-(Y - mLastMouseY));
        Vector	location(transform.translation);
        Vector	translation((transform.get_k()) * translationInc);

        if(translationInc > 0)
		{
			if(location.magnitude () <= translation.magnitude ())
            {
				translation.zero();
            }
        }

		transform.translation = location - translation;
        CameraSetTransform(mCamera, transform);
        SetAssociatedCameraToInternalCamera();

        mLastMouseX = X;
        mLastMouseY = Y;

        Repaint();
    }
    else
    {   
		if(ROS::ASceneEntity* sceneEntity = GetEntity(X, Y))
        {
			POINT	point;
        	point.x = X;
            point.y = Y;

        	if(ClientToScreen(mWndH, &point))
            {
				CString  name = sceneEntity->GetConstSceneEntityStateAccessor()->GetName().c_str();
#ifdef PORTED
                TRect       hintRect = mHint->CalcHintRect(255, name, NULL);

                // move hint to provided point
                hintRect.Left += point.x;
                hintRect.Right += point.x;
                hintRect.Top += point.y;
                hintRect.Bottom += point.y;

                // move hint to lower-right corner of cursor
                int cursorWidth = GetSystemMetrics(SM_CXCURSOR);
                int cursorHeight = GetSystemMetrics(SM_CYCURSOR);

                hintRect.Left += cursorWidth;
                hintRect.Right += cursorWidth;
                hintRect.Top += cursorHeight;
                hintRect.Bottom += cursorHeight;

                mHint->ActivateHint(hintRect, name);
#endif
//              Caption = mSceneViewTitle + mSeparator + name;
                return;
            }
        }
    }

#ifdef PORTED
    mHint->ReleaseHandle();
#endif
	//    Caption = mSceneViewTitle;
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::OpenGLMouseUp(TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if (mCurrentMode == translateViewMode)
	{
		KillTimer (mWndH, XLAT_TIMER);
	}
	else if (mCurrentMode == viewRectSelectMode)
	{
		// The rectangle is selected. Move the camera forward enough that the selected rectangle on the view plane
		// just touches one of the sides of the screen. Also, move the camera so that it is centered in the selected
		// rectangle.

		// This is done in two parts: translate the camera in its XY plane so that the center of the rectangle becomes
		// the center of the view, then translate the camera along its Z axis so that the rectangle just fits inside 
		// the view.

		// First, calculate the rectangle's center and its half width and half height.
		int x0, y0, x1, y1;
		if (mStartMouseX < mLastMouseX)
		{
			x0 = mStartMouseX;
			x1 = mLastMouseX;
		}
		else
		{
			x0 = mLastMouseX;
			x1 = mStartMouseX;
		}
		if (mStartMouseY < mLastMouseY)
		{
			y0 = mStartMouseY;
			y1 = mLastMouseY;
		}
		else
		{
			y0 = mLastMouseY;
			y1 = mStartMouseY;
		}
		
		int centerX = (x1 + x0) / 2;
		int centerY = (y1 + y0) / 2;
		int halfX = x1 - centerX;
		int halfY = y1 - centerY;

		// This is a very subtle operation. This only works with the proper selection of an "interest plane" 
		// perpendicular to the camera's view direction. Using the near plane results in little change, while the far
		// plane results in too much change.
		// The right way would be to find the objects contained by the frustum of the selected rectangle, the choose
		// either the closest depth, farthest depth, or something in the middle.

		// Get the camera's current location and the distance to the plane of interest.
		Vector	cameraLocation;
		CameraGetPosition(mCamera, cameraLocation);
		
		SINGLE farDist;
		
		farDist = CameraGetZFar(mCamera);  // far clip is too far
		farDist = CameraGetZNear(mCamera); // near clip is too near.
		farDist = 200.0;                   // a fixed distance yields mediocre results.

		SINGLE nearest, furthest;
		RECT r;
		r.top = y0;
		r.bottom = y1;
		r.left = x0;
		r.right = x1;
		if (GetDepthRangeOfObjects (r, nearest, furthest))
		{
			// Use the nearest distance.
			farDist = nearest;
		}

		// Find the intersection with the plane of interest of the rays from the camera to the view center and to the
		// rectangle center. The view intersection is simply the view ray scaled by the plane of interest distance.
		// The plane's normal is the inverse of the view ray, and all points on the plane satisfy the
		// equation: (-viewRay) dot Point + D = 0. Thus D = viewRay dot Point.
		// Using the view ray intersection, we can calculate D.
		// All points on the rectangle ray satisfy the equation: P(t) = t * rectRay.
		// So, substituting into the plane equation yields: t * (-viewRay dot rectRay) + D = 0.
		// Thus, t = -D / (-viewRay dot rectRay). Thus t = D / (viewRay dot rectRay).
		// From there, the actual point can be calculated.

		Vector	rectRay, viewRay;

		CameraGetScreenToPoint(mCamera, centerX, centerY, rectRay);
		CameraGetScreenToPoint(mCamera, mWidth/2, mHeight/2, viewRay);
		rectRay.normalize();
		viewRay.normalize();

		Vector viewFarCenter = viewRay * farDist;
		//SINGLE D = dot_product (viewRay, viewFarCenter);
		//SINGLE t = D / dot_product (viewRay, rectRay);
		//Vector rectFarCenter = t * rectRay;
		Vector rectFarCenter = (dot_product (viewRay, viewFarCenter) / dot_product (viewRay, rectRay)) * rectRay;

		// NOTE: The center points are relative to the current camera position, but in the world's frame. We only
		// care about the vector between them, so the we don't need to translate them fully into world space.

		// The translation vector is simply the difference between these two.
		Vector shift = rectFarCenter - viewFarCenter;

		// Now calculate the required forward motion.
		// This can be done by a simple ratio: the desired distance from the current Z clip plane is to the current
		// distance from the Z clip plane as the desired rectangle dimensions are to the current view dimensions.
		// We will be preserving the aspect ratio, so we need to pick either the width or height of the rectangle; we
		// will pick the largest dimension.
		// Once you have the desired distance from the current clip plane, you subtract that from the current clip
		// plane distance to yield the zoom distance.
		// NOTE: The code below has been matematically simplified symbolically to a single scale of the vector from the
		// camera to the far clipping plane.

		Vector zoom = viewFarCenter;
		if (halfX >= halfY)
		{
			zoom *= (1.0f - (halfX * 2.0f / mWidth));
		}
		else
		{
			zoom *= (1.0f - (halfY * 2.0f / mHeight));
		}

		// Move the camera
        CameraSetPosition(mCamera, cameraLocation + zoom + shift);
		SetAssociatedCameraToInternalCamera();
	}

    mCurrentMode = neutralMode;

    ReleaseCapture();

	Repaint ();
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::OpenGLKeyDown(unsigned int virtualKey)
{
	switch(virtualKey)
	{
		case VK_CONTROL:
			mIsControlKeyDown = true;
			break;
		case 'Z':
			if(mIsControlKeyDown)
			{
				mAssociatedSceneView.UndoLastOperation();
			}
			break;
		case 'Y':
			if(mIsControlKeyDown)
			{
				mAssociatedSceneView.RedoLastOperation();
			}
			break;
		case 'A':
			if(mIsControlKeyDown)
			{
				GenerateAVI();
			}
			break;
	}
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::OpenGLKeyUp(unsigned int virtualKey)
{
	if(virtualKey == VK_CONTROL)
	{
		mIsControlKeyDown = false;
	}
}
#ifdef PORTED
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::OpenGLMouseLeave()
{
    mHint->ReleaseHandle();
}
#endif
//---------------------------------------------------------------------------
bool TGLSceneViewUIForm::GetDepthRangeOfObjects (RECT &screenRect, float &nearest, float &furthest)
{
	ROS::SceneEntityCollection	sceneEntityColl;

    mAssociatedSceneView.GetSceneEntities(sceneEntityColl);

    ROS::SceneEntityCollection::const_iterator   begin = sceneEntityColl.begin();
    const ROS::SceneEntityCollection::const_iterator kEnd = sceneEntityColl.end();

	furthest = 0.0f;
	nearest = CameraGetZFar(mCamera);

	bool foundOne = false;
    while(begin != kEnd)
    {
		ROS::AStaticSceneEntity*	entity = dynamic_cast<ROS::AStaticSceneEntity*>(*begin);

		if(entity)
		{
			int objX, objY;
			float objDepth;

			Vector objPos = entity->GetConstStaticsStateAccessor()->GetLocation().GetVector();
			CameraGetPointToScreen(mCamera, objPos, objX, objY, objDepth);
			if (objDepth > 0.0)
			{
				// Object in front of camera. Check inside rect.
				if (objX <= screenRect.right && objX >= screenRect.left &&
					objY <= screenRect.bottom && objY >= screenRect.top)
				{
					foundOne = true;
					if (objDepth > furthest)
					{
						furthest = objDepth;
					}
					if (objDepth < nearest)
					{
						nearest = objDepth;
					}
				}
			}
			else
			{
				// Object behind camera. Skip.
			}
		}

        ++begin;
    }

	return foundOne;
}
//---------------------------------------------------------------------------
ROS::ASceneEntity* TGLSceneViewUIForm::GetEntity(int winX, int winY) const
{
	Vector	rayStart, rayDirection;

	CameraGetScreenToPoint(mCamera, winX, winY, rayDirection);
	CameraGetPosition(mCamera, rayStart);

	rayStart += rayDirection;
	
	const ROS::IntersectInfo	intersectInfo(CameraGetBaseCamera(mCamera), winX, winY, rayStart, rayDirection);

	ROS::ASceneEntity*			selectedSceneEntity = NULL;
	bool						collided = false;
	float						shortestDistance = 0;
	ROS::SceneEntityCollection	sceneEntityColl;

    mAssociatedSceneView.GetSceneEntities(sceneEntityColl);

    ROS::SceneEntityCollection::const_iterator   begin = sceneEntityColl.begin();
    const ROS::SceneEntityCollection::const_iterator kEnd = sceneEntityColl.end();

    while(begin != kEnd)
    {
		float	currDistance;

		if((*begin)->GetConstSceneEntityStateAccessor()->Intersect(intersectInfo, &currDistance))
		{
			if(collided)
			{
				if(currDistance < shortestDistance)
				{
					selectedSceneEntity = (*begin);
					shortestDistance = currDistance;
				}
			}
			else
			{
				collided = true;
				selectedSceneEntity = (*begin);
				shortestDistance = currDistance;
			}
		}

        ++begin;
    }

	return selectedSceneEntity;
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::SelectionXYPlaneClick()
{
	const ROS::ASceneEntity*		sceneEntity = mAssociatedSceneView.GetSelectedSceneEntity();
	const ROS::AStaticSceneEntity*	sEntity = dynamic_cast<const ROS::AStaticSceneEntity*>(sceneEntity);
	
	if(sEntity)
	{
		const Vector	entityLocation = sEntity->GetConstStaticsStateAccessor()->GetLocation().GetVector();

		Transform		originalTr = CameraGetTransform(mCamera);
		const Vector	translation = originalTr.translation;
		const Vector	offsetToEnity = entityLocation - translation;
		const float		magnitude = offsetToEnity.magnitude();

		Transform		newTr;
		const Vector	newTranslation(entityLocation.x, entityLocation.y, entityLocation.z + magnitude);

		newTr.translation = newTranslation;

		CameraSetTransform(mCamera, newTr);
		SetAssociatedCameraToInternalCamera();

		Repaint();
	}
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::SelectionYZPlaneClick()
{
	const ROS::ASceneEntity*		sceneEntity = mAssociatedSceneView.GetSelectedSceneEntity();
	const ROS::AStaticSceneEntity*	sEntity = dynamic_cast<const ROS::AStaticSceneEntity*>(sceneEntity);
	
	if(sEntity)
	{
		const float rotateAboutY = 90;

		const Vector	entityLocation = sEntity->GetConstStaticsStateAccessor()->GetLocation().GetVector();

		Transform		originalTr = CameraGetTransform(mCamera);
		const Vector	translation = originalTr.translation;
		const Vector	offsetToEnity = entityLocation - translation;
		const float		magnitude = offsetToEnity.magnitude();

		Transform		newTr;
		const Vector	newTranslation(entityLocation.x + magnitude, entityLocation.y, entityLocation.z);

		newTr.compose_rotation(Y_AXIS, rotateAboutY);
		newTr.translation = newTranslation;

		CameraSetTransform(mCamera, newTr);
		SetAssociatedCameraToInternalCamera();

		Repaint();
	}
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::SelectionZXPlaneClick()
{
	const ROS::ASceneEntity*		sceneEntity = mAssociatedSceneView.GetSelectedSceneEntity();
	const ROS::AStaticSceneEntity*	sEntity = dynamic_cast<const ROS::AStaticSceneEntity*>(sceneEntity);
	
	if(sEntity)
	{
		const float rotateAboutX = -90;

		const Vector	entityLocation = sEntity->GetConstStaticsStateAccessor()->GetLocation().GetVector();

		Transform		originalTr = CameraGetTransform(mCamera);
		const Vector	translation = originalTr.translation;
		const Vector	offsetToEnity = entityLocation - translation;
		const float		magnitude = offsetToEnity.magnitude();

		Transform		newTr;
		const Vector	newTranslation(entityLocation.x, entityLocation.y + magnitude, entityLocation.z);

	    newTr.compose_rotation(X_AXIS, rotateAboutX);
		newTr.translation = newTranslation;

		CameraSetTransform(mCamera, newTr);
		SetAssociatedCameraToInternalCamera();

		Repaint();
	}
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::XYPlaneClick()
{
    Transform		originalTr = CameraGetTransform(mCamera);
    const Vector	translation = originalTr.translation;
    const float		magnitude = translation.magnitude();

    Transform	newTr;
	
	newTr.translation = Vector(0, 0, magnitude);

    CameraSetTransform(mCamera, newTr);
    SetAssociatedCameraToInternalCamera();

    Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::YZPlaneClick()
{
	const float rotateAboutY = 90;

    Transform   originalTr, newTr;

    newTr.compose_rotation(Y_AXIS, rotateAboutY);

    originalTr = CameraGetTransform(mCamera);
    const Vector	translation = originalTr.translation;
    const float		magnitude = translation.magnitude();

    newTr.translation = Vector(magnitude, 0, 0);

    CameraSetTransform(mCamera, newTr);
    SetAssociatedCameraToInternalCamera();

    Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::ZXPlaneClick()
{
    const float rotateAboutX = -90;

    Transform   originalTr, newTr;
    newTr.compose_rotation(X_AXIS, rotateAboutX);

    originalTr = CameraGetTransform(mCamera);
    const Vector	translation = originalTr.translation;
    const float		magnitude = translation.magnitude();

    newTr.translation = Vector(0, magnitude, 0);

    CameraSetTransform(mCamera, newTr);
    SetAssociatedCameraToInternalCamera();

    Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::MeshClick()
{
    UINT	state = mPopupMenus.GetMenuState(ID_SCENE_VIEW_MESH, MF_BYCOMMAND);
	bool	checked = (state & MF_CHECKED) != 0;
	UINT	newChecked = MF_BYCOMMAND;

	newChecked |= checked ? MF_UNCHECKED : MF_CHECKED;

	mPopupMenus.CheckMenuItem(ID_SCENE_VIEW_MESH, newChecked);

   	mShowMesh = !checked;

	Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::AxesClick()
{
    UINT	state = mPopupMenus.GetMenuState(ID_SCENE_VIEW_AXES, MF_BYCOMMAND);
	bool	checked = (state & MF_CHECKED) != 0;
	UINT	newChecked = MF_BYCOMMAND;

	newChecked |= checked ? MF_UNCHECKED : MF_CHECKED;

	mPopupMenus.CheckMenuItem(ID_SCENE_VIEW_AXES, newChecked);

   	mShowAxes = !checked;

	Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::ShowLetterboxClick()
{
	UINT	state = mPopupMenus.GetMenuState(ID_SCENEVIEW_SHOWLETTERBOX, MF_BYCOMMAND);
	bool	checked = (state & MF_CHECKED) != 0;
	UINT	newChecked = MF_BYCOMMAND;

	newChecked |= checked ? MF_UNCHECKED : MF_CHECKED;

	mPopupMenus.CheckMenuItem(ID_SCENEVIEW_SHOWLETTERBOX, newChecked);

   	mShowLetterbox = !checked;

	Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::RenderTexturedClick()
{
    UINT	state = mPopupMenus.GetMenuState(ID_SCENE_VIEW_RENDER_TEXTURED, MF_BYCOMMAND);
	bool	checked = (state & MF_CHECKED) != 0;
	UINT	newChecked = MF_BYCOMMAND;

	newChecked |= checked ? MF_UNCHECKED : MF_CHECKED;

	mPopupMenus.CheckMenuItem(ID_SCENE_VIEW_RENDER_TEXTURED, newChecked);

   	mRenderTextured = !checked;

	Repaint();
}
#if 0
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::ConnectCameraClick()
{
    UINT	state = mPopupMenus.GetMenuState(ID_SCENE_VIEW_CONNECT_CAMERA, MF_BYCOMMAND);
	bool	checked = (state & MF_CHECKED) != 0;
	UINT	newChecked = MF_BYCOMMAND;

	newChecked |= checked ? MF_UNCHECKED : MF_CHECKED;

	mPopupMenus.CheckMenuItem(ID_SCENE_VIEW_CONNECT_CAMERA, newChecked);

}
#endif
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::AssociateCameraClick(unsigned int cameraIdx)
{
	if(cameraIdx == 0)
	{
		AssociateCamera(NULL);
		return;
	}

	ROS::SceneEntityCollection   sceneEntityColl;

    mAssociatedSceneView.GetSceneEntities(sceneEntityColl);

    ROS::SceneEntityCollection::const_iterator   begin = sceneEntityColl.begin();
    const ROS::SceneEntityCollection::const_iterator kEnd = sceneEntityColl.end();

	mMinCommandID = gMaxCommandID;

	int cameraFoundIdx = 0;	// Starting at 0 to account for the <None> case

    while(begin != kEnd)
    {
		ROS::ADynamicCamera*	camera = dynamic_cast<ROS::ADynamicCamera*>(*begin);

		if(camera)
		{
			++cameraFoundIdx;

			if(cameraFoundIdx == cameraIdx)
			{
				AssociateCamera(camera);
				return;
			}
		}

        ++begin;
    }

	ASSERT(0);	// Incorrect cameraIdx
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::SelectCompoundPopupClick(unsigned int idx)
{
	ROS::ASceneEntity*			sceneEntity = mAssociatedSceneView.GetSelectedSceneEntity();
	ROS::ACompoundSceneEntity*	aCompoundEntity = dynamic_cast<ROS::ACompoundSceneEntity*>(sceneEntity);

	if(aCompoundEntity)
	{
		const std::auto_ptr<ROS::ConstStaticsStateAccessor>	constStaticsAccessor = aCompoundEntity->GetConstStaticsStateAccessor();
		const std::auto_ptr<ROS::ConstMotionStateAccessor>	motionAccessor = aCompoundEntity->GetConstMotionStateAccessor();

		const unsigned int hardPointCount = motionAccessor->GetHardPointCount();

		if(idx < (hardPointCount + 1))	// Offsetting by one to account for the <Root> menu item
		{
			// hardpoint to remember was selected
			ROS::Location				location = constStaticsAccessor->GetLocation();
			ROS::Orientation			orientation = constStaticsAccessor->GetOrientation();
			const ROS::HardPointHost*	hardPointHost = NULL;
			ROS::ROSString				hardPointName;
						
			if(idx > 0)
			{
				// User did not select the <Root> entry. Add the hardpoint transform
				const unsigned int	hardPointIdx = idx - 1;		// Offsetting by 1 to account for the <Root> menu item

				const ROS::Location		hardpointLocation = motionAccessor->GetHardPointLocation(hardPointIdx);
				const ROS::Orientation	hardpointOrientation = motionAccessor->GetHardPointOrientation(hardPointIdx);

				const Transform compoundTr(Matrix(orientation.GetI(), orientation.GetJ(), orientation.GetK()), location.GetVector());
				const Transform hardpointTr(Matrix(hardpointOrientation.GetI(), hardpointOrientation.GetJ(), hardpointOrientation.GetK()), hardpointLocation.GetVector());

				const Transform finalTr = compoundTr * hardpointTr;

				Vector	finalLoc = finalTr.get_position();
				Matrix	finalOr = finalTr.get_orientation();

				location = ROS::Location(finalLoc.x, finalLoc.y, finalLoc.z);
				orientation = ROS::Orientation(finalOr.get_i(), finalOr.get_j(), finalOr.get_k());
				hardPointHost = motionAccessor->GetHardPointHost(hardPointIdx);
				hardPointName = motionAccessor->GetHardPointName(hardPointIdx);
			}

			// Remember the position
			mAssociatedSceneView.RememberPosition(ROS::Position(location, orientation));

			// Remember the hardpoint as well
			mAssociatedSceneView.RememberHardPoint(ROS::HardPoint(aCompoundEntity, hardPointHost, hardPointName));
		}
		else if(idx < (2 * (hardPointCount + 1)))	// Offsetting by one to account for the <Root> menu item
		{
			// hardpoint to move was selected
			std::auto_ptr<ROS::StaticsStateAccessor> staticsAccessor = aCompoundEntity->GetStaticsStateAccessor();

			Transform transformInWorld;

			if(idx > (hardPointCount + 1))	// Offsetting by 1 to account for the <Root> menu items
			{
				// User did not select the <Root> entry. Add the hardpoint transform
				const unsigned int	hardPointIdx = idx - ((hardPointCount + 1) + 1);		// Offsetting by 2 to account for the two <Root> menu item

				const ROS::Location hardpointLocation = motionAccessor->GetHardPointLocation(hardPointIdx);
				const ROS::Orientation hardpointOrientation = motionAccessor->GetHardPointOrientation(hardPointIdx);

				const Transform hardpointTransformInLocal(Matrix(hardpointOrientation.GetI(), hardpointOrientation.GetJ(), hardpointOrientation.GetK()), hardpointLocation.GetVector());

				transformInWorld = transformInWorld * hardpointTransformInLocal;
			}

/*				Vector	finalLoc = finalTr.get_position();
				Matrix	finalOr = finalTr.get_orientation();

				location = ROS::Location(finalLoc.x, finalLoc.y, finalLoc.z);
				orientation = ROS::Orientation(finalOr.get_i(), finalOr.get_j(), finalOr.get_k());
*/
			const Transform	inverseTransformInWorld = transformInWorld.get_inverse();

			const ROS::Position		position = mAssociatedSceneView.RecallPosition();
			const ROS::Orientation	orientation = position.GetOrientation();
			const ROS::Location		location = position.GetLocation();

			const Transform transformToRememberedInWorld(Matrix(orientation.GetI(), orientation.GetJ(), orientation.GetK()), location.GetVector());
			const Transform	finalTransformInWorld = transformToRememberedInWorld * inverseTransformInWorld;
			const Vector	finalLoc = finalTransformInWorld.get_position();
			const Matrix	finalOr = finalTransformInWorld.get_orientation();

			const ROS::Location		finalLocation(finalLoc.x, finalLoc.y, finalLoc.z);
			const ROS::Orientation	finalOrientation(finalOr.get_i(), finalOr.get_j(), finalOr.get_k());

			staticsAccessor->SetLocation(finalLocation);
			staticsAccessor->SetOrientation(finalOrientation);

			mAssociatedSceneView.SelectedSceneEntityUpdated();
		}
		else if(idx < (3 * (hardPointCount + 1)))	// Offsetting by one to account for the <Root> menu item
		{
			// Make child was selected
			// Make sure that the parent and child entities aren't the same
			const ROS::HardPoint	parent = mAssociatedSceneView.RecallHardPoint();

			if(parent.GetACompoundSceneEntity() == aCompoundEntity)
			{
				MessageBox(mWndH, "Can't make entity child of itself", "Error", MB_OK);
				
				return;
			}

			// Check if the currently remembered hardpoint is valid.
			if(parent.GetACompoundSceneEntity() == NULL)
			{
				MessageBox(mWndH, "Parent hardpoint unknown", "Error", MB_OK);
				
				return;
			}

			if(parent.GetHardPointName().empty())
			{
				MessageBox(mWndH, "Parent's root cannot serve as hardpoint", "Error", MB_OK);

				return;
			}

			// Parent hardpoint is valid
			// Check if the selected hardpoint to make a child is valid
			if(idx > 2 * (hardPointCount + 1))	// Offsetting by 1 to account for the <Root> menu items
			{
				// The child hardpoint is valid. Now to hook up the child to the parent.
				const unsigned int	hardPointIndex = idx - ((2 * (hardPointCount + 1)) + 1);	// Offsetting by 1 to account for the <Root> menu items

				aCompoundEntity->GetMotionStateAccessor()->AttachHardPointToParent(hardPointIndex, parent);
			}
			else
			{
				MessageBox(mWndH, "Child's root cannot serve as hardpoint", "Error", MB_OK);

				return;
			}

			mAssociatedSceneView.SelectedSceneEntityUpdated();
		}
		else
		{
			ROS::CompoundSceneEntity*	compoundEntity = dynamic_cast<ROS::CompoundSceneEntity*>(aCompoundEntity);

			if(compoundEntity)
			{
				//camera was selected
				const std::auto_ptr<ROS::ConstCompoundStateAccessor>	compoundAccessor = compoundEntity->GetConstCompoundStateAccessor();

				const unsigned int	cameraIndex = idx - (3 * (hardPointCount + 1));	// Offsetting by 1 to account for the <Root> menu items
    			const unsigned int  cameraCount = compoundAccessor->GetCameraCount();
				ASSERT(cameraIndex < cameraCount);

				if(cameraCount > 0)
				{
					ROS::Location location = compoundAccessor->GetCameraLocation(cameraIndex);
					ROS::Orientation orient = compoundAccessor->GetCameraOrientation(cameraIndex);
					float horFOV = compoundAccessor->GetCameraHorizontalFOV(cameraIndex);
					float verFOV = compoundAccessor->GetCameraVerticalFOV(cameraIndex);

    				CameraSetPosition(mCamera, Vector(location.GetX(), location.GetY(), location.GetZ()));
    				CameraSetOrientation(mCamera, orient);
					CameraSetHorizontalFOV(mCamera, horFOV);
					CameraSetVerticalFOV(mCamera, verFOV);

        			SetAssociatedCameraToInternalCamera();
		
					Repaint();
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::StartClick()
{
	ROS::ACompoundSceneEntity* sceneEntity = dynamic_cast<ROS::ACompoundSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

	if(sceneEntity)
    {
		std::auto_ptr<ROS::MotionStateAccessor> motionAccessor = sceneEntity->GetMotionStateAccessor();

		try
		{
			motionAccessor->Start(motionAccessor->GetCurrentMotionName(), ROS::kTime0, ROS::Time(0.5));
		}
		catch(std::exception& ex)
		{
			MessageBox(mWndH, ex.what(), "Error", MB_OK);
			
			return;
		}

        mAssociatedSceneView.SelectedSceneEntityUpdated();

		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_START_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_LOOP_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_PAUSE_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_RESUME_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_STOP_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_PLAY_SOUND, MF_ENABLED);
    }
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::LoopClick()
{
	ROS::ACompoundSceneEntity* sceneEntity = dynamic_cast<ROS::ACompoundSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

	if(sceneEntity)
    {
		std::auto_ptr<ROS::MotionStateAccessor> motionAccessor = sceneEntity->GetMotionStateAccessor();
		
		try
		{
			motionAccessor->Loop(motionAccessor->GetCurrentMotionName(), ROS::kTime0, ROS::Time(0.5));
		}
		catch(std::exception& ex)
		{
			MessageBox(mWndH, ex.what(), "Error", MB_OK);

			return;
		}

        mAssociatedSceneView.SelectedSceneEntityUpdated();

		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_START_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_LOOP_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_PAUSE_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_RESUME_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_STOP_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_PLAY_SOUND, MF_ENABLED);
    }
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::PauseClick()
{
	ROS::ACompoundSceneEntity* sceneEntity = dynamic_cast<ROS::ACompoundSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

	if(sceneEntity)
    {
		std::auto_ptr<ROS::MotionStateAccessor> motionAccessor = sceneEntity->GetMotionStateAccessor();
    	motionAccessor->Pause(motionAccessor->GetCurrentMotionName());

        mAssociatedSceneView.SelectedSceneEntityUpdated();

		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_START_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_LOOP_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_PAUSE_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_RESUME_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_STOP_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_PLAY_SOUND, MF_ENABLED);
    }
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::ResumeClick()
{
	ROS::ACompoundSceneEntity* sceneEntity = dynamic_cast<ROS::ACompoundSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

	if(sceneEntity)
    {
		std::auto_ptr<ROS::MotionStateAccessor> motionAccessor = sceneEntity->GetMotionStateAccessor();
    	motionAccessor->Resume(motionAccessor->GetCurrentMotionName());

        mAssociatedSceneView.SelectedSceneEntityUpdated();

		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_START_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_LOOP_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_PAUSE_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_RESUME_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_STOP_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_PLAY_SOUND, MF_ENABLED);
    }
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::StopClick()
{
	ROS::ACompoundSceneEntity* sceneEntity = dynamic_cast<ROS::ACompoundSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

	if(sceneEntity)
    {
		std::auto_ptr<ROS::MotionStateAccessor> motionAccessor = sceneEntity->GetMotionStateAccessor();
    	motionAccessor->Stop(motionAccessor->GetCurrentMotionName());

        mAssociatedSceneView.SelectedSceneEntityUpdated();

		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_START_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_LOOP_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_PAUSE_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_RESUME_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_STOP_MOTION, MF_ENABLED);
		mPopupMenus.EnableMenuItem(ID_DEFORMABLE_ENTITY_PLAY_SOUND, MF_ENABLED);
    }
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::HardPointsClick()
{
	ROS::ACompoundSceneEntity* sceneEntity = dynamic_cast<ROS::ACompoundSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

	if(sceneEntity)
    {
		UINT	state = mPopupMenus.GetMenuState(ID_COMPOUND_ENTITY_SHOW_HARDPOINTS, MF_BYCOMMAND);
		bool	checked = (state & MF_CHECKED) != 0;
		UINT	newChecked = MF_BYCOMMAND;

		newChecked |= checked ? MF_UNCHECKED : MF_CHECKED;

		mPopupMenus.CheckMenuItem(ID_COMPOUND_ENTITY_SHOW_HARDPOINTS, newChecked);
        
		std::auto_ptr<ROS::MotionStateAccessor> accessor = sceneEntity->GetMotionStateAccessor();
    	
		accessor->ShowHardPoints(!checked);

		Repaint();
    }
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::SkeletonClick()
{
	ROS::ACompoundSceneEntity* sceneEntity = dynamic_cast<ROS::ACompoundSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

	if(sceneEntity)
    {
		UINT	state = mPopupMenus.GetMenuState(ID_DEFORMABLE_ENTITY_SHOW_SKELETON, MF_BYCOMMAND);
		bool	checked = (state & MF_CHECKED) != 0;
		UINT	newChecked = MF_BYCOMMAND;

		newChecked |= checked ? MF_UNCHECKED : MF_CHECKED;

		mPopupMenus.CheckMenuItem(ID_DEFORMABLE_ENTITY_SHOW_SKELETON, newChecked);
        
		std::auto_ptr<ROS::MotionStateAccessor> accessor = sceneEntity->GetMotionStateAccessor();
    	
		accessor->ShowSkeleton(!checked);

		Repaint();
    }
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::MotionPathClick()
{
	ROS::AStaticSceneEntity* sceneEntity = dynamic_cast<ROS::AStaticSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

	if(sceneEntity)
    {
#if 0
		UINT	state = mPopupMenus.GetMenuState(ID_COMPOUND_ENTITY_SHOW_MOTION_PATH, MF_BYCOMMAND);
		bool	checked = (state & MF_CHECKED) != 0;

		UINT	newChecked = MF_BYCOMMAND;

		newChecked |= checked ? MF_UNCHECKED : MF_CHECKED;

		mPopupMenus.CheckMenuItem(ID_COMPOUND_ENTITY_SHOW_MOTION_PATH, newChecked);
#endif        
		std::auto_ptr<ROS::StaticsStateAccessor> accessor = sceneEntity->GetStaticsStateAccessor();
    	
		accessor->SetStaticsPathVisible(!accessor->IsStaticsPathVisible());
    }
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::PlaySoundClick()
{
	CWaitCursor	waitCursor;

	// Put up DB dialog, but only for audio
	CSelectDBEntityDialog	dbEntityDialog(NULL);

	CSelectDBEntityDialog::EntityTypeList	entityTypes;

	entityTypes.push_back(CSelectDBEntityDialog::kAudio);

	dbEntityDialog.SetEntityTypes(entityTypes);

	const int	modalReturn = dbEntityDialog.DoModal();
	
	if(modalReturn == IDCANCEL || modalReturn == -1)
	{
		return;
	}

	waitCursor.Restore();

	// Get the information from the dialog and set up audio event
	const ROS::ROSString					entityName = dbEntityDialog.GetEntityName();
	const ROS::StringList					entityDescriptionStrings = dbEntityDialog.GetEntityDescriptionStrings();
	const CSelectDBEntityDialog::EntityType	entityType = dbEntityDialog.GetEntityType();
	ASSERT(entityType == CSelectDBEntityDialog::kAudio);

    ROS::AAudibleSceneEntity* sceneEntity = dynamic_cast<ROS::AAudibleSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

    if(sceneEntity)
    {
		std::auto_ptr<ROS::AudioStateAccessor> audioAccessor = sceneEntity->GetAudioStateAccessor();
        audioAccessor->Start(entityName, entityDescriptionStrings, ROS::Time(0));

        mAssociatedSceneView.SelectedSceneEntityUpdated();
    }
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::SetToRememberedPositionClick()
{
	ROS::AStaticSceneEntity* staticSceneEntity = dynamic_cast<ROS::AStaticSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

	if(staticSceneEntity)
	{
		const ROS::Position	position = mAssociatedSceneView.RecallPosition();

		std::auto_ptr<ROS::StaticsStateAccessor> stateAccessor = staticSceneEntity->GetStaticsStateAccessor();
		
		stateAccessor->SetLocation(position.GetLocation());
		stateAccessor->SetOrientation(position.GetOrientation());

		mAssociatedSceneView.SelectedSceneEntityUpdated();
	}
}
//---------------------------------------------------------------------------
void __fastcall TGLSceneViewUIForm::SetIKTarget()
{
	ROS::ACompoundSceneEntity* entity = dynamic_cast<ROS::ACompoundSceneEntity*>(mAssociatedSceneView.GetSelectedSceneEntity());

	if(entity)
	{
		CEntityBrowser	browser(&mAssociatedSceneView);
		
		browser.DoModal();

		const unsigned int				generations = browser.GetRootEffectorGenerations();
		ROS::AStaticSceneEntity*		target = browser.GetTargetEntity();
		const ROS::ROSString			endEffectorName = browser.GetEndEffectorName();

		if(generations > 0 && target && !endEffectorName.empty())
		{
			entity->GetMotionStateAccessor()->StartIK(endEffectorName, generations, *target, ROS::Time(0.5));

			mAssociatedSceneView.SelectedSceneEntityUpdated();
		}
	}
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::CreateOpenGLPopupMenu()
{
	mPopupMenus.CheckMenuItem(ID_SCENE_VIEW_MESH, mShowMesh ? MF_CHECKED : MF_UNCHECKED);
	mPopupMenus.CheckMenuItem(ID_SCENE_VIEW_AXES, mShowAxes ? MF_CHECKED : MF_UNCHECKED);
	mPopupMenus.CheckMenuItem(ID_SCENE_VIEW_RENDER_TEXTURED, mRenderTextured? MF_CHECKED : MF_UNCHECKED);
	mPopupMenus.CheckMenuItem(ID_SCENEVIEW_SHOWLETTERBOX, mShowLetterbox ? MF_CHECKED : MF_UNCHECKED);
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::CreateSceneEntityPopupMenu()
{
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::CreateCompoundSceneEntityPopupMenu()
{
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::CreateDeformableSceneEntityPopupMenu()
{
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::CreatePositionMarkerPopupMenu()
{
}
#ifdef PORTED
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::CreateHint()
{
    ASSERT(mOwnerComponent);
    
    mHint = new THintWindow(mOwnerComponent);
}
#endif
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::SetInternalCameraToAssociatedCamera()
{
	if(IsNotNull(mAssociatedCamera))
    {
		std::auto_ptr<ROS::ConstStaticsStateAccessor>	staticsAccessor = mAssociatedCamera->GetConstStaticsStateAccessor();
		std::auto_ptr<ROS::ConstCameraStateAccessor>	cameraAccessor = mAssociatedCamera->GetConstCameraStateAccessor();

    	const Vector			location = staticsAccessor->GetLocation().GetVector();
        const ROS::Orientation	orientation = staticsAccessor->GetOrientation();
		const float				horFOV = cameraAccessor->GetHorizontalFOV();
		const float				verFOV = cameraAccessor->GetVerticalFOV();

		CameraSetPosition(mCamera, location);
		CameraSetOrientation(mCamera, orientation);
		CameraSetHorizontalFOV(mCamera, horFOV);
		CameraSetVerticalFOV(mCamera, verFOV);
    }
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::SetAssociatedCameraToInternalCamera()
{
	if(IsNotNull(mAssociatedCamera))
    {
		Vector				location;
    	ROS::Orientation	orientation;
		float				horFOV, verFOV;

		CameraGetPosition(mCamera, location);
		CameraGetOrientation(mCamera, orientation);
		horFOV = CameraGetHorizontalFOV(mCamera);
		verFOV = CameraGetVerticalFOV(mCamera);

		std::auto_ptr<ROS::StaticsStateAccessor>	staticsAccessor = mAssociatedCamera->GetStaticsStateAccessor();
        std::auto_ptr<ROS::CameraStateAccessor>		cameraAccessor = mAssociatedCamera->GetCameraStateAccessor();

        staticsAccessor->SetLocation(ROS::Location(location.x, location.y, location.z));
        staticsAccessor->SetOrientation(orientation);
		cameraAccessor->SetHorizontalFOV(horFOV);
		cameraAccessor->SetVerticalFOV(verFOV);

        ROS::ASceneEntity* selectedSceneEntity = mAssociatedSceneView.GetSelectedSceneEntity();

        if(selectedSceneEntity != mAssociatedCamera)
        {
			mAssociatedSceneView.SetSecondarySceneEntity(mAssociatedCamera);
            mAssociatedSceneView.SecondarySceneEntityUpdated();
        }
        else
        {
			mAssociatedSceneView.SelectedSceneEntityUpdated();
        }
    }
}
#if 0
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::OpenGLUpdateHint(UINT message)
{
    if(mHint->IsHintMsg(message))
    {
		mHint->ReleaseHandle();
    }
}
#endif
//---------------------------------------------------------------------------
struct RPBITMAPINFO
{
	BITMAPINFOHEADER bmiHeader;
	DWORD            bmiColors[3];
};
//---------------------------------------------------------------------------
HBITMAP TGLSceneViewUIForm::CreateBufferBitmap(HDC hDC, RECT *srcRect, RPBITMAPINFO* bmp_info)
{
	RPLOCKDATA lock_data;

	memset(&lock_data, 0, sizeof(RPLOCKDATA));

	int ret = PIPE->lock_buffer(&lock_data);

	if (ret != GR_OK)
	{
		TRACE0("Failed to lock render pipeline buffer.\n");
		return NULL;
	}

	// Build the appropriate source rectangle.
	int sourceWidth, sourceHeight;
	
	if (srcRect)
	{
		sourceWidth = srcRect->right - srcRect->left;
		sourceHeight = srcRect->bottom - srcRect->top;
		
		if (sourceWidth > lock_data.width)
		{
			sourceWidth = lock_data.width;
		}
		
		if (sourceHeight > lock_data.height)
		{
			sourceHeight = lock_data.height;
		}
	}
	else
	{
		sourceWidth = lock_data.width;
		sourceHeight = lock_data.height;
	}

	// Make a DIB to move the rendered scene into
	ASSERT (lock_data.pf.num_bits() == 16);

	bmp_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmp_info->bmiHeader.biWidth = sourceWidth;
	bmp_info->bmiHeader.biHeight = -sourceHeight;
	bmp_info->bmiHeader.biPlanes = 1;
	bmp_info->bmiHeader.biBitCount = 16;
	bmp_info->bmiHeader.biCompression = BI_BITFIELDS;
	bmp_info->bmiHeader.biSizeImage = 0;
	bmp_info->bmiHeader.biXPelsPerMeter = 0;
	bmp_info->bmiHeader.biYPelsPerMeter = 0;
	bmp_info->bmiHeader.biClrUsed = 0;
	bmp_info->bmiHeader.biClrImportant = 0;
	bmp_info->bmiColors[0] = lock_data.pf.get_r_mask();
	bmp_info->bmiColors[1] = lock_data.pf.get_g_mask();
	bmp_info->bmiColors[2] = lock_data.pf.get_b_mask();

	void* da_bits;
	HBITMAP da_bitmap = CreateDIBSection(hDC, (LPBITMAPINFO) bmp_info, DIB_RGB_COLORS, &da_bits, NULL, 0);
	if (da_bitmap == NULL) 
	{
//		AfxMessageBox ("Failed to create DIB Section for buffer blit!\n");
		TRACE0("Failed to create DIB section for buffer blit!\n");
		PIPE->unlock_buffer();
		return NULL;
	}

	// Get the information about the allocated bitmap.
	DIBSECTION da_dib;

	GetObject (da_bitmap, sizeof(DIBSECTION), &da_dib);

	// There are three ways to copy the data here:
	// 1) Straight copy. Only valid if the dib's pixel format and the buffer's pixel format are
	//    the same, and the buffer's pitch matches that of the DIB
	// 2) Per-line copy. Only valid if the dib's pixel format and the buffer's pixel format match.
	// 3) Convertion copy. When nothing matches.

	bool colorMatch = false;
	bool pitchMatch = false;
	if
	(
		lock_data.pf.get_r_mask() == da_dib.dsBitfields[0] && 
		lock_data.pf.get_g_mask() == da_dib.dsBitfields[1] && 
		lock_data.pf.get_b_mask() == da_dib.dsBitfields[2]
	)
	{
		colorMatch = true;
	}

	if (da_dib.dsBm.bmWidthBytes == lock_data.pitch)
	{
		pitchMatch = true;
	}

	if (pitchMatch && colorMatch)
	{
		// Perform a single copy. Oh happy day.
		memcpy (da_bits, lock_data.pixels, lock_data.width * lock_data.height * sizeof(short));
	}
	else if (colorMatch)
	{
		// Perform a line-by-line copy.
		unsigned char* src_row_start = (unsigned char*)lock_data.pixels;
		unsigned char* dest_row_start = (unsigned char*)da_bits;
		for (int i = 0; i < sourceHeight; ++i)
		{
			memcpy (dest_row_start, src_row_start, sizeof(short) * sourceWidth);
			dest_row_start += da_dib.dsBm.bmWidthBytes;
			src_row_start += lock_data.pitch;
		}
	}
	else
	{
		ASSERT (false);
		// Copy the scene into the DIB -- painfully slow
		unsigned char* src_row_start = (unsigned char*)lock_data.pixels;
		unsigned char* dest_row_start = (unsigned char*)da_bits;

		unsigned short* dest_row_start_t = (unsigned short*)dest_row_start;
		unsigned short* src_row_start_t = (unsigned short*)src_row_start;
		for (int i = 0; i < sourceHeight; i++) 
		{
			for (int j = 0; j < sourceWidth; j++) 
			{
				short src_pixel = *src_row_start_t++;
				U8 red = (src_pixel >> 11);
				U8 green = ((src_pixel >> 6) & 0x001F);
				U8 blue = (src_pixel & 0x001f);
				*dest_row_start_t++ = (red << 10) | (green << 5) | blue;
			}

			src_row_start += lock_data.pitch;
			dest_row_start += da_dib.dsBm.bmWidthBytes;

			dest_row_start_t = (unsigned short*)dest_row_start;
			src_row_start_t = (unsigned short*)src_row_start;
		}
	}

	PIPE->unlock_buffer();

	return da_bitmap;
}
#if 0
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::BltRenderBuffer(HDC hDC, RECT *srcRect, RECT *destRect)
{
	// Copy the contents of the rendered buffer onto the screen
	HBITMAP	da_bitmap = CreateBufferBitmap(hDC, srcRect);

	if(da_bitmap)
	{
		// Put the DIB into a memory device context so we can add to it
		HDC memDC = CreateCompatibleDC(hDC);

		// Select the bitmap into the memory DC, so we can draw on it if needed and
		// blit from it.
		HBITMAP old_bmp = (HBITMAP) SelectObject(memDC, da_bitmap);

		// Blit the DIB to the screen so we can see it.
		// If a destination rect was provided, perform a stretch blit, otherwise to a
		// straight blit.
		SIZE	size;

		GetBitmapDimensionEx(da_bitmap, &size);

		if (destRect)
		{
			StretchBlt
			(
				hDC,
				destRect->left, destRect->top,
				destRect->right - destRect->left, destRect->bottom - destRect->top,
				memDC,
				0,0,
				size.cx, size.cy,
				SRCCOPY
			);
		}
		else
		{
			BitBlt(hDC, 0, 0, size.cx, size.cy, memDC, 0, 0, SRCCOPY);
		}
 		
		// Cleanup the paint
		SelectObject(memDC, old_bmp);
		DeleteObject(da_bitmap);
		DeleteDC (memDC);
	}
}
#endif
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::Resize(int width, int height)
{
	if (width > mPipeHeight)
	{
		width = mPipeWidth;
	}
	if (height > mPipeHeight)
	{
		height = mPipeHeight;
	}

	mWidth = width;
	mHeight = height;

	ViewRect	pane = { 0, 0, width - 1, height - 1 };

	CameraGetBaseCamera(mCamera)->set_pane(&pane);
}
//---------------------------------------------------------------------------

void TGLSceneViewUIForm::OverlayPaint (HDC paintDC)
{
	// If in view translation mode, draw the little start position marker.
	if(mCurrentMode == translateViewMode)
	{
		HPEN hp = (HPEN) GetStockObject (WHITE_PEN);
		HPEN oldPen = (HPEN) SelectObject (paintDC, hp);
		HBRUSH hbHollow = (HBRUSH) GetStockObject(HOLLOW_BRUSH);
		HBRUSH hbBlack = (HBRUSH) GetStockObject(BLACK_BRUSH);
		HBRUSH oldBrush = (HBRUSH) SelectObject (paintDC, hbHollow);

		// Draw a crosshairs at the starting point, and a line to the last position.
		const int rad = 10;
		Ellipse (paintDC, mStartMouseX - rad, mStartMouseY - rad, mStartMouseX + rad, mStartMouseY + rad);
		MoveToEx (paintDC, mStartMouseX, mStartMouseY - rad, NULL);
		LineTo (paintDC, mStartMouseX, mStartMouseY + rad);
		MoveToEx (paintDC, mStartMouseX - rad, mStartMouseY, NULL);
		LineTo (paintDC, mStartMouseX + rad, mStartMouseY);
		MoveToEx (paintDC, mStartMouseX, mStartMouseY, NULL);
		LineTo (paintDC, mLastMouseX, mLastMouseY);
		SelectObject (paintDC, hbBlack);
		Ellipse (paintDC, mLastMouseX - rad, mLastMouseY - rad, mLastMouseX + rad, mLastMouseY + rad);

		SelectObject (paintDC, oldPen);
		SelectObject (paintDC, oldBrush);
	}
	else if(mCurrentMode == viewRectSelectMode)
	{
		HPEN hp = (HPEN) GetStockObject (WHITE_PEN);
		HPEN hbp = (HPEN) GetStockObject (BLACK_PEN);
		HPEN oldPen = (HPEN) SelectObject (paintDC, hp);
		HBRUSH hbHollow = (HBRUSH) GetStockObject(HOLLOW_BRUSH);
		HBRUSH oldBrush = (HBRUSH) SelectObject (paintDC, hbHollow);

		// Draw the rectangle formed by mStartMouseX,Y and mLastMouseX,Y

		int x0, y0, x1, y1;
		if (mStartMouseX < mLastMouseX)
		{
			x0 = mStartMouseX;
			x1 = mLastMouseX;
		}
		else
		{
			x0 = mLastMouseX;
			x1 = mStartMouseX;
		}
		if (mStartMouseY < mLastMouseY)
		{
			y0 = mStartMouseY;
			y1 = mLastMouseY;
		}
		else
		{
			y0 = mLastMouseY;
			y1 = mStartMouseY;
		}

		Rectangle (paintDC, x0, y0, x1, y1);

		// Draw the actual aspect ratio as well
#if 0
		int w, h;
		if (x1 - x0 < y1 - y0)
		{
			w = x1 - x0;
			h = mHeight * w / mWidth;
		}
		else
		{
			h = y1 - y0;
			w = mWidth * h / mHeight;
		}
		SelectObject (paintDC, hbp);
		Rectangle (paintDC, x0, y0, x0 + w, y0 + h);
#endif

		// Clean up
		SelectObject (paintDC, oldPen);
		SelectObject (paintDC, oldBrush);
	}

	// Display the letterbox bars if active.
	if (mShowLetterbox)
	{
		HPEN hp = (HPEN) GetStockObject (WHITE_PEN);
		HPEN oldPen = (HPEN) SelectObject (paintDC, hp);

		int h = mWidth / mLetterboxHToV;
		int y0 = (mHeight - h) / 2;
		int y1 = (mHeight + h) / 2;

		MoveToEx (paintDC, 0, y0, NULL);
		LineTo (paintDC, mWidth, y0);
		MoveToEx (paintDC, 0, y1, NULL);
		LineTo (paintDC, mWidth, y1);

		SelectObject (paintDC, oldPen);
	}
}

//---------------------------------------------------------------------------

bool TGLSceneViewUIForm::OnTimer (UINT wTimerId, TIMERPROC *proc)
{
	// Return true if the message is processed. False to perform default processing.

	if (wTimerId == XLAT_TIMER)
	{
		if (mCurrentMode == translateViewMode)
		{
			// Convert the x and y distances between the start and current mouse positions into
			// x and y translation steps, then apply those steps.
			// For now, use a linear scale of the distance, but in the future it should probably be
			// logrithmic.

			const SINGLE xlatScale = 1.5f; // this is pixels of translation per step per pixel distance.
			const SINGLE deadZone = 0.000000001;

			// Calculate the translation vector on the near clipping plane
			Vector	startRay, endRay;

			// Compute starting point on near clipping plane in world
			CameraGetScreenToPoint(mCamera, mStartMouseX, mStartMouseY, startRay);
			
			// Compute current point on near clipping plane in world
			CameraGetScreenToPoint(mCamera, mLastMouseX, mLastMouseY, endRay);
			
			// Compute translation vector on near clipping plane in world
			Vector displacement = endRay - startRay;

			// Scale it.
			displacement *= xlatScale;

			// Now move the camera by this displacement
			Vector	cameraLocation;
			CameraGetPosition(mCamera, cameraLocation);
			CameraSetPosition(mCamera, cameraLocation + displacement);
			SetAssociatedCameraToInternalCamera();

			// Show the changes.
			Repaint ();
		}
		else
		{
			// Somehow we got a timer message outside of view translation mode.
			// Kill the timer for safety, and report the error.

			OutputDebugString ("Got timer message outside of translate view mode. Killing the timer.\n");
			KillTimer (mWndH, XLAT_TIMER);
		}
	}

	return true;
}
//---------------------------------------------------------------------------
/*
** MakeDib(hbitmap)
**
** Take the given bitmap and transform it into a DIB with parameters:
**
** BitsPerPixel:    8
** Colors:          palette
**
*/
static HANDLE  MakeDib( HBITMAP hbitmap, UINT bits )
{
	HANDLE              hdib ;
	HDC                 hdc ;
	BITMAP              bitmap ;
	UINT                wLineLen ;
	DWORD               dwSize ;
	DWORD               wColSize ;
	LPBITMAPINFOHEADER  lpbi ;
	LPBYTE              lpBits ;
	
	GetObject(hbitmap,sizeof(BITMAP),&bitmap) ;

	//
	// DWORD align the width of the DIB
	// Figure out the size of the colour table
	// Calculate the size of the DIB
	//
	wLineLen = (bitmap.bmWidth*bits+31)/32 * 4;
	wColSize = sizeof(RGBQUAD)*((bits <= 8) ? 1<<bits : 0);
	dwSize = sizeof(BITMAPINFOHEADER) + wColSize +
		(DWORD)(UINT)wLineLen*(DWORD)(UINT)bitmap.bmHeight;

	//
	// Allocate room for a DIB and set the LPBI fields
	//
	hdib = GlobalAlloc(GHND,dwSize);
	if (!hdib)
		return hdib ;

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib) ;

	lpbi->biSize = sizeof(BITMAPINFOHEADER) ;
	lpbi->biWidth = bitmap.bmWidth ;
	lpbi->biHeight = bitmap.bmHeight ;
	lpbi->biPlanes = 1 ;
	lpbi->biBitCount = (WORD) bits ;
	lpbi->biCompression = BI_RGB ;
	lpbi->biSizeImage = dwSize - sizeof(BITMAPINFOHEADER) - wColSize ;
	lpbi->biXPelsPerMeter = 0 ;
	lpbi->biYPelsPerMeter = 0 ;
	lpbi->biClrUsed = (bits <= 8) ? 1<<bits : 0;
	lpbi->biClrImportant = 0 ;

	//
	// Get the bits from the bitmap and stuff them after the LPBI
	//
	lpBits = (LPBYTE)(lpbi+1)+wColSize ;

	hdc = CreateCompatibleDC(NULL) ;

	GetDIBits(hdc,hbitmap,0,bitmap.bmHeight,lpBits,(LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

	// Fix this if GetDIBits messed it up....
	lpbi->biClrUsed = (bits <= 8) ? 1<<bits : 0;

	DeleteDC(hdc) ;
	GlobalUnlock(hdib);

	return hdib ;
}
//---------------------------------------------------------------------------
void TGLSceneViewUIForm::GenerateAVI()
{
	/* first let's make sure we are running on 1.1 */
	WORD	wVer = HIWORD(VideoForWindowsVersion());
	if(wVer < 0x010a)
	{
		 /* oops, we are too old, blow out of here */
		MessageBeep(MB_ICONHAND);
		MessageBox(NULL, "Video for Windows version is too old", "WriteAVI Error", MB_OK|MB_ICONSTOP);
		return;
	}

	HRESULT	hr;

	AVIFileInit();	// 1
	
	//
	// Open the movie file for writing....
	//
	PAVIFILE	pfile = NULL;
	
	hr = AVIFileOpen(&pfile,		    // returned file pointer	// 2
		       "Yes.avi",		            // file name
		       OF_WRITE | OF_CREATE,	    // mode to open file with
		       NULL);			    // use handler determined
	
	if(hr == AVIERR_OK) 
	{
		// And create the stream;
		PAVISTREAM		ps = NULL, psCompressed = NULL;
		AVISTREAMINFO	strhdr;

		// Fill in the header for the video stream....
		// The video stream will run in 30ths of a second....
		HDC	hDC = GetDC(mWndH);

		if(hDC)
		{
#if 0
			RPBITMAPINFO	bmp_info;
			HBITMAP			da_bitmap = CreateBufferBitmap(hDC, NULL, &bmp_info);

			ReleaseDC(mWndH, hDC);

			if(da_bitmap)
			{
				memset(&strhdr, 0, sizeof(strhdr));
				strhdr.fccType                = streamtypeVIDEO;// stream type
				strhdr.fccHandler             = 0;
				strhdr.dwScale                = 1;
				strhdr.dwRate                 = 30;		    // 30 fps
				strhdr.dwSuggestedBufferSize  = bmp_info.bmiHeader.biWidth * (-bmp_info.bmiHeader.biHeight);
				SetRect(&strhdr.rcFrame, 0, 0,		    // rectangle for stream
					bmp_info.bmiHeader.biWidth,
					-bmp_info.bmiHeader.biHeight);

				hr = AVIFileCreateStream(pfile,		    // file pointer
								 &ps,		    // returned stream pointer
								 &strhdr);	    // stream header
		
				if(hr == AVIERR_OK) 
				{
					AVICOMPRESSOPTIONS		opts;
					AVICOMPRESSOPTIONS FAR*	aopts[1] = {&opts};

					memset(&opts, 0, sizeof(AVICOMPRESSOPTIONS));

					if(AVISaveOptions(NULL, 0, 1, &ps, (LPAVICOMPRESSOPTIONS FAR *) &aopts))
					{
						hr = AVIMakeCompressedStream(&psCompressed, ps, &opts, NULL);

						if(hr == AVIERR_OK) 
						{
							hr = AVIStreamSetFormat(psCompressed, 0,
													   &(bmp_info.bmiHeader),	    // stream format
													   bmp_info.bmiHeader.biSize);

							for(unsigned int idx = 0; idx < 1; ++idx)
							{
								hr = AVIStreamWrite(psCompressed,	// stream pointer
									0,				// time of this frame
									1,				// number to write
									&bmp_info.bmiHeader,		// pointer to data
									bmp_info.bmiHeader.biWidth * (-bmp_info.bmiHeader.biHeight),	// size of this frame
									AVIIF_KEYFRAME,			 // flags....
									NULL,
									NULL);
							}
						}
					}
				}

				DeleteObject(da_bitmap);
			}
#else
			RPBITMAPINFO	bmp_info_rp;
			HBITMAP			da_bitmap_rp = CreateBufferBitmap(hDC, NULL, &bmp_info_rp);

			HGLOBAL			globalH = MakeDib(da_bitmap_rp, 8);

			LPBITMAPINFOHEADER	header = (LPBITMAPINFOHEADER)GlobalLock(globalH);

			ReleaseDC(mWndH, hDC);

			DeleteObject(da_bitmap_rp);

			if(header)
			{
				memset(&strhdr, 0, sizeof(strhdr));
				strhdr.fccType                = streamtypeVIDEO;// stream type
				strhdr.fccHandler             = 0;
				strhdr.dwScale                = 1;
				strhdr.dwRate                 = 30;		    // 30 fps
				strhdr.dwSuggestedBufferSize  = header->biWidth * header->biHeight;
				SetRect(&strhdr.rcFrame, 0, 0,		    // rectangle for stream
					header->biWidth,
					header->biHeight);

				hr = AVIFileCreateStream(pfile,		    // file pointer
								 &ps,		    // returned stream pointer
								 &strhdr);	    // stream header
		
				if(hr == AVIERR_OK) 
				{
					AVICOMPRESSOPTIONS		opts;
					AVICOMPRESSOPTIONS FAR*	aopts[1] = {&opts};

					memset(&opts, 0, sizeof(AVICOMPRESSOPTIONS));

					if(AVISaveOptions(NULL, 0, 1, &ps, (LPAVICOMPRESSOPTIONS FAR *) &aopts))
					{
						hr = AVIMakeCompressedStream(&psCompressed, ps, &opts, NULL);

						if(hr == AVIERR_OK) 
						{
							hr = AVIStreamSetFormat(psCompressed, 0,
													   header,	    // stream format
													   header->biSize +
													   header->biClrUsed * sizeof(RGBQUAD));

							for(unsigned int idx = 0; idx < 1; ++idx)
							{
								hr = AVIStreamWrite(psCompressed,	// stream pointer
									0,				// time of this frame
									1,				// number to write
									(LPBYTE)header +// pointer to data
									header->biSize +
									header->biClrUsed * sizeof(RGBQUAD),
									header->biSizeImage,	// size of this frame
									AVIIF_KEYFRAME,			 // flags....
									NULL,
									NULL);
							}
						}
					}
				}

				GlobalUnlock(globalH);
				GlobalFree(globalH);
			}
#endif
			
		}		
		
		AVIFileRelease(pfile);	// 2
	}

	AVIFileExit();	// 1
}
//---------------------------------------------------------------------------
