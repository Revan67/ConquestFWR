//--------------------------------------------------------------------------//
//                                                                          //
//                                Trigger.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Trigger.cpp 21    8/18/00 3:55p Rmarr $
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
#include <DTrigger.h>
#include "IMissionActor.h"
#include "Mission.h"
#include "Startup.h"
#include "DrawAgent.h"
#include "TObjMission.h"
#include "ITrigger.h"
#include "MPart.h"
#include "Sector.h"
#include "MScript.h"
#include "GridVector.h"
#include "DQuickSave.h"
#include <TriggerFlags.h>
#include "TObjRender.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <ITextureLibrary.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#define NUM_LINE_SEGS 40

Vector triggerRangeLines[NUM_LINE_SEGS];
SINGLE lastRangeHighlight = 0.0;

struct TRIGGER_INIT : RenderArch
{
	S32 archIndex;
	IMeshArchetype * meshArch;
	const BT_TRIGGER * pData;

	TRIGGER_INIT (S32 _archIndex, const BT_TRIGGER * _pData)
	{
		meshArch = NULL;
		archIndex = _archIndex;
		pData = _pData;
	}
};

struct _NO_VTABLE Trigger : public ObjectRender
										<ObjectSelection
											<ObjectMission
												<ObjectTransform
													<ObjectFrame<IBaseObject,TRIGGER_SAVELOAD,TRIGGER_INIT>
													>
												>
											>
										>,
									ISaveLoad, IQuickSaveLoad, ITrigger, BASE_TRIGGER_SAVELOAD
{
	BEGIN_MAP_INBOUND(Trigger)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(ITrigger)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	END_MAP()

	Trigger (void) 
	{
	}

	virtual ~Trigger (void);	// See ObjList.cpp

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

	/* ITrigger methods */

	virtual void EnableTrigger(bool bEnable);

	virtual void SetFilter(U32 number, U32 flags,bool bAddTo = false);

	virtual void SetTriggerRange(SINGLE range);

	virtual void SetTriggerProgram (const char * progName);

	virtual IBaseObject * GetLastTriggerObject();

	/* ISaveLoad */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	/* IQuickSaveLoad methods */

	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void);

	/* Trigger methods */

	void activateTrigger();
};

//---------------------------------------------------------------------------
//
Trigger::~Trigger (void)
{
}
//---------------------------------------------------------------------------
//
void Trigger::activateTrigger()
{
	if(progName)
	{
		MScript::RunProgramByName(progName, dwMissionID);
	}
}
//---------------------------------------------------------------------------
//
void Trigger::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	if ((bVisible = (systemID == currentSystem)) != 0)
	{
		bVisible = defaults.bEditorMode;
	}
}
//---------------------------------------------------------------------------
//
void Trigger::EnableTrigger(bool bEnable)
{
	bEnabled = bEnable;
}
//---------------------------------------------------------------------------
//
void Trigger::SetFilter(U32 number, U32 flags,bool bAddTo)
{
	if(bAddTo)
		triggerFlags |= flags;
	else
		triggerFlags = flags;
	if(flags&TRIGGER_SHIP_ID)
		triggerShipID = number;
	else if(flags&TRIGGER_OBJCLASS)
		triggerObjClassID = number;
	else if(flags&TRIGGER_MOBJCLASS)
		triggerMObjClassID = number;
	else if(flags&TRIGGER_PLAYER)
		triggerPlayerID = number;
	else if(flags&TRIGGER_FORCEREADY)
		bDetectOnlyReady = true;
	else if(flags&TRIGGER_NOFORCEREADY)
		bDetectOnlyReady = false;
}
//---------------------------------------------------------------------------
//
void Trigger::SetTriggerRange(SINGLE range)
{
	triggerRange = range;
}
//---------------------------------------------------------------------------
//
void Trigger::SetTriggerProgram (const char * _progName)
{
	strcpy(progName,_progName);
}
//---------------------------------------------------------------------------
//
IBaseObject * Trigger::GetLastTriggerObject()
{
	return OBJLIST->FindObject(lastTriggerID);
}
//---------------------------------------------------------------------------
//
SINGLE Trigger::TestHighlight (const RECT & rect)
{
	bHighlight = 0;
	if (DEFAULTS->GetDefaults()->bEditorMode)
		return ObjectSelection
										<ObjectMission
											<ObjectTransform
												<ObjectFrame<IBaseObject,TRIGGER_SAVELOAD,TRIGGER_INIT> 
												>
											>
										>::TestHighlight(rect);
	else
		return 0.0f;
}
//--------------------------------------------------------------------------//
//
void Trigger::DEBUG_print (void) const
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
BOOL32 Trigger::Update (void)
{
	if(bEnabled && MGlobals::IsUpdateFrame(dwMissionID))
	{
		IBaseObject * obj = OBJLIST->GetTargetList();
		while(obj)
		{
			if(obj->GetSystemID() == systemID)
			{
				MPart part(obj);
				if(part.isValid() && (part->bReady || (!bDetectOnlyReady)))
				{
					if(obj->GetGridPosition()-GetGridPosition() <= triggerRange)
					{
						bool bTrigger = true;
						if(triggerFlags & TRIGGER_PLAYER)
							bTrigger &= (obj->GetPlayerID() == triggerPlayerID);
						if(triggerFlags & TRIGGER_SHIP_ID)
							bTrigger &= (obj->GetPartID() == triggerShipID);
						if(triggerFlags & TRIGGER_OBJCLASS)
							bTrigger &= (obj->objClass == (S32)triggerObjClassID);
						if(triggerFlags & TRIGGER_MOBJCLASS)
							bTrigger &= (part->mObjClass == (S32)triggerMObjClassID);
						
						if(bTrigger)
						{
							lastTriggerID = obj->GetPartID();
							activateTrigger();
							break;
						}
					}
				}
			}
			obj=obj->nextTarget;
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void Trigger::Render (void)
{
	if (bVisible)
	{
		if (DEFAULTS->GetDefaults()->bEditorMode)
			TreeRender(mc);
	}
	if((systemID == SECTOR->GetCurrentSystem()) && bHighlight && triggerRange != 0.0)
	{
		if(lastRangeHighlight != triggerRange)
		{
			lastRangeHighlight = triggerRange;
			for(int i = 0; i < NUM_LINE_SEGS; ++i)
			{
				triggerRangeLines[i] = Vector(cos((2*PI*i)/NUM_LINE_SEGS)*triggerRange*GRIDSIZE,
					sin((2*PI*i)/NUM_LINE_SEGS)*triggerRange*GRIDSIZE,0);
			}
		}
		BATCH->set_state(RPR_BATCH,true);
		DisableTextures();
		CAMERA->SetModelView();
		BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		PB.Begin(PB_LINES);
		PB.Color3ub(64,128,64);
		Vector oldVect = triggerRangeLines[0]+transform.translation;
		for(U32 i = 1 ; i <= NUM_LINE_SEGS; ++i)
		{
			PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
			oldVect = triggerRangeLines[i%NUM_LINE_SEGS]+transform.translation;
			PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		}
		PB.End();
	}
}
//---------------------------------------------------------------------------
//
void Trigger::View (void)
{
	TRIGGER_VIEW view;
	
	view.mission = this;
	view.position = get_position(instanceIndex);

	view.bEnabled = bEnabled;
	view.triggerMObjClassID = (M_OBJCLASS)triggerMObjClassID;
	view.triggerObjClassID = (OBJCLASS)triggerObjClassID;
	view.triggerPlayerID = triggerPlayerID;
	view.triggerRange = triggerRange;
	view.triggerShipID = triggerShipID;
	view.bDetectOnlyReady = bDetectOnlyReady;
	strcpy(view.progName,progName);

	if (DEFAULTS->GetUserData("TRIGGER_VIEW", view.mission->partName, &view, sizeof(view)))
	{
		set_position(instanceIndex, view.position);
		
		strcpy(progName,view.progName);
		bEnabled = view.bEnabled;
		triggerMObjClassID = view.triggerMObjClassID;
		triggerObjClassID = view.triggerObjClassID;
		triggerPlayerID = view.triggerPlayerID;
		triggerRange = view.triggerRange;
		triggerShipID = view.triggerShipID;
		bDetectOnlyReady = view.bDetectOnlyReady;

		triggerFlags = 0;
		if(triggerMObjClassID)
			triggerFlags |= TRIGGER_MOBJCLASS;
		if(triggerObjClassID)
			triggerFlags |= TRIGGER_OBJCLASS;
		if(triggerPlayerID)
			triggerFlags |= TRIGGER_PLAYER;
		if(triggerShipID)
			triggerFlags |= TRIGGER_SHIP_ID;
	}
}
//---------------------------------------------------------------------------
//
BOOL32 Trigger::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "TRIGGER_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	TRIGGER_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));

	save.baseSave = *static_cast<BASE_TRIGGER_SAVELOAD*>(this);

	FRAME_save(save);
	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 Trigger::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "TRIGGER_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	TRIGGER_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("TRIGGER_SAVELOAD", buffer, &load);

	 *static_cast<BASE_TRIGGER_SAVELOAD*>(this) = load.baseSave;

	FRAME_load(load);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void Trigger::ResolveAssociations()
{
}
//--------------------------------------------------------------------------------------
//
void Trigger::QuickSave (struct IFileSystem * file)
{
	DAFILEDESC fdesc = partName;
	HANDLE hFile;

	file->CreateDirectory("MT_TRIGGER_QLOAD");
	if (file->SetCurrentDirectory("MT_TRIGGER_QLOAD") == 0)
		CQERROR0("QuickSave failed on 'MT_TRIGGER_QLOAD'");

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	if ((hFile = file->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR1("QuickSave failed on part '%s'", fdesc.lpFileName);
	}
	else
	{
		MT_TRIGGER_QLOAD qload;
		DWORD dwWritten;
		
		qload.position.init(GetGridPosition(),systemID);

		qload.bDetectOnlyReady = bDetectOnlyReady;
		qload.bEnabled = bEnabled;
		strcpy(qload.progName,progName);
		qload.triggerFlags = triggerFlags;
		qload.triggerMObjClassID = triggerMObjClassID;
		qload.triggerObjClassID = triggerObjClassID;
		qload.triggerPlayerID = triggerPlayerID;
		qload.triggerRange = triggerRange;
		qload.triggerShipID = triggerShipID;

		file->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		file->CloseHandle(hFile);
	}
}
//--------------------------------------------------------------------------------------
//
void Trigger::QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize)
{
	MT_TRIGGER_QLOAD qload;
	MISSION->CorrelateSymbol(szSaveLoadType, buffer, &qload);

	bDetectOnlyReady = qload.bDetectOnlyReady;
	bEnabled = qload.bEnabled;
	strcpy(progName,qload.progName);
	triggerFlags = qload.triggerFlags;
	triggerMObjClassID = qload.triggerMObjClassID;
	triggerObjClassID = qload.triggerObjClassID;
	triggerPlayerID = qload.triggerPlayerID;
	triggerRange = qload.triggerRange;
	triggerShipID = qload.triggerShipID;

	SetPosition(qload.position, qload.position.systemID);
	MGlobals::InitMissionData(this, MGlobals::CreateNewPartID(0));
	partName = szInstanceName;
	SetReady(true);

	OBJLIST->AddPartID(this, dwMissionID);
}
//--------------------------------------------------------------------------------------
//
void Trigger::QuickResolveAssociations (void)
{
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createTrigger (PARCHETYPE pArchetype, S32 archIndex)
{
	Trigger * obj = new ObjectImpl<Trigger>;
	TRIGGER_INIT initStruct(archIndex, (const BT_TRIGGER *)ARCHLIST->GetArchetypeData(pArchetype));
	obj->FRAME_init(initStruct);

	obj->pArchetype = pArchetype;
	obj->objClass = OC_TRIGGER;
	obj->transform.rotate_about_i(90*MUL_DEG_TO_RAD);

	obj->bEnabled = false;
	obj->triggerFlags = 0;
	obj->triggerRange = 0;
	obj->triggerShipID = 0;
	obj->triggerObjClassID = 0;
	obj->triggerMObjClassID = 0;
	obj->triggerPlayerID = 0;
	obj->progName[0] = 0;
	obj->lastTriggerID = 0;
	return obj;
}
//------------------------------------------------------------------------------------------
//---------------------------Trigger Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE TriggerFactory : public IObjectFactory
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

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(TriggerFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	TriggerFactory (void) { }

	~TriggerFactory (void);

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

	/* WaypointFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
TriggerFactory::~TriggerFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void TriggerFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE TriggerFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_TRIGGER)
	{
		BT_TRIGGER * data = (BT_TRIGGER *) _data;

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
BOOL32 TriggerFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * TriggerFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createTrigger(objtype->pArchetype, objtype->archeIndex);
}
//-------------------------------------------------------------------
//
void TriggerFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _trigger : GlobalComponent
{
	TriggerFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<TriggerFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _trigger __ship;

//---------------------------------------------------------------------------
//---------------------------End Trigger.cpp--------------------------------
//---------------------------------------------------------------------------
