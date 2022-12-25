#ifndef TOBJREPAIR_H
#define TOBJREPAIR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              TObjRepair.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjRepair.h 47    11/14/01 4:46p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

#ifndef FILESYS_H
#include <FileSys.h>
#endif

#ifndef DSHIPSAVE_H
#include "DShipSave.h"
#endif

#ifndef DREPAIRSAVE_H
#include "DRepairSave.h"
#endif

#ifndef ARCHHOLDER_H
#include "ArchHolder.h"
#endif

#ifndef OPAGENT_H
#include "OpAgent.h"
#endif

#ifndef UNITCOMM_H
#include "UnitComm.h"
#endif

#ifndef REPAIREE_H
#include "IRepairee.h"
#endif

#ifndef IREPAIRPLATFORM_H
#include "IRepairPlatform.h"
#endif

#ifndef IHARDPOINT_H
#include "IHardPoint.h"
#endif

#ifndef COMMPACKET_H
#include "CommPacket.h"
#endif
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#define ObjectRepair _Cor

struct TObjRepBuffer
{
	U8 command;
};

struct TObjRepBufferGrid
{
	U8 command;
	GRIDVECTOR grid;
};

template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectRepair : public Base, IRepairee, REPAIR_SAVELOAD
{
	struct UpdateNode       updateNode;
	struct SaveNode			saveNode;
	struct LoadNode         loadNode;
	struct ResolveNode		resolveNode;
	struct InitNode			initNode;
	struct PreDestructNode	destructNode;
	struct OnOpCancelNode	onOpCancelNode;
	struct PreTakeoverNode	preTakeoverNode;
	struct ReceiveOpDataNode	receiveOpDataNode;
		
	typename typedef Base::SAVEINFO REPAIRSAVEINFO;
	typename typedef Base::INITINFO REPAIRINITINFO;
	//----------------------------------

	//the diffent modes involed in a repair operation
	enum REPAIRMODES
	{
		RM_NO_MODE,
		RM_WAIT_BEGIN_CLIENT,
		RM_MOVING_TO_REPAIR,
		RM_WAITING_TO_REPAIR,
		RM_DOCKING,
		RM_DOCKED,
		RM_RETURNTOLOC,
		RM_WAIT_CLIENT_CANCEL
	};

	enum REPAIR_COM
	{
		REPAIR_C_END,
		REPAIR_C_CANCEL,
		REPAIR_C_BEGIN_REG,
		REPAIR_C_BEGIN_MOVE
	};
	
	// repair data
	OBJPTR<IRepairPlatform> repairAtPlatform;

	HardpointInfo  repairHardpoint;
	INSTANCE_INDEX repairHardIndex;

	ObjectRepair (void);
	~ObjectRepair (void);
	
	// IRepairee methods

	virtual void RepairYourselfAt (IBaseObject * platform, U32 agentID);
	
	virtual void RepairStartReceived(IBaseObject * platform);

	virtual void RepairCompleted();

	virtual TRANSFORM RepairIdlePos ();
	
	/* ObjectRepair methods */

	bool moveToRepairPos();

	bool moveToOldPos();

	BOOL32 updateRepair ();

	void cancelRepair (U32 agentID);

	void receiveOpData(U32 agentID, void * buffer, U32 bufferSize);

	void preTakeover (U32 newMissionID, U32 troopID);

	void resolveRepair();
	void saveRepair(REPAIRSAVEINFO &);
	void loadRepair(REPAIRSAVEINFO &);
	void explodeRepair (void);
	void preDestructRepair (void);
	void repairInit(const REPAIRINITINFO &);

	void repairMasterChange(bool bIsMaster);

private:

	struct HPRepEnumerator : IHPEnumerator
	{
		HardpointInfo  repairHardpoint;
		INSTANCE_INDEX repairHardIndex;

		virtual bool EnumerateHardpoint (const HPENUMINFO & info)
		{
			repairHardpoint = info.hardpointinfo;
			repairHardIndex = info.index;
			return false;
		}
	};

};

//---------------------------------------------------------------------------
//
template <class Base> 
ObjectRepair< Base >::ObjectRepair (void) :
					updateNode(this, UpdateProc(&ObjectRepair::updateRepair)),
					saveNode(this, SaveLoadProc(&ObjectRepair::saveRepair)),
					loadNode(this, SaveLoadProc(&ObjectRepair::loadRepair)),
					resolveNode(this, ResolveProc(&ObjectRepair::resolveRepair)),
					initNode(this, InitProc(&ObjectRepair::repairInit)),
					destructNode(this, PreDestructProc(&ObjectRepair::preDestructRepair)),
					onOpCancelNode(this, OnOpCancelProc(&ObjectRepair::cancelRepair)),
					preTakeoverNode(this,PreTakeoverProc(&ObjectRepair::preTakeover)),
					receiveOpDataNode(this,ReceiveOpDataProc(&ObjectRepair::receiveOpData))
{
	repairHardIndex = -1;
}

template <class Base> 
ObjectRepair< Base >::~ObjectRepair (void) 
{

}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectRepair< Base>::RepairYourselfAt (IBaseObject * platform, U32 agentID)
{
	CQASSERT(!repairAgentID);
	if(THEMATRIX->IsMaster())
	{
		if(platform)
		{
			platform->QueryInterface(IRepairPlatformID,repairAtPlatform,playerID);
			if(repairAtPlatform)
			{
				repairAgentID = agentID;
				repairAtID = platform->GetPartID();
				repairMode = RM_MOVING_TO_REPAIR;
				GRIDVECTOR repPos;
				repPos = platform->GetPosition();

				TObjRepBufferGrid buffer;
				buffer.command = REPAIR_C_BEGIN_REG;
				buffer.grid = repPos;
				THEMATRIX->SendOperationData(repairAgentID,dwMissionID,&buffer,sizeof(buffer));

				moveToPos(repPos);
			}
			else
			{
				GRIDVECTOR repPos;
				repPos = platform->GetPosition();

				TObjRepBufferGrid buffer;
				buffer.command = REPAIR_C_BEGIN_MOVE;
				buffer.grid = repPos;
				THEMATRIX->SendOperationData(agentID,dwMissionID,&buffer,sizeof(buffer));

				moveToPos(repPos,agentID);
			}
		}
		else
		{
			TObjRepBuffer buffer;
			buffer.command = REPAIR_C_CANCEL;
			THEMATRIX->SendOperationData(agentID,dwMissionID,&buffer,sizeof(buffer));

			THEMATRIX->OperationCompleted(agentID,dwMissionID);
		}
	}
	else
	{
		repairMode = RM_WAIT_BEGIN_CLIENT;
		repairAgentID = agentID;
		if(platform)
			repairAtID = platform->GetPartID();
		else
			repairAtID = 0;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectRepair< Base>::RepairStartReceived(IBaseObject * platform)
{
	MGlobals::UpgradeMissionObj(this);//upgrade the tech stuff before starting the repair (the host has alredy)
	if(platform)
	{
		platform->QueryInterface(IRepairPlatformID,repairAtPlatform,playerID);
	}
	if(repairAtPlatform)
	{
		repairAtPlatform->LockDock(this);
		repairMode = RM_DOCKING;
		resetMoveVars();
		disableAutoMovement();
		rotateShip(0,0,0);
		setAltitude(0);
	}
	else
	{
		repairMode = RM_NO_MODE;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectRepair< Base>::RepairCompleted()
{
	if(repairAtPlatform)
	{
		if(repairAtPlatform->IsDockLocked())
			repairAtPlatform->UnlockDock(this);
		repairMode = RM_RETURNTOLOC;
	}
	else
	{
		repairMode = RM_RETURNTOLOC;
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
TRANSFORM ObjectRepair< Base>::RepairIdlePos ()
{
	if(repairHardIndex != -1)
	{
		TRANSFORM trans;
		trans.set_orientation(repairHardpoint.orientation);
		trans.set_position(repairHardpoint.point);
		return trans;
	}
	else
	{
		TRANSFORM trans;
		trans.set_identity();
		return trans;
	}
}
//----------------------------------------------------------------------------
//
template <class Base>
bool ObjectRepair< Base>::moveToRepairPos()
{
	OBJPTR<IPlatform> platform;
	repairAtPlatform->QueryInterface(IPlatformID,platform);
	TRANSFORM dockTrans = platform->GetShipTransform();

	bool result = false;
	bool moveResult = false;
	IBaseObject * planet = OBJLIST->FindObject(platform->GetPlanetID());

	if(planet)
	{
		Vector planetPos = planet->GetPosition();


		Vector v2 = dockTrans.translation-planetPos;
		v2.normalize();

		Vector v1 = transform.translation-(dockTrans.translation-v2*500);
		if(v1.fast_magnitude() == 0)
			return true;
		v1.normalize();

		//if I have a good line of sight go for it
		if(dot_product(v1,v2) >= 0)
		{
			Vector targDir;
			if((dockTrans.get_position()-transform.get_position()).fast_magnitude() >= 2000)
				targDir = dockTrans.get_position()-transform.get_position();
			else
				targDir = dockTrans.get_k();
			if(targDir.x == 0 && targDir.y == 0 && targDir.z == 0)
			{
				moveResult = true;
				result = true;
			}
			else
			{
				targDir.normalize();
				SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
				relYaw = fixAngle(relYaw);
				result = rotateShip(relYaw,0,0);
				Vector relDir = dockTrans.get_position();
				relDir -= transform.get_position();
				moveResult = setPosition(relDir);
			}
		}else
		{
			v1 = transform.translation-dockTrans.translation;
			v1.normalize();
			Vector v3 = cross_product(v1,Vector(0,0,1));
			v3.normalize();
			Vector try1 = planetPos+(v3*6000);
			Vector try2 = planetPos-(v3*6000);
			if((try1-dockTrans.translation).fast_magnitude() < (try2-dockTrans.translation).fast_magnitude())
			{
				Vector targDir = try1 - transform.translation;
				targDir.normalize();
				SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
				relYaw = fixAngle(relYaw);
				rotateShip(relYaw,0,0);
				Vector relDir = try1;
				relDir -= transform.get_position();
				setPosition(relDir);
			}
			else
			{
				Vector targDir = try2 - transform.translation;
				targDir.normalize();
				SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
				relYaw = fixAngle(relYaw);
				rotateShip(relYaw,0,0);
				Vector relDir = try2;
				relDir -= transform.get_position();
				setPosition(relDir);
			}
		}
	}

	return moveResult && result;
}
//----------------------------------------------------------------------------
//
template <class Base>
bool ObjectRepair< Base>::moveToOldPos()
{
	Vector targetPos = getLastGrid();
	bool result = false;
	bool moveResult = false;
	if(repairAtPlatform)
	{
		OBJPTR<IPlatform> platform;
		repairAtPlatform->QueryInterface(IPlatformID,platform);


		IBaseObject * planet = OBJLIST->FindObject(platform->GetPlanetID());

		if(planet)
		{
			Vector planetPos = planet->GetPosition();


			Vector v2 = targetPos-planetPos;
			v2.normalize();

			Vector v1 = transform.translation-(planetPos);
			if(v1.fast_magnitude() == 0)
				return true;
			v1.normalize();

			//if I have a good line of sight go for it
			if(dot_product(v1,v2) >= 0)
			{
				Vector targDir = targetPos-transform.get_position();
				if(!targDir.fast_magnitude())
					return true;
				targDir.normalize();
				SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
				relYaw = fixAngle(relYaw);
				result = rotateShip(relYaw,0,0);
				Vector relDir = targetPos;
				relDir -= transform.get_position();
				moveResult = setPosition(relDir);
			}else
			{
				v1 = transform.translation-targetPos;
				v1.normalize();
				Vector v3 = cross_product(v1,Vector(0,0,1));
				v3.normalize();
				Vector try1 = planetPos+(v3*6000);
				Vector try2 = planetPos-(v3*6000);
				if((try1-targetPos).fast_magnitude() < (try2-targetPos).fast_magnitude())
				{
					Vector targDir = try1 - transform.translation;
					targDir.normalize();
					SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
					relYaw = fixAngle(relYaw);
					rotateShip(relYaw,0,0);
					Vector relDir = try1;
					relDir -= transform.get_position();
					setPosition(relDir);
				}
				else
				{
					Vector targDir = try2 - transform.translation;
					targDir.normalize();
					SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
					relYaw = fixAngle(relYaw);
					rotateShip(relYaw,0,0);
					Vector relDir = try2;
					relDir -= transform.get_position();
					setPosition(relDir);
				}
			}
		}
		else
		{
			//not ideal but we don't have a pointer to the planet we may need to avoid.
			Vector targDir = targetPos-transform.get_position();
			if(!targDir.fast_magnitude())
				return true;
			targDir.normalize();
			SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
			relYaw = fixAngle(relYaw);
			result = rotateShip(relYaw,0,0);
			Vector relDir = targetPos;
			relDir -= transform.get_position();
			moveResult = setPosition(relDir);
		}
	}
	else
	{
		//not ideal but we don't have a pointer to the planet we may need to avoid.
		Vector targDir = targetPos-transform.get_position();
		if(!targDir.fast_magnitude())
			return true;
		targDir.normalize();
		SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
		relYaw = fixAngle(relYaw);
		result = rotateShip(relYaw,0,0);
		Vector relDir = targetPos;
		relDir -= transform.get_position();
		moveResult = setPosition(relDir);
	}

	return moveResult && result;
}
//---------------------------------------------------------------------------
//
template <class Base>
BOOL32 ObjectRepair< Base>::updateRepair (void)
{
	if(repairMode == RM_MOVING_TO_REPAIR)
	{
		if(!(isMoveActive()))
		{
			repairMode = RM_WAITING_TO_REPAIR;
//			U32 agentID = repairAgentID;
//			repairAgentID = 0;
//			THEMATRIX->OperationCompleted(agentID,dwMissionID);
		}
	}
	else if(repairMode == RM_WAITING_TO_REPAIR)
	{
		if(repairAtPlatform)
		{
			if(THEMATRIX->IsMaster())
			{
				MPart repPlat = repairAtPlatform.Ptr();
				bool bNeedRepair = (repPlat->caps.salvageOk) || ((supplies != supplyPointsMax) && repPlat->caps.supplyOk) || ((hullPoints != hullPointsMax) && repPlat->caps.repairOk);
				if((!bNeedRepair) || (! SECTOR->SystemInSupply(repairAtPlatform.Ptr()->GetSystemID(),repairAtPlatform.Ptr()->GetPlayerID())))
				{
					TObjRepBuffer buffer;
					buffer.command = REPAIR_C_CANCEL;
					THEMATRIX->SendOperationData(repairAgentID,dwMissionID,&buffer,sizeof(buffer));

					U32 agentID = repairAgentID;
					repairAtID = 0;
					repairAtPlatform = 0;
					repairAgentID = 0;
					repairMode = RM_NO_MODE;
					THEMATRIX->OperationCompleted(agentID,dwMissionID);
				}
				else if(!(repairAtPlatform->IsDockLocked()))
				{
					TObjRepBuffer buffer;
					buffer.command = REPAIR_C_END;
					THEMATRIX->SendOperationData(repairAgentID,dwMissionID,&buffer,sizeof(buffer));
					U32 agentID = repairAgentID;
					repairAgentID = 0;

					repairAtPlatform->LockDock(this);
					repairMode = RM_DOCKING;

					resetMoveVars();
					disableAutoMovement();
					MGlobals::UpgradeMissionObj(this);//upgrade the tech stuff before starting the repair
					repairAtPlatform->BeginRepairOperation(agentID,this);

					//remove the folling code to properly dock
//					repairMode = RM_DOCKED;
//					repairAtPlatform->RepaireeDocked();
				}
			}
		}
		else
		{
			if(THEMATRIX->IsMaster())
			{
				TObjRepBuffer buffer;
				buffer.command = REPAIR_C_CANCEL;
				THEMATRIX->SendOperationData(repairAgentID,dwMissionID,&buffer,sizeof(buffer));

				U32 agentID = repairAgentID;
				repairAtID = 0;
				repairAtPlatform = 0;
				repairAgentID = 0;
				repairMode = RM_NO_MODE;
				THEMATRIX->OperationCompleted(agentID,dwMissionID);
			}
			else
			{
				repairMode = RM_WAIT_CLIENT_CANCEL;
			}
		}
	}
	else if(repairMode == RM_DOCKING)
	{
		if(repairAtPlatform)
		{
/*			OBJPTR<IPlatform> platform;
			repairAtPlatform->QueryInterface(IPlatformID,platform);
			TRANSFORM trans = platform->GetShipTransform();
		
			Vector my_dir = -(transform.get_k());
			Vector targDir =-(trans.get_k()); 
			SINGLE relYaw = get_angle(targDir.x,targDir.y) - TRANSFORM::get_yaw(my_dir);
			if (relYaw < -PI)
				relYaw += PI*2;
			else if (relYaw > PI)
				relYaw -= PI*2;
			bool rotateResult = rotateShip(relYaw,0,0);
			Vector relDir = trans.get_position()-transform.get_position();
			bool moveResult = setPosition(relDir);
*/			if(moveToRepairPos())
			{
				repairMode = RM_DOCKED;
				repairAtPlatform->RepaireeDocked();
			}
/*			Vector my_dir = -(transform.get_k());
			Vector targDir = repairAtPlatform.ptr->GetPosition() - GetPosition();
			if(targDir.magnitude())
			{
				targDir.normalize();
				SINGLE relYaw = get_angle(targDir.x,targDir.y) - TRANSFORM::get_yaw(my_dir);
				if (relYaw < -PI)
					relYaw += PI*2;
				else if (relYaw > PI)
					relYaw -= PI*2;
				bool rotateResult = rotateShip(relYaw,0,0);
				Vector relDir = trans.get_position()-transform.get_position();
				bool moveResult = setPosition(relDir);
				if(rotateResult)
				{
					if(moveResult)
					{
						repairMode = RM_DOCKED;
						repairAtPlatform->RepaireeDocked();
					}
				}
			}
			else
			{
				repairMode = RM_DOCKED;
				repairAtPlatform->RepaireeDocked();
			}*/
		}
		else
		{
			//assumes that the platform ended the operation
			repairMode = RM_NO_MODE;
		}
	}
	else if(repairMode == RM_DOCKED)
	{
		rotateShip(0,0,0);
		setPosition(Vector(0,0,0));
		if(!repairAtPlatform)
		{
			//assumes that the platform ended the operation
			repairMode = RM_NO_MODE;
		}
	}
	else 
	if(repairMode == RM_RETURNTOLOC)
	{
		if(moveToOldPos())
		{
			repairMode = RM_NO_MODE;
		}
/*		Vector targDir = getLastGrid() - GetPosition();
		SINGLE targDist = targDir.magnitude();
		SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
		relYaw = fixAngle(relYaw);

		bool rotateResult = (targDist < 20) || rotateShip(relYaw,0-transform.get_roll(),0-transform.get_pitch());
		Vector relDir = getLastGrid()-transform.get_position();
		bool moveResult = setPosition(relDir);
		if(rotateResult)
		{
			if(moveResult)
			{
				repairMode = RM_NO_MODE;
			}
		}
*/	}
	else
	if(repairMode == RM_WAIT_CLIENT_CANCEL && THEMATRIX->IsMaster())
	{
		TObjRepBuffer buffer;
		buffer.command = REPAIR_C_CANCEL;
		THEMATRIX->SendOperationData(repairAgentID,dwMissionID,&buffer,sizeof(buffer));

		U32 agentID = repairAgentID;
		repairAtID = 0;
		repairAtPlatform = 0;
		repairAgentID = 0;
		repairMode = RM_NO_MODE;
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
	}
	return true;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectRepair< Base>::cancelRepair (U32 agentID)
{
	if(agentID == repairAgentID)
	{
		if(repairMode == RM_MOVING_TO_REPAIR)
		{
			repairAtID = 0;
			repairAtPlatform = 0;
			repairAgentID = 0;
			repairMode = RM_NO_MODE;
		}
		else
		{
			repairAtID = 0;
			repairAtPlatform = 0;
			repairAgentID = 0;
			repairMode = RM_NO_MODE;
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectRepair< Base>::receiveOpData(U32 agentID, void * buffer, U32 bufferSize)
{
	if(agentID == repairAgentID)
	{
		TObjRepBuffer * buf = (TObjRepBuffer *) buffer;
		switch(buf->command)
		{
		case REPAIR_C_BEGIN_REG:
			{
				repairMode = RM_MOVING_TO_REPAIR;
				TObjRepBufferGrid * myBuf = (TObjRepBufferGrid *) buffer;

				moveToPos(myBuf->grid);
				break;
			}
		case REPAIR_C_BEGIN_MOVE:
			{
				repairMode = RM_NO_MODE;
				TObjRepBufferGrid * myBuf = (TObjRepBufferGrid *) buffer;
				repairAgentID = 0;
				repairAtID = 0;
				repairAtPlatform = 0;

				moveToPos(myBuf->grid,agentID);
				break;
			}
		case REPAIR_C_CANCEL:
			{
				U32 agentID = repairAgentID;
				repairAtID = 0;
				repairAtPlatform = 0;
				repairAgentID = 0;
				repairMode = RM_NO_MODE;
				THEMATRIX->OperationCompleted(agentID,dwMissionID);
				break;
			}
		case REPAIR_C_END:
			{
				repairAgentID = 0;
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectRepair< Base>::preTakeover (U32 newMissionID, U32 troopID)
{
	if(repairAgentID)
	{
		TObjRepBuffer buffer;
		buffer.command = REPAIR_C_CANCEL;
		THEMATRIX->SendOperationData(repairAgentID,dwMissionID,&buffer,sizeof(buffer));

		U32 agentID = repairAgentID;
		repairAtID = 0;
		repairAtPlatform = 0;
		repairAgentID = 0;
		repairMode = RM_NO_MODE;
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
	}
	else if((repairMode == RM_DOCKING) || (repairMode == RM_DOCKED))
	{
		if(repairAtPlatform)
		{
			repairAtPlatform->RepaireeTakeover();
		}
	}

}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectRepair< Base>::resolveRepair ()
{
	if(repairAtID)
	{
		OBJLIST->FindObject(repairAtID, playerID, repairAtPlatform, IRepairPlatformID);
	}
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectRepair< Base>::loadRepair(REPAIRSAVEINFO &loadInfo)
{
	*static_cast<REPAIR_SAVELOAD *> (this) = loadInfo.repair_SL;
}
//---------------------------------------------------------------------------
//
template <class Base>
void ObjectRepair< Base>::saveRepair(REPAIRSAVEINFO &saveInfo)
{
	saveInfo.repair_SL = *static_cast<REPAIR_SAVELOAD *> (this);
}

template <class Base>
void ObjectRepair< Base >::repairInit(const REPAIRINITINFO & data)
{
	HPRepEnumerator hardpointEnum;
	hardpointEnum.repairHardIndex = INVALID_INSTANCE_INDEX;
	EnumerateHardpoints(instanceIndex,"hp_supply",&hardpointEnum);
	if(hardpointEnum.repairHardIndex != INVALID_INSTANCE_INDEX)
	{
		repairHardpoint = hardpointEnum.repairHardpoint;
		repairHardIndex = hardpointEnum.repairHardIndex;
	}
}
//--------------------------------------------------------------------------
//
template <class Base>
void ObjectRepair< Base >::repairMasterChange(bool bIsMaster)
{
	if(bIsMaster)
	{
		if(repairMode == RM_WAIT_BEGIN_CLIENT)
		{
			IBaseObject * platform = OBJLIST->FindObject(repairAtID);
			if(platform)
			{
				platform->QueryInterface(IRepairPlatformID,repairAtPlatform,playerID);
				if(repairAtPlatform)
				{
					repairAtID = platform->GetPartID();
					repairMode = RM_MOVING_TO_REPAIR;
					GRIDVECTOR repPos;
					repPos = platform->GetPosition();

					TObjRepBufferGrid buffer;
					buffer.command = REPAIR_C_BEGIN_REG;
					buffer.grid = repPos;
					THEMATRIX->SendOperationData(repairAgentID,dwMissionID,&buffer,sizeof(buffer));

					moveToPos(repPos);
				}
				else
				{
					U32 agentID = repairAgentID;
					repairAgentID = 0;
					repairMode = RM_NO_MODE;

					GRIDVECTOR repPos;
					repPos = platform->GetPosition();

					TObjRepBufferGrid buffer;
					buffer.command = REPAIR_C_BEGIN_MOVE;
					buffer.grid = repPos;
					THEMATRIX->SendOperationData(agentID,dwMissionID,&buffer,sizeof(buffer));

					moveToPos(repPos,agentID);
				}
			}
			else
			{
				U32 agentID = repairAgentID;
				repairAgentID = 0;
				repairMode = RM_NO_MODE;

				TObjRepBuffer buffer;
				buffer.command = REPAIR_C_CANCEL;
				THEMATRIX->SendOperationData(agentID,dwMissionID,&buffer,sizeof(buffer));

				THEMATRIX->OperationCompleted(agentID,dwMissionID);
			}
		}
	}
}
//--------------------------------------------------------------------------
// we're about to die!!! oh no!
template <class Base>
void ObjectRepair< Base >::preDestructRepair (void)
{
	if((repairMode == RM_DOCKING) || (repairMode == RM_DOCKED))
	{
		if(repairAtPlatform)
		{
			if(repairAtPlatform->IsDockLocked())
				repairAtPlatform->UnlockDock(this);
			repairAtPlatform->RepaireeDestroyed();
		}
	}
	repairMode = RM_NO_MODE;
	repairAgentID = 0;
}

//---------------------------------------------------------------------------
//---------------------------End TObjRepair.h---------------------------------
//---------------------------------------------------------------------------
#endif