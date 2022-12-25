#ifndef TSPACESHIP_H
#define TSPACESHIP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               TSpaceShip.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TSpaceShip.h 146   8/23/01 1:53p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "TObjMove.h"
#include "TObjFrame.h"
#include "TObjWarp.h"
#include "TObjBuild.h"
#include "TObjSelect.h"
#include "TObjPhys.h"
#include "TObjControl.h"
#include "TObject.h"
#include "IExplosion.h"
#include "IMissionActor.h"
#include "IWeapon.h"
#include "IGotoPos.h"
#include "TObjTrans.h"
#include "TObjExtent.h"
#include "TObjCloak.h"
#include "TObjTeam.h"
#include "TObjGlow.h"
#include "TObjDamage.h"
#include "TObjMission.h"
#include "TObjRepair.h"
#include "TObjEffectTarget.h"
#include "SFX.h"
#include "Anim2d.h"
#include "GenData.h"
#include "IBlinkers.h"
#include "TManager.h"
#include "Sysmap.h"

#include <DSpaceShip.h>

#ifndef __IANIM_H
#include <IAnim.h>
#endif

typedef SPACESHIP_INIT<BASE_SPACESHIP_DATA> BASESHIPINIT;

template <class SaveStruct, class InitStruct>
struct _NO_VTABLE SpaceShip : public ObjectEffectTarget
										<ObjectRepair
											<ObjectGlow
												<ObjectTeam
													<ObjectCloak  
														<ObjectDamage//depends on ObjectBuild
															<ObjectBuild  
																<ObjectExtent
																	<ObjectWarp
																		<ObjectMove
																			<ObjectSelection
																				<ObjectMission
																					<ObjectPhysics
																					<ObjectControl
																						<ObjectTransform
																							<ObjectFrame<IBaseObject,SPACESHIP_SAVELOAD,BASESHIPINIT> >
																						> 
																					> 		
																					>
																				> 
																			> 
																		> 
																	> 
																>
															>
														>
													>
												>
											>
										>, 
										IGotoPos, ISaveLoad, IQuickSaveLoad
{
	BEGIN_MAP_INBOUND(SpaceShip)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IGotoPos)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(ICloak)
	_INTERFACE_ENTRY(IRepairee)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IShipMove)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()

	//----------------------------------
	//
//	SaveNode    saveNode;
//	LoadNode    loadNode;
//	struct UpdateNode  updateNode;
	struct ExplodeNode explodeNode;
	struct RenderNode  renderNode;
	struct InitNode    initNode;
	struct PreTakeoverNode preTakeoverNode;
	struct GeneralSyncNode  genSyncNode;

	typedef SaveStruct SAVEINFO;		// override base typedef
	typedef InitStruct INITINFO;		// override base typedef

	//----------------------------------
	// animation index
	//----------------------------------
	S32 ambientAnimIndex;

	U32 hiliteTex;

	U32 billboardTex;
	U32 billboardTextTwo;

	bool bUpdateOnce;
	
	//
	// explosion data
	//
	PARCHETYPE pExplosionType;
	U32 firstNuggetID;  // mission part ID for first nugget


	// render hint
	U32 hintID;

	// networking fixups
	S32 displaySupplies;		// negative value means "uninitialized"
	S32 displayHull;
	S32 trueNetSupplies;	// last value we sent to clients
	S32 trueNetHull;		// last value we sent to clients
	U32 myKillerOwnerID;	// ID of unit that killed me
	bool bHasHadHullPoints;
	//----------------------------------
	
	SpaceShip (void);

	virtual ~SpaceShip (void);	

	/* IBaseObject methods */

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void DrawFleetMoniker (bool bAllShips);

	virtual void Render (void);

	virtual void MapRender (bool bPing);

	virtual void View (void);

	virtual void DEBUG_print (void) const;

	virtual void RevealFog (const U32 currentSystem);	// call the FogOfWar manager, if appropriate

	virtual void CastVisibleArea (void);				// set visible flags of objects nearby

	virtual void SetReady(bool _bReady);

	virtual bool MatchesSomeFilter(DWORD filter);

	/* IGotoPos methods */

	virtual void GotoPosition (const struct GRIDVECTOR & pos, U32 agentID, bool bSlowMove);

	virtual void PrepareForJump (IBaseObject * jumpgate, bool bUserMove, U32 agentID, bool bSlowMove);

	virtual void UseJumpgate (IBaseObject * outgate, IBaseObject * ingate, const Vector& jumpToPosition, SINGLE heading, SINGLE speed, U32 agentID);

	virtual bool IsJumping (void);

	virtual bool IsHalfSquare();

	virtual void Patrol (const GRIDVECTOR & src, const GRIDVECTOR & dst, U32 agentID);

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations (void);

	/* IQuickSaveLoad methods */
	
	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void);


	/* IExplosionOwner methods */

	virtual void RotateShip (SINGLE relYaw, SINGLE relRoll, SINGLE relPitch, SINGLE altitude);

	virtual void OnChildDeath (INSTANCE_INDEX child);

	virtual U32 GetScrapValue (void);
	
	virtual U32 GetFirstNuggetID (void);

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

	// methods for accessing the "displayed" hullPoints/supplies
	virtual U32 GetDisplayHullPoints (void) const
	{
		return (displayHull>=0)? displayHull : hullPoints;
	}
	virtual U32 GetDisplaySupplies (void) const
	{
		return (displaySupplies>=0)? displaySupplies : supplies;
	}

	virtual void TakeoverSwitchID (U32 newMissionID);

	/* IWeaponTarget methods */
	
	//returns true if a shield hit was created
	virtual BOOL32 ApplyDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit=1);

	//returns true if a shield hit was created
	virtual BOOL32 ApplyVisualDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit=1);

	// move "amount" from the pending pile to the actual. (assumes complex formula has already been used)
	virtual void ApplyAOEDamage (U32 ownerID, U32 damageAmount);

	/* SpaceShip methods */

	void initSpaceShip (const InitStruct & data);

	void explodeSpaceShip (bool bExplode);

	void renderSpaceShip (void);

	void drawBillboardShip (void);

	void updateDisplayValues (void);

	void updateOnce();

	void updateFieldInfo (void);

	void preTakeoverShip (U32 newMissionID, U32 troopID);

	U32 getSyncShipData (void * buffer);

	void putSyncShipData (void * buffer, U32 bufferSize, bool bLateDelivery);

	// override this in derived class, default behavior is to tell the nearest admiral that we are getting damaged
	virtual void broadcastHelpMsg (U32 attackerID);
	
	
	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "SPACESHIP_SAVELOAD";
	}

	virtual void * getViewStruct (void)				// must be overriden implemented by derived class
	{
		CQERROR0("getViewStruct() not be implemented!");
		return 0;
	}

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
	
#define CASTINITPROC(x) castInitProc(InitProc2(x))
#define CASTSAVELOADPROC(x) castSaveLoadProc(SaveLoadProc2(x))
	
	//void FRAME_init (const INITINFO & initStruct)
	void FRAME_init (const InitStruct & _initStruct)
	{
		const BASESHIPINIT * initStruct = (const BASESHIPINIT *) (&_initStruct);
		ObjectFrame<IBaseObject,SPACESHIP_SAVELOAD,BASESHIPINIT>::FRAME_init(*initStruct);
	}

};

bool arch_callback( ARCHETYPE_INDEX parent_arch_index, ARCHETYPE_INDEX child_arch_index, void *user_data );

extern U32 shipMapTex;
extern U32 shipMapTexRef;

//-------------------------------------------------------------------------
// SpaceShip archetype data class, defined in DSpaceship.h
//-------------------------------------------------------------------------
//
template <class BT_TYPE> 
bool SPACESHIP_INIT<BT_TYPE>::loadSpaceshipArchetype (BT_TYPE * _pData, PARCHETYPE _pArchetype)		// load archetype data
{
	bool result=false;

	pData = _pData;
	pArchetype = _pArchetype;

	meshArch = MESHMAN->CreateMeshArch(_pData->fileName);
	
	if(pData->ambientEffect[0])
		ambientEffect = EFFECTPLAYER->LoadEffect(pData->ambientEffect);

	//try to determine the ship's footprint size for usage by ObjGen
	if (ENGINE->is_archetype_compound(archIndex))
	{
		ENGINE->enumerate_archetype_parts(archIndex,arch_callback,&fp_radius);
	}
	else
	{
		float local_box[6];
		REND->get_archetype_bounding_box(archIndex,1.0,local_box);
		fp_radius = __max(local_box[BBOX_MAX_X],-local_box[BBOX_MIN_X]);
		fp_radius = __max(fp_radius,local_box[BBOX_MAX_Z]);
		fp_radius = __max(fp_radius,-local_box[BBOX_MIN_Z]);
		fp_radius *= 2;
	}
	///

	if (pData->explosionType[0])
	{
		pExplosionType = ARCHLIST->LoadArchetype(pData->explosionType);
		CQASSERT(pExplosionType);
		ARCHLIST->AddRef(pExplosionType, OBJREFNAME);
	}

	pSparkBlast = ARCHLIST->LoadArchetype("BLAST!!Spark");
	if (pSparkBlast)
		ARCHLIST->AddRef(pSparkBlast, OBJREFNAME);

/*	if (pData->shieldHitType[0])
	{
		pShieldHitType = ARCHLIST->LoadArchetype(pData->shieldHitType);
		if (pShieldHitType)
			ARCHLIST->AddRef(pShieldHitType);
	}*/
	if (pData->trailType[0])
	{
		pTrailType = ARCHLIST->LoadArchetype(pData->trailType);
		if (pTrailType)
			ARCHLIST->AddRef(pTrailType, OBJREFNAME);
	}

	if (pData->damage.damageBlast[0])
	{
		pDamageBlast = ARCHLIST->LoadArchetype(pData->damage.damageBlast);
		if (pDamageBlast)
			ARCHLIST->AddRef(pDamageBlast, OBJREFNAME);
	}

	if (pData->cloak.cloakEffectType[0])
	{
		pCloakEffect = ARCHLIST->LoadArchetype(pData->cloak.cloakEffectType);
		if (pCloakEffect)
			ARCHLIST->AddRef(pCloakEffect, OBJREFNAME);
	}
		
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
			return 0;
		}
	}

	{
		DAFILEDESC fdesc=pData->shield.animName;
		switch (pData->missionData.race)
		{
		case M_TERRAN:
			fdesc.lpFileName = "SH_Terran.anm";
			break;
		case M_MANTIS:
			fdesc.lpFileName = "SH_Mantis.anm";		
			break;
		default:
		case M_SOLARIAN:
			fdesc.lpFileName = "SH_Celareon.anm";
			break;
		}

		COMPTR<IFileSystem> objFile;
		if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
		{
			shieldAnimArch = ANIM2D->create_archetype(objFile);
		}
		else 
		{
			CQFILENOTFOUND(fdesc.lpFileName);
			shieldAnimArch =0;
			return 0;
		}
	}

	{
		DAFILEDESC fdesc=pData->shield.fizzAnimName;
		COMPTR<IFileSystem> objFile;
		if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
		{
			shieldFizzAnimArch = ANIM2D->create_archetype(objFile);
		}
		else 
		{
			CQFILENOTFOUND(fdesc.lpFileName);
			shieldFizzAnimArch =0;
			return 0;
		}
	}

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
/*					CollisionMesh *mesh = new CollisionMesh;
					smesh->MakeCollisionMesh(mesh);
					m_extent = new MeshExtent(mesh);
					m_extent->xform.set_identity();
					smesh->Sort(Vector(0,0,-1));*/
				}
				else
				{
					delete smesh;
					smesh = 0;
				}
			}
		}
	}

	{
		COMPTR<IFileSystem> file;
		DAFILEDESC fdesc = "smoke.pte";

		if (OBJECTDIR->CreateInstance(&fdesc,file) == GR_OK)
			smoke_archID = ENGINE->create_archetype("smoke.pte",file);
	}


	const char * fname;

	fname = pData->engineGlow.engine_texture_name;
	if (fname[0])
	{
		engineTex = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, PF_4CC_DAA4);
	}

	if (pData->blinkers.light_script[0])
	{
		blink_arch = CreateBlinkersArchetype(pData->blinkers.light_script,archIndex);
		fname = pData->blinkers.textureName;
		if (fname[0])
		{
			blinkTex = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, PF_4CC_DAA4);
		}
	}

	fname = pData->cloak.cloakTex;
	if (fname[0])
	{
		cloakTex = TMANAGER->CreateTextureFromFile(fname,TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
	}

	fname = "comm.tga";
	if (fname[0])
	{
		hiliteTex = TMANAGER->CreateTextureFromFile(fname,TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
	}
	
	fname = pData->billboard.billboardTexName;
	if (fname[0])
	{
		billboardTex = TMANAGER->CreateTextureFromFile(fname,TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
	}

	//	if (cloakTex2 == 0)
	//		cloakTex2 = CreateTextureFromFile("Cloak2.tga",TEXTURESDIR,DA::TGA,PF_4CC_DAA4);
	
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

	// preload some sound effectds

	SFXMANAGER->Preload(pData->shield.sfx);
	SFXMANAGER->Preload(pData->shield.fizzOut);

	if(shipMapTex == -1)
	{
		shipMapTex = SYSMAP->RegisterPlayerIcon("SysMap\\ship.bmp");
	}
	++shipMapTexRef;

	result = true;

	return result;
}
//-------------------------------------------------------------------------
//
template <class Base> 
SPACESHIP_INIT<Base>::~SPACESHIP_INIT (void)
{
	if(meshArch)
	{
		meshArch->Release();
		meshArch = NULL;
	}
	if (animArchetype != -1)
		ANIM->release_script_set_arch(animArchetype);
	if (archIndex != -1)
		ENGINE->release_archetype(archIndex);
	if (smoke_archID != -1)
		ENGINE->release_archetype(smoke_archID);

	if (pExplosionType)
		ARCHLIST->Release(pExplosionType, OBJREFNAME);
	//if (pShieldHitType)
//		ARCHLIST->Release(pShieldHitType);
	if (pTrailType)
		ARCHLIST->Release(pTrailType, OBJREFNAME);
	if (pDamageBlast)
		ARCHLIST->Release(pDamageBlast, OBJREFNAME);
	if (pSparkBlast)
		ARCHLIST->Release(pSparkBlast, OBJREFNAME);


	if (damageAnimArch)
		delete damageAnimArch;
	//shield parts
	if (shieldAnimArch)
		delete shieldAnimArch;
	if (shieldFizzAnimArch)
		delete shieldFizzAnimArch;
	if (smesh)
		delete smesh;
//	if (m_extent)
//		delete m_extent;
	//
	if (blink_arch)
		DestroyBlinkersArchetype(blink_arch);

	TMANAGER->ReleaseTextureRef(engineTex);
	TMANAGER->ReleaseTextureRef(blinkTex);
	TMANAGER->ReleaseTextureRef(cloakTex);
	TMANAGER->ReleaseTextureRef(hiliteTex);
	TMANAGER->ReleaseTextureRef(billboardTex);
	TMANAGER->ReleaseTextureRef(cloakTex2);
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

	--shipMapTexRef;
	if(!shipMapTexRef)
		shipMapTex = -1;
}

//---------------------------------------------------------------------------
//------------------------End TSpaceShip.h-----------------------------------
//---------------------------------------------------------------------------

#endif