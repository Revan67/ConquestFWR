//--------------------------------------------------------------------------//
//                                                                          //
//                                 Swapper.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/Swapper.cpp 13    10/13/00 12:06p Jasony $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "Camera.h"
#include "Objlist.h"
#include "sfx.h"
#include "IConnection.h"
#include "Startup.h"
#include "SuperTrans.h"
#include "DSpecial.h"
#include "IWeapon.h"
#include "CQLight.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "Mission.h"
#include "IMissionActor.h"

#include <TComponent.h>
#include <lightman.h>
#include <Engine.h>
#include <Vector.h>
#include <Matrix.h>
#include <IHardpoint.h>
#include <stdlib.h>
#include <FileSys.h>
#include <ICamera.h>
#include <Pixel.h>
//#include <RPUL\PrimitiveBuilder.h>
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct SwapperArchetype
{
	const char *name;
	BT_SWAPPER_DATA *data;
	PARCHETYPE pArchetype;

	SwapperArchetype (void)
	{
		memset(this, 0, sizeof(*this));
	}

	~SwapperArchetype (void)
	{
	}
};


struct _NO_VTABLE Swapper : public IBaseObject, IWeapon, ISaveLoad//, SWAPPER_SAVELOAD
{
	BEGIN_MAP_INBOUND(Swapper)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IWeapon)
	_INTERFACE_ENTRY(ISaveLoad)
	END_MAP()

	//------------------------------------------
	unsigned int textureID;
	const BT_SWAPPER_DATA * data;
	HSOUND hSound;
	OBJPTR<IBaseObject> owner;

	//------------------------------------------

	Swapper (void);
	virtual ~Swapper (void);	// See ObjList.cpp

	virtual BOOL32 Update (void);

	//virtual void PhysicalUpdate(SINGLE dt);

	virtual U32 GetSystemID (void) const
	{
		return 0;//systemID;
	}

	virtual U32 GetPartID (void) const
	{
		return 0;//ownerID;
	}

	/* IWeapon methods */

	virtual void InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * obj, U32 flags, const class Vector * pos=0);

	void init (const SwapperArchetype & data);

	/* ISaveLoad */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();
};

//----------------------------------------------------------------------------------
//
Swapper::Swapper (void) 
{

}

//----------------------------------------------------------------------------------
//
Swapper::~Swapper (void)
{
	textureID = 0;
	if (hSound)
		SFXMANAGER->CloseHandle(hSound);
}

//---------------------------------------------------------------------------
//
BOOL32 Swapper::Save (struct IFileSystem * inFile)
{
/*	DAFILEDESC fdesc = "SWAPPER_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	SWAPPER_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));
	memcpy(&save, static_cast<SWAPPER_SAVELOAD *>(this), sizeof(SWAPPER_SAVELOAD));

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;*/
	return 1;
}
//---------------------------------------------------------------------------
//
BOOL32 Swapper::Load (struct IFileSystem * inFile)
{
	/*DAFILEDESC fdesc = "SWAPPER_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	SWAPPER_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("SWAPPER_SAVELOAD", buffer, &load);

	*static_cast<SWAPPER_SAVELOAD *>(this) = load;

	owner = OBJLIST->FindObject(ownerID);

	result = 1;

Done:	
	return result;*/
	return 1;
}

//---------------------------------------------------------------------------
//

void Swapper::ResolveAssociations()
{
	
}
//----------------------------------------------------------------------------------
//
BOOL32 Swapper::Update (void)
{
	return 0;
}

/*void Swapper::PhysicalUpdate (SINGLE dt)
{
	Vector dir = destPos-owner->GetPosition();
	SINGLE mag = dir.magnitude();
	mag = min(dt*data->speed/mag,1);
	OBJPTR<IMissionActor> ship;
	if (owner->QueryInterface(IMissionActorID,ship) != 0)
	{
		ship->SetPosition(owner->GetPosition()+dir*mag);
	}
}*/
//----------------------------------------------------------------------------------
//
void Swapper::InitWeapon (IBaseObject * _owner, const class TRANSFORM & orientation, IBaseObject * _target, U32 flags, const class Vector * pos)
{
	CQASSERT(_target != 0);

	U32 ownerID = _owner->GetPartID();
	//note : this depends on ownerID grabbing the ship here
	IBaseObject *owner = OBJLIST->FindObject(ownerID);
	CQASSERT (owner != _owner);

//	MStructure ship(owner);
	// fix this!!!
	//ship.Cancel(M_CANCEL_ALL);
	
	Vector ownerPos = owner->GetPosition();
	U32 systemID = owner->GetSystemID();

//	ship.SetPosition(_target->GetPosition());
//	ship.SetSystemID(_target->GetSystemID());

//	MStructure tship(_target);
	// fix this!!
	//tship.SetPosition(ownerPos);
	//tship.Cancel(M_CANCEL_ALL);
//	_target->SetSystemID(systemID);

	hSound = SFXMANAGER->Open(data->launchSfx);
	SFXMANAGER->Play(hSound,systemID,&ownerPos);
}
//---------------------------------------------------------------------------
//
void Swapper::init (const SwapperArchetype & arch)
{
	CQASSERT(arch.data->wpnClass == WPN_SWAPPER);
	CQASSERT(arch.data->objClass == OC_WEAPON);

	data = (BT_SWAPPER_DATA *)arch.data;

	pArchetype = arch.pArchetype;
	objClass = OC_WEAPON;

}

//----------------------------------------------------------------------------------------------
//-------------------------------class SwapperManager--------------------------------------------
//----------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE SwapperManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(SwapperManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	U32 factoryHandle;

	//child object info
	SwapperArchetype *pArchetype;

	//SwapperManager methods

	SwapperManager (void) 
	{
	}

	~SwapperManager();
	
    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	IDAComponent * GetBase (void)
	{
		return (IObjectFactory *) this;
	}

	void init();

	//IObjectFactory
	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);
};


//--------------------------------------------------------------------------
// SwapperManager methods

SwapperManager::~SwapperManager()
{
	COMPTR<IDAConnectionPoint> connection;
	if (OBJLIST)
	{
		if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
			connection->Unadvise(factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
void SwapperManager::init()
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(GetBase(), &factoryHandle);
}
//--------------------------------------------------------------------------
//
HANDLE SwapperManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *_data)
{
	if (objClass == OC_WEAPON)
	{
		BT_SWAPPER_DATA * data = (BT_SWAPPER_DATA *)_data;
		if (data->wpnClass == WPN_SWAPPER)
		{
			
			SwapperArchetype *newguy = new SwapperArchetype;
			
			newguy->name = szArchname;
			newguy->data = data;
			newguy->pArchetype = ARCHLIST->GetArchetype(szArchname);
			//
			// force preload of sound effect
			// 
			SFXMANAGER->Preload(data->launchSfx);
			
			return newguy;
		}
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
BOOL32 SwapperManager::DestroyArchetype(HANDLE hArchetype)
{
	SwapperArchetype *deadguy = (SwapperArchetype *)hArchetype;
	delete deadguy;

	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * SwapperManager::CreateInstance(HANDLE hArchetype)
{
	SwapperArchetype *pWeapon = (SwapperArchetype *)hArchetype;
	BT_SWAPPER_DATA *objData = pWeapon->data;
	
	if (objData->objClass == OC_WEAPON)
	{
		IBaseObject * obj = NULL;
		switch (objData->wpnClass)
		{
		case WPN_SWAPPER:
			{
				Swapper *wpn = new ObjectImpl<Swapper>;
				wpn->init(*pWeapon);
				obj = wpn;
			}
			break;
		}

		return obj;
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
void SwapperManager::EditorCreateInstance(HANDLE hArchetype,const MISSION_INFO & info)
{
	// not supported
}

static struct SwapperManager *SWAPPERMGR;
//----------------------------------------------------------------------------------------------
//
struct _swappers : GlobalComponent
{
	virtual void Startup (void)
	{
		struct SwapperManager *swapperMgr = new DAComponent<SwapperManager>;
		SWAPPERMGR = swapperMgr;
		AddToGlobalCleanupList((IDAComponent **) &SWAPPERMGR);
	}

	virtual void Initialize (void)
	{
		SWAPPERMGR->init();
	}
};

static _swappers swappers;

//--------------------------------------------------------------------------//
//------------------------------END Swapper.cpp--------------------------------//
//--------------------------------------------------------------------------//
