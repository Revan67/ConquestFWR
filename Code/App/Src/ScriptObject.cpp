//--------------------------------------------------------------------------//
//                                                                          //
//                                ScriptObject.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ScriptObject.cpp 31    9/07/00 6:35p Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "TObjSelect.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "TObjRender.h"
#include "ObjList.h"
#include "Camera.h"
#include "SuperTrans.h"
#include "UserDefaults.h"
#include <DScriptObject.h>
#include "IMissionActor.h"
#include "Mission.h"
#include "Startup.h"
#include "DrawAgent.h"
#include "TObjMission.h"
#include "DQuickSave.h"
#include "ObjMap.h"
#include "IScriptObject.h"
#include "TManager.h"
#include "SFX.h"
#include "IBlinkers.h"
#include "IAnim.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <ITextureLibrary.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct SCRIPTOBJECT_INIT : RenderArch
{
	S32 archIndex;
	IMeshArchetype * meshArch;
	const BT_SCRIPTOBJECT * pData;

	SCRIPTOBJECT_INIT (S32 _archIndex, const BT_SCRIPTOBJECT * _pData)
	{
		meshArch = NULL;
		archIndex = _archIndex;
		pData = _pData;
	}
};

struct _NO_VTABLE ScriptObject : public ObjectRender
											<ObjectSelection
												<ObjectMission
													<ObjectTransform
														<ObjectFrame<IBaseObject,SCRIPTOBJECT_SAVELOAD,SCRIPTOBJECT_INIT> 
													>
												>
											>
										>,
									ISaveLoad, IQuickSaveLoad, IScriptObject,BASE_SCRIPTOBJECT_SAVELOAD
{
	BEGIN_MAP_INBOUND(ScriptObject)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IScriptObject)
	END_MAP()

	int map_square;
	unsigned int map_sys;

	U32 mapTex;
	HSOUND hAmbientSound;

	//BLINKER STUFF
	COMPTR<IBlinkers> blinkers;
	U32 blinkerTexID;

	S32 ambientAnimIndex;

	ScriptObject (void);

	virtual ~ScriptObject (void);	// See ObjList.cpp

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate(SINGLE dt);

	virtual void Render (void);

	virtual void MapRender (bool bPing);

	virtual void View (void);

	virtual void DEBUG_print (void) const;

	virtual void CastVisibleArea (void)
	{
		// propogate visibility
		SetVisibleToAllies(GetVisibilityFlags());
	}

	virtual void SetPosition (const Vector &position, U32 newSystemID)
	{
		systemID = newSystemID;
		CQASSERT(systemID);
		ObjectTransform<ObjectFrame<IBaseObject,SCRIPTOBJECT_SAVELOAD,SCRIPTOBJECT_INIT> >::SetPosition(position, systemID);

		if (systemID && systemID <= MAX_SYSTEMS)
		{
			int new_map_square = OBJMAP->GetMapSquare(systemID,transform.translation);
			if (new_map_square != map_square || map_sys != systemID)
			{
				OBJMAP->RemoveObjectFromMap(this,map_sys,map_square);
				map_square = new_map_square;
				map_sys = systemID;
				OBJMAP->AddObjectToMap(this,map_sys,map_square);
			}
		}
		if(hAmbientSound)
		{
			SFXMANAGER->Play(hAmbientSound,GetSystemID(),&position,SFXPLAYF_LOOPING|SFXPLAYF_NORESTART);
		}
	}

	virtual void SetTransform (const TRANSFORM & transform, U32 newSystemID)
	{
		systemID = newSystemID;
		CQASSERT(systemID);
		ObjectTransform<ObjectFrame<IBaseObject,SCRIPTOBJECT_SAVELOAD,SCRIPTOBJECT_INIT> >::SetTransform(transform, newSystemID);

		if (systemID && systemID <= MAX_SYSTEMS)
		{
			int new_map_square = OBJMAP->GetMapSquare(systemID,transform.translation);
			if (new_map_square != map_square || map_sys != systemID)
			{
				OBJMAP->RemoveObjectFromMap(this,map_sys,map_square);
				map_square = new_map_square;
				map_sys = systemID;
				OBJMAP->AddObjectToMap(this,map_sys,map_square);
			}
		}
		if(hAmbientSound)
		{
			Vector position = GetPosition();
			SFXMANAGER->Play(hAmbientSound,GetSystemID(),&position,SFXPLAYF_LOOPING|SFXPLAYF_NORESTART);
		}
	}

	/* IScriptObject */

	virtual void SetTowing(bool setting, U32 _towerID );

    virtual U32 GetTowerID() { return towerID; };

	virtual bool IsBeingTowed();

	/* ISaveLoad */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	/* IQuickSaveLoad methods */

	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void);

	/* ScriptObject methods */

	void renderBlinkers();

};
//---------------------------------------------------------------------------
//
ScriptObject::ScriptObject (void)
{
	ambientAnimIndex = -1;
}

//---------------------------------------------------------------------------
//
ScriptObject::~ScriptObject (void)
{
	OBJMAP->RemoveObjectFromMap(this,map_sys,map_square);
	SFXMANAGER->CloseHandle(hAmbientSound);

	ANIM->release_script_inst(ambientAnimIndex);
	ambientAnimIndex = -1;
}
//--------------------------------------------------------------------------//
//
void ScriptObject::DEBUG_print (void) const
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
void ScriptObject::SetTowing(bool setting, U32 _towerID )
{
    towerID = _towerID;

	bTowed = setting;

	if(bTowed)
	{
		ANIM->script_stop(ambientAnimIndex);
	}
	else
	{
		ANIM->script_start(ambientAnimIndex, Animation::LOOP, Animation::BEGIN);
	}
}
//---------------------------------------------------------------------------
//
bool ScriptObject::IsBeingTowed()
{
	return bTowed;
}
//---------------------------------------------------------------------------
//
void ScriptObject::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)	
{
	bVisible = (GetSystemID() == currentSystem &&
			   (IsVisibleToPlayer(currentPlayer) ||
			     defaults.bVisibilityRulesOff ||
			     defaults.bEditorMode) );

}
//---------------------------------------------------------------------------
//
BOOL32 ScriptObject::Update (void)
{
	FRAME_update();
	return 1;
}
//---------------------------------------------------------------------------
//
void ScriptObject::PhysicalUpdate(SINGLE dt)
{
	ENGINE->update_instance(instanceIndex,0,dt);
	ANIM->update_instance(instanceIndex,dt);
	FRAME_physicalUpdate(dt);
	if (blinkers)
		blinkers->Update(dt);
}
//---------------------------------------------------------------------------
//
void ScriptObject::Render (void)
{
	if (bVisible)
	{
		TreeRender(mc);
		renderBlinkers();
	}
}
//---------------------------------------------------------------------------
//
void ScriptObject::renderBlinkers()
{
	if(blinkers)
	{
		Vector points[32];
		SINGLE intensity[32];

		S32 numBlinkers = blinkers->GetBlinkers(points,intensity,32);

		U8 i;
		
	//	BATCH->set_texture_stage_texture( 0,blinkerTexID);
		SetupDiffuseBlend(blinkerTexID,TRUE);
		BATCH->set_state(RPR_STATE_ID,blinkerTexID);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_DITHERENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		
		//render blinkers
		CAMERA->SetModelView();

		PB.Begin(PB_QUADS);
		
		Vector cpos (CAMERA->GetPosition());
		for (i=0;i<numBlinkers;i++)
		{
				PB.Color4ub(255,255,255,intensity[i]*255);
				//set dynamic lights
		/*		blinkLight[i].color.r = cosB*255;
				blinkLight[i].setColor(cosB*255,0,0);
				blinkLight[i].set_On(TRUE);*/
					

				
				Vector t = points[i];//transform.rotate_translate(points[i]);
				//TEMPORARY
		//		blinkLight[i].set_position(Vector(t.x,t.y,t.z+400));
		//		blinkLight[i].setSystem(systemID);

				Vector look (t - cpos);
				
				Vector i (look.y, -look.x, 0);
				
				if (fabs (i.x) < 1e-4 && fabs (i.y) < 1e-4)
				{
					i.x = 1.0f;
				}
				
				i.normalize ();
				
				Vector k (-look);
				k.normalize ();
				Vector j (cross_product (k, i));
				
	#define BLINKER_SIZE 60

				i *= BLINKER_SIZE;
				j *= BLINKER_SIZE;
				Vector v0,v1,v2,v3;
				v0 = t-i-j;
				v1 = t+i-j;
				v2 = t+i+j;
				v3 = t-i+j;
				
				PB.TexCoord2f(0,0);  PB.Vertex3f(v0.x,v0.y,v0.z+30);
				PB.TexCoord2f(1,0);  PB.Vertex3f(v1.x,v1.y,v1.z+30);
				PB.TexCoord2f(1,1);  PB.Vertex3f(v2.x,v2.y,v2.z+30);
				PB.TexCoord2f(0,1);  PB.Vertex3f(v3.x,v3.y,v3.z+30);
		}

		PB.End();
		BATCH->set_state(RPR_STATE_ID,0);
	}
}
//---------------------------------------------------------------------------
//
void ScriptObject::MapRender (bool bPing)
{
}
//---------------------------------------------------------------------------
//
void ScriptObject::View (void)
{
	SCRIPTOBJECT_VIEW view;
	
	view.mission = this;
	view.position = get_position(instanceIndex);

	if (DEFAULTS->GetUserData("SCRIPTOBJECT_VIEW", view.mission->partName, &view, sizeof(view)))
	{
		set_position(instanceIndex, view.position);
	}
}
//---------------------------------------------------------------------------
//
BOOL32 ScriptObject::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "SCRIPTOBJECT_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	SCRIPTOBJECT_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));

	FRAME_save(save);
	memcpy(&save, static_cast<BASE_SCRIPTOBJECT_SAVELOAD *>(this), sizeof(BASE_SCRIPTOBJECT_SAVELOAD));
	save.exploredFlags = GetVisibilityFlags();
	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 ScriptObject::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "SCRIPTOBJECT_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	SCRIPTOBJECT_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("SCRIPTOBJECT_SAVELOAD", buffer, &load);

	FRAME_load(load);
	
	*static_cast<BASE_SCRIPTOBJECT_SAVELOAD *>(this) = load;

	SetVisibleToAllies(load.exploredFlags);
	UpdateVisibilityFlags();

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void ScriptObject::ResolveAssociations()
{
	if (systemID && systemID <= MAX_SYSTEMS)
	{
		int new_map_square = OBJMAP->GetMapSquare(systemID,transform.translation);
		if (new_map_square != map_square || map_sys != systemID)
		{
			OBJMAP->RemoveObjectFromMap(this,map_sys,map_square);
			map_square = new_map_square;
			map_sys = systemID;
			OBJMAP->AddObjectToMap(this,map_sys,map_square);
		}
	}
}
//--------------------------------------------------------------------------------------
//
void ScriptObject::QuickSave (struct IFileSystem * file)
{
	DAFILEDESC fdesc = partName;
	HANDLE hFile;

	file->CreateDirectory("MT_SCRIPTOBJECT_QLOAD");
	if (file->SetCurrentDirectory("MT_SCRIPTOBJECT_QLOAD") == 0)
		CQERROR0("QuickSave failed on directory 'MT_SCRIPTOBJECT_QLOAD'");

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	if ((hFile = file->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR1("QuickSave failed on part '%s'", fdesc.lpFileName);
	}
	else
	{
		MT_SCRIPTOBJECT_QLOAD qload;
		DWORD dwWritten;
		
		qload.dwMissionID = dwMissionID;
		qload.position.init(GetGridPosition(),systemID);

		file->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		file->CloseHandle(hFile);
	}
}
//--------------------------------------------------------------------------------------
//
void ScriptObject::QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize)
{
	MT_SCRIPTOBJECT_QLOAD qload;
	MISSION->CorrelateSymbol(szSaveLoadType, buffer, &qload);

	MGlobals::InitMissionData(this, MGlobals::CreateNewPartID(MGlobals::GetPlayerFromPartID(qload.dwMissionID)));
	partName = szInstanceName;
	SetReady(true);

	OBJLIST->AddPartID(this, dwMissionID);

	SetPosition(qload.position, qload.position.systemID);
}
//--------------------------------------------------------------------------------------
//
void ScriptObject::QuickResolveAssociations (void)
{
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createScriptObject (PARCHETYPE pArchetype, S32 archIndex, U32 mapTex, BlinkersArchetype * blink_arch,U32 blinkTex, S32 animArchetype)
{
	ScriptObject * obj = new ObjectImpl<ScriptObject>;
	const BT_SCRIPTOBJECT * data = (const BT_SCRIPTOBJECT *)ARCHLIST->GetArchetypeData(pArchetype);
	obj->FRAME_init(SCRIPTOBJECT_INIT(archIndex, data));

	obj->pArchetype = pArchetype;
	obj->objClass = OC_SCRIPTOBJECT;
	obj->transform.rotate_about_i(90*MUL_DEG_TO_RAD);

	obj->bTowed = false;
	obj->bSysMapActive = data->bSysMapActive;
	obj->mapTex = mapTex;
	obj->hAmbientSound  = SFXMANAGER->Open(data->ambientSound);

	//blinkers
	if (blink_arch)
		CreateBlinkers(obj->blinkers,blink_arch,obj->instanceIndex);

	obj->blinkerTexID = blinkTex;

	if (data->ambient_animation[0])
	{
		obj->ambientAnimIndex = ANIM->create_script_inst(animArchetype, obj->instanceIndex, data->ambient_animation);
		ANIM->script_start(obj->ambientAnimIndex, Animation::LOOP, Animation::BEGIN);
	}

	return obj;
}
//------------------------------------------------------------------------------------------
//---------------------------ScriptObject Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE ScriptObjectFactory : public IObjectFactory
{
	struct OBJTYPE 
	{
		PARCHETYPE pArchetype;
		S32 archeIndex;
		U32 mapTex;
		BlinkersArchetype * blink_arch;
		U32 blinkTex;

		S32 animArchetype;
		
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
			TMANAGER->ReleaseTextureRef(mapTex);

			if (archeIndex != -1)
				ENGINE->release_archetype(archeIndex);

			if (blink_arch)
				DestroyBlinkersArchetype(blink_arch);
			TMANAGER->ReleaseTextureRef(blinkTex);

			if (animArchetype != -1)
				ANIM->release_script_set_arch(animArchetype);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ScriptObjectFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	ScriptObjectFactory (void) { }

	~ScriptObjectFactory (void);

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

	/* ScriptObjectFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
ScriptObjectFactory::~ScriptObjectFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void ScriptObjectFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE ScriptObjectFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_SCRIPTOBJECT)
	{
		BT_SCRIPTOBJECT * data = (BT_SCRIPTOBJECT *) _data;

		SFXMANAGER->Preload(data->ambientSound);

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

		result->animArchetype = ANIM->create_script_set_arch(objFile);

		if (data->blinkers.light_script[0])
		{
			result->blink_arch = CreateBlinkersArchetype(data->blinkers.light_script,result->archeIndex);
			result->blinkTex = TMANAGER->CreateTextureFromFile(data->blinkers.textureName, TEXTURESDIR, DA::TGA, PF_4CC_DAA4);
		}

		result->mapTex = TMANAGER->CreateTextureFromFile("mapship.bmp",TEXTURESDIR, DA::BMP,PF_4CC_DAOT);
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
BOOL32 ScriptObjectFactory::DestroyArchetype (HANDLE hArchetype)
{
	
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * ScriptObjectFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createScriptObject(objtype->pArchetype, objtype->archeIndex, objtype->mapTex,objtype->blink_arch,objtype->blinkTex,objtype->animArchetype);
}
//-------------------------------------------------------------------
//
void ScriptObjectFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _scriptobject : GlobalComponent
{
	ScriptObjectFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<ScriptObjectFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _scriptobject __scobj;

//---------------------------------------------------------------------------
//---------------------------End ScriptObject.cpp--------------------------------
//---------------------------------------------------------------------------
