//--------------------------------------------------------------------------//
//                                                                          //
//                             Minelayer.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MineLayer.cpp 86    5/07/01 9:22a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObjTrans.h"
#include "TObjControl.h"
#include "TObjFrame.h"
#include "TSpaceShip.h"
#include <DSpaceShip.h>
#include "TObject.h"
#include "Objlist.h"
#include "Startup.h"
#include "Mission.h"
#include "IAttack.h"
#include "Field.h"
#include "IMinefield.h"
#include "DMinefield.h"
#include "OpAgent.h"
#include "IHardPoint.h"
#include "Cursor.h"

#include <TComponent.h>
#include <IConnection.h>
#include <TSmartPointer.h>
#include <WindowManager.h>

enum MINELAYER_NETCODES
{
	MINELAYER_NET_START = 1,
	MINELAYER_NET_START_FAIL,
	MINEFIELD_NET_BEGIN_LAYING,
	MINEFIELD_NET_RESTART_LAYING,
	MINEFIELD_NET_FINISH_LAYING,
};

struct MineLayerCommand
{
	U8 command;
};

struct MineLayerCommandPos
{
	U8 command;
	GRIDVECTOR position;
};

struct MineLayerCommandHandle
{
	U8 command;
	U32 handle;
};

struct _NO_VTABLE Minelayer : public SpaceShip<MINELAYER_SAVELOAD, MINELAYER_INIT>, IAttack, BASE_MINELAYER_SAVELOAD
{
	BEGIN_MAP_INBOUND(Minelayer)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IGotoPos)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IAttack)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IRepairee)
	_INTERFACE_ENTRY(IShipMove)
	_INTERFACE_ENTRY(ICloak)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()

	SaveNode	saveNode;
	LoadNode	loadNode;
	UpdateNode		updateNode;
	PhysUpdateNode physUpdateNode;
	ResolveNode		resolveNode;
	OnOpCancelNode	onOpCancelNode;
	ReceiveOpDataNode	receiveOpDataNode;
	PreDestructNode	destructNode;
	PreTakeoverNode preTakeoverNode;

	MINELAYER_INIT * pArch;

	OBJPTR<IMinefield> targetField;
	HardpointInfo  hardpointinfo;
	INSTANCE_INDEX mineReleaseIndex;
	SCRIPT_INST animReleaseIndex;
	SINGLE animDuration;
	SCRIPT_SET_ARCH animSetArch;
	bool bHasHardpoint:1;
	bool bDropedForAnim:1;

	Minelayer (void);

	virtual ~Minelayer (void);

	virtual void Render();

	/* IGotoPos methods */


	/* SpaceShip methods */
	
	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "MINELAYER_SAVELOAD";
	}
	virtual void * getViewStruct (void) 	// must be overriden implemented by derived class
	{
		return static_cast<BASE_MINELAYER_SAVELOAD *>(this);
	}

	/* IAttack methods */

	virtual void Attack (IBaseObject * victim, U32 agentID, bool bUserGenerated);

	virtual void AttackPosition(const struct GRIDVECTOR & position, U32 agentID); 

	virtual void CancelAttack (void);

	virtual void SpecialAttack (IBaseObject * victim, U32 agentID);

	virtual void SpecialAOEAttack (const struct GRIDVECTOR & position, U32 agentID);

	virtual void WormAttack (IBaseObject * victim, U32 agentID);

	virtual void ReportKill (U32 partID);

	virtual void Escort (IBaseObject * target, U32 agentID)
	{
		CQBOMB0("Not implemented!");
	}

	virtual const bool TestFightersRetracted (void) const 
	{ 
		return true;
	}

	virtual void DoSpecialAbility (U32 specialID)
	{
	}

	virtual void MultiSystemAttack(struct GRIDVECTOR & position, U32 targSystemID,U32 agentID)
	{
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
	}

	virtual void DoCreateWormhole(U32 systemID, U32 agentID)
	{
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
	}

	virtual void DoSpecialAbility(IBaseObject * obj)
	{
	}

	virtual void DoCloak (void)
	{
	}
	
	virtual void SetUnitStance (const UNIT_STANCE stance)
	{
	}

	virtual void SetFighterStance (const FighterStance stance)
	{
	}

	virtual const FighterStance GetFighterStance (void)
	{
		return FS_NORMAL;
	}

	virtual const UNIT_STANCE GetUnitStance () const
	{
		return US_STOP;
	}

	virtual void GetTarget(IBaseObject* & targObj, U32 targID);

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bSpecialEnabled)
	{
		ability = USA_MINELAYER;
		bSpecialEnabled =  (supplies > pArch->pData->fieldSupplyCostPerMine);
//		ability = USA_NONE;
//		bSpecialEnabled = false;
	}

	virtual void OnAllianceChange (U32 allyMask)
	{
		// do nothing
	}

	/* IMissionActor */

	virtual void OnMasterChange (bool bIsMaster);

	// the following are process related
	// called in response to OpAgent::CreateOperation()
	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize);

	// called in response to OpAgent::SendOperationData()
	virtual void receiveOperationData (U32 agentID, void *buffer, U32 bufferSize);

	// user has requested a different action
	virtual void onOperationCancel (U32 agentID);
		
	virtual void preSelfDestruct (void);

	virtual void preTakeover (U32 newMissionID, U32 troopID);

	/* Minelayer methods */

	void save (MINELAYER_SAVELOAD & save);
	void load (MINELAYER_SAVELOAD & load);

	BOOL32 update (void);

	void physicalUpdate (SINGLE dt);

	void resolve (void);

	bool initMinelayer (const MINELAYER_INIT & data);

//	BOOL32 findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent);

//	S32 findChild (const char * pathname, INSTANCE_INDEX parent);
};
//---------------------------------------------------------------------------
//
Minelayer::Minelayer (void) : 
		saveNode(this, CASTSAVELOADPROC(&Minelayer::save)),
		loadNode(this, CASTSAVELOADPROC(&Minelayer::load)),
		resolveNode(this, ResolveProc(&Minelayer::resolve)),
		updateNode(this, UpdateProc(&Minelayer::update)),
		physUpdateNode(this, PhysUpdateProc(&Minelayer::physicalUpdate)),
		onOpCancelNode(this, OnOpCancelProc(&Minelayer::onOperationCancel)),
		receiveOpDataNode(this, ReceiveOpDataProc(&Minelayer::receiveOperationData)),
		destructNode(this, PreDestructProc(&Minelayer::preSelfDestruct)),
		preTakeoverNode(this, PreTakeoverProc(&Minelayer::preTakeover))
{

}
//---------------------------------------------------------------------------
//
Minelayer::~Minelayer (void)
{
}
//---------------------------------------------------------------------------
//
void Minelayer::Render (void)
{
	SpaceShip<MINELAYER_SAVELOAD, MINELAYER_INIT>::Render();
	if(bSelected && (CURSOR->GetCursor() == IDC_CURSOR_SPECIAL_ATTACK))
	{
		Vector vec;
		
		SINGLE fieldWidth = GRIDSIZE;

		S32 mouseX, mouseY;
		WM->GetCursorPos(mouseX,mouseY);
		vec.x = mouseX;
		vec.y = mouseY;
		BATCH->set_state(RPR_BATCH,FALSE);
		DisableTextures();
		
		CAMERA->SetPerspective();
		CAMERA->SetModelView();
		BATCH->set_render_state(D3DRS_ZENABLE,0);
		
		if (CAMERA->ScreenToPoint(vec.x, vec.y, 0) != 0)
		{
			GRIDVECTOR gVect;
			gVect = vec;
			gVect = gVect.centerpos();
						
			IField *field =  FIELDMGR->GetFields();
			OBJPTR<IMinefield> mineField;
			while(field)
			{
				if(field->fieldType == FC_MINEFIELD)
				{
					field->QueryInterface(IMinefieldID,mineField);
					if(mineField != 0)
					{
						if(mineField->AtPosition(gVect) &&
							(mineField->GetLayerRef() || mineField->GetMineNumber()))
						{
							break;
						}
					}
					mineField = NULL;
				}
				field = field->nextField;
			}
			PB.Begin(PB_LINES);
			if(mineField != 0)
			{
				SINGLE maxNum = mineField->MaxMineNumber();
				SINGLE mineNum = mineField->GetMineNumber();
				if(maxNum == mineNum)
				{
					PB.Color3ub(0,255,0);
				}
				else
				{
					PB.Color3ub(255,255*(mineNum/maxNum),0);
				}
			}
			else
			{
				PB.Color3ub(255,255,255);
			}
			
			vec = gVect;

			SINGLE halfWidth = fieldWidth /2;
			PB.Vertex3f(vec.x-halfWidth,vec.y-halfWidth,0);
			PB.Vertex3f(vec.x+halfWidth,vec.y-halfWidth,0);
			
			PB.Vertex3f(vec.x-halfWidth,vec.y-halfWidth,0);
			PB.Vertex3f(vec.x-halfWidth,vec.y+halfWidth,0);
			
			PB.Vertex3f(vec.x-halfWidth,vec.y+halfWidth,0);
			PB.Vertex3f(vec.x+halfWidth,vec.y+halfWidth,0);
			
			PB.Vertex3f(vec.x+halfWidth,vec.y-halfWidth,0);
			PB.Vertex3f(vec.x+halfWidth,vec.y+halfWidth,0);
			
			PB.End(); //PB_LINES
		}

	}
}

//---------------------------------------------------------------------------
//
void Minelayer::Attack (IBaseObject * victim, U32 agentID, bool bUserGenerated)
{
	THEMATRIX->OperationCompleted(agentID, dwMissionID);
	//watch as I fling mines at you point blank and kill us both... hahahaha
};
//---------------------------------------------------------------------------
//
void Minelayer::AttackPosition(const struct GRIDVECTOR & position, U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void Minelayer::SpecialAttack (IBaseObject * victim, U32 agentID)
{
	CQBOMB0("Not implemented.");
};
//---------------------------------------------------------------------------
//
void Minelayer::SpecialAOEAttack (const struct GRIDVECTOR & position, U32 agentID)
{
	CQASSERT(!workingID);
	if(THEMATRIX->IsMaster())
	{
		if(supplies > pArch->pData->fieldSupplyCostPerMine)
		{
			targetMinePos = position;
			targetMinePos = targetMinePos.centerpos();

			MineLayerCommandPos buffer;
			buffer.command = MINELAYER_NET_START;
			buffer.position = targetMinePos;
			THEMATRIX->SendOperationData(agentID,dwMissionID,(void *)&buffer,sizeof(buffer));


			if(bHasHardpoint)
			{
				moveToPos(targetMinePos,0);
			}
			else
			{
				moveToPos(targetMinePos,0);
			}

			workingID = agentID;

			mode = MLAY_MOVING_TO_POS;
		}
		else
		{
			MineLayerCommand buffer;
			buffer.command =  MINELAYER_NET_START_FAIL;
			THEMATRIX->SendOperationData(agentID,dwMissionID,(void *)&buffer,sizeof(buffer));

			THEMATRIX->OperationCompleted(agentID,dwMissionID);
			mode = MLAY_IDLE;
		}
	}
	else
	{
		targetMinePos = position;
		targetMinePos = targetMinePos.centerpos();
		workingID = agentID;
		mode = MLAY_WAIT_CLIENT;
	}
};
//---------------------------------------------------------------------------
//
void Minelayer::WormAttack (IBaseObject * victim, U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void Minelayer::CancelAttack (void)
{
};
//---------------------------------------------------------------------------
//
void Minelayer::GetTarget (IBaseObject* & targObj, U32 targID)
{
};
//---------------------------------------------------------------------------
//
void Minelayer::ReportKill (U32 partID)
{
};
//---------------------------------------------------------------------------
//
void Minelayer::OnMasterChange (bool bIsMaster)
{
	repairMasterChange(bIsMaster);
	if(bIsMaster)
	{
		if(mode == MLAY_WAIT_CLIENT)
		{
			if(supplies > pArch->pData->fieldSupplyCostPerMine)
			{
				MineLayerCommandPos buffer;
				buffer.command = MINELAYER_NET_START;
				buffer.position = targetMinePos;
				THEMATRIX->SendOperationData(workingID,dwMissionID,(void *)&buffer,sizeof(buffer));


				if(bHasHardpoint)
				{
					moveToPos(targetMinePos,0);
				}
				else
				{
					moveToPos(targetMinePos,0);
				}

				mode = MLAY_MOVING_TO_POS;
			}
			else
			{
				MineLayerCommand buffer;
				buffer.command =  MINELAYER_NET_START_FAIL;
				THEMATRIX->SendOperationData(workingID,dwMissionID,(void *)&buffer,sizeof(buffer));

				THEMATRIX->OperationCompleted(workingID,dwMissionID);
				mode = MLAY_IDLE;
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void Minelayer::OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
{
	
}
//---------------------------------------------------------------------------
//
void Minelayer::receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
{
	if(workingID == agentID)
	{
		MineLayerCommand * buf = (MineLayerCommand *)buffer;
		
		switch(buf->command)
		{
		case MINELAYER_NET_START:
			{
				MineLayerCommandPos * posBuf = (MineLayerCommandPos *)buffer;
				targetMinePos = posBuf->position;
				if(bHasHardpoint)
				{
					moveToPos(targetMinePos,0);
				}
				else
				{
					moveToPos(targetMinePos,0);
				}

				workingID = agentID;

				mode = MLAY_MOVING_TO_POS;

				return;
			}
		case MINELAYER_NET_START_FAIL:
			{
				THEMATRIX->OperationCompleted2(workingID,dwMissionID);
				mode = MLAY_IDLE;
				return;
			}
		case MINEFIELD_NET_BEGIN_LAYING:
			{
				MineLayerCommandHandle * myBuf = (MineLayerCommandHandle *) buffer;
				minefieldMissionID =  myBuf->handle;
				IBaseObject * obj = MGlobals::CreateInstance(pArch->pMineFieldType,minefieldMissionID);
				if(obj)
				{
					obj->QueryInterface(IMinefieldID,targetField,NONSYSVOLATILEPTR);
					targetField->InitMineField(targetMinePos,systemID);
					OBJLIST->AddObject(obj);
					targetField->AddLayerRef();
				}
				bDropedForAnim = false;

				if (animReleaseIndex != -1)
				{
					ANIM->script_start(animReleaseIndex, Animation::FORWARD, Animation::BEGIN);
					animDuration = ANIM->get_duration(animSetArch,pArch->pData->drop_anim);
				}else
					animDuration =0;
				mode = MLAY_LAYING;
				return;
			}
		case MINEFIELD_NET_RESTART_LAYING:
			{
				MineLayerCommandHandle * myBuf = (MineLayerCommandHandle *) buffer;
				minefieldMissionID =  myBuf->handle;
				IBaseObject * obj = OBJLIST->FindObject(minefieldMissionID);
				CQASSERT(obj);
				obj->QueryInterface(IMinefieldID,targetField,NONSYSVOLATILEPTR);

				if(targetField)
					targetField->AddLayerRef();

				bDropedForAnim = false;
				if (animReleaseIndex != -1)
				{
					ANIM->script_start(animReleaseIndex, Animation::FORWARD, Animation::BEGIN);
					animDuration = ANIM->get_duration(animSetArch,pArch->pData->drop_anim);
				}else
					animDuration =0;
				mode = MLAY_LAYING;
				return;
			}
		case MINEFIELD_NET_FINISH_LAYING:
			{
				MineLayerCommandHandle * myBuf = (MineLayerCommandHandle *) buffer;
				THEMATRIX->OperationCompleted(workingID,minefieldMissionID);
				if(targetField != 0)
				{
					U32 finalHull =  myBuf->handle;
					targetField->SetMineNumber(finalHull);
					targetField->SubLayerRef();
					targetField = NULL;
					minefieldMissionID = 0;
				}
				mode = MLAY_IDLE;
				THEMATRIX->OperationCompleted2(workingID,dwMissionID);
				return;
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void Minelayer::onOperationCancel (U32 agentID)
{
	if(THEMATRIX->IsMaster() && agentID == workingID)
	{
		bRotating = false;
		if(mode == MLAY_LAYING)
		{
			MineLayerCommandHandle buffer;
			buffer.command = MINEFIELD_NET_FINISH_LAYING;
			if(targetField)
				buffer.handle = targetField->GetMineNumber();
			else
				buffer.handle = 0;
			THEMATRIX->SendOperationData(workingID,dwMissionID,(void *)&buffer,sizeof(buffer));
			layingTime = 0;
			THEMATRIX->OperationCompleted(workingID,minefieldMissionID);
			U32 finalHull =  buffer.handle;
			if(targetField)
			{
				targetField->SetMineNumber(finalHull);
				targetField->SubLayerRef();
			}
			targetField = NULL;
			minefieldMissionID = 0;
			mode = MLAY_IDLE;
			THEMATRIX->OperationCompleted2(workingID,dwMissionID);
		}
		else
		{
			MineLayerCommand buffer;
			buffer.command =  MINELAYER_NET_START_FAIL;
			THEMATRIX->SendOperationData(workingID,dwMissionID,(void *)&buffer,sizeof(buffer));

			THEMATRIX->OperationCompleted2(workingID,dwMissionID);
			mode = MLAY_IDLE;
		}
	}
	CQASSERT(!workingID);
}		

void Minelayer::preSelfDestruct (void)
{
	if(THEMATRIX->IsMaster() && workingID)
	{
		bRotating = false;
		if(mode == MLAY_LAYING)
		{
			MineLayerCommandHandle buffer;
			buffer.command = MINEFIELD_NET_FINISH_LAYING;
			if(targetField)
				buffer.handle = targetField->GetMineNumber();
			else
				buffer.handle = 0;
			THEMATRIX->SendOperationData(workingID,dwMissionID,(void *)&buffer,sizeof(buffer));
			layingTime = 0;
			THEMATRIX->OperationCompleted(workingID,minefieldMissionID);
			U32 finalHull =  buffer.handle;
			if(targetField)
			{
				targetField->SetMineNumber(finalHull);
				targetField->SubLayerRef();
				targetField = NULL;
			}
			minefieldMissionID = 0;
			mode = MLAY_IDLE;
			THEMATRIX->OperationCompleted2(workingID,dwMissionID);
		}
		else
		{
			MineLayerCommand buffer;
			buffer.command =  MINELAYER_NET_START_FAIL;
			THEMATRIX->SendOperationData(workingID,dwMissionID,(void *)&buffer,sizeof(buffer));

			THEMATRIX->OperationCompleted2(workingID,dwMissionID);
			mode = MLAY_IDLE;
		}
	}
	CQASSERT(!workingID);
}
//---------------------------------------------------------------------------
//
void Minelayer::preTakeover (U32 newMissionID, U32 troopID)
{
	if(THEMATRIX->IsMaster() && workingID)
	{
		bRotating = false;
		if(mode == MLAY_LAYING)
		{
			MineLayerCommandHandle buffer;
			buffer.command = MINEFIELD_NET_FINISH_LAYING;
			if(targetField)
				buffer.handle = targetField->GetMineNumber();
			else
				buffer.handle = 0;
			THEMATRIX->SendOperationData(workingID,dwMissionID,(void *)&buffer,sizeof(buffer));
			layingTime = 0;
			THEMATRIX->OperationCompleted(workingID,minefieldMissionID);
			U32 finalHull =  buffer.handle;
			if(targetField)
			{
				targetField->SetMineNumber(finalHull);
				targetField->SubLayerRef();
				targetField = NULL;
			}
			minefieldMissionID = 0;
			mode = MLAY_IDLE;
			THEMATRIX->OperationCompleted2(workingID,dwMissionID);
		}
		else
		{
			MineLayerCommand buffer;
			buffer.command =  MINELAYER_NET_START_FAIL;
			THEMATRIX->SendOperationData(workingID,dwMissionID,(void *)&buffer,sizeof(buffer));

			THEMATRIX->OperationCompleted2(workingID,dwMissionID);
			mode = MLAY_IDLE;
		}
	}
	CQASSERT(!workingID);}
//---------------------------------------------------------------------------
//
void Minelayer::physicalUpdate (SINGLE dt)
{
	if(mode == MLAY_LAYING && (targetField != 0))//if I am in mine laying mode and have recived the minefield to lay
	{
		layingTime += dt;
		SINGLE timeToLayMine = ((SINGLE)pArch->pData->fieldCompletionTime)/(targetField->MaxMineNumber());
		animDuration -= dt;
		if(layingTime > timeToLayMine)
		{
			if((!bDropedForAnim) && (animDuration <= -pArch->pData->dropAnimDelay))
			{
				bDropedForAnim = true;
				Vector pos;
				Vector vel;
				if(bHasHardpoint)
				{
					TRANSFORM hpTrans;

					hpTrans.TRANSFORM::TRANSFORM(hardpointinfo.orientation, hardpointinfo.point);
					hpTrans = ENGINE->get_transform(mineReleaseIndex).multiply(hpTrans);
					pos = hpTrans.translation;
					vel = -hpTrans.get_k()* pArch->pData->mineReleaseVelocity;
				}
				else
				{
					pos = GetPosition();
					vel = transform.get_k()*200;
				}
				
				while(layingTime > timeToLayMine)
				{
					layingTime -= timeToLayMine;
					if((supplies < pArch->pData->fieldSupplyCostPerMine)||(targetField->GetMineNumber() == targetField->MaxMineNumber()))
					{
						if(THEMATRIX->IsMaster())
						{
							MineLayerCommandHandle buffer;
							buffer.command = MINEFIELD_NET_FINISH_LAYING;
							buffer.handle = targetField->GetMineNumber();
							THEMATRIX->SendOperationData(workingID,dwMissionID,(void *)&buffer,sizeof(buffer));
							layingTime = 0;
							THEMATRIX->OperationCompleted(workingID,minefieldMissionID);
							if(targetField != 0)
							{
								U32 finalHull =  buffer.handle;
								targetField->SetMineNumber(finalHull);
								targetField->SubLayerRef();
								targetField = NULL;
								minefieldMissionID = 0;
							}
							mode = MLAY_IDLE;
							THEMATRIX->OperationCompleted2(workingID,dwMissionID);
						}
					}
					else 
					{
						targetField->AddMine(pos,vel);
						if(THEMATRIX->IsMaster())
						{
							supplies -= pArch->pData->fieldSupplyCostPerMine;
						}
					}
				}
			}
		}
		if(animDuration <= -pArch->pData->dropAnimDelay)
			bDropedForAnim = true; //missed it's chance to fire will save for next shot!
		if(animDuration <= 0)
		{
			bDropedForAnim = false;
			if (animReleaseIndex != -1)
			{
				ANIM->script_start(animReleaseIndex, Animation::FORWARD, Animation::BEGIN);
				animDuration = ANIM->get_duration(animSetArch,pArch->pData->drop_anim);
			}else
				animDuration =0;
		}
	}
}
//---------------------------------------------------------------------------
//
BOOL32 Minelayer::update (void)
{
	if(workingID)
	{
		CQASSERT(mode != MLAY_IDLE);
		if(mode == MLAY_MOVING_TO_POS)
		{
			if(!(isMoveActive()))//if I have stoped moving
			{
				Vector targDir = transform.translation-targetMinePos;
				TRANSFORM hpTrans;
				hpTrans.TRANSFORM::TRANSFORM(hardpointinfo.orientation, hardpointinfo.point);
				hpTrans = ENGINE->get_transform(mineReleaseIndex).multiply(hpTrans);
				SINGLE relYaw = get_angle(targDir.x,targDir.y) - TRANSFORM::get_yaw(hpTrans.get_k());
				if (relYaw < -PI)
					relYaw += PI*2;
				else if (relYaw > PI)
					relYaw -= PI*2;
				bool result = rotateShip(relYaw,0,0);
				setAltitude(0);
				if (result==0)
					disableAutoMovement();
				bRotating = (result == 0);
				mode = MLAY_ROTATING_TO_POS;
			}
		}
		else if(mode == MLAY_ROTATING_TO_POS)
		{
			if(!bRotating && THEMATRIX->IsMaster())
			{
				BT_MINEFIELD_DATA * mineData = (BT_MINEFIELD_DATA *)(ARCHLIST->GetArchetypeData(pArch->pMineFieldType));
				M_RESOURCE_TYPE failtype;
				if(BANKER->SpendMoney(playerID,mineData->cost,&failtype))
				{
					//check if a minefield already exists
					IField *field =  FIELDMGR->GetFields();
					while(field)
					{
						if(field->fieldType == FC_MINEFIELD && field->pArchetype == pArch->pMineFieldType && field->GetPlayerID() == playerID)
						{
							field->QueryInterface(IMinefieldID,targetField,NONSYSVOLATILEPTR);
							if(targetField != 0)
							{
								if(targetField->AtPosition(targetMinePos) &&
									(targetField->GetLayerRef() || targetField->GetMineNumber()))
								{
									break;
								}
							}
							targetField = NULL;
						}
						field = field->nextField;
					}
					if(targetField != 0)//add mines to the existing field
					{
						if(targetField->GetLayerRef())//only one mielayer per mine field
						{
							MineLayerCommand buffer;
							buffer.command =  MINELAYER_NET_START_FAIL;
							THEMATRIX->SendOperationData(workingID,dwMissionID,(void *)&buffer,sizeof(buffer));

							mode = MLAY_IDLE;						
							THEMATRIX->OperationCompleted2(workingID,dwMissionID);
						}
						else
						{
							targetField->AddLayerRef();
							MineLayerCommandHandle buffer;
							buffer.command = MINEFIELD_NET_RESTART_LAYING;
							OBJPTR<IBaseObject> obj;
							targetField->QueryInterface(IBaseObjectID,obj);
							minefieldMissionID = buffer.handle= obj->GetPartID();
							THEMATRIX->SendOperationData(workingID,dwMissionID,(void *)&buffer,sizeof(buffer));

							bDropedForAnim = false;
							if (animReleaseIndex != -1)
							{
								ANIM->script_start(animReleaseIndex, Animation::FORWARD, Animation::BEGIN);
								animDuration = ANIM->get_duration(animSetArch,pArch->pData->drop_anim);
							}else
								animDuration =0;
							mode = MLAY_LAYING;
						}
					}else //start laying the mines in new field
					{
						MineLayerCommandHandle buffer;
						buffer.command = MINEFIELD_NET_BEGIN_LAYING;
						buffer.handle = MGlobals::CreateNewPartID(playerID);
						minefieldMissionID = buffer.handle;
						THEMATRIX->AddObjectToOperation(workingID,minefieldMissionID);
						IBaseObject * obj = MGlobals::CreateInstance(pArch->pMineFieldType,minefieldMissionID);
						CQASSERT(obj);
						if(obj)
						{
							obj->QueryInterface(IMinefieldID,targetField,NONSYSVOLATILEPTR);
							CQASSERT(targetField);
							if(targetField != 0)
							{
								targetField->InitMineField(targetMinePos,systemID);
								OBJLIST->AddObject(obj);
								OBJPTR<IBaseObject> obj;
								targetField->QueryInterface(IBaseObjectID,obj);
								THEMATRIX->SendOperationData(workingID,dwMissionID,(void *)&buffer,sizeof(buffer));
								targetField->AddLayerRef();

								bDropedForAnim = false;

								if (animReleaseIndex != -1)
								{
									ANIM->script_start(animReleaseIndex, Animation::FORWARD, Animation::BEGIN);
									animDuration = ANIM->get_duration(animSetArch,pArch->pData->drop_anim);
								}else
									animDuration =0;
								mode = MLAY_LAYING;
							}
						}
					}
				}
				else
				{
					MineLayerCommand buffer;
					buffer.command =  MINELAYER_NET_START_FAIL;
					THEMATRIX->SendOperationData(workingID,dwMissionID,(void *)&buffer,sizeof(buffer));

					THEMATRIX->OperationCompleted2(workingID,dwMissionID);
					mode = MLAY_IDLE;
				}
			}
			else if(bRotating)
			{
				Vector targDir = transform.translation-targetMinePos;
				TRANSFORM hpTrans;
				hpTrans.TRANSFORM::TRANSFORM(hardpointinfo.orientation, hardpointinfo.point);
				hpTrans = ENGINE->get_transform(mineReleaseIndex).multiply(hpTrans);
				SINGLE relYaw = get_angle(targDir.x,targDir.y) - TRANSFORM::get_yaw(hpTrans.get_k());
				if (relYaw < -PI)
					relYaw += PI*2;
				else if (relYaw > PI)
					relYaw -= PI*2;
				bool result = rotateShip(relYaw,0,0);
				setAltitude(0);
				if (result==0)
					disableAutoMovement();
				bRotating = (result == 0);
			}
		}
	}
	return true;
}
//---------------------------------------------------------------------------
//
void Minelayer::save (MINELAYER_SAVELOAD & save)
{
	save.mineLayerSaveLoad = *static_cast<BASE_MINELAYER_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void Minelayer::load (MINELAYER_SAVELOAD & load)
{
	*static_cast<BASE_MINELAYER_SAVELOAD *>(this) = load.mineLayerSaveLoad;
}
//---------------------------------------------------------------------------
//
void Minelayer::resolve (void)
{
	if(minefieldMissionID)
	{
		IBaseObject * obj = OBJLIST->FindObject(minefieldMissionID);
		if(obj)
			obj->QueryInterface(IMinefieldID,targetField,NONSYSVOLATILEPTR);
	}
}
//---------------------------------------------------------------------------
//
bool Minelayer::initMinelayer (const MINELAYER_INIT & data)
{
	pArch = (MINELAYER_INIT *)((void *)(&data));
	workingID = 0;
	targetField = NULL;
	layingTime = 0;
	minefieldMissionID = 0;
	animDuration = 0;
	bDropedForAnim = true;
	mode = MLAY_IDLE;

	bHasHardpoint = (FindHardpoint(data.pData->mineReleaseHardpoint,mineReleaseIndex,hardpointinfo,instanceIndex) != 0);

	animSetArch = data.animArchetype;
	if (data.pData->drop_anim[0])
		animReleaseIndex = ANIM->create_script_inst(animSetArch, instanceIndex, data.pData->drop_anim);
	else
		animReleaseIndex = -1;
	
	return TRUE;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
struct IBaseObject * createMinelayer (const MINELAYER_INIT & data)
{
	Minelayer * obj = new ObjectImpl<Minelayer>;

	obj->FRAME_init(data);
	obj->initMinelayer(data);
	return obj;
}
//------------------------------------------------------------------------------------------
//---------------------------HavestShip Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE MinelayerFactory : public IObjectFactory
{
	struct OBJTYPE : MINELAYER_INIT
	{
		~OBJTYPE (void)
		{
			if (pMineFieldType)
				ARCHLIST->Release(pMineFieldType, OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(MinelayerFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	MinelayerFactory (void) { }

	~MinelayerFactory (void);

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

	/* MinelayerFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
MinelayerFactory::~MinelayerFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void MinelayerFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE MinelayerFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_SPACESHIP)
	{
		BT_MINELAYER_DATA * data = (BT_MINELAYER_DATA *) _data;

		if (data->type == SSC_MINELAYER)	   
		{
			result = new OBJTYPE;
			if (result->loadSpaceshipArchetype(data, ARCHLIST->GetArchetype(szArchname)) == false)
				goto Error;
			if (data->mineFieldType[0])
			{
				result->pMineFieldType = ARCHLIST->LoadArchetype(data->mineFieldType);
				if (result->pMineFieldType)
					ARCHLIST->AddRef(result->pMineFieldType, OBJREFNAME);
			}
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
BOOL32 MinelayerFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * MinelayerFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createMinelayer(*objtype);
}
//-------------------------------------------------------------------
//
void MinelayerFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _Minelayer : GlobalComponent
{
	MinelayerFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<MinelayerFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _Minelayer __ship;

//---------------------------------------------------------------------------
//--------------------------End Minelayer.cpp--------------------------------
//---------------------------------------------------------------------------