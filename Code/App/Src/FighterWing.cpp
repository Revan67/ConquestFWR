//--------------------------------------------------------------------------//
//                                                                          //
//                               FighterWing.cpp                            //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/FighterWing.cpp 71    5/07/01 9:22a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "FighterWing.h"

#include "SuperTrans.h"
#include "ObjList.h"

#include "Mission.h"
#include "IMissionActor.h"
#include "Startup.h"
#include "ArchHolder.h"
#include "CQTrace.h"
#include "MGlobals.h"
#include "Camera.h"
#include "renderer.h"
#include "MPart.h"
#include <DMBaseData.h>
#include "Sector.h"
#include "UserDefaults.h"
#include "ObjMapIterator.h"
#include "TerrainMap.h"


#include <mesh.h>
#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h>
#include <IRenderPrimitive.h>

#include <stdlib.h>

#define MAX_FIGHTERS_IN_WING 6
//---------------------------------------------------------------------------
//
FighterWing::FighterWing (void) 
{
	bayDoorIndex = animIndex = -1;
	bSpecialAttack = false;
}
//---------------------------------------------------------------------------
//
FighterWing::~FighterWing (void)
{
	ANIM->release_script_inst(animIndex);
	animIndex = bayDoorIndex = -1;

	destroyFighters();
}
//---------------------------------------------------------------------------
//
void FighterWing::init (PARCHETYPE _pArchetype, PARCHETYPE _pFighterType)
{
	const BT_FIGHTER_WING * data = (const BT_FIGHTER_WING *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(data);
	CQASSERT(data->objClass == OC_LAUNCHER);
	CQASSERT(data->type == LC_FIGHTER_WING);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;

	pFighterType = _pFighterType;
	fighterData = (BT_FIGHTER_DATA *) ARCHLIST->GetArchetypeData(pFighterType);

	//
	// assign launcher constants
	//

	maxFighters = data->maxFighters;									// maximum number of fighters in group
	CQASSERT(maxFighters > 0);
	maxWingFighters = data->maxWingFighters;							// maximum number of fighters in wing
	CQASSERT(maxWingFighters>0);
	CQASSERT(maxWingFighters <= 6);
	maxCapFighters = data->maxCapFighters;									// maximum number of fighters in group
	createPeriod = U32(data->refirePeriod * REALTIME_FRAMERATE);		// time to create a new fighter
	minLaunchPeriod = U32(data->minLaunchPeriod * REALTIME_FRAMERATE);	// time between launches of fighter wings
	costOfNewFighter = data->costOfNewFighter;
	costOfRefueling = data->costOfRefueling;		// fixed cost for refueling a fighter that used its weapon
	baseAirAccuracy = data->baseAirAccuracy;
	baseGroundAccuracy = data->baseGroundAccuracy;
	bSpecialWeapon = data->bSpecialWeapon;
}
//---------------------------------------------------------------------------
//
void FighterWing::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	const BT_FIGHTER_WING * data = (const BT_FIGHTER_WING *) ARCHLIST->GetArchetypeData(pArchetype);

	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);

	createFighters();

	if (data->hardpoint[0])
	{
		FindHardpoint(data->hardpoint, bayDoorIndex, hardpointinfo, ownerIndex);
		CQASSERT(bayDoorIndex!=-1 && "Hardpoint not found!");
	}
	else
	{
		hardpointinfo.orientation = Matrix().set_identity();
		hardpointinfo.point = Vector(0,0,0);
		bayDoorIndex = ownerIndex;
	}
	if (data->animation[0])
	{
		animIndex = ANIM->create_script_inst(animArcheIndex, ownerIndex, data->animation);
	 	bayDoorPeriod = ANIM->get_duration(animArcheIndex, data->animation) * REALTIME_FRAMERATE;
		bayDoorClosePeriod = bayDoorPeriod * 4;
	}
	else
		bBayDoorOpen = 1;
}
//---------------------------------------------------------------------------
//
const TRANSFORM & FighterWing::GetTransform (void) const
{
	calculateTransform(hardpointTransform);
	
	return hardpointTransform;
}
//---------------------------------------------------------------------------
//
Vector FighterWing::GetVelocity (void)
{
	return owner.Ptr()->GetVelocity();
}
//---------------------------------------------------------------------------
//
BOOL32 FighterWing::Update (void)
{
	if(bSpecialWeapon)
	{
		MPartNC part(owner.Ptr());
		if(part.isValid())
		{
			if(!(part->caps.specialAttackOk))
			{
				BT_FIGHTER_WING * data = (BT_FIGHTER_WING *)(ARCHLIST->GetArchetypeData(pArchetype));
				if(data->neededTech.raceID)
				{
					if(MGlobals::GetCurrentTechLevel(owner.Ptr()->GetPlayerID()).HasTech(data->neededTech))
					{
						part->caps.specialAttackOk = true;
					}
				}
				else
				{
					part->caps.specialAttackOk = true;
				}
			}
		}
	}

	// takeover is complete when all fighters have returned (or died)
	if (bTakeoverInProgress && fighterPatrol==NULL)
		bTakeoverInProgress = false;

	MPart hShip = owner.Ptr(); 
	bool bWasRecalling = bRecallingFighters;

	if ((bRecallingFighters = hShip->bRecallFighters)==true)
	{
		if (target!=0 && target->IsVisibleToPlayer(GetPlayerID()) && target->GetSystemID() == GetSystemID())
			bRecallingFighters = false;			// ignore this flag if we're busy!
		else
		if (fighterPatrol && bWasRecalling==false)
			recallFighterList(fighterPatrol);
	}

	updateFlightDeck();

	scanForTarget();		// parent checks horizon looking for enemy fighters

	if (fighterPatrol)
		updateFighterList(fighterPatrol);

	if (fighterPatrol)
		setFootprintFighterList(fighterPatrol);

	return 1;
}
//---------------------------------------------------------------------------
//
void FighterWing::PhysicalUpdate (SINGLE dt)
{
	if (fighterPatrol)
		physicalUpdateFighterList(fighterPatrol, dt);
}
//---------------------------------------------------------------------------
//
U32 FighterWing::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
U32 FighterWing::GetSystemID (void) const
{
	return owner.Ptr()->GetSystemID();
}
//---------------------------------------------------------------------------
//
bool FighterWing::GetMissionData (MDATA & mdata) const
{
	return owner.Ptr()->GetMissionData(mdata);
}
//---------------------------------------------------------------------------
//
U32 FighterWing::GetPlayerID (void) const
{
	return owner.Ptr()->GetPlayerID();
}
//---------------------------------------------------------------------------
//
void FighterWing::Render (void)
{
	if (fighterPatrol)
		renderFighterList(fighterPatrol);
}
//---------------------------------------------------------------------------
//
void FighterWing::RevealFog (const U32 currentSystem)
{
	if (fighterPatrol)
		revealFighterList(fighterPatrol, currentSystem);
}
//---------------------------------------------------------------------------
//
void FighterWing::CastVisibleArea (void)
{
	if (fighterPatrol)
		castVisibleFighterList(fighterPatrol);
}
//---------------------------------------------------------------------------
//
void FighterWing::AttackPosition (const struct GRIDVECTOR * pos, bool bSpecial)
{
//	CQBOMB0("AttackPosition() not supported");
}
//---------------------------------------------------------------------------
//
void FighterWing::AttackObject (IBaseObject * obj)
{
	if(!bSpecialWeapon)
	{
		bRecallingFighters = false;
		bAutoAttack = 0;
		if (obj)
		{
			obj->QueryInterface(IBaseObjectID, target, GetPlayerID());
		}
		else
		{
			target = NULL;
		}
		updateTargetObject();
	}
}
//---------------------------------------------------------------------------
//
void FighterWing::SpecialAttackObject (IBaseObject * obj)
{
	if(bSpecialWeapon)
	{
		bSpecialAttack = true;
		bRecallingFighters = false;
		bAutoAttack = 0;
		if (obj)
		{
			obj->QueryInterface(IBaseObjectID, target, GetPlayerID());
		}
		else
		{
			target = NULL;
		}
		updateTargetObject();
	}
	else
	{
		bRecallingFighters = false;
		bAutoAttack = 0;
		if (obj)
		{
			obj->QueryInterface(IBaseObjectID, target, GetPlayerID());
		}
		else
		{
			target = NULL;
		}
		updateTargetObject();
	}
}
//---------------------------------------------------------------------------
//
const bool FighterWing::TestFightersRetracted (void) const
{
	// if we're busy fighting, lie about the retracted state
	return ((target!=0 && target->IsVisibleToPlayer(GetPlayerID()) && target->GetSystemID() == GetSystemID()) || 
		     fighterPatrol==0);
}
//---------------------------------------------------------------------------
//
void FighterWing::SetFighterStance(FighterStance stance)
{
	switch(stance)
	{
	case FS_NORMAL:
		maxCapFighters = 0;
		break;
	case FS_PATROL:
		maxCapFighters = maxWingFighters;
		break;
	}
}
//---------------------------------------------------------------------------
//
void FighterWing::HandlePreTakeover (U32 newMissionID, U32 troopID)
{
	bTakeoverInProgress = true;
	target = 0;
	// don't broadcast to existing fighters while in this mode
}
//---------------------------------------------------------------------------
//
void FighterWing::OnAllianceChange (U32 allyMask)
{
	if (target)
	{
		// we'll want to tell our fighters to stop attacking any new allies
		const U32 hisPlayerID = target->GetPlayerID() & PLAYERID_MASK;
						
		// if we are now allied with the target then stop shooting at him
		if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) != 0))
		{
			target = 0;
			updateTargetObject();
		}
	}
}
//---------------------------------------------------------------------------
//
IBaseObject * FighterWing::FindChildTarget(U32 childID)
{
	return FindFighter(childID);
}
//---------------------------------------------------------------------------
//
S32 FighterWing::GetObjectIndex (void) const
{
	return bayDoorIndex;
}
//---------------------------------------------------------------------------
//
BOOL32 FighterWing::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "FIGHTERWING_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	FIGHTERWING_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	save = *static_cast<FIGHTERWING_SAVELOAD *>(this);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 FighterWing::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "FIGHTERWING_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	FIGHTERWING_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("FIGHTERWING_SAVELOAD", buffer, &load);

	*static_cast<FIGHTERWING_SAVELOAD *>(this) = load;

	if (load.bBayDoorOpen)
		setBayDoorState(1, 0);		// open without animation

	result = 1;

Done:	
	return result;
}

//---------------------------------------------------------------------------
//
void FighterWing::ResolveAssociations()
{
	resolveFighters();
}
//---------------------------------------------------------------------------
// get the transform for the bay doors
//
void FighterWing::calculateTransform (TRANSFORM & result) const
{
	/*
	const Matrix matrix = ENGINE->get_orientation(index);

	pos = matrix * pos;
	pos += ENGINE->get_position(index);

	orientation = matrix * orientation;
	*/

	result.TRANSFORM::TRANSFORM(hardpointinfo.orientation, hardpointinfo.point);
	result = ENGINE->get_transform(bayDoorIndex).multiply(result);
}
//---------------------------------------------------------------------------
//
/*S32 FighterWing::findChild (const char * pathname, INSTANCE_INDEX parent)
{
	S32 index = -1;
	char buffer[MAX_PATH];
	const char *ptr=pathname, *ptr2;
	INSTANCE_INDEX child=-1;

	if (ptr[0] == '\\')
		ptr++;

	if ((ptr2 = strchr(ptr, '\\')) == 0)
	{
		strcpy(buffer, ptr);
	}
	else
	{
		memcpy(buffer, ptr, ptr2-ptr);
		buffer[ptr2-ptr] = 0;		// null terminating
	}

	while ((child = MODEL->get_child(parent, child)) != -1)
	{
		if (MODEL->is_named(child, buffer))
		{
			if (ptr2)
			{
				// found the child, go deeper if needed
				parent = child;
				child = -1;
				ptr = ptr2+1;
				if ((ptr2 = strchr(ptr, '\\')) == 0)
				{
					strcpy(buffer, ptr);
				}
				else
				{
					memcpy(buffer, ptr, ptr2-ptr);
					buffer[ptr2-ptr] = 0;		// null terminating
				}
			}
			else
			{
				index = child;
				break;
			}
		}
	}

	return index;
}
//----------------------------------------------------------------------------------------
//
BOOL32 FighterWing::findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent)
{
	BOOL32 result=0;
	char buffer[MAX_PATH];
	char *ptr=buffer;

	strcpy(buffer, pathname);
	if ((ptr = strrchr(ptr, '\\')) == 0)
		goto Done;

	*ptr++ = 0;
	if (buffer[0])
		index = findChild(buffer, parent);
	else
		index = parent;

Done:	
	if (index != -1)
	{
		HARCH arch = index;

		if ((result = HARDPOINT->retrieve_hardpoint_info(arch, ptr, hardpointinfo)) == false)
			index = -1;		// invalidate result
	}

	return result;
}*/
//---------------------------------------------------------------------------
// one of our fighters was hit by a weapon
//
void FighterWing::OnFighterDestruction (S32 fighterIndex)
{
	moveToList(fighterDead, fighterIndex);
}
//---------------------------------------------------------------------------
//
struct IFighter * FighterWing::GetFighter (S32 fighterIndex)
{
	if (fighterIndex < 0 || fighterPool[fighterIndex].fighter->GetState() != PATROLLING)
		return 0;
	return fighterPool[fighterIndex].fighter;
}
//---------------------------------------------------------------------------
//
void FighterWing::GetLandingClearance (void)
{
	if (bBayDoorOpen==0)
	{
		setBayDoorState(1, 1);		// open with animation
		minLaunchTime = bayDoorPeriod;
		bBayDoorOpen = 1;
		bayDoorCloseTime = bayDoorClosePeriod;
	}
	else
	{
//		minLaunchTime = minLaunchPeriod;
		bayDoorCloseTime = bayDoorClosePeriod;
	}
}
//---------------------------------------------------------------------------
//
void FighterWing::OnFighterLanding (S32 fighterIndex)
{
	moveToList(fighterQueue, fighterIndex);
	if (bBayDoorOpen)
		bayDoorCloseTime = bayDoorClosePeriod;
	if (minLaunchTime <= 0)		// only reset counter if we are waiting to launch right now
		minLaunchTime = minLaunchPeriod;
}
//---------------------------------------------------------------------------
//
struct IBaseObject * FighterWing::FindFighter (U32 fighterMissionID)
{
	for(U32 i = 0; i < maxFighters; ++i)
	{
		if(fighterPool[i].fighter)
		{
			OBJPTR<IBaseObject> obj;
			if(fighterPool[i].fighter->QueryInterface(IBaseObjectID,obj))
			{
				if(obj->GetPartID() == fighterMissionID)
				{
					return obj.Ptr();
				}
			}
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------
//
void FighterWing::setBayDoorState (BOOL32 bOpen, BOOL32 bAnimate)
{
	if (animIndex!=-1)
	{
		if (bOpen)
		{
			if (bAnimate)
				ANIM->script_start(animIndex);
			else
				ANIM->script_start(animIndex, Animation::FORWARD, Animation::END);
		}
		else // close the door
		{
			if (bAnimate)
				ANIM->script_start(animIndex, Animation::BACKWARDS);
			else
				ANIM->script_start(animIndex, Animation::BACKWARDS, Animation::BEGIN);
		}
	}
}
//---------------------------------------------------------------------------
//
void FighterWing::createFighters (void)
{
	U32 numFighters = maxFighters;
	U32 fighterID = 0;
	VOLPTR(IFighter) fighter;

	if (CQFLAGS.bLoadingObjlist==0)	// if not loading from disk, create our part ID now
		fighterID = firstFighterID = MGlobals::CreateSubordinatePartID();

	fighterPool = new FighterNode [maxFighters];

	while (numFighters-- > 0)
	{
		IBaseObject * obj;

		obj = ARCHLIST->CreateInstance(pFighterType);
		fighter = obj;
		fighter->InitFighter(this, numFighters, baseAirAccuracy, baseGroundAccuracy);
		fighterPool[numFighters].fighter = fighter;
		fighterPool[numFighters].index = numFighters;
		if (firstFighterID)
		{
			fighter->SetMissionID(fighterID);
			if (numFighters > 0)	// if not the last time through
				fighterID = MGlobals::CreateSubordinatePartID();
		}
	}

	//
	// set up the circular fighter instance array
	//
	if (maxFighters==1)
	{
		fighterPool[0].pPrev = &fighterPool[0];
		fighterPool[0].pNext = &fighterPool[0];
	}
	else
	if (maxFighters>1)
	{
		U32 i;
		for (i = 1; i < maxFighters-1; i++)
		{
			fighterPool[i].pPrev = &fighterPool[i-1];
			fighterPool[i].pNext = &fighterPool[i+1];
		}

		fighterPool[0].pPrev = &fighterPool[maxFighters-1];
		fighterPool[0].pNext = &fighterPool[1];
		fighterPool[maxFighters-1].pPrev = &fighterPool[maxFighters-2];
		fighterPool[maxFighters-1].pNext = &fighterPool[0];
	}
	//
	// everyone starts in the fighter queue
	//
	fighterQueue = fighterPool;
}
//---------------------------------------------------------------------------
//
void FighterWing::destroyFighters (void)
{
	if (fighterPool)
	{
		U32 numFighters = maxFighters;
		VOLPTR(IBaseObject) obj;
		//
		// add fighters on patrol to the object list
		//
		{
			FighterNode * node = fighterPatrol;		// fighters on patrol/attacking

			if (node)
			do
			{
				obj = node->fighter;
				OBJLIST->AddObject(obj);
				node->fighter = 0;
				node = node->pNext;
			} while (node != fighterPatrol);
		}
		//
		// destroy the rest
		//
		while (numFighters-- > 0)
		{
			if (fighterPool[numFighters].fighter)
			{
				obj = fighterPool[numFighters].fighter;
				delete obj.Ptr();
			}
		}

		delete [] fighterPool;
		fighterPool = 0;
	}
}
//---------------------------------------------------------------------------
//
void FighterWing::resolveFighters (void)
{
	if (fighterPool)
	{
		U32 numFighters = maxFighters;
		U32 fighterID = firstFighterID;
				
		while (numFighters-- > 0)
		{
			fighterPool[numFighters].fighter->SetMissionID(fighterID);
			fighterID += 0x01000000;		// go to next child ID
		}
	}
}
//---------------------------------------------------------------------------
//
void FighterWing::removeFromLists (U32 index)
{
	CQASSERT(index < maxFighters);
	FighterNode * node = &fighterPool[index];
	//
	// remove node from any head pointers
	//
	if (node == fighterQueue)
	{
		if (node->pNext == node)
			fighterQueue = 0;
		else
			fighterQueue = node->pNext;
	}
	else
	if (node == fighterPatrol)
	{
		if (node->pNext == node)
			fighterPatrol = 0;
		else
			fighterPatrol = node->pNext;
	}
	else
	if (node == fighterDead)
	{
		if (node->pNext == node)
			fighterDead = 0;
		else
			fighterDead = node->pNext;
	}
	//
	// remove node from current position
	//
	node->pPrev->pNext = node->pNext;
	node->pNext->pPrev = node->pPrev;
}
//---------------------------------------------------------------------------
// put a fighter on a list
//
void FighterWing::moveToList (FighterNode * & list, U32 index)
{
	CQASSERT(index < maxFighters);
	FighterNode * node = &fighterPool[index];

	removeFromLists(index);
	//
	// put node at the end of the list
	//
	if (list==0)
	{
		list = node->pPrev = node->pNext = node;
	}
	else
	{
		node->pPrev = list->pPrev;
		node->pNext = list;
		list->pPrev->pNext = node;
		list->pPrev = node;
	}
}
//---------------------------------------------------------------------------
//
void FighterWing::updateFighterList (FighterNode * & list)
{
	CQASSERT(list);
	FighterNode * node = list, *next;
	bool bExit=false;

	do
	{
	    next = node->pNext;
		bExit = (next == list);
		node->fighter->Update();
		node = next;

	} while (bExit==false);
}
//---------------------------------------------------------------------------
//
void FighterWing::physicalUpdateFighterList (FighterNode * & list, SINGLE dt)
{
	CQASSERT(list);
	FighterNode * node = list;

	do
	{
		node->fighter->PhysicalUpdate(dt);
		node = node->pNext;
	} while (node!=list);
}
//---------------------------------------------------------------------------
//
void FighterWing::renderFighterList (FighterNode * list)
{
	//The batch render no works like this
	//  step 1 - ask how many different batches there are with GetBatchRenderStateNumber()
	//			this will return the total number of states for this object and it's children
	//  step 2 - loop over the number of states and do ->
	//		2.1 - Call SetupBatchRender(stage) - this will set up the render flags for the batch render.
	//				note - if stage is greater than this objects number of states then it is the state of one of you children
	//		2.2 - loop through all objects calling BatchRender(stage) 
	//		2.3 - Call FinishBatchRender(stage)
	CQASSERT(list);
	FighterNode * node = list;
	BATCH->set_state(RPR_BATCH,FALSE);
	CAMERA->SetModelView();

	const U32 currentSystem = SECTOR->GetCurrentSystem();
	const U32 currentPlayer = MGlobals::GetThisPlayer();
	const USER_DEFAULTS & defaults = *DEFAULTS->GetDefaults();

	bool bFighterIsVisible=false;
	do
	{
		node->fighter->TestVisible(defaults, currentSystem, currentPlayer);
		if (node->fighter->IsVisible())
			bFighterIsVisible = true;
		node = node->pNext;
	} while (node!=list);

	if (!bFighterIsVisible)
		return;

	U32 numRenderState = node->fighter->GetBatchRenderStateNumber();
	for(U32 renderState = 0; renderState < numRenderState ; ++ renderState)
	{
		BATCHDESC desc;
		node = list;
		node->fighter->SetupBatchRender(renderState,desc,MAX_FIGHTERS_IN_WING);
		int fighter_cnt=0;
		do
		{
			node->fighter->BatchRender(renderState,desc);
			node = node->pNext;
			fighter_cnt++;

		} while (node != list);
		node->fighter->FinishBatchRender(renderState,desc,fighter_cnt);
	}

/*	Mesh *mesh = node->fighter->GetMesh();

	BATCH->set_state(RPR_BATCH,FALSE);
//	PIPE->set_render_state(D3DRS_TEXTUREMAPBLEND,D3DTBLEND_DECALALPHA);
	PIPE->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
	PIPE->set_render_state(D3DRS_ZENABLE,TRUE);
//	PIPE->set_texture_stage_texture( 0,mesh->material_list->texture_id);
	SetupDiffuseBlend(mesh->material_list->texture_id,TRUE);
	PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
	PB.Begin(PB_TRIANGLES);
	do
	{
		node->fighter->BatchRender();
		node = node->pNext;

	} while (node != list);
	PB.End();
//	PIPE->set_render_state(D3DRS_TEXTUREMAPBLEND,D3DTBLEND_MODULATEALPHA);
	BATCH->set_state(RPR_BATCH,TRUE);

	//draw engine trails

	IEngineTrail * trail = node->fighter->GetTrail();
	if(trail)
	{
		BATCH->set_state(RPR_BATCH,FALSE);
		PIPE->set_texture_stage_texture( 0,trail->GetTextureID());
		PIPE->set_render_state(D3DRS_ZENABLE,TRUE);
		PIPE->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
//		PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
//		PIPE->set_render_state(D3DRS_FILLMODE,D3DFILL_WIREFRAME );
		PB.Begin(PB_QUADS);
		do
		{
			node->fighter->PostBatchRender();
			node = node->pNext;

		} while (node != list);
		PB.End();
//		PIPE->set_render_state(D3DRS_FILLMODE,D3DFILL_SOLID );
		BATCH->set_state(RPR_BATCH,TRUE);
	}
*/
}

/*void FighterWing::renderFighterList (FighterNode * list)
{
	CQASSERT(list);
	FighterNode * node = list;

	do
	{
		node->fighter->Render();
		node = node->pNext;

	} while (node != list);
}*/
//---------------------------------------------------------------------------
//
void FighterWing::revealFighterList (FighterNode * list, const U32 currentSystem)
{
	CQASSERT(list);
	FighterNode * node = list;

	do
	{
		node->fighter->RevealFog(currentSystem);
		node = node->pNext;

	} while (node != list);
}
//---------------------------------------------------------------------------
//
void FighterWing::setFootprintFighterList (FighterNode *list)
{
	CQASSERT(list);
	FighterNode * node = list;

	do
	{
		node->fighter->SetRadarSigniture();
		node = node->pNext;

	} while (node != list);
}
//---------------------------------------------------------------------------
//
void FighterWing::castVisibleFighterList (FighterNode * list)
{
	CQASSERT(list);
	FighterNode * node = list;

	do
	{
		node->fighter->CastVisibleArea();
		node = node->pNext;

	} while (node != list);
}
//---------------------------------------------------------------------------
//
void FighterWing::recallFighterList (FighterNode * list)
{
	CQASSERT(list);
	FighterNode * node = list;

	do
	{
		node->fighter->ReturnToCarrier();
		node = node->pNext;

	} while (node != list);
}
//---------------------------------------------------------------------------
// assumes target is within range
//
inline bool FighterWing::checkRange (void)
{
	SINGLE RANGE = owner->GetWeaponRange()/GRIDSIZE;
	bool result = (RANGE <= 0);

	if (result==false)
	{
		SINGLE dist = target->GetGridPosition() - owner.Ptr()->GetGridPosition();
		result = (dist <= RANGE);
	}
	return result;
}
//---------------------------------------------------------------------------
// refuel, build new fighters, launch ready fighters
//
void FighterWing::updateFlightDeck (void)
{
	FighterNode * node = fighterQueue;

	if (--createTime < 0)
		createTime = 0;
	if (--minLaunchTime < 0)
		minLaunchTime = 0;

	//
	// launch fighter wings
	//
	if (bRecallingFighters == 0 && node && minLaunchTime==0 && GetSystemID() <= MAX_SYSTEMS && 
		((target!=0 && checkRange()) || getNumFighters(fighterPatrol) < maxCapFighters)&&
		((!bSpecialWeapon) || bSpecialAttack ) && owner.Ptr()->effectFlags.canShoot())	// if it's time to launch a wing
	{
		U32 supplies;
		U32 numAvail=0;	   // number of fighters ready to go
		
		do
		{
			supplies = node->fighter->GetFighterSupplies();
			if (supplies < fighterData->maxSupplies)
			{
				if (costOfRefueling)
				{
					if (owner->UseSupplies(costOfRefueling))		// if supplies were available
						supplies = fighterData->maxSupplies;
					else
						break;
					node->fighter->SetFighterSupplies(supplies);
				}
				else
					node->fighter->SetFighterSupplies(fighterData->maxSupplies);
			}
			
			numAvail++;
			node = node->pNext;

		} while (node != fighterQueue);

		//
		// do we have enough fighters to launch a wing?
		//
		if (numAvail >= maxWingFighters && bTakeoverInProgress==false)
		{
			if (bBayDoorOpen==0)
			{
				setBayDoorState(1, 1);		// open with animation
				minLaunchTime = bayDoorPeriod;
				bBayDoorOpen = 1;
				bayDoorCloseTime = bayDoorClosePeriod;
			}
			else
			{
				bayDoorCloseTime = bayDoorClosePeriod;
				minLaunchTime = minLaunchPeriod;
				assembleFlight(__min(numAvail,maxWingFighters));
			}
 		}
 	}

	//
	// close the door if no one wants out
	//
	if (bayDoorCloseTime>0)
	{
		if (--bayDoorCloseTime == 0)
		{
			setBayDoorState(0, 1);		// close with animation
			bBayDoorOpen = 0;
		}
	}
	//
	// create a fighter if it's time
	//
	if (fighterDead && createTime==0 && createPeriod)
	{
		if (owner->UseSupplies(costOfNewFighter))		// if supplies were available
			createTime = createPeriod;

		if (createTime)
		{
			// don't refuel for free, that would give advantage to leaving fighters behind
//			fighterDead->fighter->SetFighterSupplies(fighterData->maxSupplies);
			moveToList(fighterQueue, fighterDead->index);	// move fighter to the flight deck
		}
		else
		{
			createTime = createPeriod;
		}
	}
}
//---------------------------------------------------------------------------
//
U32 FighterWing::getNumFighters (FighterNode * list)
{
	FighterNode * node = list;
	U32 result = 0;

	if (node)
	do
	{
		node = node->pNext;
		result++;
	} while (node != list);

	return result;
}
//---------------------------------------------------------------------------
// assumes that number of fighters are available on deck
//
void FighterWing::assembleFlight (U32 numFighters)
{
	FighterNode * array[MAX_FIGHTERS_IN_WING+1];
	const TRANSFORM & orientation = GetTransform();
	Vector velocity = GetVelocity();
	U32 i;

	memset(array, 0, sizeof(array));

	for (i = 0; i < numFighters; i++)
	{
		array[i] = fighterQueue;
		moveToList(fighterPatrol, fighterQueue->index);
	}
	
	for (i = 0; i < numFighters; i++)
	{
		array[i]->fighter->LaunchFighter(orientation, velocity, i, array[i]->index, (i>0)?array[i-1]->index:-1, (i+1<numFighters)?array[i+1]->index:-1);
	}

	if (target)
		updateTargetObject();

	if(bSpecialAttack)
		bSpecialAttack = false;
}
//---------------------------------------------------------------------------
// tell all airborne fighters about new target
//
void FighterWing::updateTargetObject (void)
{
	if (bTakeoverInProgress==false)
	{
		FighterNode * node = fighterPatrol;
		IBaseObject * obj = target;

		CQASSERT(bRecallingFighters == 0);

		if (bAutoAttack==0)
		{
			if (obj && (obj->GetSystemID() != GetSystemID() || obj->IsVisibleToPlayer(GetPlayerID())==false) )
				obj = 0;
		}

		if (node)
		do
		{
			if (node->fighter->IsLeader())
				node->fighter->SetTarget(obj);
			node = node->pNext;

		} while (node != fighterPatrol);
	}
}
//---------------------------------------------------------------------------
//
void FighterWing::scanForTarget (void)	// parent checks horizon looking for enemy fighters
{
	// see if enough time has gone by, or our target is destroyed
	if ((target == 0 && bAutoAttack) || MGlobals::IsUpdateFrame(owner.Ptr()->GetPartID()))
	{
		//
		// if attacking a target outside of patrol range, stop it!
		//
		if (target!=0)
		{
			// if recalling fighters, do not adjust the target object anymore!!!
			if (bAutoAttack && bRecallingFighters == 0)
			{
				Vector relVec = target.Ptr()->GetPosition() - GetPosition();
				SINGLE mag = relVec.magnitude();

				if (mag > fighterData->patrolRadius)
				{
					target = 0;
					updateTargetObject();
				}
			}
			else
			{
				if (bRecallingFighters == 0)
					updateTargetObject();
			}
		}
		else // look for an enemy fighter
		if (bRecallingFighters == 0)
		{
			VOLPTR(IFighter) fighter;
			U32 myPlayerID = owner.Ptr()->GetPlayerID();
			SINGLE minDistance = 1E14;
			IBaseObject *bestObject = 0;
			const GRIDVECTOR ownerPos = owner.Ptr()->GetGridPosition();
			U32 systemID = GetSystemID();


			ObjMapIterator iter(systemID,ownerPos,fighterData->patrolRadius*GRIDSIZE);
			
			while (iter)
			{
				if (iter->flags & OM_AIR)
				{	
					fighter = iter->obj;
					if (fighter && fighter->IsLeader())
					{
						if (!MGlobals::AreAllies(iter->dwMissionID & PLAYERID_MASK, myPlayerID))
						{
							SINGLE mag = (fighter.Ptr()->GetGridPosition() - ownerPos);
							
							if (mag < fighterData->patrolRadius)
							{
								if (mag < minDistance)
								{
									bestObject = fighter.Ptr();
									minDistance = mag;
								}
							}
						}
					}
				}
				++iter;
			}
			
			if (bestObject)
			{
				bestObject->QueryInterface(IBaseObjectID, target, GetPlayerID());
				bAutoAttack = 1;
				updateTargetObject();
			}
			else
			if (target==0)
				bAutoAttack = 0;
		}
	}
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline IBaseObject * createFighterWing (PARCHETYPE pArchetype, PARCHETYPE pFighterType)
{
	FighterWing * wing = new ObjectImpl<FighterWing>;

	wing->init(pArchetype, pFighterType);

	return wing;
}
//------------------------------------------------------------------------------------------
//---------------------------FighterWing Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE FighterWingFactory : public IObjectFactory
{
	struct OBJTYPE 
	{
		PARCHETYPE pArchetype;
		PARCHETYPE pFighterType;

		void * operator new (size_t size)
		{
			return calloc(size,1);
		}
		void   operator delete (void *ptr)
		{
			::free(ptr);
		}

		OBJTYPE (void)
		{
		}

		~OBJTYPE (void)
		{
			if (pFighterType)
				ARCHLIST->Release(pFighterType, OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(FighterWingFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	FighterWingFactory (void) { }

	~FighterWingFactory (void);

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

	/* FighterWingFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
FighterWingFactory::~FighterWingFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void FighterWingFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE FighterWingFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_FIGHTER_WING * data = (BT_FIGHTER_WING *) _data;

		if (data->type == LC_FIGHTER_WING)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
 
			result->pFighterType = ARCHLIST->LoadArchetype(data->weaponType);
			CQASSERT(result->pFighterType);
			ARCHLIST->AddRef(result->pFighterType, OBJREFNAME);
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 FighterWingFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * FighterWingFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createFighterWing(objtype->pArchetype, objtype->pFighterType);
}
//-------------------------------------------------------------------
//
void FighterWingFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _fighterwing : GlobalComponent
{
	FighterWingFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<FighterWingFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _fighterwing __factory;
//---------------------------------------------------------------------------------------------
//-------------------------------End FighterWing.cpp-------------------------------------------
//---------------------------------------------------------------------------------------------
