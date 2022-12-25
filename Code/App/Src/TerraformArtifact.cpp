//--------------------------------------------------------------------------//
//                                                                          //
//                         TerraformArtifact.cpp                            //
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
#include "IPlanet.h"
#include "OpAgent.h"

#include <TSmartPointer.h>
#include <TComponent.h>
#include <IConnection.h> 

#include <stdlib.h>


struct TerraformArtifactArchetype
{
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE TerraformArtifact : IBaseObject, IArtifact
{
	BEGIN_MAP_INBOUND(TerraformArtifact)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IArtifact)
	END_MAP()

	BT_TERRAFORM_ARTIFACT* pData;

	OBJPTR<IArtifactHolder> owner;	// person who created us
	U32 ownerID;

	OBJPTR<IPlanet> victim;	

	bool bFired;

	//----------------------------------
	//----------------------------------
	
	TerraformArtifact (void);

	~TerraformArtifact (void);	

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

	/* TerraformArtifact methods */
	void init (PARCHETYPE pArchetype);
};

//---------------------------------------------------------------------------
//
TerraformArtifact::TerraformArtifact (void) 
{
	bFired = false;
}
//---------------------------------------------------------------------------
//
TerraformArtifact::~TerraformArtifact (void)
{
}
//---------------------------------------------------------------------------
//
void TerraformArtifact::init (PARCHETYPE _pArchetype)
{
	pData = (BT_TERRAFORM_ARTIFACT *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->wpnClass == WPN_ART_TERRAFORM);
	CQASSERT(pData->objClass == OC_WEAPON);

	pArchetype = _pArchetype;
	objClass = OC_WEAPON;
}
//---------------------------------------------------------------------------
//
void TerraformArtifact::InitArtifact(struct IBaseObject * _owner)
{
	_owner->QueryInterface(IArtifactHolderID,owner,NONSYSVOLATILEPTR);
	ownerID = _owner->GetPartID();
}
//---------------------------------------------------------------------------
//
void TerraformArtifact::RemoveArtifact()
{
}
//---------------------------------------------------------------------------
//
ArtifactButtonInfo * TerraformArtifact::GetButtonInfo()
{
	return &(pData->buttonInfo);
}
//---------------------------------------------------------------------------
//
bool TerraformArtifact::IsUsable()
{
	return true;
}
//---------------------------------------------------------------------------
//
bool TerraformArtifact::IsToggle()
{
	return false;
}
//---------------------------------------------------------------------------
//
bool TerraformArtifact::IsTargetArea()
{
	return false;
}
//---------------------------------------------------------------------------
//
bool TerraformArtifact::IsTargetPlanet()
{
	return true;
}
//---------------------------------------------------------------------------
//
void TerraformArtifact::SetTarget(IBaseObject * target)
{
	if(target)
		target->QueryInterface(IPlanetID,victim,NONSYSVOLATILEPTR);
	else
		victim = NULL;
}
//---------------------------------------------------------------------------
//
U32 TerraformArtifact::GetSyncDataSize()
{
	if(bFired)
		return 1;
	return 0;
}
//---------------------------------------------------------------------------
//
U32 TerraformArtifact::GetSyncData(void * buffer)
{
	if(bFired)
		return 1;
	return 0;
}
//---------------------------------------------------------------------------
//
void TerraformArtifact::PutSyncData(void * buffer, U32 bufferSize)
{
	CQASSERT(bufferSize == 1);
	if(victim)
	{
		Vector hitDir = victim.Ptr()->GetPosition()-owner.Ptr()->GetPosition();
		hitDir.fast_normalize();
		victim->TeraformPlanet(pData->targetPlanetType,pData->changeTime,hitDir);
	}
	owner->DestroyArtifact();
}
//---------------------------------------------------------------------------
//
S32 TerraformArtifact::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 TerraformArtifact::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
BOOL32 TerraformArtifact::Update (void)
{
	if(victim && (!bFired))
	{
		if((owner.Ptr()->GetGridPosition() - victim.Ptr()->GetGridPosition())*GRIDSIZE < owner->GetWeaponRange())
		{
			Vector hitDir = victim.Ptr()->GetPosition()-owner.Ptr()->GetPosition();
			hitDir.fast_normalize();
			victim->TeraformPlanet(pData->targetPlanetType,pData->changeTime,hitDir);
			bFired = true;
			THEMATRIX->ForceSyncData(owner.Ptr());
			//after this call we have been deleted, be careful
			owner->DestroyArtifact();
			return 1;
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
const TRANSFORM & TerraformArtifact::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createTerraformArtifact (PARCHETYPE pArchetype)
{
	TerraformArtifact * terraformArtifact = new ObjectImpl<TerraformArtifact>;

	terraformArtifact->init(pArchetype);

	return terraformArtifact;
}
//------------------------------------------------------------------------------------------
//---------------------------TerraformArtifact Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE TerraformArtifactFactory : public IObjectFactory
{
	struct OBJTYPE : TerraformArtifactArchetype
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

	BEGIN_DACOM_MAP_INBOUND(TerraformArtifactFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	TerraformArtifactFactory (void) { }

	~TerraformArtifactFactory (void);

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

	/* TerraformArtifactFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
TerraformArtifactFactory::~TerraformArtifactFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void TerraformArtifactFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE TerraformArtifactFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_WEAPON)
	{
		BT_TERRAFORM_ARTIFACT * data = (BT_TERRAFORM_ARTIFACT *) _data;

		if (data->wpnClass == WPN_ART_TERRAFORM)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 TerraformArtifactFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * TerraformArtifactFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createTerraformArtifact(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void TerraformArtifactFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _terraformArtifact : GlobalComponent
{
	TerraformArtifactFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<TerraformArtifactFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _terraformArtifact __ship;

//---------------------------------------------------------------------------------------------
//-------------------------------End TerraformArtifact.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------