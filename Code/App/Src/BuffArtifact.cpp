//--------------------------------------------------------------------------//
//                                                                          //
//                              BuffArtifact.cpp                            //
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

#include <TSmartPointer.h>
#include <TComponent.h>
#include <IConnection.h> 

#include <stdlib.h>


struct BuffArtifactArchetype
{
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE BuffArtifact : IBaseObject, IArtifact
{
	BEGIN_MAP_INBOUND(BuffArtifact)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IArtifact)
	END_MAP()

	BT_BUFF_ARTIFACT* pData;

	OBJPTR<IArtifactHolder> owner;	// person who created us
	U32 ownerID;

	//----------------------------------
	//----------------------------------
	
	BuffArtifact (void);

	~BuffArtifact (void);	

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

	/* BuffArtifact methods */
	void init (PARCHETYPE pArchetype);
};

//---------------------------------------------------------------------------
//
BuffArtifact::BuffArtifact (void) 
{
}
//---------------------------------------------------------------------------
//
BuffArtifact::~BuffArtifact (void)
{
}
//---------------------------------------------------------------------------
//
void BuffArtifact::init (PARCHETYPE _pArchetype)
{
	pData = (BT_BUFF_ARTIFACT *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->wpnClass == WPN_ART_BUFF);
	CQASSERT(pData->objClass == OC_WEAPON);

	pArchetype = _pArchetype;
	objClass = OC_WEAPON;
}
//---------------------------------------------------------------------------
//
void BuffArtifact::InitArtifact(struct IBaseObject * _owner)
{
	_owner->QueryInterface(IArtifactHolderID,owner,NONSYSVOLATILEPTR);
	ownerID = _owner->GetPartID();
	if(owner)
	{
		owner.Ptr()->effectFlags.bInihibitorBreak = true;
	}
}
//---------------------------------------------------------------------------
//
void BuffArtifact::RemoveArtifact()
{
	if(owner)
	{
		owner.Ptr()->effectFlags.bInihibitorBreak = false;
	}
}
//---------------------------------------------------------------------------
//
ArtifactButtonInfo * BuffArtifact::GetButtonInfo()
{
	return &(pData->buttonInfo);
}
//---------------------------------------------------------------------------
//
bool BuffArtifact::IsUsable()
{
	return false;
}
//---------------------------------------------------------------------------
//
bool BuffArtifact::IsToggle()
{
	return false;
}
//---------------------------------------------------------------------------
//
bool BuffArtifact::IsTargetArea()
{
	return false;
}
//---------------------------------------------------------------------------
//
bool BuffArtifact::IsTargetPlanet()
{
	return false;
}
//---------------------------------------------------------------------------
//
void BuffArtifact::SetTarget(IBaseObject * target)
{
}
//---------------------------------------------------------------------------
//
U32 BuffArtifact::GetSyncDataSize()
{
	return 0;
}
//---------------------------------------------------------------------------
//
U32 BuffArtifact::GetSyncData(void * buffer)
{
	return 0;
}
//---------------------------------------------------------------------------
//
void BuffArtifact::PutSyncData(void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
S32 BuffArtifact::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 BuffArtifact::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
BOOL32 BuffArtifact::Update (void)
{
	return 1;
}
//---------------------------------------------------------------------------
//
const TRANSFORM & BuffArtifact::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createBuffArtifact (PARCHETYPE pArchetype)
{
	BuffArtifact * buffArtifact = new ObjectImpl<BuffArtifact>;

	buffArtifact->init(pArchetype);

	return buffArtifact;
}
//------------------------------------------------------------------------------------------
//---------------------------BuffArtifact Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE BuffArtifactFactory : public IObjectFactory
{
	struct OBJTYPE : BuffArtifactArchetype
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

	BEGIN_DACOM_MAP_INBOUND(BuffArtifactFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	BuffArtifactFactory (void) { }

	~BuffArtifactFactory (void);

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

	/* BuffArtifactFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
BuffArtifactFactory::~BuffArtifactFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void BuffArtifactFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE BuffArtifactFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_WEAPON)
	{
		BT_BUFF_ARTIFACT * data = (BT_BUFF_ARTIFACT *) _data;

		if (data->wpnClass == WPN_ART_BUFF)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 BuffArtifactFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * BuffArtifactFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createBuffArtifact(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void BuffArtifactFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _buffArtifact : GlobalComponent
{
	BuffArtifactFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<BuffArtifactFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _buffArtifact __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End BuffArtifact.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------