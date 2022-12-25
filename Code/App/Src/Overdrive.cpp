//--------------------------------------------------------------------------//
//                                                                          //
//                                 Overdrive.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Overdrive.cpp 20    10/13/00 12:06p Jasony $
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
#include "MGlobals.h"

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

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
struct OverdriveArchetype
{
	const char *name;
	BT_OVERDRIVE_DATA *data;
	PARCHETYPE pArchetype;

	OverdriveArchetype (void)
	{
		memset(this, 0, sizeof(*this));
	}

	~OverdriveArchetype (void)
	{
	}
};


struct _NO_VTABLE Overdrive : public IBaseObject, IWeapon, ISaveLoad, OVERDRIVE_SAVELOAD
{
	BEGIN_MAP_INBOUND(Overdrive)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IWeapon)
	_INTERFACE_ENTRY(ISaveLoad)
	END_MAP()

	//------------------------------------------
	unsigned int textureID;
	const BT_OVERDRIVE_DATA * data;
	HSOUND hSound;
	OBJPTR<IBaseObject> owner;

	//------------------------------------------

	Overdrive (void);
	virtual ~Overdrive (void);	// See ObjList.cpp

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate(SINGLE dt);

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual U32 GetPartID (void) const
	{
		return ownerID;
	}

	virtual U32 GetPlayerID (void) const
	{
		return MGlobals::GetPlayerFromPartID(ownerID);
	}

	/* IWeapon methods */

	virtual void InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * obj, U32 flags, const class Vector * pos=0);

	void init (const OverdriveArchetype & data);

	/* ISaveLoad */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();
};

//----------------------------------------------------------------------------------
//
Overdrive::Overdrive (void) 
{

}

//----------------------------------------------------------------------------------
//
Overdrive::~Overdrive (void)
{
	textureID = 0;
	if (hSound)
		SFXMANAGER->CloseHandle(hSound);
}

//---------------------------------------------------------------------------
//
BOOL32 Overdrive::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "OVERDRIVE_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	OVERDRIVE_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));
	memcpy(&save, static_cast<OVERDRIVE_SAVELOAD *>(this), sizeof(OVERDRIVE_SAVELOAD));

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 Overdrive::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "OVERDRIVE_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	OVERDRIVE_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("OVERDRIVE_SAVELOAD", buffer, &load);

	*static_cast<OVERDRIVE_SAVELOAD *>(this) = load;

	result = 1;

Done:	
	return result;
}

//---------------------------------------------------------------------------
//

void Overdrive::ResolveAssociations()
{
	OBJLIST->FindObject(ownerID, NONSYSVOLATILEPTR, owner);
}
//----------------------------------------------------------------------------------
//
BOOL32 Overdrive::Update (void)
{
	if (owner == 0)
		return 0;

	Vector dir = destPos-owner->GetPosition();
	SINGLE mag = dir.magnitude();
	if (mag < 10)
		return 0;

	return 1;
}

void Overdrive::PhysicalUpdate (SINGLE dt)
{
	Vector dir = destPos-owner->GetPosition();
	SINGLE mag = dir.magnitude();
	mag = min(dt*data->speed/mag,1);
	VOLPTR(IPhysicalObject) ship;
	if ((ship = owner) != 0)
		ship->SetPosition(owner->GetPosition()+dir*mag, owner->GetSystemID());
}
//----------------------------------------------------------------------------------
//
void Overdrive::InitWeapon (IBaseObject * _owner, const class TRANSFORM & orientation, IBaseObject * _target, U32 flags, const class Vector * pos)
{
	systemID = _owner->GetSystemID();
	ownerID = _owner->GetPartID();
	//note : this depends on ownerID grabbing the ship here
	OBJLIST->FindObject(ownerID, NONSYSVOLATILEPTR, owner);
	CQASSERT (owner != _owner);

	// fix this!!!
	/*
	MStructure ship(owner.ptr);
	ship.Cancel(M_CANCEL_ALL);
	*/
	
	destPos = *pos;

	Vector startPos = _owner->GetPosition();
	
	hSound = SFXMANAGER->Open(data->launchSfx);
	SFXMANAGER->Play(hSound,systemID,&startPos);
}
//---------------------------------------------------------------------------
//
void Overdrive::init (const OverdriveArchetype & arch)
{
	CQASSERT(arch.data->wpnClass == WPN_OVERDRIVE);
	CQASSERT(arch.data->objClass == OC_WEAPON);

	data = (BT_OVERDRIVE_DATA *)arch.data;

	pArchetype = arch.pArchetype;
	objClass = OC_WEAPON;

}

//----------------------------------------------------------------------------------------------
//-------------------------------class OverdriveManager--------------------------------------------
//----------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE OverdriveManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(OverdriveManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	U32 factoryHandle;

	//child object info
	OverdriveArchetype *pArchetype;

	//OverdriveManager methods

	OverdriveManager (void) 
	{
	}

	~OverdriveManager();
	
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
// OverdriveManager methods

OverdriveManager::~OverdriveManager()
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
void OverdriveManager::init()
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(GetBase(), &factoryHandle);
}
//--------------------------------------------------------------------------
//
HANDLE OverdriveManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *_data)
{
	if (objClass == OC_WEAPON)
	{
		BT_OVERDRIVE_DATA * data = (BT_OVERDRIVE_DATA *)_data;
		if (data->wpnClass == WPN_OVERDRIVE)
		{
			
			OverdriveArchetype *newguy = new OverdriveArchetype;
			
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
BOOL32 OverdriveManager::DestroyArchetype(HANDLE hArchetype)
{
	OverdriveArchetype *deadguy = (OverdriveArchetype *)hArchetype;
	delete deadguy;

	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * OverdriveManager::CreateInstance(HANDLE hArchetype)
{
	OverdriveArchetype *pWeapon = (OverdriveArchetype *)hArchetype;
	BT_OVERDRIVE_DATA *objData = pWeapon->data;
	
	if (objData->objClass == OC_WEAPON)
	{
		IBaseObject * obj = NULL;
		switch (objData->wpnClass)
		{
		case WPN_OVERDRIVE:
			{
				Overdrive *wpn = new ObjectImpl<Overdrive>;
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
void OverdriveManager::EditorCreateInstance(HANDLE hArchetype,const MISSION_INFO & info)
{
	// not supported
}

static struct OverdriveManager *OVERDRIVEMGR;
//----------------------------------------------------------------------------------------------
//
struct _overdrives : GlobalComponent
{
	virtual void Startup (void)
	{
		struct OverdriveManager *overdriveMgr = new DAComponent<OverdriveManager>;
		OVERDRIVEMGR = overdriveMgr;
		AddToGlobalCleanupList((IDAComponent **) &OVERDRIVEMGR);
	}

	virtual void Initialize (void)
	{
		OVERDRIVEMGR->init();
	}
};

static _overdrives overdrives;

//--------------------------------------------------------------------------//
//------------------------------END Overdrive.cpp--------------------------------//
//--------------------------------------------------------------------------//
