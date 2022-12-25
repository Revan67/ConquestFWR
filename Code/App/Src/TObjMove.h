//--------------------------------------------------------------------------//
//                                                                          //
//                                TObjMove.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjMove.h 189   8/23/01 1:53p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef XFORM_H
#include <xform.h>
#endif

#ifndef TERRAINMAP_H
#include "TerrainMap.h"
#endif

#ifndef DSHIPSAVE_H
#include <DShipSave.h>
#endif

#ifndef DSPACESHIP_H
#include <DSpaceShip.h>
#endif

#ifndef _INC_STDLIB
#include <stdlib.h>
#endif 

#ifndef OPAGENT_H
#include "OPAgent.h"
#endif

#ifndef GRIDVECTOR_H
#include "GridVector.h"
#endif

#ifndef OBJMAP_H
#include "ObjMap.h"
#endif

#ifndef MPART_H
#include "MPart.h"
#endif

#ifndef DTROOPSHIP_H
#include <DTroopship.h>
#endif

#ifndef OBJSET_H
#include "Objset.h"
#endif

#ifndef ISHIPMOVE_H
#include "IShipMove.h"
#endif

#ifndef IADMIRAL_H
#include "IAdmiral.h"
#endif

#define ObjectMove _CoM

struct TRUEANIMPOS
{
	NETGRIDVECTOR gridVec;
	Vector pos;
};

extern TRUEANIMPOS g_trueAnimPos;

#define SEND_OPERATION(x)   \
	{	TSHIP_NET_COMMANDS command = x;   \
		THEMATRIX->SendOperationData(attackAgentID, dwMissionID, &command, 1); }

#define DEF_ROCK_LINEAR_MAX 150.0F
#define DEF_ROCK_ANG_MAX 1
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectMove : public Base, private SPACESHIP_SAVELOAD::TOBJMOVE, private IFindPathCallback, public IShipMove
{
	struct SaveNode   saveNode;
	struct LoadNode   loadNode;
	struct InitNode   initNode;
	struct UpdateNode updateNode;
	struct OnOpCancelNode	onOpCancelNode;
	struct PreTakeoverNode	preTakeoverNode;
	struct GeneralSyncNode  genSyncNode;
	struct GeneralSyncNode  genSyncNode2;
	struct PhysUpdateNode   physUpdateNode;
	struct ReceiveOpDataNode receiveOpDataNode;
	struct ExplodeNode		 explodeNode;

	typename typedef Base::SAVEINFO MOVESAVEINFO;
	typename typedef Base::INITINFO MOVEINITINFO;

private:
	ROCKING_DATA rockingData;
	
	SINGLE origAcceleration;

	SINGLE maxMoveSlop;			// make movement irregular, for Chris
	Vector slopOffset;
	bool bRollTooHigh:1;		// current roll is too much
	bool bHalfSquare:1;
	U8	 tooHighCounter;
	int map_square;
	U32 map_sys;

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



	//---------------------------------------------------------------------------
	//
	struct TestPassibleCallback : ITerrainSegCallback
	{
		GRIDVECTOR gridPos;

		TestPassibleCallback (void)
		{
		}

		virtual bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
		{
			if (info.flags & TERRAIN_IMPASSIBLE)
			{
				gridPos = pos;
				return false;
			}
			return true;
		}
	};

	struct TestPushLOSCallback : ITerrainSegCallback
	{
		GRIDVECTOR gridPos;

		TestPushLOSCallback (void)
		{
			gridPos.zero();
		}

		virtual bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
		{
			if (info.flags & (TERRAIN_IMPASSIBLE|TERRAIN_BLOCKLOS|TERRAIN_OUTOFSYSTEM))
				return false;
			gridPos = pos;
			return true;
		}
	};

	//----------------------------------

public:
	//----------------------------------
	
	ObjectMove (void);

	~ObjectMove (void);

	/* ObjectMove methods */

	void resetMoveVars (void)
	{
		if (moveAgentID!=0)
			CQBOMB1("Invalid call to resetMoveVars() for part=%s (Ignorable)", (char *)partName);	// should already be completed before this call
		if (bMoveActive)
		{
			currentPosition = transform.translation;
			bMoveActive=0;
		}
		velocity.z = 0;
		setCruiseSpeed(0);		// set it back to default
		setForwardAcceleration(0);	// set it back to default
		bRotatingBeforeMove = false;
		bFinalMove = false;
		cruiseDepth = 0;
		bMockRotate = false;
		bCompletionAllowed = false;
	}

	void moveToPos (const GRIDVECTOR & pos)  // start a move operation
	{
		moveToPos(pos, 0);  // start a move operation
	}

	void moveToPos (const GRIDVECTOR & pos, U32 agentID, bool bSlowMove = false);  // start a move operation

	void moveToJump (IBaseObject * jumpgate, U32 agentID, bool bSlowMove);

	// move toward goal position, return true when we get there
	// does not use path finding or avoidance
	bool doMove (const Vector & goal);

	bool isMovingToJump (void) const
	{
		return (jumpAgentID!=0);
	}

	bool isMoveActive (void) const
	{
		return bMoveActive;
	}

	bool isPatroling (void) const
	{
		return bPatroling;
	}

	bool isAutoMovementEnabled (void) const
	{
		return bAutoMovement;
	}

	void disableAutoMovement (void)
	{
		bAutoMovement = false;
	}

	void enableAutoMovement (void)
	{
		bAutoMovement = true;
	}

	// override this method in derived class if you want to
	virtual bool testReadyForJump (void)
	{
		return true;
	}

	bool isHalfSquare()
	{
		return bHalfSquare;
	}

	GRIDVECTOR getLastGrid()
	{
		return footprintHistory.vec[0];
	}

	const GRIDVECTOR & getGoalPosition (void) const
	{
		return goalPosition;
	}

	/* IMissionActor overrides */

	virtual bool IsMoveActive (void) const
	{
		return bMoveActive;
	}

	virtual void TakeoverSwitchID (U32 newID);

	SINGLE getCruiseSpeed (void)
	{
		SINGLE fleetMod = 1.0;
		if(fleetID)
		{
			VOLPTR(IAdmiral) flagship;
			OBJLIST->FindObject(fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
			if(flagship.Ptr())
			{
				MPart part(this);
				fleetMod = 1 + flagship->GetSpeedBonus(mObjClass,part.pInit->armorData.myArmor);
			}				
		}
		SINGLE sectorMod = 1.0 + SECTOR->GetSectorEffects(playerID,systemID)->getSpeedMod();
		return (maxVelocity*fieldFlags.getSpeedModifier()*effectFlags.getSpeedModifier()*fleetMod*sectorMod);
	}

	void setCruiseSpeed (SINGLE speed)
	{
		CQASSERT(speed >= 0);
		DYNAMICS_DATA dyn = getDynamicsData();

		if (speed)
			dyn.maxLinearVelocity = cruiseSpeed = speed;
		else
			dyn.maxLinearVelocity = cruiseSpeed = maxVelocity;

		setDynamicsData(dyn);
	}

	SINGLE getForwardAcceleration (void)
	{
		const DYNAMICS_DATA & dyn = getDynamicsData();
		return dyn.linearAcceleration;
	}

	void setForwardAcceleration (SINGLE acceleration)
	{
		CQASSERT(acceleration >= 0);
		DYNAMICS_DATA dyn = getDynamicsData();

		if (acceleration)
			dyn.linearAcceleration = groupAcceleration = acceleration;
		else
			dyn.linearAcceleration = groupAcceleration = origAcceleration;

		setDynamicsData(dyn);
	}

	void setRandomCruiseDepth (void)
	{
		SINGLE depths[4] = { -200, -400, -800, -1000};
		cruiseDepth = depths[rand() & 3];
	}

	SINGLE getCruiseDepth (void)
	{
		return cruiseDepth;
	}

	bool isRotatingBeforeMove (void) const
	{
		return bRotatingBeforeMove;
	}

	U32 getSyncData (void * buffer);

	void putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery);

	U32 getSyncPatrolData (void * buffer);

	void putSyncPatrolData (void * buffer, U32 bufferSize, bool bLateDelivery);

	void TESTING_shudder (const Vector & dir, SINGLE mag);

	bool getRollTooHigh (void) const
	{
		return bRollTooHigh;
	}

	const GRIDVECTOR & getCurrentPosition (void) const
	{
		return currentPosition;
	}

	void undoFootprintInfo (struct ITerrainMap * terrainMap);

	void patrol(const GRIDVECTOR & src, const GRIDVECTOR & dst, U32 agentID)
	{
		// patrol between two points
		patrolIndex = 0;
		patrolVectors[0] = src;
		patrolVectors[1] = dst;
		bPatroling = true;
	}

	/* IPhysicalObject methods */

	virtual void SetPosition (const Vector & position, U32 newSystemID)
	{
		CQASSERT(newSystemID && newSystemID <= MAX_SYSTEMS);
		bool bNewSystem = (systemID != newSystemID);
		systemID = newSystemID;
		Base::SetPosition(position, newSystemID);
		if (bNewSystem || isMoveActive()==0)		// don't do this thing on load if we are busy
		{
			currentPosition = transform.translation;
			if (bHalfSquare)
				currentPosition.quarterpos();
			else
				currentPosition.centerpos();
			if (isAutoMovementEnabled())
				moveToPos(currentPosition);
			else
				resetMoveVars();	// could be moving but temporarily disabled
		}
	}

	virtual void SetTransform (const TRANSFORM & _transform, U32 newSystemID)
	{
		CQASSERT(newSystemID && newSystemID <= MAX_SYSTEMS);
		bool bNewSystem = (systemID != newSystemID);
		systemID = newSystemID;
		Base::SetTransform(_transform, newSystemID);
		if (bNewSystem || isMoveActive()==0)		// don't do this thing on load if we are busy
		{
			currentPosition = transform.translation;
			if (bHalfSquare)
				currentPosition.quarterpos();
			else
				currentPosition.centerpos();
			if (isAutoMovementEnabled())
				moveToPos(currentPosition);		// don't do this if disabled (admirals)
			else
				resetMoveVars();	// could be moving but temporarily disabled
		}
	}

	/* TObjControl methods */

	bool rotateShip (SINGLE relYaw, SINGLE relRoll, SINGLE relPitch)
	{
		disableAutoMovement();
		return Base::rotateShip(relYaw, relRoll, relPitch);
	}
	void rotateTo (SINGLE absYaw, SINGLE absRoll, SINGLE absPitch)
	{
		disableAutoMovement();
		Base::rotateTo(absYaw, absRoll, absPitch);
	}
	void setAltitude (SINGLE relAltitude)
	{
		disableAutoMovement();
		Base::setAltitude(relAltitude);
	}
	bool setPosition (const Vector & relPosition)
	{
		disableAutoMovement();
		return Base::setPosition(relPosition);
	}
	bool setPosition (const Vector & relPosition, SINGLE _finalVel)
	{
		disableAutoMovement();
		return Base::setPosition(relPosition, _finalVel);
	}
	void moveTo (const Vector & absPosition)
	{
		disableAutoMovement();
		Base::moveTo(absPosition);
	}
	void moveTo (const Vector & absPosition, SINGLE _finalVel)
	{
		disableAutoMovement();
		Base::moveTo(absPosition, _finalVel);
	}
	void setThrustersOn (void)
	{
		disableAutoMovement();
		Base::setThrustersOn();
	}
	
	/* IBaseObject methods */

	virtual struct GRIDVECTOR GetGridPosition (void) const;

	virtual void SetTerrainFootprint (struct ITerrainMap * terrainMap);

	/* IShipMove methods */

	virtual void PushShip (U32 attackerID, const Vector & direction, SINGLE velMag);

	virtual void PushShipTo (U32 attackerID, const Vector & position, SINGLE velMag);

	virtual void DestabilizeShip (U32 attackerID);

	virtual void ForceShipOrientation (U32 attackerID, SINGLE yaw);

	virtual void ReleaseShipControl (U32 attackerID);

	virtual SINGLE GetCurrentCruiseVelocity (void);

	virtual void RemoveFromMap (void);

	virtual bool IsMoving (void);


private:

	void updateObjMap (void)
	{
		if (systemID && systemID <= MAX_SYSTEMS)		// don't do this in hyperspace
		{
			int new_map_square = OBJMAP->GetMapSquare(systemID,transform.translation);
			if (new_map_square != map_square || map_sys != systemID)
			{
				OBJMAP->RemoveObjectFromMap(this,map_sys,map_square);
				map_square = new_map_square;
				map_sys = systemID;
				U32 flags = (bDerelict) ? 0 : OM_TARGETABLE;
				if (aliasPlayerID)
					flags |= OM_MIMIC;
				objMapNode = OBJMAP->AddObjectToMap(this,map_sys,map_square,flags);
				CQASSERT(objMapNode);
			}
		}
	}

	virtual void SetPath (ITerrainMap * map, const GRIDVECTOR * const squares, int numSquares);

	BOOL32 updateMoveState (void);

	void initMoveState (const MOVEINITINFO & data);

	void loadMoveState (MOVESAVEINFO & saveStruct);

	void saveMoveState (MOVESAVEINFO & saveStruct);

	void rockTheBoat (void);

	void onOpCancel (U32 agentID);

	void preTakeover (U32 newMissionID, U32 troopID);

	// use pathfinding, avoidance to reach goalPosition
	bool doPathMove (void);

	void doJumpPreparation (void);

	void onPathComplete (void);

	bool fleetRotationCheck (void);

	bool testPassible (const struct GRIDVECTOR & pos);

	GRIDVECTOR findValidGridPosition (void) const;

	void calcSlopOffset (void);

	void physUpdateMove (SINGLE dt);

	void pushShipTo (const Vector & position);

	void cancelPush (void);

	void receiveOperationData (U32 agentID, void *buffer, U32 bufferSize);

	void explodeMove (bool bExplode);

	SINGLE completionModifier (void) const
	{
		return (bCompletionAllowed) ? 1.5 : 1.0;		// catch up with host
	}

	void scriptingUpdate( FootprintHistory& oldFootprint, FootprintHistory& newFootprint );
};

//---------------------------------------------------------------------------
//
template <class Base> 
ObjectMove<Base>::ObjectMove (void) :
					physUpdateNode(this, PhysUpdateProc(&ObjectMove::physUpdateMove)),
					saveNode(this, SaveLoadProc(&ObjectMove::saveMoveState)),
					loadNode(this, SaveLoadProc(&ObjectMove::loadMoveState)),
					initNode(this, InitProc(&ObjectMove::initMoveState)),
					updateNode(this, UpdateProc(&ObjectMove::updateMoveState)),
					onOpCancelNode(this, OnOpCancelProc(&ObjectMove::onOpCancel)),
					preTakeoverNode(this, PreTakeoverProc(&ObjectMove::preTakeover)),
					receiveOpDataNode(this, ReceiveOpDataProc(&ObjectMove::receiveOperationData)),
					explodeNode(this, ExplodeProc(&ObjectMove::explodeMove)),
					genSyncNode(this, SyncGetProc(&ObjectMove::getSyncData), SyncPutProc(&ObjectMove::putSyncData)),
					genSyncNode2(this, SyncGetProc(&ObjectMove::getSyncPatrolData), SyncPutProc(&ObjectMove::putSyncPatrolData))
{
	bAutoMovement = true;						
}
//---------------------------------------------------------------------------
//------------------------End TObjMove.h-------------------------------------
//---------------------------------------------------------------------------
