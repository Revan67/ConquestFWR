//--------------------------------------------------------------------------//
//                                                                          //
//                             HarvestShip.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/HarvestShip.cpp 323   8/23/01 1:53p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>
//#define _DEBUG_HARVEST  //uncomment this to see the orbit code in action

#include "TObjTrans.h"
#include "TObjControl.h"
#include "TObjFrame.h"
#include "TSpaceShip.h"
#include <DSpaceShip.h>
#include "TObject.h"
#include "Objlist.h"
#include "Startup.h"
#include "Mission.h"
#include "IPlanet.h"
#include "IHardpoint.h" 
#include "IHarvest.h"
#include "MPart.h"
#include "Sector.h"
#include "CommPacket.h"
#include "INugget.h"
#include "UnitComm.h"
#include "Objset.h"
#include "FogOfWar.h"
#include "ObjMapIterator.h"
#include "IRecoverShip.h"
#include "IScriptObject.h"
#include "TObjRender.h"

#include <TComponent.h>
#include <IConnection.h>
#include <TSmartPointer.h>

#define HOST_CHECK (THEMATRIX->IsMaster())

#define OPPRINT0(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp)
#define OPPRINT1(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1)
#define OPPRINT2(exp,p1,p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2)

#define COMP_OP(operation) { U32 tempID = workingID; workingID = 0; THEMATRIX->OperationCompleted(tempID,operation);}

//#define NET_METRIC(opSize) {OBJLIST->DEBUG_AddNetworkBytes(IObjectList::HARVEST,opSize);}
#define NET_METRIC(opSize) ((void)0)


#define TOTAL_RECOVER_TIME 2.0
#define TIMEOUT_TIME 2.0

//HAR_C_TARGET_LOST
//HAR_C_CANCEL_OPERATION
//HAR_C_AT_REFINERY
//HAR_C_DOCK_REF_BEGIN
//HAR_C_DOCK_END
//HAR_C_PLANET_MOVETO
//HAR_C_AT_PLANET
//HAR_C_NUGGET_MOVETO
//HAR_C_AT_NUGGET
//HAR_C_PRE_TAKEOVER
#pragma pack(push,1)
struct BaseHarCommand
{
	U8 command;
};

//HAR_C_PLANET_PARKING_SLOT
//HAR_C_NUGGET_BEGIN
//HAR_C_NUGGET_ASSIST_BEGIN
struct HarCommandWHandle
{
	U8 command;
	U32 handle;
};

//HAR_C_FINISH_PLANET_HARVEST
struct HarCommandWDecision
{
	U8 command;
	U8 decision:2;
};

//HAR_C_NUGGET_END
struct HarCommandW2Decision
{
	U8 command;
	U8 decision:2;
	U8 decision2:2;
};

#pragma pack(pop)

struct _NO_VTABLE HarvestShip : public SpaceShip<HARVEST_SAVELOAD, HARVEST_INIT>, IHarvest,IRecoverShip,BASE_HARVEST_SAVELOAD
{
	BEGIN_MAP_INBOUND(HarvestShip)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IGotoPos)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IHarvest)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IRepairee)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IRecoverShip)
	_INTERFACE_ENTRY(IShipMove)
	_INTERFACE_ENTRY(ICloak)
	_INTERFACE_ENTRY(IEffectTarget)
	END_MAP()

	enum HARVEST_NET_COMMANDS
	{
		HAR_C_TARGET_LOST = 0,
		HAR_C_CANCEL_OPERATION,

		HAR_C_AT_REFINERY,
		HAR_C_DOCK_REF_BEGIN,
		HAR_C_DOCK_END,

		HAR_C_NUGGET_MOVETO,
		HAR_C_AT_NUGGET,
		HAR_C_NUGGET_BEGIN,
		HAR_C_NUGGET_END,
		HAR_C_NUGGET_ASSIST_BEGIN,
		HAR_C_END_NUGGET_ON_CANCEL,

		HAR_C_PRE_TAKEOVER,
		HAR_C_PRE_TAKEOVER_NUG
	};

	typedef INITINFO HARVESTINITINFO;

	PhysUpdateNode physUpdateNode;
	UpdateNode  updateNode;
	SaveNode	saveNode;
	LoadNode	loadNode;
	InitNode	initNode;
	RenderNode  renderNode;
	ResolveNode		resolveNode;
	OnOpCancelNode	onOpCancelNode;
	PreTakeoverNode preTakeoverNode;
	ReceiveOpDataNode	receiveOpDataNode;
	PreDestructNode	destructNode;
	GeneralSyncNode  genSyncNode;
	GeneralSyncNode  idleSyncNode;

	HARVEST_INIT * hArch;
	SINGLE timeout;

	OBJPTR<IBaseObject> pTargetObject;
	OBJPTR<IBaseObject> recoveryTarget;
	
	HSOUND dockingSound;
	HSOUND nuggetingSound;
	SFX::ID dockingSoundID, nuggetingSoundID;
	U8 netGas,netMetal;

	SINGLE updateTimer;

	//----------------------------------
	// hardpoint data
	//----------------------------------
	HardpointInfo  beamPointInfo;

	HardpointInfo tankPoint[4];

	HarvestShip (void);

	virtual ~HarvestShip (void);

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual void DrawHighlighted();

	virtual void Render ();

	/* IGotoPos methods */

	virtual void GotoPosition (const GRIDVECTOR & pos, U32 agentID, bool bSlowMove);

	virtual void PrepareForJump (IBaseObject * jumpgate, bool bUserMove, U32 agentID, bool bSlowMove);


	// IHarvest method

	void BeginHarvest(IBaseObject * victim, U32 agentID, bool autoSelected);

	void DockTaken (void);

	virtual U32 GetGas();

	virtual U32 GetMetal();

	virtual bool IsIdle();

	virtual void SetAutoHarvest(enum M_NUGGET_TYPE nType);

	// IRecoveryShip

	virtual void RecoverWreck(IBaseObject * object, U32 agentID);

	virtual void ReturnReck(IBaseObject * object, U32 agentID);

	/* SpaceShip methods */
	
	virtual const char * getSaveStructName (void) const		// must be overriden implemented by derived class
	{
		return "HARVEST_SAVELOAD";
	}
	virtual void * getViewStruct (void) 	// must be overriden implemented by derived class
	{
		return static_cast<BASE_HARVEST_SAVELOAD *>(this);
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

	virtual void OnStopRequest (U32 agentID);

	virtual void OnMasterChange (bool bIsMaster);

	virtual void OnAddToOperation (U32 agentID);
	/* HarvestShip methods */

	void harvest (IBaseObject *victim);

	void harvestNugget ();

	BOOL32 updateHarvest ();

	void physUpdateHarvest (SINGLE dt);

	IBaseObject * findNearRefinery (void);

	IBaseObject * findHarvestTarget ();

	void initHarvestShip (const HARVESTINITINFO & data);//PARCHETYPE pArchetype, S32 archIndex, S32 animIndex);

	void save (HARVEST_SAVELOAD & save);
	void load (HARVEST_SAVELOAD & load);

	void renderNuggeting (void);

	void renderHarvestShip (void);

//	S32 findChild (const char * pathname, INSTANCE_INDEX parent);

//	BOOL32 findHardpoint (const char * pathname, INSTANCE_INDEX & index, HardpointInfo & hardpointinfo, INSTANCE_INDEX parent);

	void resolve (void);

	void preTakeover (U32 newMissionID, U32 troopID);

	SINGLE TestHighlight (const RECT & rect);

	void cancelHarvest ();

	void localMovToPos (const Vector & _vec)
	{
		GRIDVECTOR vec;
		vec = _vec;
		CQASSERT(workingID && "Harvest ship moving without an agent!");
		moveToPos(vec);
	}

	U32 getSyncData (void * buffer);
	void putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery);
	U32 sendIdleMessage(void * buffer);
	void putIdleMessage(void * buffer, U32 bufferSize, bool bLateDelivery);

	bool localRotateShip (SINGLE relYaw)
	{
		return rotateShip(relYaw, 0 - transform.get_roll(), 0 - transform.get_pitch());
	}

	bool moveToDockPos();

	void doUnitAI();

#ifdef _DEBUG_HARVEST
#ifdef _DEBUG
	void renderMath();
#endif
#endif
};
//---------------------------------------------------------------------------
//
HarvestShip::HarvestShip (void) : 
		physUpdateNode(this, PhysUpdateProc(&HarvestShip::physUpdateHarvest)),
		updateNode(this, UpdateProc(&HarvestShip::updateHarvest)),
		saveNode(this, CASTSAVELOADPROC(&HarvestShip::save)),
		loadNode(this, CASTSAVELOADPROC(&HarvestShip::load)),
		initNode(this, CASTINITPROC(&HarvestShip::initHarvestShip)),
		renderNode(this, RenderProc(&HarvestShip::renderHarvestShip)),
		resolveNode(this, ResolveProc(&HarvestShip::resolve)),
		onOpCancelNode(this, OnOpCancelProc(&HarvestShip::onOperationCancel)),
		preTakeoverNode(this, PreTakeoverProc(&HarvestShip::preTakeover)),
		receiveOpDataNode(this, ReceiveOpDataProc(&HarvestShip::receiveOperationData)),
		destructNode(this, PreDestructProc(&HarvestShip::preSelfDestruct)),
		genSyncNode(this, SyncGetProc(&HarvestShip::getSyncData), SyncPutProc(&HarvestShip::putSyncData)),
		idleSyncNode(this, SyncGetProc(&HarvestShip::sendIdleMessage), SyncPutProc(&HarvestShip::putIdleMessage))
{
	bNuggeting = false;
}
//---------------------------------------------------------------------------
//
IBaseObject * HarvestShip::findNearRefinery (void)
{
	IBaseObject * obj = OBJLIST->GetTargetList();
	IBaseObject * closest = NULL;
	U32 closeSector = 0;
	SINGLE closeDist2 = 0;
	bool closeInSystem = false;
	bool haveCloseDist = false;
	U32 closeList[16];
	while(obj)
	{
		if((obj->objClass == OC_PLATFORM) && (GetPlayerID() == obj->GetPlayerID()))
		{
			MPart part(obj);
			if(((part->mObjClass == M_REFINERY) || (part->mObjClass == M_COLLECTOR) ||
				(part->mObjClass == M_HEAVYREFINERY) || (part->mObjClass == M_SUPERHEAVYREFINERY)||
				(part->mObjClass == M_OXIDATOR)|| (part->mObjClass == M_GREATER_COLLECTOR) ||
				(part->mObjClass == M_COALESCER)) && (part->bReady) && (obj->GetSystemID() <= MAX_SYSTEMS) &&
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
							if(numJumps != -1)
							{
								if(numJumps < closeSector) 
								{
									closeDist2 = 0;
									haveCloseDist = false;
									closest = obj;
									closeSector = numJumps;
								}
								else if(numJumps == closeSector)
								{
									OBJPTR<IPlatform> platform;
									IBaseObject * jump1;
									IBaseObject * jump2;
									Vector pos1,pos2;
									U32 jumpPos;
									if(!haveCloseDist)//do I have the distance to my current closest
									{
										//if not compute it
										haveCloseDist = true;
										closest->QueryInterface(IPlatformID,platform);
										CQASSERT(platform);
										//find the amount of work the platfrom has left to do and convert it to distance
										SINGLE workDist = (platform->GetNumDocking()*((supplyPointsMax/hArch->pData->dockTiming.unloadRate)+2) * maxVelocity);
										jumpPos = 0;
										jump1 = SECTOR->GetJumpgateTo(closeList[jumpPos],closeList[jumpPos+1],transform.translation);
										closeDist2 = (transform.translation-jump1->GetPosition()).magnitude_squared() + (workDist*workDist);
										++jumpPos;
										while(jumpPos < closeSector-1)
										{
											jump1 = SECTOR->GetJumpgateDestination(jump1);
											jump2 = SECTOR->GetJumpgateTo(closeList[jumpPos],closeList[jumpPos+1],jump1->GetPosition());
											closeDist2 += (jump1->GetPosition() - jump2->GetPosition()).magnitude_squared();
											jump1 = jump2;
											++jumpPos;
										}
										jump1 = SECTOR->GetJumpgateDestination(jump1);
										closeDist2 += (jump1->GetPosition() - obj->GetPosition()).magnitude_squared();
										platform =0;
									}
									obj->QueryInterface(IPlatformID,platform);
									CQASSERT(platform);
									SINGLE workDist = (platform->GetNumDocking()*((supplyPointsMax/hArch->pData->dockTiming.unloadRate)+2) * maxVelocity);
									jumpPos = 0;
									jump1 = SECTOR->GetJumpgateTo(list[jumpPos],list[jumpPos+1],transform.translation);
									U32 newDist2 = (transform.translation-jump1->GetPosition()).magnitude_squared() + (workDist*workDist);
									++jumpPos;
									while(jumpPos < numJumps-1)
									{
										jump1 = SECTOR->GetJumpgateDestination(jump1);
										jump2 = SECTOR->GetJumpgateTo(list[jumpPos],list[jumpPos+1],jump1->GetPosition());
										newDist2 += (jump1->GetPosition() - jump2->GetPosition()).magnitude_squared();
										jump1 = jump2;
										++jumpPos;
									}
									jump1 = SECTOR->GetJumpgateDestination(jump1);
									newDist2 += (jump1->GetPosition() - obj->GetPosition()).magnitude_squared();
									if(newDist2 < closeDist2)
									{
										closeDist2 = newDist2;
										haveCloseDist = true;
										closest = obj;
										closeSector = numJumps;
										memcpy(closeList,list,sizeof(list));
									}
								}
							}
						}
					}
					else
					{
						OBJPTR<IPlatform> platform;
						obj->QueryInterface(IPlatformID,platform);
						CQASSERT(platform);
						if(!closeInSystem)
						{
							closeInSystem = true;
							Vector dist = (transform.translation-obj->GetPosition());
							SINGLE workDist = (platform->GetNumDocking()*((supplyPointsMax/hArch->pData->dockTiming.unloadRate)+2) * maxVelocity);
							closeDist2 = (dist.x*dist.x)+(dist.y*dist.y)+(dist.z*dist.z)+
								workDist*workDist;
							closest = obj;
						}
						else
						{
							Vector dist = (transform.translation-obj->GetPosition());
							SINGLE workDist = (platform->GetNumDocking()*((supplyPointsMax/hArch->pData->dockTiming.unloadRate)+2) * maxVelocity);
							SINGLE testDist = (dist.x*dist.x)+(dist.y*dist.y)+(dist.z*dist.z)+
								workDist*workDist;
							if(testDist < closeDist2)
							{
								closeDist2 = testDist;
								closest = obj;
							}
						}
					}

				} else
				{
					OBJPTR<IPlatform> platform;
					obj->QueryInterface(IPlatformID,platform);
					CQASSERT(platform);
					U32 list[16];
					closeSector = 0; 
					closeDist2 = 0;
					if(GetSystemID() != obj->GetSystemID())
					{
						U32 testSect = SECTOR->GetShortestPath(GetSystemID(), obj->GetSystemID(), list, GetPlayerID());
						if(testSect != -1)
						{
							closest = obj;
							closeSector = testSect;
							memcpy(closeList,list,sizeof(list));
							haveCloseDist = false;
						}
					}
					else
					{
						closest = obj;
						closeInSystem = true;
						Vector dist = (transform.translation-obj->GetPosition());
						SINGLE workDist = (platform->GetNumDocking()*((supplyPointsMax/hArch->pData->dockTiming.unloadRate)+2) * maxVelocity);
						closeDist2 = (dist.x*dist.x)+(dist.y*dist.y)+(dist.z*dist.z)+
						workDist*workDist;
					}
				}
			}
		}
		obj = obj->nextTarget;
	}
	return closest;
}
//---------------------------------------------------------------------------
//
IBaseObject * HarvestShip::findHarvestTarget ()
{
	IBaseObject * closest = NULL;
	if(harvestVector.systemID != 0)
	{
		GRIDVECTOR vect = harvestVector;
		ObjMapIterator iter(harvestVector.systemID,vect,/*sensorRadius*/64*GRIDSIZE);
		SINGLE dist = 0;
		while(iter)
		{
			if(iter->obj->objClass == OC_NUGGET)
			{
				if(iter->obj->IsVisibleToPlayer(playerID))
				{
					OBJPTR<INugget> nugget;
					iter->obj->QueryInterface(INuggetID,nugget);
					if(nugget->GetSupplies()> 0 && nugget->GetNuggetType() == nuggetType)
					{
						if(closest)
						{
							SINGLE newDist = vect-iter->obj->GetGridPosition();
							if(newDist < dist)
							{
								dist = newDist;
								closest = iter->obj;
							}
						}
						else
						{
							closest = iter->obj;
							dist = vect-iter->obj->GetGridPosition();
						}
					}
				}
			}
			++iter;
		}
	}
	return closest;
}
//---------------------------------------------------------------------------
//
HarvestShip::~HarvestShip (void)
{
}
//--------------------------------------------------------------------------//
// set bVisible if possible for any part of object to appear
//
void HarvestShip::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)	
{
	bVisible = (GetSystemID() == currentSystem &&
			   (IsVisibleToPlayer(currentPlayer) ||
			     defaults.bVisibilityRulesOff ||
			     defaults.bEditorMode) );

}

void HarvestShip::Render ()
{
	SpaceShip<HARVEST_SAVELOAD, HARVEST_INIT>::Render();
	if(bNuggeting)
		renderNuggeting();
}

void HarvestShip::DrawHighlighted()
{
	if (bVisible==0)
		return;
	const USER_DEFAULTS * const pDefaults = DEFAULTS->GetDefaults();

	SINGLE hull;
	U32 hullMax;

	Vector point;
	S32 x, y;

	hull = ((SINGLE)GetDisplayHullPoints())/hullPointsMax;
	hullMax = hullPointsMax;
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
	SINGLE drawGas = ((SINGLE)gas)/supplyPointsMax;
	SINGLE drawMetal = ((SINGLE)metal)/supplyPointsMax;
	if ((pDefaults->bCheatsEnabled && DBHOTKEY->GetHotkeyState(IDH_HIGHLIGHT_ALL)) || pDefaults->bEditorMode || playerID == 0 || MGlobals::AreAllies(playerID, MGlobals::GetThisPlayer()))
	{
		DA::RectangleHash(pane, x-(TBARLENGTH/2), y+5, x+(TBARLENGTH/2), y+5+2, RGB(128,128,128));
		int xpos = x-(TBARLENGTH/2);
		int max = S32(TBARLENGTH*drawGas);
		if (drawGas > 0.0f)
		{
//					DA::RectangleFill(pane, x-(TBARLENGTH/2), y+5, x-(TBARLENGTH/2)+S32(TBARLENGTH*supplies), y+5+2, RGB(0,128,255));
			COLORREF color = RGB(128,0,255);

			int xrc = xpos;

			// make sure at least one bar gets displayed for one supply point
			if (max == 0 && drawGas > 0.0f)
			{
				max = 1;
			}
			
			for (int i = 0; i < max; i+=5)
			{
				xrc = xpos + i;
				DA::RectangleFill(pane, xrc, y+5, xrc+3, y+7, color);
			}
			xpos = xrc;
		}
		max = S32(TBARLENGTH*drawMetal);
		if (drawMetal > 0.0f)
		{
//					DA::RectangleFill(pane, x-(TBARLENGTH/2), y+5, x-(TBARLENGTH/2)+S32(TBARLENGTH*supplies), y+5+2, RGB(0,128,255));
			COLORREF color = RGB(255,255,255);

			int xrc;

			// make sure at least one bar gets displayed for one supply point
			if (max == 0 && drawMetal > 0.0f)
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

				if (bMimic)
				{
					VOLPTR(ILaunchOwner) launchOwner=static_cast<IBaseObject *>(this);
					CQASSERT(launchOwner);
					OBJPTR<ILauncher> launcher;
					launchOwner->GetLauncher(2,launcher);
					if (launcher)
					{
						VOLPTR(IMimic) mimic=launcher.ptr;
						// if we are enemies
						U32 allyMask=MGlobals::GetAllyMask(MGlobals::GetThisPlayer());
						if (mimic->IsDiscoveredTo(allyMask)==0)
						{
							wcsncpy(temp, _localLoadStringW(IDS_MIMICKED_SHIP), sizeof(temp)/sizeof(wchar_t));
						}
					}
				}

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

SINGLE HarvestShip::TestHighlight (const RECT & rect)
{
	SINGLE closeness = SpaceShip<HARVEST_SAVELOAD, HARVEST_INIT>::TestHighlight(rect);

	if (!bVisible)
	{
		if (pTargetObject)
		{
			if (pTargetObject->bVisible)
			{
				bVisible = TRUE;
			}
		}
	}

	return closeness;
}
//---------------------------------------------------------------------------
//
void HarvestShip::renderNuggeting (void)
{
	if (pTargetObject || (recoveryTarget && (mode == HAR_RECOVERING || bTowingShip)))
	{
		BOOL32 bNugVis = bVisible;
		if(!bNugVis)
		{
			if(pTargetObject)
				bNugVis = pTargetObject->bVisible && (GetVisibilityFlags()&MGlobals::GetAllyMask(MGlobals::GetThisPlayer()));
			else if(recoveryTarget)
				bNugVis = recoveryTarget->bVisible;
		}
		if(bNugVis)
		{
			BATCH->set_state(RPR_BATCH,TRUE);
			SetupDiffuseBlend(hArch->mineTex,TRUE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_ZENABLE,true);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE, false);
			CAMERA->SetModelView();
			
			IBaseObject * obj;
			SINGLE deltaTime;
			S32 frameNum;
			if(recoveryTarget && (mode == HAR_RECOVERING || bTowingShip) )
			{
				obj = recoveryTarget.Ptr();
				deltaTime = 0.5;
				frameNum = 1;
			}
			else
			{
				obj = pTargetObject;
				OBJPTR<INugget> nugget;
				obj->QueryInterface(INuggetID,nugget);
				if((nugget->GetMaxSupplies()) && (nugget->GetSupplies() <= nugget->GetMaxSupplies()))
				{
					deltaTime = 1.0-(((SINGLE)(nugget->GetSupplies()))/((SINGLE)(nugget->GetMaxSupplies())));
					CQASSERT(deltaTime >= 0);
				}
				else
					deltaTime = 0.5;
				frameNum = 1;//((U32)(nugget->GetSupplies()/hArch->pData->nuggetTiming.nuggetTime))%16;
			}

			Vector dir,dir2;
			Vector cpos = CAMERA->GetPosition();
			Vector look = obj->GetPosition()-cpos;
			look.normalize();
			
			
			dir = transform.translation-obj->GetPosition();
			dir.normalize();
			dir2 = cross_product(look,dir);
			dir2.normalize();


			Vector v0,v1,v2,v3,v4,v5;
			
			v0 = v1 = v4 = obj->GetPosition();
			v0 += (20+80*(1-deltaTime))*dir2;
			v1 -= (20+80*(1-deltaTime))*dir2;
			v2 = v3 = v5 = transform*beamPointInfo.point;
			v2 -= 20*dir2;
			v3 += 20*dir2;

			
			PB.Color3ub(255,255,255);
			PB.Begin(PB_QUADS);
			PB.TexCoord2f(0.1,0);  PB.Vertex3f(v0.x,v0.y,v0.z);
			PB.TexCoord2f(0.5,0);  PB.Vertex3f(v4.x,v4.y,v4.z);
			PB.TexCoord2f(0.4,1);  PB.Vertex3f(v5.x,v5.y,v5.z);
			PB.TexCoord2f(0,1); PB.Vertex3f(v3.x,v3.y,v3.z);

			PB.TexCoord2f(0.9,0);    PB.Vertex3f(v1.x,v1.y,v1.z);
			PB.TexCoord2f(0.5,0);  PB.Vertex3f(v4.x,v4.y,v4.z);
			PB.TexCoord2f(0.6,1);  PB.Vertex3f(v5.x,v5.y,v5.z);
			PB.TexCoord2f(1,1);    PB.Vertex3f(v2.x,v2.y,v2.z);
			PB.End();
			
			
			v0 = v1 = v2 = v3 = obj->GetPosition();
						
			dir = cross_product(look,Vector(0,0,1));
			dir.normalize();
			dir2 = cross_product(dir,look);
			dir2.normalize();
		

	//		S32 frameNum = min(15,S32(16*(deltaTime)));
			AnimFrame *frame = &hArch->harvestAnimArch->frames[frameNum];
			//blend states set above
			BATCH->set_texture_stage_texture(0,frame->texture);
			SINGLE animSize = 100+300*(1-deltaTime);
			v0 += dir*animSize-dir2*animSize;
			v1 += dir*animSize+dir2*animSize;
			v2 -= dir*animSize-dir2*animSize;
			v3 -= dir*animSize+dir2*animSize;
			
			PB.Begin(PB_QUADS);
			PB.TexCoord2f(frame->x0,frame->y0);  PB.Vertex3f(v0.x,v0.y,v0.z);
			PB.TexCoord2f(frame->x1,frame->y0);  PB.Vertex3f(v1.x,v1.y,v1.z);
			PB.TexCoord2f(frame->x1,frame->y1);  PB.Vertex3f(v2.x,v2.y,v2.z);
			PB.TexCoord2f(frame->x0,frame->y1);  PB.Vertex3f(v3.x,v3.y,v3.z);
			PB.End();
		}
	}
	else
	{
		if (bNuggeting)
		{
			SFXMANAGER->Stop(nuggetingSound);
			bNuggeting = FALSE;
		}
	}
}
//---------------------------------------------------------------------------
//
void HarvestShip::renderHarvestShip (void)
{
	if(bVisible)
	{
		#ifdef _DEBUG_HARVEST
		#ifdef _DEBUG
		if(mode == HAR_DOCKING_REFINERY && pTargetObject)
		{
			renderMath();
		}
		#endif
		#endif
//		if (bNuggeting)
//			renderNuggeting();
		if(hArch->gasTankArch != -1 && hArch->metalTankArch != -1)
		{
			S32 totalSup = gas+metal;
			if(totalSup)
			{
				S32 quarter = supplyPointsMax/4;
				S32 tempGas = gas;
				S32 tempMetal = metal;
				U32 tank = 0;
				while(totalSup > 0 && (tank < 4 || (recoverPartID && (tank < 3))))
				{
					if(tempGas > quarter)
					{
						tempGas -= quarter;
						totalSup -= quarter;
						TRANSFORM trans;
						trans.set_orientation(tankPoint[tank].orientation);
						trans.translation = tankPoint[tank].point;
						trans = transform*trans;
						BATCH->set_state(RPR_BATCH,TRUE);
						BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
						BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
						ENGINE->set_transform(hArch->gasTankMeshObj->id,trans);
						ENGINE->update_instance(hArch->gasTankMeshObj->id,0,0);
						TreeRender(hArch->gasTankMeshObj->mc);
						++tank;
					}
					else if(tempMetal > quarter)
					{
						tempMetal -= quarter;
						totalSup -= quarter;
						TRANSFORM trans;
						trans.set_orientation(tankPoint[tank].orientation);
						trans.translation = tankPoint[tank].point;
						trans = transform*trans;
						BATCH->set_state(RPR_BATCH,TRUE);
						BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
						BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
						ENGINE->set_transform(hArch->metalTankMeshObj->id,trans);
						ENGINE->update_instance(hArch->metalTankMeshObj->id,0,0);
						TreeRender(hArch->metalTankMeshObj->mc);
						++tank;
					}
					else if(tempGas > tempMetal)
					{
						tempGas -= quarter;
						totalSup -= quarter;
						TRANSFORM trans;
						trans.set_orientation(tankPoint[tank].orientation);
						trans.translation = tankPoint[tank].point;
						trans = transform*trans;
						BATCH->set_state(RPR_BATCH,TRUE);
						BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
						BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
						ENGINE->set_transform(hArch->gasTankMeshObj->id,trans);
						ENGINE->update_instance(hArch->gasTankMeshObj->id,0,0);
						TreeRender(hArch->gasTankMeshObj->mc);
						++tank;
					}else
					{
						tempMetal -= quarter;
						totalSup -= quarter;
						TRANSFORM trans;
						trans.set_orientation(tankPoint[tank].orientation);
						trans.translation = tankPoint[tank].point;
						trans = transform*trans;
						BATCH->set_state(RPR_BATCH,TRUE);
						BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
						BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
						ENGINE->set_transform(hArch->metalTankMeshObj->id,trans);
						ENGINE->update_instance(hArch->metalTankMeshObj->id,0,0);
						TreeRender(hArch->metalTankMeshObj->mc);
						++tank;
					}
				}
			}
		}
	}
}

void HarvestShip::BeginHarvest(IBaseObject * victim, U32 agentID, bool autoSelected)
{
	CQASSERT(agentID);
	CQASSERT(!workingID);
	workingID = agentID;
	bNuggetCancelOp = false;
	if(victim)
	{
		if(victim->objClass == OC_NUGGET)
		{
			harvestRemainder = 0;
			harvestVector.init(victim->GetGridPosition(),victim->GetSystemID());
			//OPPRINT2("HARVESTER - BeginHarvest of Nugget for part 0x%X using agent:#%d\n",dwMissionID,agentID);
			victim->QueryInterface(IBaseObjectID, pTargetObject, playerID);
			targetPartID = pTargetObject->GetPartID();

			OBJPTR<INugget> nugget;
			victim->QueryInterface(INuggetID,nugget);
			CQASSERT(nugget);
			nuggetType = nugget->GetNuggetType();

			if(HOST_CHECK)
			{
				if(gas+metal != supplyPointsMax)
				{
					BaseHarCommand buffer;
					buffer.command = HAR_C_NUGGET_MOVETO;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));

					mode = HAR_MOVING_TO_NUGGET;
					Vector plPos = pTargetObject->GetPosition();
					Vector dirVect = transform.translation-plPos;
					dirVect.normalize();
					plPos = plPos+(dirVect*hArch->pData->nuggetTiming.offDist*GRIDSIZE);
					localMovToPos(plPos);
				}
				else
				{
					BaseHarCommand buffer;
					buffer.command = HAR_C_CANCEL_OPERATION;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));

					mode = HAR_NO_MODE_AI_ON;
					COMP_OP(dwMissionID);
				}
			}
			else
			{
				mode = HAR_WAIT_FOR_NUGGET_BEGIN_CLIENT;
			}
		}
		else if((victim->objClass == OC_PLATFORM) && ((MPart(victim)->mObjClass == M_REFINERY) || (MPart(victim)->mObjClass == M_COLLECTOR)||
				(MPart(victim)->mObjClass == M_HEAVYREFINERY) || (MPart(victim)->mObjClass == M_SUPERHEAVYREFINERY) ||
				(MPart(victim)->mObjClass == M_GREATER_COLLECTOR)|| (MPart(victim)->mObjClass == M_OXIDATOR) ||
				(MPart(victim)->mObjClass == M_COALESCER)))
		{
			bDockingWithGas = false;
			victim->QueryInterface(IBaseObjectID, pTargetObject, playerID);
			targetPartID = victim->GetPartID();
			mode = HAR_MOVING_TO_REFINERY;
			TRANSFORM trans = pTargetObject->GetTransform();
			Vector dirVect = trans.get_position()+((-(trans.get_k()))*hArch->pData->dockTiming.offDist);
			localMovToPos(dirVect);
			OBJPTR<IPlatform> platform;
			pTargetObject->QueryInterface(IPlatformID,platform);
			platform->IncNumDocking();
		}
		else
		{
			CQBOMB1("You asked me to harvest something I can't : %s",MPart(victim)->partName);
		}
	} else
	{
		//the victim died or something?
		if(HOST_CHECK)
		{
			BaseHarCommand buffer;
			buffer.command = HAR_C_TARGET_LOST;
			NET_METRIC(sizeof(buffer));
			THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));

			mode = HAR_NO_MODE_AI_ON;
			COMP_OP(dwMissionID);
		}
		else
		{
			mode = HAR_NO_MODE_CLIENT_WAIT_CANCEL;
			targetPartID = 0;
			pTargetObject = 0;
		}
	}
}

void HarvestShip::DockTaken()
{
	if(HOST_CHECK)
	{
		if(pTargetObject)
		{
			SFXMANAGER->Stop(dockingSound);
			BaseHarCommand buffer;
			buffer.command = HAR_C_DOCK_END;
			NET_METRIC(sizeof(buffer));
			THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));

			OBJPTR<IPlatform> platform;
			pTargetObject->QueryInterface(IPlatformID,platform);
			if(bLockingPlatform)
			{
				platform->UnlockDock(this);
				bLockingPlatform = false;
//				THEMATRIX->OperationCompleted(workingID,pTargetObject->GetPartID());
			}
			pTargetObject = NULL;
			targetPartID = 0;
			mode = HAR_NO_MODE_AI_ON;
			COMP_OP(dwMissionID);
		}
		else
		{
			SFXMANAGER->Stop(dockingSound);
			
			BaseHarCommand buffer;
			buffer.command = HAR_C_TARGET_LOST;
			NET_METRIC(sizeof(buffer));
			THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));

			bLockingPlatform = false;
			mode = HAR_NO_MODE_AI_ON;
			COMP_OP(dwMissionID);
		}
	}
}

U32 HarvestShip::GetGas()
{
	return gas;
}

U32 HarvestShip::GetMetal()
{
	return metal;
}

bool HarvestShip::IsIdle()
{
	return ((mode == HAR_NO_MODE_AI_OFF) || (gas == 0 && metal == 0));
}

void HarvestShip::SetAutoHarvest(enum M_NUGGET_TYPE nType)
{
	nuggetType = nType;
	if(mode == HAR_NO_MODE_AI_OFF)
	{
		harvestVector.init(GetGridPosition(),GetSystemID());
		mode = HAR_NO_MODE_AI_ON;
	}
}

void HarvestShip::RecoverWreck(IBaseObject * object, U32 agentID)
{
	if(object && !recoverPartID)
	{
		workingID = agentID;
		mode = HAR_MOVE_TO_RECOVERY;

		object->QueryInterface(IBaseObjectID, recoveryTarget, NONSYSVOLATILEPTR);
		recoverPartID = object->GetPartID();

		recoverPos = object->GetPosition();
		recoverTime = 0;
		Vector dirVect = GetPosition()-recoverPos;
		dirVect.normalize();
		Vector plPos = recoverPos+(dirVect*hArch->pData->nuggetTiming.offDist*GRIDSIZE);
		localMovToPos(plPos);
	}
	else
	{
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
	}
}

void HarvestShip::ReturnReck(IBaseObject * object, U32 agentID)
{
	if(object && recoverPartID)
	{
		workingID = agentID;
		mode = HAR_MOVE_TO_RECOVERY_DROP;

		object->QueryInterface(IBaseObjectID, pTargetObject, playerID);

		targetPartID = object->GetPartID();

		Vector plPos = object->GetPosition();
		Vector dirVect = GetPosition()-plPos;
		dirVect.normalize();
		plPos = plPos+(dirVect*hArch->pData->nuggetTiming.offDist*GRIDSIZE);
		localMovToPos(plPos);
	}
	else
	{
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
	}
}

U32 HarvestShip::getSyncData (void * buffer)
{
	if(metal != netMetal || gas != netGas)
	{
		U8 * num = (U8 *) buffer;
		num[0] = metal;
		num[1] = gas;
		netMetal = metal;
		netGas = gas;
		return (sizeof(U8)*2);
	}
	return 0;
}

void HarvestShip::putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	CQASSERT(sizeof(U8)*2 == bufferSize);
	U8 * num = (U8 *) buffer;
	if(metal > num[0])
		MGlobals::SetMetalGained(playerID,MGlobals::GetMetalGained(playerID)+(metal-num[0]));
	if(gas > num[1])
		MGlobals::SetGasGained(playerID,MGlobals::GetGasGained(playerID)+(gas-num[1]));
	netMetal = metal = num[0];
	netGas = gas = num[1];
}

void HarvestShip::OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
{
	CQASSERT(!workingID);
	CQASSERT(agentID);
	BaseHarCommand * buf = (BaseHarCommand *)buffer;
	switch(buf->command)
	{
	case HAR_C_DOCK_REF_BEGIN:
		{
//			OPPRINT2("HARVESTER - BeginOp HAR_C_DOCK_REF_BEGIN for part 0x%X using agent:#%d\n",dwMissionID,agentID);
			CQASSERT1(mode == HAR_WAITING_TO_DOCK || mode == HAR_NO_MODE_CLIENT_WAIT_CANCEL,"mode = %d",mode);
			workingID = agentID;
			THEMATRIX->SetCancelState(workingID,false);
			if(pTargetObject)
			{
				OBJPTR<IPlatform> plat;
				pTargetObject->QueryInterface(IPlatformID,plat);
				if(!(bLockingPlatform))
				{
					plat->LockDock(this);
					bLockingPlatform = true;
				}

				mode = HAR_DOCKING_REFINERY;
			}
			else
			{
				mode = HAR_WAIT_DOCKING_CANCEL_CLIENT;
			}
			break;
		}
	case HAR_C_NUGGET_BEGIN:
		{
			//OPPRINT2("HARVESTER - BeginOp HAR_C_NUGGET_BEGIN for part 0x%X using agent:#%d\n",dwMissionID,agentID);
			workingID = agentID;
			if(pTargetObject)
			{
				HarCommandWHandle * myBuf = (HarCommandWHandle *) buffer;
				CQASSERT((pTargetObject->GetPartID() == targetPartID) &&  (targetPartID == myBuf->handle));
				Vector vect = transform.translation;
				if (nuggetingSound==0)
					nuggetingSound  = SFXMANAGER->Open(nuggetingSoundID);
				SFXMANAGER->Play(nuggetingSound,GetSystemID(),&vect,SFXPLAYF_LOOPING|SFXPLAYF_NORESTART);
				bNuggeting = true;
				mode = HAR_NUGGETING;
				OBJPTR<INugget> nugget;
				pTargetObject->QueryInterface(INuggetID,nugget);
				nugget->IncHarvestCount();
				nugget->SetProcessID(workingID);
			}
			else
			{
				HarCommandWHandle * myBuf = (HarCommandWHandle *) buffer;
				targetPartID = myBuf->handle;
				mode = HAR_WAIT_NUGGET_CANCEL_CLIENT;
			}
			break;
		}
	case HAR_C_TARGET_LOST:
		{
			//OPPRINT2("HARVESTER - BeginOp HAR_C_TARGET_LOST for part 0x%X using agent:#%d\n",dwMissionID,agentID);
			CQASSERT((mode == HAR_WAITING_TO_DOCK) ||
				(mode == HAR_NO_MODE_CLIENT_WAIT_CANCEL));
			bLockingPlatform = false;
			mode = HAR_NO_MODE_AI_ON;
			THEMATRIX->OperationCompleted(agentID,dwMissionID);
			break;			
		}
	default:
		CQASSERT(0 && "Bad data");
	}
}

void HarvestShip::receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
{
	if(agentID != workingID && agentID != posibleWorkingID)
		return;
	if(agentID == posibleWorkingID)
	{
		if(bufferSize == sizeof(HarCommandWHandle))
		{
			HarCommandWHandle * myBuf = (HarCommandWHandle *) buffer;
			if(myBuf->command == HAR_C_NUGGET_ASSIST_BEGIN)
			{
				workingID = agentID;
				posibleWorkingID = 0;
				//OPPRINT2("HARVESTER - Recieved HAR_C_NUGGET_ASSIST_BEGIN for part 0x%X using agent:#%d\n",dwMissionID,workingID);
				if(pTargetObject)
				{
					Vector vect = transform.translation;
					if (nuggetingSound==0)
						nuggetingSound  = SFXMANAGER->Open(nuggetingSoundID);
					SFXMANAGER->Play(nuggetingSound,GetSystemID(),&vect,SFXPLAYF_LOOPING|SFXPLAYF_NORESTART);
					bNuggeting = true;
					mode = HAR_NUGGETING;

					OBJPTR<INugget> nugget;
					pTargetObject->QueryInterface(INuggetID,nugget);
					nugget->IncHarvestCount();
				}
				else
				{
					targetPartID = myBuf->handle;
					mode = HAR_WAIT_NUGGET_CANCEL_CLIENT;
				}
			}
		}
		return;
	}
	BaseHarCommand * buf = (BaseHarCommand *) buffer;
	switch(buf->command)
	{
	case HAR_C_END_NUGGET_ON_CANCEL:
		{
			bNuggetCancelOp = true;
			break;
		}
	case HAR_C_PRE_TAKEOVER_NUG:
		{
			HarCommandWDecision * decBuf = (HarCommandWDecision *) buffer;
			if(pTargetObject)
			{
				OBJPTR<INugget> mNugget;
				pTargetObject->QueryInterface(INuggetID,mNugget);
				mNugget->DecHarvestCount();
			}
			if(decBuf->decision)
				THEMATRIX->OperationCompleted(workingID,targetPartID);

			pTargetObject = NULL;
			targetPartID = 0;
			SFXMANAGER->Stop(nuggetingSound);
			bNuggeting = false;
			mode = HAR_NO_MODE_AI_OFF;
			COMP_OP(dwMissionID);
			break;
		}
	case HAR_C_PRE_TAKEOVER:
		{
			if(mode == HAR_DOCKING_REFINERY || mode == HAR_DOCKED_TO_REFINERY )
			{
				if(pTargetObject)
				{
					OBJPTR<IPlatform> platform;
					pTargetObject->QueryInterface(IPlatformID,platform);
					if(bLockingPlatform)
					{
						platform->UnlockDock(this);
						bLockingPlatform = false;
//						THEMATRIX->OperationCompleted(workingID,pTargetObject->GetPartID());
					}
					pTargetObject = NULL;	
					targetPartID = 0;
				}
			}
			mode = HAR_NO_MODE_AI_OFF;
			COMP_OP(dwMissionID);
			break;
		}
	case HAR_C_AT_REFINERY:
		{
			//OPPRINT2("HARVESTER - Recieved HAR_C_AT_REFINERY for part 0x%X using agent:#%d\n",dwMissionID,workingID);
			CQASSERT1((mode ==  HAR_AT_REFINERY_CLIENT) || (mode == HAR_MOVING_TO_REFINERY),"mode = %d",mode);
			if(mode == HAR_AT_REFINERY_CLIENT)
			{
				mode = HAR_WAITING_TO_DOCK;
				COMP_OP(dwMissionID);
			}
			else
			{
				mode = HAR_MOVING_TO_READY_REFINERY_CLIENT;
			}
			break;
		}
	case HAR_C_DOCK_END:
		{
			//OPPRINT2("HARVESTER - Recieved HAR_C_DOCK_END for part 0x%X using agent:#%d\n",dwMissionID,workingID);
			if(pTargetObject)
			{
				OBJPTR<IPlatform> platform;
				pTargetObject->QueryInterface(IPlatformID,platform);
				if(bLockingPlatform)
				{
					platform->UnlockDock(this);
//					THEMATRIX->OperationCompleted(workingID,pTargetObject->GetPartID());
				}
				platform->DecNumDocking();
			}
			bLockingPlatform = false;
			SFXMANAGER->Stop(dockingSound);

			pTargetObject = NULL;
			targetPartID = 0;
			mode = HAR_NO_MODE_AI_ON;
			COMP_OP(dwMissionID);
			break;
		}
	case HAR_C_CANCEL_OPERATION:
	case HAR_C_TARGET_LOST:
		{
			//OPPRINT2("HARVESTER - Recieved HAR_C_TARGET_LOST for part 0x%X using agent:#%d\n",dwMissionID,workingID);
			CQASSERT((mode == HAR_WAIT_NUGGET_CANCEL_CLIENT) ||	
				(mode == HAR_WAIT_DOCKING_CANCEL_CLIENT) ||
				(mode == HAR_NO_MODE_CLIENT_WAIT_CANCEL) ||
				(mode == HAR_MOVING_TO_NUGGET) || 
				(mode == HAR_WAIT_NUGGET_ARRIVAL) ||
				(mode == HAR_ROTATING_TO_NUGGET) ||
				(mode == HAR_MOVING_TO_NUGGET) || 
				(mode == HAR_WAIT_FOR_NUGGET_BEGIN_CLIENT) ||
				(mode == HAR_DOCKED_TO_REFINERY) ||
				(mode == HAR_DOCKING_REFINERY) ||
				(mode == HAR_MOVING_TO_REFINERY) ||
				(mode == HAR_AT_REFINERY_CLIENT));
			if(mode == HAR_DOCKED_TO_REFINERY)
				SFXMANAGER->Stop(dockingSound);

			bLockingPlatform = false;
			mode = HAR_NO_MODE_AI_ON;
			COMP_OP(dwMissionID);
			break;
		}
	case HAR_C_NUGGET_MOVETO:
		{
			CQASSERT((mode == HAR_WAIT_FOR_NUGGET_BEGIN_CLIENT) ||
				(mode == HAR_NO_MODE_CLIENT_WAIT_CANCEL));
			//OPPRINT2("HARVESTER - Recieved HAR_C_NUGGET_MOVETO for part 0x%X using agent:#%d\n",dwMissionID,workingID);
			if(pTargetObject)
			{
				mode = HAR_MOVING_TO_NUGGET;
				Vector plPos = pTargetObject->GetPosition();
				Vector dirVect = transform.translation-plPos;
				dirVect.normalize();
				plPos = plPos+(dirVect*hArch->pData->nuggetTiming.offDist*GRIDSIZE);
				localMovToPos(plPos);
			}
			else
			{
				mode = HAR_WAIT_NUGGET_CANCEL_CLIENT;
			}
			break;
		}
	case HAR_C_AT_NUGGET:
		{
			//OPPRINT2("HARVESTER - Recieved HAR_C_AT_NUGGET for part 0x%X using agent:#%d\n",dwMissionID,workingID);
			CQASSERT((mode == HAR_ROTATING_TO_NUGGET) || (mode == HAR_WAIT_NUGGET_ARRIVAL) ||
				(mode == HAR_MOVING_TO_NUGGET) || (mode ==HAR_WAIT_NUGGET_CANCEL_CLIENT) ||
				(mode == HAR_NO_MODE_CLIENT_WAIT_CANCEL));
			if((mode == HAR_WAIT_NUGGET_ARRIVAL)|| (mode ==HAR_WAIT_NUGGET_CANCEL_CLIENT) ||
				(mode == HAR_NO_MODE_CLIENT_WAIT_CANCEL))
			{
				mode = HAR_WAIT_NUGGET_START;
				COMP_OP(dwMissionID);
			}
			else if(mode == HAR_ROTATING_TO_NUGGET)
			{
				mode = HAR_ROTATING_TO_READY_NUGGET_CLIENT;
				timeout = 0;
			}
			else
			{
				mode = HAR_MOVING_TO_READY_NUGGET_CLIENT;
				timeout = 0;
			}
			break;
		}
	case HAR_C_NUGGET_END:
		{
			//OPPRINT2("HARVESTER - Recieved HAR_C_NUGGET_END for part 0x%X using agent:#%d\n",dwMissionID,workingID);
			HarCommandW2Decision * myBuf = (HarCommandW2Decision *) buffer;
			if(myBuf->decision)
				THEMATRIX->OperationCompleted(workingID,targetPartID);
			
			if(pTargetObject)
			{
				OBJPTR<INugget> mNugget;
				pTargetObject->QueryInterface(INuggetID,mNugget);
				if(myBuf->decision2)
					mNugget->SetDepleted(true);
				mNugget->DecHarvestCount();
			}
			pTargetObject = NULL;
			targetPartID = 0;
			SFXMANAGER->Stop(nuggetingSound);
			bNuggeting = false;
			mode = HAR_NO_MODE_AI_ON;
			COMP_OP(dwMissionID);
			break;
		}
	default:
		CQASSERT(0 && "Bad data");
	}
}
//---------------------------------------------------------------------------
//
void HarvestShip::OnMasterChange (bool bIsMaster)
{
	repairMasterChange(bIsMaster);

	if(bIsMaster)
	{
		switch(mode)
		{
		case HAR_NO_MODE_AI_ON:
		case HAR_NO_MODE_AI_OFF:
		case HAR_MOVING_TO_REFINERY:
		case HAR_MOVING_TO_READY_REFINERY_HOST:
		case HAR_WAITING_TO_DOCK:
		case HAR_DOCKING_REFINERY:
		case HAR_DOCKED_TO_REFINERY:
		case HAR_MOVING_TO_NUGGET:
		case HAR_ROTATING_TO_NUGGET:
		case HAR_MOVING_TO_READY_NUGGET_HOST:
		case HAR_ROTATING_TO_READY_NUGGET_HOST:
		case HAR_NUGGETING:
		case HAR_MOVE_TO_RECOVERY:
		case HAR_ROTATING_TO_RECOVERY:
		case HAR_RECOVERING:
		case HAR_MOVE_TO_RECOVERY_DROP:
			{
				break;
			}
		case HAR_NO_MODE_CLIENT_WAIT_CANCEL:
		case HAR_WAIT_DOCKING_CANCEL_CLIENT:
		case HAR_WAIT_NUGGET_CANCEL_CLIENT:
		case HAR_AT_REFINERY_CLIENT:
			{
				BaseHarCommand buffer;
				buffer.command = HAR_C_AT_REFINERY;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
				mode = HAR_WAITING_TO_DOCK;
				COMP_OP(dwMissionID);
				break;
			}
		case HAR_MOVING_TO_READY_REFINERY_CLIENT:
			{
				mode = HAR_MOVING_TO_READY_REFINERY_HOST;
				break;
			}
		case HAR_WAIT_FOR_NUGGET_BEGIN_CLIENT:
			{
				if(supplies != supplyPointsMax)
				{
					BaseHarCommand buffer;
					buffer.command = HAR_C_NUGGET_MOVETO;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));

					mode = HAR_MOVING_TO_NUGGET;
					Vector plPos = pTargetObject->GetPosition();
					Vector dirVect = transform.translation-plPos;
					dirVect.normalize();
					plPos = plPos+(dirVect*hArch->pData->nuggetTiming.offDist*GRIDSIZE);
					localMovToPos(plPos);
				}
				else
				{
					BaseHarCommand buffer;
					buffer.command = HAR_C_CANCEL_OPERATION;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));

					mode = HAR_NO_MODE_AI_ON;
					COMP_OP(dwMissionID);
				}
				break;
			}
		case HAR_WAIT_NUGGET_START:
			{
				if(pTargetObject)
				{
					HarCommandWHandle buffer;
					buffer.handle = pTargetObject->GetPartID();
					OBJPTR<INugget> nugget;
					pTargetObject->QueryInterface(INuggetID,nugget);
					if(nugget->GetSupplies())
					{
					
						if(nugget->GetHarvestCount())
						{
							workingID = nugget->GetProcessID();
							THEMATRIX->AddObjectToOperation(workingID,dwMissionID);
							buffer.command = HAR_C_NUGGET_ASSIST_BEGIN;
							NET_METRIC(sizeof(buffer));
							THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(&buffer));
							nugget->IncHarvestCount();
							mode = HAR_NUGGETING;
							Vector vect = transform.translation;
							if (nuggetingSound==0)
								nuggetingSound  = SFXMANAGER->Open(nuggetingSoundID);
							SFXMANAGER->Play(nuggetingSound,GetSystemID(),&vect,SFXPLAYF_LOOPING|SFXPLAYF_NORESTART);
							bNuggeting = true;
						}
						else
						{
							ObjSet set;
							set.numObjects = 2;
							set.objectIDs[0] = dwMissionID;//my ID
							set.objectIDs[1] = pTargetObject->GetPartID(); //platforms ID
							buffer.command = HAR_C_NUGGET_BEGIN;
							NET_METRIC(sizeof(buffer));
							workingID = THEMATRIX->CreateOperation(set,&buffer,sizeof(buffer));
//							THEMATRIX->SetCancelState(workingID,false);
							nugget->SetProcessID(workingID);
							nugget->IncHarvestCount();
							mode = HAR_NUGGETING;

							Vector vect = transform.translation;
							if (nuggetingSound==0)
								nuggetingSound  = SFXMANAGER->Open(nuggetingSoundID);
							SFXMANAGER->Play(nuggetingSound,GetSystemID(),&vect,SFXPLAYF_LOOPING|SFXPLAYF_NORESTART);
							bNuggeting = true;
						}
					}
					else
					{
						mode = HAR_NO_MODE_AI_ON;
					}
				}
				else
					mode = HAR_NO_MODE_AI_ON;

				break;
			}
		case HAR_MOVING_TO_READY_NUGGET_CLIENT:
			{
				mode = HAR_MOVING_TO_READY_NUGGET_HOST;
				break;
			}
		case HAR_ROTATING_TO_READY_NUGGET_CLIENT:
			{
				mode = HAR_ROTATING_TO_READY_NUGGET_HOST;
				break;
			}
		case HAR_WAIT_NUGGET_ARRIVAL:
			{
				BaseHarCommand buffer;
				buffer.command = HAR_C_AT_NUGGET;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));

				COMP_OP(dwMissionID);
		
				OBJPTR<INugget> nugget;
				pTargetObject->QueryInterface(INuggetID,nugget);
				if(nugget->GetSupplies())
				{
					HarCommandWHandle bufferWHandle;
					bufferWHandle.handle = pTargetObject->GetPartID();
					if(nugget->GetHarvestCount())
					{
						workingID = nugget->GetProcessID();
						THEMATRIX->AddObjectToOperation(workingID,dwMissionID);
						bufferWHandle.command = HAR_C_NUGGET_ASSIST_BEGIN;
						NET_METRIC(sizeof(bufferWHandle));
						THEMATRIX->SendOperationData(workingID,dwMissionID,&bufferWHandle,sizeof(bufferWHandle));
						nugget->IncHarvestCount();
						mode = HAR_NUGGETING;
						Vector vect = transform.translation;
						if (nuggetingSound==0)
							nuggetingSound  = SFXMANAGER->Open(nuggetingSoundID);
						SFXMANAGER->Play(nuggetingSound,GetSystemID(),&vect,SFXPLAYF_LOOPING|SFXPLAYF_NORESTART);
						bNuggeting = true;
					}
					else
					{
						ObjSet set;
						set.numObjects = 2;
						set.objectIDs[0] = dwMissionID;//my ID
						set.objectIDs[1] = pTargetObject->GetPartID(); //platforms ID
						bufferWHandle.command = HAR_C_NUGGET_BEGIN;
						NET_METRIC(sizeof(bufferWHandle));
						workingID = THEMATRIX->CreateOperation(set,&bufferWHandle,sizeof(bufferWHandle));
//						THEMATRIX->SetCancelState(workingID,false);
						nugget->SetProcessID(workingID);
						nugget->IncHarvestCount();
						mode = HAR_NUGGETING;

						Vector vect = transform.translation;
						if (nuggetingSound==0)
							nuggetingSound  = SFXMANAGER->Open(nuggetingSoundID);
						SFXMANAGER->Play(nuggetingSound,GetSystemID(),&vect,SFXPLAYF_LOOPING|SFXPLAYF_NORESTART);
						bNuggeting = true;
					}
				}
				else
				{
					mode = HAR_NO_MODE_AI_ON;
				}
				break;
			}
		default:
			CQASSERT(0 && "bad mode for migration");
		}
	}
	else
	{
		switch(mode)
		{
		case HAR_NO_MODE_AI_ON:
		case HAR_NO_MODE_AI_OFF:
		case HAR_NO_MODE_CLIENT_WAIT_CANCEL:
		case HAR_MOVING_TO_REFINERY:
		case HAR_AT_REFINERY_CLIENT:
		case HAR_MOVING_TO_READY_REFINERY_CLIENT:
		case HAR_WAITING_TO_DOCK:
		case HAR_WAIT_DOCKING_CANCEL_CLIENT:
		case HAR_DOCKING_REFINERY:
		case HAR_DOCKED_TO_REFINERY:
		case HAR_MOVING_TO_NUGGET:
		case HAR_ROTATING_TO_NUGGET:
		case HAR_WAIT_NUGGET_START:
		case HAR_MOVING_TO_READY_NUGGET_CLIENT:
		case HAR_ROTATING_TO_READY_NUGGET_CLIENT:
		case HAR_WAIT_NUGGET_ARRIVAL:
		case HAR_NUGGETING:
		case HAR_WAIT_NUGGET_CANCEL_CLIENT:
		case HAR_WAIT_FOR_NUGGET_BEGIN_CLIENT:
		case HAR_MOVE_TO_RECOVERY:
		case HAR_ROTATING_TO_RECOVERY:
		case HAR_RECOVERING:
		case HAR_MOVE_TO_RECOVERY_DROP:
			{
				break;
			}
		case HAR_MOVING_TO_READY_REFINERY_HOST:
			{
				mode = HAR_MOVING_TO_READY_REFINERY_CLIENT;
				break;
			}
		case HAR_MOVING_TO_READY_NUGGET_HOST:
			{
				mode = HAR_MOVING_TO_READY_NUGGET_CLIENT;
				break;
			}
		case HAR_ROTATING_TO_READY_NUGGET_HOST:
			{
				mode = HAR_ROTATING_TO_READY_NUGGET_CLIENT;
				break;
			}
		default:
			CQASSERT(0 && "bad mode for migration");
		}
	}
}
//---------------------------------------------------------------------------
//
void HarvestShip::OnAddToOperation (U32 agentID)
{
	//OPPRINT2("HARVESTER - Added to operation, part 0x%X using agent:#%d\n",dwMissionID,agentID);
	CQASSERT(!workingID);
	posibleWorkingID = agentID;
}
//---------------------------------------------------------------------------
//
void HarvestShip::onOperationCancel (U32 agentID)
{
//	SpaceShip<HARVEST_SAVELOAD, HARVEST_INIT>::OnOperationCancel(agentID);
	//OPPRINT2("HARVESTER - OnOperationCancel for part 0x%X using agent:#%d\n",dwMissionID,agentID);

	if(agentID == workingID)
	{
		if(mode == HAR_NUGGETING)
		{
			if(THEMATRIX->IsMaster())
			{
				if(pTargetObject)
				{
					OBJPTR<INugget> mNugget;
					pTargetObject->QueryInterface(INuggetID,mNugget);
					mNugget->DecHarvestCount();
					if(!(mNugget->GetHarvestCount()))
					{
						mNugget->SetProcessID(0);
						BaseHarCommand buffer;
						buffer.command = HAR_C_END_NUGGET_ON_CANCEL;
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
						THEMATRIX->OperationCompleted(workingID,targetPartID);
					}
				}
			}
			else
			{
				if(pTargetObject)
				{
					OBJPTR<INugget> mNugget;
					pTargetObject->QueryInterface(INuggetID,mNugget);
					mNugget->DecHarvestCount();
					if(bNuggetCancelOp)
						mNugget->SetProcessID(0);
				}
				if(bNuggetCancelOp)
				{
					bNuggetCancelOp = false;
					THEMATRIX->OperationCompleted(workingID,targetPartID);
				}
			}
			bNuggeting = false;

			pTargetObject = NULL;
			targetPartID = 0;
			mode = HAR_NO_MODE_AI_OFF;
			workingID = 0;
		}
		else
		{
			CQASSERT(!bLockingPlatform);
			if(pTargetObject)
			{
				if(pTargetObject->objClass == OC_PLATFORM && (mode != HAR_MOVE_TO_RECOVERY_DROP) )
				{
					OBJPTR<IPlatform> platform;
					pTargetObject->QueryInterface(IPlatformID,platform);
					platform->DecNumDocking();
				}
				pTargetObject = NULL;
				targetPartID = 0;
			}
			if(mode == HAR_RECOVERING || mode == HAR_MOVE_TO_RECOVERY || mode == HAR_ROTATING_TO_RECOVERY)
			{
				if(mode == HAR_RECOVERING && recoveryTarget)
				{
					OBJPTR<IScriptObject>  scriptPtr;
					recoveryTarget->QueryInterface(IScriptObjectID,scriptPtr);
					if(scriptPtr)
						scriptPtr->SetTowing(false, 0);
				}
				recoverPartID = 0;
				recoveryTarget = NULL;
			}
			mode = HAR_NO_MODE_AI_OFF;
			workingID = 0;
			bNuggeting = false; 
		}
	}
}
		
void HarvestShip::preSelfDestruct (void)
{
	//OPPRINT2("HARVESTER - PreSelfDestruct called for part 0x%X working on:#%d\n",dwMissionID,workingID);
	if(workingID && HOST_CHECK)
	{
		if(mode == HAR_DOCKING_REFINERY || mode == HAR_DOCKED_TO_REFINERY )
		{
			if(pTargetObject)
			{
				OBJPTR<IPlatform> platform;
				pTargetObject->QueryInterface(IPlatformID,platform);
				if(bLockingPlatform)
				{
					platform->UnlockDock(this);
					bLockingPlatform = false;
//					THEMATRIX->OperationCompleted(workingID,pTargetObject->GetPartID());
				}
				pTargetObject = NULL;	
				targetPartID = 0;
			}
			BaseHarCommand buffer;
			buffer.command = HAR_C_PRE_TAKEOVER;
			NET_METRIC(sizeof(buffer));
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			mode = HAR_NO_MODE_AI_OFF;
			COMP_OP(dwMissionID);
		}
		else if(mode == HAR_NUGGETING)
		{
			HarCommandWDecision buffer;
			buffer.command = HAR_C_PRE_TAKEOVER_NUG;
			NET_METRIC(sizeof(buffer));

			if(pTargetObject)
			{
				OBJPTR<INugget> mNugget;
				pTargetObject->QueryInterface(INuggetID,mNugget);
				mNugget->DecHarvestCount();
				if(!(mNugget->GetHarvestCount()))
				{
					mNugget->SetProcessID(0);
				}
			}
			const ObjSet * set;
			THEMATRIX->GetOperationSet(workingID,&set);
			if(set->numObjects == 2)
				buffer.decision = true;
			else
				buffer.decision = false;
			SFXMANAGER->Stop(nuggetingSound);
			bNuggeting = false;

			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			if(buffer.decision)
				THEMATRIX->OperationCompleted(workingID,targetPartID);
			pTargetObject = NULL;
			targetPartID = 0;
			mode = HAR_NO_MODE_AI_OFF;
			COMP_OP(dwMissionID);			
		}
		else
		{
			BaseHarCommand buffer;
			buffer.command = HAR_C_PRE_TAKEOVER;
			NET_METRIC(sizeof(buffer));
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			mode = HAR_NO_MODE_AI_OFF;
			COMP_OP(dwMissionID);
		}
	}
	if(mode == HAR_MOVE_TO_RECOVERY || mode == HAR_ROTATING_TO_RECOVERY)
	{
		recoverPartID = 0;
		recoveryTarget = NULL;
	}
	if(recoveryTarget)
	{
		OBJPTR<IScriptObject>  scriptPtr;
		recoveryTarget->QueryInterface(IScriptObjectID,scriptPtr);
		if(scriptPtr)
			scriptPtr->SetTowing(false, 0);
		else
		{
			MPartNC part(recoveryTarget);
			if(part.isValid())
				part->bTowing = false;
		}
		recoverPartID = 0;
		recoveryTarget = NULL;
	}
	mode = HAR_NO_MODE_AI_OFF;
	CQASSERT(!workingID);
}
//----------------------------------------------------------------------------
//
void HarvestShip::OnStopRequest(U32 agentID)
{
	if(agentID == workingID)
	{
		if(mode == HAR_NUGGETING)
		{
			HarCommandWDecision buffer;
			buffer.command = HAR_C_PRE_TAKEOVER_NUG;
			NET_METRIC(sizeof(buffer));

			if(pTargetObject)
			{
				OBJPTR<INugget> mNugget;
				pTargetObject->QueryInterface(INuggetID,mNugget);
				mNugget->DecHarvestCount();
				if(!(mNugget->GetHarvestCount()))
				{
					mNugget->SetProcessID(0);
				}
			}
			const ObjSet * set;
			THEMATRIX->GetOperationSet(workingID,&set);
			if(set->numObjects == 2)
				buffer.decision = true;
			else
				buffer.decision = false;
			SFXMANAGER->Stop(nuggetingSound);
			bNuggeting = false;

			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			if(buffer.decision)
				THEMATRIX->OperationCompleted(workingID,targetPartID);
			pTargetObject = NULL;
			targetPartID = 0;
			mode = HAR_NO_MODE_AI_OFF;
			COMP_OP(dwMissionID);			
		}
	}
}
//----------------------------------------------------------------------------
//
bool HarvestShip::moveToDockPos()
{
	OBJPTR<IPlatform> platform;
	pTargetObject->QueryInterface(IPlatformID,platform);
	TRANSFORM dockTrans = platform->GetDockTransform();


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
			Vector targDir = dockTrans.get_k();
			targDir.normalize();
			SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
			relYaw = fixAngle(relYaw);
			result = localRotateShip(relYaw);
			Vector relDir = dockTrans.get_position();
			relDir -= transform.get_position();
			moveResult = setPosition(relDir);
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
				localRotateShip(relYaw);
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
				localRotateShip(relYaw);
				Vector relDir = try2;
				relDir -= transform.get_position();
				setPosition(relDir);
			}
		}
	}

	return moveResult && result;
}
#ifdef _DEBUG_HARVEST
#ifdef _DEBUG
//----------------------------------------------------------------------------
//
void HarvestShip::renderMath()
{
	BATCH->set_state(RPR_BATCH,false);
	DisableTextures();
	CAMERA->SetModelView();
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);


	OBJPTR<IPlatform> platform;
	pTargetObject->QueryInterface(IPlatformID,platform);
	TRANSFORM dockTrans = platform->GetDockTransform();

	IBaseObject * planet = OBJLIST->FindObject(platform->GetPlanetID());

	Vector planetPos = planet->GetPosition();

	Vector v2 = dockTrans.translation-planetPos;
	v2.normalize();

	PB.Begin(PB_LINES);
	PB.Color3ub(255,255,255);
	PB.Vertex3f(planetPos.x,planetPos.y,planetPos.z);
	PB.Vertex3f(dockTrans.translation.x,dockTrans.translation.y,dockTrans.translation.z);
	PB.End();

	Vector v1 = transform.translation-(dockTrans.translation-v2*500);
	if(v1.fast_magnitude() == 0)
		return;
	v1.normalize();
	PB.Begin(PB_LINES);
	PB.Color3ub(255,255,255);
	PB.Vertex3f(transform.translation.x,transform.translation.y,transform.translation.z);
	PB.Vertex3f((dockTrans.translation-v2*500).x,(dockTrans.translation-v2*500).y,(dockTrans.translation-v2*500).z);
	PB.End();

	//if I have a good line of sight go for it
	if(dot_product(v1,v2) >= 0)
	{
		Vector targDir = pTargetObject->GetPosition() - transform.translation;
		targDir.normalize();
		SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
		relYaw = fixAngle(relYaw);
		Vector relDir = dockTrans.get_position();
		relDir -= transform.get_position();
		PB.Begin(PB_LINES);
		PB.Color3ub(0,255,0);
		PB.Vertex3f(transform.translation.x,transform.translation.y,transform.translation.z);
		PB.Vertex3f(transform.translation.x+relDir.x,transform.translation.y+relDir.y,transform.translation.z+relDir.z);
		PB.End();

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
			Vector relDir = try1;
			relDir -= transform.get_position();
			PB.Begin(PB_LINES);
			PB.Color3ub(255,0,0);
			PB.Vertex3f(transform.translation.x,transform.translation.y,transform.translation.z);
			PB.Vertex3f(transform.translation.x+relDir.x,transform.translation.y+relDir.y,transform.translation.z+relDir.z);
			PB.End();
		}
		else
		{
			Vector targDir = try2 - transform.translation;
			targDir.normalize();
			SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
			relYaw = fixAngle(relYaw);
			Vector relDir = try2;
			relDir -= transform.get_position();
			PB.Begin(PB_LINES);
			PB.Color3ub(0,0,255);
			PB.Vertex3f(transform.translation.x,transform.translation.y,transform.translation.z);
			PB.Vertex3f(transform.translation.x+relDir.x,transform.translation.y+relDir.y,transform.translation.z+relDir.z);
			PB.End();
		}
	}

}
#endif
#endif
//----------------------------------------------------------------------------
//
void HarvestShip::doUnitAI()
{

#define MAX_AI_UPDATE_TIME 20
#define AI_ACTION_TIME 8

	//if I am full
	//	find the nearest refinery
	//  if no refinery look again at next update
	//else 
	//	find the resources nearest our last resource
	//	if none 
	//		if i hae some metal or gas 
	//			find nearest refinery
	//		else if I am at last resource spot
	//			go idle
	//		else
	//			goto last resource spot and look again
	//	else
	//		goto resource and harvest
	//		
	if(updateTimer < MAX_AI_UPDATE_TIME)
	{
		updateTimer += AI_ACTION_TIME;
		if(MISSION->IsComputerControlled(playerID))
		{
			if(gas+metal == supplyPointsMax)
			{
				IBaseObject * target = findNearRefinery();
				if(target)
				{
					bHostParking = false;
					USR_PACKET<USRHARVEST> packet;

					packet.objectID[0] = GetPartID();
					packet.targetID = target->GetPartID();
					packet.bAutoSelected = true;
					packet.init(1);
					NETPACKET->Send(HOSTID,0,&packet);
					mode = HAR_NO_MODE_AI_OFF;
				}
				else
				{
					mode = HAR_NO_MODE_AI_OFF;
					bSendIdle = true;
					bHostParking = false;
				}
			}
			else if(gas || metal)
			{
				IBaseObject * target = findHarvestTarget();
				if(target)
				{
					bHostParking = false;
					USR_PACKET<USRHARVEST> packet;
					packet.objectID[0] = GetPartID();
					packet.targetID = target->GetPartID();
					packet.bAutoSelected = true;
					packet.init(1);
					NETPACKET->Send(HOSTID,0,&packet);
					mode = HAR_NO_MODE_AI_OFF;
				}
				else
				{
					IBaseObject * refinery = findNearRefinery();
					if(gas+metal > 0 && refinery )
					{
						bHostParking = false;
						USR_PACKET<USRHARVEST> packet;
						packet.objectID[0] = GetPartID();
						packet.targetID = refinery->GetPartID();
						packet.bAutoSelected = true;
						packet.init(1);
						NETPACKET->Send(HOSTID,0,&packet);
						mode = HAR_NO_MODE_AI_OFF;
					}
					else if((!bHostParking) && harvestVector.systemID != 0 && harvestVector.systemID != -1 && (harvestVector.systemID != systemID || !(GetGridPosition().isMostlyEqual(harvestVector))))
					{
						bHostParking = true;
						USR_PACKET<USRMOVE> packet;
						packet.position = harvestVector;
						packet.objectID[0] = GetPartID();
						packet.init(1);
						NETPACKET->Send(HOSTID,0,&packet);
						mode = HAR_NO_MODE_AI_OFF;
					}
					else
					{
						mode = HAR_NO_MODE_AI_OFF;
						bSendIdle = true;
						bHostParking = false;
					}
				}
			} 
			else
			{
				mode = HAR_NO_MODE_AI_OFF;
				bSendIdle = true;
				bHostParking = false;
			}
		}
		else
		{
			if(gas+metal == supplyPointsMax)
			{
				IBaseObject * target = findNearRefinery();
				if(target)
				{
					bHostParking = false;
					USR_PACKET<USRHARVEST> packet;

					packet.objectID[0] = GetPartID();
					packet.targetID = target->GetPartID();
					packet.bAutoSelected = true;
					packet.init(1);
					NETPACKET->Send(HOSTID,0,&packet);
					mode = HAR_NO_MODE_AI_OFF;
				}
				else
				{
					mode = HAR_NO_MODE_AI_OFF;
					bSendIdle = true;
					bHostParking = false;
				}
			}
			else
			{
				IBaseObject * target = findHarvestTarget();
				if(target)
				{
					bHostParking = false;
					USR_PACKET<USRHARVEST> packet;
					packet.objectID[0] = GetPartID();
					packet.targetID = target->GetPartID();
					packet.bAutoSelected = true;
					packet.init(1);
					NETPACKET->Send(HOSTID,0,&packet);
					mode = HAR_NO_MODE_AI_OFF;
				}
				else
				{
					IBaseObject * refinery = findNearRefinery();
					if(gas+metal > 0 && refinery )
					{
						bHostParking = false;
						USR_PACKET<USRHARVEST> packet;
						packet.objectID[0] = GetPartID();
						packet.targetID = refinery->GetPartID();
						packet.bAutoSelected = true;
						packet.init(1);
						NETPACKET->Send(HOSTID,0,&packet);
						mode = HAR_NO_MODE_AI_OFF;
					}
					else if((!bHostParking) && harvestVector.systemID != 0 && harvestVector.systemID != -1 && (harvestVector.systemID != systemID || !(GetGridPosition().isMostlyEqual(harvestVector))))
					{
						bHostParking = true;
						USR_PACKET<USRMOVE> packet;
						packet.position = harvestVector;
						packet.objectID[0] = GetPartID();
						packet.init(1);
						NETPACKET->Send(HOSTID,0,&packet);
						mode = HAR_NO_MODE_AI_OFF;
					}
					else
					{
						mode = HAR_NO_MODE_AI_OFF;
						bSendIdle = true;
						bHostParking = false;
					}
				}
			}
		}
	}
}
//----------------------------------------------------------------------------
//
BOOL32 HarvestShip::updateHarvest ()
{
//	CQASSERT(isAutoMovementEnabled() || (!isMoveActive()));
	if(updateTimer > ELAPSED_TIME)
		updateTimer -= ELAPSED_TIME;
	else 
		updateTimer = 0;

	if(mode == HAR_RECOVERING)
	{
		setAltitude(0);
		localRotateShip(0);
	}
	else if(mode == HAR_MOVE_TO_RECOVERY_DROP)
	{
		if(pTargetObject)
		{
			if((!(isMoveActive())) && (systemID == pTargetObject->GetSystemID()) &&
				((GetGridPosition()-pTargetObject->GetGridPosition()) < 2.5))
			{
				// Pass in recovered part NOT the tanker part (like before)
				MScript::RunProgramsWithEvent(CQPROGFLAG_RECOVERY_DROPOFF,recoveryTarget.Ptr()->GetPartID());
				//MScript::RunProgramsWithEvent(SCPORGFLAG_RECOVERY_DROPOFF,dwMissionID);
				
				OBJPTR<IScriptObject>  scriptPtr;
				recoveryTarget->QueryInterface(IScriptObjectID,scriptPtr);
				if(scriptPtr)
					scriptPtr->SetTowing(false, 0);
				else
				{
					MPartNC part(recoveryTarget);
					if(part.isValid())
						part->bTowing = false;
				}
				recoveryTarget = NULL;
				recoverPartID = 0;
				bNuggeting = false;
				COMP_OP(dwMissionID);
				mode = HAR_NO_MODE_AI_OFF;
			}
		}
		else
		{
			mode = HAR_NO_MODE_AI_ON;
			COMP_OP(dwMissionID);
		}
	}
	if(mode == HAR_MOVE_TO_RECOVERY)
	{
		if(recoveryTarget)
		{
			if((!(isMoveActive())))
			{
				mode = HAR_ROTATING_TO_RECOVERY;

				Vector targDir = transform.translation-recoveryTarget.Ptr()->GetPosition();
				targDir.normalize();

				SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();

				bRotating = (localRotateShip(fixAngle(relYaw))==0);
			}
		}
		else
		{
			mode = HAR_NO_MODE_AI_ON;
			COMP_OP(dwMissionID);
		}
	}
	else if(mode == HAR_ROTATING_TO_RECOVERY)
	{
		setAltitude(0);
		if(recoveryTarget)
		{
			Vector targDir = transform.translation-recoveryTarget.Ptr()->GetPosition();
			targDir.normalize();

			SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
			bRotating = (localRotateShip(fixAngle(relYaw))==0);

			if(!bRotating)
			{
				localRotateShip(0);
				OBJPTR<IScriptObject>  scriptPtr;
				recoveryTarget->QueryInterface(IScriptObjectID,scriptPtr);
				if(scriptPtr && !scriptPtr->IsBeingTowed())
				{
					bTowingShip = false;
					bNuggeting = true;
					recoverYaw = recoveryTarget.Ptr()->GetTransform().get_yaw();
					recoverPos = recoveryTarget.Ptr()->GetPosition();
					recoverTime = 0;
					mode = HAR_RECOVERING;
					scriptPtr->SetTowing(true, dwMissionID);
				}
				else
				{
					MPartNC part(recoveryTarget);
					if(part.isValid() && part->bDerelict && (!(part->bTowing)))
					{
						bTowingShip = true;
						bNuggeting = true;
						recoverYaw = recoveryTarget.Ptr()->GetTransform().get_yaw();
						recoverPos = recoveryTarget.Ptr()->GetPosition();
						recoverTime = 0;
						mode = HAR_RECOVERING;
						part->bTowing = true;
					}
					else
					{
						recoveryTarget = NULL;
						recoverPartID = 0;
						mode = HAR_NO_MODE_AI_ON;
						COMP_OP(dwMissionID);
					}
				}
			}
		}
		else
		{
			mode = HAR_NO_MODE_AI_ON;
			COMP_OP(dwMissionID);
		}
	}
	else if(mode == HAR_RECOVERING)
	{
		localRotateShip(0);
		setAltitude(0);
	}
	else if((mode == HAR_MOVING_TO_NUGGET) || (mode == HAR_MOVING_TO_READY_NUGGET_CLIENT) || (mode == HAR_MOVING_TO_READY_NUGGET_HOST))
	{
		if(mode == HAR_MOVING_TO_READY_NUGGET_CLIENT)
			timeout += ELAPSED_TIME;
		if(!pTargetObject)
		{
			if(HOST_CHECK)
			{
				if(mode != HAR_MOVING_TO_READY_NUGGET_HOST)
				{
					BaseHarCommand buffer;
					buffer.command = HAR_C_TARGET_LOST;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));
				}

				mode = HAR_NO_MODE_AI_ON;
				COMP_OP(dwMissionID);
			}
			else
			{
				if(mode == HAR_MOVING_TO_READY_NUGGET_CLIENT)
				{
					mode = HAR_WAIT_NUGGET_START;
					COMP_OP(dwMissionID);
				}
				else
				{
					mode = HAR_NO_MODE_CLIENT_WAIT_CANCEL;
				}
			}
		} 
		else if((!(isMoveActive())) || ((timeout > TIMEOUT_TIME) && mode == HAR_MOVING_TO_READY_NUGGET_CLIENT))
		{
			if(HOST_CHECK && hArch->pData->nuggetTiming.offDist*3 < GetGridPosition()-pTargetObject->GetGridPosition())
			{
				if(mode != HAR_MOVING_TO_READY_NUGGET_HOST)
				{
					BaseHarCommand buffer;
					buffer.command = HAR_C_TARGET_LOST;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));
				}

				mode = HAR_NO_MODE_AI_ON;
				COMP_OP(dwMissionID);
			}
			else
			{
				Vector targDir = pTargetObject->GetPosition()-transform.translation;
				targDir.normalize();

				SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
				bRotating = (localRotateShip(fixAngle(relYaw))==0);

				if(mode == HAR_MOVING_TO_READY_NUGGET_CLIENT)
				{
					mode = HAR_ROTATING_TO_READY_NUGGET_CLIENT;
					timeout = 0;
				}
				else if(mode == HAR_MOVING_TO_READY_NUGGET_HOST)
					mode = HAR_ROTATING_TO_READY_NUGGET_HOST;
				else
					mode = HAR_ROTATING_TO_NUGGET;
			}
		}
	}
	else if((mode == HAR_ROTATING_TO_NUGGET) || (mode == HAR_ROTATING_TO_READY_NUGGET_CLIENT) || (mode == HAR_ROTATING_TO_READY_NUGGET_HOST))
	{
		if(mode == HAR_ROTATING_TO_READY_NUGGET_CLIENT)
			timeout += ELAPSED_TIME;
		if(!pTargetObject)
		{
			if(HOST_CHECK)
			{
				if(mode != HAR_ROTATING_TO_READY_NUGGET_HOST)
				{
					BaseHarCommand buffer;
					buffer.command = HAR_C_TARGET_LOST;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));
				}

				mode = HAR_NO_MODE_AI_ON;
				COMP_OP(dwMissionID);
			}
			else
			{
				if(mode == HAR_ROTATING_TO_READY_NUGGET_CLIENT)
				{
					mode = HAR_WAIT_NUGGET_START;
					COMP_OP(dwMissionID);
				}
				else
					mode = HAR_NO_MODE_CLIENT_WAIT_CANCEL;
			}
		}  
		else if((!bRotating)|| ((timeout > TIMEOUT_TIME) && mode == HAR_ROTATING_TO_READY_NUGGET_CLIENT))		
		{
			localRotateShip(0);
			setAltitude(0);
			if(HOST_CHECK)
			{
				BaseHarCommand buffer;
				if(mode != HAR_ROTATING_TO_READY_NUGGET_HOST)
				{
					buffer.command = HAR_C_AT_NUGGET;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
				}

				COMP_OP(dwMissionID);
		
				OBJPTR<INugget> nugget;
				pTargetObject->QueryInterface(INuggetID,nugget);
				if(nugget->GetSupplies())
				{
					HarCommandWHandle bufferWHandle;
					bufferWHandle.handle = pTargetObject->GetPartID();
				
					if(nugget->GetHarvestCount())
					{
						workingID = nugget->GetProcessID();
						THEMATRIX->AddObjectToOperation(workingID,dwMissionID);
						bufferWHandle.command = HAR_C_NUGGET_ASSIST_BEGIN;
						NET_METRIC(sizeof(bufferWHandle));
						THEMATRIX->SendOperationData(workingID,dwMissionID,&bufferWHandle,sizeof(bufferWHandle));
						nugget->IncHarvestCount();
						mode = HAR_NUGGETING;
						Vector vect = transform.translation;
						if (nuggetingSound==0)
							nuggetingSound  = SFXMANAGER->Open(nuggetingSoundID);
						SFXMANAGER->Play(nuggetingSound,GetSystemID(),&vect,SFXPLAYF_LOOPING|SFXPLAYF_NORESTART);
						bNuggeting = true;
					}
					else
					{
						ObjSet set;
						set.numObjects = 2;
						set.objectIDs[0] = dwMissionID;//my ID
						set.objectIDs[1] = pTargetObject->GetPartID(); //platforms ID
						bufferWHandle.command = HAR_C_NUGGET_BEGIN;
						NET_METRIC(sizeof(bufferWHandle));
						workingID = THEMATRIX->CreateOperation(set,&bufferWHandle,sizeof(bufferWHandle));
//						THEMATRIX->SetCancelState(workingID,false);
						nugget->SetProcessID(workingID);
						nugget->IncHarvestCount();
						mode = HAR_NUGGETING;

						Vector vect = transform.translation;
						if (nuggetingSound==0)
							nuggetingSound  = SFXMANAGER->Open(nuggetingSoundID);
						SFXMANAGER->Play(nuggetingSound,GetSystemID(),&vect,SFXPLAYF_LOOPING|SFXPLAYF_NORESTART);
						bNuggeting = true;
					}
				}
				else
				{
					mode = HAR_NO_MODE_AI_ON;
				}
			}
			else
			{
				if(mode == HAR_ROTATING_TO_READY_NUGGET_CLIENT)
				{
					mode = HAR_WAIT_NUGGET_START;
					COMP_OP(dwMissionID);
				}
				else
					mode = HAR_WAIT_NUGGET_ARRIVAL;
			}
		}
		else
		{
			Vector targDir = pTargetObject->GetPosition()-transform.translation;
			targDir.normalize();

			SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
			bRotating = (localRotateShip(fixAngle(relYaw))==0);
			setAltitude(0);		//  maintain altitude
		}
	}
	else if((mode == HAR_MOVING_TO_REFINERY) || (mode == HAR_MOVING_TO_READY_REFINERY_HOST))
	{
		CQASSERT(workingID);
		if((!pTargetObject) || (!(MPart(pTargetObject)->bReady)))
		{
			if(HOST_CHECK)
			{
				BaseHarCommand buffer;
				buffer.command = HAR_C_TARGET_LOST;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));

				mode = HAR_NO_MODE_AI_ON;
				COMP_OP(dwMissionID);
			}
			else if (!pTargetObject)
			{
				mode = HAR_NO_MODE_CLIENT_WAIT_CANCEL;
			}
		} 
		else if(!(isMoveActive()))
		{
			if(HOST_CHECK)
			{
				BaseHarCommand buffer;
				if(mode != HAR_MOVING_TO_READY_REFINERY_HOST)
				{
					buffer.command = HAR_C_AT_REFINERY;
					NET_METRIC(sizeof(buffer));
					THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
				}
				COMP_OP(dwMissionID);

				OBJPTR<IPlatform> platform;
				pTargetObject->QueryInterface(IPlatformID,platform);
				if(!(platform->IsDockLocked()))
				{
					platform->LockDock(this);
					bLockingPlatform = true;
					buffer.command = HAR_C_DOCK_REF_BEGIN;

//					ObjSet set;
//					set.numObjects = 2;
//					set.objectIDs[0] = dwMissionID;//my ID
//					set.objectIDs[1] = pTargetObject->GetPartID(); //platforms ID
//					NET_METRIC(sizeof(buffer));
//					workingID = THEMATRIX->CreateOperation(set,&buffer,sizeof(buffer));
					NET_METRIC(sizeof(buffer));
					workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));
					THEMATRIX->SetCancelState(workingID,false);

					OBJPTR<IPlatform> plat;
					pTargetObject->QueryInterface(IPlatformID,plat);
					mode = HAR_DOCKING_REFINERY;
				}
				else
				{
					mode = HAR_WAITING_TO_DOCK;
				}
			}
			else
			{
				mode = HAR_AT_REFINERY_CLIENT;
			}
		}
	}else if(mode == HAR_WAITING_TO_DOCK)
	{
		if(pTargetObject && (MPart(pTargetObject)->bReady))
		{
			if(HOST_CHECK)
			{
				OBJPTR<IPlatform> platform;
				pTargetObject->QueryInterface(IPlatformID,platform);
				if(!(platform->IsDockLocked()))
				{
					platform->LockDock(this);
					bLockingPlatform = true;
					BaseHarCommand buffer;
					buffer.command = HAR_C_DOCK_REF_BEGIN;

//					ObjSet set;
//					set.numObjects = 2;
//					set.objectIDs[0] = dwMissionID;//my ID
//					set.objectIDs[1] = pTargetObject->GetPartID(); //platforms ID
//					NET_METRIC(sizeof(buffer));
//					workingID = THEMATRIX->CreateOperation(set,&buffer,sizeof(buffer));
					NET_METRIC(sizeof(buffer));
					workingID = THEMATRIX->CreateOperation(dwMissionID,&buffer,sizeof(buffer));
					THEMATRIX->SetCancelState(workingID,false);

					OBJPTR<IPlatform> plat;
					pTargetObject->QueryInterface(IPlatformID,plat);

					mode = HAR_DOCKING_REFINERY;
				}	
			}
		}
		else
		{
			if(HOST_CHECK && (!bExploding))
			{
				BaseHarCommand buffer;
				buffer.command = HAR_C_TARGET_LOST;
				NET_METRIC(sizeof(buffer));
				workingID = THEMATRIX->CreateOperation(GetPartID(),&buffer,sizeof(buffer));

				mode = HAR_NO_MODE_AI_ON;
				COMP_OP(dwMissionID);
			}
			else if(!pTargetObject)
			{
				mode = HAR_NO_MODE_CLIENT_WAIT_CANCEL;
			}
		}
	}else if(mode == HAR_MOVING_TO_READY_REFINERY_CLIENT)
	{
		CQASSERT(!HOST_CHECK);
		if(!(isMoveActive()))
		{
			mode = HAR_WAITING_TO_DOCK;
			COMP_OP(dwMissionID);
		}
	}else if(mode == HAR_DOCKING_REFINERY)
	{
		if(pTargetObject)
		{
/*			OBJPTR<IPlatform> platform;
			pTargetObject->QueryInterface(IPlatformID,platform);
			TRANSFORM trans = platform->GetShipTransform();
		
			Vector targDir = pTargetObject->GetPosition() - transform.translation;
			targDir.normalize();
			SINGLE relYaw = get_angle(targDir.x,targDir.y) - transform.get_yaw();
			relYaw = fixAngle(relYaw);
			bool result = localRotateShip(relYaw);
			Vector relDir = trans.get_position();
			relDir -= transform.get_position();
			bool moveResult = setPosition(relDir);
*/
			if(moveToDockPos())
			{
				setAltitude(0);
				localRotateShip(0);
				mode = HAR_DOCKED_TO_REFINERY;
			}
		}
		else
		{
			if(HOST_CHECK)
			{
				BaseHarCommand buffer;
				buffer.command = HAR_C_TARGET_LOST;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));
				bLockingPlatform = false;

				mode = HAR_NO_MODE_AI_ON;
				COMP_OP(dwMissionID);
			}
			else
			{
				mode = HAR_NO_MODE_CLIENT_WAIT_CANCEL;
			}
		}
	}else if(mode == HAR_DOCKED_TO_REFINERY)
	{
		setAltitude(0);
		localRotateShip(0);
		Vector vect = transform.translation;
		if (dockingSound==0)
			dockingSound  = SFXMANAGER->Open(dockingSoundID);
		SFXMANAGER->Play(dockingSound,GetSystemID(),&vect,SFXPLAYF_LOOPING|SFXPLAYF_NORESTART);
		if(HOST_CHECK)
		{
			if(pTargetObject)
			{
				if(bDockingWithGas)//docking with a gas harvester not a refinery
				{
					MPartNC part(pTargetObject);
					SINGLE unloadRate = hArch->pData->dockTiming.unloadRate * MGlobals::GetHarvestUpgrade(this);
					U32 transNum = unloadRate/REALTIME_FRAMERATE;
					if(transNum == 0) ++transNum;
					if(transNum > part->supplies)
						transNum = part->supplies;
					if(transNum + gas+metal > supplyPointsMax)
						transNum = supplyPointsMax-supplies;
					gas += transNum;
					part->supplies -= transNum;
					if(supplies == supplyPointsMax)
					{
						SFXMANAGER->Stop(dockingSound);
						BaseHarCommand buffer;
						buffer.command = HAR_C_DOCK_END;
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));

						OBJPTR<IPlatform> platform;
						pTargetObject->QueryInterface(IPlatformID,platform);
						if(bLockingPlatform)
						{
							platform->UnlockDock(this);
							bLockingPlatform = false;
	//						THEMATRIX->OperationCompleted(workingID,pTargetObject->GetPartID());
						}
						platform->DecNumDocking();
						pTargetObject = NULL;
						targetPartID = 0;
						mode = HAR_NO_MODE_AI_ON;
						COMP_OP(dwMissionID);
					}
				}
				else
				{
					SINGLE unloadRate = hArch->pData->dockTiming.unloadRate * MGlobals::GetHarvestUpgrade(this);
					S32 transNum = unloadRate/REALTIME_FRAMERATE;
					if(transNum == 0) ++transNum;
					if(transNum > gas+metal)
						transNum = gas+metal;
					if(gas)
					{
						if(transNum > gas)
						{
							BANKER->AddGas(GetPlayerID(),gas*(1.0+MGlobals::GetAIBonus(playerID)));
							MGlobals::SetGasGained(playerID,MGlobals::GetGasGained(playerID)+(gas*(1.0+MGlobals::GetAIBonus(playerID))));
							transNum -= gas;
							gas = 0;
							metal -= transNum;
							BANKER->AddMetal(playerID,transNum*(1.0+MGlobals::GetAIBonus(playerID)));
							MGlobals::SetMetalGained(playerID,MGlobals::GetMetalGained(playerID)+(transNum*(1.0+MGlobals::GetAIBonus(playerID))));
						}
						else
						{
							gas -= transNum;
							BANKER->AddGas(GetPlayerID(),transNum*(1.0+MGlobals::GetAIBonus(playerID)));
							MGlobals::SetGasGained(playerID,MGlobals::GetGasGained(playerID)+(transNum*(1.0+MGlobals::GetAIBonus(playerID))));
						}
					}
					else
					{
						metal -= transNum;
						BANKER->AddMetal(GetPlayerID(),transNum*(1.0+MGlobals::GetAIBonus(playerID)));
						MGlobals::SetMetalGained(playerID,MGlobals::GetMetalGained(playerID)+(transNum*(1.0+MGlobals::GetAIBonus(playerID))));
					}
					if(gas+metal == 0)
					{
						SFXMANAGER->Stop(dockingSound);
						BaseHarCommand buffer;
						buffer.command = HAR_C_DOCK_END;
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));

						OBJPTR<IPlatform> platform;
						pTargetObject->QueryInterface(IPlatformID,platform);
						if(bLockingPlatform)
						{
							platform->UnlockDock(this);
							bLockingPlatform = false;
	//						THEMATRIX->OperationCompleted(workingID,pTargetObject->GetPartID());
						}
						platform->DecNumDocking();
						pTargetObject = NULL;
						targetPartID = 0;
						mode = HAR_NO_MODE_AI_ON;
						COMP_OP(dwMissionID);

						SetPosition(transform.translation, systemID);
						bHostParking = true;
						// redundant move, SetPosition() already does this
						/*
						USR_PACKET<USRMOVE> packet;
						packet.objectID[0] = GetPartID();
						packet.position.init(GetGridPosition(),systemID);
						packet.init(1);
						NETPACKET->Send(HOSTID,0,&packet);
						*/
					}
				}
			}else
			{
				SFXMANAGER->Stop(dockingSound);
				
				BaseHarCommand buffer;
				buffer.command = HAR_C_TARGET_LOST;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));

				bLockingPlatform = false;
				mode = HAR_NO_MODE_AI_ON;
				COMP_OP(dwMissionID);
				
				bHostParking = true;
				SetPosition(transform.translation, systemID);
				// redundant move, SetPosition() already does this
				/*
				USR_PACKET<USRMOVE> packet;
				packet.objectID[0] = GetPartID();
				packet.position.init(GetGridPosition(),systemID);
				packet.init(1);
				NETPACKET->Send(HOSTID,0,&packet);
				*/
			}
		}
	}else if(mode == HAR_NUGGETING )
	{
		setAltitude(0);
		localRotateShip(0);
		if(HOST_CHECK)
		{
			if(pTargetObject)
			{
				OBJPTR<INugget> nugPtr;
				pTargetObject->QueryInterface(INuggetID,nugPtr);
				SINGLE nuggetPerSec = hArch->pData->nuggetTiming.minePerSecond * MGlobals::GetHarvestUpgrade(this);
				harvestRemainder += ELAPSED_TIME*nuggetPerSec;
				S32 load = floor(harvestRemainder);
				harvestRemainder -= load;
				if(load)
				{
					S32 mySup = metal+gas;
					if(load + mySup > supplyPointsMax)
						load = supplyPointsMax-mySup;
					S32 nuggetSupplies = nugPtr->GetSupplies();
					if(load > nuggetSupplies)
						load = nuggetSupplies;

					if(load > 0)
					{
						if(nugPtr->GetResourceType() == M_GAS)
							gas += load;
						else
							metal += load;
						CQASSERT(nuggetSupplies-load >= 0);
						nugPtr->SetSupplies(nuggetSupplies-load);//This can NULL our pTargetObject
					}

					if((nugPtr->GetSupplies() == 0) || (load+mySup >= supplyPointsMax))
					{
//						bHostHarvestEnded = true;
						HarCommandW2Decision buffer;
						buffer.command = HAR_C_NUGGET_END;
						nugPtr->DecHarvestCount();
						if(!(nugPtr->GetHarvestCount()))
						{
							buffer.decision = true;
						}
						else
						{
							buffer.decision = false;
						}

						if(nugPtr->GetSupplies() == 0)
							buffer.decision2 = true;
						else
							buffer.decision2 = false;
	
						NET_METRIC(sizeof(buffer));
						THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));

						if(buffer.decision)
						{
							THEMATRIX->OperationCompleted(workingID,targetPartID);
						}

						if(buffer.decision2)
						{
							nugPtr->SetDepleted(true);
						}

						pTargetObject = NULL;
						targetPartID = 0;
						SFXMANAGER->Stop(nuggetingSound);
						bNuggeting = false;
						mode = HAR_NO_MODE_AI_ON;
						COMP_OP(dwMissionID);
					}
				}
			} else
			{
				HarCommandW2Decision buffer;
				buffer.command = HAR_C_NUGGET_END;
				const ObjSet * set;
				THEMATRIX->GetOperationSet(workingID,&set);
				if(set->numObjects == 2)
					buffer.decision = true;
				else
					buffer.decision = false;
				buffer.decision2 = false;
				NET_METRIC(sizeof(buffer));
				THEMATRIX->SendOperationData(workingID,GetPartID(),&buffer,sizeof(buffer));

				bNuggeting = false;
				mode = HAR_NO_MODE_AI_ON;
				if(buffer.decision)
					THEMATRIX->OperationCompleted(workingID,targetPartID);
				COMP_OP(dwMissionID);
			}
		}
	}else if(bReady && (mode == HAR_NO_MODE_AI_ON) && (HOST_CHECK) && (!bExploding))
	{
		doUnitAI();
	}
	return 1;
}

U32 HarvestShip::sendIdleMessage(void * buffer)
{
	if(bSendIdle)
	{
		HARVESTERCOMM(this,dwMissionID,planetDepleted,SUB_HAR_DEPLETED,pInitData->displayName);
		bSendIdle = false;
		return 1;
	}
	return 0;
}

void HarvestShip::putIdleMessage(void * buffer,U32 bufferSize, bool bLateDelivery)
{
	HARVESTERCOMM(this,dwMissionID,planetDepleted,SUB_HAR_DEPLETED,pInitData->displayName);
}

void HarvestShip::physUpdateHarvest (SINGLE dt)
{
	// don't run the rocking code when we've got something to do
	//bool bDesiredAutoMove = false;
	if(recoveryTarget)
	{
		if(mode != HAR_RECOVERING && mode != HAR_ROTATING_TO_RECOVERY && mode != HAR_MOVE_TO_RECOVERY )
		{
			if(bTowingShip)
			{
				SINGLE recoverDist = hArch->pData->nuggetTiming.offDist*GRIDSIZE;
				recoverDist = recoverDist*recoverDist;
				Vector pos = recoveryTarget->GetPosition();
				Vector dir = pos-transform.translation;
				OBJPTR<IPhysicalObject> physTarg;
				recoveryTarget->QueryInterface(IPhysicalObjectID,physTarg);
				if(dir.magnitude_squared() > recoverDist)
				{
					CQASSERT(physTarg);
					dir.normalize();
					physTarg->SetPosition(transform.translation+dir*(hArch->pData->nuggetTiming.offDist*GRIDSIZE), systemID);
				}	
				if(recoveryTarget.Ptr()->GetSystemID() != systemID)
				{
					if(systemID < MAX_SYSTEMS)
						physTarg->SetTransform(transform, systemID);
					UnregisterSystemVolatileWatchersForObject(recoveryTarget.Ptr());
				}
			}
			else
			{
				TRANSFORM trans;
				trans.set_orientation(tankPoint[3].orientation);
				trans.translation = tankPoint[3].point;
				trans = transform*trans;

				OBJPTR<IPhysicalObject> physTarg;
				recoveryTarget->QueryInterface(IPhysicalObjectID,physTarg);
				CQASSERT(physTarg);
				bool bUnreg = false;
				if(recoveryTarget.Ptr()->GetSystemID() != systemID)
					bUnreg = true;
				physTarg->SetTransform(trans, systemID);
				if(bUnreg)
					UnregisterSystemVolatileWatchersForObject(recoveryTarget.Ptr());
			}
		}
		else if(mode == HAR_RECOVERING)
		{
			if(bTowingShip)
			{
				mode = HAR_NO_MODE_AI_OFF;
				COMP_OP(dwMissionID);

				MScript::RunProgramsWithEvent(CQPROGFLAG_RECOVERY_PICKUP,recoveryTarget.Ptr()->GetPartID());
			}
			else
			{
				recoverTime+=dt;
				TRANSFORM trans;
				trans.set_orientation(tankPoint[3].orientation);
				trans.translation = tankPoint[3].point;
				trans = transform*trans;

				TRANSFORM hisTrans;
				SINGLE t = (recoverTime/TOTAL_RECOVER_TIME);
				if(t > 1.0)
					t = 1.0;
				hisTrans.set_position((trans.translation-recoverPos)*t+recoverPos);
				hisTrans.set_orientation(0,0,((trans.get_yaw()-recoverYaw)*t+recoverYaw)*MUL_RAD_TO_DEG);

				OBJPTR<IPhysicalObject> physTarg;
				recoveryTarget->QueryInterface(IPhysicalObjectID,physTarg);
				CQASSERT(physTarg);
				physTarg->SetTransform(hisTrans, systemID);

				if(recoverTime > TOTAL_RECOVER_TIME)
				{
					bNuggeting = false;
					mode = HAR_NO_MODE_AI_OFF;
					COMP_OP(dwMissionID);

					MScript::RunProgramsWithEvent(CQPROGFLAG_RECOVERY_PICKUP,recoveryTarget.Ptr()->GetPartID());
		//				MScript::RunProgramsWithEvent(CQPROGFLAG_RECOVERY_PICKUP,dwMissionID);
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
void HarvestShip::GotoPosition (const GRIDVECTOR & pos, U32 agentID, bool bSlowMove)
{
	CQASSERT(!workingID);
	if(bHostParking)
	{
		mode = HAR_NO_MODE_AI_ON;
	}
	else
	{
		mode = HAR_NO_MODE_AI_OFF;
	}
//	bHostParking = false;
//	action = NONE;
	SpaceShip<HARVEST_SAVELOAD, HARVEST_INIT>::GotoPosition(pos,agentID, bSlowMove);
}

//---------------------------------------------------------------------------
//
void HarvestShip::PrepareForJump (IBaseObject * jumpgate, bool bUserMove, U32 agentID, bool bSlowMove)
{
	CQASSERT(!workingID);
	mode = HAR_NO_MODE_AI_OFF;
//	bHostParking = false;
//	action = NONE;
	SpaceShip<HARVEST_SAVELOAD, HARVEST_INIT>::PrepareForJump(jumpgate, bUserMove, agentID, bSlowMove);
}

//---------------------------------------------------------------------------
//
void HarvestShip::save (HARVEST_SAVELOAD & save)
{
	save.baseSaveLoad = *static_cast<BASE_HARVEST_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void HarvestShip::load (HARVEST_SAVELOAD & load)
{
	*static_cast<BASE_HARVEST_SAVELOAD *>(this) = load.baseSaveLoad;
}
//---------------------------------------------------------------------------
//
void HarvestShip::resolve (void)
{
	if(targetPartID)
	{
		OBJLIST->FindObject(targetPartID, playerID, pTargetObject);
		if(!pTargetObject)
			NUGGETMANAGER->FindNugget(targetPartID, playerID, pTargetObject,IBaseObjectID);
	}
	if(recoverPartID)
		OBJLIST->FindObject(recoverPartID, NONSYSVOLATILEPTR, recoveryTarget,IBaseObjectID);
}
//---------------------------------------------------------------------------
//
void HarvestShip::preTakeover (U32 newMissionID, U32 troopID)
{
	if(workingID && HOST_CHECK)
	{
		if(mode == HAR_DOCKING_REFINERY || mode == HAR_DOCKED_TO_REFINERY )
		{
			if(pTargetObject)
			{
				OBJPTR<IPlatform> platform;
				pTargetObject->QueryInterface(IPlatformID,platform);
				if(bLockingPlatform)
				{
					platform->UnlockDock(this);
					bLockingPlatform = false;
//					THEMATRIX->OperationCompleted(workingID,pTargetObject->GetPartID());
				}
				pTargetObject = NULL;	
				targetPartID = 0;
			}
			BaseHarCommand buffer;
			buffer.command = HAR_C_PRE_TAKEOVER;
			NET_METRIC(sizeof(buffer));
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			mode = HAR_NO_MODE_AI_OFF;
			COMP_OP(dwMissionID);
		}
		else if(mode == HAR_NUGGETING)
		{
			HarCommandWDecision buffer;
			buffer.command = HAR_C_PRE_TAKEOVER_NUG;
			NET_METRIC(sizeof(buffer));

			if(pTargetObject)
			{
				OBJPTR<INugget> mNugget;
				pTargetObject->QueryInterface(INuggetID,mNugget);
				mNugget->DecHarvestCount();
				if(!(mNugget->GetHarvestCount()))
				{
					mNugget->SetProcessID(0);
				}
			}
			const ObjSet * set;
			THEMATRIX->GetOperationSet(workingID,&set);
			if(set->numObjects == 2)
				buffer.decision = true;
			else
				buffer.decision = false;
			SFXMANAGER->Stop(nuggetingSound);
			bNuggeting = false;

			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			if(buffer.decision)
				THEMATRIX->OperationCompleted(workingID,targetPartID);
			pTargetObject = NULL;
			targetPartID = 0;
			mode = HAR_NO_MODE_AI_OFF;
			COMP_OP(dwMissionID);			
		}
		else
		{
			BaseHarCommand buffer;
			buffer.command = HAR_C_PRE_TAKEOVER;
			NET_METRIC(sizeof(buffer));
			THEMATRIX->SendOperationData(workingID,dwMissionID,&buffer,sizeof(buffer));
			mode = HAR_NO_MODE_AI_OFF;
			COMP_OP(dwMissionID);
		}
	}
	if(mode == HAR_MOVE_TO_RECOVERY || mode == HAR_ROTATING_TO_RECOVERY)
	{
		recoverPartID = 0;
		recoveryTarget = NULL;
	}
	if(recoveryTarget)
	{
		OBJPTR<IScriptObject>  scriptPtr;
		recoveryTarget->QueryInterface(IScriptObjectID,scriptPtr);
		if(scriptPtr)
			scriptPtr->SetTowing(false, 0);
		else
		{
			MPartNC part(recoveryTarget);
			if(part.isValid())
				part->bTowing = false;
		}
		recoverPartID = 0;
		recoveryTarget = NULL;
	}
	mode = HAR_NO_MODE_AI_OFF;
}
//---------------------------------------------------------------------------
//
void HarvestShip::initHarvestShip (const HARVESTINITINFO & _data)//PARCHETYPE pArchetype, S32 archIndex, S32 animIndex)
{
	CQASSERT(supplyPointsMax <= 255);
	harvestVector.systemID = 0;
	gas = 0;
	metal = 0;
	bRotating = 0;
	netGas = 0;
	netMetal = 0;
	bHostParking = false;
	mode = HAR_NO_MODE_AI_OFF;
	targetPartID = 0;
	harvestRemainder = 0;
	updateTimer = 0;

	hArch = (HARVEST_INIT *)((void *)(&_data));

	INSTANCE_INDEX dummy = -1;
	if(_data.pData->nuggetTiming.hardpointName[0])
		FindHardpoint(_data.pData->nuggetTiming.hardpointName, dummy, beamPointInfo, instanceIndex);
	else
		beamPointInfo.point = Vector(0,0,0);
	dummy = -1;
	if(!(FindHardpoint("\\hp_tankpoint1", dummy, tankPoint[0], instanceIndex)))
	{
		tankPoint[0].point = Vector(0,400,0);
		tankPoint[0].orientation.set_identity();
	}
	if(!(FindHardpoint("\\hp_tankpoint2", dummy, tankPoint[1], instanceIndex)))
	{
		tankPoint[1].point = Vector(0,400,400);
		tankPoint[1].orientation.set_identity();
	}
	if(!(FindHardpoint("\\hp_tankpoint3", dummy, tankPoint[2], instanceIndex)))
	{
		tankPoint[2].point = Vector(0,400,800);
		tankPoint[2].orientation.set_identity();
	}
	if(!(FindHardpoint("\\hp_tankpoint4", dummy, tankPoint[3], instanceIndex)))
	{
		tankPoint[3].point = Vector(0,400,1200);
		tankPoint[3].orientation.set_identity();
	}

	dockingSoundID	  = hArch->pData->dockTiming.dockingSound;
	nuggetingSoundID  = hArch->pData->nuggetTiming.nuggetSound;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
struct IBaseObject * createHarvestShip (const HARVEST_INIT & data)
{
	HarvestShip * obj = new ObjectImpl<HarvestShip>;

 	obj->FRAME_init(data);

	return obj;
}
//------------------------------------------------------------------------------------------
//---------------------------HavestShip Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE HarvestShipFactory : public IObjectFactory
{
	struct OBJTYPE : HARVEST_INIT
	{
		OBJTYPE (void)
		{
			mineTex = 0;
		}

		~OBJTYPE (void)
		{
		/*	if(metalTankIndex != -1)
				ENGINE->destroy_instance(metalTankIndex);
			if(metalTankArch != -1)
				ENGINE->destroy_instance(gasTankIndex);*/
			if (metalTankArch != -1)
			{
				ENGINE->release_archetype(metalTankArch);
				delete metalTankRenderArch;
				delete metalTankMeshObj;
			}
			if (gasTankArch != -1)
			{
				ENGINE->release_archetype(gasTankArch);
				delete gasTankRenderArch;
				delete gasTankMeshObj;
			}

			TMANAGER->ReleaseTextureRef(mineTex);
			mineTex = 0;
			delete harvestAnimArch;
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(HarvestShipFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	HarvestShipFactory (void) { }

	~HarvestShipFactory (void);

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

	/* HarvestShipFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
HarvestShipFactory::~HarvestShipFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void HarvestShipFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE HarvestShipFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_SPACESHIP)
	{
		BT_HARVESTSHIP_DATA * data = (BT_HARVESTSHIP_DATA *) _data;

		if (data->type == SSC_HARVESTSHIP)	   
		{
			result = new OBJTYPE;
			if (result->loadSpaceshipArchetype(data, ARCHLIST->GetArchetype(szArchname)) == false)
				goto Error;

			SFXMANAGER->Preload(data->dockTiming.dockingSound);
			SFXMANAGER->Preload(data->nuggetTiming.nuggetSound);
			
			if (result->mineTex == 0)
				result->mineTex = TMANAGER->CreateTextureFromFile("nuggetbeam.tga",TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
			
			{
				DAFILEDESC fdesc="getnugget.anm";
				COMPTR<IFileSystem> objFile;
				if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
				{
					result->harvestAnimArch = ANIM2D->create_archetype(objFile);
				}
				else 
				{
					CQFILENOTFOUND(fdesc.lpFileName);
					result->harvestAnimArch =0;
					goto Error;
				}
			}

			if(data->gasTankMesh[0])
			{
				DAFILEDESC fdesc = data->gasTankMesh;
				COMPTR<IFileSystem> objFile;

				if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
					TEXLIB->load_library(objFile, 0);
				else
					goto Error;

				if ((result->gasTankArch = ENGINE->create_archetype(fdesc.lpFileName, objFile)) == INVALID_ARCHETYPE_INDEX)
					goto Error;
			//	if((result->gasTankIndex = ENGINE->create_instance2(result->gasTankArch,0)) == INVALID_INSTANCE_INDEX)
			//		goto Error;
				result->gasTankRenderArch = new RenderArch;
				result->gasTankMeshObj = CreateMeshObj(result->gasTankRenderArch,result->gasTankArch);

			}
			else
			{
				result->gasTankArch = -1;
				//result->gasTankIndex = -1;
			}
			if(data->metalTankMesh[0])
			{
				DAFILEDESC fdesc = data->metalTankMesh;
				COMPTR<IFileSystem> objFile;

				if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
					TEXLIB->load_library(objFile, 0);
				else
					goto Error;

				if ((result->metalTankArch = ENGINE->create_archetype(fdesc.lpFileName, objFile)) == INVALID_ARCHETYPE_INDEX)
					goto Error;

			//	if((result->metalTankIndex = ENGINE->create_instance2(result->metalTankArch,0)) == INVALID_INSTANCE_INDEX)
			//		goto Error;
				result->metalTankRenderArch = new RenderArch;
				result->metalTankMeshObj = CreateMeshObj(result->metalTankRenderArch,result->metalTankArch);

			}
			else
			{
				result->metalTankArch = -1;
				//result->metalTankIndex = -1;
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
BOOL32 HarvestShipFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * HarvestShipFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createHarvestShip(*objtype);
}
//-------------------------------------------------------------------
//
void HarvestShipFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _harvestship : GlobalComponent
{
	HarvestShipFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<HarvestShipFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _harvestship __ship;

//---------------------------------------------------------------------------
//--------------------------End HarvestShip.cpp--------------------------------
//---------------------------------------------------------------------------