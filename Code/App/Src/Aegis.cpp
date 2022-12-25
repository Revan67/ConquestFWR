//--------------------------------------------------------------------------//
//                                                                          //
//                                 Aegis.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Aegis.cpp 44    9/13/01 10:01a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "IConnection.h"
#include "Camera.h"
#include "DEffect.h"
#include "Objlist.h"
#include "Anim2D.h"
#include "CQlight.h"
#include "Sfx.h"
#include "TObjTrans.h"
#include "TobjFrame.h"
#include "Startup.h"
#include "IBlast.h"
#include "DSpecial.h"
#include "IWeapon.h"
#include "IFighter.h"
#include "ILauncher.h"
#include "Mission.h"
#include "Mpart.h"
#include "GridVector.h"
#include "ObjMapIterator.h"
#include "OpAgent.h"
#include "IShipDamage.h"

#include <Mesh.h>
#include <FileSys.h>
#include <Engine.h>
#include <IRenderPrimitive.h>
#include <Renderer.h>

#define FPS 10
#define RADIUS 5000.0
#define OBJECTSCALE (1/OBJ_RAD)
#define OBJ_RAD 103.91

struct AegisArchetype
{
	const char *name;
	BT_AEGIS_DATA *data;
	S32 archIndex;
	IMeshArchetype * meshArch;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	AegisArchetype (void)
	{
		meshArch = NULL;
		archIndex = INVALID_INSTANCE_INDEX;
	}

	~AegisArchetype (void)
	{
		if (archIndex != INVALID_INSTANCE_INDEX)
			ENGINE->release_archetype(archIndex);
	}

};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE Aegis : public ObjectTransform<ObjectFrame<IBaseObject,AEGIS_SAVELOAD,AegisArchetype> >, IAOEWeapon, ISaveLoad, ILauncher, BASE_AEGIS_SAVELOAD
{

	BEGIN_MAP_INBOUND(Aegis)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IAOEWeapon)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	//------------------------------------------

	INSTANCE_INDEX instanceIndex;
	BT_AEGIS_DATA *data;

	OBJPTR<ILaunchOwner> owner;

	SINGLE accumedSupplies;

	//------------------------------------------

	Aegis (void)
	{
		accumedSupplies = 0;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~Aegis (void);	// See ObjList.cpp

	/* IBaseObject methods */

	virtual void PhysicalUpdate (SINGLE dt);
	
	virtual BOOL32 Update ();
	
	virtual void Render (void);
	
	//---------------------------------------------------------------------------
	//
	U32 GetPartID (void) const
	{
		return owner.Ptr()->GetPartID();
	}
	
	// IAOEWeapon

	// "target" can be NULL on the client side
	// owner is the spaceship, not the launcher!!!
	virtual void InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * target, U32 flags=0, const class Vector * pos=0);

	// weapon should determine who it will damage, and how much, then return the result to the caller
	virtual U32 GetAffectedUnits (U32 partIDs[MAX_AOE_VICTIMS], U32 damage[MAX_AOE_VICTIMS])
	{
		return 0;
	}

	// caller has determined who it will damage, and how much.
	virtual void SetAffectedUnits (const U32 partIDs[MAX_AOE_VICTIMS], const U32 damage[MAX_AOE_VICTIMS])
	{
		return;
	}

	// ILauncher

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range);

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial);

	virtual void AttackObject (IBaseObject * obj);

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
	{}

	virtual void WormAttack (IBaseObject * obj)
	{};

	virtual void DoCreateWormhole(U32 systemID)
	{}
	
	virtual const bool TestFightersRetracted (void) const; // return true if fighters are retracted

	virtual void SetFighterStance(FighterStance stance)
	{
	}

	virtual void HandlePreTakeover (U32 newMissionID, U32 troopID)
	{
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const;

	virtual U32 GetSyncData (void * buffer);			// buffer points to use supplied memory

	virtual void PutSyncData (void * buffer, U32 bufferSize);

	virtual void DoSpecialAbility (U32 specialID);

	virtual void DoSpecialAbility (IBaseObject *obj){}

	virtual void DoCloak (void){};

	virtual void SpecialAttackObject (IBaseObject * obj);

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bSpecialEnabled)
	{
		MPart part = owner.Ptr();
	
		ability = USA_AEGIS;
		bSpecialEnabled =  (part->caps.specialAbilityOk);
	}

	virtual const U32 GetApproxDamagePerSecond (void) const
	{
		return 0;
	}

	virtual void InformOfCancel() {};

	virtual void LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize) {};

	virtual void LauncherReceiveOpData(U32 agentID, void * buffer, U32 bufferSize) {};

	virtual void LauncherOpCompleted(U32 agentID) {};

	virtual bool CanCloak(){return false;};

	virtual bool IsToggle() {return true;};

	virtual bool CanToggle()
	{
		MPart part = owner.Ptr();
		if(part.isValid())
		{
			if(part->supplies)
				return true;
		}
		return false;
	};

	virtual bool IsOn()
	{
		return bShieldOn;
	};

	virtual void OnAllianceChange (U32 allyMask)
	{
		// don't do anything
	}

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	// Aegis methods

	void findTarget (OBJPTR<IWeaponTarget> & target);

	void init(AegisArchetype *arch);
};

//----------------------------------------------------------------------------------
//
Aegis::~Aegis (void)
{
}
//----------------------------------------------------------------------------------
//
void Aegis::PhysicalUpdate (SINGLE dt)
{
}

BOOL32 Aegis::Update ()
{
	MPartNC part(owner.Ptr());
	if(part.isValid())
	{
		if(!(part->caps.specialAbilityOk))
		{
			BT_AEGIS_DATA * data = (BT_AEGIS_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
			if(data->neededTech.raceID)
			{
				if(MGlobals::GetCurrentTechLevel(owner.Ptr()->GetPlayerID()).HasTech(data->neededTech))
				{
					part->caps.specialAbilityOk = true;
				}
			}
			else
			{
				part->caps.specialAbilityOk = true;
			}
		}
	}

	if(THEMATRIX->IsMaster() && bShieldOn)
	{
		accumedSupplies += data->supplyPerSec*ELAPSED_TIME;
		if(accumedSupplies >= 1)
		{
			S32 useSup = accumedSupplies;
			accumedSupplies -= useSup;
			owner->UseSupplies(useSup);
		}

		MPart part(owner.Ptr());
		if(part->supplies == 0 || (owner.Ptr()->fieldFlags.suppliesLocked()))
		{
			bShieldOn = false;
			owner.Ptr()->effectFlags.bAgeisShield = false;
		}

	}

	if(bShieldOn)
	{
		OBJPTR<IShipDamage> shipDam;
		owner.Ptr()->QueryInterface(IShipDamageID,shipDam);
		shipDam->FancyShieldRender();
	}

	return 1;
}
//----------------------------------------------------------------------------------
//
void Aegis::Render (void)
{
}
//----------------------------------------------------------------------------------
//
void Aegis::InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * target, U32 flags, const class Vector * _pos)
{
}

// ILauncher

void Aegis::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	CQASSERT(_owner);
	//ownerID = _owner->GetPartID();
	_owner->QueryInterface(ILaunchOwnerID,owner,LAUNCHVOLATILEPTR);
}

void Aegis::AttackPosition (const struct GRIDVECTOR * position, bool bSpecial)
{
	CQBOMB0("AttackPosition not supported");
}

void Aegis::AttackObject (IBaseObject * obj)
{
}

const bool Aegis::TestFightersRetracted (void) const
{
	return true;
}  // return true if fighters are retracted

// the following methods are for network synchronization of realtime objects
U32 Aegis::GetSyncDataSize (void) const
{
	return 1;
}

U32 Aegis::GetSyncData (void * buffer)			// buffer points to use supplied memory
{
	if(bShieldOn != bNetShieldOn)
	{
		bNetShieldOn = bShieldOn;
		*((U8 *)buffer) = bShieldOn;
		return 1;
	}
	return 0;
}
//---------------------------------------------------------------------------
//
void Aegis::PutSyncData (void * buffer, U32 bufferSize)
{
	bNetShieldOn = ((*((U8 *)buffer)) != 0 );
	if(bShieldOn != bNetShieldOn)
	{
		bShieldOn = bNetShieldOn;
		owner.Ptr()->effectFlags.bAgeisShield = bShieldOn;
	}
}
//---------------------------------------------------------------------------
//
void Aegis::DoSpecialAbility (U32 specialID)
{
	if(THEMATRIX->IsMaster())
	{
		if(bShieldOn)
		{
			bShieldOn = false;
			owner.Ptr()->effectFlags.bAgeisShield = false;
		}
		else
		{
			MPart part(owner.Ptr()); 
			if(part->supplies > 0)
			{
				owner.Ptr()->effectFlags.bAgeisShield = true;
				bShieldOn = true;
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void Aegis::SpecialAttackObject (IBaseObject * obj)
{
	CQBOMB0("SpecialAttackObject not supported");
}
//---------------------------------------------------------------------------
//
BOOL32 Aegis::Save (struct IFileSystem * outFile)
{
	DAFILEDESC fdesc = "AEGIS_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	AEGIS_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));
	memcpy(&save, static_cast<BASE_AEGIS_SAVELOAD *>(this), sizeof(BASE_AEGIS_SAVELOAD));

	FRAME_save(save);
	
	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 Aegis::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "AEGIS_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	AEGIS_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("AEGIS_SAVELOAD", buffer, &load);

	*static_cast<BASE_AEGIS_SAVELOAD *>(this) = load;

	FRAME_load(load);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void Aegis::ResolveAssociations()
{
}
//---------------------------------------------------------------------------
//
void Aegis::init(AegisArchetype *arch)
{
	data = arch->data;
}
//----------------------------------------------------------------------------------
//---------------------------------Aegis Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

struct DACOM_NO_VTABLE AegisManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(AegisManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

//	struct AegisNode *explosionList;
	U32 factoryHandle;


	//child object info
	AegisArchetype *pArchetype;

	//AegisManager methods

	AegisManager (void) 
	{
	}

	~AegisManager();
	
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
// AegisManager methods

AegisManager::~AegisManager()
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
void AegisManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE AegisManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	AegisArchetype *newguy = 0;
	if (objClass == OC_WEAPON)
	{
		BT_AEGIS_DATA *objData = (BT_AEGIS_DATA *)data;
		if (objData->wpnClass == WPN_AEGIS)
		{
			newguy = new AegisArchetype;
			newguy->name = szArchname;
			newguy->data = objData;

			goto Done;
		}
	}

Done:
	return newguy;
}
//--------------------------------------------------------------------------
//
BOOL32 AegisManager::DestroyArchetype(HANDLE hArchetype)
{
	AegisArchetype *deadguy = (AegisArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * AegisManager::CreateInstance(HANDLE hArchetype)
{
	AegisArchetype *pAegis = (AegisArchetype *)hArchetype;
	
	Aegis * obj = new ObjectImpl<Aegis>;
	obj->objClass = OC_EFFECT;
	obj->init(pAegis);

	return obj;
	
}
//--------------------------------------------------------------------------
//
void AegisManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}

struct AegisManager *AegisMgr;
//----------------------------------------------------------------------------------------------
//
struct _agaa : GlobalComponent
{


	virtual void Startup (void)
	{
		AegisMgr = new DAComponent<AegisManager>;
		AddToGlobalCleanupList((IDAComponent **) &AegisMgr);
	}

	virtual void Initialize (void)
	{
		AegisMgr->init();
	}
};

static _agaa agaa;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End Aegis.cpp------------------------------------
//---------------------------------------------------------------------------
