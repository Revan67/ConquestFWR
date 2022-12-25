#ifndef TPLATFORM_H
#define TPLATFORM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                TPlatform.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TPlatform.h 201   9/18/01 4:16p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef CQTRACE_H
#include "CQTrace.h"
#endif

#ifndef TSMARTPOINTER_H
#include <TSmartPointer.h>
#endif

#ifndef ITEXTURELIBRARY_H
#include <ITextureLibrary.h>
#endif

#ifndef IANIM_H
#include <IAnim.h>
#endif

#ifndef ANIM2D_H
#include "Anim2D.h"
#endif

#ifndef IBLINKERS_H
#include "IBlinkers.h"
#endif

#ifndef DQUICKSAVE_H
#include "DQuickSave.h"
#endif

#ifndef IFABRICATOR_H
#include "IFabricator.h"
#endif

#ifndef OBJMAP_H
#include "ObjMap.h"
#endif

#ifndef SYSMAP_H
#include "sysmap.h"
#endif

#ifndef FIELD_H
#include "Field.h"
#endif

#include "TerrainMap.h"
#include "IWeapon.h"
#include "IPlanet.h"
#include "FogofWar.h"
#include "Mission.h"
#include "MPart.h"
#include <DPlatform.h>
#include <DPlatSave.h>
#include "TObjFrame.h"
#include "TObject.h"
#include "TObjSelect.h"
#include "TObjTrans.h"
#include "TObjExtent.h"
#include "TObjMission.h"
#include "TObjTeam.h"
#include "TObjBuild.h"
#include "TObjDamage.h"
#include "TObjExtension.h"
#include "TObjEffectTarget.h"
#include "TManager.h"
#include "CQLight.h"

#include <DEffectOpts.h>

#ifndef __IANIM_H
#include <IAnim.h>
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
typedef PLATFORM_INIT<BASE_PLATFORM_DATA> BASEPLATINIT;
#define HOST_CHECK THEMATRIX->IsMaster()

#define SUPPLIES_PER_FRAME 5

template <class SaveStruct, class InitStruct>
struct _NO_VTABLE Platform : public 	ObjectEffectTarget
											<ObjectTeam
												<ObjectDamage
													<ObjectBuild  //ObjectDamage depends on this
														<ObjectExtension
															<ObjectExtent
																<ObjectSelection
																	<ObjectMission
																		<ObjectTransform
																				<ObjectFrame<IBaseObject,BASE_PLATFORM_SAVELOAD,BASEPLATINIT> >
																		>
																	>
																>
															>
														>
													>
												>
											>, ISaveLoad, IQuickSaveLoad, IPlatform
{
	struct NETVALUES		// used to sync supplies, hullpoints
	{
		U16 hullPoints;
		U16 supplies;
	};
	
	ExplodeNode explodeNode;
	struct InitNode    initNode;
	struct PreTakeoverNode preTakeoverNode;
	struct PreDestructNode	destructNode;
	struct GeneralSyncNode  genSyncNode;

	typedef SaveStruct SAVEINFO;		// override base typedef
	typedef InitStruct INITINFO;		// override base typedef

	// networking fixups
	S32 displaySupplies;		// negative value means "uninitialized"
	S32 displayHull;
	S32 trueNetSupplies;	// last value we sent to clients
	S32 trueNetHull;		// last value we sent to clients
	U32 myKillerOwnerID;	// ID of unit that killed me
	bool bHasHadHullPoints;
	//----------------------------------
	// animation index
	//----------------------------------
	S32 ambientAnimIndex;

	OBJPTR<IExplosion> explosion;

	SINGLE pulseTimer;
	U32 instIndexShadowList;
	U32 shadowChildren;
	IMeshInfoTree * meshShadow;
	MeshInfo * meshShadowMI[MAX_CHILDS];
	S8 shadowUpgrade[MAX_PLAYERS];
	S8 shadowUpgradeWorking[MAX_PLAYERS];
	U8 shadowUpgradeFlags[MAX_PLAYERS];
	SINGLE shadowPercent[MAX_PLAYERS];
	U16 shadowHullPoints[MAX_PLAYERS];
	U16 shadowMaxHull[MAX_PLAYERS];
	OBJPTR<IBuildEffect> shadowBuildEffect;
	OBJPTR<IBuildEffect> shadowBuildEffect2;
	IMeshInfoTree * shadowAddMesh;
	IMeshInfoTree * shadowRemoveMesh;
	U32 shadowAddInstance;
	U32 shadowRemoveInstance;
	U8 shadowPlayer;

	U32 firstNuggetID;
	U32 buildPlanetID;
	U16 buildSlot;		// 10 bit bitfield (0 means no build slot)
	U8 commandPoints;
	U8 shadowVisibilityFlags;

	bool bSetCommandPoints:1;
	bool bHalfSquare:1;
	bool bPlatDead:1;
	bool bPlatRealyDead:1;
	bool bUpdateOnce:1;

	//
	// explosion data
	//
	PARCHETYPE pExplosionType;

	//ObjMap stuff
	int map_sys;
	int map_square;

	//Roots stuff
	IMeshInfoTree *rootChildMeshInfo;
	MeshInfo *rootChild;


	struct FootprintHistory
	{
		FootprintInfo info[2];
		GRIDVECTOR vec[2];
		int numEntries;		// how many entries are valid ?
		U32 systemID;
	
		FootprintHistory (void)
		{
			numEntries = 0;
			systemID = 0;
			vec[0].zero();
			vec[1].zero();
		}

		bool operator == (const FootprintHistory & foot)
		{
			return (memcmp(this, &foot, sizeof(*this)) == 0);
		}

		bool operator != (const FootprintHistory & foot)
		{
			return (memcmp(this, &foot, sizeof(*this)) != 0);
		}

	} footprintHistory;

	Platform (void);
	~Platform (void);


	/* IBaseObject methods */

	virtual void View (void);

	virtual void MapRender (bool bPing);

	virtual void SetTerrainFootprint (struct ITerrainMap * terrainMap);

	virtual void RevealFog (const U32 currentSystem);	// call the FogOfWar manager, if appropriate

	virtual void CastVisibleArea (void);				// set visible flags of objects nearby

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	virtual S32 GetObjectIndex (void) const
	{
		return instanceIndex;
	}

	virtual struct GRIDVECTOR GetGridPosition (void) const
	{
		GRIDVECTOR temp;
		temp = GetPosition();
		return (temp);
	}

	virtual void SetReady(bool _bReady);

	virtual void UpdateVisibilityFlags (void);

	virtual void UpdateVisibilityFlags2 (void);

	virtual SINGLE TestHighlight (const RECT & rect);

	virtual void DrawHighlighted (void);

	virtual void SelfDestruct (bool bExplode);

	virtual bool IsTargetableByPlayer(U32 _playerID) const
	{
		if(bPlatDead&& (_playerID >0) && (_playerID <= MAX_PLAYERS))
		{
			return ((0x01 << (_playerID-1)) & shadowVisibilityFlags) != 0;
		}
		return true;
	}

	virtual U32 GetTrueVisibilityFlags(void) const
	{
		return GetVisibilityFlags() & (~shadowVisibilityFlags);
	}

	/*IPhysicalObject */

	virtual void SetPosition(const Vector & vector, U32 newSystemID);

	virtual void SetTransform (const TRANSFORM & transform, U32 newSystemID);

	void undoFootprintInfo (struct ITerrainMap * terrainMap);

	/* IExplosionOwner methods */

	virtual U32 GetScrapValue (void)
	{
		return (bReady) ? pInitData->scrapValue : 0;	// no value for uncompleted buildings
	}

	U32 GetFirstNuggetID (void)
	{
		return firstNuggetID;
	}

	virtual void RotateShip (SINGLE relYaw, SINGLE relRoll, SINGLE relPitch, SINGLE relAltitude)
	{
		CQBOMB0("huh?");
	}

	virtual void OnChildDeath (INSTANCE_INDEX child)
	{
	}

	/* IWeaponTarget methods */
	
	//returns true if a shield hit was created
	virtual BOOL32 ApplyDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit=1);

	//returns true if a shield hit was created
	virtual BOOL32 ApplyVisualDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit=1);

	// move "amount" from the pending pile to the actual. (assumes complex formula has already been used)
	virtual void ApplyAOEDamage (U32 ownerID, U32 damageAmount);

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations (void);

	/* IQuickSaveLoad methods */

	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void);

	/* IPlatform */

	virtual bool IsDeepSpacePlatform();

	virtual bool IsMoonPlatform();

	virtual bool IsJumpPlatform();

	virtual void ParkYourself (const TRANSFORM & _trans, U32 planetID, U32 slotID)
	{
		CQASSERT(0 && "Should never be called");
	}
	virtual void ParkYourself (IBaseObject * jumpgate)
	{
		CQASSERT(0 && "Not supported for regular platforms");
	}

	virtual bool IsRootSupply()
	{
		return (mObjClass == M_HQ || mObjClass == M_COCOON || mObjClass == M_ACROPOLIS || mObjClass == M_LOCUS);
	}

	virtual bool IsTempSupply()
	{
		return false;
	}

	virtual bool IsReallyDead (void)
	{
		return bPlatDead;
	};

	virtual bool IsHalfSquare (void)
	{
		return bHalfSquare;
	}

	virtual void ReceiveDeathPacket();

	virtual void StopActions() {};

	virtual U32 GetMetalStored();

	virtual U32 GetGasStored();

	virtual U32 GetCrewStored();

	virtual U32 GetMaxMetalStored();

	virtual U32 GetMaxGasStored();

	virtual U32 GetMaxCrewStored();

	virtual MeshInfo *GetRoots()
	{
		return rootChild;
	}

	virtual void AddHarvestRates(SINGLE & gas, SINGLE & metal, SINGLE & crew)
	{
	}
	
	virtual void ClientSideTakeover(U32 newID);

	/* IMissionActor methods */

	virtual void InitActor (void)
	{
		//
		// reserve partID's for the nuggets + 1 ID for explosion
		//
		firstNuggetID = MGlobals::CreateSubordinatePartID();
		for (int i = 0; i < MAX_NUGGETS; i++)
			MGlobals::CreateSubordinatePartID();		// don't need to save the partID, because we know it's sequencial
	}

	virtual void OnMasterChange (bool bIsMaster);

	// methods for accessing the "displayed" hullPoints/supplies
	virtual U32 GetDisplayHullPoints (void) const
	{
		return (displayHull>=0)? displayHull : hullPoints;
	}
	virtual U32 GetDisplaySupplies (void) const
	{
		return (displaySupplies>=0)? displaySupplies : supplies;
	}

	virtual void TakeoverSwitchID (U32 newID);


	// platform routines

	void preSelfDestructPlat (void);

	U32 getSyncPlatData (void * buffer);

	void putSyncPlatData (void * buffer, U32 bufferSize, bool bLateDelivery);

	void explodePlatform (bool bExplode);
	
	void initPlatform (const InitStruct & data);
	
	void updateDisplayValues (void);

	void preTakeover(U32 newMissionID, U32 troopID);

	// Platform helper routines

	virtual void killPlatform();

	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		CQERROR0("getSaveStructName() not be implemented!");
		return "BASE_PLATFORM_SAVELOAD";
	}

	virtual void * getViewStruct (void)				// must be overriden implemented by derived class
	{
		CQERROR0("getViewStruct() not be implemented!");
		return 0;
	}

	void setColorsShadowMesh (U32 playerIndex);

	void detachChildren(U32 playerIndex);

	bool getPlatStats (SINGLE & hull, SINGLE & supplies, U32 & hullMax, U32 & suppliesMax) const;

	bool get_pbs_platform(INSTANCE_INDEX id,float & cx,float & cy,float & radius,float & depth);

	typedef void   (IBaseObject::*InitProc2) (const InitStruct & initStruct);
	typedef void   (IBaseObject::*SaveLoadProc2) (SaveStruct & saveStruct);

	InitProc castInitProc (InitProc2 _proc)
	{
		return reinterpret_cast<InitProc> (_proc);
	}
	
	SaveLoadProc castSaveLoadProc (SaveLoadProc2 _proc)
	{
		return reinterpret_cast<SaveLoadProc> (_proc);
	}

#ifndef CASTINITPROC
#define CASTINITPROC(x) castInitProc(InitProc2(x))
#define CASTSAVELOADPROC(x) castSaveLoadProc(SaveLoadProc2(x))
#endif

	//void FRAME_init (const INITINFO & initStruct)
	void FRAME_init (const InitStruct & _initStruct)
	{
		const BASEPLATINIT * initStruct = (const BASEPLATINIT *) (&_initStruct);
		ObjectFrame<IBaseObject,BASE_PLATFORM_SAVELOAD,BASEPLATINIT>::FRAME_init(*initStruct);

		initExtension(*initStruct);
	}

};


//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
Platform<SaveStruct,InitStruct>::Platform (void) :
				explodeNode(this, ExplodeProc(&Platform::explodePlatform)),
				initNode(this, CASTINITPROC(&Platform::initPlatform)),
				preTakeoverNode(this,PreTakeoverProc(&Platform::preTakeover)),
				destructNode(this, PreDestructProc(&Platform::preSelfDestructPlat)),
				genSyncNode(this, SyncGetProc(&Platform::getSyncPlatData), SyncPutProc(&Platform::putSyncPlatData))
{
	bUpdateOnce = false;
	pulseTimer = 0;
	ambientAnimIndex = -1;
	displaySupplies = displayHull = trueNetSupplies = trueNetHull = -1;
	buildPlanetID = buildSlot = 0;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
Platform<SaveStruct,InitStruct>::~Platform (void)
{
	if(instIndexShadowList != -1)
	{
		ENGINE->destroy_instance(instIndexShadowList);
		DestroyMeshInfoTree(meshShadow);
		instIndexShadowList = -1;
		if(shadowBuildEffect)
		{
			delete shadowBuildEffect.Ptr();
			shadowBuildEffect = NULL;
		}
		if(shadowBuildEffect2)
		{
			delete shadowBuildEffect2.Ptr();
			shadowBuildEffect2 = NULL;
		}
		if(shadowAddInstance != -1)
		{
			ENGINE->destroy_instance(shadowAddInstance);
			DestroyMeshInfoTree(shadowAddMesh);
			shadowAddInstance = -1;
		}
		if(shadowRemoveInstance != -1)
		{
			ENGINE->destroy_instance(shadowRemoveInstance);
			DestroyMeshInfoTree(shadowRemoveMesh);
			shadowRemoveInstance = -1;
		}
		shadowPlayer = 0;
	}
	else
	{
		CQASSERT(shadowBuildEffect == 0 && shadowBuildEffect2 == 0);
	}

	if(explosion)
	{
		delete explosion.Ptr();
		explosion = NULL;
	}
	//this is where we update the techtree of our loss....
	if (GetPlayerID())
	{
		IBaseObject * obj = OBJLIST->GetObjectList();
		bool setNoPlatform = true;
		while(obj)
		{
			if((obj->objClass == OC_PLATFORM) && (MPart(obj)->mObjClass == mObjClass) && (obj != ((IBaseObject *)this)) &&
				(obj->GetPlayerID() == GetPlayerID()))
			{
				setNoPlatform = false;
				obj = NULL;
			}
			else
				obj = obj->next;
		}
		if(setNoPlatform)
		{
			TECHNODE oldTech = MGlobals::GetCurrentTechLevel(GetPlayerID());
			SINGLE_TECHNODE myNode = ((BASE_PLATFORM_DATA *)ARCHLIST->GetArchetypeData(pArchetype))->techActive;
			oldTech.RemoveFromNode(myNode);
			MGlobals::SetCurrentTechLevel(oldTech,GetPlayerID());
		}
	}

	ANIM->release_script_inst(ambientAnimIndex);
	ambientAnimIndex = -1;

	if (buildPlanetID && buildSlot)
	{
		IBaseObject * obj = OBJLIST->FindObject(buildPlanetID);
		OBJPTR<IPlanet> planet;

		if (obj && obj->QueryInterface(IPlanetID, planet))
			planet->DeallocateBuildSlot(dwMissionID, buildSlot);
		buildPlanetID = 0;
	}

	COMPTR<ITerrainMap> terrainMap;
	SECTOR->GetTerrainMap(systemID, terrainMap);
	undoFootprintInfo(terrainMap);

	//root stuff
	if (rootChildMeshInfo)
	{
		ENGINE->destroy_instance(rootChild[0].instanceIndex);
		DestroyMeshInfoTree(rootChildMeshInfo);
	}
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
BOOL32 Platform<SaveStruct,InitStruct>::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = getSaveStructName();
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	SaveStruct save;
	U32 i;

#ifdef _DEBUG
	if (sizeof(save) != sizeof(BASE_PLATFORM_SAVELOAD) && strcmp(fdesc.lpFileName, "BASE_PLATFORM_SAVELOAD")==0)
	{
		CQERROR0("Possible load/save problem. getSaveStructName() not implemented in inherited class.");
	}
#endif

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));

	save.firstNuggetID = firstNuggetID;
	save.buildSlot = buildSlot;		// 10 bit bitfield (0 means no build slot)
	save.buildPlanetID = buildPlanetID;
	save.bSetCommandPoints = bSetCommandPoints;
	save.shadowVisibilityFlags = shadowVisibilityFlags;
	save.bPlatDead = bPlatDead;
	save.bPlatRealyDead = bPlatRealyDead;
	for(i = 0; i <MAX_PLAYERS;++i)
	{
		save.shadowUpgrade[i] =shadowUpgrade[i];
		save.shadowUpgradeWorking[i] =shadowUpgradeWorking[i];
		save.shadowUpgradeFlags[i] =shadowUpgradeFlags[i];
		save.shadowPercent[i] = shadowPercent[i];
		save.shadowHullPoints[i] =shadowHullPoints[i];
		save.shadowMaxHull[i] =shadowMaxHull[i];
	}
	save.exploredFlags = GetVisibilityFlags();
	
	FRAME_save(save);
	if (save.mission.hullPoints == 0)
		save.mission.hullPoints = 1;		// prevent pending dead objects from coming up a zombie
	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
BOOL32 Platform<SaveStruct,InitStruct>::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = getSaveStructName();
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	SaveStruct load;
	U8 buffer[1024];
	U32 i;

#ifdef _DEBUG
	if (sizeof(load) != sizeof(BASE_PLATFORM_SAVELOAD) && strcmp(fdesc.lpFileName, "BASE_PLATFORM_SAVELOAD")==0)
	{
		CQERROR0("Possible load/save problem. getSaveStructName() not implemented in inherited class.");
	}
#endif

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol(fdesc.lpFileName, buffer, &load);

	firstNuggetID = load.firstNuggetID;
	buildSlot = load.buildSlot;		// 10 bit bitfield (0 means no build slot)
	buildPlanetID = load.buildPlanetID;
	bSetCommandPoints = load.bSetCommandPoints;
	shadowVisibilityFlags = load.shadowVisibilityFlags;
	bPlatDead = load.bPlatDead;
	bPlatRealyDead = load.bPlatRealyDead;
	for(i = 0; i <MAX_PLAYERS;++i)
	{
		shadowUpgrade[i] = load.shadowUpgrade[i];
		shadowUpgradeWorking[i] = load.shadowUpgradeWorking[i];
		shadowUpgradeFlags[i] = load.shadowUpgradeFlags[i];
		shadowPercent[i] = load.shadowPercent[i];
		shadowHullPoints[i] = load.shadowHullPoints[i];
		shadowMaxHull[i] = load.shadowMaxHull[i];
	}
	SetJustVisibilityFlags(load.exploredFlags);
	UpdateVisibilityFlags();

	FRAME_load(load);
	SetPosition(transform.translation, systemID);		// ensure terrain footprint is set

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::ResolveAssociations (void)
{
	FRAME_resolve();
}
//--------------------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::QuickSave (struct IFileSystem * file)
{
	DAFILEDESC fdesc = partName;
	HANDLE hFile;

	file->CreateDirectory("MT_PLATFORM_QLOAD");
	if (file->SetCurrentDirectory("MT_PLATFORM_QLOAD") == 0)
		CQERROR0("QuickSave failed");

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	if ((hFile = file->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR0("QuickSave failed");
	}
	else
	{
		MT_PLATFORM_QLOAD qload;
		DWORD dwWritten;
		
		qload.dwMissionID = dwMissionID;
		qload.position.init(GetGridPosition(),systemID);

		file->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		file->CloseHandle(hFile);
	}
}
//--------------------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize)
{
	MT_PLATFORM_QLOAD qload;
	MISSION->CorrelateSymbol(szSaveLoadType, buffer, &qload);

	MGlobals::InitMissionData(this, MGlobals::CreateNewPartID(MGlobals::GetPlayerFromPartID(qload.dwMissionID)));
	partName = szInstanceName;
	hullPoints = pInitData->hullPointsMax;
	if ((mObjClass != M_HARVEST) 
				&&	(mObjClass != M_GALIOT) &&(mObjClass != M_SIPHON))
		supplies   = supplyPointsMax;
	SetPosition(qload.position, qload.position.systemID);

	OBJLIST->AddPartID(this, dwMissionID);
	if(GetPlayerID())
		SECTOR->RevealSystem(systemID, GetPlayerID());
	bUnderCommand = true;
}
//--------------------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::QuickResolveAssociations (void)
{
	//
	// find nearest open slot
	//

	if(IsJumpPlatform())
	{
		IBaseObject * bestJumpgate = NULL;
		IBaseObject * obj = OBJLIST->GetTargetList();     // nav hazard list
		SINGLE bestDistance=10e8;
		while(obj)
		{
			if(obj->GetSystemID() == systemID && obj->objClass == OC_JUMPGATE)
			{
				SINGLE newDist = GetGridPosition()-obj->GetGridPosition();
				if(newDist < bestDistance)
				{
					bestJumpgate = obj;
					bestDistance = newDist;
				}
			}
			obj = obj->nextTarget;
		}
		if(bestJumpgate)
		{
			ParkYourself(bestJumpgate);
		}
		else
		{
			CQBOMB1("Could not park platform %s around a jumpgate.",partName.string);
		}
		
	}
	else if(!IsDeepSpacePlatform())
	{
		IBaseObject * obj = OBJLIST->GetTargetList();     // nav hazard list
		S32 bestSlot=-1;
		SINGLE bestDistance=10e8;
		OBJPTR<IPlanet> planet, bestPlanet;
		Vector position = GetPosition();

		while (obj)
		{
			if (obj->GetSystemID() == systemID && obj->QueryInterface(IPlanetID, planet)!=0)
			{
				U32 newSlot = planet->FindBestSlot(pArchetype,&position);
				if(newSlot)
				{
					SINGLE newDist = (position-planet->GetSlotTransform(newSlot).translation).magnitude();
					if(newDist < bestDistance)
					{
						bestDistance = newDist;
						bestSlot = newSlot;
						bestPlanet = planet;
					}
				}

			} // end obj->GetSystemID() ...
			
			obj = obj->nextTarget;
		} // end while()

		if (bestPlanet && bestDistance < 10000)
		{
			CQASSERT(bestSlot);
			bestSlot = bestPlanet->AllocateBuildSlot(dwMissionID, bestSlot);
			TRANSFORM trans = bestPlanet->GetSlotTransform(bestSlot);
			position = trans.translation;

			ParkYourself(trans, bestPlanet.Ptr()->GetPartID(), bestSlot);
		}
		else
		{
			CQBOMB1("Could not park platform %s around a planet.",partName.string);
		}
		
		SetPosition(position, systemID);
		ENGINE->update_instance(GetObjectIndex(),0,0);
	}
	SetReady(true);
}
//--------------------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
bool Platform<SaveStruct,InitStruct>::IsDeepSpacePlatform ()
{
	if(((BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype)))->slotsNeeded)
		return false;
	return true;
}
//--------------------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
bool Platform<SaveStruct,InitStruct>::IsMoonPlatform ()
{
	return ((BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype)))->bMoonPlatform;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
bool Platform<SaveStruct,InitStruct>::IsJumpPlatform()
{
	return false;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::ReceiveDeathPacket()
{
	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(systemID, map);
	undoFootprintInfo(map);
	OBJLIST->DeferredDestruction(dwMissionID);
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
U32 Platform<SaveStruct,InitStruct>::GetMetalStored()
{
	const BASE_PLATFORM_DATA * objData = (const BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
	if(objData->metalStorage)
	{
		U32 max = MGlobals::GetMaxMetal(playerID)-BASE_MAX_METAL;
		if(max)
		{
			U32 metalStore = MGlobals::GetCurrentMetal(playerID);
			if(metalStore > BASE_MAX_METAL)
				metalStore -= BASE_MAX_METAL;
			else
				metalStore = 0;
			return (objData->metalStorage*metalStore)/max;
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
U32 Platform<SaveStruct,InitStruct>::GetGasStored()
{
	const BASE_PLATFORM_DATA * objData = (const BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
	if(objData->gasStorage)
	{
		U32 max = MGlobals::GetMaxGas(playerID)-BASE_MAX_GAS;
		if(max)
		{
			U32 gasStore = MGlobals::GetCurrentGas(playerID);
			if(gasStore > BASE_MAX_GAS)
				gasStore -= BASE_MAX_GAS;
			else
				gasStore = 0;
			return (objData->gasStorage*gasStore)/max;
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
U32 Platform<SaveStruct,InitStruct>::GetCrewStored()
{
	const BASE_PLATFORM_DATA * objData = (const BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
	if(objData->crewStorage)
	{
		U32 max = MGlobals::GetMaxCrew(playerID)-BASE_MAX_CREW;
		if(max)
		{
			U32 crewStore = MGlobals::GetCurrentCrew(playerID);
			if(crewStore > BASE_MAX_CREW)
				crewStore -= BASE_MAX_CREW;
			else
				crewStore = 0;
			return (objData->crewStorage*crewStore)/max;
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::OnMasterChange (bool bIsMaster)
{
	if(bIsMaster)
	{
		if(bPlatDead)
		{
			UnregisterWatchersForObjectForPlayerMask(this,((~shadowVisibilityFlags) << 1)|(0x01 << SYSVOLATILEPTR));
		}
	}
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
U32 Platform<SaveStruct,InitStruct>::GetMaxMetalStored()
{
	const BASE_PLATFORM_DATA * objData = (const BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
	return objData->metalStorage;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
U32 Platform<SaveStruct,InitStruct>::GetMaxGasStored()
{
	const BASE_PLATFORM_DATA * objData = (const BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
	return objData->gasStorage;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
U32 Platform<SaveStruct,InitStruct>::GetMaxCrewStored()
{
	const BASE_PLATFORM_DATA * objData = (const BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
	return objData->crewStorage;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::ClientSideTakeover(U32 newMissionID)
{
	if(newMissionID != dwMissionID)
	{
		if (buildPlanetID && buildSlot)
		{
			IBaseObject * obj = OBJLIST->FindObject(buildPlanetID);
			OBJPTR<IPlanet> planet;

			if (obj && obj->QueryInterface(IPlanetID, planet))
			{
				planet->DeallocateBuildSlot(dwMissionID, buildSlot);
				planet->AllocateBuildSlot(newMissionID,buildSlot);
			}
		}
		const U32 newPlayerID = MGlobals::GetPlayerFromPartID(newMissionID); 
		const BASE_PLATFORM_DATA * objData = (const BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
		BANKER->TransferMaxResources(objData->metalStorage,objData->gasStorage,objData->crewStorage,playerID,newPlayerID);

		if (GetPlayerID())
		{
			IBaseObject * obj = OBJLIST->GetObjectList();
			bool setNoPlatform = true;
			while(obj)
			{
				if((obj->objClass == OC_PLATFORM) && (MPart(obj)->mObjClass == mObjClass) && (obj != ((IBaseObject *)this)) &&
					(obj->GetPlayerID() == GetPlayerID()))
				{
					setNoPlatform = false;
					obj = NULL;
				}
				else
					obj = obj->next;
			}
			if(setNoPlatform)
			{
				TECHNODE oldTech = MGlobals::GetCurrentTechLevel(GetPlayerID());
				SINGLE_TECHNODE myNode = ((BASE_PLATFORM_DATA *)ARCHLIST->GetArchetypeData(pArchetype))->techActive;
				oldTech.RemoveFromNode(myNode);
				MGlobals::SetCurrentTechLevel(oldTech,GetPlayerID());
			}
		}
	}
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::View (void)
{
	PLATFORM_VIEW view;
	BASIC_INSTANCE data;

	view.rtData = &data;
	view.mission = this;
	view.platform.nothing = getViewStruct();

	Vector vec;
	Matrix matrix = get_orientation(instanceIndex);
	memset(&data, 0, sizeof(data));
	
	vec = get_position(instanceIndex);
	memcpy(&data.position, &vec, sizeof(data.position));

	vec = ENGINE->get_angular_velocity(instanceIndex);
	vec = vec * matrix;		// transpose multiply (convert to object coordinates)

	memcpy(&data.rotation, &vec, sizeof(data.rotation));

	if (DEFAULTS->GetUserData("PLATFORM_VIEW", view.mission->partName, &view, sizeof(view)))
	{
		memcpy(&vec, &data.position, sizeof(data.position));
		set_position(instanceIndex, vec);

		memcpy(&vec, &data.rotation, sizeof(data.rotation));
		vec = matrix * vec;		// multiply (convert back to world coordinates)

		ENGINE->set_angular_velocity(instanceIndex, vec);

		if (dwMissionID != ((dwMissionID & ~0xF) | playerID))	// playerID has changed
		{
			OBJLIST->RemovePartID(this, dwMissionID);
			dwMissionID = (dwMissionID & ~0xF) | playerID;
			OBJLIST->AddPartID(this, dwMissionID);
		}
	}
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::MapRender (bool bPing)
{
	if ((IsVisibleToPlayer(MGlobals::GetThisPlayer()) && 
		(!(bPlatDead && ((~shadowVisibilityFlags) &(0x01 << (MGlobals::GetThisPlayer()-1)))))) || 
		DEFAULTS->GetDefaults()->bEditorMode || (bPing && !bPlatDead) || DEFAULTS->GetDefaults()->bSpectatorModeOn)
	{
		COLORREF color;
		if(bSelected)
			color = RGB(255,255,255);
		else
			color = COLORTABLE[MGlobals::GetColorID(playerID)];

		if(IsDeepSpacePlatform())
		{
			SYSMAP->DrawCircle(transform.translation,GRIDSIZE/2,color);
		}
		else if(IsJumpPlatform())
		{
			SYSMAP->DrawRing(transform.translation,GRIDSIZE*2,1,color);
		}
		else if(IsMoonPlatform())
		{
			SYSMAP->DrawArc(transform.translation,GRIDSIZE*2,0,PI*2,5,color);
		}
		else if(buildSlot)
		{
			U32 firstSlotID = 0;
			while(((0x01 << firstSlotID) & buildSlot))
				++firstSlotID;
			while(!((0x01 << firstSlotID) & buildSlot))
				firstSlotID = (firstSlotID+1)%12;
			SINGLE angle2 = (PI/6.0f)*firstSlotID;
			SINGLE angle1 = angle2+(PI/6.0f)*((BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype)))->slotsNeeded;
			SYSMAP->DrawArc(transform.translation,GRIDSIZE*2,angle1,angle2,5,color);
		}

	}

}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::RevealFog (const U32 currentSystem)
{
	if (systemID==currentSystem && bReady && playerID && MGlobals::AreAllies(playerID, MGlobals::GetThisPlayer()))
	{
		SINGLE bonus = fieldFlags.getSensorDampingMod()*effectFlags.getSensorDampingMod()*SECTOR->GetSectorEffects(playerID,systemID)->getSensorMod();
		FOGOFWAR->RevealZone(this, __max(0.75,sensorRadius*bonus), cloakedSensorRadius*bonus);
	}
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::CastVisibleArea (void)
{
	const U32 mask = MGlobals::GetAllyMask(playerID);
	SetVisibleToAllies(mask);

	if(bPlatDead)
	{
		if(shadowVisibilityFlags & (0x01 << (MGlobals::GetThisPlayer() -1)))
			SECTOR->AddPlatformToSystem(systemID, mask, playerID, mObjClass);
	}
	else if (IsVisibleToPlayer(MGlobals::GetThisPlayer()))
		SECTOR->AddPlatformToSystem(systemID, mask, playerID, mObjClass);

	if (playerID && bReady && (systemID & HYPER_SYSTEM_MASK)==0)
	{
		SINGLE bonus = fieldFlags.getSensorDampingMod()*effectFlags.getSensorDampingMod()*SECTOR->GetSectorEffects(playerID,systemID)->getSensorMod();
		OBJLIST->CastVisibleArea(playerID, systemID, transform.translation, fieldFlags, __max(0.75,sensorRadius*bonus), cloakedSensorRadius*bonus);
	}
}
#define MAXDAMAGECHANGE(x)  F2LONG(1+ (x / (5 * REALTIME_FRAMERATE)))
#define MAXSUPPLYCHANGE(x)  F2LONG(1+ (x / (1 * REALTIME_FRAMERATE)))
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::updateDisplayValues (void)
{
	//
	// update the hullpoints/supplies toward the true values.
	//
	if (displaySupplies < 0)
		displaySupplies = supplies;
	else
	{
		S32 diff = supplies - displaySupplies;
		S32 newDiff;

		if (diff < 0)
		{
			const S32 maxChange = -MAXSUPPLYCHANGE(supplyPointsMax);
			newDiff = __max(maxChange, diff);
		}
		else
		{
			const S32 maxChange = MAXSUPPLYCHANGE(supplyPointsMax);
			newDiff = __min(maxChange, diff);
		}

		displaySupplies += newDiff;
	}

	bool bDead = false;
	if (hullPoints)
		bHasHadHullPoints=true;

	if (displayHull < 0)
	{
		if ((displayHull = hullPoints) == 0 && bHasHadHullPoints)
			bDead = true;
	}
	else
	{
		S32 diff = hullPoints - displayHull;
		S32 newDiff;

		if (diff < 0)
		{
			const S32 maxChange = -MAXDAMAGECHANGE(hullPointsMax);
			newDiff = __max(maxChange, diff);
		}
		else
		{
			const S32 maxChange = MAXDAMAGECHANGE(hullPointsMax);
			newDiff = __min(maxChange, diff);
		}

		displayHull += newDiff;
		if (displayHull == 0 && newDiff!=0 && bHasHadHullPoints)		// only do this once!
			bDead = true;
	}

 	if (bDead && bReverseBuild==0)
	{
		if (THEMATRIX->IsMaster())
			THEMATRIX->ObjectTerminated(dwMissionID, myKillerOwnerID);
	}
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::preTakeover(U32 newMissionID, U32 troopID)
{
	if(newMissionID != dwMissionID)
	{
		if (buildPlanetID && buildSlot)
		{
			IBaseObject * obj = OBJLIST->FindObject(buildPlanetID);
			OBJPTR<IPlanet> planet;

			if (obj && obj->QueryInterface(IPlanetID, planet))
			{
				planet->DeallocateBuildSlot(dwMissionID, buildSlot);
				planet->AllocateBuildSlot(newMissionID,buildSlot);
			}
		}
		const U32 newPlayerID = MGlobals::GetPlayerFromPartID(newMissionID); 
		if(THEMATRIX->IsMaster())
		{
			if(bSetCommandPoints)
			{
				BANKER->LoseCommandPt(playerID,commandPoints);
				BANKER->AddCommandPt(newPlayerID, commandPoints);
			}
			BANKER->UseCommandPt(newPlayerID, pInitData->resourceCost.commandPt);
			BANKER->FreeCommandPt(playerID,pInitData->resourceCost.commandPt);
		}
		const BASE_PLATFORM_DATA * objData = (const BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
		BANKER->TransferMaxResources(objData->metalStorage,objData->gasStorage,objData->crewStorage,playerID,newPlayerID);
	}
	else
	{
/*		const BASE_PLATFORM_DATA * objData = (const BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
		if(THEMATRIX->IsMaster() && bSetCommandPoints)
		{
			bSetCommandPoints = false;
			BANKER->LoseCommandPt(playerID,commandPoints);
		}
		BANKER->RemoveMaxResources(objData->metalStorage,objData->gasStorage,objData->crewStorage,playerID);
*/	}
	
	// Yo Thomas...I took out this SetReady because it was messin with my boy, da Troopship
	// damn, time to give props, the H-Dog gots to represent
//	SetReady(false);
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
U32 Platform<SaveStruct,InitStruct>::getSyncPlatData (void * buffer)
{
	U32 result=0;

	if (bPlatDead==false)
	{
		NETVALUES * const data = (NETVALUES *) buffer;

		if (supplies != trueNetSupplies || hullPoints != trueNetHull)  // have values changed?
		{
			data->supplies = supplies;
			data->hullPoints = hullPoints;
			trueNetSupplies = supplies;
			trueNetHull = hullPoints;
			result = sizeof(*data);
		}
	}

	return result;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::putSyncPlatData (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	const NETVALUES * const data = (NETVALUES *) buffer;
	CQASSERT(bufferSize == sizeof(*data));

	supplies = data->supplies;
	CQASSERT((!THEMATRIX->IsMaster()) || data->hullPoints <= hullPointsMax);
	CQASSERT(data->hullPoints <= hullPointsMax*2);
	hullPoints = data->hullPoints;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::preSelfDestructPlat (void)
{
	if (buildPlanetID && buildSlot)
	{
		IBaseObject * obj = OBJLIST->FindObject(buildPlanetID);
		OBJPTR<IPlanet> planet;

		if (obj && obj->QueryInterface(IPlanetID, planet))
			planet->DeallocateBuildSlot(dwMissionID, buildSlot);
		buildPlanetID = 0;
	}
	SetReady(false);
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
BOOL32 Platform<SaveStruct,InitStruct>::Update (void)
{
	if(bPlatRealyDead)
		return 1;

	if(!bUpdateOnce)
	{
		bUpdateOnce = true;
		InitStruct * pArchData = (InitStruct *)(ARCHLIST->GetArchetypeHandle(pArchetype));
		if(pArchData->ambientEffect)
		{
			IEffectInstance * effInst = pArchData->ambientEffect->CreateInstance();
			effInst->SetTarget(this,0,0);
			effInst->SetSystemID(this);
			effInst->TriggerStartEvent();
		}
	}

	if(explosion)
	{
		if(!explosion.Ptr()->Update())
		{
			delete explosion.Ptr();
			explosion = 0;
		}
	}
	if (map_sys==0)
	{
		map_square = OBJMAP->GetMapSquare(systemID,transform.translation);
		//OBJMAP->RemoveObjectFromMap(this,map_sys,map_square);
		map_sys = systemID;
		U32 flags = OM_TARGETABLE;
		if(bPlatDead)
			flags |= (OM_SHADOW| (shadowVisibilityFlags << 24));
		objMapNode = OBJMAP->AddObjectToMap(this,map_sys,map_square,flags);
	}

	updateDisplayValues();

	FRAME_update();
	
	//this is where we update the techTree
	if(!bExploding && bReady && playerID)
	{
		if(!bSetCommandPoints)
		{
			bSetCommandPoints = true;
			if(THEMATRIX->IsMaster())
			{
				BANKER->AddCommandPt(playerID,commandPoints);
			}
			const BASE_PLATFORM_DATA * objData = (const BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
			BANKER->AddMaxResources(objData->metalStorage,objData->gasStorage,objData->crewStorage,playerID);
		}

		TECHNODE currentNode = MGlobals::GetCurrentTechLevel(MGlobals::GetPlayerFromPartID(dwMissionID));
		SINGLE_TECHNODE myNode = ((BASE_PLATFORM_DATA *)ARCHLIST->GetArchetypeData(pArchetype))->techActive;
		if(!(currentNode.HasTech(myNode)))
		{
			currentNode.AddToNode(myNode);
			MGlobals::SetCurrentTechLevel(currentNode,MGlobals::GetPlayerFromPartID(dwMissionID));
		}
	}

	if(THEMATRIX->IsMaster() && !explosion && bPlatDead && !shadowVisibilityFlags)
	{
		killPlatform();
	}

	return 1;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::killPlatform()
{
	THEMATRIX->SendPlatformDeath(dwMissionID);
	bPlatRealyDead = true;
	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(systemID, map);
	undoFootprintInfo(map);
	OBJLIST->DeferredDestruction(dwMissionID);
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::PhysicalUpdate (SINGLE dt)
{
	if(explosion)
		explosion.Ptr()->PhysicalUpdate(dt);

	if (bVisible && !building)
	{
		ENGINE->update_instance(instanceIndex, 0, dt);
		ANIM->update_instance(instanceIndex,dt);
	}
	FRAME_physicalUpdate(dt);
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::setColorsShadowMesh (U32 playerIndex)
{	
	S32 children[30];
	S32 num_children=0;

	Material *mat[MAX_MATS];
//	int mat_cnt=0;

	memset(mat, 0, sizeof(mat));

	GetAllChildren(instIndexShadowList,children,num_children,30);
	COLORREF color = COLORTABLE[MGlobals::GetColorID(playerID)];

	for (int c=0;c<num_children;c++)
	{	
		//Mesh *mesh = REND->get_unique_instance_mesh(children[c]);
		Mesh *mesh = REND->get_instance_mesh(children[c]);

		if (mesh)
		{
			for (int fg_cnt=0;fg_cnt<mesh->face_group_cnt;fg_cnt++)
			{
				Material *tmat = &mesh->material_list[mesh->face_groups[fg_cnt].material];
							
				if (strstr(tmat->name, "markings") != 0)
				{
					color = COLORTABLE[MGlobals::GetColorID(playerID)];

					tmat->diffuse.r = GetRValue(color);
					tmat->diffuse.g = GetGValue(color);
					tmat->diffuse.b = GetBValue(color);

					tmat->emission.r = 0.3*tmat->diffuse.r;
					tmat->emission.g = 0.3*tmat->diffuse.g;
					tmat->emission.b = 0.3*tmat->diffuse.b;
				}

				FaceGroupInfo *fgi = &meshShadowMI[c]->fgi[fg_cnt];
				
				fgi->a = 128;

				fgi->diffuse.r = tmat->diffuse.r;
				fgi->diffuse.g = tmat->diffuse.g;
				fgi->diffuse.b = tmat->diffuse.b;

				fgi->emissive.r = tmat->emission.r;
				fgi->emissive.g = tmat->emission.g;
				fgi->emissive.b = tmat->emission.b;
			}
		}
	}
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::detachChildren(U32 playerIndex)
{
	InitStruct * initSt = ((InitStruct *) (ARCHLIST->GetArchetypeHandle(pArchetype)));

	ExtensionInfo shadowExt[MAX_EXTENSIONS];

	//find the child objects used for extensions
	U32 exCount;
	for(exCount = 0; exCount < MAX_EXTENSIONS; ++exCount)
	{
		shadowExt[exCount].bAttached = false;
		if(initSt->pData->extension[exCount].extensionName[0])
		{
			BT_EXTENSION_INFO * exData = (BT_EXTENSION_INFO *) ARCHLIST->GetArchetypeData(initSt->pData->extension[exCount].extensionName);

			if(exData->addChildName[0])
			{	
				bool found = false;
				for(U32 i = 0; i < exCount && (!found); ++i)
				{
					if(shadowExt[i].addIndex != INVALID_INSTANCE_INDEX)
					{
						if(strcmp(exData->addChildName,ENGINE->get_instance_part_name(shadowExt[i].addIndex)) == 0)
						{
							found = true;
							shadowExt[exCount].bDuplicateAdd = true;
							shadowExt[i].bDuplicateAdd = true;
							shadowExt[exCount].addIndex = shadowExt[i].addIndex;
							memcpy (&(shadowExt[exCount].addJointInfo),&(shadowExt[i].addJointInfo),sizeof(JointInfo));
							shadowExt[exCount].add_mesh_info = shadowExt[i].add_mesh_info;
						}
					}
				}
				if(!found)
				{
					if((shadowExt[exCount].addIndex = ENGINE->get_instance_child_next(instIndexShadowList,0,INVALID_INSTANCE_INDEX)) != INVALID_ARCHETYPE_INDEX)
					{
						while(strcmp(exData->addChildName,ENGINE->get_instance_part_name(shadowExt[exCount].addIndex))!=0)
						{
							shadowExt[exCount].addIndex = ENGINE->get_instance_child_next(instIndexShadowList,0,shadowExt[exCount].addIndex);
							if(shadowExt[exCount].addIndex == INVALID_ARCHETYPE_INDEX)
								break;
						}
					}
					if(shadowExt[exCount].addIndex != INVALID_INSTANCE_INDEX)
					{
						shadowExt[exCount].bDuplicateAdd = false;
						const JointInfo *temp = ENGINE->get_joint_info(shadowExt[exCount].addIndex);
						memcpy (&(shadowExt[exCount].addJointInfo),temp,sizeof(JointInfo));
						ENGINE->destroy_joint(instIndexShadowList,shadowExt[exCount].addIndex);
						meshShadow->DetachChild(shadowExt[exCount].addIndex,&shadowExt[exCount].add_mesh_info);
					}
				}
			}
			else
			{
				shadowExt[exCount].addIndex = INVALID_ARCHETYPE_INDEX;
			}
			if(exData->removeChildName[0])
			{	
				bool found = false;
				for(U32 i = 0; i < exCount && (!found); ++i)
				{
					if(shadowExt[i].removeIndex != INVALID_INSTANCE_INDEX)
					{
						if(strcmp(exData->removeChildName,ENGINE->get_instance_part_name(shadowExt[i].removeIndex)) == 0)
						{
							shadowExt[exCount].bDuplicateRemove = true;
							shadowExt[i].bDuplicateRemove = true;
							found = true;
							shadowExt[exCount].removeIndex = shadowExt[i].removeIndex;
							memcpy (&(shadowExt[exCount].removeJointInfo),&(shadowExt[i].removeJointInfo),sizeof(JointInfo));
							shadowExt[exCount].remove_mesh_info = shadowExt[i].remove_mesh_info;
						}
					}
				}
				if(!found)
				{
					if((shadowExt[exCount].removeIndex = ENGINE->get_instance_child_next(instIndexShadowList,0,INVALID_INSTANCE_INDEX)) != INVALID_ARCHETYPE_INDEX)
					{
						while(strcmp(exData->removeChildName,ENGINE->get_instance_part_name(shadowExt[exCount].removeIndex))!=0)
						{
							shadowExt[exCount].removeIndex = ENGINE->get_instance_child_next(instIndexShadowList,0,shadowExt[exCount].removeIndex);
							if(shadowExt[exCount].removeIndex == INVALID_ARCHETYPE_INDEX)
								break;
						}
					}
					if(shadowExt[exCount].removeIndex != INVALID_INSTANCE_INDEX)
					{
						shadowExt[exCount].bDuplicateRemove = false;
						const JointInfo *temp = ENGINE->get_joint_info(shadowExt[exCount].removeIndex);
						memcpy (&(shadowExt[exCount].removeJointInfo),temp,sizeof(JointInfo));
					}
				}
			}
			else
			{
				shadowExt[exCount].removeIndex = INVALID_ARCHETYPE_INDEX;
			}
		}
		else
		{
			shadowExt[exCount].addIndex = INVALID_ARCHETYPE_INDEX;
			shadowExt[exCount].removeIndex = INVALID_ARCHETYPE_INDEX;
		}
	}

	shadowChildren = meshShadow->ListChildren(meshShadowMI);
	for(U32 i = 0 ; i < MAX_EXTENSIONS; ++i)
	{
		if(shadowUpgradeFlags[playerIndex] & (0x01 << i))
		{
			if(!(shadowExt[i].bAttached))
			{
				shadowExt[i].bAttached = true;
				if(shadowExt[i].addIndex != INVALID_INSTANCE_INDEX)
				{	
					if(shadowExt[i].addEffect)
						delete shadowExt[i].addEffect;
					ENGINE->create_joint(instIndexShadowList,shadowExt[i].addIndex,&(shadowExt[i].addJointInfo));
					meshShadow->AttachChild(instIndexShadowList,shadowExt[i].add_mesh_info);

				}
				if(shadowExt[i].removeIndex != INVALID_INSTANCE_INDEX)
				{
					if(shadowExt[i].removeEffect)
						delete shadowExt[i].removeEffect;
					else //must still be connected
					{
						ENGINE->destroy_joint(instIndexShadowList,shadowExt[i].removeIndex);
						meshShadow->DetachChild(shadowExt[i].removeIndex,&shadowExt[i].remove_mesh_info);
					}
				}

				//TobjExtent::SetColors
				shadowChildren = meshShadow->ListChildren(meshShadowMI);
			}
		}
	}
	
	if(shadowPercent[playerIndex] != 1.0 && shadowUpgradeWorking[playerIndex] != -1)
	{
		if(shadowExt[shadowUpgradeWorking[playerIndex]].addIndex != INVALID_INSTANCE_INDEX)
		{
			IBaseObject *obj = ARCHLIST->CreateInstance(initSt->pBuildEffect);
			obj->QueryInterface(IBuildEffectID,shadowBuildEffect, NONSYSVOLATILEPTR);
			shadowBuildEffect->SetupMesh(this,shadowExt[shadowUpgradeWorking[playerIndex]].add_mesh_info,0);
			shadowBuildEffect->SetBuildPercent(shadowPercent[playerIndex]);
			shadowAddMesh = shadowExt[shadowUpgradeWorking[playerIndex]].add_mesh_info;
			shadowAddInstance = shadowExt[shadowUpgradeWorking[playerIndex]].addIndex;
		}
		if(shadowExt[shadowUpgradeWorking[playerIndex]].removeIndex != INVALID_INSTANCE_INDEX)
		{
			ENGINE->destroy_joint(instIndexShadowList,shadowExt[shadowUpgradeWorking[playerIndex]].removeIndex);
			meshShadow->DetachChild(shadowExt[shadowUpgradeWorking[playerIndex]].removeIndex,&shadowExt[shadowUpgradeWorking[playerIndex]].remove_mesh_info);

			IBaseObject *obj = ARCHLIST->CreateInstance(initSt->pBuildEffect);
			obj->QueryInterface(IBuildEffectID,shadowBuildEffect2, NONSYSVOLATILEPTR);
			shadowBuildEffect2->SetupMesh(this,shadowExt[shadowUpgradeWorking[playerIndex]].remove_mesh_info,0);
			shadowBuildEffect2->SetBuildPercent(1.0-shadowPercent[playerIndex]);
			shadowRemoveMesh = shadowExt[shadowUpgradeWorking[playerIndex]].remove_mesh_info;
			shadowRemoveInstance = shadowExt[shadowUpgradeWorking[playerIndex]].removeIndex;
		}
	}
	shadowChildren = meshShadow->ListChildren(meshShadowMI);

	for(exCount = 0; exCount < MAX_EXTENSIONS; ++exCount)
	{
		if(shadowExt[exCount].bAttached)
		{
			if(shadowExt[exCount].addIndex != -1 && shadowExt[exCount].bDuplicateAdd)
			{
				for(U32 i = exCount+1; i < MAX_EXTENSIONS; ++i)
				{
					if(shadowExt[i].addIndex == shadowExt[exCount].addIndex)
					{
						shadowExt[i].addIndex = -1;
					}
				}
			}
			if(shadowExt[exCount].removeIndex != -1)
			{
				if(shadowExt[exCount].bDuplicateRemove)
				{
					for(U32 i = exCount+1; i < MAX_EXTENSIONS; ++i)
					{
						if(shadowExt[i].removeIndex == shadowExt[exCount].removeIndex)
						{
							shadowExt[i].removeIndex = -1;
						}
					}
				}
				if(shadowExt[exCount].removeIndex != shadowRemoveInstance)
				{
					ENGINE->destroy_instance(shadowExt[exCount].removeIndex);
					DestroyMeshInfoTree(shadowExt[exCount].remove_mesh_info);
				}
			}
		}
		else
		{
			if(shadowExt[exCount].addIndex != -1)
			{
				if(!shadowExt[exCount].bDuplicateAdd)
				{
					if(shadowExt[exCount].addIndex != shadowAddInstance)
					{
						ENGINE->destroy_instance(shadowExt[exCount].addIndex);
						DestroyMeshInfoTree(shadowExt[exCount].add_mesh_info);
					}
				}
				else
				{
					bool deleteNeeded = true;
					for(U32 i = exCount+1; i < MAX_EXTENSIONS; ++i)
					{
						if(shadowExt[i].addIndex == shadowExt[exCount].addIndex)
						{
							if(shadowExt[i].bAttached)
							{
								deleteNeeded = false;
							}
							shadowExt[i].addIndex = -1;
						}
					}
					if(deleteNeeded)
					{
						if(shadowExt[exCount].addIndex != shadowAddInstance)
						{
							ENGINE->destroy_instance(shadowExt[exCount].addIndex);
							DestroyMeshInfoTree(shadowExt[exCount].add_mesh_info);
						}
					}
				}
			}
		}
	}


}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::Render (void)
{
	if (bVisible)
	{
		if((0x01 << (MGlobals::GetThisPlayer()-1)) & shadowVisibilityFlags)
		{
			U32 newShadowPlayer = MGlobals::GetThisPlayer();
			if(newShadowPlayer != shadowPlayer)
			{
				if(shadowPlayer)
				{
					ENGINE->destroy_instance(instIndexShadowList);
					DestroyMeshInfoTree(meshShadow);
					instIndexShadowList = -1;
					if(shadowBuildEffect)
					{
						delete shadowBuildEffect.Ptr();
						shadowBuildEffect = NULL;
					}
					if(shadowBuildEffect2)
					{
						delete shadowBuildEffect2.Ptr();
						shadowBuildEffect2 = NULL;
					}
					if(shadowAddInstance != -1)
					{
						ENGINE->destroy_instance(shadowAddInstance);
						DestroyMeshInfoTree(shadowAddMesh);
						shadowAddInstance = -1;
					}
					if(shadowRemoveInstance != -1)
					{
						ENGINE->destroy_instance(shadowRemoveInstance);
						DestroyMeshInfoTree(shadowRemoveMesh);
						shadowRemoveInstance = -1;
					}
					shadowPlayer = 0;
				}
				shadowPlayer = newShadowPlayer;
				U32 playerIndex = shadowPlayer-1;

				InitStruct * initSt = ((InitStruct *) (ARCHLIST->GetArchetypeHandle(pArchetype)));
				instIndexShadowList = ENGINE->create_instance2(initSt->archIndex,NULL);
				ENGINE->set_transform(instIndexShadowList,GetTransform());
				ENGINE->update_instance(instIndexShadowList,0,0);
				meshShadow = CreateMeshInfoTree(instIndexShadowList);
				shadowChildren = meshShadow->ListChildren(meshShadowMI);
		
				if (initSt->mr == 0)
				{
					CQASSERT(shadowChildren);
					typedef IMeshRender * booga;
					IMeshRender **mr = new booga[shadowChildren];
					for (U32 i=0;i<shadowChildren;i++)
					{
						mr[i] = CreateMeshRender();
						mr[i]->AddRef();
						mr[i]->Init(meshShadowMI[i]);
					}
					
					IMeshRender ***pmr = (IMeshRender ***)(void *)&initSt->mr;
					int *nm = (int *)(void *)&initSt->numChildren;
					*nm = shadowChildren;
					*pmr = mr;
				}

				for (U32 i=0;i<shadowChildren;i++)
				{
					//we need the exact corresponding mr array
					initSt->mr[i]->SetupMeshInfo(meshShadowMI[i]);  //somewhat less suspicious code
				}
				setColorsShadowMesh(playerIndex);

				detachChildren(playerIndex);

				if(shadowPercent[playerIndex] != 1.0 && shadowUpgradeWorking[playerIndex] == -1)
				{
					IBaseObject *obj = ARCHLIST->CreateInstance(initSt->pBuildEffect);
					obj->QueryInterface(IBuildEffectID,shadowBuildEffect, NONSYSVOLATILEPTR);
					shadowBuildEffect->SetupMesh(NULL,NULL,meshShadow,instIndexShadowList,0);
					shadowBuildEffect->SetBuildPercent(shadowPercent[playerIndex]);
				}
			}
			if (!bSpecialRender)
			{
				AlphaStackRender(meshShadowMI,shadowChildren);
				if (rootChild)
				{
					rootChild[0].fgi[0].a = 128;
					if (buildPlanetID)
					{
						IBaseObject * obj = OBJLIST->FindObject(buildPlanetID);
							OBJPTR<IPlanet> planet;
						
						if (obj && obj->QueryInterface(IPlanetID, planet))
						{
								if (planet->IsMouseOver())
								rootChild[0].fgi[0].a = 80;
						}
					}

					AlphaStackRender(&rootChild,1);
				}
			}
			if(shadowBuildEffect)
				shadowBuildEffect->Render();
			if(shadowBuildEffect2)
				shadowBuildEffect2->Render();
		}
		else if(!bPlatDead )
		{
			FRAME_preRender();
			
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			U8 ambientColorRed = 0;
			U8 ambientColorGreen = 0;
			U8 ambientColorBlue = 0;
			if(bNoAutoTarget)
			{
				pulseTimer += OBJLIST->GetRealRenderTime();
				if(pulseTimer > 2.0)
					pulseTimer -= 2.0;

				LIGHTS->GetSysAmbientLight(ambientColorRed,ambientColorGreen,ambientColorBlue);
				SINGLE i = pulseTimer;
				if(i > 1.0)
					i = 1.0-(i-1.0);
				LIGHTS->SetSysAmbientLight((255-ambientColorRed)*i+ambientColorRed,(255-ambientColorGreen)*i+ambientColorGreen,(0-ambientColorBlue)*i+ambientColorBlue);
			}
			if (!bSpecialRender)
			{
				if(instanceMesh)
					instanceMesh->Render();
			}
			if(bNoAutoTarget)
			{
				LIGHTS->SetSysAmbientLight(ambientColorRed,ambientColorGreen,ambientColorBlue);
			}


			FRAME_render();

			FRAME_postRender();

			PIPE->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
			if(explosion)
				explosion.Ptr()->Render();
		}
		else if(explosion)
		{
			if(explosion->ShouldRenderParent())
			{
				FRAME_preRender();
				
				BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
				BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
				/*if (building)//||bExploding)
				{
				ENGINE->render_instance(MAINCAM, instanceIndex, 1.0);
				}
				else
				if (CQEFFECTS.bFastRender == 0)//  || bNoMeshRender)
				ENGINE->render_instance(MAINCAM, instanceIndex,0, LODPERCENT,0,0);
				else
				{*/
				if (!bSpecialRender)
				{
					if(instanceMesh)
						instanceMesh->Render();
				}
				
				
				FRAME_render();

				FRAME_postRender();

				PIPE->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
			}
			explosion.Ptr()->Render();
		}
	}
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::SetReady(bool _bReady)
{
	bReady = _bReady;

	ObjectTeam
		<ObjectDamage
		<ObjectBuild  //ObjectDamage depends on this

				<ObjectExtension
					<ObjectExtent
						<ObjectSelection
							<ObjectMission
								<ObjectTransform
										<ObjectFrame<IBaseObject,BASE_PLATFORM_SAVELOAD,BASEPLATINIT> >
								>
							>
						>
					>
				>
			>
		>::SetReady(_bReady);

	if(mObjClass == M_JUMPPLAT || mObjClass == M_HQ || mObjClass == M_COCOON || mObjClass == M_ACROPOLIS || mObjClass == M_LOCUS)
	{
		SECTOR->ComputeSupplyForAllPlayers();
	}
}
//--------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::UpdateVisibilityFlags (void)
{
	U8 newShadowVisibilityFlags;
	if(bPlatDead)
	{
		newShadowVisibilityFlags = shadowVisibilityFlags & (~GetPendingVisibilityFlags());
	}
	else
		newShadowVisibilityFlags = GetVisibilityFlags() & (~GetPendingVisibilityFlags());
	if(newShadowVisibilityFlags != shadowVisibilityFlags)
	{
		U32 newFlags = newShadowVisibilityFlags & (~shadowVisibilityFlags);
		if(newFlags)
		{
			for(U32 i = 0; i < MAX_PLAYERS; ++i)
			{
				if((newFlags >> i) & 0x01)
				{
					shadowMaxHull[i] = hullPointsMax;
					shadowHullPoints[i] = hullPoints;
					shadowUpgrade[i] = GetUpgrade();
					shadowUpgradeWorking[i] = GetWorkingUpgrade();
					shadowUpgradeFlags[i] = GetUpgradeFlags();
					if(shadowUpgradeWorking[i] != -1)
					{
						shadowPercent[i] = GetUpgradePercent();
					}
					else if(!IsComplete())
					{
						U32 dummy;
						shadowPercent[i] = GetBuildProgress(dummy);
					}else
					{
						shadowPercent[i] = 1.0;
					}
				}
			}
		}
		U32 oldFlags = (~newShadowVisibilityFlags) & shadowVisibilityFlags;
		if(oldFlags)
		{
			if(bPlatDead)
			{
				if(objMapNode)
					objMapNode->flags &= ~(oldFlags << 24);
				if(THEMATRIX->IsMaster())
					UnregisterWatchersForObjectForPlayerMask(this,oldFlags << 1);
			}
			if(shadowPlayer)
			{
				if((0x01 <<(shadowPlayer-1)) & oldFlags)
				{
					if(instIndexShadowList != -1)
					{
						ENGINE->destroy_instance(instIndexShadowList);
						DestroyMeshInfoTree(meshShadow);
						instIndexShadowList = -1;
						if(shadowBuildEffect)
						{
							delete shadowBuildEffect.Ptr();
							shadowBuildEffect = NULL;
						}
						if(shadowBuildEffect2)
						{
							delete shadowBuildEffect2.Ptr();
							shadowBuildEffect2 = NULL;
						}
						if(shadowAddInstance != -1)
						{
							ENGINE->destroy_instance(shadowAddInstance);
							DestroyMeshInfoTree(shadowAddMesh);
							shadowAddInstance = -1;
						}
						if(shadowRemoveInstance != -1)
						{
							ENGINE->destroy_instance(shadowRemoveInstance);
							DestroyMeshInfoTree(shadowRemoveMesh);
							shadowRemoveInstance = -1;
						}
					}
					shadowPlayer = 0;
				}
			}
		}
		shadowVisibilityFlags = newShadowVisibilityFlags;
	}
	IBaseObject::UpdateVisibilityFlags2();//calling 2 on purpose;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::UpdateVisibilityFlags2 (void)
{
	U8 newShadowVisibilityFlags;
	if(bPlatDead)
	{
		newShadowVisibilityFlags = shadowVisibilityFlags & (~GetPendingVisibilityFlags());
	}
	else
		newShadowVisibilityFlags = GetVisibilityFlags() & (~GetPendingVisibilityFlags());
	if(newShadowVisibilityFlags != shadowVisibilityFlags)
	{
		U32 newFlags = newShadowVisibilityFlags & (~shadowVisibilityFlags);
		if(newFlags)
		{
			for(U32 i = 0; i < MAX_PLAYERS; ++i)
			{
				if((newFlags >> i) & 0x01)
				{
					shadowMaxHull[i] = hullPointsMax;
					shadowHullPoints[i] = hullPoints;
					shadowUpgrade[i] = GetUpgrade();
					shadowUpgradeWorking[i] = GetWorkingUpgrade();
					shadowUpgradeFlags[i] = GetUpgradeFlags();
					if(shadowUpgradeWorking[i] != -1)
					{
						shadowPercent[i] = GetUpgradePercent();
					}
					else if(!IsComplete())
					{
						U32 dummy;
						shadowPercent[i] = GetBuildProgress(dummy);
					}else
					{
						shadowPercent[i] = 1.0;
					}
				}
			}
		}
		U32 oldFlags = (~newShadowVisibilityFlags) & shadowVisibilityFlags;
		if(oldFlags)
		{
			if(bPlatDead)
			{
				if(objMapNode)
					objMapNode->flags &= ~(oldFlags << 24);
				if(THEMATRIX->IsMaster())
					UnregisterWatchersForObjectForPlayerMask(this,oldFlags << 1);
			}
			if(shadowPlayer)
			{
				if((0x01 <<(shadowPlayer-1)) & oldFlags)
				{
					if(instIndexShadowList != -1)
					{
						ENGINE->destroy_instance(instIndexShadowList);
						DestroyMeshInfoTree(meshShadow);
						instIndexShadowList = -1;
						if(shadowBuildEffect)
						{
							delete shadowBuildEffect.Ptr();
							shadowBuildEffect = NULL;
						}
						if(shadowBuildEffect2)
						{
							delete shadowBuildEffect2.Ptr();
							shadowBuildEffect2 = NULL;
						}
						if(shadowAddInstance != -1)
						{
							ENGINE->destroy_instance(shadowAddInstance);
							DestroyMeshInfoTree(shadowAddMesh);
							shadowAddInstance = -1;
						}
						if(shadowRemoveInstance != -1)
						{
							ENGINE->destroy_instance(shadowRemoveInstance);
							DestroyMeshInfoTree(shadowRemoveMesh);
							shadowRemoveInstance = -1;
						}
					}
					shadowPlayer = 0;
				}
			}
		}
		shadowVisibilityFlags = newShadowVisibilityFlags;
	}
	IBaseObject::UpdateVisibilityFlags2();//calling 2 on purpose;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
SINGLE Platform<SaveStruct,InitStruct>::TestHighlight (const RECT & rect)
{
	SINGLE closeness = 999999.0f;

	bHighlight = 0;
	if (bVisible)
	{
		if ((instanceIndex != INVALID_INSTANCE_INDEX) != 0)
		{
			float depth=0, center_x=0, center_y=0, radius=0;

			if ((bVisible = get_pbs(center_x, center_y, radius, depth)) != 0)
			{
				RECT _rect;

				_rect.left  = center_x - radius;
				_rect.right	= center_x + radius;
				_rect.top = center_y - radius;
				_rect.bottom = center_y + radius;

				RECT screenRect = { 0, 0, SCREENRESX, SCREENRESY };

				if ((bVisible = RectIntersects(_rect, screenRect)) != 0)
				{
					if(BUILDARCHEID == 0)		// no highlighting in buildmode
					{
						if (RectIntersects(rect, _rect))
						{
							ViewPoint points[64];
							int numVerts = sizeof(points) / sizeof(ViewPoint);
							INSTANCE_INDEX id = instanceIndex;
							if (aliasArchetypeID != INVALID_INSTANCE_INDEX)
							{
								MeshChain *mc;
								mc = UNBORNMANAGER->GetMeshChain(transform,aliasArchetypeID);
								id = mc->mi[0]->instanceIndex;
							}
							if (REND->get_instance_projected_bounding_polygon(id, MAINCAM, LODPERCENT, numVerts, points, numVerts, depth))
							{
								bHighlight = RectIntersects(rect, points, numVerts);

								if (rect.left == rect.right && rect.top == rect.bottom)
									closeness = fabs(rect.left - center_x) * fabs(rect.top - center_y);
								else
									closeness = 0.0f;
							}
						}
					}
				}
			}
		}
		else
		{
			float depth=0, center_x=0, center_y=0, radius=0;

			MeshChain *mc;
			mc = UNBORNMANAGER->GetMeshChain(transform,ARCHLIST->GetArchetypeDataID(pArchetype));
			INSTANCE_INDEX id = mc->mi[0]->instanceIndex;
			if ((bVisible = get_pbs_platform(id,center_x, center_y, radius, depth)) != 0)
			{
				RECT _rect;

				_rect.left  = center_x - radius;
				_rect.right	= center_x + radius;
				_rect.top = center_y - radius;
				_rect.bottom = center_y + radius;

				RECT screenRect = { 0, 0, SCREENRESX, SCREENRESY };

				if ((bVisible = RectIntersects(_rect, screenRect)) != 0)
				{
					if(BUILDARCHEID == 0)		// no highlighting in buildmode
					{
						if (RectIntersects(rect, _rect))
						{
							ViewPoint points[64];
							int numVerts = sizeof(points) / sizeof(ViewPoint);
							if (REND->get_instance_projected_bounding_polygon(id, MAINCAM, LODPERCENT, numVerts, points, numVerts, depth))
							{
								bHighlight = RectIntersects(rect, points, numVerts);

								if (rect.left == rect.right && rect.top == rect.bottom)
									closeness = fabs(rect.left - center_x) * fabs(rect.top - center_y);
								else
									closeness = 0.0f;
							}
						}
					}
				}
			}
		}
	}
	
	if(bVisible)
	{
		if(objClass == OC_SPACESHIP || objClass == OC_PLATFORM)//right now I only want ships and platforms to count toward the metric
			OBJLIST->IncrementShipsToRender();
	}
	
	if(bHighlight && bPlatDead)
	{
		if(!((0x01 << (MGlobals::GetThisPlayer()-1) ) & shadowVisibilityFlags))
		{
			bHighlight = false;
			return 0;
		}
	}
	return closeness;
}
//-------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::DrawHighlighted (void)
{
	if (bVisible==0)
		return;
	const USER_DEFAULTS * const pDefaults = DEFAULTS->GetDefaults();

	SINGLE hull, supplies;
	U32 hullMax, suppliesMax;

	if (getPlatStats(hull, supplies, hullMax, suppliesMax))
	{
		Vector point;
		S32 x, y;

		int TBARLENGTH = 100;
		if (hullMax < 1000)
		{
			if (hullMax < 100)
			{
				if (hullMax > 0)
					TBARLENGTH = 20;
				else
				{
					// no hull points, length should be decided by supplies
					// use same length as max supplies
				}
			}
			else // hullMax >= 100
			{
				TBARLENGTH = 20 + (((hullMax - 100)*80) / (1000-100));
			}
		}
		TBARLENGTH = IDEAL2REALX(TBARLENGTH);

		// want the bar length to match up with a little rectangle square
		if (TBARLENGTH%5)
		{
			TBARLENGTH -= TBARLENGTH%5;
		}


		point.x = 0;
		point.y = H2+250.0;
		point.z = 0;

		CAMERA->PointToScreen(point, &x, &y, &transform);
		PANE * pane = CAMERA->GetPane();

		if (hull >= 0.0f)
		{
			COLORREF color;

			// draw the green (health) bar
			// colors  (0,130,0) (227,227,34) (224, 51, 37)

			// choose the color

			if (hull > 0.667F)
				color = RGB(0,130,0);
			else
			if (hull > 0.5F)
			{
				SINGLE diff = (0.667F - hull) / (0.667F - 0.5F);
				U8 r, g, b;
				r = (U8) ((227 - 0) * diff) + 0;
				g = (U8) ((227 - 130) * diff) + 130;
				b = (U8) ((34 - 0) * diff) + 0;
				color = RGB(r,g,b);
			}
			else
			if (hull > 0.25F)
			{
				SINGLE diff = (0.5F - hull) / (0.5F - 0.25F);
				U8 r, g, b;
				r = (U8) ((224 - 227) * diff) + 227;
				g = (U8) ((51 - 227) * diff) + 227;
				b = (U8) ((37 - 34) * diff) + 34;
				color = RGB(r,g,b);
			}
			else
			{
				color = RGB(224,51,37);
			}

			// done choosing the color
			
			DA::RectangleHash(pane, x-(TBARLENGTH/2), y, x+(TBARLENGTH/2), y+2, RGB(128,128,128));
//			DA::RectangleFill(pane, x-(TBARLENGTH/2), y, x-(TBARLENGTH/2)+S32(TBARLENGTH*hull), y+2, color);

			int xpos = x-(TBARLENGTH/2);
			int max = S32(TBARLENGTH*hull);
			int xrc;

			// make sure at least one bar gets displayed for one health point
			if (max == 0 && hull > 0.0f)
			{
				max = 1;
			}
			
			for (int i = 0; i < max; i+=5)
			{
				xrc = xpos + i;
				DA::RectangleFill(pane, xrc, y, xrc+3, y+2, color);
			}
		}
		if (supplies >= 0.0f)
		{
			//
			// draw the blue (supplies) bar RGB(0,128,225)
			//
			if ((pDefaults->bCheatsEnabled && DBHOTKEY->GetHotkeyState(IDH_HIGHLIGHT_ALL)) || pDefaults->bEditorMode || playerID == 0 || MGlobals::AreAllies(playerID, MGlobals::GetThisPlayer()))
			{
				DA::RectangleHash(pane, x-(TBARLENGTH/2), y+5, x+(TBARLENGTH/2), y+5+2, RGB(128,128,128));
				if (supplies > 0.0f)
				{
//					DA::RectangleFill(pane, x-(TBARLENGTH/2), y+5, x-(TBARLENGTH/2)+S32(TBARLENGTH*supplies), y+5+2, RGB(0,128,255));
					COLORREF color;
					if(fieldFlags.suppliesLocked())
						color = RGB(180,180,240);
					else
						color = RGB(0,128,240);

					int xpos = x-(TBARLENGTH/2);
					int max = S32(TBARLENGTH*supplies);
					int xrc;

					// make sure at least one bar gets displayed for one supply point
					if (max == 0 && supplies > 0.0f)
					{
						max = 1;
					}
					
					for (int i = 0; i < max; i+=5)
					{
						xrc = xpos + i;
						DA::RectangleFill(pane, xrc, y+5, xrc+3, y+7, color);
					}
				}
			}
			else	// don't know supply info
			{
			}
		}

		if (nextHighlighted==0 && OBJLIST->GetHighlightedList()==this)
		{
			COMPTR<IFontDrawAgent> pFont;
			if (OBJLIST->GetUnitFont(pFont) == GR_OK)
			{
				if (bShowPartName)
					pFont->SetFontColor(RGB(140,140,180) | 0xFF000000, 0);
				else
					pFont->SetFontColor(RGB(180,180,180) | 0xFF000000, 0);
				wchar_t temp[M_MAX_STRING];
				WM->GetCursorPos(x, y);
				y += IDEAL2REALY(24);
#ifdef _DEBUG
				_localAnsiToWide(partName, temp, sizeof(temp));
				pFont->StringDraw(pane, x, y, temp);
#else
				if (bShowPartName)
				{
					_localAnsiToWide(partName, temp, sizeof(temp));
				}
				else
				{
					wchar_t * ptr;
					wcsncpy(temp, _localLoadStringW(pInitData->displayName), sizeof(temp)/sizeof(wchar_t));

					if ((ptr = wcschr(temp, '#')) != 0)
					{
						*ptr = 0;
					}
					if ((ptr = wcschr(temp, '(')) != 0)
					{
						*ptr = 0;
					}
				}
				pFont->StringDraw(pane, x, y, temp);
#endif
			}
		}

	}
}
//-------------------------------------------------------------------
//
// hull,supplies in range 0 to 1.0
//
template <class SaveStruct, class InitStruct>
bool Platform<SaveStruct,InitStruct>::getPlatStats (SINGLE & hull, SINGLE & supplies, U32 & hullMax, U32 & suppliesMax) const
{
	hull = -1.0f;
	supplies = -1.0F;

	U32 dispPlayer = MGlobals::GetThisPlayer()-1;
	if((0x01 << dispPlayer) & shadowVisibilityFlags)
	{
		if ((hullMax = shadowMaxHull[dispPlayer]) != 0)
			hull = SINGLE(shadowHullPoints[dispPlayer]) / SINGLE(hullMax);
	}
	else
	{
		if ((hullMax = hullPointsMax) != 0)
			hull = SINGLE(GetDisplayHullPoints()) / SINGLE(hullPointsMax);
	}

	if ((suppliesMax = supplyPointsMax) != 0)
		supplies = SINGLE(GetDisplaySupplies()) / SINGLE(supplyPointsMax);

	return (hull >= 0 || supplies >= 0);
}
//-------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
bool Platform<SaveStruct,InitStruct>::get_pbs_platform( INSTANCE_INDEX id, float & cx,
														float & cy,
														float & radius,
														float & depth)
{
	bool result = false;
	CQASSERT(id != INVALID_INSTANCE_INDEX);
	float obj_rad;
	Vector wcenter(0,0,0);
	const Transform *cam2world = CAMERA->GetTransform();
	ENGINE->get_instance_bounding_sphere(id,0,&obj_rad,&wcenter);

#ifdef _ROB
	if (obj_rad > 20000)
		CQBOMB1("%s moved without update - potentially ignorable",(char *)partName);
#endif

	Vector vcenter = cam2world->inverse_rotate_translate(transform*wcenter);
				
	// Make sure object is in front of near plane.
	if (vcenter.z < -MAINCAM->get_znear())
	{
		const struct ViewRect * pane = MAINCAM->get_pane();
		
		float x_screen_center = float(pane->x1 - pane->x0) * 0.5f;
		float y_screen_center = float(pane->y1 - pane->y0) * 0.5f;
		float screen_center_x = pane->x0 + x_screen_center;
		float screen_center_y = pane->y0 + y_screen_center;
		
		float w = -1.0 / vcenter.z;
		float sphere_center_x = vcenter.x * w;
		float sphere_center_y = vcenter.y * w;
		
		cx = screen_center_x + sphere_center_x * MAINCAM->get_hpc()*MAINCAM->get_znear();
		cy = screen_center_y + sphere_center_y * MAINCAM->get_vpc()*MAINCAM->get_znear();
		
		float center_distance = vcenter.magnitude();
		
		if(center_distance >= obj_rad)
		{
			float dx = fabs(cx - screen_center_x);
			float dy = fabs(cy - screen_center_y);
			
			//changes 1/26 - rmarr
			//function should now not return TRUE with obscene radii
			float outer_angle = asin(obj_rad / center_distance);
			sphere_center_x = fabs(sphere_center_x);
			float inner_angle = atan(sphere_center_x);
			
			//	float near_plane_radius = tan(inner_angle + outer_angle);
			//	near_plane_radius -= sphere_center_x;
			//	radius = near_plane_radius * camera->get_hpc();
			
			float near_plane_radius = tan(inner_angle - outer_angle);
			near_plane_radius = sphere_center_x-near_plane_radius;
			radius = near_plane_radius * MAINCAM->get_hpc()*MAINCAM->get_znear();
			
			int view_w = (pane->x1 - pane->x0 + 1) >> 1;
			int view_h = (pane->y1 - pane->y0 + 1) >> 1;
			
			if ((dx < (view_w + radius)) && (dy < (view_h + radius)))
			{
				depth = -vcenter.z;
				result = true;
			}
		}
	}
				
	return result;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::SelfDestruct (bool bExplode)
{
	if(!bPlatDead)
	{
		if(bSelected)
			OBJLIST->FlushSelectedList();
		SetReady(false);
		FRAME_explode(bExplode);
		bPlatDead = true;
		U32 flags = shadowVisibilityFlags;
		if(objMapNode)
			objMapNode->flags |= (OM_SHADOW| (flags << 24));
		if(THEMATRIX->IsMaster())
			UnregisterWatchersForObjectForPlayerMask(this,((~shadowVisibilityFlags) << 1)|(0x01 << SYSVOLATILEPTR));
	}
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::SetTerrainFootprint (struct ITerrainMap * terrainMap)
{
	bool bFloating = (((BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype)))->slotsNeeded == 0);
	FootprintHistory footprint;
	//
	// calculate new footprint
	//
	if (systemID<=MAX_SYSTEMS)// && isAutoMovementEnabled())
	{
		footprint.info[0].flags = footprint.info[1].flags = bHalfSquare ? TERRAIN_HALFSQUARE : TERRAIN_FULLSQUARE; 
		footprint.info[0].height = footprint.info[1].height = box[2];		// maxy
		footprint.info[0].missionID = footprint.info[1].missionID = dwMissionID;
		footprint.systemID = systemID;

		footprint.info[0].flags |= TERRAIN_PARKED;
		footprint.vec[0] = GetGridPosition();
		footprint.numEntries = 1;
	}
	
	//
	// send information if different
	//
	if (footprint != footprintHistory)
	{
		undoFootprintInfo(terrainMap);

		if (bFloating)		// only set a terrain footprint if we are free - floating
		{
			if (footprint.numEntries>0)
				terrainMap->SetFootprint(&footprint.vec[0], 1, footprint.info[0]);
			if (footprint.numEntries>1)
				terrainMap->SetFootprint(&footprint.vec[1], 1, footprint.info[1]);
		}
		
		footprintHistory = footprint;

		if (systemID && systemID <= MAX_SYSTEMS)
		{
			int new_map_square = OBJMAP->GetMapSquare(systemID,transform.translation);
			if (new_map_square != map_square || map_sys != ((int)systemID) )
			{
				OBJMAP->RemoveObjectFromMap(this,map_sys,map_square);
				map_square = new_map_square;
				map_sys = systemID;
				U32 flags = OM_TARGETABLE;
				if(bPlatDead)
					flags |= (OM_SHADOW| (shadowVisibilityFlags << 24));
				objMapNode = OBJMAP->AddObjectToMap(this,map_sys,map_square,flags);
			}
		}
	}
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::SetPosition(const Vector & vector, U32 newSystemID)
{
	ObjectTeam								
		<ObjectDamage
												<ObjectBuild  //ObjectDamage depends on this
														<ObjectExtension
															<ObjectExtent
																<ObjectSelection
																	<ObjectMission
																		<ObjectTransform
																				<ObjectFrame<IBaseObject,BASE_PLATFORM_SAVELOAD,BASEPLATINIT> >
																		>
																	>
																>
															>
														>
													>
													>::SetPosition(vector, newSystemID);
	
	systemID = newSystemID;
	CQASSERT(systemID && systemID<=MAX_SYSTEMS);
	COMPTR<ITerrainMap> terrainMap;
	SECTOR->GetTerrainMap(systemID, terrainMap);
	SetTerrainFootprint(terrainMap);
	
	if (rootChild)
	{
		ENGINE->set_transform(rootChild[0].instanceIndex,transform);
	}
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::SetTransform (const TRANSFORM & transform, U32 newSystemID)
{
	ObjectTeam

													<ObjectDamage
														<ObjectBuild  //ObjectDamage depends on this
														<ObjectExtension
															<ObjectExtent
																<ObjectSelection
																	<ObjectMission
																		<ObjectTransform
																				<ObjectFrame<IBaseObject,BASE_PLATFORM_SAVELOAD,BASEPLATINIT> >
																		>
																	>
																>
															>
														>
													>
													>::SetTransform(transform, newSystemID);

	systemID = newSystemID;

	CQASSERT(systemID && systemID<=MAX_SYSTEMS);
	if (IsDeepSpacePlatform())
	{
		COMPTR<ITerrainMap> terrainMap;
		SECTOR->GetTerrainMap(systemID, terrainMap);
		SetTerrainFootprint(terrainMap);
	}
	
	if (rootChild)
	{
		ENGINE->set_transform(rootChild[0].instanceIndex,transform);
	}
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::TakeoverSwitchID (U32 newMissionID)
{
	// first thing, undo the current footprint
	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(systemID, map);
	undoFootprintInfo(map);

	U32 plId = (MGlobals::GetPlayerFromPartID(dwMissionID));
	if (plId)
	{
		IBaseObject * obj = OBJLIST->GetObjectList();
		bool setNoPlatform = true;
		while(obj)
		{
			if((obj->objClass == OC_PLATFORM) && (MPart(obj)->mObjClass == mObjClass) && (obj != ((IBaseObject *)this)) &&
				(obj->GetPlayerID() == plId))
			{
				setNoPlatform = false;
				obj = NULL;
			}
			else
				obj = obj->next;
		}
		if(setNoPlatform)
		{
			TECHNODE oldTech = MGlobals::GetCurrentTechLevel(plId);
			SINGLE_TECHNODE myNode = ((BASE_PLATFORM_DATA *)ARCHLIST->GetArchetypeData(pArchetype))->techActive;
			oldTech.RemoveFromNode(myNode);
			MGlobals::SetCurrentTechLevel(oldTech,plId);
		}
	}

	OBJLIST->RemovePartID(this, dwMissionID);
	dwMissionID = newMissionID;

	OBJLIST->AddPartID(this, dwMissionID);

	UnregisterWatchersForObject(this);

	// now set our footprint again
	SetTerrainFootprint(map);
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::undoFootprintInfo (struct ITerrainMap * terrainMap)
{
	if (footprintHistory.numEntries>0)
	{
		COMPTR<ITerrainMap> map;

		if (footprintHistory.systemID != systemID)
			SECTOR->GetTerrainMap(footprintHistory.systemID, map);
		else
			map = terrainMap;
		map->UndoFootprint(&footprintHistory.vec[0], 1, footprintHistory.info[0]);

		if (footprintHistory.numEntries>1)
			map->UndoFootprint(&footprintHistory.vec[1], 1, footprintHistory.info[1]);
	}

	footprintHistory.numEntries = 0;

	if (OBJMAP && map_sys)
	{
		OBJMAP->RemoveObjectFromMap(this,map_sys,map_square);
		objMapNode = 0;
		map_sys = map_square = 0;
	}
}

//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
BOOL32 Platform<SaveStruct,InitStruct>::ApplyDamage (IBaseObject * collider, U32 _ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit)
{
	SYSMAP->RegisterAttack(systemID,GetGridPosition(),playerID);
	if(bAllEventsOn)
		MScript::RunProgramsWithEvent(CQPROGFLAG_UNITHIT,dwMissionID,_ownerID);
	BOOL32 result=0;
	USER_DEFAULTS * const defaults = DEFAULTS->GetDefaults();
	if (defaults->bCheatsEnabled && defaults->bNoDamage)
		return 0;

	amount = MGlobals::GetEffectiveDamage(amount, OBJLIST->FindObject(_ownerID), this, _ownerID);

	if (displayHull < 0)		// if uninitialized
		displayHull = hullPoints;

	U32 const hisID = MGlobals::GetPlayerFromPartID(_ownerID);
	if (MGlobals::AreAllies(hisID, MGlobals::GetPlayerFromPartID(dwMissionID)) == 0)
	{
		FLEETSHIP_UNDERATTACK;
	}

	if(bInvincible)
	{
		if(S32(hullPoints - amount) <= hullPointsMax/10)
		{
			amount = 0;
		}
	}

	if (hullPoints!=0 && THEMATRIX->IsMaster())
	{
		if (S32(hullPoints - amount) <= 0)
		{
			myKillerOwnerID = _ownerID;
			hullPoints = 0;
		}
		else
			hullPoints -= amount;
	}
	
	if (bVisible)
	{
		if(fieldFlags.hasBlast())
			FIELDMGR->CreateFieldBlast(this,pos,systemID);
		Vector collide_pos(0,0,0);
		if (hullPoints < 0.3*hullPointsMax)
		{
			BOOL32 bHit;
			Vector norm;
			bHit = GetCollisionPosition(collide_pos,norm,pos,dir);
			if(bHit)
			{
				collide_pos = transform.inverse_rotate_translate(collide_pos);
			}
		}
		else
		{
			result = 1;
			CreateShieldHit(pos,dir,collide_pos,amount);
		}
		RegisterDamage(collide_pos,amount);
	}

	return result;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
BOOL32 Platform<SaveStruct,InitStruct>::ApplyVisualDamage (IBaseObject * collider, U32 _ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit)
{
	BOOL32 result=0;

	amount = MGlobals::GetEffectiveDamage(amount, OBJLIST->FindObject(_ownerID), this, _ownerID);

	if (bVisible)
	{
		if(fieldFlags.hasBlast())
			FIELDMGR->CreateFieldBlast(this,pos,systemID);
		Vector collide_pos(0,0,0);
		if (hullPoints < 0.3*hullPointsMax)
		{
			BOOL32 bHit;
			Vector norm;
			bHit = GetCollisionPosition(collide_pos,norm,pos,dir);
			if(bHit)
			{
				collide_pos = transform.inverse_rotate_translate(collide_pos);
			}
		}
		else
		{
			result = 1;
			CreateShieldHit(pos,dir,collide_pos,amount);
		}
	}

	return result;
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::ApplyAOEDamage (U32 _ownerID, U32 amount)
{
	SYSMAP->RegisterAttack(systemID,GetGridPosition(),playerID);
	if(bAllEventsOn)
		MScript::RunProgramsWithEvent(CQPROGFLAG_UNITHIT,dwMissionID,_ownerID);
	USER_DEFAULTS * const defaults = DEFAULTS->GetDefaults();
	if (defaults->bCheatsEnabled && defaults->bNoDamage)
		return;

	amount = MGlobals::GetEffectiveDamage(amount, OBJLIST->FindObject(_ownerID), this, _ownerID);

	if (displayHull < 0)		// if uninitialized
		displayHull = hullPoints;

	U32 const hisID = MGlobals::GetPlayerFromPartID(_ownerID);
	if (MGlobals::AreAllies(hisID, MGlobals::GetPlayerFromPartID(dwMissionID)) == 0)
	{
		FLEETSHIP_UNDERATTACK;
	}

	if(bInvincible)
	{
		if(S32(hullPoints - amount) <= hullPointsMax/10)
		{
			amount = 0;
		}
	}

	if (hullPoints!=0 && THEMATRIX->IsMaster())
	{
		if (S32(hullPoints - amount) <= 0)
		{
			myKillerOwnerID = _ownerID;
			hullPoints = 0;
		}
		else
			hullPoints -= amount;

		displayHull = hullPoints;		// apply immediately!
		displayHull = __max(1, displayHull);	// prevent zombie
	}
}
//---------------------------------------------------------------------------
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::explodePlatform (bool bExplode)
{
	if(!bPlatDead)
	{
		//this is where we update the techtree of our loss....
		if (playerID)
		{
			IBaseObject * obj = OBJLIST->GetObjectList();
			bool setNoPlatform = true;
			while(obj)
			{
				if((obj->objClass == OC_PLATFORM) && (MPart(obj)->mObjClass == mObjClass) && (obj != ((IBaseObject *)this)) &&
					(obj->GetPlayerID() == MGlobals::GetPlayerFromPartID(dwMissionID)))
				{
					setNoPlatform = false;
					obj = NULL;
				}
				else
					obj = obj->next;
			}
			if(setNoPlatform)
			{
				TECHNODE oldTech = MGlobals::GetCurrentTechLevel(MGlobals::GetPlayerFromPartID(dwMissionID));
				SINGLE_TECHNODE myNode = ((BASE_PLATFORM_DATA *)ARCHLIST->GetArchetypeData(pArchetype))->techActive;
				oldTech.RemoveFromNode(myNode);
				MGlobals::SetCurrentTechLevel(oldTech,MGlobals::GetPlayerFromPartID(dwMissionID));
			}
		
			const BASE_PLATFORM_DATA * objData = (const BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
			if(bSetCommandPoints)
				BANKER->RemoveMaxResources(objData->metalStorage,objData->gasStorage,objData->crewStorage,playerID);
			if(THEMATRIX->IsMaster())
			{
				if(bSetCommandPoints)
				{
					bSetCommandPoints = false;
					BANKER->LoseCommandPt(playerID,commandPoints);
				}
				BANKER->FreeCommandPt(playerID,pInitData->resourceCost.commandPt);
				BANKER->LoseResources(GetMetalStored(),GetGasStored(),GetCrewStored(),playerID);
			}

			MGlobals::SetPlatformsLost(playerID, MGlobals::GetPlatformsLost(playerID)+1);
		}

		bExploding = true;
		const bool bSamePlayer = (playerID == MGlobals::GetThisPlayer());

		if (bExplode)
		{
			IBaseObject * expBase;
			bool bExplodedNow = false;	// did we create one?
			CQASSERT(explosion==0);		// should not have an explosion already
			if (EXPCOUNT < UPPER_EXP_BOUND)
			{
				if (EXPCOUNT < LOWER_EXP_BOUND || bVisible)
				{
					//explosion code
					if ((bSamePlayer||systemID==SECTOR->GetCurrentSystem()) && pExplosionType && (expBase = ARCHLIST->CreateInstance(pExplosionType)) != 0)
					{
						if (expBase->QueryInterface(IExplosionID, explosion,NONSYSVOLATILEPTR))
						{
							explosion->InitExplosion(this,playerID,sensorRadius,false,true);
						}
						else
							CQBOMB0("QueryInterface() failed!?");

						bExplodedNow = true;
				//		OBJLIST->AddObject(explosion);
					}
				}
			}
			if (bExplodedNow==false)	// just create the nuggets
			{
				IExplosion::CreateDebrisNuggets(this);
				IExplosion::RealizeDebrisNuggets(this);
			}
		}

		// remove the platform from the object map
	//	if (OBJMAP)
	//	{
	//		OBJMAP->RemoveObjectFromMap(this, systemID, OBJMAP->GetMapSquare(systemID,transform.translation));
	//		objMapNode = 0;
	//	}
	}
}
//--------------------------------------------------------------------------//
//
template <class SaveStruct, class InitStruct>
void Platform<SaveStruct,InitStruct>::initPlatform (const InitStruct & data)
{
	const BASE_PLATFORM_DATA * objData = data.pData;

	pArchetype = pArchetype;
	objClass = OC_PLATFORM;
	transform.rotate_about_i(90*MUL_DEG_TO_RAD);
	pExplosionType = data.pExplosionType;
	damageAnimArch = data.damageAnimArch;

	if (objData->ambient_animation[0])
	{
		ambientAnimIndex = ANIM->create_script_inst(data.animArchetype, instanceIndex, objData->ambient_animation);
		ANIM->script_start(ambientAnimIndex, Animation::LOOP, Animation::BEGIN);
	}
	bSetCommandPoints = false;
	commandPoints = objData->commandPoints;
	if ((HALFGRID/2 - boxRadius) >= 0)
		bHalfSquare = true;
	else
		bHalfSquare = false;

	bPlatDead = false;
	bPlatRealyDead = false;

	instIndexShadowList = -1;
	shadowAddInstance = -1;
	shadowRemoveInstance = -1;
	shadowPlayer = 0;
}

bool arch_callback( ARCHETYPE_INDEX parent_arch_index, ARCHETYPE_INDEX child_arch_index, void *user_data );

//--------------------------------------------------------------------------//
//-----------------------Platform archetype loading-------------------------//
//--------------------------------------------------------------------------//
//
template <class BT_TYPE> 
bool PLATFORM_INIT<BT_TYPE>::loadPlatformArchetype (BT_TYPE * data, PARCHETYPE _pArchetype)		// load archetype data
{
	bool result = false;

	pData = data;
	pArchetype = _pArchetype;

	meshArch = MESHMAN->CreateMeshArch(data->fileName);

	if(data->ambientEffect[0])
		ambientEffect = EFFECTPLAYER->LoadEffect(data->ambientEffect);
	
	//try to determine the ship's footprint size for usage by ObjGen
	if (ENGINE->is_archetype_compound(meshArch->GetEngineArchtype()))
	{
		ENGINE->enumerate_archetype_parts(meshArch->GetEngineArchtype(),arch_callback,&fp_radius);
	}
	else
	{
		float local_box[6];
		REND->get_archetype_bounding_box(meshArch->GetEngineArchtype(),1.0,local_box);
		fp_radius = __max(local_box[BBOX_MAX_X],-local_box[BBOX_MIN_X]);
		fp_radius = __max(fp_radius,local_box[BBOX_MAX_Z]);
		fp_radius = __max(fp_radius,-local_box[BBOX_MIN_Z]);
		fp_radius *= 2;
	}
	
	if (data->explosionType[0])
	{
		pExplosionType = ARCHLIST->LoadArchetype(data->explosionType);
		CQASSERT(pExplosionType);
		ARCHLIST->AddRef(pExplosionType, OBJREFNAME);
	}
	if (data->shieldHitType[0])
	{
		pShieldHitType = ARCHLIST->LoadArchetype(data->shieldHitType);
		if (pShieldHitType)
			ARCHLIST->AddRef(pShieldHitType, OBJREFNAME);
	}

	pSparkBlast = ARCHLIST->LoadArchetype("BLAST!!Spark");
	if (pSparkBlast)
		ARCHLIST->AddRef(pSparkBlast, OBJREFNAME);

	{
		DAFILEDESC fdesc="tinnard_fire.anm";
		COMPTR<IFileSystem> objFile;
		//fdesc.lpFileName = objData->animName;
		if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
		{
			damageAnimArch = ANIM2D->create_archetype(objFile);
		}
		else 
		{
			CQFILENOTFOUND(fdesc.lpFileName);
			damageAnimArch =0;
			goto Done;
		}
	}
	
	{
		COMPTR<IFileSystem> file;
		DAFILEDESC fdesc = "smoke.pte";

		if (OBJECTDIR->CreateInstance(&fdesc,file) == GR_OK)
			smoke_archID = ENGINE->create_archetype("smoke.pte",file);
	}

	const char *fname;

	if (data->blinkers.light_script[0])
	{
		blink_arch = CreateBlinkersArchetype(data->blinkers.light_script,archIndex);
		fname = data->blinkers.textureName;
		blinkTex = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, PF_4CC_DAA4);
	}

	switch (pData->missionData.race)
	{
	case M_MANTIS:
		fname = "dmg_mantis.tga";
		break;
	case M_SOLARIAN:
		fname = "dmg_solarian.tga";
		break;
	case M_TERRAN:
	default:
		fname = "dmg_terran.tga";
		break;
	}
	
	damageTexID = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR,DA::TGA,PF_4CC_DAA4);

	switch (pData->missionData.race)
	{
	case M_TERRAN:
		pBuildEffect = ARCHLIST->LoadArchetype("BUILD!!Terran");
		pExtBuildEffect = ARCHLIST->LoadArchetype("BUILD!!Terran");
		break;
	case M_MANTIS:
		pBuildEffect = ARCHLIST->LoadArchetype("BUILD!!Mantis");
		pExtBuildEffect = ARCHLIST->LoadArchetype("BUILD!!Mantis");
		break;
	case M_SOLARIAN:
		pBuildEffect = ARCHLIST->LoadArchetype("BUILD!!Solarian");
		pExtBuildEffect = ARCHLIST->LoadArchetype("BUILD!!Solarian");
		break;
	}
	if (pBuildEffect)
		ARCHLIST->AddRef(pBuildEffect, OBJREFNAME);
	if (pExtBuildEffect)
		ARCHLIST->AddRef(pExtBuildEffect, OBJREFNAME);

	{
		char outName[64];
		strcpy(outName,pData->fileName);
		strlwr(outName);
		char * extenpos = strstr(outName,".3db");
		if (extenpos == 0)
			extenpos = strstr(outName,".cmp");
		if (extenpos)
		{
			strcpy(extenpos,".shield");
			DAFILEDESC fdesc=outName;
			COMPTR<IFileSystem> objFile;
			if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
			{
				smesh = new SMesh;
				if (smesh->load(objFile))
				{
				/*	CollisionMesh *mesh = new CollisionMesh;
					smesh->MakeCollisionMesh(mesh);
					m_extent = new MeshExtent(mesh);
					m_extent->xform.set_identity();*/
					smesh->Sort(Vector(0,0,-1));
				}
				else
				{
					delete smesh;
					smesh = 0;
				}
			}
		}
	}

	result = true;

Done:
	return result;
}
//------------------------------------------------------------------------------//
//
template <class Base> 
PLATFORM_INIT<Base>::~PLATFORM_INIT (void)
{
	if (animArchetype != -1)
		ANIM->release_script_set_arch(animArchetype);
	if (archIndex != -1)
		ENGINE->release_archetype(archIndex);
	if (smoke_archID != -1)
		ENGINE->release_archetype(smoke_archID);

	if (pExplosionType)
		ARCHLIST->Release(pExplosionType, OBJREFNAME);
	if (pShieldHitType)
		ARCHLIST->Release(pShieldHitType, OBJREFNAME);
	if (pSparkBlast)
		ARCHLIST->Release(pSparkBlast, OBJREFNAME);
	if (damageAnimArch)
		delete damageAnimArch;
	if (blink_arch)
		DestroyBlinkersArchetype(blink_arch);
	if (pBuildEffect)
		ARCHLIST->Release(pBuildEffect, OBJREFNAME);
	if (pExtBuildEffect)
		ARCHLIST->Release(pExtBuildEffect, OBJREFNAME);

	TMANAGER->ReleaseTextureRef(blinkTex);
	TMANAGER->ReleaseTextureRef(damageTexID);

	if(ambientEffect)
	{
		EFFECTPLAYER->ReleaseEffect(ambientEffect);
		ambientEffect = NULL;
	}
	//render experiment
//	DeleteMeshRenders(mr,numChildren);
	for (int i=0;i<numChildren;i++)
	{
		mr[i]->Release();
	}
	delete [] mr;

	if (smesh)
		delete smesh;
//	if (m_extent)
//		delete m_extent;
}

//------------------------------------------------------------------------------//
//--------------------------------End TPlatform.h-------------------------------//
//------------------------------------------------------------------------------//

#endif