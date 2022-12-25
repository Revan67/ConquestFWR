//--------------------------------------------------------------------------//
//                                                                          //
//                               KamikazeWing.cpp                            //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/KamikazeWing.cpp 13    10/04/00 8:35p Jasony $
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
#include "MPart.h"
#include "MGlobals.h"
#include "Camera.h"
#include "renderer.h"


#include <mesh.h>
#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h>

#include <stdlib.h>

#define KAMIKAZE	5		// dive into target ship
#define MAX_FIGHTERS_IN_WING 6

struct KamikazeWing : FighterWing
{
	virtual void init (PARCHETYPE pArchetype, PARCHETYPE _pFighterType);

	virtual void KamikazeWing::updateFlightDeck (void);

	virtual void AttackObject (IBaseObject * obj,BOOL32 bSpecialAttack);

	virtual void assembleFlight (U32 numFighters);
};
//---------------------------------------------------------------------------
//
/*KamikazeWing::KamikazeWing (void) 
{
	bayDoorIndex = animIndex = -1;
}
//---------------------------------------------------------------------------
//
KamikazeWing::~KamikazeWing (void)
{
	ANIM->release_script_inst(animIndex);
	animIndex = bayDoorIndex = -1;

	destroyFighters();
}*/
//---------------------------------------------------------------------------
//
void KamikazeWing::init (PARCHETYPE _pArchetype, PARCHETYPE _pFighterType)
{
	const BT_FIGHTER_WING * data = (const BT_FIGHTER_WING *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(data);
	CQASSERT(data->objClass == OC_LAUNCHER);
	CQASSERT(data->type == LC_KAMIKAZE_WING);

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
}
//---------------------------------------------------------------------------
//
void KamikazeWing::AttackObject (IBaseObject * obj,BOOL32 bSpecialAttack)
{
	CQASSERT(bSpecialAttack == 1 && "Kamikaze is a special attack");
	obj->QueryInterface(IBaseObjectID, target, GetPlayerID());
}
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
// refuel, build new fighters, launch ready fighters
//
void KamikazeWing::updateFlightDeck (void)
{
	FighterNode * node = fighterQueue;

	if (--createTime < 0)
		createTime = 0;
	if (--minLaunchTime < 0)
		minLaunchTime = 0;

	//
	// launch fighter wings
	//
	if (bRecallingFighters == 0 && node && minLaunchTime==0 && 
		(target!=0)	)// if it's time to launch a wing
	{
		U32 numAvail=0;	   // number of fighters ready to go
		
		do
		{
			numAvail++;
			node = node->pNext;

		} while (node != fighterQueue);

		//
		// do we have enough fighters to launch a wing?
		//
		if (numAvail >= maxWingFighters)
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
	if (fighterDead && createTime==0)
	{
		if (owner->UseSupplies(costOfNewFighter))		// if supplies were available
			createTime = createPeriod;

		if (createTime)
		{
			fighterDead->fighter->SetFighterSupplies(fighterData->maxSupplies);
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
/*U32 KamikazeWing::getNumFighters (FighterNode * list)
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
}*/
//---------------------------------------------------------------------------
// assumes that number of fighters are available on deck
//
void KamikazeWing::assembleFlight (U32 numFighters)
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
		if (target)
			array[i]->fighter->SetTarget(target);
		array[i]->fighter->SetPatrolState(KAMIKAZE);
	}
}
//---------------------------------------------------------------------------
// tell all airborne fighters about new target
//
/*void KamikazeWing::updateTargetObject (void)
{
	FighterNode * node = fighterPatrol;

	if (node)
	do
	{
		node->fighter->SetTarget(target);
		node = node->pNext;

	} while (node != fighterPatrol);
}*/
//---------------------------------------------------------------------------
//
/*void KamikazeWing::scanForTarget (void)	// parent checks horizon looking for enemy fighters
{
	// see if enough time has gone by, or our target is destroyed
	const MPart& hPart = GetPartHandle();

	if (target==0 || hPart.IsUpdateFrame())
	{
		//
		// if attacking a target outside of patrol range, stop it!
		//
		if (target!=0)
		{
			if (bAutoAttack)
			{
				Vector relVec = target.ptr->GetPosition() - GetPosition();
				SINGLE mag = relVec.magnitude();

				if (mag > fighterData->patrolRadius)
				{
					target = 0;
					updateTargetObject();
				}
			}
		}
		else // look for an enemy fighter
		{
			OBJPTR<IFighter> fighter;
			U32 myPlayerID = hPart->playerID;
			SINGLE minDistance = 1E10;
			IBaseObject *bestObject = 0;
			const Vector ownerPos = GetPosition();
			U32 systemID = GetSystemID();
			IBaseObject * obj = OBJLIST->GetFighterList();

			if (obj && obj->QueryInterface(IFighterID, fighter))
			do
			{
				if (fighter.ptr->GetSystemID() == systemID && 
					fighter->IsLeader() && 
					!MGlobals::AreAllies(fighter->GetPlayerID(), myPlayerID))
				{
					Vector relVec = fighter.ptr->GetPosition() - ownerPos;
					SINGLE mag = relVec.magnitude();

					if (mag < fighterData->patrolRadius)
					{
				 		if (mag < minDistance)
				 		{
				 			bestObject = fighter.ptr;
				 			minDistance = mag;
						}
					}
				}

			} while (fighter->GetNextFighter(fighter) != 0);

			if (bestObject)
			{
				target = bestObject;
				bAutoAttack = 1;
				updateTargetObject();
			}
		}
	}
}*/
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline IBaseObject * createKamikazeWing (PARCHETYPE pArchetype, PARCHETYPE pFighterType)
{
	KamikazeWing * wing = new ObjectImpl<KamikazeWing>;

	wing->init(pArchetype, pFighterType);

	return wing;
}
//------------------------------------------------------------------------------------------
//---------------------------KamikazeWing Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE KamikazeWingFactory : public IObjectFactory
{
	struct OBJTYPE 
	{
		PARCHETYPE pArchetype;
		PARCHETYPE pFighterType;

		void * operator new (size_t size)
		{
			return calloc(size,1);
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

	BEGIN_DACOM_MAP_INBOUND(KamikazeWingFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	KamikazeWingFactory (void) { }

	~KamikazeWingFactory (void);

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

	/* KamikazeWingFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
KamikazeWingFactory::~KamikazeWingFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void KamikazeWingFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE KamikazeWingFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_FIGHTER_WING * data = (BT_FIGHTER_WING *) _data;

		if (data->type == LC_KAMIKAZE_WING)	   
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
BOOL32 KamikazeWingFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * KamikazeWingFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createKamikazeWing(objtype->pArchetype, objtype->pFighterType);
}
//-------------------------------------------------------------------
//
void KamikazeWingFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _kamikazeWing : GlobalComponent
{
	KamikazeWingFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<KamikazeWingFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _kamikazeWing __kfactory;
//---------------------------------------------------------------------------------------------
//-------------------------------End KamikazeWing.cpp-------------------------------------------
//---------------------------------------------------------------------------------------------
