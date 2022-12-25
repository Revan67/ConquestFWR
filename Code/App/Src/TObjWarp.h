#ifndef TOBJWARP_H
#define TOBJWARP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               TObjWarp.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjWarp.h 93    11/07/00 11:02a Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef ICAMERA_H
#include <ICamera.h>
#endif

#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

#ifndef ARCHHOLDER_H
#include "ArchHolder.h"
#endif

#ifndef IJUMPGATE_H
#include "IJumpGate.h"
#endif

#ifndef DSHIPSAVE_H
#include <DShipSave.h>
#endif

#ifndef FOGOFWAR_H
#include "FogOfWar.h"
#endif

#define ObjectWarp _Cow
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectWarp : public Base, WARP_SAVELOAD
{
#define MAX_SHIP_STRETCH	3.0	// multiplier on ship's normal length.

//	struct PreRenderNode	preRenderNode;
//	struct PostRenderNode   postRenderNode;
	struct PhysUpdateNode   physUpdateNode;
	struct UpdateNode		updateNode;
	struct ExplodeNode      explodeNode;
	struct SaveNode			saveNode;
	struct LoadNode			loadNode;
	struct ResolveNode		resolveNode;
	struct InitNode			initNode;
	struct OnOpCancelNode	onOpCancelNode;

	typename typedef Base::SAVEINFO WARPSAVEINFO;
	typename typedef Base::INITINFO WARPINITINFO;

	OBJPTR<IJumpGate> targetGate;
	OBJPTR<IJumpGate> inTargetGate;
	Transform scaleTrans;
	PARCHETYPE pTrailType;
	Vector warpVector;
	SINGLE acceleration;
	Vector angVel;
	Vector gatePosition;
	//----------------------------------
	
	ObjectWarp (void);

	/* ObjectWarp methods */

	void startWarpIn  ();//IBaseObject * jumpgate, const Vector& jumpToPosition, SINGLE heading, SINGLE speed);
	void startWarpOut (IBaseObject * outGate,IBaseObject *inGate);
	
	void useJumpgate (IBaseObject * outgate, IBaseObject * ingate, const Vector& jumpToPosition, SINGLE heading, SINGLE speed, U32 agentID);

	void cancelWarp (void);

private:
	// handleWarp() should be called when ship is the desired distance from
	// the jumpgate and facing the jumpgate. Distance will vary with ship size I'd guess.
	// For warp IN, the ship should be positioned "behind" the jumpgate, facing it.
	void handleWarp(WARP_STAGE ws);

	// Internal methods used in Update(), Render(), etc.
	BOOL32 doWarp (SINGLE dt);					// warp scaling.
	void warpAccelerate (SINGLE dt);			// acceleration. Needs to happen after UpdateDynamics()
	void warpPreRender (void);
	void warpPostRender (void);
	void updateWarp (SINGLE dt);
	void initWarp (const WARPINITINFO & data);
	void explodeWarp (bool bExplode);

	void loadWarpState (WARPSAVEINFO & saveStruct);
	void saveWarpState (WARPSAVEINFO & saveStruct);
	void resolveWarpState (void);
	void onOpCancelWarp (U32 agentID);
	void recalcVisibilityOnWarpIn (void);
	BOOL32 handleWarpUpdate (void);

	static void setModelScale(INSTANCE_INDEX instanceIndex,Vector scale,Vector scale_origin);
};

//---------------------------------------------------------------------------
//
template <class Base> 
ObjectWarp< Base >::ObjectWarp (void) :
			physUpdateNode(this, PhysUpdateProc(&ObjectWarp::updateWarp)),
			explodeNode(this, ExplodeProc(&ObjectWarp::explodeWarp)),
			saveNode(this, SaveLoadProc(&ObjectWarp::saveWarpState)),
			loadNode(this, SaveLoadProc(&ObjectWarp::loadWarpState)),
			resolveNode(this, ResolveProc(&ObjectWarp::resolveWarpState)),
			onOpCancelNode(this, OnOpCancelProc(&ObjectWarp::onOpCancelWarp)),
			initNode(this, InitProc(&ObjectWarp::initWarp)),
			updateNode(this, UpdateProc(&ObjectWarp::handleWarpUpdate))
{
}
//---------------------------------------------------------------------------
//
template <class Base> 
BOOL32 ObjectWarp< Base >::handleWarpUpdate (void)
{
	if (warpStage != WS_NONE)
		bEnablePhysics = 1;
	return 1;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectWarp< Base >::handleWarp (WARP_STAGE ws)
{
	SINGLE relYaw=0;
	if (pTrailType && systemID==SECTOR->GetCurrentSystem() && FOGOFWAR->CheckVisiblePosition(transform.translation))	// don't bother if off screen
	{
		IBaseObject *trail = CreateTrail(pTrailType,this,transform.translation+warpVector*(warpRadius-400),systemID);
		OBJLIST->AddObject(trail);
	}

	if (ws == WS_PRE_WARP)
	{
		warpTimer = PRE_JUMP_TIME;
		
		warpSpeed = 1000;
		
		SINGLE yaw = transform.get_yaw();
		SINGLE desiredYaw = get_angle(warpVector.x,warpVector.y);
		relYaw = fixAngle(desiredYaw-yaw);
	}

	rotateShip(relYaw, 0, 0);
}

//
// this function exists for testing!!
//
inline int WarpDummy (void)
{
	return 5;
}
//---------------------------------------------------------------------------
//
template <class Base> 
BOOL32 ObjectWarp< Base >::doWarp (SINGLE dt)
{
	BOOL32 bWarping = TRUE;

	Vector pos = ENGINE->get_position(instanceIndex);
	Vector dp = pos - gatePosition;
	SINGLE zStretch=1.0;
	Vector svec(1,1,1),svec2(0,0,0);

	if (warpStage == WS_LIMBO)
	{
		warpTimer -= dt;
		if (warpTimer < 0)
		{
			// WARP IS DONE. SHIP SHOULD BE CHANGED TO NEW SYSTEM, etc.
			bWarping = FALSE;
			velocity.zero();
			warpStage = WS_NONE;
			startWarpIn();
		}

		goto Done;
	}

	if (warpStage == WS_PRE_WARP)
	{
		SINGLE yaw = transform.get_yaw();
		SINGLE desiredYaw = get_angle(warpVector.x,warpVector.y);
		SINGLE relYaw = fixAngle(desiredYaw-yaw);

		if (rotateShip(relYaw, 0, 0))		// are we facing the jumpgate?
		{
			warpStage = WS_WARP_OUT;
			warpTimer = JUMP_TIME;
			handleWarp(WS_WARP_OUT);
		}
		goto Done;
	}

	if (warpStage == WS_WARP_OUT)
	{
		warpTimer-= dt;
		if (warpTimer > 0)
		{
			//looking for a difference of 1 here - BE CAREFUL
			zStretch = 3-2*fabs((warpTimer)/PRE_JUMP_TIME);
			CQASSERT(zStretch >= 1);
		}
		else
		{
			//MOVE SHIP TO "0 SPACE"
			warpStage = WS_LIMBO;
			U32 sysID = inTargetGate.Ptr()->GetSystemID();
			SECTOR->RevealSystem(sysID,playerID);
			FOGOFWAR->RevealBlackZone(playerID,sysID,inTargetGate.Ptr()->GetPosition(),5000.0/GRIDSIZE);
			SetSystemID(sysID | HYPER_SYSTEM_MASK);
			// update the map with our latest location
			COMPTR<ITerrainMap> map;
			SECTOR->GetTerrainMap(systemID, map);
			if (map)
				SetTerrainFootprint(map);

			UnregisterSystemVolatileWatchersForObject(this);

		//	warpTimer = inTargetGate->RegisterWarpIn();
			warpTimer = releaseTime;
			goto Done;
		}
	}

	if (warpStage == WS_WARP_IN)
	{
		warpTimer+=dt;
		CQASSERT(warpTimer >= 0);
		if (warpTimer < JUMP_TIME)
		{
			zStretch = 3-2*fabs((warpTimer)/JUMP_TIME);
		}
		else
		{
			bWarping = FALSE;
			warpStage = WS_NONE;
			velocity = warpVector*stop_speed;
			ang_velocity.zero();
			THEMATRIX->OperationCompleted2(warpAgentID, dwMissionID);
			CQTRACEM2("%s completed jump to system %d", (char*)partName, systemID);
			GRIDVECTOR grid;
			grid = transform.translation + velocity;
			moveToPos(grid);		// reestablish a grid location
			goto Done;
		}
	}
		

	CQASSERT (warpTimer >= 0);

	svec.set(1,1, zStretch);
	svec2.set(0,0,box[BBOX_MAX_Z]);
	
	warpAccelerate(dt);

	SINGLE stretchRatio;
	stretchRatio = dp.magnitude()/warpRadius;
	svec.set(stretchRatio, stretchRatio, zStretch*stretchRatio);
	svec2.set(0,0,box[BBOX_MAX_Z]);

	scaleTrans.d[0][0] = svec.x;
	scaleTrans.d[1][1] = svec.y;
	scaleTrans.d[2][2] = svec.z;
	scaleTrans.translation = -svec2;

Done:

	return bWarping;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectWarp< Base >::warpAccelerate (SINGLE dt)
{
//	CQASSERT(warpStage == WS_WARP_IN || warpStage == WS_WARP_OUT);

	Vector vel(0,0,0);

	if (warpStage == WS_WARP_IN)
	{
		warpSpeed -= acceleration*dt;
		if (warpSpeed < 0)//stop_speed)
		{
			warpSpeed = 0;//stop_speed;
		}
		//Vector dir = -ENGINE->get_transform(instanceIndex).get_k();
		vel = warpVector*warpSpeed;
	}
	
	if (warpStage == WS_WARP_OUT)
	{
		if (warpTimer < PRE_JUMP_TIME)
		{
			/*if (angVel.z)
			{
				SINGLE yaw = transform.get_yaw();
				SINGLE desiredYaw = get_angle(warpVector.x,warpVector.y);
				SINGLE relYaw = desiredYaw-yaw;
				if (relYaw < -PI)
					relYaw += PI*2;
				else if (relYaw > PI)
					relYaw -= PI*2;
				
				if (fabs(relYaw) < 0.1)
					angVel.set(0,0,0);
				
				angVel.set(0,0,-4*relYaw/JUMP_TIME);
				ang_velocity = angVel;
			}*/
			warpSpeed += acceleration*dt;
			Vector dir = gatePosition - transform.translation;
			if(dir.x == 0.0 && dir.y == 0.0  && dir.z == 0.0  )
				dir = Vector(0,1,0);
			dir.normalize();
			vel = dir * warpSpeed;
		}
	}

	velocity = vel;
}

//---------------------------------------------------------------------------
//
template <class Base>
void ObjectWarp< Base >::updateWarp (SINGLE dt)
{
	if (warpStage != WS_NONE)
	{
		doWarp(dt);
		disableAutoMovement();
		DEBUG_resetInputCounter();			// no movement needed while warping!
	}
	
	/*if (warpStage != WS_NONE && warpStage != WS_SPECIAL)
	{

		Vector orient = -GetTransform().get_k();
		switch (warpStage)
		{
/*
		case WS_WARP_ROTATE:
			{
				Vector heading_goal;
				heading_goal=gatePosition-transform.translation;
				heading_goal.normalize();
				SINGLE relAngle = get_angle(heading_goal.x,heading_goal.y) - get_angle(orient.x,orient.y);

				if (relAngle < -PI)
					relAngle += PI*2;
				else
				if (relAngle > PI)
					relAngle -= PI*2;

				if (fabs(relAngle) > 4 * MUL_DEG_TO_RAD)
				{
					enableAutoMovement(false);
					rotateShip(transform,relAngle,0,0);
				}
				else
				{
					warpStage = WS_WARP_READY;
					resetMoveVars();
					hPart.JumpReady();
				}
			}
			break;

		case WS_WARP:
		//	bits.reset();
			doWarp(dt);
			break;
			
		}
	}*/
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectWarp< Base >::initWarp (const WARPINITINFO & data)
{
	pTrailType = data.pTrailType;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectWarp< Base >::explodeWarp (bool bExplode)
{
	warpStage = WS_NONE;
	warpAgentID = 0;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectWarp< Base >::recalcVisibilityOnWarpIn (void)
{
	//
	// recalculate visibility
	//
	IBaseObject * obj = OBJLIST->GetObjectList(), *end=0;
	while (obj)
	{
		if (obj->GetSystemID() == systemID)
			obj->CastVisibleArea();
		end = obj;
		obj = obj->next;
	}

	obj = end;
	while (obj)
	{
		if (obj->GetSystemID() == systemID)
			obj->UpdateVisibilityFlags();
		obj = obj->prev;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectWarp< Base >::startWarpIn ()
{
	gatePosition = inTargetGate.Ptr()->GetPosition();
	TRANSFORM trans;
	trans.set_j(Vector(0,0,1));
	Vector facing(sin(stop_heading),cos(stop_heading),0);
	trans.set_k(-facing);
	trans.set_i(Vector(facing.y,-facing.x,0));
	trans.translation = gatePosition;
	SetTransform(trans, inTargetGate.Ptr()->GetSystemID());
	ENGINE->update_instance(instanceIndex,0,0);

	warpStage = WS_WARP_IN;

	warpTimer = 0;

	warpRadius = warpInVector.magnitude();
	warpInVector /= warpRadius;
	warpVector = warpInVector;
	acceleration = 2*warpRadius/(JUMP_TIME*JUMP_TIME);

	warpSpeed = acceleration*JUMP_TIME;

	velocity = warpSpeed*warpInVector;

	handleWarp(WS_WARP_IN);
	recalcVisibilityOnWarpIn();
}
/*
	jumpGate->QueryInterface(IJumpGateID, targetGate);
	CQASSERT(targetGate!=0);

 	gatePosition = targetGate.ptr->GetPosition();
	TRANSFORM trans;
	trans.set_j(Vector(0,0,1));
	Vector facing(cos(heading),sin(heading),0);
	trans.set_k(-facing);
	trans.set_i(Vector(facing.y,-facing.x,0));
	trans.translation = gatePosition;
	SetTransform(trans);
	//SetPosition(gatePosition);

	handleWarp(WARP_IN);
	warpVector = jumpToPosition-gatePosition;
	warpVector.normalize();
	stop_speed = speed;

	warpDir = WARP_IN;
	warpStage = WS_WARP;
}*/
//---------------------------------------------------------------------------
// 	bWaitingForLaunchers will already be TRUE if we need to wait for fighters
// 
template <class Base>
void ObjectWarp< Base >::startWarpOut (IBaseObject * outGate,IBaseObject *inGate)
{
/*	resetMoveVars();

	outGate->QueryInterface(IJumpGateID, targetGate);
	CQASSERT(targetGate!=0);

	inGate->QueryInterface(IJumpGateID, inTargetGate);
	CQASSERT(inTargetGate!=0);
	

	gatePosition = targetGate.ptr->GetPosition();
	warpVector = gatePosition-transform.translation;
	warpVector.normalize();

	warpDir = WARP_OUT;

	warpStage = WS_WARP;
	SINGLE time = targetGate->Activate(this,0,TRUE);
	inTargetGate->Activate(this,time,FALSE);
	handleWarp(WARP_OUT);

	enableAutoMovement(false);*/
}
//---------------------------------------------------------------------------
// 
template <class Base>
void ObjectWarp< Base >::useJumpgate (IBaseObject * outgate, IBaseObject * ingate, const Vector& jumpToPosition, SINGLE heading, SINGLE speed, U32 agentID)
{
	if (warpAgentID!=0)
	{
		CQBOMB4("%s initiating jump to system %d, oldAgent=%d, newAgent=%d", (char*)partName, ingate->GetSystemID(), warpAgentID, agentID);
	}
	ingate->SetVisibleToPlayer(playerID);		// make remote side visible immediately

	resetMoveVars();
	scaleTrans.set_identity();

	outgate->QueryInterface(IJumpGateID, targetGate, NONSYSVOLATILEPTR);
	CQASSERT(targetGate!=0);

	ingate->QueryInterface(IJumpGateID, inTargetGate, NONSYSVOLATILEPTR);
	CQASSERT(inTargetGate!=0);

	/*
	U32 alertState = SECTOR->GetAlertState(ingate->GetSystemID(),GetPlayerID());
	if(alertState & S_LOCKED)
	{
		bUntouchable = true;
		if (objMapNode)
			objMapNode->flags |= OM_UNTOUCHABLE;
		if(bSelected)
			OBJLIST->UnselectObject(this);
	}
	else
	{
		bUntouchable = false;
		if (objMapNode)
			objMapNode->flags &= ~OM_UNTOUCHABLE;
	}
	*/


	gatePosition = targetGate.Ptr()->GetPosition();
	warpVector = gatePosition-transform.translation;
	if(warpVector.x == 0.0 && warpVector.y == 0.0  && warpVector.z == 0.0  )
		warpVector = Vector(0,1,0);
	warpRadius = warpVector.magnitude();
	warpVector.normalize();

	//warp IN stuff
	warpInVector = jumpToPosition-ingate->GetPosition();
	//warpInVector.normalize();

	warpStage = WS_PRE_WARP;
	warpTimer = PRE_JUMP_TIME;
	SINGLE time = targetGate->JumpOut(this,0);
//	inTargetGate->Activate(this,time,FALSE);
	releaseTime = inTargetGate->JumpIn(this,time,warpInVector);
	handleWarp(WS_PRE_WARP);

	disableAutoMovement();




	stop_speed = speed;
	stop_heading = heading;

	acceleration = 2*warpRadius/(JUMP_TIME*JUMP_TIME);

	//SetTransform(trans);

	warpAgentID = agentID;
	CQTRACEM3("%s initiating jump to system %d, agent=%d", (char*)partName, ingate->GetSystemID(), agentID);
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectWarp< Base >::cancelWarp (void)
{
//	bWaitingForWarp = false;
	warpStage = WS_NONE;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectWarp< Base >::loadWarpState (WARPSAVEINFO & load)
{
	*static_cast<WARP_SAVELOAD *> (this) = load.warp_SL;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectWarp< Base >::saveWarpState (WARPSAVEINFO & save)
{
	if (targetGate)
		targetGateID = targetGate.Ptr()->GetPartID();
	else
		targetGateID = 0;

	if (inTargetGate)
		inTargetGateID = inTargetGate.Ptr()->GetPartID();
	else
		inTargetGateID = 0;
	save.warp_SL = *static_cast<WARP_SAVELOAD *> (this);
} 
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectWarp< Base >::resolveWarpState (void)
{
	if (targetGateID)
	{
		IBaseObject * obj;

		obj = OBJLIST->FindObject(targetGateID);
		CQASSERT(obj);
		obj->QueryInterface(IJumpGateID, targetGate, NONSYSVOLATILEPTR);
		CQASSERT(targetGate);
	}

	if (inTargetGateID)
	{
		IBaseObject * obj;

		obj = OBJLIST->FindObject(inTargetGateID);
		CQASSERT(obj);
		obj->QueryInterface(IJumpGateID, inTargetGate, NONSYSVOLATILEPTR);
		CQASSERT(inTargetGate);
	}

	if (warpStage == WS_WARP_OUT)
	{
		gatePosition = targetGate.Ptr()->GetPosition();
		warpVector = gatePosition-transform.translation;
		warpVector.normalize();
	}

	if (warpStage == WS_WARP_IN)
	{
		gatePosition = inTargetGate.Ptr()->GetPosition();
		warpVector = warpInVector;
	}

	acceleration = 2*warpRadius/(JUMP_TIME*JUMP_TIME);

	if (warpStage != WS_NONE)
		doWarp(0);
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectWarp< Base >::onOpCancelWarp (U32 agentID)
{
	CQASSERT(THEMATRIX->IsMaster()==0 || (warpAgentID==0 && warpStage == WS_NONE));
}
//---------------------------------------------------------------------------
template <class Base>
void ObjectWarp< Base >::setModelScale(INSTANCE_INDEX instanceIndex,Vector scale,Vector scale_origin)
{
	ENGINE->set_instance_property(instanceIndex, "Stretch", &scale);
	ENGINE->set_instance_property(instanceIndex, "StretchPoint", &scale_origin);

	INSTANCE_INDEX lastChild = INVALID_INSTANCE_INDEX,child;
	while ((child = MODEL->get_child(instanceIndex,lastChild)) != INVALID_INSTANCE_INDEX)
	{
		Transform parentTrans = ENGINE->get_transform(instanceIndex);
		Transform invChildTrans = ENGINE->get_transform(child).get_transpose();
		Vector childPos = ENGINE->get_position(child);
		Vector scale_origin2 = invChildTrans*(parentTrans.translation-childPos+parentTrans.rotate(scale_origin));
		Vector scale2;
		setModelScale(child,scale,scale_origin2);
		lastChild = child;
	}
}
//---------------------------------------------------------------------------
//---------------------------End TObjWarp.h----------------------------------
//---------------------------------------------------------------------------
#endif