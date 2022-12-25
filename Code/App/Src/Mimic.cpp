//--------------------------------------------------------------------------//
//                                                                          //
//                                 Mimic.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Mimic.cpp 48    9/13/01 10:01a Tmauer $
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
#include "Sfx.h"
#include "TObjTrans.h"
#include "TobjFrame.h"
#include "Startup.h"
#include "IExplosion.h"
#include "IFighter.h"
#include "ILauncher.h"
#include "Mission.h"
#include "ICloak.h"
#include "ArchHolder.h"
#include "MGlobals.h"
#include <TSmartPointer.h>
#include "TObjTeam.h"
#include "MPart.h"
#include "IUnbornMeshList.h"
#include "FogOfWar.h"
#include "Sector.h"
#include <DSpecial.h>
#include <DMBaseData.h>


#include <Mesh.h>
#include <FileSys.h>
#include <Engine.h>
#include <IRenderPrimitive.h>
#include <Renderer.h>

struct MimicArchetype
{
	const char *name;
	BT_MIMIC_DATA *data;
	S32 archIndex;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	MimicArchetype (void)
	{
		archIndex = INVALID_INSTANCE_INDEX;
	}

	~MimicArchetype (void)
	{
		ENGINE->release_archetype(archIndex);
	}

};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE Mimic : public IBaseObject,ISaveLoad, ILauncher, IMimic, BASE_MIMIC_SAVELOAD
{

	BEGIN_MAP_INBOUND(Mimic)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	_INTERFACE_ENTRY(IMimic)
	END_MAP()

	//------------------------------------------

	//INSTANCE_INDEX instanceIndex;
	BT_MIMIC_DATA *data;
//	bool bStandby:1;		// turn on the cloaking as soon as we can (ie. no longer visible to enemies)
	U8 discoveredFlags;
	U8 pendingDiscoveredFlags;

	OBJPTR<ILaunchOwner> owner;

	SINGLE cloakSupplyCount;
	SINGLE cloakSupplyUse;

	//------------------------------------------

	Mimic (void)
	{
//		instanceIndex = INVALID_INSTANCE_INDEX;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~Mimic (void);	// See ObjList.cpp

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
	
	// ILauncher

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range);

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial);

	virtual void AttackObject (IBaseObject * obj);

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID);

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
		bCloakEnabled = 0;
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const;

	virtual U32 GetSyncData (void * buffer);			// buffer points to use supplied memory

	virtual void PutSyncData (void * buffer, U32 bufferSize);

	virtual void DoSpecialAbility (U32 specialID);

	virtual void DoSpecialAbility (IBaseObject * obj);

	virtual void DoCloak (void)
	{
	}

	virtual void OnAllianceChange (U32 allyMask)
	{
		// do nothing
	}

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};
	
	virtual void SpecialAttackObject (IBaseObject * obj);

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bEnabled)
	{
		ability = USA_MIMIC;
		MPart part(owner.Ptr());

		bEnabled = (checkSupplies() && part->caps.mimicOk);
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

	virtual bool IsToggle() {return false;};

	virtual bool CanToggle(){return false;};

	virtual bool IsOn() {return false;};

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations ();

	//IMimic

	virtual void SetDiscoveredToAllies(U32 allyMask);

	virtual bool IsDiscoveredTo(U32 allyMask);

	virtual void UpdateDiscovered();

	// Mimic methods

//	void setColors (COLORREF color);

//	void getAllChildren (INSTANCE_INDEX instanceIndex,INSTANCE_INDEX *array,S32 &last,S32 arraySize);

	void checkTechLevel ();

//	void enableCloak (bool bCloakEnabled);

	bool checkSupplies ();

	void init (MimicArchetype *arch);
};

//----------------------------------------------------------------------------------
//
Mimic::~Mimic (void)
{
}
//----------------------------------------------------------------------------------
//
void Mimic::PhysicalUpdate (SINGLE dt)
{
	if (bCloakEnabled)
	{
		BOOL32 bLastState = owner.Ptr()->bSpecialRender;
			
		U8 allyMask = MGlobals::GetAllyMask(MGlobals::GetThisPlayer());
		owner.Ptr()->bSpecialRender = TRUE;
		if ((allyMask & (1 << (owner.Ptr()->GetPlayerID()-1)))==0)
		{
			if (allyMask & discoveredFlags)
			{
				owner.Ptr()->bSpecialRender = FALSE;
			}
		}
	
		if (owner.Ptr()->bSpecialRender != bLastState)
		{
			VOLPTR(IExtent) extentObj = owner;
			CQASSERT(extentObj);
			if (owner.Ptr()->bSpecialRender)
			{
			//	MeshChain *mc = UNBORNMANAGER->GetMeshChain(Transform(),aliasArchetypeID);
			//	owner.ptr->ComputeCorners(box, mc->mi[0]->instanceIndex);
				extentObj->CalculateRect(true);
			}
			else
				extentObj->CalculateRect(false);
		}
	}
}

BOOL32 Mimic::Update ()
{
	if (owner.Ptr() == 0)
		return 0;

	checkTechLevel();

//	discoveredFlags = pendingDiscoveredFlags;
	// has enough time gone by to decrease our supplies?

	// are we within range of other ships?
/*	U8 enemyMask = ~(MGlobals::GetAllyMask(owner.ptr->GetPlayerID()));
	U8 visMask   = owner.ptr->GetVisibilityFlags();
	U8 visible = enemyMask & visMask;
*/
	if (bCloakEnabled)
	{
		cloakSupplyCount += cloakSupplyUse;

		if (cloakSupplyCount >= 1)
		{
			owner->UseSupplies(cloakSupplyCount);
			
			// do we need to turn off cloaking?
			if (!checkSupplies())
			{
				bCloakEnabled = false;
				owner.Ptr()->bMimic = false;
				owner.Ptr()->bSpecialRender = false;
			//	enableCloak(bCloakEnabled);

				
				VOLPTR(IExtent) extentObj = owner;
				CQASSERT(extentObj);
				extentObj->SetAliasData(-1,0);
				aliasArchetypeID = -1;
			}
			
			cloakSupplyCount = 0.0f;
		}
		
	/*	if (bCloakEnabled)
		{
			if (visible)
			{
				bStandby = true;
				bCloakEnabled = false;
				enableCloak(bCloakEnabled);
				
				OBJPTR<IExtent> extentObj;
				owner->QueryInterface(IExtentID,extentObj);
				CQASSERT(extentObj);
				extentObj->SetAliasInstanceIndex(INVALID_INSTANCE_INDEX);
			}
			else
			{
				bStandby = false;
			}
		}*/
	}
	/*else if (bStandby && checkSupplies())
	{
		if (visible == 0)
		{
			// turn the cloaking back on
			bCloakEnabled = true;
			enableCloak(bCloakEnabled);
				
			OBJPTR<IExtent> extentObj;
			owner->QueryInterface(IExtentID,extentObj);
			CQASSERT(extentObj);
			extentObj->SetAliasInstanceIndex(instanceIndex);
		}
	}*/

	return 1;
}
//----------------------------------------------------------------------------------
//
void Mimic::Render (void)
{
	if (owner && owner.Ptr()->bSpecialRender && owner.Ptr()->bVisible && bCloakEnabled)
	{
		BATCH->set_state(RPR_BATCH,TRUE);
		/*ENGINE->set_transform(instanceIndex,owner.ptr->GetTransform());
		ENGINE->update_instance(instanceIndex,0,0);
		ENGINE->render_instance(MAINCAM, instanceIndex, 0, LODPERCENT, 0 ,0);*/
		UNBORNMANAGER->RenderMeshAt(owner.Ptr()->GetTransform(),aliasArchetypeID,aliasPlayerID);
	}
}

// ILauncher

void Mimic::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	CQASSERT(_owner);
	//ownerID = _owner->GetPartID();
	_owner->QueryInterface(ILaunchOwnerID,owner,LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);

	cloakSupplyCount = 0.0f;
	cloakSupplyUse = data->supplyUse/REALTIME_FRAMERATE;
	
//	VOLPTR(ICloak) cloaker = _owner;
//	cloaker->SetCloakType(FALSE);
}

void Mimic::AttackPosition (const struct GRIDVECTOR * position, bool bSpecial)
{
//	CQBOMB0("AttackPosition not supported");
}

void Mimic::AttackObject (IBaseObject * obj)
{
	pendingDiscoveredFlags = 0xff;
}
 
void Mimic::AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
{
}

const bool Mimic::TestFightersRetracted (void) const
{
	return true;
}  // return true or you won't jump

// the following methods are for network synchronization of realtime objects
U32 Mimic::GetSyncDataSize (void) const
{
	return 0;
}

U32 Mimic::GetSyncData (void * buffer)			// buffer points to use supplied memory
{
	return NULL;
}
//---------------------------------------------------------------------------
//
void Mimic::PutSyncData (void * buffer, U32 bufferSize)
{}
//---------------------------------------------------------------------------
//
void Mimic::DoSpecialAbility (U32 specialID)
{
//	CQBOMB0("DoSpecialAbility not supported");
/*	if (instanceIndex != INVALID_INSTANCE_INDEX)
	{
		ENGINE->destroy_instance(instanceIndex);
		instanceIndex = INVALID_INSTANCE_INDEX;	
	}

	OBJPTR<IExtent> extentObj;
	owner->QueryInterface(IExtentID,extentObj);
	CQASSERT(extentObj);
	extentObj->SetAliasInstanceIndex(instanceIndex);*/
	DoSpecialAbility((IBaseObject * )NULL);
}
//---------------------------------------------------------------------------
//
void Mimic::DoSpecialAbility (IBaseObject * obj)
{
	aliasArchetypeID = -1;
	aliasPlayerID = owner.Ptr()->GetPlayerID();
	
	// do our magical cloaking stuff
//	bool oldBool = bCloakEnabled;
	bCloakEnabled = (obj != 0);
	
	// check our owners supplies and make sure there is enough to turn on cloaking
	if (bCloakEnabled)
	{
		if (!checkSupplies())
		{
			// make sure we turn off cloaking!
			bCloakEnabled = false;
		}
	}
	
	
	if (obj && bCloakEnabled)
	{
		VOLPTR(IExtent) extentObj2 = obj;
		CQASSERT(extentObj2);
		extentObj2->GetAliasData(aliasArchetypeID,aliasPlayerID);

		if (aliasArchetypeID == -1)
		{
			aliasArchetypeID = ARCHLIST->GetArchetypeDataID(obj->pArchetype);
		}
		CQASSERT(aliasArchetypeID != -1);
		if (aliasPlayerID == 0)
			aliasPlayerID = obj->GetPlayerID();
		//setup mimicked ship
		/*instanceIndex = ENGINE->create_instance2(archType, 0);
		COLORREF color = COLORTABLE[MGlobals::GetColorID(aliasPlayerID)];
		setColors(color);*/
	}

	if (!bCloakEnabled)
		owner.Ptr()->bSpecialRender = false;
	owner.Ptr()->bMimic = bCloakEnabled;
/*	if (bCloakEnabled != oldBool)
	{
		enableCloak(bCloakEnabled);
	}*/
	
	VOLPTR(IExtent) extentObj = owner;
	CQASSERT(extentObj);
	extentObj->SetAliasData(aliasArchetypeID,aliasPlayerID);
}
//---------------------------------------------------------------------------
//
/*
void Mimic::setColors (COLORREF color)
{	
	S32 children[30];
	S32 num_children=0;

	Material *mat[MAX_MATS];

	memset(mat, 0, sizeof(mat));

	getAllChildren(instanceIndex,children,num_children,30);

	for (int c=0;c<num_children;c++)
	{
		Mesh *mesh = REND->get_unique_instance_mesh(children[c]);

		if (mesh)
		{
			Material *tmat = mesh->material_list;
			for (int i = 0; i < mesh->material_cnt; i++, tmat++)
			{
				if (strstr(tmat->name, "markings") != 0)
				{
					if (CQRENDERFLAGS.bSoftwareRenderer)
					{
						tmat->texture_id = 0;
					}
					
					tmat->specular.r = tmat->diffuse.r = GetRValue(color);
					tmat->specular.g = tmat->diffuse.g = GetGValue(color);
					tmat->specular.b = tmat->diffuse.b = GetBValue(color);

					tmat->emission.r = 0.3*tmat->diffuse.r;
					tmat->emission.g = 0.3*tmat->diffuse.g;
					tmat->emission.b = 0.3*tmat->diffuse.b;
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void Mimic::getAllChildren (INSTANCE_INDEX instanceIndex,INSTANCE_INDEX *array,S32 &last,S32 arraySize)
{
	if (last < arraySize)
	{
		array[last] = instanceIndex;
		last++;
		INSTANCE_INDEX lastChild = INVALID_INSTANCE_INDEX,child;
		while ((child = ENGINE->get_instance_child_next(instanceIndex,EN_DONT_RECURSE,lastChild)) != INVALID_INSTANCE_INDEX)
		{
			Vector childPos = ENGINE->get_position(child);
			getAllChildren(child,array,last,arraySize);
			lastChild = child;
		}
	}
}*/
//---------------------------------------------------------------------------
//
void Mimic::SpecialAttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
BOOL32 Mimic::Save (struct IFileSystem * outFile)
{
	DAFILEDESC fdesc = "MIMIC_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	MIMIC_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));
	memcpy(&save, static_cast<BASE_MIMIC_SAVELOAD *>(this), sizeof(BASE_MIMIC_SAVELOAD));

//	FRAME_save(save);
	
	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 Mimic::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "MIMIC_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	MIMIC_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("MIMIC_SAVELOAD", buffer, &load);

	*static_cast<BASE_MIMIC_SAVELOAD *>(this) = load;

//	FRAME_load(load);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void Mimic::ResolveAssociations ()
{
	VOLPTR(IExtent) extentObj;
	
	if (bCloakEnabled && owner)
	{
		extentObj = owner;
		CQASSERT(extentObj);
		extentObj->SetAliasData(aliasArchetypeID,aliasPlayerID);

		owner.Ptr()->bMimic = true;
		owner.Ptr()->bSpecialRender = TRUE;
	}
}

void Mimic::SetDiscoveredToAllies(U32 allyMask)
{
	pendingDiscoveredFlags |= allyMask;
}

bool Mimic::IsDiscoveredTo(U32 allyMask)
{
	bool result = true;

	if (bCloakEnabled)
	{
		if ((allyMask & (1 << (owner.Ptr()->GetPlayerID()-1)))==0)
		{
			if ((allyMask & discoveredFlags)==0)
			{
				result = false;
			}
		}
	}

	return result;
}

void Mimic::UpdateDiscovered()
{
	discoveredFlags = pendingDiscoveredFlags;
	pendingDiscoveredFlags = 0;
}
//---------------------------------------------------------------------------
//
void Mimic::checkTechLevel()
{
	if (owner.Ptr() == 0)
		return;

	// check if we are able to use this special weapon
	TECHNODE techLevel = MGlobals::GetCurrentTechLevel(owner.Ptr()->GetPlayerID());

	// here, we are going to set the caps bit
	MPartNC partNC(owner.Ptr());

	if (techLevel.HasTech(data->techNode))
	{
		partNC->caps.mimicOk = true;
	}
	else
	{
		partNC->caps.mimicOk = false;
	}
}
//---------------------------------------------------------------------------
//
/*void Mimic::enableCloak (bool bCloakEnabled)
{
	VOLPTR(ICloak) cloakShip = owner;
	cloakShip->EnableCloak(bCloakEnabled);
}*/
//---------------------------------------------------------------------------
//
bool Mimic::checkSupplies ()
{
	MDATA mdata;
	owner.Ptr()->GetMissionData(mdata);
	SINGLE maxPts = mdata.pInitData->supplyPointsMax;
	SINGLE curPts = mdata.pSaveData->supplies;

	if (curPts/maxPts <= data->shutoff || (!owner.Ptr()->effectFlags.canShoot()) || (owner.Ptr()->fieldFlags.suppliesLocked()))
	{
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------
//
void Mimic::init (MimicArchetype *arch)
{
	data = arch->data;
}
//----------------------------------------------------------------------------------
//---------------------------------Mimic Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

struct DACOM_NO_VTABLE MimicManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(MimicManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	U32 factoryHandle;


	//child object info
	MimicArchetype *pArchetype;

	//MimicManager methods

	MimicManager (void) 
	{
	}

	~MimicManager();
	
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
// MimicManager methods

MimicManager::~MimicManager()
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
void MimicManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE MimicManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	MimicArchetype *newguy = 0;
	if (objClass == OC_WEAPON)
	{
		BT_MIMIC_DATA *objData = (BT_MIMIC_DATA *)data;
		if (objData->wpnClass == WPN_MIMIC)
		{
			newguy = new MimicArchetype;
			newguy->name = szArchname;
			newguy->data = objData;
			
			goto Done;
		}
	}
//Error:
	delete newguy;
	newguy = 0;

Done:
	return newguy;
}
//--------------------------------------------------------------------------
//
BOOL32 MimicManager::DestroyArchetype(HANDLE hArchetype)
{
	MimicArchetype *deadguy = (MimicArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * MimicManager::CreateInstance(HANDLE hArchetype)
{
	MimicArchetype *pMimic = (MimicArchetype *)hArchetype;
	
	Mimic * obj = new ObjectImpl<Mimic>;
	obj->objClass = OC_EFFECT;
	obj->init(pMimic);

	return obj;
	
}
//--------------------------------------------------------------------------
//
void MimicManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}

struct MimicManager *MimicMgr;
//----------------------------------------------------------------------------------------------
//
struct _mmc : GlobalComponent
{


	virtual void Startup (void)
	{
		MimicMgr = new DAComponent<MimicManager>;
		AddToGlobalCleanupList((IDAComponent **) &MimicMgr);
	}

	virtual void Initialize (void)
	{
		MimicMgr->init();
	}
};

static _mmc mmc;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End Mimic.cpp------------------------------------
//---------------------------------------------------------------------------
