//--------------------------------------------------------------------------//
//                                                                          //
//                               GeneralPlat.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/GeneralPlat.cpp 18    8/16/00 9:28p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TPlatform.h"
#include "Startup.h"
#include "MGlobals.h"
#include "ObjList.h"
#include "GenData.h"

#include <DGeneralPlat.h>

#include <Physics.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <HeapObj.h>

#define HOST_CHECK THEMATRIX->IsMaster()

#define COMP_OP(operation) { U32 tempID = workingID; workingID = 0; THEMATRIX->OperationCompleted(tempID,operation);}

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE GeneralPlat :Platform<GENERALPLAT_SAVELOAD, GENERALPLAT_INIT>,
											BASE_GENERALPLAT_SAVELOAD
{
	BEGIN_MAP_INBOUND(GeneralPlat)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IPlatform)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()

	SaveNode		saveNode;
	LoadNode		loadNode;
	ResolveNode		resolveNode;

	GeneralPlat (void);

	virtual ~GeneralPlat (void);	// See ObjList.cpp

	/* IExplosionOwner methods */

	virtual void OnChildDeath (INSTANCE_INDEX child)
	{
	}


	// IPlatform method

	virtual U32 GetPlanetID ()
	{
		return buildPlanetID;
	};

	virtual U32 GetSlotID ()
	{
		return buildSlot;
	};

	virtual TRANSFORM GetShipTransform ();

	virtual TRANSFORM GetDockTransform ()
	{
		return transform;
	}

	virtual void SetRallyPoint (const struct NETGRIDVECTOR & point)
	{
		CQBOMB0("What!?");
	}

	virtual bool GetRallyPoint (struct NETGRIDVECTOR & point)
	{
		return false;
	}

	virtual bool IsDockLocked()
	{
		return false;
	}

	virtual void LockDock(IBaseObject * locker)
	{
	}

	virtual void UnlockDock(IBaseObject * locker)
	{
	}

	virtual void FreePlanetSlot()
	{
		if (buildPlanetID && buildSlot)
		{
			IBaseObject * obj = OBJLIST->FindObject(buildPlanetID);
			OBJPTR<IPlanet> planet;

			if (obj && obj->QueryInterface(IPlanetID, planet))
				planet->DeallocateBuildSlot(dwMissionID, buildSlot);
			buildPlanetID = buildSlot = 0;
		}
	}

	virtual void ParkYourself (const TRANSFORM & _trans, U32 planetID, U32 slotID);
	
	virtual U32 GetNumDocking ()
	{
		CQBOMB0("What!?");
		return 0;
	}

	virtual void IncNumDocking ()
	{
		CQBOMB0("What!?");
	}

	virtual void DecNumDocking ()
	{
		CQBOMB0("What!?");
	}

	/* Platform methods */


	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "GENERALPLAT_SAVELOAD";
	}

	virtual void * getViewStruct (void)				// must be overriden implemented by derived class
	{
		return static_cast<BASE_GENERALPLAT_SAVELOAD *>(this);
	}

	/* BuildPlat methods */

	bool initGeneralPlat (const GENERALPLAT_INIT & data);

	void save (GENERALPLAT_SAVELOAD & save);

	void load (GENERALPLAT_SAVELOAD & load);

	void resolve (void);
};
//---------------------------------------------------------------------------
//
GeneralPlat::GeneralPlat (void) :
			saveNode(this, CASTSAVELOADPROC(&GeneralPlat::save)),
			loadNode(this, CASTSAVELOADPROC(&GeneralPlat::load)),
			resolveNode(this, ResolveProc(&GeneralPlat::resolve))
{
}
//---------------------------------------------------------------------------
//
GeneralPlat::~GeneralPlat (void)
{
}
//---------------------------------------------------------------------------
//
void GeneralPlat::ParkYourself (const TRANSFORM & _transform, U32 planetID, U32 slotID)
{
	transform = _transform;

	IBaseObject * obj = OBJLIST->FindObject(planetID);
	OBJPTR<IPlanet> planet;

	if (obj && obj->QueryInterface(IPlanetID, planet))
	{
		U32 slotUser = planet->GetSlotUser(slotID);
		if(slotUser != dwMissionID)
			planet->AllocateBuildSlot(dwMissionID, slotID);
		systemID = obj->GetSystemID();
	}
	
	buildPlanetID = planetID;
	buildSlot = slotID;
	if(mObjClass == M_JUMPPLAT || mObjClass == M_HQ || mObjClass == M_COCOON || mObjClass == M_ACROPOLIS || mObjClass == M_LOCUS)
	{
		SECTOR->ComputeSupplyForAllPlayers();
	}
}
//---------------------------------------------------------------------------
//
TRANSFORM GeneralPlat::GetShipTransform()
{
	return TRANSFORM();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//

bool GeneralPlat::initGeneralPlat (const GENERALPLAT_INIT & data)
{
	return true;
}
//---------------------------------------------------------------------------
//
void GeneralPlat::save (GENERALPLAT_SAVELOAD & save)
{
	save.generalPlatSaveload = *static_cast<BASE_GENERALPLAT_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void GeneralPlat::load (GENERALPLAT_SAVELOAD & load)
{
 	*static_cast<BASE_GENERALPLAT_SAVELOAD *>(this) = load.generalPlatSaveload;
}
//---------------------------------------------------------------------------
//
void GeneralPlat::resolve (void)
{
}
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createPlatform (const GENERALPLAT_INIT & data)
{
	GeneralPlat * obj = new ObjectImpl<GeneralPlat>;

	obj->FRAME_init(data);
	if (obj->initGeneralPlat(data) == false)
	{
		delete obj;
		obj = 0;
	}

	return obj;
}
//------------------------------------------------------------------------------------------
//
GENERALPLAT_INIT::~GENERALPLAT_INIT (void)					// free archetype references
{
	// nothing yet!?
}
//------------------------------------------------------------------------------------------
//---------------------------GeneralPlat Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE GeneralPlatFactory : public IObjectFactory
{
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(GeneralPlatFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	GeneralPlatFactory (void) { }

	~GeneralPlatFactory (void);

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

	/* GeneralPlatFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
GeneralPlatFactory::~GeneralPlatFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void GeneralPlatFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE GeneralPlatFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	GENERALPLAT_INIT * result = 0;

	if (objClass == OC_PLATFORM)
	{
		BT_PLAT_GENERAL_DATA * data = (BT_PLAT_GENERAL_DATA *) _data;
		
		if (data->type == PC_GENERAL)
		{
			result = new GENERALPLAT_INIT;

			if (result->loadPlatformArchetype(data, ARCHLIST->GetArchetype(szArchname)))
			{
				// do something here...
			}
			else
				goto Error;
		}

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
BOOL32 GeneralPlatFactory::DestroyArchetype (HANDLE hArchetype)
{
	GENERALPLAT_INIT * objtype = (GENERALPLAT_INIT *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * GeneralPlatFactory::CreateInstance (HANDLE hArchetype)
{
	GENERALPLAT_INIT * objtype = (GENERALPLAT_INIT *)hArchetype;
	return createPlatform(*objtype);
}
//-------------------------------------------------------------------
//
void GeneralPlatFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	GENERALPLAT_INIT * objtype = (GENERALPLAT_INIT *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _generalplatfactory : GlobalComponent
{
	GeneralPlatFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<GeneralPlatFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _generalplatfactory __plat;

//--------------------------------------------------------------------------//
//----------------------------End GeneralPlat.cpp-------------------------------//
//--------------------------------------------------------------------------//
