//--------------------------------------------------------------------------//
//                                                                          //
//                                ObjComm.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ObjComm.cpp 404   8/23/01 1:53p Tmauer $
*/			   
//---------------------------------------------------------------------------
#include "pch.h"
#include <globals.h>

#include "TResClient.h"
#include "IObject.h"
#include "IPlanet.h"
#include "Objlist.h"
#include "ObjClass.h"
#include "Startup.h"
#include "Camera.h"
#include "Resource.h"
#include "Hotkeys.h"
#include "DBHotkeys.h"
#include "Sector.h"
#include "SysMap.h"
#include "SuperTrans.h"
#include "UserDefaults.h"
#include "IMissionActor.h"
#include <DPlanet.h>
#include "TerrainMap.h"
#include "IBanker.h"
#include "IUnbornMeshList.h"
#include "IWeapon.h"
#include "IArtifact.h"

#include <DSounds.h>
#include "Sfx.h"
#include "MPart.h"
#include <MGlobals.h>
#include "CommPacket.h"
#include "IGroup.h"
#include "OpAgent.h"
#include "UnitComm.h"
#include "FogofWar.h"
#include "ObjSet.h"
#include "IBuild.h"
#include "IJumpGate.h"
#include "DPlatform.h"
#include "DRepairPlat.h"
#include "DGunPlat.h"
#include "IJumpPlat.h"
#include "ITroopship.h"
#include "IAdmiral.h"
#include "IAttack.h"
#include "SoundManager.h"
#include "MScript.h"
#include "DSupplyShipSave.h"

#include <3DMath.h>
#include <FileSys.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <EventSys.h>
#include <TComponent.h>
#include <IConnection.h>
#include <TConnPoint.h>
#include <TConnContainer.h>
#include <WindowManager.h>
#include <HKEvent.h>

#include <stdlib.h>
#include <stdio.h>

static struct ObjectComm * comm;

//----------------------------//
//
enum CURSOR_MODE
{
	CM_NOT_OWNED=0,
	CM_MOVE,
	CM_BAN_INSIDE,			//  inside system boundary
	CM_BAN_OUTSIDE,		// outside system boundary
	CM_ATTACK,
	CM_HARVEST,
	CM_HARVEST_UNLOAD,
	CM_HARVEST_START,
	CM_SHIP_REPAIR,
	CM_SHIP_RESUPPLY,
	CM_SHIP_REPAIR_AND_RESUPPLY,
	CM_SHIPSELL,
	CM_JUMP,
	CM_JUMPCAMERA,
	CM_UNKJUMPCAMERA,		// can't use camera yet because it hasn't been explored
	CM_JUMPOBJECT,			// jump object only, not camera
	CM_RESUPPLY,
	CM_RESUPPLYDENIED,		// out of reach
	CM_RESUPPLYDENIED2,	// supply ship in different system than target
	CM_RESUPPLYDENIED3,	// attempt to resupply a harvest ship
	CM_ATTACKOUTOFRANGE,	// out of range for platform / deployed gunsat
	CM_SUPPLYESCORT,		// same as ESCORT
	CM_CAPTURE,
	CM_CAPTUREDENIED,
	CM_BAN_HARDFOG,
	CM_RECOVER,
	CM_DROP_OFF,
	CM_JUMPFORBIDDEN,
	CM_WORMHOLEGENERATOR,

	CMS_BEGIN,	// from here on down, the cursor mode does the same action in both right and left click mouse options
	CMS_FABREPAIR,
	CMS_SELL,
	CMS_BUILD_PLANET,
	CMS_BUILD_SPACE,
	CMS_BUILD_JUMPGATE,
	CMS_BUILDDENIED_PLANET,
	CMS_BUILDDENIED_SPACE,
	CMS_BUILDDENIED_JUMPGATE,
	CMS_BUILDFULL,
	CMS_PROBE,
	CMS_PROBEJUMP,
	CMS_PROBEJUMPUNK,
	CMS_ESCORT_DENIED,				// area
	CMS_ESCORT,				// defend a unit
	CMS_PATROL,
	CMS_PATROL_UNEXPLORED,
	CMS_MIMIC,
	CMS_SPECIAL_ATTACK,
	CMS_SPECIAL_ATTACK_WORM,
	CMS_SPECIALDENIED,		// special ability not allowed here
	CMS_RALLY,
	CMS_RAM,
	CMS_SYNTHESIS,
	CMS_TARGET_POSITION,
	CMS_SPECIAL_PLANET_ATTACK,
	CMS_ARTIFACT_PLANET_ATTACK,
};

#define MOVE_DISTANCEX IDEAL2REALX(20)
#define MOVE_DISTANCEY IDEAL2REALY(20)
#define DUP_PERIOD	   2000	
#define SELECT_GOTO_PERIOD 500

#define MAX_GROUPS 10
#define MAX_UI_PACKET_SIZE 256

inline MISSION_DATA::M_CAPS & MISSION_DATA::M_CAPS::operator |= (const MISSION_DATA::M_CAPS & other)
{
	CQASSERT(sizeof(*this) == sizeof(U32));
	U32 * ptr = (U32 *) this;
	*ptr |= *((U32 *) &other);

	return *this;
}
//--------------------------------------------------------------------------//
//
inline MISSION_DATA::M_CAPS & MISSION_DATA::M_CAPS::operator &= (const MISSION_DATA::M_CAPS & other)
{
	CQASSERT(sizeof(*this) == sizeof(U32));
	U32 * ptr = (U32 *) this;
	*ptr &= *((U32 *) &other);

	return *this;
}

struct TRUEANIMPOS
{
	NETGRIDVECTOR gridVec;
	Vector pos;
} g_trueAnimPos;

//--------------------------------------------------------------------------//
//------------------------------ObjectComm Class----------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE ObjectComm : public IEventCallback, ResourceClient<>
{
	U32 eventHandle;		// handles to callback
	BOOL32 bHasFocus;

	CURSOR_MODE cursorMode;
	bool bOwnershipInvalid;		// user moved mouse while holding down LBUTTON
	bool bLMouseDown;
	bool bEditorMode;
	bool bRallyEventSent;		// true if we told the world we were in rally mode

	enum GLOBAL_STATE
	{
		GS_EMPTY_STATE,
		GS_SELL_STATE,
		GS_SPECIAL_STATE,
		GS_DEFEND_STATE,
		GS_RALLY_STATE,
		GS_PATROL_STATE,
		GS_REPAIR_STATE,
		GS_TARGET_POSITON_STATE,
		GS_ARTIFACT
	} globalState;


	S32 oldMouseX, oldMouseY;
	bool bEditorMoveOk;
	bool bMissionClosed;

	OBJPTR<IGroup> group[MAX_GROUPS];

	union {
		struct BASE_PACKET
		{
			DWORD dwSize;
		} lastPacket;		// a copy of the last packet sent to host
		char dummyMemory[MAX_UI_PACKET_SIZE];
	};
	S32 dupTimer;			// when this is <= 0, allow a duplicate packet
	S32 selectTimer;		// when this is > 0, goto selected group
	int selectTimerGroup;	// index of group selected

	OBJPTR<IBaseObject> pRallyAnimation;

	// sound bytes
	HSOUND hsndSpecialDenied;

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ObjectComm)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	END_DACOM_MAP()

	//----------------------------------------
	// struct used for terrainMap callback
	struct TCallback : ITerrainSegCallback
	{
		const U32 allyMask;
		TCallback (void) : allyMask(MGlobals::GetAllyMask(MGlobals::GetThisPlayer()))
		{
		}

		virtual bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
		{
			if ((info.flags & (TERRAIN_IMPASSIBLE|TERRAIN_BLOCKLOS)) != 0)
			{
				IBaseObject * obj = OBJLIST->FindObject(info.missionID);
				if (obj && obj->bVisible)
					return false;
			}
			return true;
		}
	};

	struct ANIMQUEUENODE
	{
		bool bValid;
		OBJPTR<IBaseObject> pAnim;
		U32 opID;
		NETGRIDVECTOR targetPosition;
		bool bTruePosValid;
		Vector truePosition;

		ANIMQUEUENODE (void)
		{
			bValid = 0;
		}

		~ANIMQUEUENODE (void);
	};

#define MAX_QUEUE_SIZE 10

	struct ANIMQUEUELIST
	{
		ANIMQUEUENODE q[MAX_QUEUE_SIZE];
				
		~ANIMQUEUELIST (void);

		void * operator new (size_t size)
		{
			return calloc(size, 1);
		}

		void   operator delete (void *ptr)
		{
			::free(ptr);
		}
	};

	ANIMQUEUELIST * pQueueList;

	struct AgentEnumerator : IAgentEnumerator
	{
		ANIMQUEUELIST & pQueue;
		TRUEANIMPOS & trueAnimPos;
		const U32 systemID;

		AgentEnumerator (ANIMQUEUELIST & _pQueue, TRUEANIMPOS & _trueAnimPos, U32 _systemID) : pQueue(_pQueue), trueAnimPos(_trueAnimPos), systemID(_systemID)
		{
		}

		ANIMQUEUENODE * findNode (U32 opID)
		{
			int i;
			ANIMQUEUENODE * empty = 0;
			for (i = 0; i < MAX_QUEUE_SIZE; i++)
			{
				if (pQueue.q[i].opID == opID)
					return pQueue.q+i;
				if (pQueue.q[i].bValid==0 || pQueue.q[i].pAnim==0)
					empty = pQueue.q+i;
			}
			return empty;
		}

		/*
		U32 opID;
		enum HOSTCMD;
		const struct ObjSet *pSet;
		Vector targetPosition;
		U32 targetSystemID;
		*/

		virtual bool EnumerateAgent (const NODE & node)
		{
			if (node.targetSystemID != systemID)
				return true;
			else
			{
				ANIMQUEUENODE * anode = findNode(node.opID);

				if (anode)
				{
					anode->bValid = true;
					// if this is a new assignment, see if we have more accurate position information
					if (anode->opID != node.opID)
					{
						NETGRIDVECTOR testGrid;
						testGrid.init(node.targetPosition, node.targetSystemID);

						if ((anode->bTruePosValid = (testGrid == trueAnimPos.gridVec)) != 0)
						{
							anode->truePosition = trueAnimPos.pos;
						}
						delete anode->pAnim.Ptr();
						anode->pAnim = 0;
					}
					anode->opID = node.opID;
					anode->targetPosition.init(node.targetPosition, node.targetSystemID);
					if (anode->pAnim == 0)
					{
						IBaseObject * ptr = ARCHLIST->CreateUIAnim(UI_MOVE_WAYPOINT, (anode->bTruePosValid) ? anode->truePosition : node.targetPosition);
						InitObjectPointer(anode->pAnim, NONSYSVOLATILEPTR,ptr,0);
					}

					return true;
				}
				return false;
			}
		}
	};

	ObjectComm (void);

	~ObjectComm (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* ObjectComm methods */

	void setCursorMode (CURSOR_MODE newMode);

	static BOOL32 insideArea (void);

	void update (U32 dt);	// milliseconds

	void mouseSystem (U32 message, void *param);

	void optionLeftClick (U32 message, void *param);

	void optionRightClick (U32 message, void *param);

	void handleGotoPos (S32 x, S32 y, WPARAM wParam);

	void handleAttack (bool special, S32 x, S32 y, WPARAM wParam);

	void handleArtifactUse(S32 x, S32 y, WPARAM wParam);

	bool handleFormationAttack(S32 x, S32 y);

	void handleAttackPosition (S32 x, S32 y, WPARAM wParam, bool bSpecial);

	void handleAttackWorm (S32 x, S32 y, WPARAM wParam);

	void handleProbe(S32 x, S32 y, WPARAM wParam);

	void handleProbe (WPARAM wParam);

	void handleRecover (WPARAM wParam);

	void handleDropOff (WPARAM wParam);

	void handleMimic (WPARAM wParam);

	void handleStop (void);

	void DEBUG_handleExplode (void);

	void DEBUG_handleDelete (void);

	void handleDestroyUnits (void);

	void handleCloakUnits (void);

	void handleEjectArtifact (void);

	void handleUseArtifact (void);

	void handleHarvest (IBaseObject * target, WPARAM wParam);

	void handleJump (WPARAM wParam);

	void handleForbidenJump (IBaseObject * jumpgateID);

	void handleJumpCamera (IBaseObject * gate);

	void handleFabricate (IBaseObject * target, WPARAM wParam,S32 x, S32 y);

	void handleFabRepair (IBaseObject * target, WPARAM wParam);

	void handleShipRepair (IBaseObject * attacker, IBaseObject * target, WPARAM wParam);

	void handleShipSell (IBaseObject * attacker, IBaseObject * target, WPARAM wParam);

	void handleFabSalvage (IBaseObject * attacker, IBaseObject * target, WPARAM wParam);

	void handleResupply (IBaseObject * target);

	void handleGotoUnits (void);

	void handleSetRally (S32 x, S32 y);

	void handleDefend (S32 x, S32 y);

	void handleDockFlagship (IBaseObject * target);

	void handleUnDockFlagship (S32 x, S32 y);

	void handleCapture (IBaseObject * target, WPARAM wParam);
	
	void handleSpecialAbility (IBaseObject * selected, WPARAM wParam, U32 hotkey);

	void handleGroupCreate(U32 hotkey);

	void handleGroupSelect(U32 hotkey);

	void handleFleetSelect(U32 hotkey);

	void handleFleetDefine(U32 hotkey);
	
	void makeGroupsExclusive (U32 index);

	void handleStanceChange (UNIT_STANCE unitStance);

	void handleSupplyStanceChange (enum SUPPLY_SHIP_STANCE stance);

	void handleHarvestStanceChange (enum HARVEST_STANCE stance);

	void handleFighterStanceChange (enum FighterStance stance);

	void handleAdmiralTacticChange (enum ADMIRAL_TACTIC stance);

	void handleFormationChange (U32 slotID);

	void handlePatrolPos (S32 x, S32 y);

	void handleMovePos (S32 x, S32 y, WPARAM wParam);

	bool findLowestAdmiral (IBaseObject * & admiral, U32 & admiralID);

	MISSION_DATA::M_CAPS getCaps (IBaseObject * selected);

	MISSION_DATA::M_CAPS getCapsExclusive (IBaseObject * selected);

	BOOL32 mouseHasMoved (S32 x, S32 y) const;

	static BOOL32 highlightedIsSelected (void);	// true if all highlighted objects are selected as well

	IDAComponent * getBase (void)
	{
		return (IEventCallback *) this;
	}

	bool ScreenToPoint(S32 x, S32 y, Vector& vec);

	void playSound (SFX::ID id);

	bool testAreaAccess (const Vector & pos);

	bool testPlatformRange (IBaseObject * obj, const Vector & pos);

	void handleGlobalModeSwitch (U32 hotkey);

	void onMoveEvent (const NETGRIDVECTOR &vec, bool bQueued, bool bAnim, bool bSlow);

	bool onFormationMove(IBaseObject * selected,const NETGRIDVECTOR &vec, bool bQueued, bool bAnim);

	bool onFormationJump(IBaseObject * gate,IBaseObject * selected);

	void onRallyEvent (const NETGRIDVECTOR & netvec, bool bAnim);

	void onSelectionEvent (void);

	void updateDisplayQueue (void);

	void updateRallyAnimation (void);

	void destroyDisplayQueue (void)
	{
		if (OBJLIST->DEBUG_isInUpdate())
			EVENTSYS->Post(CQE_UI_DEFDELANIM, pQueueList);
		else
			delete pQueueList;
		pQueueList = 0;
	}

	void localSendPacket (const BASE_PACKET * packet)
	{
		if (dupTimer>0 && lastPacket.dwSize == packet->dwSize && memcmp(&lastPacket, packet, packet->dwSize)==0)
		{
			return;	// ignore this input
		}
		else
		{
			CQASSERT(packet->dwSize <= MAX_UI_PACKET_SIZE);
			dupTimer = DUP_PERIOD;
			memcpy(&lastPacket, packet, packet->dwSize);
			NETPACKET->Send(HOSTID, 0, packet);
		}
	}

	// return true if destination gate is the same as current system
	static bool isLoopBack (IBaseObject *selected, IBaseObject *gate);

	static bool isExplored (IBaseObject * jumpgate);

	// check to make sure that all selected items reside in the same system as one another
	static bool selectedInSameSystem (IBaseObject *selected);

	// return true if all selected ships are in the system
	static bool allInSameSystem (IBaseObject * selected, U32 systemID);

	static IBaseObject * findBestSpeaker (IBaseObject * selected, const MISSION_DATA::M_CAPS & matchCaps);

	static bool allSameType (IBaseObject * selected);

	void insertSortObj (U32 objectID[MAX_SELECTED_UNITS], int i, MPart & part);

	bool isSameCursorModeAction (CURSOR_MODE mode)
	{
		return mode > CMS_BEGIN;
	}

	void doSameCursorModeAction (MSG * msg);

	template <class Type> 
	int initializeUserPacket (Type * packet, IBaseObject * selected, const MISSION_DATA::M_CAPS & matchCaps)
	{
		int i=0;
		const U32 match = ((const U32 *) &matchCaps)[0];
		MPart part;

		while (selected)
		{
			part = selected;
			if (part.isValid())
			{
				if ((match == 0) || ((((U32 *)&part->caps)[0] & match) != 0))
				{
					insertSortObj(packet->objectID, i++, part);
//					packet->objectID[i] = part->dwMissionID;
//					i++;
				}
			}
			selected = selected->nextSelected;
		}
		CQASSERT(i && i <= MAX_SELECTED_UNITS);

		if ((selected = OBJLIST->FindGroupObject(packet->objectID, i)) != 0)
		{
			packet->objectID[0] = selected->GetPartID();
			i = 1;
		}

		packet->init(i, false);

		return i;
	}

	template <class Type> 
	int initializeUserPacketSystem (Type * packet, IBaseObject * selected, const MISSION_DATA::M_CAPS & matchCaps, U32 systemID)
	{
		int i=0;
		const U32 match = ((const U32 *) &matchCaps)[0];
		MPart part;

		while (selected)
		{
			part = selected;
			if (part.isValid())
			{
				if (((match == 0) || ((((U32 *)&part->caps)[0] & match) != 0)) && part->systemID == systemID)
				{
					insertSortObj(packet->objectID, i++, part);
//					packet->objectID[i] = part->dwMissionID;
//					i++;
				}
			}
			selected = selected->nextSelected;
		}
		CQASSERT(i && i <= MAX_SELECTED_UNITS);

		if ((selected = OBJLIST->FindGroupObject(packet->objectID, i)) != 0)
		{
			packet->objectID[0] = selected->GetPartID();
			i = 1;
		}

		packet->init(i, false);

		return i;
	}

	template <class Type> 
	void initializeUserPacket2 (Type * packet, IBaseObject * selected, IBaseObject * target)
	{
		packet->objectID[0] = selected->GetPartID();
		packet->objectID[1] = target->GetPartID();


		if ((selected = OBJLIST->FindGroupObject(packet->objectID, 2)) != 0)
		{
			packet->objectID[0] = selected->GetPartID();
			packet->init(1, false);
		}
		else
			packet->init(2, false);
	}

	template <class Type> 
	int initializeUserPacket3 (Type * packet, IBaseObject * selected, const MISSION_DATA::M_CAPS & matchCaps)
	{
		int i=0;
		const U32 match = ((const U32 *) &matchCaps)[0];
		MPart part;

		while (selected)
		{
			part = selected;
			if (part.isValid())
			{
				if (match==0 || (((U32 *)&part->caps)[0] & match) != 0)
				{
					insertSortObj(packet->objectID, i++, part);
				}
			}
			selected = selected->nextSelected;
		}

		// may not have anything in the selection list which meets the given properties
		if (i > 0)
		{
			if ((selected = OBJLIST->FindGroupObject(packet->objectID, i)) != 0)
			{
				packet->objectID[0] = selected->GetPartID();
				i = 1;
			}

			packet->init(i, false);
		}
		return i;
	}

	IBaseObject * initializeUserMovePacket (USR_PACKET<USRMOVE> * packet, IBaseObject * selected)
	{
		int i=0;
		MPart part;
		IBaseObject * speaker = 0;

		while (selected)
		{
			part = selected;
			if (part.isValid())
			{
				if (part->caps.moveOk)
				{
					if (part->caps.jumpOk || part->systemID == packet->position.systemID)
						insertSortObj(packet->objectID, i++, part);
				}
			}
			selected = selected->nextSelected;
		}

		// may not have anything in the selection list which meets the given properties
		if (i > 0)
		{
			speaker = OBJLIST->FindObject(packet->objectID[0]);		// use the first guy in the array

			if ((selected = OBJLIST->FindGroupObject(packet->objectID, i)) != 0)
			{
				packet->objectID[0] = selected->GetPartID();
				i = 1;
			}

			packet->init(i, false);
		}
		return speaker;
	}



	void renderEditorMode (void)
	{
		if (cursorMode == CM_MOVE)
		{
			RECT rect;
			Vector pos;

			getObjectMoveRect(rect,pos);

			Vector v[4];

			v[3].x = v[0].x = rect.left;
			v[0].y = v[1].y = rect.top;
			v[1].x = v[2].x = rect.right;
			v[2].y = v[3].y = rect.bottom;
			
			BATCH->set_state(RPR_BATCH,false);
			
			CAMERA->SetPerspective();
			CAMERA->SetModelView();
			DisableTextures();
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);


			if (bEditorMoveOk)
			{
				PB.Color3ub(55,55,55);
			}
			else
			{
				PB.Color3ub(180,0,0);
			}
			
			PB.Begin(PB_QUADS);
			PB.Vertex3f(v[0].x,v[0].y,0);
			PB.Vertex3f(v[1].x,v[1].y,0);
			PB.Vertex3f(v[2].x,v[2].y,0);
			PB.Vertex3f(v[3].x,v[3].y,0);
			PB.End();		
		}
	}

	bool testCapture (IBaseObject * highlite);

	static bool checkInsideSystem (const Vector & pos);

	void arrangeResources (U32 leftStatusTextID, U32 rightStatusTextID, U32 uCursorID);

	void getObjectMoveRect (RECT &rect, Vector &pos);

	void handleEditorMove (void);

	void onMissionClose (void);

	void onMissionOpen (void);

	void setOwnershipInvalid (const bool bInvalid)
	{
		bOwnershipInvalid = bInvalid;
	}

	const bool getOwnershipInvalid (void) const
	{
		return bOwnershipInvalid;
	}

	CURSOR_MODE determineCursorMode (IBaseObject* selected, IBaseObject* highlighted, const Vector & pos);

	const U32 determineLocalGlobalCursorState (const IBaseObject * selected, MISSION_DATA::M_CAPS caps,  const bool bRightClick) const;

	bool determineCursorModeNoHilight (const IBaseObject * selected, const IBaseObject * highlighted, const MISSION_DATA::M_CAPS caps, 
									   const U32 localGlobalState, CURSOR_MODE & mode, const Vector & pos);

	bool determineCursorModeHighlightSelected (const IBaseObject * selected, IBaseObject * highlighted, 
											   const MISSION_DATA::M_CAPS caps, const U32 localGlobalState, CURSOR_MODE & mode);

	bool determineWormholeGeneratorSelected (IBaseObject * selected, CURSOR_MODE & mode);

	bool determineCursorModeNotHighlightingSelected (IBaseObject * selected, IBaseObject * highlighted, const MISSION_DATA::M_CAPS caps,
													 const U32 localGlobalState, CURSOR_MODE & mode, const Vector & pos);

	bool selectedIgnoreJump(IBaseObject * selected);

	bool isBuildCursor (void) const
	{
		switch (cursorMode)
		{
		case CMS_BUILD_PLANET:
		case CMS_BUILD_SPACE:
		case CMS_BUILD_JUMPGATE:
		case CMS_BUILDDENIED_PLANET:
		case CMS_BUILDDENIED_SPACE:
		case CMS_BUILDDENIED_JUMPGATE:
		case CMS_BUILDFULL:
			return true;
		}

		return false;
	}

};
//--------------------------------------------------------------------------//
//
ObjectComm::ObjectComm (void)
{
	bHasFocus = 1;
	resPriority = RES_PRIORITY_MEDIUM;

	// load the sounds
	if (SFXMANAGER)
	hsndSpecialDenied = SFXMANAGER->Open(SFX::SPECIALDENIED);
}
//--------------------------------------------------------------------------//
//
ObjectComm::~ObjectComm (void)
{
	COMPTR<IDAConnectionPoint> connection;

//	destroyDisplayQueue();
	delete pQueueList; 
	pQueueList = 0;

	// close the  sound handles
	if (hsndSpecialDenied && SFXMANAGER)
	{
		SFXMANAGER->CloseHandle(hsndSpecialDenied);
		hsndSpecialDenied = NULL;
	}

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);
}
//-------------------------------------------------------------------
//
void ObjectComm::insertSortObj (U32 objectID[MAX_SELECTED_UNITS], int size, MPart & part)
{
	int i;
	MPart cmpObj;
	bool bCmpAdmiral, bPartAdmiral;

	bPartAdmiral = (part->bShowPartName || part->admiralID!=0 || MGlobals::IsFlagship(part->mObjClass));

	for (i = 0; i < MAX_SELECTED_UNITS; i++)
	{
		if (i >= size)
		{
			objectID[i] = part->dwMissionID;
			break;
		}

		cmpObj = OBJLIST->FindObject(objectID[i]);

		bCmpAdmiral = (cmpObj->bShowPartName || cmpObj->admiralID!=0 || MGlobals::IsFlagship(cmpObj->mObjClass));

		// if we are missionSpecific -> push if we are higher or other guy is not missionSpecific
		// if we are not missionSpecific ->push if other guy is not mission Specific and we are higher
		if ( (bPartAdmiral    && (bCmpAdmiral==0 || part->hullPointsMax > cmpObj->hullPointsMax)) ||
			 (bPartAdmiral==0 && (bCmpAdmiral==0 && part->hullPointsMax > cmpObj->hullPointsMax)) )
		{
			// push everyone else down
			int j;
			for (j = size; j > i ; j--)
			{
				objectID[j] = objectID[j-1];
			}

			objectID[i] = part->dwMissionID;
			break;
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::playSound (SFX::ID id)
{
	HSOUND hSound;
	
	if ((hSound = SFXMANAGER->Open(id)) != 0)
	{
		SFXMANAGER->Play(hSound);
		SFXMANAGER->CloseHandle(hSound);
	}
}
//-------------------------------------------------------------------
//
bool ObjectComm::ScreenToPoint(S32 x, S32 y, Vector& vec)
{
	vec.x = x;
	vec.y = y;
	vec.z = 0;

	if (y > CAMERA->GetPane()->y1)
		return false;

	return (CAMERA->ScreenToPoint(vec.x, vec.y, vec.z) != 0);
}
//-------------------------------------------------------------------
//
BOOL32 ObjectComm::mouseHasMoved (S32 x, S32 y) const
{
	return ( (abs(x - oldMouseX) > S32(MOVE_DISTANCEX)) || (abs(y - oldMouseY) > S32(MOVE_DISTANCEY)) );
}

//-------------------------------------------------------------------
//
void ObjectComm::mouseSystem (U32 message, void *param)
{
	// if the cursor state is independent
	if (isSameCursorModeAction(cursorMode) && (message == WM_LBUTTONUP || message == WM_RBUTTONUP))
	{
		doSameCursorModeAction((MSG*)param);
	}
	else if (DEFAULTS->GetDefaults()->bRightClickOption == false || bEditorMode)
	{
		optionLeftClick(message, param);
	}
	else
	{
		optionRightClick(message, param);
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::optionLeftClick (U32 message, void *param)
{
	MSG *msg = (MSG*) param;

	switch (message)
	{
	case WM_RBUTTONUP:
		if (bHasFocus)
		{
			if (ownsResources())
			{
				IBaseObject * obj;
				if ((obj = OBJLIST->GetHighlightedList()) != 0)
				{
					if (obj->objClass == OC_JUMPGATE)
					{
						handleJumpCamera(obj);
						break; // Don't clear selections, even if jumpgate is closed
					}
				}

				if (globalState != GS_EMPTY_STATE)
					globalState = GS_EMPTY_STATE;
				else
				{
					EVENTSYS->Send(CQE_NEW_SELECTION, static_cast<ObjectComm *>(this));
					OBJLIST->FlushSelectedList();
					g_trueAnimPos.gridVec.zero();
				}
			}
		}
		break;

	case WM_LBUTTONDOWN:
		bLMouseDown = 1;
		if (bHasFocus && (desiredOwnedFlags==0 || ownsResources()))
		{
			oldMouseX = short(LOWORD(msg->lParam));
			oldMouseY = short(HIWORD(msg->lParam));
			setOwnershipInvalid(0);
			if (((msg->wParam & MK_SHIFT) == 0) && pQueueList!=0)		// if shift is not pressed
				destroyDisplayQueue();
		}
		break; 

	case WM_LBUTTONUP:
		if (bHasFocus && CQFLAGS.bGameActive)
		{
			setOwnershipInvalid(0);

			if (bLMouseDown)
			{
				if (CQFLAGS.bGamePaused==0 && ownsResources())
				{
					switch (cursorMode)
					{
					case CM_BAN_OUTSIDE:
					case CM_BAN_INSIDE:
					case CM_MOVE:
						handleMovePos(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)), msg->wParam);
						break;

					case CM_RECOVER:
						handleRecover(msg->wParam);
						break;

					case CM_DROP_OFF:
						handleDropOff(msg->wParam);
						break;

					case CM_ATTACK:
						handleAttack(false, short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)), msg->wParam);
						break;
					
					case CM_CAPTURE:
						handleCapture(OBJLIST->GetHighlightedList(), msg->wParam);
						break;

					case CM_HARVEST:
					case CM_HARVEST_UNLOAD:
					case CM_HARVEST_START:
						handleHarvest(OBJLIST->GetHighlightedList(), msg->wParam);
						break;

					case CM_JUMP:
					case CM_JUMPOBJECT:
						handleJump(msg->wParam);
						break;
					
					case CM_RESUPPLY:
						handleResupply(OBJLIST->GetHighlightedList());
						break;

					case CM_SHIP_REPAIR_AND_RESUPPLY:
					case CM_SHIP_RESUPPLY:
					case CM_SHIP_REPAIR:
						handleShipRepair(OBJLIST->GetSelectedList(), OBJLIST->GetHighlightedList(), msg->wParam);
						break;

					case CM_SHIPSELL:
						handleShipSell(OBJLIST->GetSelectedList(), OBJLIST->GetHighlightedList(), msg->wParam);
						break;

					case CM_JUMPFORBIDDEN:
						handleForbidenJump(OBJLIST->GetHighlightedList());
					}
				}
			}  // end if (bLMousedown)
		}  // end case WM_LBUTTONUP
		bLMouseDown = 0;
		break;
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::doSameCursorModeAction (MSG * msg)
{
	CQASSERT(cursorMode > CMS_BEGIN);

	if (!bHasFocus || !CQFLAGS.bGameActive)
	{
		return;
	}

	if (msg->message == WM_LBUTTONUP)
	{
		// do the action, independant of mouse mode
		setOwnershipInvalid(0);
		bLMouseDown = false;

		if (CQFLAGS.bGamePaused==0 && ownsResources())
		{
			switch (cursorMode)
			{
			case CM_SUPPLYESCORT:
			case CMS_ESCORT:
				handleDefend(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)));
				break;
				
			case CMS_FABREPAIR:
				handleFabRepair(OBJLIST->GetHighlightedList(), msg->wParam);
				break;

			case CMS_PATROL:
			case CMS_PATROL_UNEXPLORED:
				handlePatrolPos(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)));
				break;

			case CMS_TARGET_POSITION:
				handleAttackPosition(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)), msg->wParam, false);
				break;

			case CMS_MIMIC:
				handleMimic(msg->wParam);
				break;

			case CMS_ARTIFACT_PLANET_ATTACK:
				handleArtifactUse(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)), msg->wParam);
				break;

			case CMS_SPECIAL_ATTACK:
			case CMS_SPECIAL_ATTACK_WORM:
			case CMS_SYNTHESIS:
			case CMS_SPECIAL_PLANET_ATTACK:
				handleAttack(true, short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)), msg->wParam);
				break;

			case CMS_PROBEJUMP:
			case CMS_PROBEJUMPUNK:
				handleProbe(msg->wParam);
				break;

			case CMS_PROBE:
				handleProbe( short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)), msg->wParam);
				break;


			case CMS_BUILD_PLANET:
			case CMS_BUILD_SPACE:
			case CMS_BUILD_JUMPGATE:
				handleFabricate(OBJLIST->GetHighlightedList(), msg->wParam,short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)));
				break;

			case CMS_RALLY:
				handleSetRally(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)));
				break;
			
/*			case CMS_ATTACH_ADMIRAL:
				handleDockFlagship(OBJLIST->GetHighlightedList());
				break;

			case CMS_DETACH_ADMIRAL:
				handleUnDockFlagship(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)));
				break;

			case CMS_ATTACH_ADMIRAL_FULL:
				// shouldn't we tell the user they can't do this?
				break;
*/
			case CMS_ESCORT_DENIED:
				// tell the dumb-ass user they need to select a friendly ship?
				break;

			case CMS_BUILDDENIED_PLANET:
			case CMS_BUILDDENIED_SPACE:
			case CMS_BUILDDENIED_JUMPGATE:
			case CMS_BUILDFULL:
				{
					MISSION_DATA::M_CAPS matchCaps;
					memset(&matchCaps, 0, sizeof(matchCaps));
					matchCaps.buildOk = true;

					IBaseObject * speaker = findBestSpeaker(OBJLIST->GetSelectedList(), matchCaps);
					if (speaker)
					{
						MPart part = speaker;
						FABRICATORCOMM2(speaker, speaker->GetPartID(), buildImposible, SUB_NO_BUILD, part.pInit->displayName);
					}
				}
				break;

			case CMS_SPECIALDENIED:
				if (hsndSpecialDenied == NULL)
				{
					hsndSpecialDenied = SFXMANAGER->Open(SFX::SPECIALDENIED);
				}
				SFXMANAGER->Play(hsndSpecialDenied, 0, NULL);
				break;

			case CMS_SELL:
				handleFabSalvage(OBJLIST->GetSelectedList(), OBJLIST->GetHighlightedList(), msg->wParam);
				break;


			default:
				CQBOMB0("what is the cursor state, I mean, what the hell am I doing here?");
				break;
			}
		}
	}
	else if (msg->message == WM_RBUTTONUP)
	{
		// cancel the cursor mode, but keep the selected list
		// goto the move cursor, right?
		globalState = GS_EMPTY_STATE;
		cursorMode = CM_MOVE;
		BUILDARCHEID = 0;
	}
	else
	{
		CQBOMB0("what message did I pass along here?");
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::optionRightClick (U32 message, void *param)
{
	MSG *msg = (MSG*) param;

	switch (message)
	{
	case WM_LBUTTONUP:
		if (bHasFocus)
		{
			IBaseObject * obj;
			if ((obj = OBJLIST->GetHighlightedList()) != 0)
			{
				if (obj->objClass == OC_JUMPGATE && obj->nextHighlighted == NULL)

				{
					handleJumpCamera(obj);
					break; // Don't clear selections, even if jumpgate is closed
				}
			}
			if (ownsResources())
			{
				if (globalState != GS_EMPTY_STATE)
					globalState = GS_EMPTY_STATE;
				else
				{
					EVENTSYS->Send(CQE_NEW_SELECTION, static_cast<ObjectComm *>(this));
					OBJLIST->FlushSelectedList();
					g_trueAnimPos.gridVec.zero();
				}

			}
			setOwnershipInvalid(0);
		}
		bLMouseDown = false;
		break;

	case WM_LBUTTONDOWN:
		bLMouseDown = true;
		if (bHasFocus)
		{
			oldMouseX = short(LOWORD(msg->lParam));
			oldMouseY = short(HIWORD(msg->lParam));
			setOwnershipInvalid(0);
		}
		break;

	case WM_RBUTTONDOWN:
		if (bHasFocus && (desiredOwnedFlags==0 || ownsResources()))
		{
			oldMouseX = short(LOWORD(msg->lParam));
			oldMouseY = short(HIWORD(msg->lParam));
			setOwnershipInvalid(0);
			if (((msg->wParam & MK_SHIFT) == 0) && pQueueList!=0)		// if shift is not pressed
				destroyDisplayQueue();
		}
		break; 

	case WM_RBUTTONUP:
		if (bHasFocus && CQFLAGS.bGameActive)
		{
			setOwnershipInvalid(0);

			if (CQFLAGS.bGamePaused==0 && ownsResources())
			{
				switch (cursorMode)
				{
				case CM_BAN_OUTSIDE:
				case CM_BAN_INSIDE:
				case CM_MOVE:
					handleMovePos(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)), msg->wParam);
					break;

				case CM_RECOVER:
					handleRecover(msg->wParam);
					break;

				case CM_DROP_OFF:
					handleDropOff(msg->wParam);
					break;

				case CM_ATTACK:
					handleAttack(false, short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)), msg->wParam);
					break;
				case CM_CAPTURE:
					handleCapture(OBJLIST->GetHighlightedList(), msg->wParam);
					break;

				case CM_HARVEST:
				case CM_HARVEST_UNLOAD:
				case CM_HARVEST_START:
					handleHarvest(OBJLIST->GetHighlightedList(), msg->wParam);
					break;

				case CM_JUMP:
				case CM_JUMPOBJECT:
					handleJump(msg->wParam);
					break;

				case CM_RESUPPLY:
					handleResupply(OBJLIST->GetHighlightedList());
					break;

				case CM_SHIP_REPAIR_AND_RESUPPLY:
				case CM_SHIP_RESUPPLY:
				case CM_SHIP_REPAIR:
					handleShipRepair(OBJLIST->GetSelectedList(), OBJLIST->GetHighlightedList(), msg->wParam);
					break;

				case CM_SHIPSELL:
					handleShipSell(OBJLIST->GetSelectedList(), OBJLIST->GetHighlightedList(), msg->wParam);
					break;

				case CM_JUMPFORBIDDEN:
					handleForbidenJump(OBJLIST->GetHighlightedList());
				}
			}
			else if (CQFLAGS.bGamePaused == 0)
			{
				IBaseObject * obj;
				if ((obj = OBJLIST->GetSelectedList()) != 0)
				{
					if (DEFAULTS->GetDefaults()->bEditorMode)
					{
						S32 x = S32(LOWORD(msg->lParam));
						S32 y = S32(HIWORD(msg->lParam));

						Vector vec;

						if (ScreenToPoint(x, y, vec) != 0)
						{
							VOLPTR(IPhysicalObject) phys;
							if ((phys = obj) != 0)
								phys->SetPosition(vec, SECTOR->GetCurrentSystem());
							ENGINE->update_instance(obj->GetObjectIndex(),0,0);
						}
					}
					else
					{
						// make sure that we can move, dumb-ass!
						if (getCaps(OBJLIST->GetSelectedList()).moveOk && ownsResources())
						{
							handleGotoPos(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)), msg->wParam);
						}
					}
				}
			}
		}  // end case WM_RBUTTONUP
		break;
	}

}
//-------------------------------------------------------------------
// receive notifications from event system
//
GENRESULT ObjectComm::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_UI_DEFDELANIM:
		{
			ANIMQUEUELIST * list = (ANIMQUEUELIST *) param;
			delete list;
		}
		break;

	case CQE_MISSION_LOAD_COMPLETE:
		onMissionOpen();
		break;
	case CQE_MISSION_CLOSE:
		if (param==0)
			onMissionClose();
		break;

	case CQE_RENDER_LAST3D:
		if (bEditorMode)
		{
			renderEditorMode();
		}
		break;

	case CQE_EDITOR_MODE:
		bEditorMode = (param != 0);
		break;

	case CQE_SELECTION_EVENT:
		onSelectionEvent();
		destroyDisplayQueue();
		break;
	case CQE_QMOVE_SELECTED_UNITS:
		onMoveEvent(*((const NETGRIDVECTOR *)param), true, true, false);
		break;
	case CQE_MOVE_SELECTED_UNITS:
		onMoveEvent(*((const NETGRIDVECTOR *)param), false, true, false);
		break;
	case CQE_SQMOVE_SELECTED_UNITS:
		onMoveEvent(*((const NETGRIDVECTOR *)param), true, true, true);
		break;
	case CQE_SMOVE_SELECTED_UNITS:
		onMoveEvent(*((const NETGRIDVECTOR *)param), false, true, true);
		break;
	case CQE_SET_RALLY_POINT:
		onRallyEvent(*((const NETGRIDVECTOR *)param), true);
		break;

	case CQE_NEW_SELECTION:
		if ((void *)static_cast<ObjectComm *>(this) != param)
		{
			globalState = GS_EMPTY_STATE;
			bLMouseDown = 0;
		}
		BUILDARCHEID = 0;
		destroyDisplayQueue();
		g_trueAnimPos.gridVec.zero();
		break;

	case CQE_KILL_FOCUS:
		if ((IDAComponent *)param != getBase())
		{
			bHasFocus = false;
			desiredOwnedFlags = 0;
			releaseResources();
		}
		break;
	case CQE_SET_FOCUS:
		bHasFocus = true;
		break;

	case CQE_UPDATE:
		if (bRallyEventSent && globalState != GS_RALLY_STATE)
		{
			bRallyEventSent = false;
			EVENTSYS->Send(CQE_UI_RALLYPOINT, 0);
		}
		if (bHasFocus)
		{
			if (CQFLAGS.bGameActive)
			{
				update(U32(param)>>10);
				updateRallyAnimation();
				if (pQueueList)
					updateDisplayQueue();
			}
			else
			{
				cursorMode = CM_NOT_OWNED;
				desiredOwnedFlags = 0;
				releaseResources();
			}
		}
		break;

	case WM_MOUSEMOVE:
		if (bHasFocus)
		{
			if (mouseHasMoved(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam))))
			{
				// ond't enable lasso while in build mode
				if (bLMouseDown && cursorMode != CM_BAN_HARDFOG && isBuildCursor()==0)
				{
					setOwnershipInvalid(1);
					setCursorMode(CM_NOT_OWNED);
				}
				else if (bEditorMode && cursorMode == CM_MOVE && ownsResources())
				{
					handleEditorMove();
				}
			}
		}
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
		break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		if (getOwnershipInvalid()) 
		{
			setOwnershipInvalid(0);

			// make extra extra sure that that fucking goddamn bLMouseDown isn't misleading
			if (message == WM_LBUTTONUP)
			{
				bLMouseDown = false;
			}
			break;
		}
		// fall-through intentional -sb

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		if (bHasFocus && CQFLAGS.bGameActive && ownsResources())
		{
			update(0);
			mouseSystem(message, param);
		}
		// make extra extra sure that that fucking goddamn bLMouseDown isn't misleading
		if (message == WM_LBUTTONUP)
		{
			bLMouseDown = false;
		}
		break;

	case CQE_DEBUG_HOTKEY:
		if (bHasFocus && CQFLAGS.bGameActive)
		{
			switch((U32)param)
			{
			case IDH_EXPLODE:
				if (CQFLAGS.bGamePaused==0 && THEMATRIX->IsMaster())
				{
					if(DEFAULTS->GetDefaults()->bEditorMode)
						DEBUG_handleDelete();
					else
					if(DEFAULTS->GetDefaults()->bCheatsEnabled)
						DEBUG_handleExplode();
				}
				break;
			} // end switch (param)
		} // end if
		break; // end case CQE_HOTKEY

	case CQE_HOTKEY:
		if (bHasFocus && CQFLAGS.bGameActive)
		{
			switch((U32)param)
			{
			case IDH_USE_ARTIFACT:
				handleUseArtifact();
				break;
			case IDH_EJECT_ARTIFACT:
				handleEjectArtifact();
				break;
			case IDH_CLOAK:
				handleCloakUnits();
				break;
			case IDH_DESTROY:
				handleDestroyUnits();
				break;
			case IDH_CENTER_SCREEN:
				handleGotoUnits();
				break;
			
			case IDH_REPAIR:
			case IDH_SELL:
			case IDH_SPECIAL_ABILITY:
			case IDH_SPECIAL_ABILITY1:
			case IDH_SPECIAL_ABILITY2:
			case IDH_DEFEND:
			case IDH_RALLY_POINT:
//			case IDH_ATTACH_ADMIRAL:
			case IDH_PATROL:
			case IDH_TARGET_POSITION:
				handleGlobalModeSwitch(U32(param));
				break;
			
			case IDH_CANCEL_COMMAND:
				if (CQFLAGS.bGamePaused==0)
					handleStop();
				break;

			case IDH_SEEK_EVENT:
//				MPart::SeekEvent();
				break;
			case IDH_GROUP_1_CREATE:
			case IDH_GROUP_2_CREATE:
			case IDH_GROUP_3_CREATE:
			case IDH_GROUP_4_CREATE:
			case IDH_GROUP_5_CREATE:
			case IDH_GROUP_6_CREATE:
			case IDH_GROUP_7_CREATE:
			case IDH_GROUP_8_CREATE:
			case IDH_GROUP_9_CREATE:
			case IDH_GROUP_0_CREATE:
				if (CQFLAGS.bGamePaused==0)
					handleGroupCreate((U32)param);
				break;

			case IDH_FLEET_GROUP_1:
			case IDH_FLEET_GROUP_2:
			case IDH_FLEET_GROUP_3:
			case IDH_FLEET_GROUP_4:
			case IDH_FLEET_GROUP_5:
			case IDH_FLEET_GROUP_6:
				if (CQFLAGS.bGamePaused == 0)
				{
					handleFleetSelect((U32)param);
				}
				break;
			case IDH_FLEET_SET_GROUP_1:
			case IDH_FLEET_SET_GROUP_2:
			case IDH_FLEET_SET_GROUP_3:
			case IDH_FLEET_SET_GROUP_4:
			case IDH_FLEET_SET_GROUP_5:
			case IDH_FLEET_SET_GROUP_6:
				if (CQFLAGS.bGamePaused == 0)
				{
					handleFleetDefine((U32)param);
				}
				break;

			case IDH_GROUP_1_SELECT:
			case IDH_GROUP_2_SELECT:
			case IDH_GROUP_3_SELECT:
			case IDH_GROUP_4_SELECT:
			case IDH_GROUP_5_SELECT:
			case IDH_GROUP_6_SELECT:
			case IDH_GROUP_7_SELECT:
			case IDH_GROUP_8_SELECT:
			case IDH_GROUP_9_SELECT:
			case IDH_GROUP_0_SELECT:
				if (CQFLAGS.bGamePaused==0)
					handleGroupSelect((U32)param);
				break;

			case IDH_STANCE_ATTACK:
				if (CQFLAGS.bGamePaused == 0)
					handleStanceChange(US_ATTACK);
				break;
			case IDH_STANCE_DEFENSIVE:
				if (CQFLAGS.bGamePaused == 0)
					handleStanceChange(US_DEFEND);
				break;
			case IDH_STANCE_STANDGROUND:
				if (CQFLAGS.bGamePaused == 0)
					handleStanceChange(US_STAND);
				break;
			case IDH_STANCE_IDLE:
				if (CQFLAGS.bGamePaused == 0)
					handleStanceChange(US_STOP);
				break;
			case IDH_FIGHTER_NORMAL:
				if (CQFLAGS.bGamePaused == 0)
					handleFighterStanceChange(FS_NORMAL);
				break;
			case IDH_FIGHTER_PATROL:
				if (CQFLAGS.bGamePaused == 0)
					handleFighterStanceChange(FS_PATROL);
				break;
			case IDH_ADMIRAL_TACTIC_SEEK:
				if (CQFLAGS.bGamePaused == 0)
					handleAdmiralTacticChange(AT_SEEK);
				break;
			case IDH_ADMIRAL_TACTIC_HOLD:
				if (CQFLAGS.bGamePaused == 0)
					handleAdmiralTacticChange(AT_HOLD);
				break;
			case IDH_ADMIRAL_TACTIC_PEACE:
				if (CQFLAGS.bGamePaused == 0)
					handleAdmiralTacticChange(AT_PEACE);
				break;
			case IDH_ADMIRAL_TACTIC_DEFEND:
				if (CQFLAGS.bGamePaused == 0)
					handleAdmiralTacticChange(AT_DEFEND);
				break;
			case IDH_SUPPLY_STANCE_FULLYAUTO:
				if (CQFLAGS.bGamePaused == 0)
					handleSupplyStanceChange(SUP_STANCE_FULLYAUTO);
				break;
			case IDH_SUPPLY_STANCE_NOAUTO:
				if (CQFLAGS.bGamePaused == 0)
					handleSupplyStanceChange(SUP_STANCE_NONE);
				break;
			case IDH_SUPPLY_STANCE_RESUPPLY:
				if (CQFLAGS.bGamePaused == 0)
					handleSupplyStanceChange(SUP_STANCE_RESUPPLY);
				break;
			case IDH_HARVEST_STANCE_NONE:
				if (CQFLAGS.bGamePaused == 0)
					handleHarvestStanceChange(HS_NO_STANCE);
				break;
			case IDH_HARVEST_STANCE_ORE:
				if (CQFLAGS.bGamePaused == 0)
					handleHarvestStanceChange(HS_ORE_HARVEST);
				break;
			case IDH_HARVEST_STANCE_GAS:
				if (CQFLAGS.bGamePaused == 0)
					handleHarvestStanceChange(HS_GAS_HARVEST);
				break;
			case IDH_FORMATION_1:
				if (CQFLAGS.bGamePaused == 0)
					handleFormationChange(0);
				break;
			case IDH_FORMATION_2:
				if (CQFLAGS.bGamePaused == 0)
					handleFormationChange(1);
				break;
			case IDH_FORMATION_3:
				if (CQFLAGS.bGamePaused == 0)
					handleFormationChange(2);
				break;
			case IDH_FORMATION_4:
				if (CQFLAGS.bGamePaused == 0)
					handleFormationChange(3);
				break;
			case IDH_FORMATION_5:
				if (CQFLAGS.bGamePaused == 0)
					handleFormationChange(4);
				break;
			case IDH_FORMATION_6:
				if (CQFLAGS.bGamePaused == 0)
					handleFormationChange(5);
				break;
				
			} // end switch (param)
		} // end if
		break; // end case CQE_HOTKEY
	}  // end switch (message)

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
void ObjectComm::arrangeResources (U32 leftStatusTextID, U32 rightStatusTextID, U32 uCursorID)
{
	const bool bRightClick = DEFAULTS->GetDefaults()->bRightClickOption;
	U32 uStatusTextID;

//	if (bRightClick && rightStatusTextID == IDS_MOVEOBJECT_R)
//		uCursorID = IDC_CURSOR_DEFAULT;

	if (rightStatusTextID && bRightClick && cursorMode < CMS_BEGIN)
	{
		uStatusTextID = rightStatusTextID;
	}
	else
	{
		uStatusTextID = leftStatusTextID;
	}

	desiredOwnedFlags = RF_CURSOR | RF_STATUS;

	if (statusTextID == uStatusTextID && cursorID == uCursorID)
	{
		if (ownsResources() == 0)
		{
			grabAllResources();
		}
	}
	else
	{
		statusTextID = uStatusTextID;
		cursorID = uCursorID;
		if (ownsResources())
		{
			setResources();
		}
		else
		{
			grabAllResources();
		}
	}
}
//--------------------------------------------------------------------------//
//
void ObjectComm::setCursorMode (CURSOR_MODE newMode)
{
	switch (newMode)
	{
	case CM_MOVE:
		arrangeResources(IDS_MOVEOBJECT, IDS_MOVEOBJECT_R, IDC_CURSOR_MOVEOBJECT);
		break;

	case CM_ATTACK:
		arrangeResources(IDS_ATTACKOBJECT, IDS_ATTACKOBJECT_R, IDC_CURSOR_ATTACK);
		break;
	
	case CM_CAPTURE:
		arrangeResources(IDS_CAPTUREOBJECT, IDS_CAPTUREOBJECT_R, IDC_CURSOR_CAPTURE);
		break;

	case CM_CAPTUREDENIED:
		arrangeResources(IDS_CAPTUREDENIED, 0, IDC_CURSOR_BAN);
		break;

	case CM_WORMHOLEGENERATOR:
		arrangeResources(IDS_WORMHOLE_GENERATOR, IDS_WORMHOLE_GENERATOR_R, IDC_CURSOR_INTERMEDIATE);
		break;
	
	case CMS_SPECIAL_ATTACK_WORM:
		arrangeResources(IDS_SPECIALATTACKOBJECT, 0, IDC_CURSOR_WORM_ATTACK);
		break;

	case CMS_ARTIFACT_PLANET_ATTACK:
	case CMS_SPECIAL_PLANET_ATTACK:
	case CMS_SPECIAL_ATTACK:
	case CMS_PROBE:
	case CMS_PROBEJUMP:
	case CMS_PROBEJUMPUNK:
		arrangeResources(IDS_SPECIALATTACKOBJECT, 0, IDC_CURSOR_SPECIAL_ATTACK);
		break;

	case CMS_SYNTHESIS:
		arrangeResources(IDS_SYNTHESIS, 0, IDC_CURSOR_SPECIAL_ATTACK);
		break;
	
	case CMS_SPECIALDENIED:
		arrangeResources(IDS_SPECIALDENIED, 0, IDC_CURSOR_DEFAULT);
		break;
	
	case CM_RECOVER:
		arrangeResources(IDS_HARVESTOBJECT, IDS_HARVESTOBJECT_R, IDC_CURSOR_HARVEST);
		break;

	case CM_DROP_OFF:
		arrangeResources(IDS_HARVESTOBJECT, IDS_HARVESTOBJECT_R, IDC_CURSOR_HARVEST);
		break;

	case CMS_MIMIC:
		arrangeResources(IDS_MIMIC, 0, IDC_CURSOR_MIMIC);
		break;

	case CM_ATTACKOUTOFRANGE:
		arrangeResources(IDS_ATTACKOUTOFRANGE, 0, IDC_CURSOR_BAN);
		break;

	case CM_BAN_HARDFOG:
		arrangeResources(IDS_BAN_HARDFOG, 0, IDC_CURSOR_BAN);
		break;

	case CM_BAN_OUTSIDE:
		arrangeResources(IDS_OUTOFBOUNDS, 0, IDC_CURSOR_MOVEOBJECT);
		break;

	case CM_BAN_INSIDE:
		arrangeResources(IDS_OFFLIMITS, 0, IDC_CURSOR_MOVEOBJECT);
		break;

	case CMS_ESCORT:
		arrangeResources(IDS_ESCORTOBJECT, 0, IDC_CURSOR_DEFEND);
		break;

	case CMS_ESCORT_DENIED:
		arrangeResources(IDS_DEFENDAREA, 0, IDC_CURSOR_DEFEND);
		break;

	case CM_SUPPLYESCORT:
		arrangeResources(IDS_SUPPLYESCORT, 0, IDC_CURSOR_DEFEND);
		break;

	case CM_JUMPOBJECT:
		arrangeResources(IDS_JUMPOBJECT, IDS_JUMPOBJECT_R, IDC_CURSOR_JUMP);
		break;

	case CM_JUMP:
		arrangeResources(IDS_JUMPBOTH, IDS_JUMPBOTH_R, IDC_CURSOR_JUMP);
		break;

	case CM_JUMPCAMERA:
		arrangeResources(IDS_JUMPCAMERA, IDS_JUMPCAMERA_R, IDC_CURSOR_DEFAULT);
		break;

	case CM_UNKJUMPCAMERA:
		arrangeResources(IDS_UNKJUMPCAMERA, 0, IDC_CURSOR_BAN);
		break;

	case CM_JUMPFORBIDDEN:
		arrangeResources(IDS_JUMPFORBIDDEN, 0, IDC_CURSOR_BAN);
		break;

	case CM_HARVEST:
		arrangeResources(IDS_HARVESTOBJECT, IDS_HARVESTOBJECT_R, IDC_CURSOR_HARVEST);
		break;

	case CM_HARVEST_START:
		arrangeResources(IDS_HARVESTSTART, IDS_HARVESTSTART_R, IDC_CURSOR_HARVEST);
		break;

	case CM_HARVEST_UNLOAD:
		arrangeResources(IDS_HARVESTUNLOAD, IDS_HARVESTUNLOAD_R, IDC_CURSOR_HARVEST);
		break;

	case CMS_BUILDDENIED_PLANET:
	case CMS_BUILD_PLANET:
		arrangeResources(IDS_SELECT_BUILD_SLOT, 0, IDC_CURSOR_BUILD);
		break;

	case CMS_BUILDDENIED_SPACE:
	case CMS_BUILD_SPACE:
		arrangeResources(IDS_SELECT_BUILD_SPACE, 0, IDC_CURSOR_BUILD);
		break;
	
	case CMS_BUILDDENIED_JUMPGATE:
	case CMS_BUILD_JUMPGATE:
		arrangeResources(IDS_SELECT_BUILD_JUMPGATE, 0, IDC_CURSOR_BUILD);
		break;

	case CMS_BUILDFULL:
		arrangeResources(IDS_BUILDFULL, 0, IDC_CURSOR_BUILD);
		break;

	case CMS_RALLY:
		arrangeResources(IDS_SETRALLYPOINT, 0, IDC_CURSOR_RALLY);
		break;

	case CMS_FABREPAIR:
		arrangeResources(IDS_FABREPAIR, 0, IDC_CURSOR_FABREPAIR);
		break;

	case CM_SHIP_RESUPPLY:
		arrangeResources(IDS_SHIP_RESUPPLY, 0, IDC_CURSOR_RESUPPLY);
		break;

	case CM_SHIP_REPAIR_AND_RESUPPLY:
		arrangeResources(IDS_SHIP_REPAIR_AND_RESUPPLY, IDS_SHIP_REPAIR_AND_RESUPPLY_R, IDC_CURSOR_REPAIR);
		break;

	case CM_SHIP_REPAIR:
		arrangeResources(IDS_SHIPREPAIR, IDS_SHIPREPAIR_R, IDC_CURSOR_REPAIR);
		break;

	case CM_SHIPSELL:
		arrangeResources(IDS_SHIPSELL, IDS_SHIPSELL_R, IDC_CURSOR_SELLOBJECT);
		break;

	case CM_RESUPPLY:
		arrangeResources(IDS_RESUPPLYOBJECT, IDS_RESUPPLYOBJECT_R, IDC_CURSOR_RESUPPLY);
		break;

	case CM_RESUPPLYDENIED:
		arrangeResources(IDS_RESUPPLYDENIED, 0, IDC_CURSOR_BAN);
		break;
		
	case CM_RESUPPLYDENIED2:
		arrangeResources(IDS_RESUPPLYDENIED2, 0, IDC_CURSOR_BAN);
		break;

	case CM_RESUPPLYDENIED3:
		arrangeResources(IDS_RESUPPLYDENIED3, 0, IDC_CURSOR_BAN);
		break;

	case CMS_SELL:
		arrangeResources(IDS_SELLOBJECT, 0, IDC_CURSOR_SELLOBJECT);
		break;

	case CMS_RAM:
		arrangeResources(IDS_RAMOBJECT, 0, IDC_CURSOR_RAM);
		break;

/*	case CMS_ATTACH_ADMIRAL:
		arrangeResources(IDS_ATTACH_ADMIRAL, 0, IDC_CURSOR_MOVE_ADMIRAL);
		break;

	case CMS_ATTACH_ADMIRAL_FULL:
		arrangeResources(IDS_ATTACH_ADMIRAL_FULL, 0, IDC_CURSOR_BAN);
		break;

	case CMS_DETACH_ADMIRAL:
		arrangeResources(IDS_DETACH_ADMIRAL, 0, IDC_CURSOR_MOVE_ADMIRAL);
		break;

	case CMS_ATTACH_ADMIRAL_FIND:
		arrangeResources(IDS_ATTACH_FIND, 0, IDC_CURSOR_INTERMEDIATE);
		break;
*/
	case CMS_PATROL:
		arrangeResources(IDS_PATROL, IDS_PATROL_R, IDC_CURSOR_PATROL);
		break;

	case CMS_PATROL_UNEXPLORED:
		arrangeResources(IDS_BAN_HARDFOG, 0, IDC_CURSOR_PATROL);
		break;

	case CMS_TARGET_POSITION:
		arrangeResources(IDS_SPECIALATTACKOBJECT, 0, IDC_CURSOR_SPECIAL_ATTACK);
		break;

	case CM_NOT_OWNED:
		desiredOwnedFlags = 0;
		releaseResources();
		break;
	};

	cursorMode = newMode;
}
//-------------------------------------------------------------------
//
BOOL32 ObjectComm::insideArea (void)
{
	S32 x, y;

	WM->GetCursorPos(x, y);

	return (y <= CAMERA->GetPane()->y1);
}
//-------------------------------------------------------------------
//
void ObjectComm::updateRallyAnimation (void)
{
	// do we have a build platform selected?
	IBaseObject * obj = OBJLIST->GetSelectedList();
	if (obj)
	{
		MPart part = obj;
		if (part.isValid())
		{
			if (part->caps.buildOk && obj->objClass == OC_PLATFORM)
			{
				// does this platform have a rally point set?
				OBJPTR<IPlatform> platform;
				obj->QueryInterface(IPlatformID, platform);
				
				if (platform)
				{
					NETGRIDVECTOR ptRally = g_trueAnimPos.gridVec;
					if (ptRally.isZero()==0 || platform->GetRallyPoint(ptRally))
					{
						if (ptRally.systemID == SECTOR->GetCurrentSystem())
						{
							// do we need to create an animation
							if (pRallyAnimation == NULL || g_trueAnimPos.gridVec != ptRally)
							{
								Vector pos = (g_trueAnimPos.gridVec == ptRally) ? g_trueAnimPos.pos : ptRally;

								delete pRallyAnimation.Ptr();							
								IBaseObject * ptr =  ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, pos);
								InitObjectPointer(pRallyAnimation, NONSYSVOLATILEPTR,ptr,0);
								g_trueAnimPos.gridVec = ptRally;
								g_trueAnimPos.pos = pos;
							}
						}
					}
				}
			}
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::update (U32 dt)
{
	CURSOR_MODE newMode = cursorMode;

	if (dupTimer > 0)
		dupTimer -= dt;
	if (selectTimer > 0)
		selectTimer -= dt;

	if (insideArea() && bHasFocus && getOwnershipInvalid() == 0)
	{
		if (DEFAULTS->GetDefaults()->bEditorMode==0)
		{
			S32 x, y;
			Vector pos;

			WM->GetCursorPos(x, y);
			ScreenToPoint(x, y, pos);
			
			newMode = determineCursorMode(OBJLIST->GetSelectedList(),
												 OBJLIST->GetHighlightedList(), pos);

			if (CQFLAGS.bGamePaused)
			{
				switch (newMode)
				{
				case CM_JUMP:
				case CMS_PROBEJUMP:
					newMode = CM_JUMPCAMERA;
					break;
				
				case CM_JUMPOBJECT:
				case CMS_PROBEJUMPUNK:
					newMode = CM_UNKJUMPCAMERA;
					break;
				
				case CM_JUMPCAMERA:
				case CM_UNKJUMPCAMERA:
					break;

				default:
					newMode = CM_NOT_OWNED;
					break;
				}
			}

			// must check every frame now, because "ban" can change if a ship moves under it
			//
			// check to make sure mouse is within the system boundary, for certain modes
			//
			bool bInsideRect = checkInsideSystem(pos);

			// if we are in capture mode, then check to see if we can really capture the object
			if (newMode == CM_CAPTURE)
			{
				if (testCapture(OBJLIST->GetHighlightedList()) == false)
				{
					newMode = CM_CAPTUREDENIED;
				}
			}

			if (newMode == CM_RESUPPLY)
			{
				if (testPlatformRange(OBJLIST->GetSelectedList(), pos) == false)
					newMode = CM_RESUPPLYDENIED;
			}

			if (newMode == CM_ATTACK)
			{
				if (testPlatformRange(OBJLIST->GetSelectedList(), pos) == false)
					newMode = CM_ATTACKOUTOFRANGE;
			}

			if (bInsideRect==0)
			{
				switch (newMode)
				{
				case CM_MOVE:
					newMode = CM_BAN_OUTSIDE;

				case CMS_ESCORT_DENIED:
				case CMS_RALLY:
					newMode = CM_BAN_OUTSIDE;
					break;

				default:
					if (OBJLIST->GetHighlightedList() == 0)
					{
						if (newMode == CM_MOVE || FOGOFWAR->CheckHardFogVisibility(MGlobals::GetThisPlayer(), SECTOR->GetCurrentSystem(), pos))
							newMode = CM_BAN_OUTSIDE;
					}
				} // end switch()
			}

			if (newMode == CM_MOVE && ownsResources())
			{
				SECTOR->HighlightMoveSpot(pos);
			}

			if (newMode == CM_JUMP || newMode == CM_JUMPOBJECT)
			{
				SECTOR->HighlightJumpgate(OBJLIST->GetHighlightedList());
			}
		}
		else
		{
			if (OBJLIST->GetSelectedList() != 0 && OBJLIST->GetHighlightedList() == 0)
			{
				newMode = CM_MOVE;
			}
			else
				newMode = CM_NOT_OWNED;
		}
	}
	else
		newMode = CM_NOT_OWNED;

	setCursorMode(newMode);
}
//--------------------------------------------------------------------------//
//
MISSION_DATA::M_CAPS ObjectComm::getCaps (IBaseObject * selected)
{
	MISSION_DATA::M_CAPS caps;
	MPart hSource;
	bool bShiftOn = (HOTKEY->GetVkeyState(VK_LSHIFT) || HOTKEY->GetVkeyState(VK_RSHIFT));

	memset(&caps, 0, sizeof(caps));

	while (selected)
	{
		hSource = selected;
		if (hSource.isValid())
		{
			if (bShiftOn || THEMATRIX->IsFabricating(hSource.obj) == 0)
				caps |= hSource->caps;
		}

		selected = selected->nextSelected;
	}

	return caps;
}
//--------------------------------------------------------------------------//
//
MISSION_DATA::M_CAPS ObjectComm::getCapsExclusive (IBaseObject * selected)
{
	MISSION_DATA::M_CAPS caps;
	MPart hSource;

	if (selected==0)
		memset(&caps, 0, sizeof(caps));
	else
	{
		memset(&caps, 0xFF, sizeof(caps));

		while (selected)
		{
			hSource = selected;
			if (hSource.isValid())
				caps &= hSource->caps;

			selected = selected->nextSelected;
		}
	}

	return caps;
}
//--------------------------------------------------------------------------//
//
const U32 ObjectComm::determineLocalGlobalCursorState (const IBaseObject * selected, MISSION_DATA::M_CAPS caps,  const bool bRightClick) const
{
	// if we have a fabricator selected *IN RIGHT CLICK MODE* , then return the old global state since fabs won't
	// have a cursor change with the alt key
	if (bRightClick)
	{
		// if we have a BUILDARCHEID, then make sure our global state is empty
		if (BUILDARCHEID)
		{
			return GS_EMPTY_STATE;
		}

		if (caps.repairOk && selected->objClass == OC_SPACESHIP)
		{
			return globalState; 
		}
		else
		{
			return (globalState==GS_EMPTY_STATE && (selected && selected->nextSelected==0) && (HOTKEY->GetVkeyState(VK_LMENU)||HOTKEY->GetVkeyState(VK_RMENU))) ? GS_SPECIAL_STATE : globalState;	
		}
	}
	else
	{
		return (globalState==GS_EMPTY_STATE && (selected && selected->nextSelected==0) && (HOTKEY->GetVkeyState(VK_LMENU)||HOTKEY->GetVkeyState(VK_RMENU))) ? GS_SPECIAL_STATE : globalState;
	}
}
//--------------------------------------------------------------------------//
//
// return true if we are to stop determining the cursor mode after this function call
bool ObjectComm::determineCursorModeNoHilight (const IBaseObject * selected, const IBaseObject * highlighted, 
												  const MISSION_DATA::M_CAPS caps, const U32 localGlobalState, CURSOR_MODE & mode, const Vector & pos)
{
	//
	// do we have a harvester in the bunch?
	//
	if (highlighted && caps.harvestOk)
	{
		if (highlighted->objClass == OC_NUGGET)
		{
			//
			// is this a harvestable planet/nugget?
			//
			mode = CM_HARVEST;
			return true;
		}
	}

	if (localGlobalState != GS_EMPTY_STATE)
	{
		switch (localGlobalState)
		{
		case GS_RALLY_STATE:
			mode = CMS_RALLY;
			break;

		case GS_REPAIR_STATE:
			mode = CMS_FABREPAIR;
			break;
		
		case GS_SELL_STATE:
			if (BUILDARCHEID || caps.buildOk==0)
				globalState=GS_EMPTY_STATE;
			else
				mode = CMS_SELL;
			break;
		
		case GS_DEFEND_STATE:
			mode = CMS_ESCORT_DENIED;
			break;
		
		case GS_PATROL_STATE:
			mode = CMS_PATROL;
			break;

		case GS_TARGET_POSITON_STATE:
			mode = CMS_TARGET_POSITION;
			break;

		case GS_ARTIFACT:
			if(selected)
			{
				VOLPTR(IArtifactHolder) holder = OBJLIST->GetSelectedList();
				if(holder && holder->HasArtifact())
				{
					VOLPTR(IArtifact) artifact = holder->GetArtifact();
					if(artifact)
					{
						if(artifact->IsTargetArea())
						{
							mode = CMS_TARGET_POSITION;
						}
					}
				}
			}
			break;
		case GS_SPECIAL_STATE:
			if (selected && selected->objClass == OC_SPACESHIP)
			{
				if (caps.specialEOAOk)
				{
					mode = CMS_SPECIAL_ATTACK;
				}
				else if (caps.probeOk)
				{
					mode = CMS_PROBE;
				}
				else if (caps.moveOk)
				{
					mode = CM_MOVE;
				}
//					else
//					{
//						mode = CMS_SPECIALDENIED;
//					}
			}
			break;
		}
	}
	else if (caps.buildOk && BUILDARCHEID) //we could be building a free standing object
	{
		BASIC_DATA * data = (BASIC_DATA *) (ARCHLIST->GetArchetypeData(BUILDARCHEID));
		if(data->objClass == OC_PLATFORM)
		{
			BASE_PLATFORM_DATA * platData = (BASE_PLATFORM_DATA *) data;

			// must be built around a planet, or on a jumpgate
			if (platData->slotsNeeded != 0 || platData->type == PC_JUMPPLAT)
			{
				if (platData->slotsNeeded != 0)
					mode = CMS_BUILDDENIED_PLANET;
				else
					mode = CMS_BUILDDENIED_JUMPGATE;
				return true;
			}
			else
			{
				const U32 currentSystem = SECTOR->GetCurrentSystem();
				if (FOGOFWAR->CheckHardFogVisibility(MGlobals::GetThisPlayer(), currentSystem, pos) == 0)
				{
					// not explored, anything inside the system is ok
					COMPTR<ITerrainMap> map;
					SECTOR->GetTerrainMap(currentSystem, map);
					GRIDVECTOR gpos;
					gpos = pos;
					if(map->IsGridInSystem(gpos))
						mode = CMS_BUILD_SPACE;
					else
						mode = CMS_BUILDDENIED_SPACE;
					return true;
				}
				else
				{
					const bool bIsVisible = FOGOFWAR->CheckVisiblePosition(pos)!=0;
					COMPTR<ITerrainMap> map;
					GRIDVECTOR gpos;
					gpos = pos;
					SECTOR->GetTerrainMap(currentSystem, map);
					if (map->IsOkForBuilding(gpos, bIsVisible, false))
						mode = CMS_BUILD_SPACE;
					else
						mode = CMS_BUILDDENIED_SPACE;
					return true;
				}
			}
		}
		else if (caps.moveOk)
		{
			mode = CM_MOVE;
		}

	}
	else // no global state
	{
		if (caps.moveOk)
			mode = CM_MOVE;
	}
	
	return false;
}
//--------------------------------------------------------------------------//
//
// return true if we are to stop determining the cursor mode after this function call
bool ObjectComm::determineCursorModeHighlightSelected(const IBaseObject * selected, IBaseObject * highlighted, 
													  const MISSION_DATA::M_CAPS caps, const U32 localGlobalState, CURSOR_MODE & mode)
{
	// we are guarenteed to have a valid highlight here
	MPart hTarget = highlighted;

	if (globalState!=GS_EMPTY_STATE)
	{
		switch (globalState)
		{
		case GS_SELL_STATE:
		case GS_ARTIFACT:
		case GS_SPECIAL_STATE:
		case GS_DEFEND_STATE:
		case GS_RALLY_STATE:
//		case GS_ATTACH_ADMIRAL_STATE:
//		case GS_DETACH_ADMIRAL_STATE:
		case GS_PATROL_STATE:
		case GS_TARGET_POSITON_STATE:
			if (selected==0)
				globalState = GS_EMPTY_STATE;	// selected object destroyed!?
			break;

		} // end switch
		if (hTarget.pInit->mObjClass == M_PLANET)
		{
			mode = CM_BAN_INSIDE;
			return true;
		}
	} 

	//
	// is this a jumpgate?
	//
	if (hTarget.pInit->mObjClass == M_JUMPGATE)
	{
		OBJPTR<IJumpGate> jump;
		hTarget.obj->QueryInterface(IJumpGateID,jump);
		if(jump->IsJumpAllowed())
		{
			if (isExplored(hTarget.obj))
			{
				mode = CM_JUMPCAMERA;
				return true;
			}
			else
			{
				mode = CM_UNKJUMPCAMERA;		// can't use this jumpgate yet
				return true;
			}
		}
		else
		{
			mode = CM_JUMPFORBIDDEN;
			return true;
		}
	}
	return false;
}
//--------------------------------------------------------------------------//
//
bool ObjectComm::determineCursorModeNotHighlightingSelected (IBaseObject * selected, IBaseObject * highlighted, const MISSION_DATA::M_CAPS caps,
															 const U32 localGlobalState, CURSOR_MODE & mode, const Vector & pos)
{
	// we are guarenteed to have a valid highlight here
	MPart hTarget = highlighted;

	const U32 playerID = MGlobals::GetThisPlayer();
	const bool overrideMod = (HOTKEY->GetVkeyState(VK_LCONTROL)||HOTKEY->GetVkeyState(VK_RCONTROL));
	const bool lassoAdd = (HOTKEY->GetVkeyState(VK_LSHIFT)||HOTKEY->GetVkeyState(VK_RSHIFT));
	OBJPTR<IPlanet> planet;

	if(localGlobalState == GS_ARTIFACT)
	{
		if(selected)
		{
			VOLPTR(IArtifactHolder) holder = selected;
			if(holder && holder->HasArtifact())
			{
				VOLPTR(IArtifact) artifact = holder->GetArtifact();
				if(artifact)
				{
					if(artifact->IsTargetPlanet() && (highlighted->objClass == OC_PLANETOID || highlighted->objClass == OC_PLATFORM))
					{
						if(highlighted->objClass == OC_PLATFORM)
						{
							VOLPTR(IPlatform) plat = highlighted;
							if(plat)
							{
								if(!plat->IsDeepSpacePlatform())
								{
									mode = CMS_ARTIFACT_PLANET_ATTACK;
									return true;
								}
							}
						}
						else
						{
							mode = CMS_ARTIFACT_PLANET_ATTACK;
							return true;
						}
					}
				}
			}
		}
	}

	if (localGlobalState==GS_SPECIAL_STATE && caps.mimicOk && highlighted->objClass == OC_SPACESHIP)
	{
		mode = CMS_MIMIC;
		return true;
	}

	if (localGlobalState==GS_SPECIAL_STATE && caps.synthesisOk && highlighted->objClass == OC_SPACESHIP 
		&& (MGlobals::AreAllies(highlighted->GetPlayerID(), selected->GetPlayerID())))
	{
		mode = CMS_SYNTHESIS;
		return true;
	}

	if (localGlobalState==GS_SPECIAL_STATE && caps.specialAttackWormOk && highlighted->objClass == OC_JUMPGATE)
	{
		mode = CMS_SPECIAL_ATTACK_WORM;
		return true;
	}

	if (localGlobalState==GS_SPECIAL_STATE && caps.specialTargetPlanetOk && 
		(highlighted->objClass == OC_PLANETOID || highlighted->objClass == OC_PLATFORM))
	{
		if(highlighted->objClass == OC_PLATFORM)
		{
			VOLPTR(IPlatform) plat = highlighted;
			if(plat)
			{
				if(!plat->IsDeepSpacePlatform())
				{
					mode = CMS_SPECIAL_PLANET_ATTACK;
					return true;
				}
			}
		}
		else
		{
			mode = CMS_SPECIAL_PLANET_ATTACK;
			return true;
		}
	}

	// is target attackable ?
	if ((hTarget.obj->objClass & CF_PLAYERALIGNED) != 0 && (hTarget->playerID==0 || overrideMod || MGlobals::AreAllies(hTarget->playerID,playerID)==0))
	{
		if ((caps.specialAttackOk||caps.specialEOAOk) && localGlobalState == GS_SPECIAL_STATE)
		{
			mode = CMS_SPECIAL_ATTACK;
			return true;
		}

		if (caps.specialAttackShipOk && (localGlobalState == GS_SPECIAL_STATE))
		{
			if (hTarget.obj->objClass == OC_SPACESHIP)
				mode = CMS_SPECIAL_ATTACK;
			else
				mode = CMS_SPECIALDENIED;

			return true;
		}

		if (caps.attackOk)
		{
			mode = CM_ATTACK;
			return true;
		}
		if (caps.captureOk && hTarget->bReady)
		{
			mode = CM_CAPTURE;
			return true;
		}
	}

	// is this a jumpgate? && not in build mode
	if (hTarget.pInit->mObjClass == M_JUMPGATE && BUILDARCHEID==0)
	{
		OBJPTR<IJumpGate> jump;
		hTarget.obj->QueryInterface(IJumpGateID,jump);
		if (jump->IsJumpAllowed() && (jump->PlayerCanJump(playerID) || selectedIgnoreJump(selected)))
		{
			if (localGlobalState == GS_SPECIAL_STATE && caps.probeOk)
			{
				if (isExplored(hTarget.obj))
				{
					mode = CMS_PROBEJUMP;
				}
				else
				{
					mode = CMS_PROBEJUMPUNK;
				}
			}
			else if (caps.jumpOk && allInSameSystem(selected,hTarget->systemID))
			{
				if (isExplored(hTarget.obj))
				{
					mode = CM_JUMP;
				}
				else
				{
					mode = CM_JUMPOBJECT;
				}
				return true;
			}
			else
			{
				if (isExplored(hTarget.obj))
				{
					mode = CM_JUMPCAMERA;
					return true;
				}
				else
				{
					mode = CM_UNKJUMPCAMERA;		// can't use this jumpgate yet
					return true;
				}
			}
		}
		else
		{
			mode = CM_JUMPFORBIDDEN;
			return true;
		}
	}

	if (hTarget->playerID && playerID)
	{
		// make sure the playerID of the target is valid before checking for allies
		if (globalState == GS_REPAIR_STATE && hTarget->playerID != 0 && MGlobals::AreAllies(hTarget->playerID,playerID) && (hTarget.obj->objClass == OC_PLATFORM))
		{
			if ((BUILDARCHEID && lassoAdd == false) || caps.buildOk==0)  // remove repair ability if currently building (unless shift-repairing)
			{
				mode = CM_NOT_OWNED;
			}
			else
			{
				mode = CMS_FABREPAIR;
			}
			return true;
		}
	}

	if (globalState == GS_SELL_STATE && hTarget->playerID == playerID && (hTarget.obj->objClass==OC_PLATFORM))
	{
		if (BUILDARCHEID || caps.buildOk==0)  // remove sell ability if currently building (unless shift-selling)
		{
			mode = CM_NOT_OWNED;
		}
		else
		{
			mode = CMS_SELL;
		}
		return true;
	}

	if (globalState == GS_DEFEND_STATE && caps.defendOk && hTarget->playerID != 0 && MGlobals::AreAllies(hTarget->playerID,playerID))
	{
		if (lassoAdd == false)
		{
			mode = CMS_ESCORT;
			return true;
		}
	}

	if (globalState == GS_DEFEND_STATE && caps.supplyOk && hTarget->playerID!=0 && (hTarget->caps.attackOk||hTarget->caps.moveOk) && MGlobals::AreAllies(hTarget->playerID,playerID))
	{
		if (lassoAdd == false)
		{
			mode = CM_SUPPLYESCORT;
			return true;
		}
	}

	if (localGlobalState == GS_SPECIAL_STATE && caps.supplyOk && hTarget->playerID && MGlobals::AreAllies(hTarget->playerID,playerID))
	{
		if (hTarget.obj->objClass == OC_SPACESHIP)
		{
			if (lassoAdd == false)
			{
				if (hTarget->caps.attackOk==0)
					mode = CM_RESUPPLYDENIED3;
				else
				if (hTarget->systemID==selected->GetSystemID())
					mode = CM_RESUPPLY;
				else
				{
					if (selected->objClass == OC_SPACESHIP)
						mode = CM_RESUPPLYDENIED2;
					else
						mode = CM_RESUPPLYDENIED;
				}
				return true;
			}
		}
	}

	highlighted->QueryInterface(IPlanetID, planet);

	// do we have a harvester in the bunch?
	if (caps.harvestOk)
	{
		if ((hTarget->mObjClass == M_HEAVYREFINERY || hTarget->mObjClass == M_REFINERY || 
			hTarget->mObjClass == M_COLLECTOR|| hTarget->mObjClass == M_SUPERHEAVYREFINERY || 
			hTarget->mObjClass == M_OXIDATOR|| hTarget->mObjClass == M_GREATER_COLLECTOR ||
			hTarget->mObjClass == M_COALESCER) && MGlobals::AreAllies(hTarget->playerID,playerID))
		{
			MPart part = selected;
			if (part->supplies != 0)
			{
				mode = CM_HARVEST_UNLOAD;
				return true;
			}
			else
			{
				mode = CM_HARVEST_START;
				return true;
			}
		}
	}

	// do we have a builder in the bunch?
	if (caps.buildOk)
	{
		if (globalState == GS_RALLY_STATE)
		{
			mode = CMS_RALLY;
			return true;
		}
		if (planet!=0)
		{
			// 
			// either we can't see it clearly, or there is an open slot
			//
			const bool bHasOpenSlots = planet->HasOpenSlots();
			const bool bIsVisible = FOGOFWAR->CheckVisiblePosition(pos)!=0;

			if (bIsVisible==0 || bHasOpenSlots)
			{
				if (BUILDARCHEID)
				{
					BASIC_DATA * data = (BASIC_DATA *) (ARCHLIST->GetArchetypeData(BUILDARCHEID));
					if (data && data->objClass == OC_PLATFORM)
					{
						BASE_PLATFORM_DATA * platData = (BASE_PLATFORM_DATA *) data;

						if (platData->slotsNeeded != 0)
						{
							mode = CMS_BUILD_PLANET;
						}
						else
						{
							if (platData->type == PC_JUMPPLAT)
								mode = CMS_BUILDDENIED_JUMPGATE;
							else
								mode = CMS_BUILDDENIED_SPACE;
						}
						return true;
					}
				}
			}
			else  // all filled up
			{
				if (BUILDARCHEID)
				{
					mode = CMS_BUILDFULL;

					BASIC_DATA * data = (BASIC_DATA *) (ARCHLIST->GetArchetypeData(BUILDARCHEID));
					if (data && data->objClass == OC_PLATFORM)
					{
						BASE_PLATFORM_DATA * platData = (BASE_PLATFORM_DATA *) data;
						if (platData->slotsNeeded == 0)
						{
							if (platData->type == PC_JUMPPLAT)
								mode = CMS_BUILDDENIED_JUMPGATE;
							else
								mode = CMS_BUILDDENIED_SPACE;
						}
					}
	
					return true;
				}
			}
		}
		else
		if (hTarget.pInit->mObjClass == M_JUMPGATE && BUILDARCHEID!=0)
		{
			BASE_PLATFORM_DATA * data = (BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(BUILDARCHEID));
			if(data->type == PC_JUMPPLAT)
				mode = CMS_BUILD_JUMPGATE;
			else
			if (data->slotsNeeded == 0)
				mode = CMS_BUILDDENIED_SPACE;
			else
				mode = CMS_BUILDDENIED_PLANET;
			return true;
		}
	}

	// ships asking to be repaired at a platform
	if (selected->objClass==OC_SPACESHIP && caps.moveOk && (hTarget->caps.repairOk || (hTarget->caps.supplyOk &&  hTarget.obj->objClass == OC_PLATFORM)) &&
		hTarget->bReady && (hTarget.obj->objClass==OC_PLATFORM))
	{
		if (MGlobals::AreAllies(hTarget->playerID, playerID))
		{
			if (hTarget->caps.repairOk && hTarget->caps.supplyOk)
			{
				mode = CM_SHIP_REPAIR_AND_RESUPPLY;
			}
			else if (hTarget->caps.repairOk == true)// && hTarget->caps.supplyOk == false)
			{
				mode = CM_SHIP_REPAIR;
			}
			else
			{
				mode = CM_SHIP_RESUPPLY;
			}
			return true;
		}
	}
	
	// ships asking to be sold at a platform
	if (selected->objClass==OC_SPACESHIP && caps.moveOk && hTarget->caps.salvageOk &&  hTarget.obj->objClass == OC_PLATFORM &&
		hTarget->bReady)
	{
		if (hTarget->playerID == playerID)
		{
			mode = CM_SHIPSELL;
			return true;
		}
	}

	// ship trying to recover a wreck
	if (caps.recoverOk && selected->objClass == OC_SPACESHIP)
	{
		if (highlighted->objClass == OC_SCRIPTOBJECT && hTarget->caps.recoverOk)
		{
			mode = CM_RECOVER;
			return true;
		}
		else if (highlighted->objClass == OC_PLATFORM && hTarget->caps.recoverOk)
		{
			mode = CM_DROP_OFF;
			return true;
		}
		else if (hTarget->bDerelict)
		{
			mode = CM_RECOVER;
			return true;
		}
	}

	if (caps.moveOk)
	{
		if (hTarget.pInit->mObjClass == M_NUGGET || hTarget.pInit->mObjClass == M_PLANET)
		{
			mode = CM_MOVE;
			return true;
		}
	}

	return false;
}
//--------------------------------------------------------------------------//
//
bool ObjectComm::selectedIgnoreJump(IBaseObject * selected)
{
	IBaseObject * search = selected;
	while(search)
	{
		if(!(search->effectFlags.canIgnoreInhibitors()))
		{
			return false;
		}
		search = search->nextSelected;
	}
	return true;
}
//--------------------------------------------------------------------------//
//
bool ObjectComm::determineWormholeGeneratorSelected (IBaseObject * selected, CURSOR_MODE & mode)
{
	// we are guarnteed to have a platform selected, if it is a wormhole generator
	// than set the cursor state accordingly
	MPart part = selected;
	if (part.isValid() && part->mObjClass == M_PORTAL)
	{
		mode = CM_WORMHOLEGENERATOR;
		return true;
	}

	return false;
}
//--------------------------------------------------------------------------//
//
CURSOR_MODE ObjectComm::determineCursorMode (IBaseObject* selected, IBaseObject* highlighted, const Vector & pos)
{
	CURSOR_MODE mode = CM_NOT_OWNED;
	MPart hTarget = highlighted;
	MISSION_DATA::M_CAPS caps = getCaps(selected);

	const bool bRightClick = DEFAULTS->GetDefaults()->bRightClickOption;
	
	//as opposed to "globalLocalState"
	const U32 localGlobalState = determineLocalGlobalCursorState(selected, caps, bRightClick);
	
	// if we are highlighting more than one thing (ie. lasso mode) than quit out
	if (!bHasFocus || highlighted && highlighted->nextHighlighted)
	{
		return CM_NOT_OWNED;
	}

	// if we have a worm-hole generator selected, than we have a special state
	if (selected && selected->nextSelected == NULL && selected->objClass == OC_PLATFORM)
	{
		if (determineWormholeGeneratorSelected(selected, mode) == true)
		{
			goto Done;
		}
	}

	// nothing highlighted or a nugget is highlighted
	if (hTarget.isValid() == false)	
	{
		if (determineCursorModeNoHilight(selected, highlighted, caps, localGlobalState, mode, pos) == true)
		{
			goto Done;
		}
	}
	else if (selected==0 || highlighted->bSelected)
	{
		// nothing is selected OR you are highlighting a selected object
		if (determineCursorModeHighlightSelected(selected, highlighted, caps, localGlobalState, mode) == true)
		{
			goto Done;
		}
	}
	else if (highlighted->bSelected==0)	
	{
		// not highlighting a selected object
		if (determineCursorModeNotHighlightingSelected(selected, highlighted, caps, localGlobalState, mode, pos) == true)
		{
			goto Done;
		}
	}

Done:
	return mode;
}
//-------------------------------------------------------------------
//
void ObjectComm::onSelectionEvent (void)
{
	IBaseObject * selected = OBJLIST->GetSelectedList();

	if (selected->objClass == OC_SPACESHIP)		// only spaceships have speech for this
	{
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		IBaseObject * speaker = findBestSpeaker(selected, matchCaps);

		if (speaker)
		{
			MPart part = speaker;
			SHIPSELECT(speaker, speaker->GetPartID(), selected, SUB_SELECT, part.pInit->displayName);
		}
	}
	else
	if (selected->objClass == OC_PLATFORM)
	{
		MPart part = selected;
		PLATSELECT(selected, selected->GetPartID(), selected, SUB_SELECT, part.pInit->displayName);
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::updateDisplayQueue (void)
{
	CQASSERT(pQueueList);
	AgentEnumerator enumerator(*pQueueList, g_trueAnimPos, SECTOR->GetCurrentSystem());
	int i;
	ObjSet set;
	IBaseObject * selected = OBJLIST->GetSelectedList();

	while (selected)
	{
		set.objectIDs[set.numObjects++] = selected->GetPartID();
		selected = selected->nextSelected;
	}

	CQASSERT(set.numObjects <= MAX_SELECTED_UNITS);

	for (i = 0; i < MAX_QUEUE_SIZE; i++)
		pQueueList->q[i].bValid = false;

	//	OBJPTR<IBaseObject> pAnim;
	//	U32 opID;
	//	NETGRIDVECTOR targetPosition;

	THEMATRIX->EnumerateQueuedMoveAgents(set, &enumerator);

	for (i = 0; i < MAX_QUEUE_SIZE; i++)
	{
		if (pQueueList->q[i].bValid == false)
		{
			delete pQueueList->q[i].pAnim.Ptr();
			pQueueList->q[i].pAnim = 0;
		}
	}

}
//-------------------------------------------------------------------
//
void ObjectComm::onMoveEvent (const NETGRIDVECTOR & netvec, bool bQueued, bool bAnim, bool bSlow)
{
	IBaseObject* selected = OBJLIST->GetSelectedList();

	if (selected)
	{
		//if we are an admiral in formation then this commands needs to be given to the admiral to process.
		if(onFormationMove(selected,netvec, bQueued, bAnim))
		{
			return;
		}

		USR_PACKET<USRMOVE> packet;

		packet.position = netvec;
		packet.userBits = bQueued;
		packet.ctrlBits = bSlow;
		IBaseObject * speaker = initializeUserMovePacket(&packet, selected);

		if (speaker)
		{
			localSendPacket(&packet);

			if (bAnim && netvec.systemID == SECTOR->GetCurrentSystem())
				ARCHLIST->CreateUIAnim(UI_MOVE,netvec);

			if (speaker)
			{
				if(THEMATRIX->IsNotInterruptable(speaker) && (!bQueued))
				{
					MPart part = speaker;
					SHIPNOHILITECOMM(speaker, speaker->GetPartID(), destructionDenied, SUB_DENIED, part.pInit->displayName);
				}
				else
				{
					MPart part = speaker;
					if (part.isValid() && part->admiralID!=0)
						speaker = OBJLIST->FindObject(part->admiralID);
					if (speaker)
						SHIPNOHILITECOMM(speaker, speaker->GetPartID(), move, SUB_MOVE, part.pInit->displayName);
				}
			}

			if (pQueueList==0)
				pQueueList = new ANIMQUEUELIST;

			g_trueAnimPos.gridVec.zero();
		}
	}
}
//--------------------------------------------------------------------------//
//
bool ObjectComm::onFormationMove(IBaseObject * selected,const NETGRIDVECTOR &vec, bool bQueued, bool bAnim)
{
	IBaseObject * search = selected;
	while(search)
	{
		MPart part = search;
		if(part)
		{
			VOLPTR(IAdmiral) admiral;
			if(MGlobals::IsFlagship(part->mObjClass))
			{
				admiral = search;
			}
			else if( part->admiralID)
			{
				admiral = OBJLIST->FindObject(part->admiralID);
			}
			if(admiral)
			{
				if(admiral->IsInLockedFormation())
				{
					if (bAnim && vec.systemID == SECTOR->GetCurrentSystem())
						ARCHLIST->CreateUIAnim(UI_MOVE,vec);

					USR_PACKET<USRFORMATIONMOVE> packet;

					packet.position = vec;
					packet.userBits = bQueued;
					packet.objectID[0] = admiral.Ptr()->GetPartID();
					packet.init(1, false);

					localSendPacket(&packet);

					return true;
				}
				else
				{
					return false;
				}
			}
		}
		search = search->nextSelected;
	}
	return false;
}
//--------------------------------------------------------------------------//
//
bool ObjectComm::onFormationJump(IBaseObject * gate,IBaseObject * selected)
{
	IBaseObject * search = selected;
	while(search)
	{
		MPart part = search;
		if(part)
		{
			VOLPTR(IAdmiral) admiral;
			if(MGlobals::IsFlagship(part->mObjClass))
			{
				admiral = search;
			}
			else if( part->admiralID)
			{
				admiral = OBJLIST->FindObject(part->admiralID);
			}
			if(admiral)
			{
				if(admiral->IsInLockedFormation())
				{
					USR_PACKET<USRFORMATIONMOVE> packet;

					gate = SECTOR->GetJumpgateDestination(gate);

					RECT rect;
					SECTOR->GetSystemRect(gate->GetSystemID(),&rect);
					Vector centerSys((rect.right-rect.left)/2,(rect.top-rect.bottom)/2,0);
					Vector dir = centerSys-gate->GetPosition();
					dir.fast_normalize();
					NETGRIDVECTOR vec;
					vec.init(gate->GetPosition()+(dir*GRIDSIZE*3),gate->GetSystemID());

					packet.position = vec;
					packet.objectID[0] = admiral.Ptr()->GetPartID();
					packet.init(1, false);

					localSendPacket(&packet);

					return true;
				}
				else
				{
					return false;
				}
			}
		}
		search = search->nextSelected;
	}
	return false;
}
//--------------------------------------------------------------------------//
//
void ObjectComm::handleGotoPos (S32 x, S32 y, WPARAM wParam)
{
	//
	// convert x, y to world coordinates
	//
	Vector vec;

	if (ScreenToPoint(x, y, vec))
	{
		NETGRIDVECTOR netvec;

		// if the vector is outside of the map, than move it back in
		RECT rect;
		SECTOR->GetCurrentRect(&rect);
		SINGLE radius = (rect.right - rect.left)/2.0f;
		Vector cntr(radius, radius, 0);
		Vector dir = vec - cntr;
		SINGLE mag = dir.magnitude();
		if (mag > radius)
		{
			// take dir, normalize it and multiply by the magnitude
			dir.normalize();
			dir.scale(radius);
			vec = cntr + dir;
		}

		netvec.init(vec,SECTOR->GetCurrentSystem());

		onMoveEvent(netvec, ((wParam & MK_SHIFT) != 0), false, ((wParam & MK_CONTROL) != 0));

		g_trueAnimPos.gridVec = netvec;
		g_trueAnimPos.pos = vec;

		ARCHLIST->CreateUIAnim(UI_MOVE,vec);
	}
}
//-------------------------------------------------------------------
//
IBaseObject * ObjectComm::findBestSpeaker (IBaseObject * selected, const MISSION_DATA::M_CAPS & matchCaps)
{
	IBaseObject * best = 0;
	S32 bestPriority = 0;
	MPart part;
	const U32 match = ((const U32 *) &matchCaps)[0];
	CQASSERT(sizeof(MISSION_DATA::M_CAPS) == sizeof(U32));

	while (selected)
	{
		part = selected;

		if (part.isValid())
		{
			if (match==0 || (((U32 *)&part->caps)[0] & match) != 0)
			{
				if (best==0 || part.pInit->speechPriority > bestPriority)
				{
					best = selected;
					bestPriority = part.pInit->speechPriority;
				}
			}

			if (part->admiralID)
			{
				if ((part = OBJLIST->FindObject(part->admiralID)).isValid())
				{
					if (match==0 || (((U32 *)&part->caps)[0] & match) != 0)
					{
						if (best==0 || part.pInit->speechPriority > bestPriority)
						{
							best = part.obj;
							bestPriority = part.pInit->speechPriority;
						}
					}
				}
			}
		}

		selected = selected->nextSelected;
	}

	return best;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleCapture (IBaseObject * target, WPARAM wParam)
{
	IBaseObject * selected = OBJLIST->GetSelectedList();

	if (selected && target)
	{
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.captureOk = true;
		MPart troopship = findBestSpeaker(selected, matchCaps);
		USR_PACKET<USRCAPTURE> packet;

		if (initializeUserPacket(&packet, selected, matchCaps))
		{
			packet.targetID = target->GetPartID();
			packet.userBits = ((wParam & MK_SHIFT) != 0);
			packet.ctrlBits = ((wParam & MK_CONTROL) != 0);
			localSendPacket(&packet);

			ARCHLIST->CreateUIAnim(UI_ATTACK,target->GetPosition());
			if (troopship.isValid())
			{
				if (MGlobals::IsTroopship(troopship->mObjClass))
				{
					TROOPSHIPCOMM2(troopship.obj, troopship->dwMissionID, attacking,SUB_ATTACK,troopship.pInit->displayName);
				}
				else if (MGlobals::IsGunboat(troopship->mObjClass))
				{
					GUNBOATCOMM2(troopship.obj, troopship->dwMissionID, attacking,SUB_ATTACK,troopship.pInit->displayName);
				}
			}
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleSpecialAbility (IBaseObject * selected, WPARAM wParam, U32 hotkey)
{
	USR_PACKET<USRSPABILITY> packet;
	MISSION_DATA::M_CAPS matchCaps;
	memset(&matchCaps, 0, sizeof(matchCaps));
	matchCaps.specialAbilityOk = true;
	matchCaps.mimicOk = true;
	matchCaps.synthesisOk = true;

	if (initializeUserPacket(&packet, selected, matchCaps))
	{
		packet.specialID = 0;
		if(hotkey == IDH_SPECIAL_ABILITY1)
			packet.specialID = 1;
		else if(hotkey == IDH_SPECIAL_ABILITY2)
			packet.specialID = 2;

		packet.userBits = 1;			// stance now((wParam & MK_SHIFT) != 0);

		NETPACKET->Send(HOSTID, 0, &packet);	// allow duplicate packets here

		// do we have enough supplies for this command?
		// if the object is a seeker ship than we have to check if it is able to ping
		bool bNotEnoughSupplies = false;

		MPart ship = selected;

		if (MGlobals::IsSeekerShip(ship->mObjClass))
		{
			// a seeker can ping as long as ping is researched and it has at least one supply point
			if (ship->supplies > 0 && selected->effectFlags.canShoot())
			{
				bNotEnoughSupplies = false;
			}
			else
			{
				bNotEnoughSupplies = true;
			}
		}
		else
		{
			VOLPTR(IAttack) attack = selected;
			if (attack)
			{
				UNIT_SPECIAL_ABILITY usa;
				bool bEnabled;

				attack->GetSpecialAbility(usa, bEnabled);
				
				// if we are enabled to do the special weapon, then talk about it
				bNotEnoughSupplies = !bEnabled;
			}
		}

		if (bNotEnoughSupplies)
		{
			GUNBOATNOHILITECOMM(selected, selected->GetPartID(), suppliesout, SUB_NOSUPPLIES,ship.pInit->displayName);
		}
		else
		{
			playSound(SFXMANAGER->GetGlobalSounds().moveConfirm);
		}

	}
	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleAttack (bool bSpecial, S32 x, S32 y, WPARAM wParam)
{
	IBaseObject* selected = OBJLIST->GetSelectedList();
	IBaseObject * target = OBJLIST->GetHighlightedList();

	if (selected == NULL)
	{
		return;
	}

	if(getCaps(selected).specialAttackWormOk && bSpecial && target && target->objClass == OC_JUMPGATE)
	{
		handleAttackWorm(x, y, wParam);
		return;
	}

	// are we doing a special area of attack?
	if (getCaps(selected).specialEOAOk && bSpecial)
	{
		handleAttackPosition(x, y, wParam,true);
		return;
	}

	if(getCaps(selected).specialTargetPlanetOk && bSpecial && target && target->objClass == OC_PLATFORM)
	{
		VOLPTR(IPlatform) plat = target;
		if(!plat)
			return;
		target = OBJLIST->FindObject(plat->GetPlanetID());
		if(!target)
			return;
	}

	if((!bSpecial) && handleFormationAttack(x, y))
	{
		return;
	}

//this is no longer true, plents are not weapon targets but can be targeted ??? -tom
//	VOLPTR(IWeaponTarget) wpnTarget = target;		// make sure the target can be targeted
//	if (wpnTarget==0)
//		return;
	Vector vec = target->GetPosition();
	MISSION_DATA::M_CAPS matchCaps;
	memset(&matchCaps, 0, sizeof(matchCaps));
	matchCaps.attackOk = true;
	matchCaps.admiralOk = true;
	matchCaps.synthesisOk = true;
	MPart gunboat = findBestSpeaker(selected, matchCaps);

	if (bSpecial == false)
	{
		USR_PACKET<USRATTACK> packet;

		if (initializeUserPacket(&packet, selected, matchCaps))
		{
			packet.targetID = target->GetPartID();
			packet.destSystemID = target->GetSystemID();
			packet.bUserGenerated = true;
			packet.userBits = ((wParam & MK_SHIFT) != 0);
			packet.ctrlBits = ((wParam & MK_CONTROL) != 0);
			localSendPacket(&packet);

			ARCHLIST->CreateUIAnim(UI_ATTACK,vec);
			if (gunboat.isValid())
			{
				if (MGlobals::IsFlagship(gunboat->mObjClass))
				{
					FLAGSHIPCOMM2(gunboat.obj, gunboat->dwMissionID, attacking,SUB_ATTACK,gunboat.pInit->displayName);
				}
				else if (MGlobals::IsGunboat(gunboat->mObjClass))
				{
					if (gunboat->supplies)
						GUNBOATNOHILITECOMM(gunboat.obj, gunboat->dwMissionID, attacking,SUB_ATTACK,gunboat.pInit->displayName);
					else
						GUNBOATNOHILITECOMM(gunboat.obj, gunboat->dwMissionID, suppliesout,SUB_NOSUPPLIES,gunboat.pInit->displayName);
				}
			}
		}
	}
	else
	{
		USR_PACKET<USRSPATTACK> packet;

		if (initializeUserPacket(&packet, selected, matchCaps))
		{
			packet.targetID = target->GetPartID();
			packet.destSystemID = target->GetSystemID();
			packet.userBits = ((wParam & MK_SHIFT) != 0);
			packet.ctrlBits = ((wParam & MK_CONTROL) != 0);
			localSendPacket(&packet);

			ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK,vec);
			if (gunboat.isValid())
			{
				if (MGlobals::IsFlagship(gunboat->mObjClass))
				{
					FLAGSHIPCOMM2(gunboat.obj, gunboat->dwMissionID, attacking,SUB_ATTACK,gunboat.pInit->displayName);
				}
				else if (MGlobals::IsGunboat(gunboat->mObjClass))
				{
					// do we have enough supplies to do our special weapon attack?
					VOLPTR(IAttack) attack = gunboat.obj;
					if (attack)
					{
						UNIT_SPECIAL_ABILITY usa;
						bool bEnabled;

						attack->GetSpecialAbility(usa, bEnabled);
						
						// if we are enabled to do the special weapon, then talk about it
						if (bEnabled)
						{
							GUNBOATCOMM2(gunboat.obj, gunboat->dwMissionID, specialAttack,SUB_SPECIAL_ATTACK,gunboat.pInit->displayName);
						}
						else
						{
							GUNBOATNOHILITECOMM(gunboat.obj, gunboat->dwMissionID, suppliesout,SUB_NOSUPPLIES,gunboat.pInit->displayName);
						}
					}
				}
			}
		}
	}

	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleArtifactUse(S32 x, S32 y, WPARAM wParam)
{
	IBaseObject* selected = OBJLIST->GetSelectedList();
	IBaseObject * target = OBJLIST->GetHighlightedList();

	globalState = GS_EMPTY_STATE;
	
	VOLPTR(IArtifactHolder) holder = selected;
	if(holder && holder->HasArtifact())
	{
		VOLPTR(IArtifact) artifact = holder->GetArtifact();
		if(artifact)
		{
			if(artifact->IsTargetPlanet())
			{
				if(target->objClass == OC_PLATFORM)
				{
					VOLPTR(IPlatform) plat = target;
					if(!plat)
						return;
					target = OBJLIST->FindObject(plat->GetPlanetID());
					if(!target)
						return;
				}
				if(target->objClass == OC_PLANETOID)
				{
					VOLPTR(IPlanet) planet = target;
					if(!(planet->IsMoon()))
					{
						USR_PACKET<USEARTIFACTTARGETED> packet;

						packet.targetID = target->GetPartID();
						packet.objectID[0] = selected->GetPartID();
						packet.init(1, true);

						localSendPacket(&packet);
					}
				}
			}
		}
	}
}
//-------------------------------------------------------------------
//
bool ObjectComm::handleFormationAttack(S32 x, S32 y)
{
	IBaseObject * target = OBJLIST->GetHighlightedList();
	if(!target)
		return false;
	IBaseObject * search = OBJLIST->GetSelectedList();
	while(search)
	{
		MPart part = search;
		if(part)
		{
			VOLPTR(IAdmiral) admiral;
			if(MGlobals::IsFlagship(part->mObjClass))
			{
				admiral = search;
			}
			else if( part->admiralID)
			{
				admiral = OBJLIST->FindObject(part->admiralID);
			}
			if(admiral)
			{
				if(admiral->IsInLockedFormation())
				{
					USR_PACKET<FORMATIONATTACK> packet;

					packet.targetID = target->GetPartID();
					packet.destSystemID = target->GetSystemID();
					packet.objectID[0] = admiral.Ptr()->GetPartID();
					packet.init(1, false);

					localSendPacket(&packet);

					return true;
				}
				else
				{
					return false;
				}
			}
		}
		search = search->nextSelected;
	}
	return false;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleAttackPosition (S32 x, S32 y, WPARAM wParam, bool bSpecial)
{
	IBaseObject * attacker = OBJLIST->GetSelectedList();

	if (attacker == NULL)
	{
		return;
	}

	Vector vec;
	if (ScreenToPoint(x, y, vec))
	{
		USR_PACKET<USRAOEATTACK> packet;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		if(bSpecial)
			matchCaps.specialEOAOk = true;
		else
			matchCaps.targetPositionOk = true;

		packet.position.init(vec,SECTOR->GetCurrentSystem());
		packet.bSpecial = bSpecial;
		if (initializeUserPacket(&packet, attacker, matchCaps))
		{
			packet.userBits = ((wParam & MK_SHIFT) != 0);
			packet.ctrlBits = ((wParam & MK_CONTROL) != 0);

			localSendPacket(&packet);

			ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK,vec);
			playSound(SFXMANAGER->GetGlobalSounds().moveConfirm);
		}

		if(bSpecial)
		{
			// do we have enough supplies to do our special weapon attack?
			MPart gunboat = attacker;
			VOLPTR(IAttack) attack = gunboat.obj;
			if (attack)
			{
				UNIT_SPECIAL_ABILITY usa;
				bool bEnabled;

				attack->GetSpecialAbility(usa, bEnabled);
				
				// if we are enabled to do the special weapon, then talk about it
				if (bEnabled == false)
				{
					GUNBOATNOHILITECOMM(gunboat.obj, gunboat->dwMissionID, suppliesout,SUB_NOSUPPLIES,gunboat.pInit->displayName);
				}
				else
				{
					playSound(SFXMANAGER->GetGlobalSounds().moveConfirm);
				}
			}
		}

	}

	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleAttackWorm (S32 x, S32 y, WPARAM wParam)
{
	IBaseObject * attacker = OBJLIST->GetSelectedList();
	IBaseObject * target = OBJLIST->GetHighlightedList();

	if (attacker == NULL || target == NULL)
	{
		return;
	}

	USR_PACKET<USRWORMATTACK> packet;
	Vector vec = target->GetPosition();
	MISSION_DATA::M_CAPS matchCaps;
	memset(&matchCaps, 0, sizeof(matchCaps));
	matchCaps.specialAttackWormOk = true;

	if (initializeUserPacket(&packet, attacker, matchCaps))
	{
		packet.userBits = ((wParam & MK_SHIFT) != 0);
		packet.ctrlBits = ((wParam & MK_CONTROL) != 0);
		packet.targetID = target->GetPartID();

		localSendPacket(&packet);

		ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK,vec);
		playSound(SFXMANAGER->GetGlobalSounds().moveConfirm);

		// do we have enough supplies to do our special weapon attack?
		MPart gunboat = attacker;
		VOLPTR(IAttack) attack = gunboat.obj;
		if (attack)
		{
			UNIT_SPECIAL_ABILITY usa;
			bool bEnabled;

			attack->GetSpecialAbility(usa, bEnabled);
			
			// if we are enabled to do the special weapon, then talk about it
			if (bEnabled)
			{
				playSound(SFXMANAGER->GetGlobalSounds().moveConfirm);
			}
			else
			{
				GUNBOATNOHILITECOMM(gunboat.obj, gunboat->dwMissionID, suppliesout,SUB_NOSUPPLIES,gunboat.pInit->displayName);
			}
		}
	}

	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleProbe (S32 x, S32 y, WPARAM wParam)
{
	IBaseObject * prober;

	if ((prober = OBJLIST->GetSelectedList()) == NULL)
	{
		return;
	}

	USR_PACKET<USRPROBE> packet;
	Vector vec;
	MISSION_DATA::M_CAPS matchCaps;
	memset(&matchCaps, 0, sizeof(matchCaps));
	matchCaps.probeOk = true;

	if (initializeUserPacket(&packet, prober, matchCaps))
	{
		packet.userBits = ((wParam & MK_SHIFT) != 0);
		packet.ctrlBits = ((wParam & MK_CONTROL) != 0);

		if (ScreenToPoint(x, y, vec))
		{
			packet.position.init(vec,SECTOR->GetCurrentSystem());
			localSendPacket(&packet);

			ARCHLIST->CreateUIAnim(UI_MOVE,vec);
			playSound(SFXMANAGER->GetGlobalSounds().moveConfirm);
		}
	}

	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleProbe (WPARAM wParam)
{
	IBaseObject * prober = OBJLIST->GetSelectedList();
	IBaseObject * gate = OBJLIST->GetHighlightedList();

	if (prober && gate)
	{
		USR_PACKET<USRPROBE> packet;
		Vector vec;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.probeOk = true;
		if (initializeUserPacket(&packet, prober, matchCaps))
		{
			packet.userBits = ((wParam & MK_SHIFT) != 0);
			packet.ctrlBits = ((wParam & MK_CONTROL) != 0);

			RECT rect;
			
			if (gate->objClass == OC_JUMPGATE)		// sanity check, could be wrong if moving the mouse
			{
				//
				// we want to use the destination jumpgate
				//
				gate = SECTOR->GetJumpgateDestination(gate);

				SECTOR->GetSystemRect(packet.position.systemID, &rect);
				vec.x = rect.right/2;
				vec.y = rect.top / 2;
				vec.z = 0;
				vec -= gate->GetPosition();
				vec.normalize();
				vec *= 5000;
				vec += gate->GetPosition();

				packet.position.init(vec,gate->GetSystemID());
				
				localSendPacket(&packet);

				ARCHLIST->CreateUIAnim(UI_JUMP,OBJLIST->GetHighlightedList()->GetTransform().translation);

				IBaseObject * speaker = findBestSpeaker(prober, matchCaps);

				if (speaker)
				{
					MPart part = speaker;
					SHIPCOMM2(speaker, speaker->GetPartID(), move, SUB_MOVE, part.pInit->displayName);
				}
			}
		}
	}
	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleRecover (WPARAM wParam)
{
	IBaseObject * recoverer = OBJLIST->GetSelectedList();
	IBaseObject * target = OBJLIST->GetHighlightedList();
	if(recoverer && target)
	{
		USR_PACKET<USRRECOVER> packet;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.recoverOk = true;

		if (initializeUserPacket(&packet, recoverer, matchCaps))
		{
			packet.userBits = ((wParam & MK_SHIFT) != 0);
			packet.ctrlBits = ((wParam & MK_CONTROL) != 0);

			packet.targetID = target->GetPartID();
			localSendPacket(&packet);

			IBaseObject * speaker = findBestSpeaker(recoverer, matchCaps);

			if (speaker)
			{
				MPart part = speaker;
				SHIPCOMM2(speaker, speaker->GetPartID(), move, SUB_MOVE, part.pInit->displayName);
			}
		}
	}

	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleDropOff (WPARAM wParam)
{
	IBaseObject * recoverer = OBJLIST->GetSelectedList();
	IBaseObject * target = OBJLIST->GetHighlightedList();
	if(recoverer && target)
	{
		USR_PACKET<USRDROPOFF> packet;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.recoverOk = true;

		if (initializeUserPacket(&packet, recoverer, matchCaps))
		{
			packet.userBits = ((wParam & MK_SHIFT) != 0);
			packet.ctrlBits = ((wParam & MK_CONTROL) != 0);

			packet.targetID = target->GetPartID();
			localSendPacket(&packet);

			IBaseObject * speaker = findBestSpeaker(recoverer, matchCaps);

			if (speaker)
			{
				MPart part = speaker;
				SHIPCOMM2(speaker, speaker->GetPartID(), move, SUB_MOVE, part.pInit->displayName);
			}
		}
	}

	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleMimic (WPARAM wParam)
{
	IBaseObject * target = OBJLIST->GetHighlightedList();
	IBaseObject* selected = OBJLIST->GetSelectedList();

	if (selected && target)
	{
		USR_PACKET<USRMIMIC> packet;
		Vector vec;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.mimicOk = true;
		if (initializeUserPacket(&packet, selected, matchCaps))
		{
			packet.targetID = target->GetPartID();
			packet.userBits = 1;		// stance-like behavior ((wParam & MK_SHIFT) != 0);
			NETPACKET->Send(HOSTID, 0, &packet);		// allow duplicate packets
		}
	}
	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
/*void ObjectComm::handleSynthesis (WPARAM wParam)
{
	IBaseObject * target = OBJLIST->GetHighlightedList();
	IBaseObject* selected = OBJLIST->GetSelectedList();

	if (selected && target)
	{
		USR_PACKET<USRMIMIC> packet;
		Vector vec;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.Ok = true;
		if (initializeUserPacket(&packet, selected, matchCaps))
		{
			packet.targetID = target->GetPartID();
			packet.userBits = ((wParam & MK_SHIFT) != 0);
		
			localSendPacket(&packet);
		}
	}
	globalState = EMPTY_STATE;
}*/
//-------------------------------------------------------------------
//
void ObjectComm::handleStop (void)
{
	IBaseObject* selected = OBJLIST->GetSelectedList();

	if (selected)
	{
		USR_PACKET<USRSTOP> packet;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));

		if (initializeUserPacket(&packet, selected, matchCaps))
			localSendPacket(&packet);
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::onRallyEvent (const NETGRIDVECTOR & netvec, bool bAnim)
{
	IBaseObject* selected = OBJLIST->GetSelectedList();

	if (selected && globalState == GS_RALLY_STATE)
	{
		USR_PACKET<USRRALLY> packet;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));

		if (initializeUserPacket(&packet, selected, matchCaps))
		{
			packet.position = netvec;

			localSendPacket(&packet);

			if (bAnim && netvec.systemID == SECTOR->GetCurrentSystem())
				ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK,(Vector)(netvec));
			playSound(SFXMANAGER->GetGlobalSounds().moveConfirm);

			Vector vec(netvec.getX()*GRIDSIZE, netvec.getY()*GRIDSIZE, 0);
			if (netvec.systemID == SECTOR->GetCurrentSystem())
			{
				ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, vec);

				g_trueAnimPos.gridVec = netvec;
				g_trueAnimPos.pos = vec;
			}
			else
			{
				g_trueAnimPos.gridVec.zero();
			}
		}
	}

	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleSetRally (S32 x, S32 y)
{
	IBaseObject * platform = OBJLIST->GetSelectedList();
	if (platform == NULL)
	{
		return;
	}

	Vector vec;

	if (ScreenToPoint(x, y, vec))
	{
		NETGRIDVECTOR netvec;
		netvec.init(vec,SECTOR->GetCurrentSystem());
		onRallyEvent(netvec, false);
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::DEBUG_handleExplode (void)
{
	if (HOSTID == PLAYERID)
	{
		IBaseObject * obj = OBJLIST->GetSelectedList();
		MPart part;

		while (obj)
		{
			part = obj;
			if (part && obj->objMapNode!=0)
			{
				if (part->admiralID)
					THEMATRIX->ObjectTerminated(part->admiralID, 0);
				THEMATRIX->ObjectTerminated(part->dwMissionID, 0);
			}
			obj = obj->nextSelected;
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::DEBUG_handleDelete (void)
{
	if (HOSTID == PLAYERID)
	{
		IBaseObject * obj = OBJLIST->GetSelectedList();
		MPart part;

		while (obj)
		{
			if(obj->objClass == OC_JUMPGATE)
			{
				IBaseObject * delObj = obj;
				IBaseObject * otherEnd = SECTOR->GetJumpgateDestination(obj);
				obj = obj->nextSelected;
				SECTOR->RemoveLink(delObj,otherEnd);
			}
			else
			{
				part = obj;
				if (part)
				{
					if (part->admiralID)
						OBJLIST->DeferredDestruction(part->admiralID);
					OBJLIST->DeferredDestruction(part->dwMissionID);
				}
				obj = obj->nextSelected;
			}
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleCloakUnits (void)
{
	IBaseObject * obj = OBJLIST->GetSelectedList();

	// do not continue unless you've found an object that has the cloak ability
	IBaseObject * nosupplies = NULL;
	IBaseObject * p = obj;
	MPart part;
	bool bCloakFound = false;
	
	while (p)
	{
		part = p;
		if (part->caps.cloakOk == true)
		{
			bCloakFound = true;
			if (part.obj->bCloaked == false && part->supplies == 0)
			{
				nosupplies = part.obj;
				break;
			}
		}
		p = p->nextSelected;
	}

	if (obj && bCloakFound)
	{
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.cloakOk = true;

		USR_PACKET<USRCLOAK> packet;
		packet.userBits = 1;		// stance packet requires queue bit
		if (initializeUserPacket(&packet, obj, matchCaps))
			NETPACKET->Send(HOSTID, 0, &packet);	// allow duplicate packets here

		// did we find a unit that is out of supplies and can't cloak?
		if (nosupplies)
		{
			part = nosupplies;
			GUNBOATNOHILITECOMM(nosupplies, nosupplies->GetPartID(), suppliesout,SUB_NOSUPPLIES,part.pInit->displayName);
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleEjectArtifact (void)
{
	VOLPTR(IArtifactHolder) holder = OBJLIST->GetSelectedList();
	if(holder && holder->HasArtifact())
	{
		USR_PACKET<USR_EJECT_ARTIFACT> packet;
		packet.objectID[0] = holder.Ptr()->GetPartID();
		packet.init(1);
		localSendPacket(&packet);
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleUseArtifact (void)
{
	VOLPTR(IArtifactHolder) holder = OBJLIST->GetSelectedList();
	if(holder && holder->HasArtifact())
	{
		VOLPTR(IArtifact) artifact = holder->GetArtifact();
		if(artifact)
		{
			if(artifact->IsUsable())
			{
				if(artifact->IsToggle())
				{
/*					USR_PACKET<USRTOGGLEARTIFACT> packet;
					packet.userBits = 1;		// stance packet requires queue bit
					packet.objectID[0] = holder.Ptr()->GetPartID();
					packet.init(1);
					NETPACKET->Send(HOSTID, 0, &packet);	// allow duplicate packets here
*/				}
				else
				{
					handleGlobalModeSwitch(IDH_USE_ARTIFACT);
				}
			}
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleDestroyUnits (void)
{
	IBaseObject * obj = OBJLIST->GetSelectedList();

	if (obj && obj->nextSelected==0)
	{
		if (THEMATRIX->IsNotInterruptable(obj))
		{
			if(obj->objClass == OC_PLATFORM)
			{
				MPart part = obj;
				PLATFORMCOMM2(obj, obj->GetPartID(), destructionDenied, SUB_DENIED,part.pInit->displayName);
			}
			else
			{
				MPart part = obj;
				SHIPCOMM2(obj, obj->GetPartID(), destructionDenied, SUB_DENIED,part.pInit->displayName);
			}
		}
		else
		{
			USR_PACKET<USRKILLUNIT> packet;

			packet.objectID[0] = obj->GetPartID();
			packet.init(1, false);
			CQASSERT(packet.objectID[0]);
			localSendPacket(&packet);
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleHarvest (IBaseObject* target, WPARAM wParam)
{
	IBaseObject * harvester = OBJLIST->GetSelectedList();

	if (harvester && target)
	{
		USR_PACKET<USRHARVEST> packet;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.harvestOk = true;

		if (initializeUserPacket(&packet, harvester, matchCaps))
		{
			packet.targetID = target->GetPartID();
			packet.bAutoSelected = false;
			packet.userBits = ((wParam & MK_SHIFT) != 0);
			packet.ctrlBits = ((wParam & MK_CONTROL) != 0);
			localSendPacket(&packet);

			ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, target->GetPosition());

			IBaseObject * speaker = findBestSpeaker(harvester, matchCaps);

			if (speaker)
			{
				if(THEMATRIX->IsNotInterruptable(speaker) && (!packet.userBits))
				{
					MPart part = speaker;
					SHIPCOMM2(speaker, speaker->GetPartID(), destructionDenied,SUB_DENIED,part.pInit->displayName);
				}
				else
				{
					MPart part = speaker;
					SHIPCOMM2(speaker, speaker->GetPartID(), move,SUB_MOVE,part.pInit->displayName);
				}
			}
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleJump (WPARAM wParam)
{
	IBaseObject * gate = OBJLIST->GetHighlightedList();
	IBaseObject* selected = OBJLIST->GetSelectedList();

	if (selected && gate)
	{
		if(gate->objClass == OC_PLATFORM)
		{
			OBJPTR<IJumpPlat> jumpPlat;
			gate->QueryInterface(IJumpPlatID,jumpPlat);
			if(jumpPlat)
				gate = jumpPlat->GetJumpGate();
		}
		
		if (gate->objClass == OC_JUMPGATE)		// sanity check, could be wrong if moving the mouse
		{
			if(onFormationJump(gate,selected))
			{
				return;
			}
			USR_PACKET<USRJUMP> packet;
			MISSION_DATA::M_CAPS matchCaps;
			memset(&matchCaps, 0, sizeof(matchCaps));
			matchCaps.jumpOk = true;

			packet.userBits = ((wParam & MK_SHIFT) != 0);
			packet.ctrlBits = ((wParam & MK_CONTROL) != 0);
			packet.jumpgateID = gate->GetPartID();
			if (initializeUserPacket(&packet, selected, matchCaps))
			{
				localSendPacket(&packet);

				ARCHLIST->CreateUIAnim(UI_JUMP,OBJLIST->GetHighlightedList()->GetTransform().translation);

				IBaseObject * speaker = findBestSpeaker(selected, matchCaps);

				if (speaker)
				{
					MPart part = speaker;
					SHIPCOMM2(speaker, speaker->GetPartID(), move,SUB_MOVE,part.pInit->displayName);
				}

				if (packet.userBits && pQueueList==0)
					pQueueList = new ANIMQUEUELIST;
			}
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleForbidenJump(IBaseObject * jumpgate)
{
	if (OBJLIST->GetSelectedList() && jumpgate && jumpgate->objClass == OC_JUMPGATE)
		MScript::RunProgramsWithEvent(CQPROGFLAG_FORBIDEN_JUMP,jumpgate->GetPartID());
}
//-------------------------------------------------------------------
//
void ObjectComm::handleJumpCamera (IBaseObject* jumpGate)
{
	IBaseObject * exitGate = SECTOR->GetJumpgateDestination(jumpGate);

	if (exitGate && (DEFAULTS->GetDefaults()->bEditorMode || DEFAULTS->GetDefaults()->bSpectatorModeOn ||
		SECTOR->IsVisibleToPlayer(exitGate->GetSystemID(), MGlobals::GetThisPlayer())))
	{
		Vector pos = exitGate->GetPosition();

		SECTOR->SetCurrentSystem(exitGate->GetSystemID());
		CAMERA->SetLookAtPosition(pos);
	}
}
//-------------------------------------------------------------------
// look at selected units
//
void ObjectComm::handleGotoUnits (void)
{
	IBaseObject * obj = OBJLIST->GetSelectedList();

	if (obj)
	{
		const U32 systemID = obj->GetSystemID();
		if (systemID <= MAX_SYSTEMS)
		{
			SECTOR->SetCurrentSystem(systemID);
			CAMERA->SetLookAtPosition(obj->GetPosition());
		}
	}
}
//-------------------------------------------------------------------
//
class FabFinder : public IAgentEnumerator
{
public:
	FabFinder(U32 _missionID)
	{
		costFound = false;
		dwMissionID = _missionID;
	}

	bool costFound;
	ResourceCost cost;
	U32 dwMissionID;

	/* IAgentEnumerator */

	virtual bool EnumerateAgent (const NODE & node)
	{
		if(node.pSet->objectIDs[0] == dwMissionID)
		{
			BASIC_DATA * data = (BASIC_DATA *) (ARCHLIST->GetArchetypeData(node.dwFabArchetype));
			CQASSERT(data->objClass == OC_PLATFORM);
			BASE_PLATFORM_DATA * platData = (BASE_PLATFORM_DATA *) data;
			cost = platData->missionData.resourceCost;
			costFound = true;
			return false;
		}
		return true;
	}
};

bool getCurrentFabricateCost(ResourceCost & cost, U32 dwMissionID)
{
	FabFinder finder(dwMissionID);
	THEMATRIX->EnumerateFabricateAgents(MGlobals::GetThisPlayer(),&finder);
	if(finder.costFound)
	{
		cost = finder.cost;
		return true;
	}
	return false;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleFabricate (IBaseObject* highlighted, WPARAM wParam,S32 x,S32 y)
{
	IBaseObject * selected = OBJLIST->GetSelectedList();

	if(selected)
	{
		BASIC_DATA * data = (BASIC_DATA *) (ARCHLIST->GetArchetypeData(BUILDARCHEID));
		CQASSERT(data->objClass == OC_PLATFORM);
		BASE_PLATFORM_DATA * platData = (BASE_PLATFORM_DATA *) data;
		M_RESOURCE_TYPE type = M_METAL;
		bool canAfford = false;

		ResourceCost cost = platData->missionData.resourceCost;
		ResourceCost currentCost;
		memset(&currentCost, 0, sizeof(currentCost));
		if(getCurrentFabricateCost(currentCost,selected->GetPartID()))
		{
			canAfford = ((wParam & MK_SHIFT) != 0);
			if(!canAfford)
			{
				if(cost.commandPt > currentCost.commandPt)
					cost.commandPt -= currentCost.commandPt;
				else
					cost.commandPt = 0;
				if(cost.crew > currentCost.crew)
					cost.crew -= currentCost.crew;
				else
					cost.crew = 0;
				if(cost.gas > currentCost.gas)
					cost.gas -= currentCost.gas;
				else
					cost.gas = 0;
				if(cost.metal > currentCost.metal)
					cost.metal -= currentCost.metal;
				else
					cost.metal = 0;
			}
		}
		if(!canAfford)
		{
			canAfford = BANKER->HasCost(selected->GetPlayerID(),cost,&type);
		}
		if(canAfford)
		{
			if (selected && highlighted)
			{
				if(highlighted->objClass == OC_JUMPGATE)
				{
					OBJPTR<IJumpGate> jumpObj;

					if (highlighted->QueryInterface(IJumpGateID, jumpObj) != 0)
					{
						USR_PACKET<USRFABJUMP> packet;
						MISSION_DATA::M_CAPS matchCaps;
						memset(&matchCaps, 0, sizeof(matchCaps));
						matchCaps.buildOk = true;

						if (initializeUserPacket(&packet, selected, matchCaps))
						{
							packet.jumpgateID = highlighted->GetPartID();
							packet.userBits = ((wParam & MK_SHIFT) != 0);
							packet.ctrlBits = ((wParam & MK_CONTROL) != 0);
							packet.dwArchetypeID = BUILDARCHEID;
							BUILDARCHEID = 0;

							localSendPacket(&packet);

							IBaseObject * speaker = findBestSpeaker(selected, matchCaps);
							if (speaker)
							{
								if(THEMATRIX->IsNotInterruptable(speaker) && (!packet.userBits))
								{
									MPart part = speaker;
									SHIPCOMM2(speaker, speaker->GetPartID(), destructionDenied,SUB_DENIED,part.pInit->displayName);
								}
								else
								{
									MPart part = speaker;
									SHIPCOMM2(speaker, speaker->GetPartID(), move,SUB_MOVE,part.pInit->displayName);
								}
							}
						}
					}
				}
				else
				{
					OBJPTR<IPlanet> planetObj;

					if (highlighted->QueryInterface(IPlanetID, planetObj) != 0)
					{
						USR_PACKET<USRFAB> packet;
						MISSION_DATA::M_CAPS matchCaps;
						memset(&matchCaps, 0, sizeof(matchCaps));
						matchCaps.buildOk = true;

						if (initializeUserPacket(&packet, selected, matchCaps))
						{
							packet.planetID = highlighted->GetPartID();
							packet.slotID = planetObj->GetHighlightedSlot();
							packet.userBits = ((wParam & MK_SHIFT) != 0);
							packet.ctrlBits = ((wParam & MK_CONTROL) != 0);
							packet.dwArchetypeID = BUILDARCHEID;
							BUILDARCHEID = 0;

							localSendPacket(&packet);
							if (packet.slotID)
								planetObj->ClickSlot(packet.slotID);

							IBaseObject * speaker = findBestSpeaker(selected, matchCaps);
							if (speaker)
							{
								if(THEMATRIX->IsNotInterruptable(speaker) && (!packet.userBits))
								{
									MPart part = speaker;
									SHIPCOMM2(speaker, speaker->GetPartID(), destructionDenied,SUB_DENIED,part.pInit->displayName);
								}
								else
								{
									MPart part = speaker;
									SHIPCOMM2(speaker, speaker->GetPartID(), move,SUB_MOVE,part.pInit->displayName);
								}
							}
						}
					}
				}
			}
			else
			{
				if(data->objClass == OC_PLATFORM)
				{
					if(platData->slotsNeeded == 0)
					{
						Vector positon;
						ScreenToPoint(x, y, positon);

						USR_PACKET<USRFABPOS> packet;
						MISSION_DATA::M_CAPS matchCaps;
						memset(&matchCaps, 0, sizeof(matchCaps));
						matchCaps.buildOk = true;

						if (initializeUserPacket(&packet, selected, matchCaps))
						{
							packet.position.init(positon,SECTOR->GetCurrentSystem());
							if(UNBORNMANAGER->IsFullGrid(BUILDARCHEID))
								packet.position.centerpos();
							else
								packet.position.quarterpos();
							packet.userBits = ((wParam & MK_SHIFT) != 0);
							packet.ctrlBits = ((wParam & MK_CONTROL) != 0);
							packet.dwArchetypeID = BUILDARCHEID;
							if (packet.userBits == 0)		// not queuing this build
								BUILDARCHEID = 0;

							localSendPacket(&packet);

							IBaseObject * speaker = findBestSpeaker(selected, matchCaps);
							if (speaker)
							{
								if(THEMATRIX->IsNotInterruptable(speaker) && (!packet.userBits))
								{
									MPart part = speaker;
									SHIPCOMM2(speaker, speaker->GetPartID(), destructionDenied,SUB_DENIED,part.pInit->displayName);
								}
								else
								{
									MPart part = speaker;
									SHIPCOMM2(speaker, speaker->GetPartID(), move,SUB_MOVE,part.pInit->displayName);
								}
							}
						}
					}
				}
			}
		}
		else
		{
			OBJPTR<IFabricator> fab;
			selected->QueryInterface(IFabricatorID,fab);
			fab->FailSound(type);
		}
	}
	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleDefend (S32 x, S32 y)
{
	IBaseObject * obj;

	if ((obj = OBJLIST->GetSelectedList()) == NULL)
	{
		return;
	}

	IBaseObject * target = OBJLIST->GetHighlightedList();
	Vector vec;

	ScreenToPoint(x, y, vec);

	if (obj)
	{
		USR_PACKET<USRESCORT> packet;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.defendOk = true;
		matchCaps.admiralOk = true;

		if (initializeUserPacket(&packet, obj, matchCaps))
		{
			packet.targetID = (target)? target->GetPartID() : 0;		// allow defend of neutral objects
			localSendPacket(&packet);

			ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, (target)?target->GetPosition():vec);
			
			IBaseObject * speaker = findBestSpeaker(obj, matchCaps);

			if (speaker)
			{
				MPart part = speaker;
				SHIPCOMM2(speaker, speaker->GetPartID(), move,SUB_MOVE,part.pInit->displayName);
			}
		}
	}

	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleDockFlagship (IBaseObject * target)
{
	IBaseObject * selected = OBJLIST->GetSelectedList();

	if (selected && target)
	{
		// if the flagship is already attached to another ship, then we have to send an undock packet first
		// the selected object should be the flagship
		VOLPTR(IAdmiral) admiral = selected;
		if (admiral)
		{
			const bool bAttached = admiral->IsDocked() != 0;
			if (bAttached)
			{
				USR_PACKET<USRUNDOCKFLAGSHIP> packet;
				packet.objectID[0] = selected->GetPartID();
				packet.userBits = 0;	// clear out the queue - important
				packet.position.init(admiral.Ptr()->GetGridPosition(), admiral.Ptr()->GetSystemID());
				packet.init(1, false);		// don't track this command
				NETPACKET->Send(HOSTID, 0, &packet);
			}

			USR_PACKET<USRDOCKFLAGSHIP> packet;

			MPart part = selected;
			IBaseObject * obj = OBJLIST->FindObject(part->admiralID);
			if (obj)
			{
				selected = obj;
			}

			initializeUserPacket2(&packet, selected, target);
			packet.userBits = bAttached; // very important! (queue only if we just sent an UNDOCK command)
			localSendPacket(&packet);

			ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, target->GetPosition());
			
			/*
			MISSION_DATA::M_CAPS matchCaps;
			memset(&matchCaps, 0, sizeof(matchCaps));
			matchCaps.admiralOk = true;
			IBaseObject * speaker = findBestSpeaker(selected, matchCaps);
			*/

			if (selected==obj)		// admiral is leaving one ship, joining another
				FLAGSHIPCOMM2(selected, selected->GetPartID(), shipleaving,SUB_ADMIRAL_OFF_DECK,part.pInit->displayName);
			else
				SHIPCOMM2(selected, selected->GetPartID(), move,SUB_MOVE,part.pInit->displayName);
		}
	}

	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleUnDockFlagship (S32 x, S32 y)
{
	IBaseObject * selected = OBJLIST->GetSelectedList();

	if (selected == NULL)
	{
		return;
	}

	MPart part = selected;
	
	if (part.isValid() == false)
	{
		return;
	}
	
	IBaseObject * obj = OBJLIST->FindObject(part->admiralID);

	if (obj)
	{
		USR_PACKET<USRUNDOCKFLAGSHIP> packet;
		Vector vec;

		ScreenToPoint(x, y, vec);
		initializeUserPacket2(&packet, obj, selected);
		packet.position.init(vec,SECTOR->GetCurrentSystem());
		localSendPacket(&packet);

		ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, vec);

		MPart flagObj = obj;
		
		FLAGSHIPCOMM2(obj, obj->GetPartID(), shipleaving,SUB_ADMIRAL_OFF_DECK,flagObj.pInit->displayName);
	}

	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleResupply (IBaseObject * target)
{
	IBaseObject * selected = OBJLIST->GetSelectedList(); 

	if (selected && target)
	{
		USR_PACKET<USRRESUPPLY> packet;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.supplyOk = true;

		if (initializeUserPacket(&packet, selected, matchCaps))
		{
			packet.targetID = target->GetPartID();
			localSendPacket(&packet);

			ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, target->GetPosition());

			IBaseObject * speaker = findBestSpeaker(selected, matchCaps);

			if (speaker && speaker->objClass==OC_SPACESHIP)
			{
				MPart part = speaker;
				SUPPLYSHIPCOMM2(speaker, speaker->GetPartID(), resupplyShips, SUB_RESUPPLY, part.pInit->displayName);
			}
		}
	}

	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleFabRepair (IBaseObject * target, WPARAM wParam)
{
	IBaseObject * selected = OBJLIST->GetSelectedList(); 

	if (selected && target)
	{
		USR_PACKET<USRFABREPAIR> packet;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.repairOk = true;

		if (initializeUserPacket(&packet, selected, matchCaps))
		{
			packet.targetID = target->GetPartID();
			packet.userBits = ((wParam & MK_SHIFT) != 0);
			packet.ctrlBits = ((wParam & MK_CONTROL) != 0);
			localSendPacket(&packet);

			ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, target->GetPosition());

			IBaseObject * speaker = findBestSpeaker(selected, matchCaps);

			if (speaker)
			{
				MPart part = speaker;
				SHIPCOMM2(speaker, speaker->GetPartID(), move,SUB_MOVE,part.pInit->displayName);
			}
		}
	}

	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleFabSalvage (IBaseObject * selected, IBaseObject * target, WPARAM wParam)
{
	if (selected && target)
	{
		USR_PACKET<USRFABSALVAGE> packet;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.salvageOk = true;

		if (initializeUserPacket(&packet, selected, matchCaps))
		{
			packet.targetID = target->GetPartID();
			packet.userBits = ((wParam & MK_SHIFT) != 0);
			packet.ctrlBits = ((wParam & MK_CONTROL) != 0);
			localSendPacket(&packet);

			ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, target->GetPosition());

			IBaseObject * speaker = findBestSpeaker(selected, matchCaps);

			if (speaker)
			{
				MPart part = speaker;
				SHIPCOMM2(speaker, speaker->GetPartID(), move,SUB_MOVE,part.pInit->displayName);
			}
		}
	}

	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleShipRepair (IBaseObject * selected, IBaseObject * target, WPARAM wParam)
{
	if (selected && target)
	{
		USR_PACKET<USRSHIPREPAIR> packet;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.moveOk = true;

		if (initializeUserPacket(&packet, selected, matchCaps))
		{
			packet.targetID = target->GetPartID();
			packet.userBits = ((wParam & MK_SHIFT) != 0);
			packet.ctrlBits = ((wParam & MK_CONTROL) != 0);
			localSendPacket(&packet);

			ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, target->GetPosition());

			IBaseObject * speaker = findBestSpeaker(selected, matchCaps);

			if (speaker)
			{
				MPart part = speaker;
				SHIPCOMM2(speaker, speaker->GetPartID(), move,SUB_MOVE,part.pInit->displayName);
			}
		}
	}

	globalState = GS_EMPTY_STATE;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleShipSell (IBaseObject * selected, IBaseObject * target, WPARAM wParam)
{
	if (selected && target)
	{
		USR_PACKET<USRSHIPREPAIR> packet;
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.moveOk = true;

		if (initializeUserPacket(&packet, selected, matchCaps))
		{
			packet.targetID = target->GetPartID();
			packet.userBits = ((wParam & MK_SHIFT) != 0);
			packet.ctrlBits = ((wParam & MK_CONTROL) != 0);
			localSendPacket(&packet);

			ARCHLIST->CreateUIAnim(UI_SPECIAL_ATTACK, target->GetPosition());

			IBaseObject * speaker = findBestSpeaker(selected, matchCaps);

			if (speaker)
			{
				MPart part = speaker;
				SHIPCOMM2(speaker, speaker->GetPartID(), move,SUB_MOVE,part.pInit->displayName);
			}
		}
	}

	globalState = GS_EMPTY_STATE;
}//-------------------------------------------------------------------
//
void ObjectComm::handleGroupCreate(U32 hotkey)
{
	U32 index = 0;
	switch(hotkey)
	{
	case IDH_GROUP_1_CREATE:	index = 0; 
		break;
	case IDH_GROUP_2_CREATE:	index = 1; 
		break;
	case IDH_GROUP_3_CREATE:	index = 2; 
		break;
	case IDH_GROUP_4_CREATE:	index = 3; 
		break;
	case IDH_GROUP_5_CREATE:	index = 4; 
		break;
	case IDH_GROUP_6_CREATE:	index = 5; 
		break;
	case IDH_GROUP_7_CREATE:	index = 6; 
		break;
	case IDH_GROUP_8_CREATE:	index = 7; 
		break;
	case IDH_GROUP_9_CREATE:	index = 8; 
		break;
	case IDH_GROUP_0_CREATE:	index = 9; 
		break;
	default:
		CQBOMB0("Invalid hotkey for group creation");
	}

	IBaseObject * obj;
	U32 numObjects=0;
	U32 objectIDs[MAX_SELECTED_UNITS];
	MPartNC part;

	// erase old members of the group
	if (group[index].Ptr() != 0)
	{
		ObjSet set;
		set.numObjects = group[index]->GetObjects(set.objectIDs);
		
		while (set.numObjects-- > 0)
		{
			if ((part = OBJLIST->FindObject(set.objectIDs[set.numObjects])).isValid())
			{
				part->controlGroupID = 0;
			}
		}
		delete group[index].Ptr();		// delete the old group
		group[index] = 0;
	}

	obj = OBJLIST->GetSelectedList();
	while (obj)
	{
		if ((part = obj).isValid())
			part->controlGroupID = index+1;
		objectIDs[numObjects++] = obj->GetPartID();
		obj = obj->nextSelected;
	}
	CQASSERT(numObjects <= MAX_SELECTED_UNITS);

	if (numObjects > 0)
	{
		obj = ARCHLIST->CreateInstance("Group!!Default");
		CQASSERT(obj);

		obj->QueryInterface(IGroupID, group[index], NONSYSVOLATILEPTR);
		group[index]->InitGroup(objectIDs, numObjects, 0);
//		OBJLIST->AddGroupObject(obj);

		makeGroupsExclusive(index);
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleFleetSelect(U32 hotkey)
{
	U32 fkey = (hotkey - IDH_FLEET_GROUP_1) + 1;

	CQASSERT(fkey <= 6 && fkey > 0);

	// go through the object list and select the admiral with the matching hotkey
	IBaseObject * obj = OBJLIST->GetTargetList();
	VOLPTR(IAdmiral) admiral;
	MPart part;
	const U32 playerID = MGlobals::GetThisPlayer();

	while (obj)
	{
		// is this object the admiral
		part = obj;
		if (part.isValid() && (part->dwMissionID & PLAYERID_MASK) == playerID)
		{
			if (TESTADMIRAL(part->dwMissionID))
			{
				if ((admiral = obj) != 0)
				{
					if (admiral->GetAdmiralHotkey() == fkey)
					{
						// if the admiral is attached, than get it's dockship
						if (admiral->IsDocked())
						{
							IBaseObject * dockship = admiral->GetDockship();
							if (dockship)
								obj = dockship;
						}

						// we've found the admiral we're looking for
						OBJLIST->FlushHighlightedList();
						OBJLIST->FlushSelectedList();
						OBJLIST->HighlightObject(obj);
						OBJLIST->SelectHighlightedObjects();

						if (selectTimer > 0 && selectTimerGroup==S32(fkey|0x80))
							handleGotoUnits();
						selectTimer = SELECT_GOTO_PERIOD;
						selectTimerGroup = fkey|0x80;
						
						break;		// we've trashed the "obj" pointer, so we better be done
					}
				}
			}
		}

		obj = obj->nextTarget;
	}

	// if we've gotten here, than we couldn't find a matching admiral - perhaps the user was stupid enough to pound on the
	// Fkeys even though there isn't an admiral currently in the game.  My job would be oh-so-much easier if people weren't idiots.
	return;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleFleetDefine(U32 hotkey)
{
	U32 fkey = (hotkey - IDH_FLEET_SET_GROUP_1) + 1;

	CQASSERT(fkey <= 6 && fkey > 0);


	IBaseObject * obj = NULL;
	U32 admiralID;
	VOLPTR(IAdmiral) admiral;
	MPart part;
	bool bSet = false;
	if(findLowestAdmiral(obj,admiralID))
	{
		if ((admiral = obj) != 0)
		{
			admiral->SetAdmiralHotkey(fkey);
			bSet = true;
		}
	}

	if(bSet)
	{
		// go through the object list and unset any admiral with a matching hotkey
		IBaseObject * search = OBJLIST->GetTargetList();
		VOLPTR(IAdmiral) admiral;
		MPart part;
		const U32 playerID = MGlobals::GetThisPlayer();

		while (search)
		{
			if(search != obj)
			{
				// is this object the admiral
				part = search;
				if (part.isValid() && (part->dwMissionID & PLAYERID_MASK) == playerID)
				{
					if (TESTADMIRAL(part->dwMissionID))
					{
						if ((admiral = search) != 0)
						{
							if (admiral->GetAdmiralHotkey() == fkey)
							{
								admiral->SetAdmiralHotkey(0);
								break;		
							}
						}
					}
				}
			}

			search = search->nextTarget;
		}
	}
	return;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleGroupSelect(U32 hotkey)
{
	U32 index = 0;

	switch(hotkey)
	{
	case IDH_GROUP_1_SELECT:
		index = 0;
		break;
	case IDH_GROUP_2_SELECT:
		index = 1;
		break;
	case IDH_GROUP_3_SELECT:
		index = 2;
		break;
	case IDH_GROUP_4_SELECT:
		index = 3;
		break;
	case IDH_GROUP_5_SELECT:
		index = 4;
		break;
	case IDH_GROUP_6_SELECT:
		index = 5;
		break;
	case IDH_GROUP_7_SELECT:
		index = 6;
		break;
	case IDH_GROUP_8_SELECT:
		index = 7;
		break;
	case IDH_GROUP_9_SELECT:
		index = 8;
		break;
	case IDH_GROUP_0_SELECT:
		index = 9;
		break;
	default:
		CQBOMB0("Invalid hotkey for group selecting");
	}

	if (group[index])
	{
		IBaseObject * obj;
		ObjSet set, tmpSet;

		set.numObjects = group[index]->GetObjects(set.objectIDs);
		const U32 origNum = set.numObjects;
		
		OBJLIST->FlushHighlightedList();

		while (set.numObjects-- > 0)
		{
			obj = OBJLIST->FindObject(set.objectIDs[set.numObjects]);
			if (obj)
			{
				OBJLIST->HighlightObject(obj);
				tmpSet.objectIDs[tmpSet.numObjects++] = set.objectIDs[set.numObjects];
			}
		}

		if (tmpSet.numObjects==0)
		{
			delete group[index].Ptr();
			group[index] = 0;
		}
		else
		if (origNum != tmpSet.numObjects)
		{
			group[index]->InitGroup(tmpSet.objectIDs, tmpSet.numObjects, 0);
			EVENTSYS->Send(CQE_NEW_SELECTION, static_cast<ObjectComm *>(this));
			OBJLIST->FlushSelectedList();
			OBJLIST->SelectHighlightedObjects();
		}
		else  // selecting the same group
		if (selectTimer > 0 && selectTimerGroup==S32(index))
			handleGotoUnits();
		else
		{
			EVENTSYS->Send(CQE_NEW_SELECTION, static_cast<ObjectComm *>(this));
			OBJLIST->FlushSelectedList();
			OBJLIST->SelectHighlightedObjects();
		}

		selectTimer = SELECT_GOTO_PERIOD;
		selectTimerGroup = index;
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::onMissionClose (void)
{
	int i;

	for (i = 0; i < MAX_GROUPS; i++)
	{
		delete group[i].Ptr();		// delete the old group
		group[i] = 0;
	}

	bMissionClosed = true;
}
//-------------------------------------------------------------------
//
void ObjectComm::onMissionOpen (void)
{
	ObjSet tmp[MAX_GROUPS];
	const U32 playerID = MGlobals::GetThisPlayer();
	MPartNC part;
	IBaseObject * obj = OBJLIST->GetTargetList();
	int i;

	bMissionClosed = false;

	while (obj)
	{
		if ((part = obj).isValid())
		{
			if (part->playerID == playerID)
			{
				const U32 id = part->controlGroupID;
				if (id && id <= MAX_GROUPS)
				{
					CQASSERT(tmp[id-1].numObjects < MAX_SELECTED_UNITS);
					tmp[id-1].objectIDs[tmp[id-1].numObjects++] = part->dwMissionID;
				}
				else
				{
					part->controlGroupID = 0;
				}
			}
			else /// different player
			{
				part->controlGroupID = 0;
			}
		}

		obj = obj->nextTarget;
	}

	for (i = 0; i < MAX_GROUPS; i++)
	{
		if (tmp[i].numObjects)
		{
			obj = ARCHLIST->CreateInstance("Group!!Default");
			CQASSERT(obj);

			obj->QueryInterface(IGroupID, group[i], NONSYSVOLATILEPTR);
			group[i]->InitGroup(tmp[i].objectIDs, tmp[i].numObjects, 0);
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleStanceChange (UNIT_STANCE unitStance)
{
	// take everything that is selected and change it's stance
	IBaseObject *obj = OBJLIST->GetSelectedList();

	if (obj)
	{
		USR_PACKET<STANCE_PACKET> packet;
		
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.attackOk = true;

		if (initializeUserPacket3(&packet, obj, matchCaps) != 0)
		{
			packet.stance = unitStance;
			packet.userBits = 1;
			localSendPacket(&packet);
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleSupplyStanceChange (enum SUPPLY_SHIP_STANCE stance)
{
	// take everything that is selected and change it's stance
	IBaseObject *obj = OBJLIST->GetSelectedList();

	if (obj)
	{
		USR_PACKET<SUPPLY_STANCE_PACKET> packet;
		
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.supplyOk = true;

		if (initializeUserPacket3(&packet, obj, matchCaps) != 0)
		{
			packet.stance = stance;
			packet.userBits = 1;
			localSendPacket(&packet);
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleHarvestStanceChange (enum HARVEST_STANCE stance)
{
	IBaseObject *obj = OBJLIST->GetSelectedList();

	if (obj)
	{
		if(obj->objClass == OC_PLATFORM)
		{
			USR_PACKET<HARVEST_STANCE_PACKET> packet;
		
			packet.objectID[0] = obj->GetPartID();
			packet.init(1, false);
			packet.stance = stance;
			packet.userBits = 1;

			localSendPacket(&packet);
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleFighterStanceChange (enum FighterStance stance)
{
	// take everything that is selected and change it's stance
	IBaseObject *obj = OBJLIST->GetSelectedList();

	if (obj)
	{
		USR_PACKET<FIGHTER_STANCE_PACKET> packet;
		
		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.attackOk = true;

		if (initializeUserPacket3(&packet, obj, matchCaps) != 0)
		{
			packet.stance = stance;
			packet.userBits = 1;
			localSendPacket(&packet);
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleAdmiralTacticChange (enum ADMIRAL_TACTIC stance)
{
	IBaseObject * admiral=0;
	U32 admiralID = 0;
	if (findLowestAdmiral(admiral, admiralID))
	{
		USR_PACKET<ADMIRAL_TACTIC_PACKET> packet;
		
		packet.objectID[0] = admiralID;
		packet.stance = stance;
		packet.userBits = 1;
		packet.init(1);
		localSendPacket(&packet);
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleFormationChange (U32 slotID)
{
	IBaseObject * admiral=0;
	U32 admiralID = 0;
	if (findLowestAdmiral(admiral, admiralID))
	{
		USR_PACKET<ADMIRAL_FORMATION_PACKET> packet;
		
		packet.objectID[0] = admiralID;
		packet.slotID = slotID;
		packet.userBits = 1;
		packet.init(1);
		localSendPacket(&packet);
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleMovePos (S32 x, S32 y, WPARAM wParam)
{
	IBaseObject * obj;

	if ((obj = OBJLIST->GetSelectedList()) == NULL)
	{
		return;
	}

	if (DEFAULTS->GetDefaults()->bEditorMode)
	{
		Vector vec;

		if (ScreenToPoint(x, y, vec) != 0 && bEditorMoveOk )
		{
			// correct the position
			RECT rc;
			getObjectMoveRect(rc, vec);

			VOLPTR(IPhysicalObject) phys;
			if ((phys = obj) != 0)
				phys->SetPosition(vec, SECTOR->GetCurrentSystem());
			
			// reset the footprint in the sector map
			COMPTR<ITerrainMap> map;
			U32 systemID = SECTOR->GetCurrentSystem();
			SECTOR->GetTerrainMap(systemID, map);
			if (map)
			{
				IBaseObject * objList = OBJLIST->GetObjectList();
				while (objList)
				{
					if (objList->GetSystemID() == systemID)
					{
						objList->SetTerrainFootprint(map);
					}
					objList = objList->next;
				}
			}
			
			ENGINE->update_instance(obj->GetObjectIndex(),0,0);
		}
	}
	else
	{
		handleGotoPos(x, y, wParam);
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handlePatrolPos (S32 x, S32 y)
{
	// patrol the position from where you are now to where you want to go
	Vector vec;
	IBaseObject *obj = OBJLIST->GetSelectedList();

	if (obj == NULL)
	{
		return;
	}

	if (ScreenToPoint(x, y, vec) && obj)
	{
		GRIDVECTOR dst;
		dst = vec;

		USR_PACKET<PATROL_PACKET> packet;

		MISSION_DATA::M_CAPS matchCaps;
		memset(&matchCaps, 0, sizeof(matchCaps));
		matchCaps.moveOk = true;

		U32 systemID = SECTOR->GetCurrentSystem();

		//do not allow patrolling between systems
		if (initializeUserPacketSystem(&packet, obj, matchCaps,systemID))
		{
			IGroup * group = OBJLIST->FindGroupObject(packet.objectID[0]);
			if(group)
			{
				U32 gList[MAX_SELECTED_UNITS];
				group->GetObjects(gList);
				obj = OBJLIST->FindObject(gList[0]);
			}
			else
				obj = OBJLIST->FindObject(packet.objectID[0]);
			CQASSERT(obj);
			if(obj)
			{
				packet.patrolEnd = dst;
				packet.patrolStart = obj->GetGridPosition();
				packet.userBits = 0;
				localSendPacket(&packet);
			}
		}
	}

	globalState = GS_EMPTY_STATE;
}
//----------------------------------------------------------------------------------//
//
bool ObjectComm::findLowestAdmiral (IBaseObject * & admiral, U32 & admiralID)
{
	IBaseObject * obj = OBJLIST->GetSelectedList();
	MPart flagship = NULL;
	VOLPTR(IAdmiral) admiralObj;
	MPart part;

	U32 admiralKey = 10000000; // much greater than highest value
	admiralID = 0;
	admiral = NULL;

	bool bFoundAdmiral = false;

	while (obj)
	{
		flagship = NULL;
		part = obj;

		if (part.isValid())
		{
			if (MGlobals::IsFlagship(part->mObjClass))
			{
				// this object is the admiral
				flagship = part;
			}
			else if (part->admiralID)
			{
				// this object contains the admiral
				 flagship = OBJLIST->FindObject(part->admiralID);
			}

			if (flagship && flagship.isValid())
			{
				admiralObj = flagship.obj;
				CQASSERT(admiralObj);

				U32 tmpKey = admiralObj->GetAdmiralHotkey();
				if (tmpKey < admiralKey)
				{
					admiralKey = tmpKey;
					admiral = admiralObj.Ptr();
					admiralID = flagship->dwMissionID;
					bFoundAdmiral = true;
				}
			}
		}
		obj = obj->nextSelected;
	}

	return bFoundAdmiral;
}
//-------------------------------------------------------------------
//
void ObjectComm::makeGroupsExclusive (U32 index)
{
	ObjSet newSet, oldSet;
	U32 i;

	newSet.numObjects = group[index]->GetObjects(newSet.objectIDs);
	
	for (i = 0; i < 10; i++)
	{
		if (i != index && group[i].Ptr()!=0)
		{
			oldSet.numObjects = group[i]->GetObjects(oldSet.objectIDs);

			oldSet -= newSet;
			if (oldSet.numObjects)
				group[i]->InitGroup(oldSet.objectIDs, oldSet.numObjects, 0);
			else
			{
				delete group[i].Ptr();
				group[i] = 0;
			}
		}
	}
}
//-------------------------------------------------------------------
// true if all highlighted objects are selected as well
//
BOOL32 ObjectComm::highlightedIsSelected (void)
{
	IBaseObject * obj = OBJLIST->GetHighlightedList();
	BOOL32 result = 1;

	while (obj)
	{
		if (obj->bSelected == 0)
		{
			result = 0;
			break;
		}
		
		obj = obj->nextHighlighted;
	}

	return result;
}
//-------------------------------------------------------------------
//
bool ObjectComm::testAreaAccess (const Vector & pos)
{
	TCallback callback;
	COMPTR<ITerrainMap> map;
	GRIDVECTOR vec;
	vec = pos;

	SECTOR->GetTerrainMap(SECTOR->GetCurrentSystem(), map);

	return map->TestSegment(vec, vec, &callback);
}
//-------------------------------------------------------------------
//
bool ObjectComm::testCapture (IBaseObject * highlite)
{
	CQASSERT(highlite != NULL);
	CQASSERT(OBJLIST->GetSelectedList() != NULL);

	// for now, just take the first troopship in the objlist and test it
	OBJPTR<ITroopship> troop;
	OBJLIST->GetSelectedList()->QueryInterface(ITroopshipID, troop);

	if (troop)
	{
		return troop->IsCaptureOk(highlite);
	}

	return false;
}
//-------------------------------------------------------------------
//
bool ObjectComm::checkInsideSystem (const Vector & pos)
{
	RECT rect;
	SECTOR->GetCurrentRect(&rect);
	Vector temp((rect.left-rect.right)/2, (rect.bottom-rect.top)/2, 0);		// -center
	temp += pos;
	S32 dist = F2LONG(sqrt((temp.x * temp.x) + (temp.y * temp.y)));

	return (dist < rect.right/2);
}
//-------------------------------------------------------------------
//
bool ObjectComm::testPlatformRange (IBaseObject * obj, const Vector & pos)
{
	bool result = true;
	MPart part = obj;
	GRIDVECTOR gVect;
	gVect = pos;

	if (obj && (obj->objClass == OC_PLATFORM))
	{
		if (obj->GetSystemID() != SECTOR->GetCurrentSystem())
			result = false;
		else
		{
			SINGLE diff = (gVect - obj->GetGridPosition());
			SINGLE radius;
			if(obj->objClass == OC_PLATFORM)
			{
				if(part->mObjClass == M_TENDER)
				{
					radius = ((BT_PLAT_REPAIR_DATA *)(ARCHLIST->GetArchetypeData(obj->pArchetype)))->supplyRange;
				}
				else //it is a gunplat
				{
					radius = ((BT_PLAT_GUN *)(ARCHLIST->GetArchetypeData(obj->pArchetype)))->outerWeaponRange;
				}
			}
			else
			{
				return result;
			}
			if (diff > radius)
				result = false;
		}
	}

	return result;
}
//-------------------------------------------------------------------
//
bool ObjectComm::allSameType (IBaseObject * selected)
{
	MPart part=selected;
	bool result = true;
	M_OBJCLASS objClass = (part.isValid()) ? part->mObjClass : static_cast<M_OBJCLASS>(0);

	while (result && (part = part.obj->nextSelected).isValid())
	{
		result = (objClass == part->mObjClass);
	}

	return result;
}
//-------------------------------------------------------------------
//
void ObjectComm::handleGlobalModeSwitch (U32 hotkey)
{
	// change made - no longer toggle between global cursor states 

	switch (hotkey)
	{
	case IDH_SPECIAL_ABILITY:
	case IDH_SPECIAL_ABILITY1:
	case IDH_SPECIAL_ABILITY2:
		{
			MPart part = OBJLIST->GetSelectedList();
			if (part.isValid() && allSameType(part.obj))		// single selection
			{
				if (part->caps.admiralOk)
				{
					globalState = GS_EMPTY_STATE;
				}
				else
				{
					if (part->caps.mimicOk)
					{
						handleSpecialAbility(part.obj,0,hotkey);
						globalState = GS_SPECIAL_STATE;
					}
					else
					{
						if (part->caps.specialAbilityOk)
						{
							handleSpecialAbility(part.obj,0,hotkey);
						}
						else if (part->caps.specialAttackOk || part->caps.specialAttackShipOk || part->caps.specialEOAOk || part->caps.supplyOk || 
								 part->caps.repairOk || part->caps.probeOk || part->caps.synthesisOk || part->caps.specialTargetPlanetOk)
						{
							globalState = GS_SPECIAL_STATE;
						}
					}
				}
			}
		}
		break;

	case IDH_DEFEND:
		{
			MISSION_DATA::M_CAPS caps = getCaps(OBJLIST->GetSelectedList());
			if (caps.defendOk)
			{
				globalState = GS_DEFEND_STATE;
			}
		}
		break;

	case IDH_SELL:
		{
			MPart part = OBJLIST->GetSelectedList();
			if (part.isValid() && part.obj->nextSelected==0)		// single selection
			{
				if (part->caps.salvageOk && part->caps.buildOk)	// make sure we aren't already in process of building
				{
					globalState = GS_SELL_STATE;
					BUILDARCHEID = 0;
				}
			}
		}
		break;

	case IDH_REPAIR:
		{
			MPart part = OBJLIST->GetSelectedList();		

			if (part.isValid() && part.obj->nextSelected == 0)
			{
				if (part->caps.repairOk && part->caps.buildOk)
				{
					globalState = GS_REPAIR_STATE;
					BUILDARCHEID = 0;
				}
			}
		}
		break;

	case IDH_RALLY_POINT:
		{
			MISSION_DATA::M_CAPS caps = getCaps(OBJLIST->GetSelectedList());

			if (caps.buildOk && OBJLIST->GetSelectedList()->objClass == OC_PLATFORM)
			{
				globalState = GS_RALLY_STATE;
				bRallyEventSent = true;
				EVENTSYS->Send(CQE_UI_RALLYPOINT, (void *)1);
			}
		}
		break;

	case IDH_PATROL:
		{
			MISSION_DATA::M_CAPS caps = getCaps(OBJLIST->GetSelectedList());

			if (caps.moveOk)
			{
				globalState = GS_PATROL_STATE;
			}
		}
		break;
	case IDH_TARGET_POSITION:
		{
			MISSION_DATA::M_CAPS caps = getCaps(OBJLIST->GetSelectedList());

			if (caps.targetPositionOk)
			{
				globalState = GS_TARGET_POSITON_STATE;
			}
		}
		break;
	case IDH_USE_ARTIFACT:
		{
			VOLPTR(IArtifactHolder) holder = OBJLIST->GetSelectedList();
			if(holder && holder->HasArtifact())
			{
				VOLPTR(IArtifact) artifact = holder->GetArtifact();
				if(artifact && artifact->IsUsable() && (!(artifact->IsToggle())))
				{
					globalState = GS_ARTIFACT;
				}
			}
		}
		break;
	}
}
//-------------------------------------------------------------------
//
bool ObjectComm::isExplored (IBaseObject * jumpgate)
{
	IBaseObject * exitGate = SECTOR->GetJumpgateDestination(jumpgate);

	if (exitGate && (DEFAULTS->GetDefaults()->bEditorMode || DEFAULTS->GetDefaults()->bSpectatorModeOn ||
		SECTOR->IsVisibleToPlayer(exitGate->GetSystemID(), MGlobals::GetThisPlayer())))
	{
		return true;
	}
	return false;
}
//-------------------------------------------------------------------
//
bool ObjectComm::selectedInSameSystem (IBaseObject *selected)
{
	U32 systemID = selected->GetSystemID();

	while ((selected = selected->nextSelected) != 0)
	{
		if (selected->GetSystemID() != systemID)
			return false;
	}

	return true;
}
//-------------------------------------------------------------------
// return true if destination gate is the same as current system
//
bool ObjectComm::isLoopBack (IBaseObject *selected, IBaseObject *gate)
{
	IBaseObject * exitGate = SECTOR->GetJumpgateDestination(gate);

	return (exitGate->GetSystemID() == (selected->GetSystemID() & ~HYPER_SYSTEM_MASK));
}
//-------------------------------------------------------------------
//
bool ObjectComm::allInSameSystem (IBaseObject * selected, U32 systemID)
{
	// ignore hyperspace while considering systemIDs
	// make sure at least one ship is in real space

	CQASSERT(systemID!=0);

	systemID |= HYPER_SYSTEM_MASK;
	do 
	{
		const U32 objSystem = selected->GetSystemID();
		if (((objSystem ^ systemID) & ~HYPER_SYSTEM_MASK) != 0)
			return false;
		systemID &= objSystem;
	} while ((selected = selected->nextSelected) != 0);

	if ((systemID & HYPER_SYSTEM_MASK) == 0)
		return true;
	return false;
}
//-------------------------------------------------------------------
//
void ObjectComm::getObjectMoveRect (RECT &rect, Vector &pos)
{
	int i,j;
	Vector position;

	S32 x, y;
	WM->GetCursorPos(x, y);

	position.x = x;
	position.y = y;
	position.z = 0;

	RECT rc;
	SECTOR->GetSystemRect(SECTOR->GetCurrentSystem(), &rc);
	int numGrids;
	numGrids = (rc.right - rc.left)/GRIDSIZE;


	IBaseObject * obj;
	obj = OBJLIST->GetSelectedList();

//	CQASSERT(obj);
	if (obj == NULL)
	{
		return;
	}

	if (CAMERA->ScreenToPoint(position.x, position.y, position.z))
	{
		switch (obj->objClass)
		{
		case OC_JUMPGATE:
			//snap a big ship to the center of a square
			i = floor(position.x/GRIDSIZE+1.0);
			j = floor(position.y/GRIDSIZE+1.0);
			
			if (i > numGrids)
				i = numGrids;
			if (j > numGrids)
				j = numGrids;
			if (i < 1)
				i = 1;
			if (j < 1)
				j = 1;
			
			pos.x = i*GRIDSIZE-HALFGRID;
			pos.y = j*GRIDSIZE-HALFGRID;
			
			rect.left = pos.x-HALFGRID;
			rect.top = pos.y-HALFGRID;
			rect.right = pos.x+HALFGRID;
			rect.bottom = pos.y+HALFGRID;
			break;
		case OC_PLANETOID:
			// correct the planet's position to snap to the closet grid apex
			i = floor(position.x/GRIDSIZE+0.5);
			j = floor(position.y/GRIDSIZE+0.5);
			
			if (i > numGrids-2)
				i = numGrids-2;
			if (j > numGrids-2)
				j = numGrids-2;
			if (i < 1)
				i = 1;
			if (j < 1)
				j = 1;
			
			pos.x = i*GRIDSIZE;
			pos.y = j*GRIDSIZE;
			
			rect.left = pos.x-GRIDSIZE;
			rect.top = pos.y-GRIDSIZE;
			rect.right = pos.x+GRIDSIZE;
			rect.bottom = pos.y+GRIDSIZE;
			break;

		case OC_PLATFORM:
			{
				PLATFORM_INIT<BASE_PLATFORM_DATA> *arch;
				arch = static_cast<PLATFORM_INIT<BASE_PLATFORM_DATA> *>(ARCHLIST->GetArchetypeHandle(obj->pArchetype));
				if(arch->pData->slotsNeeded == 0)
				{
					if (arch->fp_radius)
					{
						if (arch->fp_radius > HALFGRID)
						{
							//snap a big ship to the center of a square
							i = floor(position.x/GRIDSIZE+1.0);
							j = floor(position.y/GRIDSIZE+1.0);
							
							if (i > numGrids)
								i = numGrids;
							if (j > numGrids)
								j = numGrids;
							if (i < 1)
								i = 1;
							if (j < 1)
								j = 1;
							
							pos.x = i*GRIDSIZE-HALFGRID;
							pos.y = j*GRIDSIZE-HALFGRID;
							
							rect.left = pos.x-HALFGRID;
							rect.top = pos.y-HALFGRID;
							rect.right = pos.x+HALFGRID;
							rect.bottom = pos.y+HALFGRID;
						}
						else
						{
							//snap a little ship to the center of a square
							i = floor(position.x/HALFGRID+1.0);
							j = floor(position.y/HALFGRID+1.0);
							
							numGrids = (rc.right - rc.left)/HALFGRID;

							if (i > numGrids)
								i = numGrids;
							if (j > numGrids)
								j = numGrids;
							if (i < 1)
								i = 1;
							if (j < 1)
								j = 1;
							
							pos.x = i*HALFGRID-(HALFGRID>>1);
							pos.y = j*HALFGRID-(HALFGRID>>1);
							
							rect.left = pos.x-(HALFGRID>>1);
							rect.top = pos.y-(HALFGRID>>1);
							rect.right = pos.x+(HALFGRID>>1);
							rect.bottom = pos.y+(HALFGRID>>1);
						}
					}
				}
			}
			break;

		case OC_SPACESHIP:
			SPACESHIP_INIT<BASE_SPACESHIP_DATA> *arch;
			arch = static_cast<SPACESHIP_INIT<BASE_SPACESHIP_DATA> *>(ARCHLIST->GetArchetypeHandle(obj->pArchetype));
			if (arch->fp_radius)
			{
				if (arch->fp_radius > HALFGRID)
				{
					//snap a big ship to the center of a square
					i = floor(position.x/GRIDSIZE+1.0);
					j = floor(position.y/GRIDSIZE+1.0);
					
					if (i > numGrids)
						i = numGrids;
					if (j > numGrids)
						j = numGrids;
					if (i < 1)
						i = 1;
					if (j < 1)
						j = 1;
					
					pos.x = i*GRIDSIZE-HALFGRID;
					pos.y = j*GRIDSIZE-HALFGRID;
					
					rect.left = pos.x-HALFGRID;
					rect.top = pos.y-HALFGRID;
					rect.right = pos.x+HALFGRID;
					rect.bottom = pos.y+HALFGRID;
				}
				else
				{
					//snap a little ship to the center of a square
					i = floor(position.x/HALFGRID+1.0);
					j = floor(position.y/HALFGRID+1.0);
					
					numGrids = (rc.right - rc.left)/HALFGRID;

					if (i > numGrids)
						i = numGrids;
					if (j > numGrids)
						j = numGrids;
					if (i < 1)
						i = 1;
					if (j < 1)
						j = 1;
					
					pos.x = i*HALFGRID-(HALFGRID>>1);
					pos.y = j*HALFGRID-(HALFGRID>>1);
					
					rect.left = pos.x-(HALFGRID>>1);
					rect.top = pos.y-(HALFGRID>>1);
					rect.right = pos.x+(HALFGRID>>1);
					rect.bottom = pos.y+(HALFGRID>>1);
				}
			}
			break;
		default:
			break;
		}
	}
}
//-------------------------------------------------------------------
//
void ObjectComm::handleEditorMove (void)
{
	// is the position we're over good enough to place down our object?
	Vector pos;
	RECT rect;
	S32 x, y;

	WM->GetCursorPos(x, y);

	pos.x = x;
	pos.y = y;
	pos.z = 0;	
	
	CAMERA->ScreenToPoint(pos.x, pos.y, 0);
	SECTOR->GetCurrentRect(&rect);
	Vector temp((rect.left-rect.right)/2, (rect.bottom-rect.top)/2, 0);
	SINGLE dist = (temp.x * temp.x);// + (temp.y * temp.y);
	temp += pos;

	if (dist < ((temp.x * temp.x) + (temp.y * temp.y)))
	{
		bEditorMoveOk = false;
	}
	else
	{
		bEditorMoveOk = true;
	}

	// if we are are already over something, than put up the ban
	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(SECTOR->GetCurrentSystem(), map);
	if (map != NULL)
	{
		GRIDVECTOR grid;
		grid = pos;
		getObjectMoveRect(rect, pos);

		if ((rect.right - rect.left)/GRIDSIZE > 1)
		{
			// the 4-grid case, make sure all 4 grids are open before we place the object
			bool bPass = true;
			Vector v[4];
			v[0] = pos;
			v[1] = Vector(pos.x - GRIDSIZE, pos.y, pos.z);
			v[2] = Vector(pos.x - GRIDSIZE, pos.y - GRIDSIZE, pos.z);
			v[3] = Vector(pos.x, pos.y - GRIDSIZE, pos.z);

			IBaseObject * obj;
			obj = OBJLIST->GetSelectedList();
//			CQASSERT(obj);
			if (obj == NULL)
			{
				return;
			}

			for (int i = 0; i < 4; i++)
			{
				grid = v[i];
				if (map->IsGridEmpty(grid, obj->GetPartID(), true) == false)
				{
					bPass = false;
					break;
				}
			}
			bEditorMoveOk = bPass;
		}
		else 
		{
			// the one quarter or one full square case
			bool bFullSquare = (rect.right - rect.left) > HALFGRID;
			bEditorMoveOk = map->IsGridEmpty(grid, 0, bFullSquare);
		}
	}
}
//-------------------------------------------------------------------
//
ObjectComm::ANIMQUEUENODE::~ANIMQUEUENODE (void)
{
	delete pAnim.Ptr();
}
//-------------------------------------------------------------------
//
ObjectComm::ANIMQUEUELIST::~ANIMQUEUELIST (void)
{
	CQTRACE12("~ANIMQUEUELIST() called. PTR=0x%08X  %s\r\n", this, (comm->bMissionClosed)? "[CLOSED]" : 0);

	if (comm->bMissionClosed)
	{
		int i;
		for (i = 0; i < MAX_QUEUE_SIZE; i++)
		{
			q[i].pAnim = 0;			// ready deleted
		}
	}
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
struct _objcomm : GlobalComponent
{
	virtual void Startup (void)
	{
		comm = new DAComponent<ObjectComm>;
		AddToGlobalCleanupList((IDAComponent **) &comm);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (OBJLIST->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(comm->getBase(), &comm->eventHandle);

		comm->initializeResources();
	}
};

static _objcomm objcomm;

//-------------------------------------------------------------------
//-------------------------END ObjComm.cpp---------------------------
//-------------------------------------------------------------------
