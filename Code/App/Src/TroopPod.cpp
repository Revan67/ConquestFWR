//--------------------------------------------------------------------------//
//                                                                          //
//                               TroopPod.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TroopPod.cpp 9     10/08/00 9:45p Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "Startup.h"
#include "ObjSet.h"
#include "MeshExplode.h"
#include "TObject.h"
#include "IBuildShip.h"
#include "MPart.h"
#include "Mission.h"
#include "Objlist.h"
#include "FogOfWar.h"
#include "ObjMap.h"

#include <DTroopship.h>
#include <DSpaceship.h>

#include <IConnection.h>
#include <FileSys.h>
#include <TSmartPointer.h>
#include <IHardpoint.h>

#define OPPRINT0(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp)
#define OPPRINT1(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1)
#define OPPRINT2(exp,p1,p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2)

#define POD_LIFETIME 4.0
//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE Trooppod : public IBaseObject, TROOPPOD_SAVELOAD, ISaveLoad
{
	BEGIN_MAP_INBOUND(Trooppod)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	END_MAP()
	//----------------------------------

	OBJPTR<IBaseObject> target;					// can now be enemy or friend

	SINGLE podLifetime;				// when this reaches zero, kill self

	bool   bFadingPods;

	OBJPTR<IShuttle> podShip[MAX_TROOPSHIP_PODS];

	U32 systemID;
	U32 dwMissionID;
	U32 objMapSquare;
	U32 objMapSystemID;

	TRANSFORM transform;
	//----------------------------------
	//----------------------------------
	
	Trooppod (void);

	virtual ~Trooppod (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const
	{
		return transform;
	}

	virtual BOOL32 Update (void);	

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	virtual void CastVisibleArea (void);			

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	U32 GetSystemID (void) const
	{
		return systemID;
	}
	U32 GetPlayerID (void) const
	{
		return dwMissionID & PLAYERID_MASK;
	}
	virtual U32 GetPartID (void) const
	{
		return dwMissionID;
	}


	void RevealFog (const U32 currentSystem);
	
	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations (void);

	/* Trooppod methods */

};
//---------------------------------------------------------------------------
//
Trooppod::Trooppod (void)
{
}
//---------------------------------------------------------------------------
//
Trooppod::~Trooppod (void)
{
	if (objMapSystemID)
		OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare);

	if (podShip[0])
	{
		U32 c;
		for (c = 0; c < MAX_TROOPSHIP_PODS; c++)
		{
			delete podShip[c].Ptr();	// objects are not on the object list!
			podShip[c] = 0;
		}
	}
}
//---------------------------------------------------------------------------
//
BOOL32 Trooppod::Update (void)	
{
	U32 result=0;

	// update the pods of the troopship
	U32 c;
	for (c = 0; c < MAX_TROOPSHIP_PODS; c++)
	{
		if (podShip[c])
		{
			podShip[c].Ptr()->Update();
		}
	}

	if (target!=0 && (podLifetime -= ELAPSED_TIME) > 0)
		result = 1;
	else
	if (target)
	{
		MPartNC part = target;

		part->playerID = (part->dwMissionID & PLAYERID_MASK);	// make sure color matches the missionID
	}

	return result;
}
//---------------------------------------------------------------------------
//
void Trooppod::PhysicalUpdate (SINGLE dt)
{
	U32 c;

	if (podShip[0])
	{
		for (c = 0; c < MAX_TROOPSHIP_PODS; c++)
		{
			podShip[c].Ptr()->PhysicalUpdate(dt);
		}
	}
}
//---------------------------------------------------------------------------
//
void Trooppod::Render (void)
{
	U32 c;

	if (!bFadingPods)
	{
		for (c = 0; c < MAX_TROOPSHIP_PODS; c++)
		{
			if (podShip[c])
			{
				podShip[c].Ptr()->Render();
			}
		}
	}

	// TODO: fade away the main ship!
}
//---------------------------------------------------------------------------
//
void Trooppod::RevealFog (const U32 currentSystem)
{
	if (systemID==currentSystem && IsVisibleToPlayer(MGlobals::GetThisPlayer()))
	{
		FOGOFWAR->RevealZone(this, 1, 0);
	}
}
//---------------------------------------------------------------------------
//
void Trooppod::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	IBaseObject::TestVisible(defaults, currentSystem, currentPlayer);

	int i;

	if (podShip[0])
	{
		for (i = 0; i < MAX_TROOPSHIP_PODS; i++)
		{
			podShip[i].Ptr()->TestVisible(defaults, currentSystem, currentPlayer);
		}
	}
}
//---------------------------------------------------------------------------
//
void Trooppod::CastVisibleArea (void)
{
	SetVisibleToAllies(GetVisibilityFlags());
}
//---------------------------------------------------------------------------
//
BOOL32 Trooppod::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "TROOPPOD_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	TROOPPOD_SAVELOAD save;

	if (target!=0)
		dwTargetID = target.Ptr()->GetPartID();
	else
		dwTargetID = 0;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memcpy(&save, static_cast<TROOPPOD_SAVELOAD *>(this), sizeof(TROOPPOD_SAVELOAD));

	//
	// only save control information if on patrol
	// 
	file->WriteFile(0, &save, sizeof(TROOPPOD_SAVELOAD), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 Trooppod::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "TROOPPOD_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	TROOPPOD_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("TROOPPOD_SAVELOAD", buffer, &load);

	memcpy(static_cast<TROOPPOD_SAVELOAD *>(this), &load, sizeof(TROOPPOD_SAVELOAD));

	result = 1;
Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void Trooppod::ResolveAssociations (void)
{
	if (dwTargetID)
		OBJLIST->FindObject(dwTargetID, NONSYSVOLATILEPTR, target);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createTrooppod (void)
{
	Trooppod * obj = new ObjectImpl<Trooppod>;
	obj->objClass = OC_EFFECT;
	return obj;
}
//------------------------------------------------------------------------------------------
//---------------------------Trooppod Factory------------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE TrooppodFactory : public IObjectFactory
{
	struct OBJTYPE
	{
		const BT_TROOPPOD_DATA * pData;
		PARCHETYPE pPodType, pArchetype;
		
		void * operator new (size_t size)
		{
			return calloc(size, 1);
		}

		void   operator delete (void *ptr)
		{
			::free(ptr);
		}

		~OBJTYPE (void)
		{
			if (pPodType)
				ARCHLIST->Release(pPodType, OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(TrooppodFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	TrooppodFactory (void) { }

	~TrooppodFactory (void);

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

	/* TrooppodFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
TrooppodFactory::~TrooppodFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void TrooppodFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE TrooppodFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_EFFECT) 
	{
		BT_TROOPPOD_DATA * data = (BT_TROOPPOD_DATA *) _data;

		if (data->fxClass == FX_TROOPPOD)
		{
			result = new OBJTYPE();
			result->pData = data;
			if(data->podType[0])
			{
				result->pPodType = ARCHLIST->LoadArchetype(data->podType);
				ARCHLIST->AddRef(result->pPodType, OBJREFNAME);
			}
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 TrooppodFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * TrooppodFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	IBaseObject * result = createTrooppod();
	result->pArchetype = objtype->pArchetype;
	return result;
}
//-------------------------------------------------------------------
//
void TrooppodFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//
void __stdcall InitializeTroopPod (IBaseObject * _pod, IBaseObject * troopship, IBaseObject * target)
{
	CQASSERT(_pod->objClass == OC_EFFECT);
	CQASSERT(troopship->objClass == OC_SPACESHIP);
	// first set up the pods
	TrooppodFactory::OBJTYPE * pType = (TrooppodFactory::OBJTYPE *) ARCHLIST->GetArchetypeHandle(_pod->pArchetype);
	CQASSERT(pType->pData->fxClass == FX_TROOPPOD);
	Trooppod * pod = (Trooppod *) _pod;
	S32 instanceIndex = troopship->GetObjectIndex();
	U32 systemID = troopship->GetSystemID();
	U32 i;	
	const RECT * sliceArray;
	SINGLE sliceWidth;
	SINGLE sliceMin;
	U8 numSlices;
	Vector targetDir;

	VOLPTR(IExtent) victim = target;
	CQASSERT(victim);
	victim->GetExtentInfo(sliceArray, &sliceWidth, &sliceMin, &numSlices);

	// watch out (dont step over this line!!!)
	targetDir = target->GetTransform().translation - troopship->GetTransform().translation;
	targetDir.z = 0;
	targetDir.normalize();

	pod->transform = troopship->GetTransform();
	pod->systemID = troopship->GetSystemID();
	pod->dwMissionID = troopship->GetPartID() | 0x01000000;
	pod->SetVisibleToAllies(troopship->GetVisibilityFlags());
	pod->UpdateVisibilityFlags();

	if (pType->pPodType)
	for (i = 0; i < MAX_TROOPSHIP_PODS; i++)
	{
		if (pType->pData->podHardpoints[i][0])
		{
			IBaseObject *obj = ARCHLIST->CreateInstance(pType->pPodType);
			CQASSERT(obj);
			
			if (obj)
			{
				obj->QueryInterface(IShuttleID, pod->podShip[i], NONSYSVOLATILEPTR);
				
				CQASSERT(pod->podShip[i] != 0);
				pod->podShip[i]->InitBuildShip(pod);

				HardpointInfo  hardpointInfo;
				INSTANCE_INDEX hardpointIndex;
				FindHardpoint(pType->pData->podHardpoints[i], hardpointIndex, hardpointInfo, instanceIndex);
				pod->podShip[i]->SetStartHardpoint(hardpointInfo, hardpointIndex);

				int sliceIdx = rand()%numSlices;
				Vector targetPos;

				targetPos.x = sliceArray[sliceIdx].right;
				targetPos.y = sliceArray[sliceIdx].top;
				targetPos.z = rand()%(sliceArray[sliceIdx].top - sliceArray[sliceIdx].bottom);
			
				pod->podShip[i]->WorkAtShipPos(target, targetPos, targetDir);
			}
		}
		else
			break;
	}
	
	pod->dwTargetID = target->GetPartID();
	target->QueryInterface(IBaseObjectID, pod->target, NONSYSVOLATILEPTR);
	pod->podLifetime = 2;
	
	//
	// init objmap stuff
	//

	CQASSERT(pod->objMapNode==0);
	pod->objMapSquare = OBJMAP->GetMapSquare(pod->systemID, pod->transform.translation);
	pod->objMapSystemID = pod->systemID;
	pod->objMapNode = OBJMAP->AddObjectToMap(pod, pod->objMapSystemID, pod->objMapSquare, 0);
	CQASSERT(pod->objMapNode);

/*
	VOLPTR(IExtent) extentObj=troopship;
	CQASSERT(extentObj);
	IMeshInfoTree *mi_tree = extentObj->GetMeshInfoTree();

	// create the debris and set up the debris pointer
	IBaseObject* obj = NULL;
	PHYS_CHUNK phys;
	phys.arm = -GetCenterOfMass(mi_tree->GetMeshInfo());

	extentObj->SetMeshInfoTree(0);
	MeshChain *mc = new MeshChain;
	mc->numChildren = mi_tree->ListChildren(mc->mi);
	mc->bOwnsChildren = true;
	for (i=0;i<mc->numChildren;i++)
	{
		mc->mi[i]->parent->LoseOwnershipOfMeshInfo();
		mc->mi[i]->parent = 0;
	}
	DestroyMeshInfoTree(mi_tree);
*/
//	obj = CreateDebris(1, false, mc, NULL, systemID,&phys);
//	if (obj)
//		OBJLIST->AddObject(obj);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _trooppod : GlobalComponent
{
	TrooppodFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<TrooppodFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _trooppod __trooppod;

//---------------------------------------------------------------------------
//----------------------------End TroopPod.cpp--------------------------------
//---------------------------------------------------------------------------