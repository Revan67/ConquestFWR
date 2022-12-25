//--------------------------------------------------------------------------//
//                                                                          //
//                             MovieCamera.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/MovieCamera.cpp 36    10/24/00 1:04p Jasony $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObjFrame.h"
#include "TObject.h"
#include "TObjTrans.h"
#include "TObjRender.h"
#include "CQLight.h"
#include "ObjList.h"
#include "UserDefaults.h"
#include "Camera.h"
#include "IMovieCamera.h"
#include "DBHotKeys.h"
#include "TResClient.h"
#include "Sector.h"
#include "MGlobals.h"
#include "DrawAgent.h"
#include "Mission.h"
#include "DQuickSave.h"
#include "BaseHotRect.h"
#include <MovieCameraFlags.h>

#include "DMovieCamera.h"
#include "Startup.h"

#include <FileSys.h>
#include <ITextureLibrary.h>
#include <WindowManager.h>

static bool bCameraMode = false;
static const MISSION_DATA * pInitData;
static struct MovieCamera * firstMovieCamera = NULL;
static struct MovieCamera * lookCam;
static bool bViewingCam;

struct MOVIECAMERA_INIT : RenderArch
{
	PARCHETYPE pArchetype;
	U32 archIndex;
	IMeshArchetype * meshArch;
};

struct _NO_VTABLE MovieCamera : public 	ObjectRender<ObjectTransform<
												ObjectFrame<IBaseObject,MOVIE_CAMERA_SAVELOAD,MOVIECAMERA_INIT> > 
											>, BASE_MOVIE_CAMERA_SAVELOAD, IMovieCamera, ISaveLoad,IQuickSaveLoad
{
	BEGIN_MAP_INBOUND(MovieCamera)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IMovieCamera)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	END_MAP()

	MovieCamera * nextCamera;

	MovieCamera();

	~MovieCamera();
	/* IMovieCamera */

	void InitCamera();

	/* IBaseObject */

	virtual U32 GetSystemID (void) const;			// current system

	virtual SINGLE TestHighlight (const RECT & rect);

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);	// set bVisible if possible for any part of object to appear

	virtual void Render (void);

	virtual void MapRender (bool bPing);

	virtual void View (void);

	virtual U32 GetPartID (void) const;

	virtual bool GetMissionData (MDATA & mdata) const;	// return true if message was handled

	virtual void SetPosition (const Vector & position, U32 newSystemID)
	{
		systemID = newSystemID;
		transform.translation = position;
	}

	virtual void SetTransform (const TRANSFORM & _transform, U32 newSystemID)
	{
		systemID = newSystemID;
		transform = _transform;
	}
	/* IPhysicalObject */

	virtual void SetSystemID (U32 newSystemID)
	{
		systemID = newSystemID;
	}

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations (void);

	/* IQuickSaveLoad methods */

	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void);

	//MovieCamera

	void init(MOVIECAMERA_INIT & data);

	void ResetCamera();
};
//--------------------------------------------------------------------------------------
//
MovieCamera::MovieCamera()
{
	nextCamera = firstMovieCamera;
	firstMovieCamera = this;
}
//--------------------------------------------------------------------------------------
//
MovieCamera::~MovieCamera()
{
	MovieCamera * cam = firstMovieCamera;
	MovieCamera * prev = NULL;
	while(cam && cam != this)
	{
		prev = cam;
		cam = cam->nextCamera;
	}
	if(prev)
		prev->nextCamera = nextCamera;
	else
		firstMovieCamera = nextCamera;

	OBJLIST->RemovePartID(this,dwMissionID);
}
//--------------------------------------------------------------------------------------
//
void MovieCamera::InitCamera()
{
	objClass = OC_MOVIECAMERA;
	transform = *(CAMERA->GetTransform());
	Vector k = -(transform.get_k());
	cam_position = transform.translation;
	if(k.z)
	{
		cam_lookAt = cam_position + k*fabs(cam_position.z/k.z);
	}
	else
	{
		cam_lookAt =cam_position + k;
	}
	cam_FOV_x = CAMERA->GetHorizontalFOV();
	cam_FOV_y = CAMERA->GetVerticalFOV();
}
//--------------------------------------------------------------------------------------
//
U32 MovieCamera::GetSystemID (void) const
{
	return systemID;
}
//--------------------------------------------------------------------------------------
//
void MovieCamera::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	bVisible = (GetSystemID() == currentSystem &&
			   (defaults.bEditorMode || bCameraMode) );
	
}
//---------------------------------------------------------------------------
//
SINGLE MovieCamera::TestHighlight (const RECT &rect)
{
	bHighlight = 0;
	if (bVisible)
	{
		const BOOL32 bEditorMode = DEFAULTS->GetDefaults()->bEditorMode;
		
		if (bCameraMode || bEditorMode)
		{
			S32 screenX,screenY;
			CAMERA->PointToScreen(transform.translation,&screenX,&screenY);
			
			
			// only highlight when mouse is over us
			if (rect.left == rect.right && rect.top == rect.bottom || bEditorMode)
			{
				if (screenX-15 < rect.right && screenX+15 > rect.left && screenY-15 < rect.bottom && screenY+15 > rect.top)
				{
					bHighlight = TRUE;
				}
			}
		}
	}

	return 0.0f;
}//--------------------------------------------------------------------------------------
//
void MovieCamera::Render (void)
{
/*	if(bVisible)
	{
		BATCH->set_state(RPR_BATCH,true);
		DisableTextures();
		CAMERA->SetModelView();
		BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		PB.Begin(PB_LINES);
		PB.Color3ub(128,128,200);
		PB.Vertex3f(cam_lookAt.x+100,cam_lookAt.y,cam_lookAt.z);
		PB.Vertex3f(cam_lookAt.x,cam_lookAt.y+100,cam_lookAt.z);
		PB.Vertex3f(cam_lookAt.x,cam_lookAt.y+100,cam_lookAt.z);
		PB.Vertex3f(cam_lookAt.x-100,cam_lookAt.y,cam_lookAt.z);
		PB.Vertex3f(cam_lookAt.x-100,cam_lookAt.y,cam_lookAt.z);
		PB.Vertex3f(cam_lookAt.x,cam_lookAt.y-100,cam_lookAt.z);
		PB.Vertex3f(cam_lookAt.x,cam_lookAt.y-100,cam_lookAt.z);
		PB.Vertex3f(cam_lookAt.x+100,cam_lookAt.y,cam_lookAt.z);
		PB.Vertex3f(cam_lookAt.x,cam_lookAt.y,cam_lookAt.z);
		PB.Vertex3f(cam_position.x,cam_position.y,cam_position.z);
		PB.End();

		if((lookCam != this) || (!bViewingCam))
		{
//			ILight * lights[8];
			LIGHT->deactivate_all_lights();
//			U32 numLights = LIGHT->get_best_lights(lights,8, GetTransform().translation,4000);
//			LIGHT->activate_lights(lights,numLights);
			LIGHTS->ActivateBestLights(transform.translation,8,4000);

			TreeRender(mc.mi,mc.numChildren);
//			ENGINE->render_instance(MAINCAM, instanceIndex,0,LODPERCENT,0,NULL);
		}
	}
*/
}
//--------------------------------------------------------------------------------------
//
void MovieCamera::MapRender (bool bPing)
{
}
//--------------------------------------------------------------------------------------
//
void MovieCamera::View (void)
{
	MOVIE_CAMERA_VIEW viewStruct;
	viewStruct.cam_FOV_x = cam_FOV_x;
	viewStruct.cam_FOV_y = cam_FOV_y;
	viewStruct.cam_lookAt = cam_lookAt;
	viewStruct.cam_position = cam_position;
	viewStruct.partName = partName;
	if (DEFAULTS->GetUserData("MOVIE_CAMERA_VIEW", partName, &viewStruct, sizeof(viewStruct)))
	{
		cam_FOV_x = viewStruct.cam_FOV_x;
		cam_FOV_y = viewStruct.cam_FOV_y;
		cam_lookAt = viewStruct.cam_lookAt;
		cam_position = viewStruct.cam_position;
		partName = viewStruct.partName;
		ResetCamera();
	}
}
//--------------------------------------------------------------------------------------
//
U32 MovieCamera::GetPartID (void) const
{
	return dwMissionID;
}
//--------------------------------------------------------------------------------------
//
bool MovieCamera::GetMissionData (MDATA & mdata) const
{
	mdata.pSaveData = this;
	mdata.pInitData = pInitData;
	return true;
}
//--------------------------------------------------------------------------------------
//
BOOL32 MovieCamera::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "MOVIE_CAMERA_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	MOVIE_CAMERA_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));
	
	save.baseSave = *((BASE_MOVIE_CAMERA_SAVELOAD *)this);

	FRAME_save(save);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//--------------------------------------------------------------------------------------
//
BOOL32 MovieCamera::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "MOVIE_CAMERA_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	MOVIE_CAMERA_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol(fdesc.lpFileName, buffer, &load);
	FRAME_load(load);

	*((BASE_MOVIE_CAMERA_SAVELOAD *)this) = load.baseSave;

	result = 1;

	OBJLIST->AddPartID(this,dwMissionID);
	ResetCamera();

Done:	
	return result;
}
//--------------------------------------------------------------------------------------
//
void MovieCamera::ResolveAssociations (void)
{
}
//--------------------------------------------------------------------------------------
//
void MovieCamera::QuickSave (struct IFileSystem * file)
{
	DAFILEDESC fdesc = partName;
	HANDLE hFile;

	file->CreateDirectory("MT_QCAMERALOAD");
	if (file->SetCurrentDirectory("MT_QCAMERALOAD") == 0)
		CQERROR0("QuickSave failed on directory 'MT_QCAMERALOAD'");

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	if ((hFile = file->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR1("QuickSave failed on part '%s'", fdesc.lpFileName);
	}
	else
	{
		MT_QCAMERALOAD qload;
		DWORD dwWritten;
		
		qload.at = cam_position;
		qload.look = cam_lookAt;
		qload.systemID = systemID;
		qload.fovX = cam_FOV_x;
		qload.fovY = cam_FOV_y;

		file->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		file->CloseHandle(hFile);
	}
}
//--------------------------------------------------------------------------------------
//
void MovieCamera::QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize)
{
	MT_QCAMERALOAD qload;
	MISSION->CorrelateSymbol(szSaveLoadType, buffer, &qload);

	cam_position = qload.at;
	cam_lookAt = qload.look;
	cam_FOV_x = qload.fovX;
	cam_FOV_y = qload.fovY;
	SetSystemID(qload.systemID);

	dwMissionID =  MGlobals::CreateNewPartID(0);
	partName = szInstanceName;
	SetReady(true);

	OBJLIST->AddPartID(this, dwMissionID);
	ResetCamera();
}
//--------------------------------------------------------------------------------------
//
void MovieCamera::QuickResolveAssociations (void)
{
}
//--------------------------------------------------------------------------------------
//
void MovieCamera::init(MOVIECAMERA_INIT & data)
{
	objClass = OC_MOVIECAMERA;
	playerID = 0;
	strcpy(partName,"movieCam");
	FRAME_init(data);
}
//--------------------------------------------------------------------------------------
//
void MovieCamera::ResetCamera()
{
	transform.set_position(cam_position);

	Vector k = cam_position - cam_lookAt;
	k.normalize();
	
	Vector i;
	if(fabs(k.z) == 1.0)
		i = cross_product(Vector(0,1,0),k);
	else
		i = cross_product(Vector(0,0,1),k);
	
	i.normalize();

	Vector j = cross_product(k,i);
	j.normalize();

	transform.set_orientation(Matrix(i,j,k));
}

//------------------------------------------------------------------------------------------
//---------------------------MovieCamera Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE MovieCameraFactory : public IObjectFactory, IEventCallback, ResourceClient<>
{
	struct OBJTYPE : MOVIECAMERA_INIT
	{
		void * operator new (size_t size)
		{
			return calloc(size,1);
		}
		void   operator delete (void *ptr)
		{
			::free(ptr);
		}
		
		OBJTYPE (void)
		{
			archIndex = -1;
		}
		
		~OBJTYPE (void)
		{
			ENGINE->release_archetype(archIndex);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback
	U32 eventHandle;

	bool bResetingLook:1;
	
	TRANSFORM oldTrans;
	SINGLE oldFOV_x,oldFOV_y;
	U32 oldSystemID;

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(MovieCameraFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	END_DACOM_MAP()

	MovieCameraFactory (void) 
	{
	}

	~MovieCameraFactory (void);

	void init (void);

	void Render (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	static BOOL CALLBACK CameraListDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	// IObjectFactory methods 

	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);

	//IEventCallBack
	DEFMETHOD (Notify) (U32 message, void *param);

	// MovieCameraFactory methods 

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}

};
//--------------------------------------------------------------------------//
//
MovieCameraFactory::~MovieCameraFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void MovieCameraFactory::init (void)
{
	lookCam = NULL;
	bResetingLook = false;
	bViewingCam = false;

	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
	{
		connection->Advise(getBase(), &eventHandle);
	}

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);

	initializeResources();

	resPriority = RES_PRIORITY_MEDIUM;	
}
//-----------------------------------------------------------------------------
//
void MovieCameraFactory::Render()
{
	S32 xPos;
	S32 yPos;
	Vector vec;
	MovieCamera * highlight = firstMovieCamera;
	while(highlight)
	{
		if(highlight->GetSystemID() == SECTOR->GetCurrentSystem())
		{
			vec = highlight->GetPosition();
			CAMERA->PointToScreen(vec,&xPos,&yPos);
			DEBUGFONT->StringDraw(0,xPos,yPos, highlight->partName);
		}
		highlight = highlight->nextCamera;
	}
}
//-----------------------------------------------------------------------------
//
HANDLE MovieCameraFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_MOVIECAMERA)
	{
		BT_MOVIE_CAMERA_DATA * data = (BT_MOVIE_CAMERA_DATA *)_data;
		result = new OBJTYPE;
			
		result->pArchetype = ARCHLIST->GetArchetype(szArchname);

		pInitData = &(data->missionData);

		DAFILEDESC fdesc = data->fileName;
		COMPTR<IFileSystem> objFile;

		if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
			TEXLIB->load_library(objFile, 0);
		else
			goto Error;

		if ((result->archIndex = ENGINE->create_archetype(fdesc.lpFileName, objFile)) == INVALID_ARCHETYPE_INDEX)
			goto Error;
	
		goto Done;
	}

Error:
	delete result;
	result = 0;
Done:
	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 MovieCameraFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * MovieCameraFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	MovieCamera * camera = new ObjectImpl<MovieCamera>;

	camera->init(*objtype);

	return camera;
}
//-------------------------------------------------------------------
//
void MovieCameraFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//-------------------------------------------------------------------
//
GENRESULT MovieCameraFactory::Notify (U32 message, void *param)
{
	switch (message)
	{
	case CQE_DEBUG_HOTKEY:
		if (CQFLAGS.bGameActive)
		switch ((U32)param)
		{
		case IDH_CAMERA_LIST:
			{
				bCameraMode = TRUE;
				HWND dialog = CreateDialogParam(hResource, MAKEINTRESOURCE(IDD_CAMERA_LIST), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) CameraListDlgProc, LPARAM(this));
				ShowWindow(dialog,SW_SHOWNORMAL);
				SetWindowPos(dialog,HWND_NOTOPMOST, 20, 20, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
			}
			break;
	
		}
		break;
	case CQE_UPDATE:
		{
			if (bCameraMode)
			{
				if(lookCam)
				{
					desiredOwnedFlags = RF_CURSOR;
					if (ownsResources() == 0)
					{
						grabAllResources();
					}
				}
				Render();
			}
		}
		break;
	case WM_RBUTTONDOWN:
		if (bCameraMode && ownsResources())
		{
			lookCam = NULL;
			desiredOwnedFlags = 0;
			releaseResources();
		}
		break;
		
	case WM_LBUTTONUP:
		if(bResetingLook&&bCameraMode && lookCam && ownsResources())
			bResetingLook = false;
	case WM_MOUSEMOVE:
		{
			if(bResetingLook&&bCameraMode && lookCam && ownsResources())
			{
				Vector vec;
				MSG *msg = (MSG *) param;
				vec.z = 0;
				vec.x = LOWORD(msg->lParam);
				vec.y = HIWORD(msg->lParam);
				if(CAMERA->ScreenToPoint(vec.x, vec.y, 0))
				{
					lookCam->cam_lookAt = vec;
					lookCam->ResetCamera();
					if(bViewingCam)
					{
						Vector temp = lookCam->GetPosition();
						CAMERA->SetWorldRotationPitchRoll(lookCam->GetTransform().get_yaw()*MUL_RAD_TO_DEG,lookCam->GetTransform().get_pitch()*MUL_RAD_TO_DEG,lookCam->GetTransform().get_roll()*MUL_RAD_TO_DEG, 0);
						CAMERA->SetPosition(&temp, 1);
						S32 xPos, yPos;
						if(CAMERA->PointToScreen(vec,&xPos,&yPos,0) !=BEHIND_CAMERA )
						{
							WM->SetCursorPos(xPos,yPos);
						}
					}
				}			
			}
		}
		break;
	case WM_LBUTTONDOWN:
		if (bCameraMode && lookCam && ownsResources())
		{
			bResetingLook = true;
			Vector vec;
			MSG *msg = (MSG *) param;
			vec.z = 0;
			vec.x = LOWORD(msg->lParam);
			vec.y = HIWORD(msg->lParam);
			if(CAMERA->ScreenToPoint(vec.x, vec.y, 0))
			{
				lookCam->cam_lookAt = vec;
				lookCam->ResetCamera();
			}
		}
		break;
	}
	return GR_OK;
}
//-------------------------------------------------------------------
//
BOOL MovieCameraFactory::CameraListDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;
	MovieCameraFactory *cameraMgr = (MovieCameraFactory *)GetWindowLong(hwnd, DWL_USER);

	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND hList = GetDlgItem(hwnd,IDC_CL_LIST);

			SetWindowLong(hwnd, DWL_USER, lParam);

			cameraMgr = (MovieCameraFactory *)lParam;

			MovieCamera * cam = firstMovieCamera;
			U32 cnt = 0;
			while (cam)
			{	
				SendMessage(hList, LB_INSERTSTRING, cnt, (LPARAM)(cam->partName.string));
				SendMessage(hList, LB_SETITEMDATA, cnt, (DWORD)(cam));
				cnt++;
				cam = cam->nextCamera;
			}

			SetFocus(GetDlgItem(hwnd, IDC_CL_LIST));
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CL_LIST:
			{
				switch (HIWORD(wParam))
				{
				case LBN_DBLCLK:
					U32 sel;
					MovieCamera * cam;
					HWND hList = GetDlgItem(hwnd,IDC_CL_LIST);

					sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
					cam = (MovieCamera *)(SendMessage(hList,LB_GETITEMDATA,sel,0));
					CQASSERT(cam);
					cam->View();			
					SendMessage(hList,LB_DELETESTRING,sel,0);
					SendMessage(hList,LB_INSERTSTRING,sel,(LPARAM)(cam->partName.string));
					SendMessage(hList, LB_SETITEMDATA, sel, (DWORD)(cam));
					break;
				}
			}
			break;
		case IDC_CL_NEW:
			{
//				U32 systemID = SECTOR->GetCurrentSystem();

				U32 partID = MGlobals::CreateNewPartID(0);
				IBaseObject * rtObject = MGlobals::CreateInstance(ARCHLIST->LoadArchetype("MISSION!!CAMERA"), partID);

				if (rtObject)
				{
					OBJPTR<IPhysicalObject> physObj;
					OBJPTR<IMovieCamera> cameraObj;
				
					OBJLIST->AddObject(rtObject);

					rtObject->QueryInterface(IPhysicalObjectID, physObj);
					rtObject->QueryInterface(IMovieCameraID, cameraObj);

					cameraObj->InitCamera();
					physObj->SetSystemID(SECTOR->GetCurrentSystem());
					ENGINE->update_instance(physObj.Ptr()->GetObjectIndex(),0,0);
					HWND hList = GetDlgItem(hwnd,IDC_CL_LIST);
					MovieCamera * cam = (MovieCamera *) rtObject;
					SendMessage(hList,LB_INSERTSTRING,0,(LPARAM)(cam->partName.string));
					SendMessage(hList, LB_SETITEMDATA, 0, (DWORD)(cam));
				}
			}
			break;
		case IDC_CL_DELETE:
			{
				U32 sel;
				MovieCamera * cam;
				HWND hList = GetDlgItem(hwnd,IDC_CL_LIST);

				sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
				if(sel != LB_ERR)
				{
					cam = (MovieCamera *)(SendMessage(hList,LB_GETITEMDATA,sel,0));
					CQASSERT(cam);
					OBJLIST->DeferredDestruction(cam->GetPartID());
					SendMessage(hList,LB_DELETESTRING,sel,0);
				}
			}
			break;
		case IDC_CL_SETLOOK:
			{
				if(lookCam)
				{
					lookCam = NULL;
					cameraMgr->desiredOwnedFlags = 0;
					cameraMgr->releaseResources();
				}
				else
				{
					U32 sel;
					HWND hList = GetDlgItem(hwnd,IDC_CL_LIST);

					sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
					if(sel != LB_ERR)
					{
						lookCam = (MovieCamera *)(SendMessage(hList,LB_GETITEMDATA,sel,0));
						CQASSERT(lookCam);
					}
				}
			}
			break;
		case IDC_CL_SETPOS:
			{
				U32 sel;
				MovieCamera * cam;
				HWND hList = GetDlgItem(hwnd,IDC_CL_LIST);

				sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
				if(sel != LB_ERR)
				{
					cam = (MovieCamera *)(SendMessage(hList,LB_GETITEMDATA,sel,0));
					CQASSERT(cam);
					cam->cam_position = CAMERA->GetPosition();
					cam->ResetCamera();
				}
			}
			break;
		case IDC_CL_VIEW:
			{
				U32 sel;
				MovieCamera * cam;
				HWND hList = GetDlgItem(hwnd,IDC_CL_LIST);

				sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
				if(sel != LB_ERR)
				{
					cam = (MovieCamera *)(SendMessage(hList,LB_GETITEMDATA,sel,0));
					CQASSERT(cam);
					cam->View();
					SendMessage(hList,LB_DELETESTRING,sel,0);
					SendMessage(hList,LB_INSERTSTRING,sel,(LPARAM)(cam->partName.string));
					SendMessage(hList, LB_SETITEMDATA, sel, (DWORD)(cam));
				}
			}
			break;
		case IDC_CL_GOTO:
			{
				U32 sel;
				MovieCamera * cam;
				HWND hList = GetDlgItem(hwnd,IDC_CL_LIST);

				sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
				if(sel != LB_ERR)
				{
					cam = (MovieCamera *)(SendMessage(hList,LB_GETITEMDATA,sel,0));
					CQASSERT(cam);
					SECTOR->SetCurrentSystem(cam->systemID);
					CAMERA->SetLookAtPosition(cam->GetPosition());
				}
			}
			break;
		case IDC_CL_LOOKFROM:
			{
				if(bViewingCam)
				{
					CQFLAGS.bMovieMode = false;
					EVENTSYS->Send(CQE_MOVIE_MODE,false);

					bViewingCam = false;
					SECTOR->SetCurrentSystem(cameraMgr->oldSystemID);
					CAMERA->SetWorldRotationPitchRoll(cameraMgr->oldTrans.get_yaw()*MUL_RAD_TO_DEG,cameraMgr->oldTrans.get_pitch()*MUL_RAD_TO_DEG,cameraMgr->oldTrans.get_roll()*MUL_RAD_TO_DEG, 0);
					CAMERA->SetPosition(&(cameraMgr->oldTrans.translation), 1);
//					CAMERA->SetHorizontalFOV(cameraMgr->oldFOV_x);
//					CAMERA->SetVerticalFOV(cameraMgr->oldFOV_y);
				}
				else
				{
					CQFLAGS.bMovieMode = true;
					EVENTSYS->Send(CQE_MOVIE_MODE,(void *)true);

					U32 sel;
					MovieCamera * cam;
					HWND hList = GetDlgItem(hwnd,IDC_CL_LIST);

					sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
					if(sel != LB_ERR)
					{
						cam = (MovieCamera *)(SendMessage(hList,LB_GETITEMDATA,sel,0));
						CQASSERT(cam);
						bViewingCam = true;
						cameraMgr->oldTrans = *(CAMERA->GetTransform());
//						cameraMgr->oldFOV_x = CAMERA->GetHorizontalFOV();
//						cameraMgr->oldFOV_y = CAMERA->GetVerticalFOV();
						cameraMgr->oldSystemID = SECTOR->GetCurrentSystem();
						SECTOR->SetCurrentSystem(cam->systemID);
						Vector temp = cam->GetPosition();
						CAMERA->SetWorldRotationPitchRoll(cam->GetTransform().get_yaw()*MUL_RAD_TO_DEG,cam->GetTransform().get_pitch()*MUL_RAD_TO_DEG,cam->GetTransform().get_roll()*MUL_RAD_TO_DEG, 0);
						CAMERA->SetPosition(&temp, 1);
//						CAMERA->SetHorizontalFOV(cam->cam_FOV_x);
//						CAMERA->SetVerticalFOV(cam->cam_FOV_y);
					}
				}
			}
			break;
		}
		break;
	case WM_CLOSE:
		{
			if(bViewingCam)
			{
				bViewingCam = false;
				SECTOR->SetCurrentSystem(cameraMgr->oldSystemID);
				Vector temp = cameraMgr->oldTrans.translation;
				CAMERA->SetWorldRotationPitchRoll(cameraMgr->oldTrans.get_yaw()*MUL_RAD_TO_DEG,cameraMgr->oldTrans.get_pitch()*MUL_RAD_TO_DEG,cameraMgr->oldTrans.get_roll()*MUL_RAD_TO_DEG, 0);
				CAMERA->SetPosition(&temp, 1);
//				CAMERA->SetHorizontalFOV(cameraMgr->oldFOV_x);
//				CAMERA->SetVerticalFOV(cameraMgr->oldFOV_y);
			}
			cameraMgr->desiredOwnedFlags = 0;
			cameraMgr->releaseResources();
			bCameraMode = false;
			lookCam = NULL;
			EndDialog(hwnd, 0);
		}
		break;
	}
	return result;
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _movieCam : GlobalComponent
{
	MovieCameraFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<MovieCameraFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _movieCam __movieCam;


//--------------------------------------------------------------------------------------
//
struct CamSpot
{
	CamSpot * nextSpot;
	SINGLE duration;
	U32 flags;
	Vector position;
	Vector lookAt;
	U32 systemID;
	bool regularMetrics;

    void * operator new (size_t size)
	{
		void * result = calloc(size, 1);
		{
			DWORD dwAddr;
			__asm
			{
				mov eax, DWORD PTR [EBP+4]
				mov DWORD PTR dwAddr, eax
			}
			HEAP->SetBlockOwner(result, dwAddr);
		}
		return result;
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
};
//--------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE MovieCameraManager : public IMovieCameraManager, IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(MovieCameraManager)
	DACOM_INTERFACE_ENTRY(IMovieCameraManager)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	CamSpot savePos;

	CamSpot * spotListStart;
	CamSpot * spotListEnd;
	SINGLE timer;
	CamSpot * lastSpot;
	OBJPTR<IBaseObject> sourceShip;
	U32 sourceShipID;
	Vector sourceOffset;
	OBJPTR<IBaseObject> targetShip;
	U32 targetShipID;

	bool bTracking;

	U32 eventHandle;		// connection handle
	
	MovieCameraManager();

	~MovieCameraManager();

	/* IMovieCameraManager */

  	//for switching to a specific camera object
	virtual void ChangeCamera (IBaseObject * camera, SINGLE time = 0.0, U32 flags=0);

	//for moving to a normal location(vied just like it was in the game)
	virtual void MoveCamera (Vector * location,U32 systemID,SINGLE time = 0.0, U32 flags=0);
	
	//for moving to an object from the regular game view.
	virtual void MoveCamera (IBaseObject * location,SINGLE time = 0.0, U32 flags=0);

	//Sets the source ship to be used with an upcomming ChangeCamera call
	virtual void SetSourceShip (IBaseObject * sourceShip, Vector * offset);

	//Sets the target ship to be used with an upcomming Change Camera call
	virtual void SetTargetShip (IBaseObject * targetShip);

	//Clears the Queue of all Camera calls
	virtual void ClearQueue ();

	//Saves the current position of the camera
	virtual void SaveCameraPos ();

	//Creates a ChangeCamera call that used the saved camera
	virtual void LoadCameraPos (SINGLE time = 0.0, U32 flags=0);

	void PhysUpdate (SINGLE dt);

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveMovieCamera();

	//
	// IEventCallback methods 
	//

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* cameraManager methods */

	void ResetCamera ()
	{
		CAMERA->SetWorldRotationPitchRoll(0,-40,0);
	};

	void CreateLastSpot()
	{
		if(!lastSpot)
		{
			lastSpot = new CamSpot;
		}
		TRANSFORM transform = *(CAMERA->GetTransform());
		Vector k = -(transform.get_k());
		lastSpot->position = transform.translation;
		if(k.z)
		{
			lastSpot->lookAt = lastSpot->position + k*fabs(lastSpot->position.z/k.z);
		}
		else
		{
			lastSpot->lookAt =lastSpot->position + k;
		}
		lastSpot->systemID = SECTOR->GetCurrentSystem();
	}

	SINGLE FindNextSpot(Vector & p2, Vector & l2, U32 & systemID);

	Vector InterVector(Vector v1, Vector v2, SINGLE t);

	void SetCamera(Vector position,Vector lookAt,U32 systemID);

	void init();

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *>(this);
	}
};
//--------------------------------------------------------------------------//
//
MovieCameraManager::MovieCameraManager()
{
};
//--------------------------------------------------------------------------//
//

MovieCameraManager::~MovieCameraManager()
{
	delete lastSpot;
	lastSpot = 0;

	if (TOOLBAR)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}
};

//--------------------------------------------------------------------------//
//
  	//for switching to a specific camera object
void MovieCameraManager::ChangeCamera(IBaseObject * camera, SINGLE time, U32 flags)
{
	if(!camera)
	{
		CQASSERT(camera);
		return;
	}
	bTracking = false;
	MovieCamera * cam = (MovieCamera *)camera;
	CamSpot * spot = new CamSpot;
	spot->duration = time;
	spot->flags = flags;
	spot->position = cam->cam_position;
	spot->lookAt = cam->cam_lookAt;
	spot->regularMetrics = false;
	spot->systemID = cam->GetSystemID();
	spot->nextSpot = NULL;
	if((flags & MOVIE_CAMERA_QUEUE) && spotListStart)
	{
		spotListEnd->nextSpot = spot;
		spotListEnd = spot;
	}
	else
	{
		timer = 0;
		ClearQueue();
		spotListStart = spot;
		spotListEnd = spot;
		CreateLastSpot();
		PhysUpdate(0.01);		// do it now!
	}
};
//--------------------------------------------------------------------------//
//
	//for moving to a normal location(vied just like it was in the game)
void MovieCameraManager::MoveCamera(Vector * location,U32 systemID,SINGLE time, U32 flags)
{
	bTracking = false;
	CamSpot * spot = new CamSpot;
	spot->duration = time;
	spot->flags = flags;
	spot->lookAt = *(location);
	spot->regularMetrics = true;
	spot->systemID = systemID;
	spot->nextSpot = NULL;
	if((flags & MOVIE_CAMERA_QUEUE) && spotListStart)
	{
		spotListEnd->nextSpot = spot;
		spotListEnd = spot;
	}
	else
	{
		timer = 0;
		ClearQueue();
		spotListStart = spot;
		spotListEnd = spot;
		CreateLastSpot();
		spot->position = lastSpot->position-lastSpot->lookAt;
	}
}
//--------------------------------------------------------------------------//
//
	//for moving to an object from the regular game view.
void MovieCameraManager::MoveCamera(IBaseObject * location, SINGLE time, U32 flags)
{
	// if not queuing things and, no time, then just do it
	if (spotListStart==0 && time==0)
	{
		SECTOR->SetCurrentSystem(location->GetSystemID());
		CAMERA->SetLookAtPosition(location->GetPosition());
	}
	else
	{
		bTracking = false;
		CamSpot * spot = new CamSpot;
		spot->duration = time;
		spot->flags = flags;
		spot->lookAt = location->GetPosition();
		spot->regularMetrics = true;
		spot->systemID = location->GetSystemID();
		spot->nextSpot = NULL;
		if((flags & MOVIE_CAMERA_QUEUE) && spotListStart)
		{
			spotListEnd->nextSpot = spot;
			spotListEnd = spot;
		}
		else
		{
			timer = 0;
			ClearQueue();
			spotListStart = spot;
			spotListEnd = spot;
			CreateLastSpot();
			spot->position = lastSpot->position-lastSpot->lookAt;
		}	
	}
}
//--------------------------------------------------------------------------//
//
	//Sets the source ship to be used with an upcomming ChangeCamera call
void MovieCameraManager::SetSourceShip(IBaseObject * _sourceShip, Vector * offset)
{
	if (_sourceShip)
		_sourceShip->QueryInterface(IBaseObjectID, sourceShip, NONSYSVOLATILEPTR);
	else
		sourceShip = 0;

	if(sourceShip)
		sourceShipID = sourceShip->GetPartID();
	else
		sourceShipID = NULL;
	if(offset)
		sourceOffset = *offset;
	else
		sourceOffset = Vector(0,0,0);
}
//--------------------------------------------------------------------------//
//
	//Sets the target ship to be used with an upcomming Change Camera call
void MovieCameraManager::SetTargetShip(IBaseObject * _targetShip)
{
	if (_targetShip)
		_targetShip->QueryInterface(IBaseObjectID, targetShip, NONSYSVOLATILEPTR);
	else
		targetShip = 0;
	if(targetShip)
		targetShipID = targetShip->GetPartID();
	else
		targetShipID = NULL;
}
//--------------------------------------------------------------------------//
//
	//Clears the Queue of all Camera calls
void MovieCameraManager::ClearQueue()
{
	bTracking = false;
	if(lastSpot)
		delete lastSpot;
	lastSpot = NULL;
	while(spotListStart)
	{
		spotListEnd = spotListStart;
		spotListStart = spotListStart->nextSpot;
		delete spotListEnd;
	}
	spotListEnd = NULL;
}
//--------------------------------------------------------------------------//
//
	//Saves the current position of the camera
void MovieCameraManager::SaveCameraPos()
{
	TRANSFORM transform = *(CAMERA->GetTransform());
	Vector k = -(transform.get_k());
	savePos.position = transform.translation;
	if(k.z)
	{
		savePos.lookAt = savePos.position + k*fabs(savePos.position.z/k.z);
	}
	else
	{
		savePos.lookAt =savePos.position + k;
	}
	savePos.systemID = SECTOR->GetCurrentSystem();
}
//--------------------------------------------------------------------------//
//
	//Creates a ChangeCamera call that used the saved camera
void MovieCameraManager::LoadCameraPos(SINGLE time, U32 flags)
{
	bTracking = false;
	CamSpot * spot = new CamSpot;
	spot->duration = time;
	spot->flags = flags;
	spot->position = savePos.position;
	spot->lookAt = savePos.lookAt;
	spot->regularMetrics = false;
	spot->systemID = savePos.systemID;
	spot->nextSpot = NULL;
	if((flags & MOVIE_CAMERA_QUEUE) && spotListStart)
	{
		spotListEnd->nextSpot = spot;
		spotListEnd = spot;
	}
	else
	{
		timer = 0;
		ClearQueue();
		spotListStart = spot;
		spotListEnd = spot;
		CreateLastSpot();
	}
}
//--------------------------------------------------------------------------//
//
SINGLE MovieCameraManager::FindNextSpot(Vector & p2, Vector & l2, U32 & systemID)
{
	if(timer > spotListStart->duration)
	{
		if(spotListStart->nextSpot)
		{
			timer -= spotListStart->duration;
			if(lastSpot)
				delete lastSpot;
			lastSpot = spotListStart;
			if((lastSpot->flags & MOVIE_CAMERA_FROM_SHIP) && sourceShip)
				lastSpot->position = sourceShip->GetPosition();
			if((lastSpot->flags & MOVIE_CAMERA_TRACK_SHIP) && targetShip)
				lastSpot->lookAt = targetShip->GetPosition();
			if(lastSpot->regularMetrics)
				lastSpot->position = lastSpot->lookAt+lastSpot->position;
			spotListStart = spotListStart->nextSpot;
			systemID = lastSpot->systemID;
			if(spotListStart->regularMetrics)
				spotListStart->position = lastSpot->position-lastSpot->lookAt;
			return FindNextSpot(p2,l2,systemID);
		}
		else
		{
			timer = 0;
			
			if((spotListStart->flags & MOVIE_CAMERA_TRACK_SHIP) && targetShip)
				l2 = targetShip->GetPosition();
			else
				l2 = spotListStart->lookAt;	

			if((spotListStart->flags & MOVIE_CAMERA_FROM_SHIP) && sourceShip)
				p2 = sourceShip->GetPosition();
			else
			{
				if(spotListStart->regularMetrics)
					p2 = l2+spotListStart->position;//Vector(120000*tan(-40),0,120000);
				else
					p2 = spotListStart->position;
			}

			systemID = spotListStart->systemID;

			if(lastSpot)
				delete lastSpot;
			lastSpot = spotListStart;
			if((lastSpot->flags & MOVIE_CAMERA_FROM_SHIP) && sourceShip)
				lastSpot->position = sourceShip->GetPosition();
			else if(lastSpot->regularMetrics)
				lastSpot->position = l2+Vector(120000*tan(-40.0f),0,120000);
			if((lastSpot->flags & MOVIE_CAMERA_TRACK_SHIP) && targetShip)
				lastSpot->lookAt = targetShip->GetPosition();
			if(lastSpot->regularMetrics)
				lastSpot->position = lastSpot->lookAt+lastSpot->position;
			spotListStart = NULL;
			spotListEnd = NULL;

			if(lastSpot->flags & (MOVIE_CAMERA_TRACK_SHIP | MOVIE_CAMERA_FROM_SHIP))
			{
				bTracking = true;
			}
			return 0;
		}
	}
	else
	{
		if(spotListStart->flags & MOVIE_CAMERA_JUMP_TO)
		{
			l2 = lastSpot->lookAt;
			if(spotListStart->regularMetrics)
				p2 = l2+spotListStart->position;
			else
				p2 = spotListStart->position;

			systemID = lastSpot->systemID;
		}
		else
		{
			if((spotListStart->flags & MOVIE_CAMERA_TRACK_SHIP) && targetShip)
				l2 = targetShip->GetPosition();
			else
				l2 = spotListStart->lookAt;
			if((spotListStart->flags & MOVIE_CAMERA_FROM_SHIP) && sourceShip)
				p2 = sourceShip->GetPosition();
			else
			{
				if(spotListStart->regularMetrics)
					p2 = l2+spotListStart->position;//Vector(120000*tan(-40),0,120000);
				else
					p2 = spotListStart->position;
			}

			systemID = spotListStart->systemID;
		}
		return spotListStart->duration;
	}
}
//--------------------------------------------------------------------------//
//
void MovieCameraManager::PhysUpdate(SINGLE dt)
{
	if(spotListStart)
	{
		Vector position,lookAt;
		Vector p1,p2,l1,l2;
		U32 systemID;

		timer += dt;
		systemID = lastSpot->systemID;
		SINGLE durration = FindNextSpot(p2,l2,systemID);
		l1 = lastSpot->lookAt;
		p1 = lastSpot->position;

		if(durration != 0.0)
			durration = timer/durration;
		else
			durration = 1;

		position = InterVector(p1,p2,durration);
		lookAt= InterVector(l1,l2,durration);

		SetCamera(position,lookAt,systemID);
	} else if(bTracking)
	{
		Vector position,lookAt;

		if((lastSpot->flags & MOVIE_CAMERA_TRACK_SHIP) && targetShip)
			lookAt = targetShip->GetPosition();
		else
			lookAt = lastSpot->lookAt;	

		if((lastSpot->flags & MOVIE_CAMERA_FROM_SHIP) && sourceShip)
			position = sourceShip->GetPosition();
		else
		{
			if(lastSpot->regularMetrics)
			{
				position = lookAt+Vector(120000*tan(-40.0f),0,120000);
			}
			else
				position = lastSpot->position;
		}
		
		SetCamera(position,lookAt,lastSpot->systemID);
	}
}
//--------------------------------------------------------------------------//
//
BOOL32 MovieCameraManager::Save (struct IFileSystem * outFile)
{
	outFile->SetCurrentDirectory("\\");
	U32 dwWritten;
	U8 lastOn;
	U32 i = 0;

	CamSpot * search;
	
	COMPTR<IFileSystem> file;

	DAFILEDESC fdesc;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	
	fdesc.lpFileName = "MovieManager";

	if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->WriteFile(0,&savePos,sizeof(savePos),&dwWritten);
	file->WriteFile(0,&timer,sizeof(timer),&dwWritten);
	file->WriteFile(0,&sourceShipID,sizeof(sourceShipID),&dwWritten);
	file->WriteFile(0,&sourceOffset,sizeof(sourceOffset),&dwWritten);
	file->WriteFile(0,&targetShipID,sizeof(targetShipID),&dwWritten);
	file->WriteFile(0,&bTracking,sizeof(bTracking),&dwWritten);

	search = spotListStart;
	while(search)
	{
		++i;
		search = search->nextSpot;
	}

	file->WriteFile(0,&i,sizeof(i),&dwWritten);

	search = spotListStart;
	while(search)
	{
		file->WriteFile(0,&(search->duration),sizeof(search->duration),&dwWritten);
		file->WriteFile(0,&(search->flags),sizeof(search->flags),&dwWritten);
		file->WriteFile(0,&(search->position),sizeof(search->position),&dwWritten);
		file->WriteFile(0,&(search->lookAt),sizeof(search->lookAt),&dwWritten);
		file->WriteFile(0,&(search->systemID),sizeof(search->systemID),&dwWritten);
		file->WriteFile(0,&(search->regularMetrics),sizeof(search->regularMetrics),&dwWritten);
		
		search = search->nextSpot;
	}

	if(lastSpot)
		lastOn = 1;
	else
		lastOn = 0;
	file->WriteFile(0,&lastOn,sizeof(lastOn),&dwWritten);
	if(lastSpot)
	{
		file->WriteFile(0,&(lastSpot->duration),sizeof(search->duration),&dwWritten);
		file->WriteFile(0,&(lastSpot->flags),sizeof(search->flags),&dwWritten);
		file->WriteFile(0,&(lastSpot->position),sizeof(search->position),&dwWritten);
		file->WriteFile(0,&(lastSpot->lookAt),sizeof(search->lookAt),&dwWritten);
		file->WriteFile(0,&(lastSpot->systemID),sizeof(search->systemID),&dwWritten);
		file->WriteFile(0,&(lastSpot->regularMetrics),sizeof(search->regularMetrics),&dwWritten);
	}
	return TRUE;
	
Done:
	return FALSE;
}
//--------------------------------------------------------------------------//
//
BOOL32 MovieCameraManager::Load (struct IFileSystem * inFile)
{
	inFile->SetCurrentDirectory("\\");
	DAFILEDESC fdesc = "MovieManager";
	COMPTR<IFileSystem> file;
	U32 result = FALSE;
	U32 dwRead;
	U32 numSpots;
	U8 lastOn;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0,&savePos,sizeof(savePos),&dwRead);
	file->ReadFile(0,&timer,sizeof(timer),&dwRead);
	file->ReadFile(0,&sourceShipID,sizeof(sourceShipID),&dwRead);
	file->ReadFile(0,&sourceOffset,sizeof(sourceOffset),&dwRead);
	file->ReadFile(0,&targetShipID,sizeof(targetShipID),&dwRead);
	file->ReadFile(0,&bTracking,sizeof(bTracking),&dwRead);

	file->ReadFile(0,&numSpots,sizeof(numSpots),&dwRead);

	while(numSpots)
	{
		CamSpot * newSpot = new CamSpot;
		newSpot->nextSpot = NULL;
		if(spotListEnd)
		{
			spotListEnd->nextSpot = newSpot;
			spotListEnd = newSpot;
		}
		else
		{
			spotListEnd = newSpot;
			spotListStart = newSpot;
		}
		file->ReadFile(0,&(newSpot->duration),sizeof(newSpot->duration),&dwRead);
		file->ReadFile(0,&(newSpot->flags),sizeof(newSpot->flags),&dwRead);
		file->ReadFile(0,&(newSpot->position),sizeof(newSpot->position),&dwRead);
		file->ReadFile(0,&(newSpot->lookAt),sizeof(newSpot->lookAt),&dwRead);
		file->ReadFile(0,&(newSpot->systemID),sizeof(newSpot->systemID),&dwRead);
		file->ReadFile(0,&(newSpot->regularMetrics),sizeof(newSpot->regularMetrics),&dwRead);

		--numSpots;
	}

	file->ReadFile(0,&lastOn,sizeof(lastOn),&dwRead);
	if(lastOn)
	{
		lastSpot = new CamSpot;
		lastSpot->nextSpot = NULL;
		file->ReadFile(0,&(lastSpot->duration),sizeof(lastSpot->duration),&dwRead);
		file->ReadFile(0,&(lastSpot->flags),sizeof(lastSpot->flags),&dwRead);
		file->ReadFile(0,&(lastSpot->position),sizeof(lastSpot->position),&dwRead);
		file->ReadFile(0,&(lastSpot->lookAt),sizeof(lastSpot->lookAt),&dwRead);
		file->ReadFile(0,&(lastSpot->systemID),sizeof(lastSpot->systemID),&dwRead);
		file->ReadFile(0,&(lastSpot->regularMetrics),sizeof(lastSpot->regularMetrics),&dwRead);
	}
	else
	{
		lastSpot = NULL;
	}
	
	result = TRUE;
Done:
	return result;
}
//--------------------------------------------------------------------------//
//
void MovieCameraManager::ResolveMovieCamera()
{
	if(sourceShipID)
		OBJLIST->FindObject(sourceShipID,NONSYSVOLATILEPTR,sourceShip);
	if(targetShipID)
		OBJLIST->FindObject(targetShipID,NONSYSVOLATILEPTR,targetShip);
}
//--------------------------------------------------------------------------//
//
GENRESULT MovieCameraManager::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_MISSION_CLOSE:
		ClearQueue();
		break;
	}

	return GR_OK;
}

//--------------------------------------------------------------------------//
//
Vector MovieCameraManager::InterVector(Vector v1, Vector v2, SINGLE t)
{
	return (v2-v1)*t+v1;
}
//--------------------------------------------------------------------------//
//
void MovieCameraManager::SetCamera(Vector position,Vector lookAt,U32 systemID)
{
	SECTOR->SetCurrentSystem(systemID);
	TRANSFORM transform;
	transform.set_position(position);

	Vector k = position - lookAt;
	k.normalize();
	
	Vector i;
	if(fabs(k.z) == 1.0)
		i = cross_product(Vector(0,1,0),k);
	else
		i = cross_product(Vector(0,0,1),k);
	
	i.normalize();

	Vector j = cross_product(k,i);
	j.normalize();

	transform.set_orientation(Matrix(i,j,k));

	CAMERA->SetWorldRotationPitchRoll(transform.get_yaw()*MUL_RAD_TO_DEG,transform.get_pitch()*MUL_RAD_TO_DEG,transform.get_roll()*MUL_RAD_TO_DEG,0);
	Vector temp = transform.translation;
	CAMERA->SetPosition(&(transform.translation), 1);
}
//--------------------------------------------------------------------------//
//
void MovieCameraManager::init()
{
	spotListStart = NULL;
	spotListEnd = NULL;
	timer = 0;
	lastSpot = NULL;
	bTracking = false;

	COMPTR<IDAConnectionPoint> connection;
	if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Advise(getBase(), &eventHandle);
}
//--------------------------------------------------------------------------//
//
struct _camMang : GlobalComponent
{
	MovieCameraManager * manager;

	virtual void Startup (void)
	{
		CAMERAMANAGER = manager = new DAComponent<MovieCameraManager>;
		AddToGlobalCleanupList(&manager);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		manager->init();
	}
};

static _camMang globalCamMang;


//---------------------------------------------------------------------------
//------------------------End MovieCamera.cpp----------------------------------------
//---------------------------------------------------------------------------


