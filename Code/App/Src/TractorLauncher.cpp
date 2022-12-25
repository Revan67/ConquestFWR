//--------------------------------------------------------------------------//
//                                                                          //
//                                ArtileryLauncher.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 2004 BY Warthog                           //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header:
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "SuperTrans.h"
#include "ObjList.h"
#include "sfx.h"

#include "Mission.h"
#include "IMissionActor.h"
#include "Startup.h"
#include "ArchHolder.h"
#include "TerrainMap.h"
#include "MPart.h"
#include "TObject.h"
#include <DArtileryLauncher.h>
#include "ILauncher.h"
#include "IWeapon.h"
#include "DSpaceShip.h"
#include "IGotoPos.h"
#include "OpAgent.h"
#include "IShipMove.h"
#include "ObjMapIterator.h"
#include "CommPacket.h"
#include "sector.h"
#include "ObjSet.h"
#include "camera.h"
#include "MeshRender.h"
#include "IBlast.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h> 
#include <MGlobals.h>
#include <DMTechNode.h>
#include <renderer.h>

#include <stdlib.h>

// so that we can use the techtree stuff
using namespace TECHTREE;

struct ArtileryLauncherArchetype
{
	PARCHETYPE blastType;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE ArtileyLauncher : IBaseObject, ILauncher, ISaveLoad, ARTILERY_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(ArtileyLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	BT_ARTILERY_LAUNCHER * pData;
	ArtileryLauncherArchetype * archetype;

	OBJPTR<ILaunchOwner> owner;	// person who created us

	//----------------------------------
	//----------------------------------
	
	ArtileyLauncher (void);

	~ArtileyLauncher (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const;

	virtual BOOL32 Update (void);	// returning FALSE causes destruction
	
	virtual void PhysicalUpdate (SINGLE dt);

	virtual S32 GetObjectIndex (void) const;

	virtual U32 GetPartID (void) const;

	virtual U32 GetSystemID (void) const
	{
		return owner.ptr->GetSystemID();
	}

	virtual U32 GetPlayerID (void) const
	{
		return owner.ptr->GetPlayerID();
	}

	virtual void Render();

	/* ILauncher methods */

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range);
	

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial);

	virtual void AttackObject (IBaseObject * obj);

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
	{}

	virtual void WormAttack (IBaseObject * obj)
	{};

	virtual void DoCreateWormhole(U32 systemID){};

	virtual const bool TestFightersRetracted (void) const 
	{ 
		return true;
	}

	virtual void SetFighterStance(FighterStance stance)
	{
	}

	virtual void HandlePreTakeover (U32 newMissionID, U32 troopID)
	{
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	virtual void DoSpecialAbility (U32 specialID);

	virtual void DoSpecialAbility (IBaseObject *obj)
	{
	}

	virtual void DoCloak (void)
	{
	}
	
	virtual void SpecialAttackObject (IBaseObject * obj)
	{
	}

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bSpecialEnabled)
	{
		if(pData->bSpecial)
		{
			MPart part = owner.ptr;

			ability = USA_ARTILERY;
			bSpecialEnabled = part->caps.specialEOAOk && checkSupplies();
		}
	}

	virtual const U32 GetApproxDamagePerSecond (void) const
	{
		return 0;
	}

	virtual void InformOfCancel();

	virtual void LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize);

	virtual void LauncherReceiveOpData(U32 agentID, void * buffer, U32 bufferSize);

	virtual void LauncherOpCompleted(U32 agentID);

	virtual bool CanCloak(){return false;};

	virtual bool IsToggle() {return false;};

	virtual bool CanToggle(){return false;};

	virtual bool IsOn() {return false;};

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const
	{
		return 0;
	}

	virtual U32 GetSyncData (void * buffer)
	{
		return 0;
	}

	virtual void PutSyncData (void * buffer, U32 bufferSize)
	{
	}

	virtual void OnAllianceChange (U32 allyMask)
	{
	}

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file)
	{
		return TRUE;
	}

	virtual BOOL32 Load (struct IFileSystem * file)
	{
		return FALSE;
	}
	
	virtual void ResolveAssociations()
	{
	}

	/* ArtileryLauncher methods */
	
	void init (PARCHETYPE pArchetype, ArtileryLauncherArchetype * objtype);

	bool checkSupplies();
};

//---------------------------------------------------------------------------
//
ArtileyLauncher::ArtileyLauncher (void) 
{
}
//---------------------------------------------------------------------------
//
ArtileyLauncher::~ArtileyLauncher (void)
{
}
//---------------------------------------------------------------------------
//
void ArtileyLauncher::init (PARCHETYPE _pArchetype,ArtileryLauncherArchetype * objtype)
{
	pData = (BT_ARTILERY_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->type == LC_ARTILERY_LAUNCH);
	CQASSERT(pData->objClass == OC_LAUNCHER);

	archetype = objtype;
	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;
}
//---------------------------------------------------------------------------
//
void ArtileyLauncher::DoSpecialAbility (U32 specialID)
{

}
//---------------------------------------------------------------------------
//
S32 ArtileyLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.ptr->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 ArtileyLauncher::GetPartID (void) const
{
	return owner.ptr->GetPartID();
}
//---------------------------------------------------------------------------
//
void ArtileyLauncher::Render()
{
}
//---------------------------------------------------------------------------
//
BOOL32 ArtileyLauncher::Update (void)
{
	if(bShooting)
	{
		if((targetPos-owner.ptr->GetPosition()).magnitude_squared() <= owner->GetWeaponRange()*owner->GetWeaponRange())
		{
			timer += ELAPSED_TIME;
			if(timer > 1.0)
			{
				timer -= 1.0;
				if(owner->UseSupplies(pData->supplyCost,pData->bSpecial))
				{
					ObjMapIterator iter(owner.ptr->GetSystemID(),targetPos,pData->areaRadius*GRIDSIZE);
					while(iter)
					{
						IBaseObject * obj = iter->obj;
						if(obj->objClass == OC_PLATFORM || obj->objClass == OC_SPACESHIP)
						{
							if(obj->GetGridPosition()-targetPos <= pData->areaRadius)
							{
								VOLPTR(IWeaponTarget) target = obj;
								if(target)
								{
									target->ApplyAOEDamage(owner.ptr->GetPlayerID(),pData->damagePerSec);
								}
							}
						}
						++iter;
					}
					if(pData->bSpecial)
					{
						for(U32 i = 0; i < 10; ++i)
						{
							if(archetype->blastType != 0)
							{
								IBaseObject * obj = ARCHLIST->CreateInstance(archetype->blastType);
								if(obj)
								{
									OBJPTR<IBlast> blast;
									obj->QueryInterface(IBlastID,blast);
									if(blast != 0)
									{
										Transform blastTrans;
										Vector center = targetPos;
										SINGLE angle = (((SINGLE)(rand()%1000))/1000.0f)*(2*PI);
										SINGLE range = (((SINGLE)(rand()%1000))/1000.0f)*(pData->areaRadius*GRIDSIZE);

										center += range*Vector(cos(angle),sin(angle),0);
										blastTrans.set_position(center);
										blast->InitBlast(blastTrans,owner.ptr->GetSystemID(),NULL);
										OBJLIST->AddObject(obj);
									}
								}
							}
						}
						bShooting = false;//only shoot once
					}
				}
				else
				{
					if(pData->bSpecial)
					{
						bShooting = false;
					}
				}
			}
			if(!pData->bSpecial && checkSupplies())
			{
				if(rand()%3 == 0)//1/3 of the time
				{
					if(archetype->blastType != 0)
					{
						IBaseObject * obj = ARCHLIST->CreateInstance(archetype->blastType);
						if(obj)
						{
							OBJPTR<IBlast> blast;
							obj->QueryInterface(IBlastID,blast);
							if(blast != 0)
							{
								Transform blastTrans;
								Vector center = targetPos;
								SINGLE angle = (((SINGLE)(rand()%1000))/1000.0f)*(2*PI);
								SINGLE range = (((SINGLE)(rand()%1000))/1000.0f)*(pData->areaRadius*GRIDSIZE);

								center += range*Vector(cos(angle),sin(angle),0);
								blastTrans.set_position(center);
								blast->InitBlast(blastTrans,owner.ptr->GetSystemID(),NULL);
								OBJLIST->AddObject(obj);
							}
						}
					}
				}
			}
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void ArtileyLauncher::PhysicalUpdate (SINGLE dt)
{
}
//---------------------------------------------------------------------------
//
void ArtileyLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
}
//---------------------------------------------------------------------------
//
void ArtileyLauncher::InformOfCancel()
{
	bShooting = false;
}
//---------------------------------------------------------------------------
//
void ArtileyLauncher::LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void ArtileyLauncher::LauncherReceiveOpData(U32 workingID, void * buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void ArtileyLauncher::LauncherOpCompleted(U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void ArtileyLauncher::AttackPosition (const struct GRIDVECTOR * position, bool bSpecial)
{
	if(bSpecial == pData->bSpecial)
	{
		targetPos = *position;
		bShooting = true;
		timer = 0;
	}
}
//---------------------------------------------------------------------------
//
void ArtileyLauncher::AttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
const TRANSFORM & ArtileyLauncher::GetTransform (void) const
{
	return owner.ptr->GetTransform();
}
//---------------------------------------------------------------------------
//
bool ArtileyLauncher::checkSupplies()
{
	MPart part = owner.ptr;

	return pData->supplyCost <= part->supplies;
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createArtileryLauncher (PARCHETYPE pArchetype,ArtileryLauncherArchetype * objtype)
{
	ArtileyLauncher * artileryLauncher = new ObjectImpl<ArtileyLauncher>;

	artileryLauncher->init(pArchetype,objtype);

	return artileryLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------ArtileryLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE ArtileryLauncherFactory : public IObjectFactory
{
	struct OBJTYPE : ArtileryLauncherArchetype
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
			if (blastType)
				ARCHLIST->Release(blastType, OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ArtileryLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	ArtileryLauncherFactory (void) { }

	~ArtileryLauncherFactory (void);

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

	/* ArtileryLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
ArtileryLauncherFactory::~ArtileryLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void ArtileryLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE ArtileryLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_ARTILERY_LAUNCHER * data = (BT_ARTILERY_LAUNCHER *) _data;

		if (data->type == LC_ARTILERY_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
			if (data->explosionType[0])
			{
				result->blastType = ARCHLIST->LoadArchetype(data->explosionType);
				CQASSERT(result->blastType);
				ARCHLIST->AddRef(result->blastType, OBJREFNAME);
			}
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 ArtileryLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * ArtileryLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createArtileryLauncher(objtype->pArchetype,objtype);
}
//-------------------------------------------------------------------
//
void ArtileryLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _artileryLauncher : GlobalComponent
{
	ArtileryLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<ArtileryLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _artileryLauncher __artileryLauncher;

//---------------------------------------------------------------------------------------------
//-------------------------------End ArtileryLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------