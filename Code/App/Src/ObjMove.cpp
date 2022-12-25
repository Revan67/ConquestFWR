//--------------------------------------------------------------------------//
//                                                                          //
//                              ObjMove.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ObjMove.cpp 55    1/15/02 10:10a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObjMove.h"
#include "TObjControl.h"
#include "TObjPhys.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "TObjSelect.h"
#include "TObjMission.h"
#include "UserDefaults.h"
#include "Scripting.h"

#include <DSpaceship.h>
#include <DShipSave.h>

#include <IAnim.h>

typedef SPACESHIP_INIT<BASE_SPACESHIP_DATA> BASESHIPINIT;

// TODO:  OnMasterChange():  If moveActive() and pathlength==0, complete the move.
// TODO:  Move Patrol to a separate sync function
// TODO:  no autoattack for gunboat when bRecallFighters==true

//--------------------------------------------------------------------------//
//
template ObjectMove<ObjectSelection
					<ObjectMission
					 <ObjectPhysics
					  <ObjectControl
					   <ObjectTransform
					     <ObjectFrame<struct IBaseObject,struct SPACESHIP_SAVELOAD, BASESHIPINIT> >
					   >
					  > 
					 >
					> 
				   >;

//----------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectMove<Base>::SetTerrainFootprint (struct ITerrainMap * terrainMap)
{
	FootprintHistory footprint;
	//
	// calculate new footprint
	//
	if (bReady && systemID<=MAX_SYSTEMS)
	{
		footprint.info[0].flags = footprint.info[1].flags = bHalfSquare ? TERRAIN_HALFSQUARE : TERRAIN_FULLSQUARE; 
		footprint.info[0].height = footprint.info[1].height = box[2];		// maxy
		footprint.info[0].missionID = footprint.info[1].missionID = dwMissionID;
		footprint.systemID = systemID;

		if (isMoveActive())
		{
			// took out the check for autoMovementEnabled because ships were setting the parked flag
			// at in-appropriate times
			// we may find that big ships are trying to take the spot of a gunboat that is rotating
			// let's wait and see...
			if (bMockRotate)
			{
				footprint.info[0].flags |= (TERRAIN_PARKED|TERRAIN_UNITROTATING);
			}
			else
			{
				footprint.info[0].flags |= TERRAIN_MOVING;
			}

			if (bFinalMove && bPathOverflow == 0)
			{
				footprint.info[1].flags |= (TERRAIN_PARKED | TERRAIN_DESTINATION);
			}
			else
			{
				footprint.info[1].flags |= TERRAIN_DESTINATION;
			}

			// set the grid vector for the footprintf
			footprint.vec[0] = transform.translation;
			footprint.vec[1] = pathList[0];
			footprint.numEntries = 2;
		}
		else
		{
			footprint.info[0].flags |= TERRAIN_PARKED;
			footprint.vec[0] = GetGridPosition();
			footprint.numEntries = 1;
		}
	}

	//
	// send information if different
	//
	if (footprint != footprintHistory)
	{
		// update the scripting engine BEFORE the terrain map really changes
		scriptingUpdate( footprintHistory, footprint );

		undoFootprintInfo(terrainMap);

		if (footprint.numEntries>0)
		{
			terrainMap->SetFootprint(&footprint.vec[0], 1, footprint.info[0]);
		}
		
		if (footprint.numEntries>1)
		{
			terrainMap->SetFootprint(&footprint.vec[1], 1, footprint.info[1]);
		}
		
		footprintHistory = footprint;
	}
	
	// always update the OBJMAP node, even if not updating the terrain footprint!
	// e.g. (ship being build, or derelict)

	updateObjMap();
}
//---------------------------------------------------------------------------
// terrainMap points to the map for the current system, which might be different than the history
//
template <class Base>
void ObjectMove<Base>::undoFootprintInfo (struct ITerrainMap * terrainMap)
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

	//if this code is ever removed, change RemoveFromMap
	if (OBJMAP && map_sys && map_sys <= MAX_SYSTEMS)
	{
		OBJMAP->RemoveObjectFromMap(this,map_sys,map_square);
		objMapNode = 0;
		map_sys = map_square = 0;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
bool ObjectMove<Base>::testPassible (const struct GRIDVECTOR & pos)
{
	TestPassibleCallback callback;
	COMPTR<ITerrainMap> map;
	bool result;

	SECTOR->GetTerrainMap(systemID, map);

	if ((result=map->TestSegment(GetGridPosition(), pos, &callback)) == false)
		result = callback.gridPos.isMostlyEqual(pos);

	return result;
}
//---------------------------------------------------------------------------
//
static bool bUpdateMoveState = true;

template <class Base> 
BOOL32 ObjectMove<Base>::updateMoveState (void)
{
	if(!bUpdateMoveState)
		return 1;
	bool bNoDynamics = 0;
	bool bOrigAutoMovement = isAutoMovementEnabled();

	if (overrideMode != OVERRIDE_NONE)
	{
		IBaseObject * obj = OBJLIST->FindObject(overrideAttackerID);
		if (obj == 0 || obj->GetSystemID() != systemID)
		{
			if (overrideMode == OVERRIDE_PUSH)
				cancelPush();
			overrideAttackerID = 0;
			overrideMode = OVERRIDE_NONE;
		}
	}

	if (bMoveDisabled==0 && effectFlags.canMove())
	{
		if (isAutoMovementEnabled() && bReady)
		{
			bNoDynamics=0;
			//
			// set the correct velocity
			//
			if (bExploding==0)
			{
				restoreDynamicsData();

				SINGLE maxV = maxVelocity;
				
				if(slowMove && (moveAgentID || jumpAgentID)) //find the slowest guy in the group.
				{
					const ObjSet * set = NULL;
					if(moveAgentID)
						THEMATRIX->GetOperationSet(moveAgentID,&set);
					else
						THEMATRIX->GetOperationSet(jumpAgentID,&set);
					if(set)
					{
						for(U32 i = 0; i < set->numObjects; ++i)
						{
							IBaseObject * obj = OBJLIST->FindObject(set->objectIDs[i]);
							if(obj)
							{
								MPart part(obj);
								if(part.isValid())
								{
									if(part->maxVelocity < maxV)
										maxV = part->maxVelocity;
								}
							}
						}
					}
				}

				SINGLE fleetMod = 1.0;
				if(fleetID)
				{
					VOLPTR(IAdmiral) flagship;
					OBJLIST->FindObject(fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
					if(flagship.Ptr())
					{
						MPart part(this);
						fleetMod = 1 + flagship->GetSpeedBonus(mObjClass, part.pInit->armorData.myArmor);
					}				
				}
				SINGLE sectorMod = 1.0 + SECTOR->GetSectorEffects(playerID,systemID)->getSpeedMod();
				setCruiseSpeed(maxV*completionModifier()*fieldFlags.getSpeedModifier()*effectFlags.getSpeedModifier()*fleetMod*sectorMod);

				if (isMovingToJump())
					doJumpPreparation();
				else
				if (isMoveActive())
					doPathMove();
				else
				{
					if (bVisible && LODPERCENT != 0)
						rockTheBoat();
					else
					if (overrideMode == OVERRIDE_NONE)
					{
						bNoDynamics=1;
						velocity.zero();
						ang_velocity.zero();
						DEBUG_resetInputCounter();
					}
					else
					{
						rotateShip(0, 0-transform.get_roll(), 0-transform.get_pitch());
						setAltitude(0);
					}
				}
			}
		}
		else
		if (bReady)				// let rotates happen when not on screen
		{
			restoreDynamicsData();
			bNoDynamics=0;
		}
		else
		{
			DEBUG_resetInputCounter();
		}
	}
	else if(((!effectFlags.bDestabilizer) && (effectFlags.bStasis)) || bMoveDisabled) // if (hPart->moveAbility==0)
	{
		bNoDynamics=1;
		velocity.zero();
		DEBUG_resetInputCounter();
		if(effectFlags.canMove())
			ang_velocity.zero();
		if (effectFlags.canMove() && goalPosition!=currentPosition && THEMATRIX->IsMaster()==0)
		{
			SetPosition(goalPosition, systemID);
			currentPosition = goalPosition;
		}
	}

	if (bOrigAutoMovement && bReady && bMoveDisabled==0 && overrideMode != OVERRIDE_NONE)
	{
		switch (overrideMode)
		{
		case OVERRIDE_PUSH:
			restoreLinearDynamicsData();
			setAbsOverridePosition(overrideDest, overrideSpeed);
			bNoDynamics = 0;
			break;
		case OVERRIDE_DESTABILIZE:
			setDestabilize();
			bNoDynamics = (bVisible==0 || LODPERCENT == 0);
			break;
		case OVERRIDE_ORIENT:
			overrideAbsShipRotate(overrideYaw, 0, 0);
			bNoDynamics = 0;
			break;
		}
	}

	bEnablePhysics = (bReady && bNoDynamics==0);
	if (bEnablePhysics && !bExploding)
	{
		// update the map with our latest location
		// we may have moved outside our square because of various external forces,
		// or because we are pathfinding to a new location.
		COMPTR<ITerrainMap> map;
		SECTOR->GetTerrainMap(systemID, map);
		if (map)
			SetTerrainFootprint(map);
	}

	bAutoMovement = true;		// always comes back on by default, must be turned off every frame

	return 1;
}
//---------------------------------------------------------------------------
//
static inline void getVFXPoints2 (VFX_POINT points[2], const TRANSFORM & transform, SINGLE distance, SINGLE b5)
{
	//
	// calculate points (in clock-wise order) for terrain foot-pad system
	//
	Vector pt, wpt;

	pt.y = 0;
	pt.x = 0;		// _box[0];	// maxx
	pt.z = b5 - distance;		//box[5];	// minz

	wpt = transform.rotate_translate(pt);
	points[0].x = wpt.x;
	points[0].y = wpt.y;

	pt.z = distance;		//box[4];	// maxz

	wpt = transform.rotate_translate(pt);
	points[1].x = wpt.x;
	points[1].y = wpt.y;
}
//--------------------------------------------------------------------------//
// actual movement code
//--------------------------------------------------------------------------//
//
template <class Base> 
bool ObjectMove<Base>::doMove (const Vector & goalPos)
{
	Vector pos = transform.get_position();
	SINGLE relYaw, relRoll, relPitch;
	SINGLE yaw   = transform.get_yaw();
	SINGLE roll  = transform.get_roll();
	SINGLE pitch = transform.get_pitch();
	bool bContinuing = (bPathOverflow || pathLength > 1);
	Vector goal=goalPos;

	bRotatingBeforeMove = false;

	goal -= pos;

	goal.z = 0;
	const SINGLE goalMag = goal.magnitude();

	relYaw = fixAngle(get_angle(goal.x, goal.y) - yaw);

#if 1
	if (goalMag < boxRadius)
	{
		bool result;

		relRoll = -roll;
		goal.z = cruiseDepth - transform.translation.z;
		if (bContinuing)
			result = (setPosition(goal, getDynamicsData().maxLinearVelocity) != 0);
		else
			result = (setPosition(goal) != 0);
		
		SINGLE sidleDist = dot_product(goal, transform.get_i());
		if (fabs(sidleDist) > getDynamicsData().maxLinearVelocity)
		{
			if (sidleDist < 0)
				relRoll -= 5.0 * MUL_DEG_TO_RAD;
			else
			if (sidleDist > 0)
				relRoll += 5.0 * MUL_DEG_TO_RAD;
		}

		goal.z = 0;
		if (goalMag < box[4]-box[5])		// don't spin if very near goal
			relYaw = 0;
		rotateShip(relYaw, relRoll, 0 - pitch);
		return result;
	}
#endif

	if (fabs(relYaw) < 60.0 * MUL_DEG_TO_RAD)
	{
		if (bContinuing)
		{
			setPosition(goal, maxLinearVelocity);
			setThrustersOn();
		}
		else
		{
			SINGLE coast;

			coast = calculateCoastingDistance(getDynamicsData().maxLinearVelocity, getDynamicsData().linearAcceleration, ELAPSED_TIME);

			if (coast < goalMag)	// not there yet
			{
				setPosition(goal, maxLinearVelocity);
				setThrustersOn();
			}
			else
			{
				setPosition(goal);
			}
		}

		setAltitude(cruiseDepth - transform.translation.z);
		if (cruiseDepth - transform.translation.z < -800)
			relPitch = (-10.0 * MUL_DEG_TO_RAD) - pitch;	// pitch down
		else
			relPitch = 0 - pitch;
	}
	else
	{
		if (bVisible)
		{
			Vector diff=velocity/2;
			diff.z = cruiseDepth - transform.translation.z;

			setPosition(diff);
		}
		else
			setAltitude(cruiseDepth - transform.translation.z);
		relPitch = (5.0 * MUL_DEG_TO_RAD) - pitch;
		bRotatingBeforeMove = true;
	}

	relRoll = 0 - roll;
	if (relRoll < -PI)
		relRoll += PI*2;
	else
	if (relRoll > PI)
		relRoll -= PI*2;

	if (fabs(relYaw) > 10.0 * MUL_DEG_TO_RAD)
	{
		if (relYaw < 0)
			relRoll -= 5.0 * MUL_DEG_TO_RAD;	// roll to the left
		else
			relRoll += 5.0 * MUL_DEG_TO_RAD;	// roll to the right
	}

	rotateShip(relYaw, relRoll, relPitch);

	if (goalMag < -box[5])
	{
		// return early if we are not at the last stop (cut corners)
		if (pathLength>1 || bPathOverflow || velocity.magnitude() < 5.0)
			return true;
	}
	return false;
}			  
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove<Base>::onPathComplete (void)
{
	if (moveAgentID==0 || THEMATRIX->IsMaster())
	{
		if (moveAgentID)
		{
			THEMATRIX->SendOperationData(moveAgentID, dwMissionID, &currentPosition, sizeof(currentPosition));
			THEMATRIX->OperationCompleted2(moveAgentID, dwMissionID);
			bSyncNeeded = 0;
		}
		GRIDVECTOR savedVec = currentPosition;
		resetMoveVars();
		currentPosition = savedVec;		// restore it
		cruiseDepth = 0;
	}
	pathLength = 0;
}
//---------------------------------------------------------------------------
//
template <class Base> 
bool ObjectMove<Base>::fleetRotationCheck (void)
{
	if(fleetID)
	{
		VOLPTR(IAdmiral) flagship;
		OBJLIST->FindObject(fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
		if(flagship.Ptr())
		{
			if(flagship->IsInLockedFormation())
			{
				flagship->MoveDoneHint(this);
				bMockRotate = true;
				bFinalMove = 1;
				COMPTR<ITerrainMap> map;

				SECTOR->GetTerrainMap(systemID, map);
				GRIDVECTOR from = GetGridPosition();
				SetPath(map, &from, 1);

				// calculate the mock rotate angle
				Vector vecDiff = flagship->GetFormationDir();
				mockRotationAngle =  TRANSFORM::get_yaw(vecDiff);

				return false;
			}
		}
	}
	return true;
}
//---------------------------------------------------------------------------
//
template <class Base> 
bool ObjectMove<Base>::doPathMove (void)
{
	if (isMoveActive() && pathLength >= 1)
	{
		CQASSERT(pathLength >= 1);
		bool bNext = ((bPathOverflow || pathLength > 1) && pathList[pathLength-1].isMostlyEqual(GRIDVECTOR::Create(transform.translation)));
		if (bMockRotate)
		{
			// rotate to position, if already there call onPathComplete
			SINGLE relYaw = mockRotationAngle - transform.get_yaw();
			Vector relPos = pathList[pathLength-1]+slopOffset;
			relPos -= transform.translation;

			if (relYaw < -PI)
				relYaw += PI*2;
			else
			if (relYaw > PI)
				relYaw -= PI*2;
			
			BOOL32 bMoveResult = setPosition(relPos);

			if (bMoveResult && fabs(relYaw) <  MUL_DEG_TO_RAD * 20)
			{
				onPathComplete();
			}
			else
			{
				rotateShip(relYaw, 0, 0);
			}
			
		}
		else if (doMove(pathList[pathLength-1]+slopOffset) || bNext)
		{
			currentPosition = pathList[--pathLength];

			if (pathLength == 0 && bFinalMove)
			{
				if(fleetRotationCheck())
					onPathComplete();
			}
			else
			if (pathLength<=1 && (bFinalMove==false||bPathOverflow))
			{
				COMPTR<ITerrainMap> map;
				GRIDVECTOR from = GetGridPosition();

				SECTOR->GetTerrainMap(systemID, map);
				U32 flags = bHalfSquare ? TERRAIN_FP_HALFSQUARE : TERRAIN_FP_FULLSQUARE; 
				if (bPathOverflow==0)	// did we receive the full path last time?
					bFinalMove = 1;
				else
					calcSlopOffset();
				if (bFinalMove)
					flags |= TERRAIN_FP_FINALPATH;

				#if (defined(_JASON) || defined(_SEAN))
					bool bParked = map->IsParkedAtGrid(from, dwMissionID, bHalfSquare==0);
				#endif

				if (map->FindPath(from, goalPosition, dwMissionID, flags, this) == 0)
				{
					if(fleetRotationCheck())
						onPathComplete();

					#if (defined(_JASON) || defined(_SEAN))
					if (bParked == false)
						CQBOMB1("TerrainMap returned pathlength==0, but \"%s\" isn't parked there! (Ignorable)", (char *)partName);
					#endif
		//			CQASSERT(bNext==false);		// we ended our move early, cannot exit path here!
				}
			}
		}
	}
	else
	{
		// on the client, we may complete the move before the host, so pathLength may be zero for a time
		if (isMoveActive() && pathLength==0 && THEMATRIX->IsMaster())	// can happen after master change
			onPathComplete();
		else
		{
			rotateShip(0,0,0);
			setAltitude(0);
		}
	}
	return isMoveActive()==0;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove<Base>::doJumpPreparation (void)
{
	// force ship to stay put, by default
	rotateShip(0,0,0);
	setAltitude(0);
	// check bRecallFighters here, so that we don't actually move to a grid
	// position while we are waiting to assemble.
	bool bMoveResult = (isMoveActive() == 0) || (bRecallFighters == true) || doPathMove();

	if ((bRecallFighters == false) && (bMoveResult == false))
	{
		// first stage, moving towards jumpgate
		// do a distance check and a line-of-sight
		GRIDVECTOR gridpos = GetGridPosition();
		SINGLE fdist = gridpos - jumpToPosition;
		if (fdist < __min(3, sensorRadius))
		{
			// we are less than 3 grid units away
			if (testPassible(jumpToPosition) == true)
			{
				resetMoveVars();
				bRecallFighters = true;
			}
		}
	}
	else if (bMoveResult == false)
	{
		// second stage, moving to a settling place
		// nothing yet...
	}
	else 	// not moving into position
	{
		if (testPassible(jumpToPosition) == false)
		{
			if (THEMATRIX->IsMaster())
			{
				resetMoveVars();
				THEMATRIX->SendOperationData(jumpAgentID, dwMissionID, &currentPosition, sizeof(currentPosition));
				THEMATRIX->OperationCompleted2(jumpAgentID, dwMissionID);
				bSyncNeeded = 0;
				bRecallFighters = false;
				THEMATRIX->FlushOpQueueForUnit(this);
				if (CQFLAGS.bTraceMission)
					CQTRACE11("Jump failed for unit \"%s\"", (char *)partName);
			}
		}
		else
		{
			bRecallFighters = true;

			GRIDVECTOR goal_pos  = jumpToPosition;

			if (goal_pos.cornerpos() - GetGridPosition() < 1)
			{
				// too close to rotate
				bMoveResult = true;
			}
			else
			{
				Vector goal = goal_pos.cornerpos() - transform.translation;
				SINGLE relYaw = get_angle(goal.x, goal.y) - transform.get_yaw();
				if (relYaw < -PI)
					relYaw += PI*2;
				else
				if (relYaw > PI)
					relYaw -= PI*2;

				rotateShip(relYaw, 0 - transform.get_roll(), 0 - transform.get_pitch());

				bMoveResult = (fabs(relYaw) < 15 * MUL_DEG_TO_RAD);
			}
			if (bMoveResult)
				bMoveResult = testReadyForJump();

			if (bMoveResult && THEMATRIX->IsMaster())
			{
				resetMoveVars();
				THEMATRIX->SendOperationData(jumpAgentID, dwMissionID, &currentPosition, sizeof(currentPosition));
				THEMATRIX->OperationCompleted2(jumpAgentID, dwMissionID);
				bSyncNeeded = 0;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
template <class Base> 
ObjectMove<Base>::~ObjectMove (void)
{
	if (footprintHistory.numEntries>0)
	{
		COMPTR<ITerrainMap> map;

		if (SECTOR)
		{
			SECTOR->GetTerrainMap(systemID, map);
			if (map)
				undoFootprintInfo(map);
		}
	}
	if (OBJMAP && map_sys && map_sys <= MAX_SYSTEMS)
		OBJMAP->RemoveObjectFromMap(this,map_sys,map_square);
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove<Base>::rockTheBoat (void)
{
	SINGLE relRoll, relAlt;

	SINGLE roll  = transform.get_roll();
	SINGLE pitch = transform.get_pitch();

	if (bRollTooHigh)
	{
		restoreDynamicsData();

		bEnablePhysics = 1;
		bool rotresult = rotateShip(0, 0 - roll, 0 - pitch);
		if (++tooHighCounter > U8(REALTIME_FRAMERATE+1) && rotresult)
			bRollTooHigh = 0;
		/*
		if (bAltUp)
			relAlt = rockingData.rockLinearMax - transform.translation.z + getCruiseDepth();
		else
			relAlt = -rockingData.rockLinearMax - transform.translation.z + getCruiseDepth();
		*/
		relAlt = 0;
		setAltitude(relAlt);
	}
	else
	{
		if (bRollUp)
			relRoll = rockingData.rockAngMax - roll;
		else
			relRoll = -rockingData.rockAngMax - roll;

		if (bAltUp)
			relAlt = rockingData.rockLinearMax - transform.translation.z;
		else
			relAlt = -rockingData.rockLinearMax - transform.translation.z;

		relRoll = fixAngle(relRoll);

		setDynamicsData(rockingData);
		if (fabs(ang_velocity.z) > 10*MUL_DEG_TO_RAD || fabs(roll) > rockingData.rockAngMax*2 || fabs(pitch) > rockingData.rockAngMax*2)
			restoreAngDynamicsData();

		rotateShip(0, relRoll, 0 - pitch);
		if (fabs(relRoll) < rockingData.rockAngMax*0.1)
			bRollUp = !bRollUp;
		setAltitude(relAlt);
		if (fabs(relAlt) < rockingData.rockLinearMax*0.1)
			bAltUp = !bAltUp;
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove<Base>::moveToPos (const GRIDVECTOR & pos, U32 agentID, bool bSlowMove)
{
	CQASSERT(bExploding == false || agentID == false);
	if(bExploding)
		return;

	COMPTR<ITerrainMap> map;

#ifndef FINAL_RELEASE
#pragma message ("commented out for demo, put this back in after to fix basalisk teleport bug")
//	if (moveAgentID!=0)
//		CQBOMB1("Invalid call to moveToPos() for part=%s (Ignorable)", (char *)partName);	// should already be completed before this call
#endif
	SECTOR->GetTerrainMap(systemID, map);
	pathLength = 0;
	goalPosition = pos;
	U32 flags = bHalfSquare ? TERRAIN_FP_HALFSQUARE : TERRAIN_FP_FULLSQUARE; 
	GRIDVECTOR from = GetGridPosition();

	if (bHalfSquare==0)
		goalPosition.centerpos();
	else
		goalPosition.quarterpos();

#if (defined(_JASON) || defined(_SEAN))
	bool bParked = map->IsParkedAtGrid(from, dwMissionID, bHalfSquare==0);
#endif
	bFinalMove = 0;
	bMoveActive=1;

	if (map->FindPath(from, goalPosition, dwMissionID, flags, this) == 0)
	{	
#if (defined(_JASON) || defined(_SEAN))
		if (bParked == false)
			CQBOMB1("TerrainMap returned pathlength==0, but \"%s\" isn't parked there! (Ignorable)", (char *)partName);
#endif
		//
		// if the user asked us to move, then sidle over a bit
		//

		if (agentID && (GRIDVECTOR(g_trueAnimPos.gridVec) == pos) && (g_trueAnimPos.gridVec.systemID == systemID))
		{
			bMockRotate = true;
			SetPath(map, &from, 1);
			bFinalMove = 1;

			bool bFoundMockAngle = false;

			if(fleetID)
			{
				VOLPTR(IAdmiral) flagship;
				OBJLIST->FindObject(fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
				if(flagship.Ptr())
				{
					if(flagship->IsInLockedFormation())
					{
						flagship->MoveDoneHint(this);
						// calculate the mock rotate angle
						Vector vecDiff = flagship->GetFormationDir();
						mockRotationAngle =  TRANSFORM::get_yaw(vecDiff);
						bFoundMockAngle = true;
					}
				}
			}
			
			if(!bFoundMockAngle)
			{
				// calculate the mock rotate angle
				Vector vecDiff = g_trueAnimPos.pos - transform.translation;
				mockRotationAngle =  TRANSFORM::get_yaw(vecDiff);
			}

			//
			// calc slop offset based on where the user clicked
			//
			slopOffset = g_trueAnimPos.pos - transform.translation;
			slopOffset.fast_normalize();
			slopOffset *= ((rand() & 127) + (rand() & 127)) * maxMoveSlop * (1.0 / 256.0);
		}
		else
		{
			moveAgentID = agentID;
			onPathComplete();
			return;
		}
	}
	else
	{
		bSyncNeeded=true;
		calcSlopOffset();
		
#if (defined(_JASON) || defined(_SEAN))
		if (pathLength==1 && bParked && pathList[0] == from)
			CQBOMB1("TerrainMap returned pathlength==1, but \"%s\" is ALREADY parked at destination! (Ignorable)", (char *)partName);
#endif
	}

	slowMove = bSlowMove;
	moveAgentID = agentID;
	setCruiseSpeed(0);			// set it back to default
	setForwardAcceleration(0);	// set it back to default
	setRandomCruiseDepth();
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove<Base>::moveToJump (IBaseObject * jumpgate, U32 agentID, bool bSlowMove)
{
	jumpToPosition = jumpgate->GetPosition();
	jumpAgentID = agentID;
	moveToPos(jumpToPosition);
	slowMove = bSlowMove;

	bRecallFighters = false;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove< Base >::loadMoveState (MOVESAVEINFO & load)
{
	*static_cast<SPACESHIP_SAVELOAD::TOBJMOVE *> (this) = load.tobjmove;
	DYNAMICS_DATA dyn = getDynamicsData();
	if (cruiseSpeed)
		dyn.maxLinearVelocity = cruiseSpeed;
	else
		dyn.maxLinearVelocity = cruiseSpeed = maxVelocity;
	if (groupAcceleration)
		dyn.angAcceleration = groupAcceleration;
	else
		groupAcceleration = origAcceleration;

	setDynamicsData(dyn);
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove< Base >::saveMoveState (MOVESAVEINFO & save)
{
	save.tobjmove = *static_cast<SPACESHIP_SAVELOAD::TOBJMOVE *> (this);
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove< Base >::initMoveState (const MOVEINITINFO & data)
{
	rockingData = data.pData->rockingData;
	if (rockingData.maxLinearVelocity==0)
		rockingData.maxLinearVelocity = DEF_ROCK_LINEAR_MAX;
	if (rockingData.maxAngVelocity==0)
		rockingData.maxAngVelocity = DEF_ROCK_ANG_MAX;

	bRollUp = ((rand() & 1) == 0);
	bAltUp = ((rand() & 1) == 0);
	DYNAMICS_DATA dyn = getDynamicsData();

	cruiseSpeed = dyn.maxLinearVelocity = maxVelocity = data.pData->missionData.maxVelocity;
	groupAcceleration = origAcceleration = dyn.linearAcceleration;
	setDynamicsData(dyn);

	if ((maxMoveSlop = HALFGRID/2 - boxRadius) >= 0)
		bHalfSquare = true;
	else
	{
		bHalfSquare = false;
		if ((maxMoveSlop += HALFGRID/2) < 0)
			maxMoveSlop = 0;
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove< Base >::onOpCancel (U32 agentID)
{
	if(agentID == moveAgentID)
	{
		moveAgentID = 0;
	}
	if (agentID == jumpAgentID)
	{
		jumpAgentID = 0;
	}
	bRecallFighters = false;
	bPatroling = false;

	if (isMoveActive() && THEMATRIX->IsMaster())
	{
		GRIDVECTOR vec;
		vec = transform.translation+(velocity/2);
		moveToPos(vec);		// end in a predictable location
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove< Base >::preTakeover (U32 newMissionID, U32 troopID)
{
	if(moveAgentID)
	{
		if (THEMATRIX->IsMaster())
			THEMATRIX->SendOperationData(moveAgentID, dwMissionID, NULL, 0);
		THEMATRIX->OperationCompleted2(moveAgentID,dwMissionID);
	}
	if (jumpAgentID)
	{
		if (THEMATRIX->IsMaster())
			THEMATRIX->SendOperationData(jumpAgentID, dwMissionID, NULL, 0);
		THEMATRIX->OperationCompleted2(jumpAgentID,dwMissionID);
	}

	bRecallFighters = false;
	bPatroling = false;

	if (isMoveActive() && THEMATRIX->IsMaster())
	{
		GRIDVECTOR vec;
		vec = transform.translation+(velocity/2);
		moveToPos(vec);		// end in a predictable location
	}
	else
		resetMoveVars();		
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove<Base>::receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
{
	if (moveAgentID && moveAgentID==agentID)
	{
		if (buffer)
			putSyncData(buffer, bufferSize, false);
		THEMATRIX->OperationCompleted2(moveAgentID,dwMissionID);
	}
	if (jumpAgentID && jumpAgentID==agentID)
	{
		if (buffer)
			putSyncData(buffer, bufferSize, false);

		THEMATRIX->OperationCompleted2(jumpAgentID,dwMissionID);
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove<Base>::explodeMove (bool bExplode)
{
	if (footprintHistory.numEntries>0)
	{
		COMPTR<ITerrainMap> map;

		if (SECTOR)
		{
			SECTOR->GetTerrainMap(systemID, map);
			if (map)
				undoFootprintInfo(map);
		}
	}
	if (OBJMAP && map_sys && map_sys <= MAX_SYSTEMS)
	{
		OBJMAP->RemoveObjectFromMap(this,map_sys,map_square);
		map_sys = map_square = 0;
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
U32 ObjectMove<Base>::getSyncData (void * buffer)
{
	GRIDVECTOR * const data = (GRIDVECTOR *) buffer;
	CQASSERT(THEMATRIX->IsMaster());

	if (isAutoMovementEnabled()==false)
		return 0;

	/*
	if (bPatroling && bMoveActive==0)
	{
		patrolIndex = (patrolIndex + 1) & 1;
		*data = patrolVectors[patrolIndex];
		bSyncNeeded=false;
		moveToPos(patrolVectors[patrolIndex]);
		return sizeof(*data);
	}
	*/

	if (bSyncNeeded && bMoveActive==0)
	{
		*data = currentPosition;
		bSyncNeeded = false;
		return sizeof(*data);
	}

	return 0;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove<Base>::putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	GRIDVECTOR * vec = (GRIDVECTOR *) buffer;

	CQASSERT(bufferSize == sizeof(GRIDVECTOR));
	CQASSERT(THEMATRIX->IsMaster()==0 && "Sync data received on non-master machine!");

	bCompletionAllowed = true;
	bMoveActive = 1;
	bFinalMove=1;
	bPathOverflow = 0;
	pathLength = 1;
	pathList[0] = goalPosition = *vec;
}
//---------------------------------------------------------------------------
//
template <class Base> 
U32 ObjectMove<Base>::getSyncPatrolData (void * buffer)
{
	GRIDVECTOR * const data = (GRIDVECTOR *) buffer;
	CQASSERT(THEMATRIX->IsMaster());

	if (isAutoMovementEnabled()==false)
		return 0;

	if (bPatroling && bMoveActive==0)
	{
		patrolIndex = (patrolIndex + 1) & 1;
		*data = patrolVectors[patrolIndex];
		bSyncNeeded=false;
		moveToPos(patrolVectors[patrolIndex]);
		return sizeof(*data);
	}

	return 0;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove<Base>::putSyncPatrolData (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	GRIDVECTOR * vec = (GRIDVECTOR *) buffer;

	CQASSERT(bufferSize == sizeof(GRIDVECTOR));
	CQASSERT(THEMATRIX->IsMaster()==0 && "Sync data received on non-master machine!");

	moveToPos(*vec);
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove<Base>::SetPath (ITerrainMap * map, const GRIDVECTOR * const squares, int numSquares)
{
	if (numSquares > MAX_PATH_SIZE)
	{
		pathLength = MAX_PATH_SIZE;
		bPathOverflow = true;
	}
	else
	{
		pathLength = numSquares;
		bPathOverflow = false;
	}
	memcpy(pathList, squares+(numSquares-pathLength), pathLength * sizeof(GRIDVECTOR));

	if (bPathOverflow == false)
	{
		SetTerrainFootprint(map);
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectMove<Base>::TakeoverSwitchID (U32 newMissionID)
{
	// first thing, undo the current footprint
	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(systemID, map);
	undoFootprintInfo(map);


	OBJLIST->RemovePartID(this, dwMissionID);
	dwMissionID = newMissionID;

	OBJLIST->AddPartID(this, dwMissionID);

	UnregisterWatchersForObject(this);

	// now set our footprint again
	SetTerrainFootprint(map);
}
//---------------------------------------------------------------------------
//
template <class Base>
GRIDVECTOR ObjectMove<Base>::GetGridPosition (void) const
{
	if (currentPosition.isZero() || overrideMode != OVERRIDE_NONE)	// can happen if unit is not built yet, or is being pushed around
	{
		GRIDVECTOR vec;
		vec = transform.translation;
		return vec;
	}
	else
	{
		if (isMoveActive())
		{
			return findValidGridPosition();
		}
		else
			return currentPosition;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
GRIDVECTOR ObjectMove<Base>::findValidGridPosition (void) const
{
	GRIDVECTOR vec;
	COMPTR<ITerrainMap> map;

	vec = transform.translation;
	if (bHalfSquare)
		vec.quarterpos();
	else
		vec.centerpos();

	SECTOR->GetTerrainMap(systemID, map);
	if (map->IsGridValid(vec))
		return vec;
	else
	{
		// back up along our path
		Vector pos = currentPosition - transform.translation;
		pos.fast_normalize();
		pos *= GRIDSIZE/2;
		pos += transform.translation;
		vec = pos;
		if (bHalfSquare)
			vec.quarterpos();
		else
			vec.centerpos();

		if (map->IsGridValid(vec))
			return vec;
		else
			return currentPosition;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectMove<Base>::calcSlopOffset (void)
{
	SINGLE angle = (SINGLE(rand() & 255) / 256 * 360 * MUL_DEG_TO_RAD);
	slopOffset = TRANSFORM::rotate_about_z(transform.get_k(), angle);		// yaw 
	slopOffset *= ((rand() & 127) + (rand() & 127)) * maxMoveSlop * (1.0 / 256.0);
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove< Base >::physUpdateMove (SINGLE dt)
{
	ANIM->update_instance(instanceIndex,dt); //my solution for now is to go back to updating all moving objects every frame

	if (bVisible && !bReady)
	{
		ENGINE->update_instance(instanceIndex, 0, dt);
	}
	else
	if (bEnablePhysics)
		ENGINE->update_instance(instanceIndex, 0, 0);		// 0 dt means "just update tree"
}
//----------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove< Base >::TESTING_shudder (const Vector & dir, SINGLE mag)
{
	if (bVisible && fabs(transform.get_roll()) < 10 * MUL_DEG_TO_RAD)
	{
		restoreDynamicsData();
		mag *= (getDynamicsData().angAcceleration*0.12);
		SINGLE angle = transform.get_yaw() - TRANSFORM::get_yaw(dir);
		if (angle < -PI)
			angle += PI*2;
		else
		if (angle > PI)
			angle -= PI*2;
		if (angle < 0)
			mag = -mag;
		mag *= 1.0 - ((fabs(fabs(angle)-(MUL_DEG_TO_RAD * 90.0)) / (MUL_DEG_TO_RAD * 90.0)));
		ang_velocity += transform.get_k() * mag;		//  take 1/3 second to stop velocity
		bRollTooHigh = 1;		// current roll is too much
		tooHighCounter = 0;
	}
}
//----------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove< Base >::PushShip (U32 attackerID, const Vector & direction, SINGLE velMag)
{
	overrideAttackerID = attackerID;
	overrideMode = OVERRIDE_PUSH;
	overrideSpeed = velMag;
	Vector dir = direction;
	dir *= (GRIDSIZE / direction.magnitude());
	Vector position = transform.translation;

	while (1)
	{
		position += dir;
		U32 _x = ((F2LONG(position.x) * 4) + ((GRIDSIZE-1)/2)) / GRIDSIZE;
		U32 _y = ((F2LONG(position.y) * 4) + ((GRIDSIZE-1)/2)) / GRIDSIZE;
		
		if (_x > 255 || _y > 255)
			break;
	}
	position -= dir;		// undo the extra one
	pushShipTo(position);
}
//----------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove< Base >::PushShipTo (U32 attackerID, const Vector & position, SINGLE velMag)
{
	overrideAttackerID = attackerID;
	overrideMode = OVERRIDE_PUSH;
	overrideSpeed = velMag;
	pushShipTo(position);
}
//----------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove< Base >::DestabilizeShip (U32 attackerID)
{
	if (overrideMode == OVERRIDE_PUSH)
		cancelPush();
	overrideAttackerID = attackerID;
	overrideMode = OVERRIDE_DESTABILIZE;
	setDestabilize();		// have this take affect immediately!
}
//----------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove< Base >::ForceShipOrientation (U32 attackerID, SINGLE yaw)
{
	if (overrideMode == OVERRIDE_PUSH)
		cancelPush();
	overrideAttackerID = attackerID;
	overrideMode = OVERRIDE_ORIENT;
	overrideYaw = yaw;
}
//----------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove< Base >::ReleaseShipControl (U32 attackerID)
{
	if (attackerID == overrideAttackerID)
	{
		if (overrideMode == OVERRIDE_PUSH)
			cancelPush();
		overrideAttackerID = 0;
		overrideMode = OVERRIDE_NONE;
	}
}
//----------------------------------------------------------------------------
//
template <class Base> 
SINGLE ObjectMove< Base >::GetCurrentCruiseVelocity (void)
{
	return getCruiseSpeed();
}

template <class Base> 
void ObjectMove< Base >::RemoveFromMap (void)
{
	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(systemID, map);
	if (map)
		undoFootprintInfo(map);
	bExploding = true;
}

template <class Base> 
bool ObjectMove< Base >::IsMoving (void)
{
	return isMoveActive();
}

//----------------------------------------------------------------------------
// find furthest position allowed within line-of-sight
//
template <class Base> 
void ObjectMove< Base >::pushShipTo (const Vector & position)
{
	TestPushLOSCallback callback;
	COMPTR<ITerrainMap> map;

	CQASSERT(systemID && systemID <= MAX_SYSTEMS);

	SECTOR->GetTerrainMap(systemID, map);

	GRIDVECTOR toPos;
	toPos = position;
	if (map->TestSegment(GetGridPosition(), toPos, &callback))
		overrideDest = toPos;
	else
	{
		if (callback.gridPos.isZero())	// no where to go
			callback.gridPos = GetGridPosition();

		overrideDest = callback.gridPos;
	}
}
//----------------------------------------------------------------------------
//
template <class Base> 
void ObjectMove< Base >::cancelPush (void)
{
	CQASSERT(overrideMode==OVERRIDE_PUSH);
	if (isMoveActive()==0 && THEMATRIX->IsMaster())
	{
		GRIDVECTOR vec;
		vec = transform.translation+(velocity/2);
		moveToPos(vec);		// end in a predictable location
	}
}
//----------------------------------------------------------------------------
//

struct FootprintQuickList : ITerrainSegCallback
{
	enum
	{
		MAX = 16
	};

	struct Info
	{
		FootprintInfo fpi;
		GRIDVECTOR    grid;

		bool operator == (const Info& _info)
		{
			return
			( 
				fpi.flags == _info.fpi.flags && 
				fpi.missionID == _info.fpi.missionID 
			);
		}
	};

	Info list[MAX];
	U32  count;

	FootprintQuickList() : count(0) {}

	virtual bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
	{
		if( count < MAX )
		{
			if( info.flags & TERRAIN_REGION )
			{
				list[count].fpi  = info;
				list[count].grid = pos;
				count++;
			}
		}
		return( count < MAX );
	}

	bool hasFootprint( Info& _test )
	{
		for( U32 i = 0; i < count; i++ )
		{
			if( list[i] == _test )
			{
				return true;
			}
		}
		return false;
	}
};

template <class Base> 
void ObjectMove< Base >::scriptingUpdate( FootprintHistory& _oldFootprint, FootprintHistory& _newFootprint )
{
	if( _oldFootprint.numEntries <= 0 || _newFootprint.numEntries <= 0 )
	{
		// invalid footprint history
		return;
	}

	// make quick copies
	FootprintHistory lastFootprint = _oldFootprint;
	FootprintHistory nextFootprint = _newFootprint;

	// make the grid space the center of the LARGE grid square
	lastFootprint.vec[0].centerpos();
	nextFootprint.vec[0].centerpos();

	if( lastFootprint.vec[0] == nextFootprint.vec[0] )
	{
		// same grid, no change
		return;
	}

	FootprintQuickList fqlLast;
	FootprintQuickList fqlNext;

	// get a list of footprints for the grid ship is leaving
	COMPTR<ITerrainMap> oldMap;
	SECTOR->GetTerrainMap(lastFootprint.systemID, oldMap);
	if( oldMap )
	{
		oldMap->TestSegment( lastFootprint.vec[0], lastFootprint.vec[0], &fqlLast );
	}

	// get a list of footprints for the grid ship is entering
	COMPTR<ITerrainMap> nextMap;
	SECTOR->GetTerrainMap(nextFootprint.systemID, nextMap);
	if( nextMap )
	{
		nextMap->TestSegment( nextFootprint.vec[0], nextFootprint.vec[0], &fqlNext );
	}

	// testing for EXITing
	U32 i;
	for(i = 0; i < fqlLast.count; i++ )
	{
		// was this region NOT in the last region?
		if( !fqlNext.hasFootprint(fqlLast.list[i]) )
		{
			// send a Ship is Exiting message to scripting here
			ScriptParameterList params;
			params.Push( dwMissionID, "shipID" );
			params.Push( fqlLast.list[i].fpi.missionID, "regionID" );

			SCRIPTING->CallScriptEvent( SE_SHIP_EXIT, &params );
		}
	}

	// testing for ENTERing
	for( i = 0; i < fqlNext.count; i++ )
	{
		// is this region NOT in the next region?
		if( !fqlLast.hasFootprint(fqlNext.list[i]) )
		{
			// send a Ship is Entering message to scripting here
			ScriptParameterList params;
			params.Push( dwMissionID, "shipID" );
			params.Push( fqlNext.list[i].fpi.missionID, "regionID" );

			SCRIPTING->CallScriptEvent( SE_SHIP_ENTER, &params );
		}
	}

}

//----------------------------------------------------------------------------
//------------------------End ObjMove.cpp-------------------------------------
//----------------------------------------------------------------------------
