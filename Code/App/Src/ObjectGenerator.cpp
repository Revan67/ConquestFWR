//--------------------------------------------------------------------------//
//                                                                          //
//                         ObjectGenerator.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ObjectGenerator.cpp 29    8/18/00 3:55p Rmarr $
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
#include <DObjectGenerator.h >
#include "IMissionActor.h"
#include "Mission.h"
#include "Startup.h"
#include "DrawAgent.h"
#include "TObjMission.h"
#include "IObjectGenerator.h"
#include "OpAgent.h"
#include "DSpaceship.h"
#include "MPart.h"
#include "IFabricator.h"
#include "IPlanet.h"
#include "DPlatform.h"
#include "CommPacket.h"
#include "MScript.h"
#include "DQuickSave.h"
#include "IBanker.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <ITextureLibrary.h>

#pragma pack(push,1)
struct GenCommand
{
	U8 command:1;
	U32 handle;
};
#pragma pack(pop)

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct OBJECT_GENERATOR_INIT : RenderArch
{
	S32 archIndex;
	IMeshArchetype * meshArch;
	const BT_OBJECT_GENERATOR * pData;

	OBJECT_GENERATOR_INIT (S32 _archIndex, const BT_OBJECT_GENERATOR * _pData)
	{
		meshArch = NULL;
		archIndex = _archIndex;
		pData = _pData;
	}
};

#define GEN_C_GENERATE 0
#define GEN_C_CHANGE_TYPE 1

struct _NO_VTABLE ObjectGenerator : public ObjectRender<ObjectSelection
										<ObjectMission
											<ObjectTransform
												<ObjectFrame<IBaseObject,OBJECT_GENERATOR_SAVELOAD,OBJECT_GENERATOR_INIT> 
													>
												>
											>
										>,
									ISaveLoad, IQuickSaveLoad, IObjectGenerator, BASE_OBJECT_GENERATOR_SAVELOAD
{
	BEGIN_MAP_INBOUND(ObjectGenerator)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IObjectGenerator)
	END_MAP()

	InitNode	initNode;

	ObjectGenerator (void);

	virtual ~ObjectGenerator (void);	// See ObjList.cpp

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

	/* IObjectGenerator */

	virtual void SetGenerationFrequency(SINGLE _mean, SINGLE _minDiference);

	virtual void SetGenerationType(U32 _typeID);

	virtual void EnableGeneration(bool bEnable);

	virtual U32 ForceGeneration();

	/* IMissionActor */

	virtual void OnOperationCreation(U32 agentID, void * buffer, U32 bufferSize);

	/* ISaveLoad */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	/* IQuickSaveLoad methods */

	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void);

	/* ObjectGenerator methods */

	void init (const OBJECT_GENERATOR_INIT & data);

	U32 generateObject();

	void resetGenType();

	void addObject (U32 archID, U32 partID);

	bool isAdmiral (PARCHETYPE pArchetype);

	void editorInitObject (IBaseObject * obj);
};
//---------------------------------------------------------------------------
//
ObjectGenerator::ObjectGenerator() : 
	initNode(this, InitProc(&ObjectGenerator::init))
{
}
//---------------------------------------------------------------------------
//
ObjectGenerator::~ObjectGenerator (void)
{
}
//---------------------------------------------------------------------------
//
void ObjectGenerator::SetGenerationFrequency(SINGLE _mean, SINGLE _minDiference)
{
	mean = _mean;
	minDiff = _minDiference;
	timer = 0;
	nextTime = ((((SINGLE)(rand()))/RAND_MAX)*2 - 1)*minDiff + mean;
}
//---------------------------------------------------------------------------
//
void ObjectGenerator::SetGenerationType(U32 _typeID)
{
	archID = _typeID;
	resetGenType();
}
//---------------------------------------------------------------------------
//
void ObjectGenerator::EnableGeneration(bool bEnable)
{
	if(bEnable && (!bGenEnabled))
	{
		bGenEnabled = true;
		timer = 0;
	}else if ((!bEnable) && bGenEnabled)
	{
		bGenEnabled = false;
	}
}
//---------------------------------------------------------------------------
//
void ObjectGenerator::init (const OBJECT_GENERATOR_INIT & data)
{
	if(data.pData->generateType[0])
		archID = ARCHLIST->GetArchetypeDataID(data.pData->generateType);
	else
		archID = 0;
	mean = data.pData->mean;
	minDiff = data.pData->minDiff;
	bGenEnabled = data.pData->startEnabled;
	nextTime = ((((SINGLE)(rand()))/RAND_MAX)*2 - 1)*minDiff + mean;
}
//---------------------------------------------------------------------------
//
U32 ObjectGenerator::ForceGeneration()
{
	return generateObject();
}
//---------------------------------------------------------------------------
//
U32 ObjectGenerator::generateObject()
{
	if(archID && THEMATRIX->IsMaster())
	{
		U32 partID = MGlobals::CreateNewPartID(playerID);
		
		GenCommand buffer;
		buffer.command = GEN_C_GENERATE;
		buffer.handle = partID;
		U32 workingID = THEMATRIX->CreateOperation(dwMissionID,&(buffer),sizeof(buffer));
		THEMATRIX->OperationCompleted(workingID,dwMissionID);

		addObject(archID,partID);

		return partID;
	}
	return 0;
}
//---------------------------------------------------------------------------
//
void ObjectGenerator::resetGenType()
{
	if(THEMATRIX->IsMaster())
	{
		GenCommand buffer;
		buffer.command = GEN_C_CHANGE_TYPE;
		buffer.handle = archID;
		U32 workingID = THEMATRIX->CreateOperation(dwMissionID,&(buffer),sizeof(buffer));
		THEMATRIX->OperationCompleted(workingID,dwMissionID);
	}

}
//-------------------------------------------------------------------
//
bool ObjectGenerator::isAdmiral (PARCHETYPE pArchetype)
{
	BASE_SPACESHIP_DATA	* data  = (BASE_SPACESHIP_DATA	*) ARCHLIST->GetArchetypeData(pArchetype);

	return (data->objClass == OC_SPACESHIP && data->type == SSC_FLAGSHIP);
}
//-------------------------------------------------------------------
//
void ObjectGenerator::editorInitObject (IBaseObject * obj)
{
	//
	// set initial supplies, hull points
	//
	MPartNC part = obj;

	if (part.isValid())
	{
		if(THEMATRIX->IsMaster())
			BANKER->UseCommandPt(playerID,part.pInit->resourceCost.commandPt);
		part->bUnderCommand = true;
		part->hullPoints = part->hullPointsMax;
		if ((part->mObjClass != M_HARVEST) && (part->mObjClass != M_GALIOT) &&(part->mObjClass != M_SIPHON))
			part->supplies   = part->supplyPointsMax;
		obj->SetReady(true);
	}

	obj->SetVisibleToAllies(MGlobals::GetAllyMask(playerID));
	obj->UpdateVisibilityFlags();
}
//-------------------------------------------------------------------
//
void ObjectGenerator::addObject (U32 archID, U32 partID)
{
	PARCHETYPE objToPlace = ARCHLIST->LoadArchetype(archID);

	if (isAdmiral(objToPlace))
		partID |= ADMIRAL_MASK;
	IBaseObject * rtObject = MGlobals::CreateInstance(objToPlace, partID);

	if (rtObject)
	{
		Vector position;
		VOLPTR(IPhysicalObject) physObj;
	
		position = GetPosition();

		editorInitObject(rtObject);

		OBJLIST->AddObject(rtObject);

		physObj = rtObject;

		VOLPTR(IPlatform) platform;
		
		platform = rtObject;
		if (platform)		// yes! we are a platform
		{
			//
			// find nearest open slot
			//
			IBaseObject * obj = OBJLIST->GetTargetList();     // nav hazard list
			S32 bestSlot=-1;
			SINGLE bestDistance=10e8;
			OBJPTR<IPlanet> planet, bestPlanet;
			U32 dwPlatformID = platform.Ptr()->GetPartID();

			while (obj)
			{
				if (obj->GetSystemID() == systemID && obj->QueryInterface(IPlanetID, planet)!=0)
				{
					//
					// object is a planet, find the closest open slot
					//
					U32 i, maxSlots;

					maxSlots = planet->GetMaxSlots();
					U32 numSlots = ((BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(objToPlace)))->slotsNeeded;
					U32 slotMask = 0x00000001;
					while(!(slotMask & (0x00000001 << (numSlots-1))))
					{
						slotMask = (slotMask << 1) | 0x00000001;
					}
					for (i=0; i < maxSlots; i++)
					{
						U16 slot = planet->AllocateBuildSlot(dwPlatformID, slotMask);
						if (slot!=0)
						{
							SINGLE distance;
							distance = (position-planet->GetSlotTransform(slotMask).translation).magnitude();
							planet->DeallocateBuildSlot(dwPlatformID, slotMask);
							if (distance < bestDistance)
							{
								bestDistance = distance;
								bestSlot = slotMask;
								bestPlanet = planet;
							}
						}
						slotMask = slotMask << 1;
						if(slotMask & (0x00000001 << maxSlots))
						{
							slotMask = (slotMask & (~(0x00000001 << maxSlots))) | 0x00000001; 
						}
					}
				} // end obj->GetSystemID() ...
				
				obj = obj->nextTarget;
			} // end while()

			if (bestPlanet && bestDistance < 10000)
			{
				CQASSERT(bestSlot);
				bestSlot = bestPlanet->AllocateBuildSlot(dwPlatformID, bestSlot);
				TRANSFORM trans = bestPlanet->GetSlotTransform(bestSlot);
				position = trans.translation;

				platform->ParkYourself(trans, bestPlanet.Ptr()->GetPartID(), bestSlot);
			}
			
			if(physObj)
			{
				physObj->SetPosition(position, systemID);
				ENGINE->update_instance(physObj.Ptr()->GetObjectIndex(),0,0);
			}
		}
		else if (physObj)
		{
			physObj->SetPosition(position, systemID);
			ENGINE->update_instance(physObj.Ptr()->GetObjectIndex(),0,0);

			if(THEMATRIX->IsMaster())
			{
				USR_PACKET<USRMOVE> packet;

				packet.objectID[0] = rtObject->GetPartID();
				packet.position.init(GetGridPosition(),systemID);
				packet.init(1);
				NETPACKET->Send(HOSTID,0,&packet);
			}
		}

		MScript::RunProgramsWithEvent(CQPROGFLAG_OBJECTCONSTRUCTED, rtObject->GetPartID());

//			if(rtObject->GetPlayerID())
//				SECTOR->RevealSystem(SECTOR->GetCurrentSystem(), rtObject->GetPlayerID());
	}
}
//---------------------------------------------------------------------------
//
void ObjectGenerator::OnOperationCreation(U32 agentID, void * buffer, U32 bufferSize)
{
	GenCommand * buf = (GenCommand *) buffer;
	if(buf->command == GEN_C_GENERATE)
	{
		addObject(archID,buf->handle);	
	}else if(buf->command == GEN_C_CHANGE_TYPE)
	{
		archID = buf->handle;
	}
	THEMATRIX->OperationCompleted(agentID,dwMissionID);
}
//---------------------------------------------------------------------------
//
void ObjectGenerator::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	if ((bVisible = (systemID == currentSystem)) != 0)
	{
		bVisible = defaults.bEditorMode;
	}
}
//---------------------------------------------------------------------------
//
SINGLE ObjectGenerator::TestHighlight (const RECT & rect)
{
	bHighlight = 0;
	if (DEFAULTS->GetDefaults()->bEditorMode)
		return ObjectSelection
										<ObjectMission
											<ObjectTransform
												<ObjectFrame<IBaseObject,OBJECT_GENERATOR_SAVELOAD,OBJECT_GENERATOR_INIT> 
												>
											>
										>::TestHighlight(rect);
	else
		return 0.0f;
}
//--------------------------------------------------------------------------//
//
void ObjectGenerator::DEBUG_print (void) const
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
BOOL32 ObjectGenerator::Update (void)
{
	if(bGenEnabled && THEMATRIX->IsMaster())
	{
		timer += ELAPSED_TIME;
		while(timer >= nextTime)
		{
			timer -= nextTime;
			nextTime = ((((SINGLE)(rand()))/RAND_MAX)*2 - 1)*minDiff + mean;
			generateObject();
		}
	}else
	{
		timer = 0;
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void ObjectGenerator::Render (void)
{
	if (bVisible)
	{
		if (DEFAULTS->GetDefaults()->bEditorMode)
			TreeRender(mc.mi,mc.numChildren);
	}
}
//---------------------------------------------------------------------------
//
void ObjectGenerator::View (void)
{
	OBJECT_GENERATOR_VIEW view;
	
	view.mission = this;
	view.position = get_position(instanceIndex);
	const char * namePtr = ARCHLIST->GetArchName(archID);
	if(namePtr)
		strcpy(view.generatorType,namePtr);
	else
		view.generatorType[0] = 0;
	view.mean = mean;
	view.minDiff = minDiff;
	view.timer = timer;
	view.nextTime = nextTime;
	view.bGenEnabled = bGenEnabled;
	strcpy(view.partName,partName);

	if (DEFAULTS->GetUserData("OBJECT_GENERATOR_VIEW", view.mission->partName, &view, sizeof(view)))
	{
		set_position(instanceIndex, view.position);

		strcpy(partName,view.partName);
		mean = view.mean;
		minDiff = view.minDiff;
		timer = view.timer;
		nextTime = view.nextTime;
		bGenEnabled = view.bGenEnabled;

		if(view.generatorType[0])
		{
			archID = ARCHLIST->GetArchetypeDataID(view.generatorType);
		}
		else
		{
			archID = 0;
		}
	}
}
//---------------------------------------------------------------------------
//
BOOL32 ObjectGenerator::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "OBJECT_GENERATOR_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	OBJECT_GENERATOR_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));

	save.baseSave = *static_cast<BASE_OBJECT_GENERATOR_SAVELOAD*>(this);

	FRAME_save(save);
	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 ObjectGenerator::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "OBJECT_GENERATOR_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	OBJECT_GENERATOR_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("OBJECT_GENERATOR_SAVELOAD", buffer, &load);

	*static_cast<BASE_OBJECT_GENERATOR_SAVELOAD*>(this)= load.baseSave;

	FRAME_load(load);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void ObjectGenerator::ResolveAssociations()
{
}
//--------------------------------------------------------------------------------------
//
void ObjectGenerator::QuickSave (struct IFileSystem * file)
{
	DAFILEDESC fdesc = partName;
	HANDLE hFile;

	file->CreateDirectory("MT_QOBJGENERATOR_LOAD");
	if (file->SetCurrentDirectory("MT_QOBJGENERATOR_LOAD") == 0)
		CQERROR0("QuickSave failed on directory 'MT_QOBJGENERATOR_LOAD'");

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	if ((hFile = file->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR1("QuickSave failed on part '%s'", fdesc.lpFileName);
	}
	else
	{
		MT_QOBJGENERATOR_LOAD qload;
		DWORD dwWritten;
		
		if(archID)
			strcpy(qload.archTypeName, ARCHLIST->GetArchName(archID));
		else
			qload.archTypeName[0] = 0;

		qload.bGenEnabled = bGenEnabled;
		qload.dwMissionID = dwMissionID;
		qload.mean = mean;
		qload.minDiff = minDiff;
		qload.nextTime = nextTime;
		qload.position.init(GetGridPosition(),systemID);
		qload.timer = timer;

		file->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		file->CloseHandle(hFile);
	}
}
//--------------------------------------------------------------------------------------
//
void ObjectGenerator::QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize)
{
	MT_QOBJGENERATOR_LOAD qload;
	MISSION->CorrelateSymbol(szSaveLoadType, buffer, &qload);

	bGenEnabled = qload.bGenEnabled;
	mean = qload.mean;
	minDiff = qload.minDiff;
	nextTime = qload.nextTime;
	timer = qload.timer;

	if(qload.archTypeName[0])
		archID = ARCHLIST->GetArchetypeDataID(qload.archTypeName);
	else
		archID = 0;

	SetPosition(qload.position, qload.position.systemID);
	MGlobals::InitMissionData(this, MGlobals::CreateNewPartID(MGlobals::GetPlayerFromPartID(qload.dwMissionID)));
	partName = szInstanceName;
	SetReady(true);

	OBJLIST->AddPartID(this, dwMissionID);
}
//--------------------------------------------------------------------------------------
//
void ObjectGenerator::QuickResolveAssociations (void)
{
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createObjectGenerator (PARCHETYPE pArchetype, S32 archIndex)
{
	ObjectGenerator * obj = new ObjectImpl<ObjectGenerator>;
	obj->FRAME_init(OBJECT_GENERATOR_INIT(archIndex, (const BT_OBJECT_GENERATOR *)ARCHLIST->GetArchetypeData(pArchetype)));

	obj->pArchetype = pArchetype;
	obj->objClass = OC_OBJECT_GENERATOR;
	obj->transform.rotate_about_i(90*MUL_DEG_TO_RAD);

	return obj;
}
//------------------------------------------------------------------------------------------
//---------------------------ObjectGenerator Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE ObjectGeneratorFactory : public IObjectFactory
{
	struct OBJTYPE 
	{
		PARCHETYPE pArchetype;
		S32 archeIndex;
		
		void * operator new (size_t size)
		{
			return calloc(size,1);
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

	BEGIN_DACOM_MAP_INBOUND(ObjectGeneratorFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	ObjectGeneratorFactory (void) { }

	~ObjectGeneratorFactory (void);

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

	/* ObjectGeneratorFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
ObjectGeneratorFactory::~ObjectGeneratorFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void ObjectGeneratorFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE ObjectGeneratorFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_OBJECT_GENERATOR)
	{
		BT_OBJECT_GENERATOR * data = (BT_OBJECT_GENERATOR *) _data;

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
BOOL32 ObjectGeneratorFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * ObjectGeneratorFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createObjectGenerator(objtype->pArchetype, objtype->archeIndex);
}
//-------------------------------------------------------------------
//
void ObjectGeneratorFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _objectGenerator : GlobalComponent
{
	ObjectGeneratorFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<ObjectGeneratorFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _objectGenerator __objGener;

//---------------------------------------------------------------------------
//---------------------------End WayPoint.cpp--------------------------------
//---------------------------------------------------------------------------
