//--------------------------------------------------------------------------//
//                                                                          //
//                             SupplyShip.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/SupplyShip.cpp 101   10/19/00 9:30a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "Sector.h"
#include "TSpaceShip.h"
#include <DSpaceShip.h>
#include "TObject.h"
#include "Objlist.h"
#include "Iplanet.h"
#include "Startup.h"
#include "Mission.h"
#include "SysMap.h"
#include "CommPacket.h"
#include "MPart.h"
#include "ObjSet.h"
#include "DSupplyShipSave.h"
#include "DSupplyShip.h"
#include "ISupplier.h"
#include "ILight.h"
#include "CQLight.h"
#include "ObjMapIterator.h"

#include <IConnection.h>
#include <TComponent.h>
#include <TSmartPointer.h>
#include <FileSys.h>

#define NUM_LINE_SEGS 40
#define CIRC_TIME 4.0

//this is so I can add HasOldPackets easily.
#define HOST_CHECK THEMATRIX->IsMaster()

#define NO_SYS_TARG 17

#define COMP_OP(operation) { U32 tempID = workingID; workingID = 0; THEMATRIX->OperationCompleted(tempID,operation);}

#define SUPSYNC_NEWTARGET 0
#define SUPSYNC_OLDTARGET 1
#define SUPSYNC_MOVE 2
#define SUPSYNC_STANCE 3

#define SUPCOMM_ENDOP 0

#define RESUPPLY_TIME 10.0

#pragma pack(push,1)
struct SupplySyncCommand
{
	U8 command;
};
struct SupplySyncNewCommand 
{
	U8 command;
	U32 target;
};
struct SupplySyncMoveCommand
{
	U8 command;
	GRIDVECTOR position;
};
struct SupplySyncStanceCommand
{
	U8 command;
	SUPPLY_SHIP_STANCE supplyStance;
};

#pragma pack(pop)

struct _NO_VTABLE SupplyShip : public SpaceShip<SUPPLYSHIP_SAVELOAD, SUPPLYSHIP_INIT>, ISupplier, BASE_SUPPLYSHIP_SAVELOAD
{
	BEGIN_MAP_INBOUND(SupplyShip)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IGotoPos)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(ISupplier)
	_INTERFACE_ENTRY(IRepairee)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IShipMove)
	_INTERFACE_ENTRY(ICloak)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()

	struct TCallback : ITerrainSegCallback
	{
		TCallback (void)
		{
		}

		virtual bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)	// return false to stop callback
		{
			if (info.flags & TERRAIN_BLOCKLOS)
			{

				return false;
			}
			return true;
		}
	};

	UpdateNode updateNode;
	SaveNode	saveNode;
	LoadNode	loadNode;
	InitNode		initNode;
	PhysUpdateNode  physUpdateNode;
	OnOpCancelNode	onOpCancelNode;
	ReceiveOpDataNode	receiveOpDataNode;
	PreDestructNode	destructNode;
	GeneralSyncNode  genSyncNode;
	PreTakeoverNode	preTakeoverNode;

	S32 mapDotCycle;

	OBJPTR<IBaseObject> supplyEscortTarget;
	OBJPTR<IBaseObject> supplyPlatformTarget;
	OBJPTR<IBaseObject> targetedTarget;
	SINGLE supplyRange;
	SINGLE suppliesPerSecond;

	SINGLE circleTime;

	SUPPLY_SHIP_STANCE netStance;

	enum SUP_NET_COMMANDS
	{
		P_OP_RESUPPLY,
		P_OP_FORGET_RESUPPLY
	};
		
	SupplyShip (void);

	virtual ~SupplyShip (void);

	/* ISupplier methods */

  	virtual void SupplyTarget(U32 agentID, IBaseObject * obj);

	virtual void SetSupplyEscort (U32 agentID, IBaseObject * target);

	virtual void SetSupplyStance (enum SUPPLY_SHIP_STANCE _stance);

	virtual enum SUPPLY_SHIP_STANCE GetSupplyStance ();
	/* IGotoPos methods */


	/* SpaceShip methods */
	
	void SupplyShip::ResolveAssociations (void);

	virtual const char * getSaveStructName (void) const
	{
		return "SUPPLYSHIP_SAVELOAD";
	}

	virtual void * getViewStruct (void)	
	{
		return static_cast<BASE_SUPPLYSHIP_SAVELOAD *>(this);
	}
	/* IMissionActor */

	// the following are process related
	// called in response to OpAgent::CreateOperation()
	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize);

	// called in response to OpAgent::SendOperationData()
	void receiveOperationData (U32 agentID, void *buffer, U32 bufferSize);

	// user has requested a different action
	void onOperationCancel (U32 agentID);
		
	void preSelfDestruct (void);

	void preTakeover (U32 newMissionID, U32 troopID);

	virtual void OnStopRequest (U32 agentID);

	virtual void OnMasterChange (bool bIsMaster);

	/* IBaseObject */

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);
	
	virtual void Render (void);

	virtual void MapRender(bool bPing);

//	virtual void OnAddToOperation (U32 agentID);
	/* Fabricator methods */

	bool checkLOS (TCallback & callback, IBaseObject * targ);	// with targetPos

	BOOL32 seekSupplyTarget (IBaseObject * targ);

	void * findNearestResupply (OBJPTR<IBaseObject> & supplyTarget);

	BOOL32 updateSupply (void);

	void physUpdateSupply (SINGLE dt);

	void renderSupply ();

	void initSupply (const SUPPLYSHIP_INIT & data);

	void save (SUPPLYSHIP_SAVELOAD & save);
	void load (SUPPLYSHIP_SAVELOAD & load);

	void localMovToPos (const Vector & _vec, U32 agentID=0)
	{
		GRIDVECTOR vec;
		vec = _vec;
		CQASSERT((agentID || workingID) && "Supply ship moving without an agent!");
		moveToPos(vec, agentID);
	}

	U32 getSyncData (void * buffer);
	void putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery);
};
//---------------------------------------------------------------------------
//
SupplyShip::SupplyShip (void) : 
	updateNode(this, UpdateProc(&SupplyShip::updateSupply)),
    physUpdateNode(this, PhysUpdateProc(&SupplyShip::physUpdateSupply)),
	saveNode(this, CASTSAVELOADPROC(&SupplyShip::save)),
	loadNode(this, CASTSAVELOADPROC(&SupplyShip::load)),
	initNode(this, CASTINITPROC(&SupplyShip::initSupply)),
	onOpCancelNode(this, OnOpCancelProc(&SupplyShip::onOperationCancel)),
	receiveOpDataNode(this, ReceiveOpDataProc(&SupplyShip::receiveOperationData)),
	destructNode(this, PreDestructProc(&SupplyShip::preSelfDestruct)),
	genSyncNode(this, SyncGetProc(&SupplyShip::getSyncData), SyncPutProc(&SupplyShip::putSyncData)),
	preTakeoverNode(this, PreTakeoverProc(&SupplyShip::preTakeover))
{
	circleTime = 0;
}
//---------------------------------------------------------------------------
//
SupplyShip::~SupplyShip (void)
{
}
//---------------------------------------------------------------------------
//
void SupplyShip::initSupply (const SUPPLYSHIP_INIT & data)
{
	const BT_SUPPLYSHIP_DATA * objData = data.pData;
	mode = SUP_IDLE;
	supplyRange = objData->supplyRange;
	suppliesPerSecond = objData->suppliesPerSecond;

	supplyPoint.systemID = NO_SYS_TARG;
	supplyEscortTarget = NULL;
	supplyEscortTargetID = 0;
	supplyStance = SUP_STANCE_RESUPPLY;

}
//---------------------------------------------------------------------------
// return TRUE if we have a clear LineOfSight with the target
//
bool SupplyShip::checkLOS (TCallback & callback, IBaseObject * targ)	// with targetPos
{
	COMPTR<ITerrainMap> map;

	SECTOR->GetTerrainMap(GetSystemID(), map);

	return map->TestSegment(GetGridPosition(),targ->GetGridPosition(), &callback);
}
//---------------------------------------------------------------------------
//
BOOL32 SupplyShip::seekSupplyTarget (IBaseObject * targ)
{
	CQASSERT(workingID && "Supply ship moving without an agent!");
	if ((supplyRange > 0) && targ && (!isMoveActive()))
	{
		//
		// set pivot point for attacks
		//
		Vector dir = transform.get_position();
		TCallback callback;
		bool bLOS = checkLOS(callback,targ);     // true if we have lineOfSight on target

		SINGLE dirMag = targ->GetGridPosition() - GetGridPosition();

		SINGLE realRange = supplyRange* MGlobals::GetTenderUpgrade(this);
		SINGLE optimalSupply =  realRange * 0.75;
		if (bLOS==0 || dirMag > realRange || dirMag >= optimalSupply)
		{
			bNeedToSendMoveToTarget = true;
			seekTarget = targ->GetGridPosition();
			return true;
		}
	}
	return false;
}

//---------------------------------------------------------------------------
//
void * SupplyShip::findNearestResupply (OBJPTR<IBaseObject> & supplyTarget)
{
	if(systemID & HYPER_SYSTEM_MASK)
	{
		supplyTarget = NULL;
		return NULL;
	}
	IBaseObject * obj = OBJLIST->GetTargetList();
	IBaseObject * closest = NULL;
	U32 closeSector = 0;
	SINGLE closeDist2 = 0;
	bool closeInSystem = false;
	while(obj)
	{
		if((obj->objClass == OC_PLATFORM) && (GetPlayerID() == obj->GetPlayerID()))
		{
			MPart part(obj);
			if((part->caps.supplyOk) && (part->bReady) && (obj->GetSystemID() <= MAX_SYSTEMS) &&
				(obj->GetSystemID()))
			{
				if(closest)
				{
					if(GetSystemID() != obj->GetSystemID())
					{
						if(!closeInSystem)
						{
							U32 list[16];
							U32 numJumps = SECTOR->GetShortestPath(GetSystemID(), obj->GetSystemID(), list, GetPlayerID());
							if(numJumps < closeSector)
							{
								closest = obj;
								closeSector = numJumps;
							}
						}
					}
					else
					{
						if(!closeInSystem)
						{
							closeInSystem = true;
							Vector dist = (GetPosition()-obj->GetPosition());
							closeDist2 = (dist.x*dist.x)+(dist.y*dist.y)+(dist.z*dist.z);
							closest = obj;
						}
						else
						{
							Vector dist = (GetPosition()-obj->GetPosition());
							SINGLE testDist = (dist.x*dist.x)+(dist.y*dist.y)+(dist.z*dist.z);
							if(testDist < closeDist2)
							{
								closeDist2 = testDist;
								closest = obj;
							}
						}
					}

				} else
				{
					closest = obj;
					U32 list[16];
					closeSector = 0; 
					closeDist2 = 0;
					if(GetSystemID() != obj->GetSystemID())
					{
						closeSector = SECTOR->GetShortestPath(GetSystemID(), obj->GetSystemID(), list, GetPlayerID());
					}
					else
					{
						closeInSystem = true;
						Vector dist = (GetPosition()-obj->GetPosition());
						closeDist2 = (dist.x*dist.x)+(dist.y*dist.y)+(dist.z*dist.z);
					}
				}
			}
		}
		obj = obj->nextTarget;
	}

	if (closest)
		closest->QueryInterface(IBaseObjectID, supplyTarget, NONSYSVOLATILEPTR);
	else
		supplyTarget=0;

	return closest;

}
//---------------------------------------------------------------------------
//
BOOL32 SupplyShip::updateSupply (void)
{
	if(HOST_CHECK && bReady && !bExploding )
	{
		SINGLE realSupplyRange = supplyRange*MGlobals::GetTenderUpgrade(this);
		//a resupply is in progress
		if(MGlobals::IsUpdateFrame(dwMissionID))
		{
			if(mode == 	SUP_RESUPPLY_ESCORT)
			{
				if(supplyEscortTarget)
				{
					GRIDVECTOR supPoint = supplyEscortTarget->GetGridPosition();
					GRIDVECTOR myPoint = GetGridPosition();
					if(supplies == 0 && supplyStance == SUP_STANCE_FULLYAUTO && !THEMATRIX->GetWorkingOp(this))
					{
						findNearestResupply(supplyPlatformTarget);

						if(supplyPlatformTarget)
						{
							U8 buffer = SUPCOMM_ENDOP;
							THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
							THEMATRIX->OperationCompleted2(workingID,dwMissionID);
							supplyEscortTargetID = 0;
							bNeedToSendMoveToTarget = false;
							mode = SUP_IDLE;

							USR_PACKET<USRSHIPREPAIR> packet;
							packet.objectID[0] = dwMissionID;
							packet.targetID = supplyPlatformTarget->GetPartID();
							packet.init(1);
							NETPACKET->Send(HOSTID, 0, &packet);

							USR_PACKET<USRESCORT> defPacket;
							defPacket.objectID[0] = dwMissionID;
							defPacket.targetID = supplyEscortTarget->GetPartID();
							defPacket.userBits = 1;
							defPacket.init(1);
							NETPACKET->Send(HOSTID, 0, &defPacket);
						}
					}
					else if(supplyEscortTarget->GetSystemID() != systemID)
					{
						U8 buffer = SUPCOMM_ENDOP;
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
						THEMATRIX->OperationCompleted2(workingID,dwMissionID);
						supplyEscortTargetID = 0;
						bNeedToSendMoveToTarget = false;
						mode = SUP_IDLE;

						USR_PACKET<USRESCORT> packet;
						packet.objectID[0] = dwMissionID;
						packet.targetID = supplyEscortTarget->GetPartID();
						packet.init(1);
						NETPACKET->Send(HOSTID, 0, &packet);
					}
					else if((supplyEscortTarget->GetSystemID() == systemID) 
						&& (supPoint-myPoint) > realSupplyRange)//too far off point;
					{
						seekSupplyTarget(supplyEscortTarget);
					}
				}
				else
				{
					U8 buffer = SUPCOMM_ENDOP;
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					THEMATRIX->OperationCompleted2(workingID,dwMissionID);
					supplyEscortTargetID = 0;
					bNeedToSendMoveToTarget = false;
					mode = SUP_IDLE;
				}
			}
			else if(mode == SUP_MOVING_TARGETED_SUPPLY)
			{
				if(targetedTarget)
				{
					MPart part(targetedTarget);
					if(part->supplies == part->supplyPointsMax || targetedTarget->GetSystemID() != systemID)
					{
						U8 buffer = SUPCOMM_ENDOP;
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
						THEMATRIX->OperationCompleted2(workingID,dwMissionID);
						bNeedToSendMoveToTarget = false;
						mode = SUP_IDLE;
					}
					else
					{
						bool bHasRange = (GetGridPosition()-targetedTarget->GetGridPosition() <= realSupplyRange);
						if(!isMoveActive())
						{
							if(!bHasRange)
							{
								moveToPos(targetedTarget->GetGridPosition());
							}
						}
						if(bHasRange)
						{
							U8 buffer = SUPCOMM_ENDOP;
							THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
							THEMATRIX->OperationCompleted2(workingID,dwMissionID);
							bNeedToSendMoveToTarget = false;
							mode = SUP_IDLE;
						}
					}
				}
				else
				{
					U8 buffer = SUPCOMM_ENDOP;
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
					THEMATRIX->OperationCompleted2(workingID,dwMissionID);
					bNeedToSendMoveToTarget = false;
					mode = SUP_IDLE;
				}
			}
			else if(supplyStance == SUP_STANCE_FULLYAUTO && !repairAgentID && !THEMATRIX->GetWorkingOp(this))
			{
				if(mode == SUP_MOVING_TO_TENDER)
				{
					if(supplies != 0)
					{
						mode = SUP_IDLE;
					}
				}
				else if(supplies == 0)
				{
					findNearestResupply(supplyPlatformTarget);

					if(supplyPlatformTarget)
					{
						USR_PACKET<USRSHIPREPAIR> packet;
						packet.objectID[0] = dwMissionID;
						packet.targetID = supplyPlatformTarget->GetPartID();
						packet.init(1);
						NETPACKET->Send(HOSTID, 0, &packet);

						USR_PACKET<USRMOVE> defPacket;
						defPacket.objectID[0] = dwMissionID;
						defPacket.position.init(GetGridPosition(),systemID);
						defPacket.userBits = 1;
						defPacket.init(1);
						NETPACKET->Send(HOSTID, 0, &defPacket);

						mode = SUP_MOVING_TO_TENDER;
					}
				}
			}
		}

		supplyTimer += ELAPSED_TIME;
		if(supplyTimer > RESUPPLY_TIME)
		{
			supplyTimer -=RESUPPLY_TIME;
			U32 resupplyAmount = RESUPPLY_TIME*suppliesPerSecond;
			if(resupplyAmount && supplies && !fieldFlags.suppliesLocked())
			{
				//supplyShip Stuff
				if(supplyStance == 	SUP_STANCE_RESUPPLY || supplyStance == SUP_STANCE_FULLYAUTO)
				{
					IBaseObject * searchObj = NULL;
					ObjMapIterator iter(systemID,GetPosition(),realSupplyRange*GRIDSIZE,playerID);
					while(iter && supplies)
					{
						U32 hisPlayerID=iter.GetApparentPlayerID(MGlobals::GetAllyMask(playerID));
						searchObj = iter->obj;
						if(hisPlayerID && MGlobals::AreAllies(hisPlayerID,playerID))
						{
							MPartNC part(searchObj);
							if(part.isValid() && part->bReady && (part->mObjClass != M_FABRICATOR) && (part->mObjClass != M_HARVEST) && 
								(part->mObjClass != M_GALIOT) &&(part->mObjClass != M_SIPHON) && (part->mObjClass != M_SUPPLY) && 
								(part->mObjClass != M_ZORAP)&& (part->mObjClass != M_STRATUM)
								&& (part->supplyPointsMax != 0) && (searchObj->GetGridPosition()-GetGridPosition() < realSupplyRange) &&
								part->supplies < part->supplyPointsMax)
							{
								U32 need = part->supplyPointsMax - part->supplies;
								if(need > resupplyAmount)
									need = resupplyAmount;
								if(need > supplies)
									need = supplies;
								part->supplies += need;
								supplies -= need;
							}
						}
						++iter;
					}
				}
			}// end if mode == idle or idle op
		}//end if is update frame
	}
	return true;
}
//---------------------------------------------------------------------------
//
void SupplyShip::physUpdateSupply (SINGLE dt)
{
}
//---------------------------------------------------------------------------
//
void SupplyShip::renderSupply ()
{
	if((systemID == SECTOR->GetCurrentSystem()) && (bHighlight || bSelected || BUILDARCHEID) && bReady && MGlobals::AreAllies(playerID,MGlobals::GetThisPlayer()))
	{
		circleTime += 0.03;
		while(circleTime >CIRC_TIME)
		{
			circleTime -= CIRC_TIME;
		}

		BATCH->set_state(RPR_BATCH,true);
		DisableTextures();
		CAMERA->SetModelView();
		BATCH->set_state(RPR_STATE_ID,0);
		BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		PB.Begin(PB_LINES);
		COLORREF color;
//		if(SECTOR->SystemInSupply(systemID,playerID))
//		{
			PB.Color3ub(64,128,64);
			color = RGB(64,128,64);
//		}
//		else
//		{
//			PB.Color3ub(128,64,64);
//			color = RGB(128,64,64);
//		}
		SINGLE realRange = supplyRange*MGlobals::GetTenderUpgrade(this);
		Vector relDir =Vector(cos((2*PI*circleTime)/CIRC_TIME)*realRange*GRIDSIZE,
					sin((2*PI*circleTime)/CIRC_TIME)*realRange*GRIDSIZE,0);
		Vector oldVect = transform.translation+relDir;
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		oldVect = transform.translation-relDir;
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		PB.End();
		drawRangeCircle(realRange,color);
	}
}
//---------------------------------------------------------------------------
//
void SupplyShip::OnStopRequest (U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void SupplyShip::OnMasterChange (bool bIsMaster)
{
	repairMasterChange(bIsMaster);
	if(bIsMaster)
	{
		if(mode == SUP_CLIENT_IDLE)
		{
			IBaseObject * obj = OBJLIST->FindObject(targetedTargetID);
			bool bOpCancel = true;
			if(obj)
			{
				if((obj->GetSystemID() == systemID) && (obj->GetPlayerID() == playerID) && (obj->objClass == OC_SPACESHIP))
				{
					MPart shipPart(obj);
					if((shipPart->mObjClass != M_FABRICATOR) && (shipPart->mObjClass != M_HARVEST) 
						&&	(shipPart->mObjClass != M_GALIOT) &&(shipPart->mObjClass != M_SIPHON) && (shipPart->mObjClass != M_SUPPLY)
						 && (shipPart->mObjClass != M_ZORAP) && (shipPart->mObjClass != M_STRATUM) )
					{
						bOpCancel = false;
						mode = SUP_MOVING_TARGETED_SUPPLY;
						moveToPos(obj->GetGridPosition());
					}
				}
			}

			if(bOpCancel)
			{
				targetedTargetID = 0;
				targetedTarget = NULL;
				U8 buffer = SUPCOMM_ENDOP;
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
				THEMATRIX->OperationCompleted2(workingID,dwMissionID);
				bNeedToSendMoveToTarget = false;
				mode = SUP_IDLE;
			}			
		}
	}
	else
	{
		if(mode == SUP_MOVING_TARGETED_SUPPLY)
		{
			mode = SUP_CLIENT_IDLE;
		}
	}
}
//---------------------------------------------------------------------------
//
void SupplyShip::OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void SupplyShip::receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
{
	if(agentID == workingID)
	{
		U8 * buf = (U8 *) buffer;
		if(buf[0] == SUPCOMM_ENDOP)
		{
			targetedTargetID = 0;
			targetedTarget = NULL;
			supplyEscortTargetID = 0;
			supplyEscortTarget = NULL;
			mode = SUP_IDLE;
			THEMATRIX->OperationCompleted2(workingID,dwMissionID);
			bNeedToSendMoveToTarget = false;
		}
	}
}
//---------------------------------------------------------------------------
//
void SupplyShip::onOperationCancel (U32 agentID)
{
	if(workingID == agentID)
	{
		workingID = 0;
		targetedTargetID = 0;
		targetedTarget = NULL;
		supplyEscortTargetID = 0;
		supplyEscortTarget = NULL;
		bNeedToSendMoveToTarget = false;
		mode = SUP_IDLE;
	}
}
//---------------------------------------------------------------------------
//
void SupplyShip::preSelfDestruct (void)
{
	if(workingID && THEMATRIX->IsMaster())
	{
		U8 buffer = SUPCOMM_ENDOP;
		THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
		THEMATRIX->OperationCompleted2(workingID,dwMissionID);
		mode = SUP_IDLE;
		targetedTargetID = 0;
		targetedTarget = NULL;
		supplyEscortTargetID = 0;
		supplyEscortTarget = NULL;
		bNeedToSendMoveToTarget = false;
	}
}
//---------------------------------------------------------------------------
//
void SupplyShip::preTakeover (U32 newMissionID, U32 troopID)
{
	if(workingID && THEMATRIX->IsMaster())
	{
		U8 buffer = SUPCOMM_ENDOP;
		THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
		THEMATRIX->OperationCompleted2(workingID,dwMissionID);
		mode = SUP_IDLE;
		targetedTargetID = 0;
		targetedTarget = NULL;
		supplyEscortTargetID = 0;
		supplyEscortTarget = NULL;
		bNeedToSendMoveToTarget = false;
	}
}
//---------------------------------------------------------------------------
//
U32 SupplyShip::getSyncData (void * buffer)			// buffer points to use supplied memory
{
	U32 size = 0;
	
	U8 * buf = (U8 *)buffer;

	if(bNeedToSendMoveToTarget)
	{
		SupplySyncMoveCommand * command = (SupplySyncMoveCommand *)(buf+size);
		command->command = SUPSYNC_MOVE;
		command->position = seekTarget;
		CQASSERT(workingID && "Supply ship moving without an agent!");
		moveToPos(seekTarget);
		size += sizeof(SupplySyncMoveCommand);
		bNeedToSendMoveToTarget = false;
	}
	if(supplyStance != netStance)
	{
		SupplySyncStanceCommand * command = (SupplySyncStanceCommand *)(buf+size);
		command->command = SUPSYNC_STANCE;
		command->supplyStance = supplyStance;
		size += sizeof(SupplySyncStanceCommand);
		netStance = supplyStance;
	}

	return size;
}
//---------------------------------------------------------------------------
//
void SupplyShip::putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	U32 offset = 0;
	U8 * buf = (U8 *)buffer;

	while(offset < bufferSize)
	{
		SupplySyncCommand * command = (SupplySyncCommand *)(buf+offset);
		if(command->command == SUPSYNC_MOVE)
		{
			SupplySyncMoveCommand * newCommand = (SupplySyncMoveCommand *)(buf+offset);
			offset += sizeof(SupplySyncMoveCommand);
			if (bLateDelivery==0)
			{
				CQASSERT(workingID && "Supply ship moving without an agent!");
				moveToPos(newCommand->position);
			}
		}
		else
		if(command->command == SUPSYNC_STANCE)
		{
			SupplySyncStanceCommand * newCommand = (SupplySyncStanceCommand *)(buf+offset);
			offset += sizeof(SupplySyncStanceCommand);
			SetSupplyStance(newCommand->supplyStance);
		}
		else
			CQBOMB0("Invalid data received");
	}

	CQASSERT(offset==bufferSize);		// should use exact size of data
}
//---------------------------------------------------------------------------
//
void SupplyShip::SupplyTarget(U32 agentID, IBaseObject * obj)
{
	CQASSERT(!workingID);

	if(HOST_CHECK)
	{
		bool bOpCancel = true;
		if(obj)
		{
			if((obj->GetSystemID() == systemID) && (obj->GetPlayerID() == playerID) && (obj->objClass == OC_SPACESHIP))
			{
				MPart shipPart(obj);
				if((shipPart->mObjClass != M_FABRICATOR) && (shipPart->mObjClass != M_HARVEST) 
					&&	(shipPart->mObjClass != M_GALIOT) &&(shipPart->mObjClass != M_SIPHON) && (shipPart->mObjClass != M_SUPPLY)
					 && (shipPart->mObjClass != M_ZORAP) && (shipPart->mObjClass != M_STRATUM) )
				{
					targetedTargetID = obj->GetPartID();
					obj->QueryInterface(IBaseObjectID,targetedTarget,NONSYSVOLATILEPTR);
					bOpCancel = false;
					workingID = agentID;
					mode = SUP_MOVING_TARGETED_SUPPLY;
					moveToPos(obj->GetGridPosition());
				}
			}
		}

		if(bOpCancel)
		{
			U8 buffer = SUPCOMM_ENDOP;
			THEMATRIX->SendOperationData(agentID,dwMissionID,&buffer,sizeof(buffer));
			THEMATRIX->OperationCompleted(agentID,dwMissionID);
		}
	}
	else
	{
		if(obj)
		{
			targetedTargetID = obj->GetPartID();
			obj->QueryInterface(IBaseObjectID,targetedTarget,NONSYSVOLATILEPTR);
		}
		else
		{
			targetedTargetID = 0;
			targetedTarget = NULL;
		}
		workingID =agentID;
		mode = SUP_CLIENT_IDLE;
	}
}

//---------------------------------------------------------------------------
//
void SupplyShip::SetSupplyEscort (U32 agentID, IBaseObject * target)
{
	workingID = agentID;
	if(target)
	{
		target->QueryInterface(IBaseObjectID, supplyEscortTarget, NONSYSVOLATILEPTR);
		supplyEscortTargetID = target->GetPartID();
		supplyPoint.systemID = NO_SYS_TARG;
	}
	else
	{
		supplyEscortTarget = NULL;
		supplyEscortTargetID = 0;
		supplyPoint.systemID = NO_SYS_TARG;
	}

	mode = 	SUP_RESUPPLY_ESCORT;
}
//---------------------------------------------------------------------------
//
void SupplyShip::SetSupplyStance (enum SUPPLY_SHIP_STANCE _stance)
{
	supplyStance = _stance;
}
//---------------------------------------------------------------------------
//
enum SUPPLY_SHIP_STANCE SupplyShip::GetSupplyStance ()
{
	return supplyStance;
}
//---------------------------------------------------------------------------
//
void SupplyShip::save (SUPPLYSHIP_SAVELOAD & save)
{
	save.baseSaveLoad = *static_cast<BASE_SUPPLYSHIP_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void SupplyShip::load (SUPPLYSHIP_SAVELOAD & load)
{
	*static_cast<BASE_SUPPLYSHIP_SAVELOAD *>(this) = load.baseSaveLoad;
}
//---------------------------------------------------------------------------
//
void SupplyShip::ResolveAssociations (void)
{
	FRAME_resolve();
	if(targetedTargetID)
	{
		OBJLIST->FindObject(targetedTargetID,NONSYSVOLATILEPTR,targetedTarget);
	}
	if(supplyEscortTargetID)
	{
		OBJLIST->FindObject(supplyEscortTargetID, NONSYSVOLATILEPTR, supplyEscortTarget);
	}
	if(supplyPlatformTargetID)
	{
		OBJLIST->FindObject(supplyPlatformTargetID, NONSYSVOLATILEPTR, supplyPlatformTarget);
	}
}
//---------------------------------------------------------------------------
//
void SupplyShip::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	SpaceShip<SUPPLYSHIP_SAVELOAD, SUPPLYSHIP_INIT>::TestVisible(defaults, currentSystem, currentPlayer);
}
//-------------------------------------------------------------------
//
void SupplyShip::Render (void)
{
	SpaceShip<SUPPLYSHIP_SAVELOAD, SUPPLYSHIP_INIT>::Render();

	renderSupply();
}
//-------------------------------------------------------------------
//
void SupplyShip::MapRender(bool bPing)
{
	SpaceShip<SUPPLYSHIP_SAVELOAD, SUPPLYSHIP_INIT>::MapRender(bPing);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createSupplyShip (const SUPPLYSHIP_INIT & data)
{
	SupplyShip * obj = new ObjectImpl<SupplyShip>;

	obj->FRAME_init(data);
	return obj;
}

//------------------------------------------------------------------------------------------
//---------------------------SupplyShip Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE SupplyShipFactory : public IObjectFactory
{
	struct OBJTYPE : SUPPLYSHIP_INIT
	{
		~OBJTYPE (void)
		{
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(SupplyShipFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	SupplyShipFactory (void) { }

	~SupplyShipFactory (void);

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

	/* FabricatorFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
SupplyShipFactory::~SupplyShipFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void SupplyShipFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE SupplyShipFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_SPACESHIP)
	{
		BT_SUPPLYSHIP_DATA * data = (BT_SUPPLYSHIP_DATA *) _data;


		if (data->type == SSC_SUPPLYSHIP)	   
		{
			result = new OBJTYPE;

			if (result->loadSpaceshipArchetype(data, ARCHLIST->GetArchetype(szArchname)) == false)
				goto Error;
		}
		goto Done;
	}

Error:
	delete result;
	result = 0;
Done:
	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 SupplyShipFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * SupplyShipFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createSupplyShip(*objtype);
}
//-------------------------------------------------------------------
//
void SupplyShipFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _supplyship : GlobalComponent
{
	SupplyShipFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<SupplyShipFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _supplyship __ship;

//---------------------------------------------------------------------------
//--------------------------End SupplyShip.cpp--------------------------------
//---------------------------------------------------------------------------