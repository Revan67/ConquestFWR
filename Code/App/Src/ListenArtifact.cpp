//--------------------------------------------------------------------------//
//                                                                          //
//                            ListenArtifact.cpp                            //
//                                                                          //
//                  COPYRIGHT (C) 2004 By Warthog TX, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header:
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "Startup.h"
#include "ArchHolder.h"
#include "MPart.h"
#include <DArtifacts.h>
#include "IArtifact.h"
#include "Objlist.h"
#include "ObjMap.h"
#include "ObjMapIterator.h"

#include <TSmartPointer.h>
#include <TComponent.h>
#include <IConnection.h> 

#include <stdlib.h>


struct ListenArtifactArchetype
{
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE ListenArtifact : IBaseObject, IArtifact
{
	BEGIN_MAP_INBOUND(ListenArtifact)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IArtifact)
	END_MAP()

	BT_LISTEN_ARTIFACT* pData;

	OBJPTR<IArtifactHolder> owner;	// person who created us
	U32 ownerID;

	SINGLE timer;

	//----------------------------------
	//----------------------------------
	
	ListenArtifact (void);

	~ListenArtifact (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const;

	virtual BOOL32 Update (void);	// returning FALSE causes destruction

	virtual S32 GetObjectIndex (void) const;

	virtual U32 GetPartID (void) const;

	virtual U32 GetSystemID (void) const
	{
		return owner.Ptr()->GetSystemID();
	}

	virtual U32 GetPlayerID (void) const
	{
		return owner.Ptr()->GetPlayerID();
	}

	/* IArtifact methods */
	virtual void InitArtifact(struct IBaseObject * _owner);

	virtual void RemoveArtifact();

	virtual struct ArtifactButtonInfo * GetButtonInfo();

	virtual bool IsUsable();

	virtual bool IsToggle();

	virtual bool IsTargetArea();

	virtual bool IsTargetPlanet();

	virtual void SetTarget(IBaseObject * target);

	virtual U32 GetSyncDataSize();

	virtual U32 GetSyncData(void * buffer);

	virtual void PutSyncData(void * buffer, U32 bufferSize);

	/* ListenArtifact methods */
	void init (PARCHETYPE pArchetype);
};

//---------------------------------------------------------------------------
//
ListenArtifact::ListenArtifact (void) 
{
	timer = 0;
}
//---------------------------------------------------------------------------
//
ListenArtifact::~ListenArtifact (void)
{
}
//---------------------------------------------------------------------------
//
void ListenArtifact::init (PARCHETYPE _pArchetype)
{
	pData = (BT_LISTEN_ARTIFACT *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->wpnClass == WPN_ART_LISTEN);
	CQASSERT(pData->objClass == OC_WEAPON);

	pArchetype = _pArchetype;
	objClass = OC_WEAPON;
}
//---------------------------------------------------------------------------
//
void ListenArtifact::InitArtifact(struct IBaseObject * _owner)
{
	_owner->QueryInterface(IArtifactHolderID,owner,NONSYSVOLATILEPTR);
	ownerID = _owner->GetPartID();
}
//---------------------------------------------------------------------------
//
void ListenArtifact::RemoveArtifact()
{
}
//---------------------------------------------------------------------------
//
ArtifactButtonInfo * ListenArtifact::GetButtonInfo()
{
	return &(pData->buttonInfo);
}
//---------------------------------------------------------------------------
//
bool ListenArtifact::IsUsable()
{
	return false;
}
//---------------------------------------------------------------------------
//
bool ListenArtifact::IsToggle()
{
	return false;
}
//---------------------------------------------------------------------------
//
bool ListenArtifact::IsTargetArea()
{
	return false;
}
//---------------------------------------------------------------------------
//
bool ListenArtifact::IsTargetPlanet()
{
	return false;
}
//---------------------------------------------------------------------------
//
void ListenArtifact::SetTarget(IBaseObject * target)
{
}
//---------------------------------------------------------------------------
//
U32 ListenArtifact::GetSyncDataSize()
{
	return 0;
}
//---------------------------------------------------------------------------
//
U32 ListenArtifact::GetSyncData(void * buffer)
{
	return 0;
}
//---------------------------------------------------------------------------
//
void ListenArtifact::PutSyncData(void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
S32 ListenArtifact::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 ListenArtifact::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
BOOL32 ListenArtifact::Update (void)
{
	timer += ELAPSED_TIME;
	if(timer > 1.0)
	{
		timer = 0;
		//have to use objlist because CastVisibleArea uses  iterator as well
		IBaseObject * obj = OBJLIST->GetTargetList();
		while(obj)
		{
			if((obj->objClass == OC_SPACESHIP || obj->objClass == OC_PLATFORM) && (obj->GetSystemID() == owner.Ptr()->GetSystemID()) &&
				(!MGlobals::AreAllies(obj->GetPlayerID(),owner.Ptr()->GetPlayerID())))
			{
				SINGLE chance = (rand()%1000);
				chance /= 1000.0f;
				if(chance < pData->successChance)
				{
					MPart part = obj;
					if(part.isValid())
					{
						OBJLIST->CastVisibleArea(owner.Ptr()->GetPlayerID(),obj->GetSystemID(),obj->GetGridPosition(),obj->fieldFlags,
							__max(0.75,part->sensorRadius),	part->cloakedSensorRadius);
					}
				}
			}
			obj = obj->nextTarget;
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
const TRANSFORM & ListenArtifact::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createListenArtifact (PARCHETYPE pArchetype)
{
	ListenArtifact * listenArtifact = new ObjectImpl<ListenArtifact>;

	listenArtifact->init(pArchetype);

	return listenArtifact;
}
//------------------------------------------------------------------------------------------
//---------------------------ListenArtifact Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE ListenArtifactFactory : public IObjectFactory
{
	struct OBJTYPE : ListenArtifactArchetype
	{
		PARCHETYPE pArchetype;

		void * operator new (size_t size)
		{
			return calloc(size,1);
		}

		OBJTYPE (void)
		{
		}

		~OBJTYPE (void)
		{
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ListenArtifactFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	ListenArtifactFactory (void) { }

	~ListenArtifactFactory (void);

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

	/* ListenArtifactFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
ListenArtifactFactory::~ListenArtifactFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void ListenArtifactFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE ListenArtifactFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_WEAPON)
	{
		BT_LISTEN_ARTIFACT * data = (BT_LISTEN_ARTIFACT *) _data;

		if (data->wpnClass == WPN_ART_LISTEN)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 ListenArtifactFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * ListenArtifactFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createListenArtifact(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void ListenArtifactFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _listenArtifact : GlobalComponent
{
	ListenArtifactFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<ListenArtifactFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _listenArtifact __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End ListenArtifact.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------