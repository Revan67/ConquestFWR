//--------------------------------------------------------------------------//
//                                                                          //
//                                Waypoint.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Waypoint.cpp 31    8/18/00 3:55p Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "TObjSelect.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "ObjList.h"
#include "Camera.h"
#include "SuperTrans.h"
#include "UserDefaults.h"
#include <DWaypoint.h>
#include "IMissionActor.h"
#include "Mission.h"
#include "Startup.h"
#include "DrawAgent.h"
#include "TObjMission.h"
#include "TObjRender.h"
#include "DQuickSave.h"

#include <EventSys.h>
#include <Engine.h>
#include <ITextureLibrary.h>
#include <TSmartPointer.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TComponent.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct WAYPOINT_INIT : RenderArch
{
	S32 archIndex;
	IMeshArchetype * meshArch;
	const BT_WAYPOINT * pData;

	WAYPOINT_INIT (S32 _archIndex, const BT_WAYPOINT * _pData)
	{
		meshArch =  NULL;
		archIndex = _archIndex;
		pData = _pData;
	}
};

struct _NO_VTABLE Waypoint : public ObjectRender
										<ObjectSelection
											<ObjectMission
												<ObjectTransform
													<ObjectFrame<IBaseObject,WAYPOINT_SAVELOAD,WAYPOINT_INIT> 
													>
												>
											>
										>,
									ISaveLoad, IQuickSaveLoad
{
	BEGIN_MAP_INBOUND(Waypoint)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	END_MAP()

	// hack stuff
	bool bHackAlwaysVisible;

	Waypoint (void) 
	{
	}

	virtual ~Waypoint (void);	// See ObjList.cpp

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);	// set bVisible if possible for any part of object to appear

	virtual SINGLE TestHighlight (const RECT & rect);

	virtual BOOL32 Update (void);

	virtual void Render (void);

	virtual void View (void);

	virtual void DEBUG_print (void) const;

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

	/* ISaveLoad */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	/* IQuickSaveLoad methods */

	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void);

	/* Waypoint methods */
};

//---------------------------------------------------------------------------
//
Waypoint::~Waypoint (void)
{
}
//---------------------------------------------------------------------------
//
void Waypoint::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	if ((bVisible = (systemID == currentSystem)) != 0)
	{
		bVisible = defaults.bEditorMode || bHackAlwaysVisible;
	}
}
//---------------------------------------------------------------------------
//
SINGLE Waypoint::TestHighlight (const RECT & rect)
{
	bHighlight = 0;
	if (DEFAULTS->GetDefaults()->bEditorMode)
		return ObjectSelection
										<ObjectMission
											<ObjectTransform
												<ObjectFrame<IBaseObject,WAYPOINT_SAVELOAD,WAYPOINT_INIT> 
												>
											>
										>::TestHighlight(rect);
	else
		return 0.0f;
}
//--------------------------------------------------------------------------//
//
void Waypoint::DEBUG_print (void) const
{
#ifndef FINAL_RELEASE
#if (defined(_DEBUG))
	const char * debugName;
	if (bHighlight && (debugName = GetDebugName()) != 0)
	{
		Vector point;
		S32 x, y;

		point.x = 0;
		point.y = box[2]+120.0F;	// maxY + 120
		point.z = 0;

		if (CAMERA->PointToScreen(point, &x, &y, &transform)==IN_PANE)
		{
			DEBUGFONT->StringDraw(0, x+20, y, debugName);
		}
	}
#endif
#endif
}
//---------------------------------------------------------------------------
//
BOOL32 Waypoint::Update (void)
{
	return 1;
}
//---------------------------------------------------------------------------
//
void Waypoint::Render (void)
{
	if (bVisible)
	{
		if (DEFAULTS->GetDefaults()->bEditorMode || bHackAlwaysVisible)
			TreeRender(mc);
	}
}
//---------------------------------------------------------------------------
//
void Waypoint::View (void)
{
	WAYPOINT_VIEW view;
	
	view.mission = this;
	view.position = get_position(instanceIndex);

	if (DEFAULTS->GetUserData("WAYPOINT_VIEW", view.mission->partName, &view, sizeof(view)))
	{
		set_position(instanceIndex, view.position);
	}
}
//---------------------------------------------------------------------------
//
BOOL32 Waypoint::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "WAYPOINT_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	WAYPOINT_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));

	FRAME_save(save);
	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 Waypoint::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "WAYPOINT_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	WAYPOINT_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("WAYPOINT_SAVELOAD", buffer, &load);

	FRAME_load(load);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void Waypoint::ResolveAssociations()
{
}
//--------------------------------------------------------------------------------------
//
void Waypoint::QuickSave (struct IFileSystem * file)
{
	DAFILEDESC fdesc = partName;
	HANDLE hFile;

	file->CreateDirectory("MT_WAYPOINT_QLOAD");
	if (file->SetCurrentDirectory("MT_WAYPOINT_QLOAD") == 0)
		CQERROR0("QuickSave failed on directory 'MT_WAYPOINT_QLOAD'");

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	if ((hFile = file->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR1("QuickSave failed on part '%s'", fdesc.lpFileName);
	}
	else
	{
		MT_WAYPOINT_QLOAD qload;
		DWORD dwWritten;
		
		qload.position.init(GetGridPosition(),systemID);

		file->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		file->CloseHandle(hFile);
	}
}
//--------------------------------------------------------------------------------------
//
void Waypoint::QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize)
{
	MT_WAYPOINT_QLOAD qload;
	MISSION->CorrelateSymbol(szSaveLoadType, buffer, &qload);

	SetPosition(qload.position, qload.position.systemID);
	MGlobals::InitMissionData(this, MGlobals::CreateNewPartID(0));
	partName = szInstanceName;
	SetReady(true);

	OBJLIST->AddPartID(this, dwMissionID);
}
//--------------------------------------------------------------------------------------
//
void Waypoint::QuickResolveAssociations (void)
{
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
static inline struct IBaseObject * createWaypoint (PARCHETYPE pArchetype, S32 archIndex, bool bHackMode)
{
	Waypoint * obj = new ObjectImpl<Waypoint>;
	obj->FRAME_init(WAYPOINT_INIT(archIndex, (const BT_WAYPOINT *)ARCHLIST->GetArchetypeData(pArchetype)));

	obj->pArchetype = pArchetype;
	obj->objClass = OC_WAYPOINT;
	obj->transform.rotate_about_i(90*MUL_DEG_TO_RAD);
	obj->bHackAlwaysVisible = bHackMode;

	return obj;
}
//------------------------------------------------------------------------------------------
//---------------------------Waypoint Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE WaypointFactory : public IObjectFactory, IEventCallback
{
	struct OBJTYPE 
	{
		PARCHETYPE pArchetype;
		S32 archeIndex;
		
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
			archeIndex = -1;
		}

		~OBJTYPE (void)
		{
			if (archeIndex != -1)
				ENGINE->release_archetype(archeIndex);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback
	U32 eventHandle;

	/* hack stuff */
	bool bHackMode;

	/* end hack stuff */

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(WaypointFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	WaypointFactory (void) { }

	~WaypointFactory (void);

	void init (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IObjectFactory methods */

	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* WaypointFactory methods */

	void doHackThing (void);

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
WaypointFactory::~WaypointFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);
}
//--------------------------------------------------------------------------//
//
void WaypointFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);

	if (OBJLIST->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Advise(getBase(), &eventHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE WaypointFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_WAYPOINT)
	{
		BT_WAYPOINT * data = (BT_WAYPOINT *) _data;

		result = new OBJTYPE;
		result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		DAFILEDESC fdesc = data->fileName;
		COMPTR<IFileSystem> objFile;

		if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
			TEXLIB->load_library(objFile, 0);
		else
			goto Error;

		if ((result->archeIndex = ENGINE->create_archetype(fdesc.lpFileName, objFile)) == INVALID_ARCHETYPE_INDEX)
			goto Error;
	}
	goto Done;

Error:
	delete result;
	result = 0;
Done:
	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 WaypointFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * WaypointFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createWaypoint(objtype->pArchetype, objtype->archeIndex, bHackMode);
}
//-------------------------------------------------------------------
//
void WaypointFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//
GENRESULT WaypointFactory::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case WM_COMMAND:
		switch (LOWORD(msg->wParam))
		{
		case IDM_EDITOR_DROP_OBJECT:
			doHackThing();
			break;
		}
		break;
	}

	return GR_OK;
}
//---------------------------------------------------------------------------
//
void WaypointFactory::doHackThing (void)
{
	char buffer[MAX_PATH+4];

	if (DEFAULTS->GetInputFilename(buffer, IDS_OPEN3DB_FILTER))
	{
		PARCHETYPE pArchetype = ARCHLIST->LoadArchetype("MISSION!!WAYPOINT");
		OBJTYPE * hackType = (OBJTYPE *) ARCHLIST->GetArchetypeHandle(pArchetype);
		DAFILEDESC fdesc = buffer;
		COMPTR<IFileSystem> objFile;

		if (DACOM->CreateInstance(&fdesc, objFile) == GR_OK)
		{
			TEXLIB->load_library(objFile, 0);

			ARCHETYPE_INDEX index;
			if ((index = ENGINE->create_archetype(fdesc.lpFileName, objFile)) != INVALID_ARCHETYPE_INDEX)
			{
				hackType->archeIndex = index;
				MISSION_INFO info;

				info.partName[0] = 0;
				EditorStartObjectInsertion(pArchetype, info);
				bHackMode=true;
			}
		}
	}
}

//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _waypoint : GlobalComponent
{
	WaypointFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<WaypointFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _waypoint __ship;

//---------------------------------------------------------------------------
//---------------------------End WayPoint.cpp--------------------------------
//---------------------------------------------------------------------------
