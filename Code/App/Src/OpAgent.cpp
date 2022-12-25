//                                                                          //
//                                OpAgent.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/OpAgent.cpp 462   9/13/01 10:01a Tmauer $

   Network game state management
*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include <MGlobals.h>
#include "Startup.h"
#include "CQTrace.h"
#include "BaseHotRect.h"
#include "ObjSet.h"
#include "ObjList.h"
#include "CommPacket.h"
#include "IGroup.h"
#include "OpAgent.h"
#include "IAttack.h"
#include "IGotoPos.h"
#include "IRepairee.h"
#include "IMissionActor.h"
#include "Sector.h"
#include <DMBaseData.h>
#include "MPart.h"
#include "DBHotkeys.h"
#include "IFabricator.h"
#include "IHarvest.h"
#include "IAdmiral.h"
#include "ISupplier.h"
#include "ITroopship.h"
#include "IRecoverShip.h"
#include "MScript.h"
#include "GridVector.h"
#include "IJumpGate.h"
#include "INugget.h"
#include "IJumpPlat.h"
#include "ScrollingText.h"
#include "ICloak.h"
#include "IBuild.h"
#include "ObjMap.h"
#include "IArtifact.h"
#include <DCQGame.h>

#include <dplay.h>
#include <dplobby.h>
#include "ZoneLobby.h"
#include <TSmartPointer.h>
#include <EventSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <HeapObj.h>
#include <FileSys.h>

#define NUM_BINS			  16
#define SYNC_PERIOD	  (1000	/ NUM_BINS)	// time between throughput measurements
#define DEFAULT_RAND_MASK  0x00000001
#define MAX_RAND_MASK  0x0000001F
#define BACKOFF_PERIOD     2000		// 2 seconds

//--------------------------------------------------------------------------
// dev notes
//
// make sure packet is in the Op list when calling doCommand().
//    this ensures that user can complete the command immediately.
//    For Ops that have more than one unit, doCommand() should be prepared
//    units to drop out of the set on return from the interface call.
//


#define SUPERBASE_OVERHEAD  daoffsetofmember(SUPERBASE_PACKET, dwSize)		
#define MAX_PACKET_SIZE 256



#ifdef FINAL_RELEASE
#define SILENCE_TRACE
#endif

#ifdef SILENCE_TRACE

#define OPPRINT0(exp) ((void)0)
#define OPPRINT1(exp,p1) ((void)0)
#define OPPRINT2(exp,p1,p2) ((void)0)
#define OPPRINT3(exp,p1,p2,p3) ((void)0)
#define OPPRINT4(exp,p1,p2,p3,p4) ((void)0)
#define OPPRINT5(exp,p1,p2,p3,p4,p5) ((void)0)
#define OPPRINT6(exp,p1,p2,p3,p4,p5,p6) ((void)0)

#else	//  !FINAL_RELEASE

#define OPPRINT0(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp)
#define OPPRINT1(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1)
#define OPPRINT2(exp,p1,p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2)
#define OPPRINT3(exp,p1,p2,p3) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2, p3)
#define OPPRINT4(exp,p1,p2,p3,p4) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2, p3, p4)
#define OPPRINT5(exp,p1,p2,p3,p4,p5) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2, p3, p4, p5)
#define OPPRINT6(exp,p1,p2,p3,p4,p5,p6) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2, p3, p4, p5, p6)

#endif   // end !FINAL_RELEASE


struct LovelyVector
{
	SINGLE x, y, z;

	operator const Vector & (void)
	{
		return *((const Vector *)this);
	}

	operator const Vector * (void)
	{
		return (const Vector *)this;
	}
};

typedef LovelyVector LovelyFormation[MAX_SELECTED_UNITS];

#define FOFFSET 1000.0
#define FZOFFSET -500.0
static LovelyFormation formation = {
								{-FOFFSET, -FOFFSET, FZOFFSET},			// 1
								{FOFFSET, FOFFSET},						// 2
								{ -FOFFSET, FOFFSET, FZOFFSET},			// 3
								{FOFFSET, -FOFFSET, FZOFFSET},			// 4
								{-FOFFSET*2, 0, FZOFFSET},				// 5
								{+FOFFSET*2, 0, FZOFFSET},				// 6
								{0, FOFFSET*3, FZOFFSET},				// 7
								{0, -FOFFSET*3, FZOFFSET},				// 8
								{-FOFFSET*2, FOFFSET*3, FZOFFSET},		// 9
								{ FOFFSET*2, -FOFFSET*3, FZOFFSET},		// 10
								{ FOFFSET*2, +FOFFSET*3, FZOFFSET},		// 11
								{ -FOFFSET*2, -FOFFSET*3, FZOFFSET},	// 12
								{ FOFFSET*4, 0, 0 },					// 13
								{-FOFFSET*4, 0, 0 },					// 14
								{ FOFFSET*4, -FOFFSET*4, FZOFFSET},		// 15
								{-FOFFSET*4, +FOFFSET*4, FZOFFSET},		// 16
								{ FOFFSET*4, +FOFFSET*4, FZOFFSET},		// 17
								{-FOFFSET*4, -FOFFSET*4, FZOFFSET},		// 18
								{ FOFFSET*6, -FOFFSET*6, FZOFFSET},		// 19
								{-FOFFSET*6, +FOFFSET*6, FZOFFSET},		// 20
								{ FOFFSET*6, +FOFFSET*6, FZOFFSET},		// 21
								{-FOFFSET*6, -FOFFSET*6, FZOFFSET}	};	// 22



enum HOSTCMD
{
	NOTHING = 0,
	NEWGROUP,
	CANCEL,
	ENABLEJUMP,
	MOVETO,
	ATTACK,
	SPATTACK,
	PREJUMP,
	JUMP,
	STOP,
	ENABLEDEATH,
	AOEATTACK,
	WORMATTACK,
	OBJSYNC,
	OBJSYNC2,
	OPDATA,
	OPDATA2,
	PROCESS,			// user defined command
	FABRICATE,
	SYNCMONEY,
	SYNCVISIBILITY,
	SYNCSCORES,
	SYNCALLIANCE,
	SYNCNUGGETS,
	HARVEST,
	ADDTO,
	RALLY,
	ESCORT,
	MIGRATIONCOMPLETE,
	DOCKFLAGSHIP,
	UNDOCKFLAGSHIP,
	RESUPPLY,
	FABREPAIR,
	SHIPREPAIR,
	DELETEGROUP,
	CAPTURE,
	SALVAGE,
	PROBE,
	RECOVER,
	DROP_OFF,
	FABRICATE_POS,
	PATROL,
	FLEETDEF,
	FABRICATE_JUMP,
	NUGGETDATA,
	NUGGETDEATH,
	PLATFORMDEATH,
	CREATEWORMHOLE,
	RELOCATE,			// move queued commands to the end of the list
	CANCELQUEUED,		// cancel cammands that never executed (on the host)
	HOSTJUMP,			// only used on host side, for using jumpgates
	FORMATIONMOVETO,
	FORMATIONATTACKCMD,
	ARTIFACTTARGETED,
};

#define CONTROL_ON_FLAG 0x80 //used to tell the client that the control key was down, very hacky but...

//--------------------------------------------------------------------------//
//
struct SUBPACKET_MONEY
{
	struct _resource
	{
		S32 gas:8;
		S32 metal:8;
		S32 crew:8;
		S32 totalComPts:4;
		S32 usedComPts:4;
	}resources[MAX_PLAYERS];

	bool hasValue()
	{
		int i;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if(*((U32 *)(&(resources[i]))))
				return true;
		}
		return false;
	}

	void updateNetValues (S32 * netGas, S32 * netMetal, S32 * netCrew, S32 * netTotalComPts, S32 * netUsedComPts)
	{
		int i;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			netGas[i+1] += resources[i].gas;
			netMetal[i+1] += resources[i].metal;
			netCrew[i+1] += resources[i].crew;
			netTotalComPts[i+1] += resources[i].totalComPts;
			netUsedComPts[i+1] += resources[i].usedComPts;
		}
	}

	void get (S32 * netGas, S32 * netMetal, S32 * netCrew, S32 * netTotalComPts, S32 * netUsedComPts)
	{
		int i;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			S32 value = MGlobals::GetCurrentGas(i+1)-netGas[i+1];
			if(value > 127)
				value = 127;
			else if(value < -128)
				value = -128;
			resources[i].gas = value;

			value = MGlobals::GetCurrentMetal(i+1)-netMetal[i+1];
			if(value > 127)
				value = 127;
			else if(value < -128)
				value = -128;
			resources[i].metal = value;

			value = MGlobals::GetCurrentCrew(i+1)-netCrew[i+1];
			if(value > 127)
				value = 127;
			else if(value < -128)
				value = -128;
			resources[i].crew = value;

			value = MGlobals::GetCurrentTotalComPts(i+1)-netTotalComPts[i+1];
			if(value > 7)
				value = 7;
			else if(value < -8)
				value = -8;
			resources[i].totalComPts = value;

			value = MGlobals::GetCurrentUsedComPts(i+1)-netUsedComPts[i+1];
			if(value > 7)
				value = 7;
			else if(value < -8)
				value = -8;
			resources[i].usedComPts = value;
		}
	}

	void set (S32 * netGas, S32 * netMetal, S32 * netCrew, S32 * netTotalComPts, S32 * netUsedComPts) const
	{
		int i;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			netGas[i+1] = MGlobals::GetCurrentGas(i+1)+resources[i].gas;
			MGlobals::SetCurrentGas(i+1, netGas[i+1]);
			netMetal[i+1] = MGlobals::GetCurrentMetal(i+1)+resources[i].metal;
			MGlobals::SetCurrentMetal(i+1, netMetal[i+1]);
			netCrew[i+1] = MGlobals::GetCurrentCrew(i+1)+resources[i].crew;
			MGlobals::SetCurrentCrew(i+1, netCrew[i+1]);
			netTotalComPts[i+1] = MGlobals::GetCurrentTotalComPts(i+1)+resources[i].totalComPts;
			MGlobals::SetCurrentTotalComPts(i+1, netTotalComPts[i+1]);
			netUsedComPts[i+1] = MGlobals::GetCurrentUsedComPts(i+1)+resources[i].usedComPts;
			MGlobals::SetCurrentUsedComPts(i+1, netUsedComPts[i+1]);
		}
	}
};
//--------------------------------------------------------------------------//
//
struct SUBPACKET_VISIBILITY
{
	U8 visibility[MAX_PLAYERS];

	void get (void)
	{
		int i;
		for (i = 0; i < MAX_PLAYERS; i++)
			visibility[i] = MGlobals::GetVisibilityMask(i+1);
	}

	void set (void) const
	{
		int i;
		for (i = 0; i < MAX_PLAYERS; i++)
			MGlobals::SetVisibilityMask(i+1, visibility[i]);
	}

	bool operator == (const U8 _visibility[MAX_PLAYERS]) const 
	{
		return (memcmp(visibility, _visibility, sizeof(visibility)) == 0);
	}

	bool operator != (const U8 _visibility[MAX_PLAYERS]) const
	{
		return (memcmp(visibility, _visibility, sizeof(visibility)) != 0);
	}
};
//--------------------------------------------------------------------------//
//
struct SUBPACKET_ALLIANCE
{
	U8 alliance[MAX_PLAYERS];

	void get (void)
	{
		MGlobals::GetAllyData(alliance, sizeof(alliance));
	}

	void set (void) const
	{
		MGlobals::SetAllyData(alliance, sizeof(alliance));
		SECTOR->ComputeSupplyForAllPlayers();
	}

	bool operator == (const U8 _alliance[MAX_PLAYERS]) const 
	{
		return (memcmp(alliance, _alliance, sizeof(alliance)) == 0);
	}

	bool operator != (const U8 _alliance[MAX_PLAYERS]) const
	{
		return (memcmp(alliance, _alliance, sizeof(alliance)) != 0);
	}
};
//--------------------------------------------------------------------------//
//
struct TERMINATION_NODE
{
	struct TERMINATION_NODE * pNext;
	U32 victimID;
	U32 attackerID;
};
//--------------------------------------------------------------------------//
//
struct OPCOMPLETE_NODE
{
	struct OPCOMPLETE_NODE * pNext;
	U32 agentID;
	U32 dwMissionID;
};
//--------------------------------------------------------------------------//
//
struct RECALCPARENT_NODE
{
	struct RECALCPARENT_NODE * pNext;
	U32 opID;
};
//--------------------------------------------------------------------------//
//
struct HSTPREJUMP : BASE_PACKET
{
	U32 jumpgateID;
	bool bUserInitiated;

	HSTPREJUMP (void)
	{
		type = PT_USRMOVE;
	}
};
//--------------------------------------------------------------------------//
//
struct HSTJUMP : BASE_PACKET
{
	U32 jumpgateID;
	bool   bEnabled;

	HSTJUMP (void)
	{
		type = PT_USRMOVE;
	}
};
//--------------------------------------------------------------------------//
//
struct SYNCSTATS : BASE_PACKET
{
	U32 playerID;	// for alignment
	U32 updateCount;	// game update count
	U8 stats[];

	SYNCSTATS (void)
	{
		type = PT_SYNCSTATS;
	}
};
//--------------------------------------------------------------------------//
//
#pragma pack (push , 1)
struct HOST_SUBPACKET_HEADER		// host-side version
{
	U8			size;
	U8			cmd;
	U32			groupID;
};
#pragma pack ( pop )
//--------------------------------------------------------------------------//
//
struct SUBPACKET_HEADER		// used on client side (must be converted from HOST_SUBPACKET_HEADER)
{
	U32			size;		// 9 bits because it can be > 256  ( size + sizeof(U32))
	HOSTCMD		cmd;
	U32			opID;	
	U32			groupID;
};
//--------------------------------------------------------------------------//
//
struct SUPERBASE_PACKET
{
	struct SUPERBASE_PACKET * pNext;
	ObjSet					* pSet;
	U32						  opID;
	HOSTCMD					  hostCmd;
	U32						  parentOp;
	U32						  groupID;
	bool					  bCancelDisabled;
	bool					  bDelayDependency;		// data op should be started AFTER parent completes
	bool					  bStalled;				// op waiting for target creation
	bool					  bLongTerm;			// process that will be around awhile

	// begin regular packet data here
	DWORD					  dwSize;
	DPID					  fromID;
	PACKET_TYPE type    : PACKETTYPEBITS;
	U32			userBits : PACKETUSERBITS;
	U32			ctrlBits : PACKETCTRLBITS;
	U32			timeStamp : PACKETTIMEBITS;
	U16						  sequenceID, ackID;

	//
	// type specific data here
	//

	BASE_PACKET * castBasePacket (void) const
	{
		return (BASE_PACKET *) (&dwSize);
	}

	HSTPREJUMP * castPreJump (void) const
	{
		CQASSERT(hostCmd == PREJUMP);
		return (HSTPREJUMP *) (&dwSize);
	}

	HSTJUMP * castJump (void) const
	{
		CQASSERT(hostCmd == JUMP);
		return (HSTJUMP *) (&dwSize);
	}

	USRATTACK * castAttack (void) const
	{
		CQASSERT(hostCmd == ATTACK || hostCmd == SPATTACK || hostCmd == WORMATTACK || hostCmd == ARTIFACTTARGETED);
		return (USRATTACK *) (&dwSize);
	}

	USRATTACK * castDeath (void) const
	{
		CQASSERT(hostCmd == ENABLEDEATH);
		return (USRATTACK *) (&dwSize);
	}

	USRMOVE * castMove (void) const
	{
		CQASSERT(hostCmd==MOVETO || 
				 hostCmd==AOEATTACK);
		return (USRMOVE *) (&dwSize);
	}

	USRFORMATIONMOVE * castFormationMove (void) const
	{
		CQASSERT(hostCmd==FORMATIONMOVETO);
		return (USRFORMATIONMOVE *) (&dwSize);
	}

	FORMATIONATTACK * castFormationAttack (void) const
	{
		CQASSERT(hostCmd==FORMATIONATTACKCMD);
		return (FORMATIONATTACK *) (&dwSize);
	}

	USRJUMP * castUserJump (void) const
	{
		CQASSERT(hostCmd==HOSTJUMP);
		return (USRJUMP *) (&dwSize);
	}

	USRAOEATTACK * castAOEAttack (void) const
	{
		CQASSERT((hostCmd == MOVETO || hostCmd == AOEATTACK));
		return (USRAOEATTACK *) (&dwSize);
	}

	USEARTIFACTTARGETED * castArtifactTargeted (void) const
	{
		CQASSERT(hostCmd == ARTIFACTTARGETED);
		return (USEARTIFACTTARGETED *) (&dwSize);
	}

	USRWORMATTACK * castWormAttack (void) const
	{
		CQASSERT(hostCmd == WORMATTACK );
		return (USRWORMATTACK  *) (&dwSize);
	}

	USRPROBE * castProbe (void) const
	{
		CQASSERT(sizeof(USRMOVE)==sizeof(USRPROBE) && (hostCmd == MOVETO || hostCmd == PROBE));
		return (USRPROBE *) (&dwSize);
	}

	USRRECOVER * castRecover (void) const
	{
		CQASSERT(hostCmd == RECOVER || hostCmd == DROP_OFF);
		return (USRRECOVER *) (&dwSize);
	}

	USRDROPOFF * castDropOff (void) const
	{
		CQASSERT(hostCmd == DROP_OFF);
		return (USRDROPOFF *) (&dwSize);
	}

	USRMIMIC * castMimic (void) const
	{
		CQASSERT(type == PT_USRMIMIC);
		return (USRMIMIC *) (&dwSize);
	}

	USRCREATEWORMHOLE * castCreateWormhole (void) const
	{
		CQASSERT(hostCmd == CREATEWORMHOLE);
		return (USRCREATEWORMHOLE *) (&dwSize);
	}

	USRBUILD * castBuild (void) const
	{
		CQASSERT(type==PT_USRBUILD);
		return (USRBUILD *) (&dwSize);
	}

	STANCE_PACKET * castStance (void) const
	{
		CQASSERT(type==PT_STANCECHANGE);
		return (STANCE_PACKET *) (&dwSize);
	}

	SUPPLY_STANCE_PACKET * castSupplyStance (void) const
	{
		CQASSERT(type==PT_SUPPLYSTANCECHANGE);
		return (SUPPLY_STANCE_PACKET *) (&dwSize);
	}

	HARVEST_STANCE_PACKET * castHarvestStance (void) const
	{
		CQASSERT(type==PT_HARVESTSTANCECHANGE);
		return (HARVEST_STANCE_PACKET *) (&dwSize);
	}

	HARVEST_AUTOMODE_PACKET * castHarvestAutoMode (void) const
	{
		CQASSERT(type==PT_HARVESTERAUTOMODE);
		return (HARVEST_AUTOMODE_PACKET *) (&dwSize);
	}

	FIGHTER_STANCE_PACKET * castFighterStance (void) const
	{
		CQASSERT(type==PT_FIGHTERSTANCECHANGE);
		return (FIGHTER_STANCE_PACKET *) (&dwSize);
	}

	ADMIRAL_TACTIC_PACKET * castAdmiralTactic (void) const
	{
		CQASSERT(type==PT_ADMIRALTACTICCHANGE);
		return (ADMIRAL_TACTIC_PACKET *) (&dwSize);
	}

	ADMIRAL_FORMATION_PACKET * castAdmiralFormation (void) const
	{
		CQASSERT(type==PT_ADMIRALFORMATIONCHANGE);
		return (ADMIRAL_FORMATION_PACKET *) (&dwSize);
	}

	USRSPABILITY * castSpecialAbility (void) const
	{
		CQASSERT(type==PT_USRSPABILITY);
		return (USRSPABILITY *) (&dwSize);
	}

	FLEETDEF_PACKET * castFleetDef (void) const
	{
		CQASSERT(hostCmd==FLEETDEF);
		return (FLEETDEF_PACKET *) (&dwSize);
	}

	USRFAB * castFab (void) const
	{
		CQASSERT(hostCmd==FABRICATE);
		return (USRFAB *) (&dwSize);
	}

	USRFABJUMP * castFabJump (void) const
	{
		CQASSERT(hostCmd== FABRICATE_JUMP);
		return (USRFABJUMP *) (&dwSize);
	}

	USRFABPOS * castFabPos (void) const
	{
		CQASSERT(hostCmd== FABRICATE_POS);
		return (USRFABPOS *) (&dwSize);
	}

	U32 getFabArchetype (void) const
	{
		CQASSERT(hostCmd==FABRICATE||hostCmd==FABRICATE_POS||hostCmd==FABRICATE_JUMP);
		return ((USRFAB *)(&dwSize))->dwArchetypeID;
	}

	U32 getFabTargetID (void) const
	{
		CQASSERT(hostCmd==FABRICATE||hostCmd==FABRICATE_POS||hostCmd==FABRICATE_JUMP);
		return ((USRFAB *)(&dwSize))->planetID;
	}

	U32 getFabSlotID (void) const
	{
		CQASSERT(hostCmd==FABRICATE||hostCmd==FABRICATE_POS||hostCmd==FABRICATE_JUMP);
		return ((USRFAB *)(&dwSize))->slotID;
	}

	Vector getFabPosition (void) const
	{
		CQASSERT(hostCmd==FABRICATE_POS);
		return ((USRFABPOS *)(&dwSize))->position;
	}

	USRHARVEST * castHarvest (void) const
	{
		CQASSERT(hostCmd==HARVEST);
		return (USRHARVEST *) (&dwSize);
	}

	USRESCORT * castEscort (void) const
	{
		CQASSERT(hostCmd==ESCORT);
		return (USRESCORT *) (&dwSize);
	}

	USRDOCKFLAGSHIP * castDock (void) const
	{
		CQASSERT(hostCmd==DOCKFLAGSHIP);
		return (USRDOCKFLAGSHIP *) (&dwSize);
	}

	USRUNDOCKFLAGSHIP * castUndock (void) const
	{
		CQASSERT(hostCmd==UNDOCKFLAGSHIP);
		return (USRUNDOCKFLAGSHIP *) (&dwSize);
	}

	USRRESUPPLY * castResupply (void) const
	{
		CQASSERT(hostCmd==RESUPPLY);
		return (USRRESUPPLY *) (&dwSize);
	}

	USRFABREPAIR * castRepair (void) const
	{
		CQASSERT(hostCmd==FABREPAIR||hostCmd==SHIPREPAIR||hostCmd==SALVAGE);
		return (USRFABREPAIR *) (&dwSize);
	}

	USRFABREPAIR * castFabRepair (void) const
	{
		CQASSERT(hostCmd==FABREPAIR);
		return (USRFABREPAIR *) (&dwSize);
	}

	USRSHIPREPAIR * castShipRepair (void) const
	{
		CQASSERT(hostCmd==SHIPREPAIR);
		return (USRSHIPREPAIR *) (&dwSize);
	}

	USRCAPTURE * castCapture (void) const
	{
		CQASSERT(hostCmd == CAPTURE);
		return (USRCAPTURE *) (&dwSize);
	}

	USRFABSALVAGE * castFabSalvage (void) const
	{
		CQASSERT(hostCmd == SALVAGE);
		return (USRFABSALVAGE *) (&dwSize);
	}

	PATROL_PACKET * castPatrol (void) const
	{
		CQASSERT(hostCmd == PATROL);
		return (PATROL_PACKET *) (&dwSize);
	}

	ObjSet & getSet (void) const
	{
		return *pSet;
	}

	U32 getOperandSize (void) const
	{
		return ((U8 *)pSet) - ((U8 *)castBasePacket()) - sizeof(BASE_PACKET);
	}

	void *getOperands (void) const
	{
		return ((U8 *)castBasePacket()) + sizeof(BASE_PACKET);
	}

	bool isComplete (void) const
	{
		return (getSet().numObjects == 0);
	}

	U32 getTrueSetSize (void) const
	{
		return (type==PT_USRDOCKFLAGSHIP||type==PT_USRUNDOCKFLAGSHIP) ? 1 : getSet().numObjects;
	}

	U32 getOpDataTarget (void) const
	{
		CQASSERT(hostCmd==OPDATA);
		U8 * ptr = (U8 *) getOperands();
		ptr += getOperandSize() - sizeof(U32);
		return ((U32 *)ptr)[0];
	}

	U32 getAddToTarget (void) const
	{
		CQASSERT(hostCmd==ADDTO);
		return ((U32 *)getOperands())[0];
	}

	U32 getRelocateTarget (void) const
	{
		CQASSERT(hostCmd==RELOCATE);
		return ((U32 *)getOperands())[0];
	}

	// data destined for one object, the rest of the set is for dependencies
	bool isUserData (void) const
	{
		switch (hostCmd)
		{
		case OBJSYNC:
		case OPDATA:
		case ENABLEDEATH:
		case CANCEL:
			return true;
		}
		return false;
	}

	bool isProcess (void) const
	{
		switch (hostCmd)
		{
		case PROCESS:
		case FABRICATE:
		case FABRICATE_POS:
		case FABRICATE_JUMP:
		case SALVAGE:
		case SHIPREPAIR:
		case AOEATTACK: // for the minelayer I don't think this will cause problems
		case DOCKFLAGSHIP:
			return true;
		}
		return false;
	}

	U32 getDestSystemID (void) const;

	void doCommand (void);
	// returns true if handled
	void read  (IFileSystem * inFile);
	void write (IFileSystem * outFile);
	void print (void) const;
private:
	void doAttack (void);
	void doSpecialAttack (void);
	void doMove (void);
	void doPrejump (void);
	void doJump (void);
	void doStop (void);
	void doObjSync (void);
	void doOpData (void);
	void doProcess (void);
	void doDeath (void);
	void doCancel (void);
	void doAOEAttack (void);
	void doArtifactTargeted (void);
	void doWormAttack (void);
	void doFabricate (void);
	void doFabricateJump (void);
	void doFabricatePos (void);
	void doHarvest (void);
	void doEscort (void);
	void doAdd (void);
	void doDock (void);
	void doUndock (void);
	void doResupply (void);
	void doFabRepair (void);
	void doShipRepair (void);
	void doCapture (void);
	void doFabSalvage (void);
	void doProbe (void);
	void doRecover (void);
	void doDropOff (void);
	void doCreateWormhole (void);
	void doPatrol (void);
	void doNuggetDeath (void);
	void doPlatformDeath (void);
	void doFleetDef (void);
};
//--------------------------------------------------------------------------//
//
struct RandomIDGenerator
{
	U32 *randomIndex;			// randomIndex[numRandomEntries]
	int numRandomEntries;


	RandomIDGenerator (void)
	{
		memset(this, 0, sizeof(*this));
	}

	~RandomIDGenerator (void)
	{
		delete [] randomIndex;
	}

	void reset (void)
	{
		delete [] randomIndex;
		randomIndex = 0;
		numRandomEntries = 0;
	}

	void init (void)
	{
		int numEntries = getNumEntries();
		
		delete [] randomIndex;

		if (numEntries)
			randomIndex = new U32[numEntries];
		else
			randomIndex = 0;

		if ((numRandomEntries = numEntries) != 0)
		{
			IBaseObject * obj = OBJLIST->GetTargetList();
			while (obj)
			{
				CQASSERT(numEntries>0);
				randomIndex[--numEntries] = obj->GetPartID();
				obj = obj->nextTarget;
			}
		}
	}

	U32 rand (void)    // returns 0 when out of numbers
	{
		if (numRandomEntries <= 0)
			return 0;

		int index = ::rand();

		if (numRandomEntries > RAND_MAX/2)
			index *= ::rand();

		index = index % numRandomEntries;
		U32 result = randomIndex[index];

		randomIndex[index] = randomIndex[--numRandomEntries];
		return result;
	}

	void restore (U32 id)		// put an ID back into the list (one call to restore() per call to rand() is allowed)
	{
		randomIndex[numRandomEntries++] = id;
	}

	bool isEmpty (void) const
	{
		return (numRandomEntries==0);
	}

private:
	static U32 getNumEntries (void)
	{
		U32 result = 0;
		IBaseObject * obj = OBJLIST->GetTargetList();

		while (obj)
		{
			result++;
			obj = obj->nextTarget;
		}
		return result;
	}
};
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE OpAgent : public IOpAgent, IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(OpAgent)
	DACOM_INTERFACE_ENTRY(IOpAgent)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	//--------------------------------
	// data items go here
	//--------------------------------
	
	U32 eventHandle;		// connection handle

	U32 updateCounter;		// mark time, not saved

	bool bHostMigration;			// true if migrating the host
	bool bRealMigration;
	bool bMissionLoaded;			// only do work when in a "loaded" state

	//
	// incoming queue of commands
	//
	SUPERBASE_PACKET *pCommandList;
	//
	// currently operating commands
	//
	SUPERBASE_PACKET *pOpList;
	//
	// 
	// 
	U32 lastOpID;
	//
	// buffer used to create command packets
	//
	U8  packetBuffer[MAX_PACKET_SIZE+4];
	U32 bufferOffset;

	//
	// data related to object termination
	//
	TERMINATION_NODE * pHitList;

	//
	// data related to operation completion
	//
	OPCOMPLETE_NODE * pCompleteList;
	
	//
	// data related to operations that need dependency recalculation
	//
	RECALCPARENT_NODE * pRecalcList;


	// data for money synchronization
	S32 netGas[MAX_PLAYERS+1];
	S32 netMetal[MAX_PLAYERS+1];
	S32 netCrew[MAX_PLAYERS+1];
	S32 netTotalComPts[MAX_PLAYERS+1];
	S32 netUsedComPts[MAX_PLAYERS+1];

	// data for visibility synchronization
	U8 netVisibility[MAX_PLAYERS];

	// data for alliance synchronization
	U8 netAlliance[MAX_PLAYERS];

	// data for score synchronization
#define SCOREBUFFER_SIZE 16
	U8 scoreBuffer[SCOREBUFFER_SIZE];

//	RandomIDGenerator randomizer;

	void * clientPlaybackBuffer;		// used for testing network code on client side
	U32 clientPlaybackBufferOffset;

	// sync data throughput
	U32 syncBin[NUM_BINS], currentBin, syncThroughput, syncTick;
	U32 opdataBin[NUM_BINS], opdataThroughput;
	U32 harvestBin[NUM_BINS], harvestThroughput;
	U32 syncRandMask;		// controls percentage of objects that get sync'ed
	U32 syncBackoffTick;	// last tick when we hit our max bandwidth
	U32 totalDataSent;


	//--------------------------------
	// class methods
	//--------------------------------
	
	OpAgent (void);

	~OpAgent (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
	
	/* IOpAgent methods */

	virtual void OperationCompleted (U32 agentID, U32 dwMissionID);

	virtual void SetCancelState (U32 agentID, bool bEnable);

	virtual void SetLongTermState (U32 agentID, bool bEnable=true);		// for blackholes

	virtual void ObjectTerminated (U32 victimID, U32 attackerID);

	virtual U32 CreateOperation (U32 dwMissionID, void *lpBuffer, U32 bufferSize);

	virtual U32 CreateOperation (const struct ObjSet & dependentSet, void *lpBuffer, U32 bufferSize);

	virtual void SendOperationData (U32 agentID, U32 dwMissionID, void *lpBuffer, U32 bufferSize);

	virtual void Update (void);	// to be called at AI frame rate

	virtual void AddObjectToOperation (U32 agentID, U32 dwMissionID);

	virtual U32 GetNumAgents (void) const;

	virtual void FlushSyncData (void);	// called by Host before shutdown of net session

	virtual bool IsMaster (void);	// return true if on host machine AND there no pending ops from old host

	virtual bool HasPendingDeath (U32 dwMissionID);

	virtual void SwitchPendingDeath (U32 oldMissionID, U32 newMissionID);

	virtual U32  GetDataThroughput (U32 * syncMask, U32 * syncData, U32 * opData, U32 * harvest);	// returns total bytes sent (before multiply by players)

	virtual void SignalStaleGroupObject (U32 dwGroupID);

	virtual bool HasPendingOp (U32 dwMissionID);

	virtual bool HasPendingOp (IBaseObject * obj);

	virtual U32  GetWorkingOp (IBaseObject * obj);

	virtual void FlushOpQueueForUnit (IBaseObject * obj);

	virtual void TerminatePlayerUnits (U32 playerID);

	virtual bool HasActiveJump (const struct ObjSet & set);

	virtual bool EnumerateQueuedMoveAgents (const struct ObjSet & set, IAgentEnumerator * callback);

	virtual bool EnumerateFabricateAgents (U32 playerID, IAgentEnumerator * callback);

	virtual bool IsFabricating (IBaseObject * obj);

	virtual bool IsNotInterruptable (IBaseObject * obj);

	virtual bool GetOperationSet (U32 agentID, const struct ObjSet ** ppSet);

	virtual void SendNuggetDeath (U32 nuggetID, void *lpBuffer, U32 bufferSize);

	virtual void SendNuggetData (void *lpBuffer, U32 bufferSize);

	virtual void SendPlatformDeath (U32 platformID);

	virtual void ForceSyncData(IBaseObject * obj);

	virtual BOOL32 __stdcall New (void);

	virtual void __stdcall Close (void);

	virtual BOOL32 __stdcall Load (struct IFileSystem * inFile);
	
	virtual BOOL32 __stdcall Save (struct IFileSystem * outFile);

	virtual void ResetSyncItems (void);

	/* IEventCallback methods */

	GENRESULT __stdcall Notify (U32 message, void *param);

	/* OpAgent methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IOpAgent *>(this);
	}

	U32 allocOpID (void)
	{
		++lastOpID;
		return lastOpID;
	}

	SUPERBASE_PACKET * createSuperPacket (const BASE_PACKET & packet, U32 baseSize, const U32 objectID[MAX_SELECTED_UNITS]);

	static SUPERBASE_PACKET * createSuperPacket (const struct RWPACKET_HEADER * header);

	U32 findPendingBirth (U32 dwMissionID);		// returns opID of birth 

	void addPacket (SUPERBASE_PACKET * packet);

	void reset (void);

	void processCommands (void);

	void processHitList (void);

	void processOperationCompletion (void);

	void processRecalculation (void);

	void sendCommand (SUPERBASE_PACKET * packet);

	void sendUndockCommand (SUPERBASE_PACKET *packet, SUPERBASE_PACKET * prev);

	void sendDockCommand (SUPERBASE_PACKET *packet, SUPERBASE_PACKET * prev);

	void sendCancel (U32 opID, const ObjSet & cancelSet, bool bQueuedCancel);

	void sendHostCommand (SUPERBASE_PACKET * packet, SUPERBASE_PACKET * prev);
	
	void sendShipRepair (SUPERBASE_PACKET * packet);

	void sendHostJumpCommand (SUPERBASE_PACKET * packet);

	U32 bufferedSend (HOSTCMD cmd, U32 opID, void *operands, U32 operandSize, const ObjSet & set, bool controlState = false);

	void flushSendBuffer (void);

	void updateOpList (void);

	void onOpCompleted (SUPERBASE_PACKET *pCompletedOp, U32 opID);

	static SUPERBASE_PACKET * findConflictingOpForAdd (SUPERBASE_PACKET *pAdd);

	void findParentOp (SUPERBASE_PACKET *pOp);

	U32  findParentOp (const ObjSet & set);

	void killSomeUnits (SUPERBASE_PACKET * packet);

	static SUPERBASE_PACKET * findDataDependency (SUPERBASE_PACKET *pOp);  // find first dependency

	SUPERBASE_PACKET * findDependentItemForDeath (SUPERBASE_PACKET * pDeath, U32 objectID);

	SUPERBASE_PACKET * findOp (U32 opID) const;

	SUPERBASE_PACKET * findLastJump (U32 dwMissionID);		// find last queued jump command

	SUPERBASE_PACKET * findFirstAgent (U32 dwMissionID)
	{
		return findFirstAgent(OBJLIST->FindObject(dwMissionID), dwMissionID);
	}

	SUPERBASE_PACKET * findFirstAgent (const MPartNC & part, U32 dwMissionID);

	static void notifySetofCancel (U32 agentID, const ObjSet & set);

	void onNewHost (void);

	bool testMigrationState (void);		// return new migration state
	
	void addToHostBuffer (HOSTCMD cmd, U32 opID, U32 groupID, const void * operands, U32 operandSize);

	static SUPERBASE_PACKET * duplicatePacket (SUPERBASE_PACKET *packet);

	static SUPERBASE_PACKET * createPacket (HOSTCMD cmd, U32 operandSize);

	void objectTerminated (U32 victimID, U32 attackerID);

	void sendHostMoveCommand (SUPERBASE_PACKET * packet);

	// homogenous set, possibly different using jumpgates
	void sendHostMoveCommandFromSystem (SUPERBASE_PACKET * packet, U32 systemID, U32 playerID);

	void sendHostFormationMoveCommand (SUPERBASE_PACKET * packet);

	void sendHostFormationAttackCommand (SUPERBASE_PACKET * packet);

	void sendHostBuildCommand (SUPERBASE_PACKET * packet);

	void sendHostStanceChange (SUPERBASE_PACKET * packet);

	void sendHostSpecialAbility (SUPERBASE_PACKET * packet);

	void sendHostCloakChange (SUPERBASE_PACKET * packet);

	void sendHostEjectArtifact (SUPERBASE_PACKET * packet);

	void sendHostMimicChange (SUPERBASE_PACKET * packet);

	void sendHostSupplyStanceChange (SUPERBASE_PACKET * packet);

	void sendHostHarvestStanceChange (SUPERBASE_PACKET * packet);

	void sendHostHarvestAutoModeChange (SUPERBASE_PACKET * packet);

	void sendHostFighterStanceChange (SUPERBASE_PACKET * packet);

	void sendHostAdmiralTacticChange (SUPERBASE_PACKET * packet);

	void sendHostAdmiralFormationChange (SUPERBASE_PACKET * packet);

	void sendSyncMessages (void);

	void sendSyncMoney (void);

	void sendSyncVisibility (void);

	void sendSyncAlliance (void);

	void sendSyncScores (void);

	void sendSyncNuggets (void);

	void sendSyncStats (void);

	void enablePendingJumps (void);

	void removeFromOpList (U32 victimID);

	void updateSyncTick (void);

	void startClientPlayback (void);

	void updateClientPlayback (void);

	SUPERBASE_PACKET * findQueuedDockCommand (U32 dwMissionID);

	// remove dead guys from the set, remove NULLS
	static void validateSet (ObjSet & set);

	static const char * getPartName (U32 dwMissionID);

	// account for the fact that obj may be in the process of jumping
	U32 getRealSystemID (IBaseObject * obj);

	static void doCommand (SUPERBASE_PACKET * packet);

	void verifyDependencies (void) const;

	void printOplist (void);

	void receivePartRename (PARTRENAMEPACKET & packet);

	static void notifyOnUserStop (HOSTCMD hostcmd, U32 agentID, const ObjSet & set);

	static void setRallyPoint (U32 dwMissionID, const NETGRIDVECTOR & vector);

	static int getNumHiddenAdmiralsInSet (const ObjSet & set, U32 systemID);

	void operationCompleted (U32 agentID, U32 dwMissionID);

	// 
	// client side methods
	//
	void clientReceivePacket (BASE_PACKET * packet);
	
	void clientSyncStats (SYNCSTATS * packet);

	static void initSetFromGroupID (ObjSet & set, U32 groupID);

	static SUPERBASE_PACKET * clientCreateSuperPacket (const SUBPACKET_HEADER * header);

	void clientAppendOp (SUPERBASE_PACKET * packet);

	void clientCreateNewGroup (SUBPACKET_HEADER *header);

	void clientDeleteGroup (SUBPACKET_HEADER *header);

	void clientCancelOp (SUBPACKET_HEADER *header);

	void clientRelocateOp (SUBPACKET_HEADER *header);

	void clientCancelQueued (SUBPACKET_HEADER *header);

	void clientMoveTo (SUBPACKET_HEADER *header);

	void clientAttack (SUBPACKET_HEADER *header);
	
	void clientSPAttack (SUBPACKET_HEADER *header);

	void clientPrejump (SUBPACKET_HEADER *header);

	void clientJump (SUBPACKET_HEADER *header);

	void clientEnableJump (SUBPACKET_HEADER *header);

	void clientObjSync (SUBPACKET_HEADER *header);

	void clientStop (SUBPACKET_HEADER *header);

	void clientEnableDeath (SUBPACKET_HEADER *header);

	void clientAOEAttack (SUBPACKET_HEADER *header);

	void clientArtifactTargeted (SUBPACKET_HEADER *header);

	void clientWormAttack (SUBPACKET_HEADER *header);

	void clientProbe (SUBPACKET_HEADER *header);

	void clientRecover (SUBPACKET_HEADER *header);

	void clientDropOff (SUBPACKET_HEADER *header);

	void clientCreateWormhole (SUBPACKET_HEADER *header);

	void clientCreateProcess (SUBPACKET_HEADER *header);

	void clientOpData (SUBPACKET_HEADER *header);

	void clientFabricate (SUBPACKET_HEADER *header);

	void clientFabricateJump (SUBPACKET_HEADER *header);

	void clientFabricatePos (SUBPACKET_HEADER *header);

	void clientSyncMoney (SUBPACKET_HEADER *header);

	void clientSyncVisibility (SUBPACKET_HEADER *header);

	void clientSyncAlliance (SUBPACKET_HEADER *header);

	void clientSyncScores (SUBPACKET_HEADER *header);

	void clientSyncNuggets (SUBPACKET_HEADER *header);

	void clientHarvest (SUBPACKET_HEADER *header);

	void clientAddTo (SUBPACKET_HEADER *header);

	void clientRally (SUBPACKET_HEADER *header);

	void clientEscort (SUBPACKET_HEADER *header);

	void clientMigrationComplete (SUBPACKET_HEADER *header);

	void clientDock (SUBPACKET_HEADER *header);

	void clientUndock (SUBPACKET_HEADER *header);

	void clientResupply (SUBPACKET_HEADER *header);

	void clientFabRepair (SUBPACKET_HEADER *header);

	void clientShipRepair (SUBPACKET_HEADER *header);

	void clientCapture (SUBPACKET_HEADER *header);

	void clientFabSalvage (SUBPACKET_HEADER *header);

	void clientPatrol (SUBPACKET_HEADER *header);

	void clientSetFleetDef (SUBPACKET_HEADER *header);

	void clientNuggetData (SUBPACKET_HEADER *header);
	
	void clientNuggetDeath (SUBPACKET_HEADER *header);

	void clientPlatformDeath (SUBPACKET_HEADER *header);

	void filterCommandsForAdmiral (SUPERBASE_PACKET * packet);

	void appendToOpList (SUPERBASE_PACKET * packet, SUPERBASE_PACKET * endList)
	{
		if (endList)
			endList->pNext = packet;
		else
			pOpList = packet;

		// clear the pendingOp value for members of this set
		if (packet->isUserData() == false)
			onRemovalFromOp(packet->getSet());
	}

	void recalcDependencies (U32 opID);
	
	//
	// templated procedures, must be defined within the class
	//

	template <class Type> 
	void receiveUserCommand (const Type & packet)
	{
		if (HOSTID==PLAYERID)
		{
			USR_PACKET<Type> *tmp = (USR_PACKET<Type> *) &packet;
			SUPERBASE_PACKET * sp = createSuperPacket(packet, sizeof(Type), tmp->objectID);
			addPacket(sp);
		}
	}

	static void onRemovalFromOp (const MPartNC & part)   // called when an object is removed from a op set, clear the cached value
	{
		if (part.isValid())
		{
			part->pendingOp = 0;
			part->bPendingOpValid = 0;
		}
	}

	static void onRemovalFromOp (const ObjSet & set)
	{
		U32 i;

		for (i = 0; i < set.numObjects; i++)
		{
			onRemovalFromOp(OBJLIST->FindObject(set.objectIDs[i]));
		}
	}

	static void onAdditionToOp (const MPartNC & part, U32 processID)   // called when an object is added to a process
	{
		if (part.isValid())
		{
			part->pendingOp = processID;
			part->bPendingOpValid = true;
		}
	}

	static void onAdditionToOp (const ObjSet & set, U32 processID)
	{
		U32 i;

		for (i = 0; i < set.numObjects; i++)
		{
			onAdditionToOp(OBJLIST->FindObject(set.objectIDs[i]), processID);
		}
	}

	static bool isStanceLike (PACKET_TYPE type)
	{
		bool result = false;

		switch (type)
		{
		case PT_ADMIRALTACTICCHANGE:
		case PT_ADMIRALFORMATIONCHANGE:
		case PT_FIGHTERSTANCECHANGE:
		case PT_SUPPLYSTANCECHANGE:
		case PT_HARVESTSTANCECHANGE:
		case PT_HARVESTERAUTOMODE:
		case PT_STANCECHANGE:
		case PT_USRSPABILITY:
		case PT_USRCLOAK:
		case PT_USRMIMIC:
			result = true;
			break;
		}

		return result;
	}
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
OpAgent::OpAgent (void)
{
	// nothing else to init so far?
}
//--------------------------------------------------------------------------//
//
OpAgent::~OpAgent (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (FULLSCREEN && FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);

	reset();
}
//-------------------------------------------------------------------
//
void OpAgent::doCommand (SUPERBASE_PACKET * packet)
{
	CQASSERT(packet->parentOp==0);
	packet->doCommand();
	// data cannot have data dependencies
	if (packet->bStalled==0 && packet->isUserData()==false)
	{
		SUPERBASE_PACKET * dependent = findDataDependency(packet);		// find first dependency

		while (dependent)
		{
			dependent->parentOp = 0;
			doCommand(dependent);			// recursion, should only go 1 deep
			dependent = findDataDependency(packet);		// find first dependency
		}
	}
}
//-------------------------------------------------------------------
//
void OpAgent::verifyDependencies (void) const
{
	SUPERBASE_PACKET *node = pOpList, *parent=0;

	while (node)
	{
		if (node->parentOp)
		{
			parent = findOp(node->parentOp);
			CQASSERT(parent);
			if (node->isUserData())
			{
				// parent can be in running state, if host was idle when it sent the data! 
				// opdata's can sometimes be made to wait until the set equalizes (after a pending death is completed)
				// data can also wait if there is a middle dependency that has to be resolved
				CQASSERT(parent->parentOp!=0 || node != findDataDependency(parent));
				// see comments in clientObjSync(), below.
				CQASSERT(parent->isUserData()==false && "data can't depend on data");
			}
		}
		
		node = node->pNext;
	}
}
//-------------------------------------------------------------------
// receive notifications from event system
//
GENRESULT OpAgent::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_UPDATE:
		if (MGlobals::IsHost())
			updateSyncTick();
		break;
	case CQE_NEWHOST:
		onNewHost();
		bRealMigration = true;
		break;
	case CQE_DEBUG_HOTKEY:
		if (U32(param) == IDH_PRINT_OPLIST)
		{
			if (CQFLAGS.bGameActive)
				printOplist();
		}
		else
		if (U32(param) == IDH_FORCE_MIGRATION)
		{
			if (clientPlaybackBuffer)
			{
				VirtualFree(clientPlaybackBuffer, 0, MEM_RELEASE);
				clientPlaybackBuffer = 0;
				clientPlaybackBufferOffset = 0;
				onNewHost();
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(msg->wParam))
		{
		case IDM_PRINT_OPLIST:
			if (CQFLAGS.bGameActive)
				printOplist();
			break;
		} // end switch (LOWORD(msg->wParam))
		break;	// end case WM_COMMAND

	case CQE_NETPACKET:		// could receive packet before game is "active", must handle this case
		if (clientPlaybackBuffer==0 && bMissionLoaded)
		{
			BASE_PACKET * packet = (BASE_PACKET *) param;
			switch (packet->type)
			{
			case PT_USRMOVE:
				receiveUserCommand(*((USRMOVE *) packet));
				break;
			case PT_USRFORMATIONMOVE:
				receiveUserCommand(*((USRFORMATIONMOVE *) packet));
				break;
			case PT_FORMATIONATTACK:
				receiveUserCommand(*((FORMATIONATTACK *) packet));
				break;
			case PT_USRATTACK:
				receiveUserCommand(*((USRATTACK *) packet));
				break;
			case PT_USRSPATTACK:
				receiveUserCommand(*((USRSPATTACK *) packet));
				break;
			case PT_USRAOEATTACK:
				receiveUserCommand(*((USRAOEATTACK *) packet));
				break;
			case PT_USEARTIFACTTARGETED:
				receiveUserCommand(*((USEARTIFACTTARGETED *) packet));
				break;
			case PT_USRWORMATTACK:
				receiveUserCommand(*((USRWORMATTACK *) packet));
				break;
			case PT_USRPROBE:
				receiveUserCommand(*((USRPROBE *) packet));
				break;
			case PT_USRRECOVER:
				receiveUserCommand(*((USRRECOVER *) packet));
				break;
			case PT_USRDROPOFF:
				receiveUserCommand(*((USRDROPOFF *) packet));
				break;
			case PT_USRMIMIC:
				receiveUserCommand(*((USRMIMIC *) packet));
				break;
			case PT_USRCREATEWORMHOLE:
				receiveUserCommand(*((USRCREATEWORMHOLE *) packet));
				break;
			case PT_USRSTOP:
				receiveUserCommand(*((USRSTOP *) packet));
				break;
			case PT_USRBUILD:
				receiveUserCommand(*((USRBUILD *) packet));
				break;
			case PT_STANCECHANGE:
				receiveUserCommand(*((STANCE_PACKET *) packet));
				break;
			case PT_SUPPLYSTANCECHANGE:
				receiveUserCommand(*((SUPPLY_STANCE_PACKET *) packet));
				break;
			case PT_HARVESTSTANCECHANGE:
				receiveUserCommand(*((HARVEST_STANCE_PACKET *) packet));
				break;
			case PT_HARVESTERAUTOMODE:
				receiveUserCommand(*((HARVEST_AUTOMODE_PACKET *) packet));
				break;
			case PT_FIGHTERSTANCECHANGE:
				receiveUserCommand(*((FIGHTER_STANCE_PACKET *) packet));
				break;
			case PT_ADMIRALTACTICCHANGE:
				receiveUserCommand(*((ADMIRAL_TACTIC_PACKET *) packet));
				break;
			case PT_ADMIRALFORMATIONCHANGE:
				receiveUserCommand(*((ADMIRAL_FORMATION_PACKET *) packet));
				break;
			case PT_FLEETDEF:
				receiveUserCommand(*((FLEETDEF_PACKET *) packet));
				break;
			case PT_USRFAB:
				receiveUserCommand(*((USRFAB *) packet));
				break;
			case PT_USRFABJUMP:
				receiveUserCommand(*((USRFABJUMP *) packet));
				break;
			case PT_USRFABPOS:
				receiveUserCommand(*((USRFABPOS *) packet));
				break;
			case PT_PARTRENAME:
				receivePartRename(*((PARTRENAMEPACKET *) packet));
				break;
			case PT_USRHARVEST:
				receiveUserCommand(*((USRHARVEST *) packet));
				break;
			case PT_USRRALLY:
				receiveUserCommand(*((USRRALLY *) packet));
				break;
			case PT_USRESCORT:
				receiveUserCommand(*((USRESCORT *) packet));
				break;
			case PT_USRDOCKFLAGSHIP:
				receiveUserCommand(*((USRDOCKFLAGSHIP *) packet));
				break;
			case PT_USRUNDOCKFLAGSHIP:
				receiveUserCommand(*((USRUNDOCKFLAGSHIP *) packet));
				break;
			case PT_USRRESUPPLY:
				receiveUserCommand(*((USRRESUPPLY *) packet));
				break;
			case PT_USRFABREPAIR:
				receiveUserCommand(*((USRFABREPAIR *) packet));
				break;
			case PT_USRSHIPREPAIR:
				receiveUserCommand(*((USRSHIPREPAIR *) packet));
				break;
			case PT_USRCAPTURE:
				receiveUserCommand(*((USRCAPTURE *) packet));
				break;
			case PT_USRSPABILITY:
				receiveUserCommand(*((USRSPABILITY *) packet));
				break;
			case PT_USRCLOAK:
				receiveUserCommand(*((USRCLOAK *) packet));
				break;
			case PT_USR_EJECT_ARTIFACT:
				receiveUserCommand(*((USR_EJECT_ARTIFACT *) packet));
				break;
			case PT_USRFABSALVAGE:
				receiveUserCommand(*((USRFABSALVAGE *) packet));
				break;
			case PT_HOSTUPDATE:
				if (HOSTID != PLAYERID && packet->fromID == HOSTID)
					clientReceivePacket(packet);
				else
				{
					if (CQFLAGS.bTraceMission)
						OPPRINT1("REJECTING host update packet from 0x%X\r\n", packet->fromID);
				}
				break;
			case PT_PATROL:
				receiveUserCommand(*((PATROL_PACKET *) packet));
				break;
			case PT_USRKILLUNIT:
				receiveUserCommand(*((USRKILLUNIT *) packet));
				break;
			case PT_SYNCSTATS:
				clientSyncStats((SYNCSTATS *) packet);
				break;
			case PT_USRJUMP:
				receiveUserCommand(*((USRJUMP *) packet));
				break;
			}
		}
		break;

	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void OpAgent::recalcDependencies (U32 opID)
{
	RECALCPARENT_NODE * node = pRecalcList, *prev=0;

	while (node)
	{
		if (node->opID == opID)
			break;
		prev = node;
		node = node->pNext;
	}

	if (node==0)	// op is not in list yet
	{
		node = new RECALCPARENT_NODE;
		node->opID = opID;
		node->pNext = 0;
		if (prev)
			prev->pNext = node;
		else
			pRecalcList = node;
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::operationCompleted (U32 agentID, U32 dwMissionID)
{
	SUPERBASE_PACKET * node;

	if (agentID && CQFLAGS.bTraceMission != 0)
		OPPRINT2("OpCmp for 0x%X on #%d\n", dwMissionID, agentID);

	if (agentID && (node = findOp(agentID)) != 0)
	{
		if (node->getSet().removeObject(dwMissionID))
		{
			MPartNC part = OBJLIST->FindObject(dwMissionID);
			if (part.isValid())
			{
				part->lastOpCompleted = agentID;
				onRemovalFromOp(part);
			}
			
			if (IsMaster()==false)
			{
				//
				// need to flush data dependencies here!
				//
				SUPERBASE_PACKET * dependent = findDataDependency(node);		// find first dependency

				while (dependent)
				{
					dependent->parentOp = 0;
					doCommand(dependent);			// recursion, should only go 1 deep
					dependent = findDataDependency(node);		// find first dependency
				}
			}

			if (part.isValid())
			{
				SUPERBASE_PACKET * dependent = findFirstAgent(part, dwMissionID);
				// don't bother posting IDLE event on the client side
				// also, find_firstAgent() may return 0 if there is pending ObjSync data (on the client)!
				if (dependent==0 && IsMaster())
					EVENTSYS->Post(CQE_IDLE_UNIT, (void *)dwMissionID);
				else
				{
					// allow this object to move on before the op completes
					// 
					if (node->getSet().numObjects > 0)
					{
						recalcDependencies(agentID);
					}
				}
			}
		}
	}
	else
	if (U32(agentID))
		CQTRACE12("OperationCompleted():Can't find agentID #%d for part 0x%X", agentID, dwMissionID);
}
//--------------------------------------------------------------------------//
//
void OpAgent::SetCancelState (U32 agentID, bool bEnable)
{
	SUPERBASE_PACKET * node;

	if (agentID && (node = findOp(agentID)) != 0)
	{
		node->bCancelDisabled = !bEnable;
	}
	else
	if (agentID)
		CQTRACE11("SetCancelState():Can't find agentID #%d", agentID);
}
//--------------------------------------------------------------------------//
//
void OpAgent::SetLongTermState (U32 agentID, bool bEnable)
{
	SUPERBASE_PACKET * node;

	if (agentID && (node = findOp(agentID)) != 0)
	{
		node->bLongTerm = bEnable;
	}
	else
	if (agentID)
		CQTRACE11("SetLongTermState():Can't find agentID #%d", agentID);
}
//--------------------------------------------------------------------------//
//
void OpAgent::OperationCompleted (U32 agentID, U32 dwMissionID)
{
	if (IsMaster())
		operationCompleted(agentID, dwMissionID);
	else
	{
		//
		// add agent to the completion list
		//
		OPCOMPLETE_NODE * node = pCompleteList, *prev = 0;

		while (node)
		{
			if (node->agentID == agentID && node->dwMissionID == dwMissionID)
				break;

			prev = node;
			node = node->pNext;
		}

		if (node==0)	// not found, add it to the list
		{
			node = new OPCOMPLETE_NODE;
			node->pNext = 0;
			node->agentID = agentID;
			node->dwMissionID = dwMissionID;

			if (prev)
				prev->pNext = node;
			else
				pCompleteList = node;
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::ObjectTerminated (U32 victimID, U32 attackerID)
{
	CQASSERT(IsMaster());

	objectTerminated(victimID, attackerID);
	
	IBaseObject * obj = OBJLIST->FindObject(victimID);
	if(obj && obj->objClass == OC_PLATFORM)
	{
		OBJPTR<IJumpPlat> jumpplat;
		obj->QueryInterface(IJumpPlatID,jumpplat);
		if (jumpplat)
		{
			IBaseObject * sibling = jumpplat->GetSibling();
			if (sibling)
			{
				objectTerminated(sibling->GetPartID(), 0/*attackerID - don't want to give double credit */);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::objectTerminated (U32 victimID, U32 attackerID)
{
	//
	// add victim to the termination list
	//
	TERMINATION_NODE * node = pHitList, *prev = 0;

	while (node)
	{
		if (node->victimID == victimID)
			break;

		prev = node;
		node = node->pNext;
	}

	if (node==0)	// not found, add it to the list
	{
		node = new TERMINATION_NODE;
		node->pNext = 0;
		node->victimID = victimID;
		node->attackerID = MGlobals::GetOwnerFromPartID(attackerID);

		if (prev)
			prev->pNext = node;
		else
			pHitList = node;
	}
}
//-------------------------------------------------------------------------------------------//
//
U32 OpAgent::CreateOperation (const struct ObjSet & set, void *lpBuffer, U32 bufferSize)
{
	CQASSERT(IsMaster());

	CQASSERT(set.numObjects!=0);
	CQASSERT(OBJLIST->FindObject(set.objectIDs[0]) && "dead guy creating a process!?");
	U32 newOp =  allocOpID();
	U32 oldTotal = totalDataSent;

	bufferedSend(PROCESS, newOp, lpBuffer, bufferSize, set);

	U32 dataAmount = totalDataSent - oldTotal;
	opdataBin[currentBin] += dataAmount;		// update opdata throughput
	opdataThroughput += dataAmount;
	
	SUPERBASE_PACKET * node, *prev=0;
	SUPERBASE_PACKET * pProcess = createPacket(PROCESS, 0);
	pProcess->getSet() = set;
	pProcess->parentOp = 0;
	pProcess->opID = newOp;

	// now append the new PROCESS to the list
	
	node = pOpList;
	while (node)
	{
		prev = node;
		node = node->pNext;
	}
	appendToOpList(pProcess, prev);
	prev = pProcess;

	onAdditionToOp(set, newOp);

	if (CQFLAGS.bTraceMission != 0)
	{
		OPPRINT1("Unit 0x%X created PROCESS: ", set.objectIDs[0]);
		pProcess->print();
	}

	node = pOpList;

	// stop when we reach the process we just added
	while (node && node->opID != newOp)
	{
		ObjSet tmpSet = node->getSet() & set;
		if (tmpSet.numObjects > 0)
		{
			if (node->parentOp==0)
				CQBOMB2("Unit '%s' attempted to create an op for a busy mission object '%s'.",
						 getPartName(set.objectIDs[0]), getPartName(tmpSet.objectIDs[0]));
			
			
			U32 relocOp = allocOpID();
			bufferedSend(RELOCATE, relocOp, &node->opID, sizeof(node->opID), tmpSet);
			node->getSet() -= tmpSet;

			SUPERBASE_PACKET * pRelOp = duplicatePacket(node);
			pRelOp->getSet() = tmpSet;
			pRelOp->opID = relocOp;
			pRelOp->parentOp = newOp;
			appendToOpList(pRelOp, prev);
			prev = pRelOp;
		}

		node = node->pNext;
	}

	return newOp;
}
//-------------------------------------------------------------------------------------------//
//
U32 OpAgent::CreateOperation (U32 dwMissionID, void *lpBuffer, U32 bufferSize)
{
	ObjSet set;
	set.numObjects = 1;
	set.objectIDs[0] = dwMissionID;
	return CreateOperation(set, lpBuffer, bufferSize);
}
//--------------------------------------------------------------------------//
//
void OpAgent::SendOperationData (U32 agentID, U32 dwMissionID, void *lpBuffer, U32 bufferSize)
{
	CQASSERT(IsMaster());
	U32 size = bufferSize + sizeof(U32);
	U8 buffer[MAX_PACKET_SIZE];
	ObjSet set;

	SUPERBASE_PACKET * pFirst = findOp(agentID);
	CQASSERT(pFirst && pFirst->getSet().contains(dwMissionID));

	set.numObjects = 1;
	set.objectIDs[0] = dwMissionID;
	if (pFirst->isProcess())
		set += pFirst->getSet();
	CQASSERT(set.objectIDs[0] == dwMissionID);		// first element is destination object

	if (lpBuffer && bufferSize)
		memcpy(buffer, lpBuffer, bufferSize);

	U32 oldTotal = totalDataSent;
	((U32 *)(buffer+bufferSize))[0] = agentID;
	bufferedSend(pFirst->isProcess() ? OPDATA: OPDATA2, allocOpID(), buffer, size, set);

	U32 dataAmount = totalDataSent - oldTotal;
	opdataBin[currentBin] += dataAmount;		// update opdata throughput
	opdataThroughput += dataAmount;
}
//--------------------------------------------------------------------------//
//
void OpAgent::SendNuggetData (void *lpBuffer, U32 bufferSize)
{
	CQASSERT(IsMaster());

	U32 oldTotal = totalDataSent;
	addToHostBuffer(NUGGETDATA, allocOpID(), 0, lpBuffer, bufferSize);
	U32 dataAmount = totalDataSent - oldTotal;
	opdataBin[currentBin] += dataAmount;		// update opdata throughput
	opdataThroughput += dataAmount;
}
//--------------------------------------------------------------------------//
//
void OpAgent::SendNuggetDeath (U32 nuggetID, void *lpBuffer, U32 bufferSize)
{
	CQASSERT(IsMaster());
	CQASSERT(nuggetID);
	CQASSERT(findFirstAgent(MPartNC(), nuggetID)==0);		// nugget must be idle before death

	U32 oldTotal = totalDataSent;
	addToHostBuffer(NUGGETDEATH, allocOpID(), nuggetID, lpBuffer, bufferSize);
	U32 dataAmount = totalDataSent - oldTotal;
	opdataBin[currentBin] += dataAmount;		// update opdata throughput
	opdataThroughput += dataAmount;
}
//--------------------------------------------------------------------------//
//
void OpAgent::SendPlatformDeath (U32 platformID)
{
	CQASSERT(IsMaster());
	CQASSERT(platformID);
	CQASSERT(findFirstAgent(MPartNC(), platformID)==0);		// platform must be idle before death

	U32 oldTotal = totalDataSent;
	addToHostBuffer(PLATFORMDEATH, allocOpID(), platformID, NULL, 0);
	U32 dataAmount = totalDataSent - oldTotal;
	opdataBin[currentBin] += dataAmount;		// update opdata throughput
	opdataThroughput += dataAmount;
}
//--------------------------------------------------------------------------//
//
void OpAgent::ForceSyncData(IBaseObject * obj)
{
	VOLPTR(IMissionActor) actor;
	char buffer[256];
	ObjSet set;
	int dataSent=0;

	set.numObjects = 1;
	set.objectIDs[0] = obj->GetPartID();
	CQASSERT(set.objectIDs[0] != 0);
	MPartNC part = obj;
	SUPERBASE_PACKET * node = findFirstAgent(part, set.objectIDs[0]);

	// do not perform updates for units that are jumping
	if (node==0 || node->hostCmd != JUMP || node->parentOp!=0)
	{
		if ((actor = obj) != 0)
		{
			int size;
			if ((size = actor->GetGeneralSyncData(buffer)) != 0)
			{
				CQASSERT(size+sizeof(U32) <= sizeof(buffer));

				U32 parentOp = 0;
				bool bNoDelayAction = false;
						
				if (node==0 || node->parentOp != 0)		// object has not yet started next command
				{
					parentOp = part->lastOpCompleted;
				}
				else
				{
					bNoDelayAction = true;		// action in progress
					parentOp = node->opID;		// current active op?

					// add the members of the process to the set, so that client will receive 
					// this packet at the right time
					if (node->isProcess())
					{
						set += node->getSet();
						CQASSERT(set.objectIDs[0] == obj->GetPartID());
					}
				}

				U32 oldTotal = totalDataSent;
				((U32 *)(buffer+size))[0] = parentOp;
				bufferedSend((bNoDelayAction) ? OBJSYNC : OBJSYNC2, allocOpID(), buffer, size+sizeof(U32), set);

				if (CQFLAGS.bTraceMission)
					OPPRINT5("Sending %s for unit 0x%X, #%d->#%d [%d bytes]\n", (bNoDelayAction) ? "OBJSYNC" : "OBJSYNC2",
							set.objectIDs[0], lastOpID, parentOp, size+sizeof(U32));
				
				U32 dataAmount = totalDataSent - oldTotal;
				syncBin[currentBin] += dataAmount;		// update sync data throughput
				syncThroughput += dataAmount;
				dataSent += dataAmount;
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::AddObjectToOperation (U32 agentID, U32 dwMissionID)
{
	CQASSERT(IsMaster());

	SUPERBASE_PACKET * node=0;
		
	if (agentID && (node = findOp(agentID)) != 0)
	{
		ObjSet & set = node->getSet();
		CQASSERT(node->isProcess());

		if (set.contains(dwMissionID) == false)
		{
			CQASSERT(set.numObjects < MAX_SELECTED_UNITS);
			set.objectIDs[set.numObjects++] = dwMissionID;

			if (CQFLAGS.bTraceMission != 0)
				OPPRINT2("Adding Unit 0x%X to agent #%d\n", dwMissionID, agentID);

			onAdditionToOp(OBJLIST->FindObject(dwMissionID), agentID);

			ObjSet newset;
			newset.numObjects = 1;
			newset.objectIDs[0] = dwMissionID;
			newset += set;
			U32 data = agentID;
			bufferedSend(ADDTO, allocOpID(), &data, sizeof(data), newset);
		}
	}	
	else
		CQBOMB2("AddObjectToOperation():Can't find agentID #%d for part 0x%X", agentID, dwMissionID);

	//
	// relocate any queued ops for this unit
	//

	if (node)
	{
		ObjSet & set = node->getSet();
		SUPERBASE_PACKET *prev=0, *pLast;
		node = pOpList;
		while (node)
		{
			prev = node;
			node = node->pNext;
		}
		node = pOpList;
		pLast = prev;

		while (node)
		{
			ObjSet tmpSet = node->getSet() & set;
			if (node->opID != agentID && tmpSet.numObjects > 0)
			{
				if (node->parentOp==0)
				{
					node = findOp(agentID);
					CQASSERT(node);
					CQBOMB2("Unit '%s' attempted to create an op for a busy mission object '%s'.",
							 getPartName(node->getSet().objectIDs[0]), getPartName(dwMissionID));
				}

				U32 relocOp = allocOpID();
				bufferedSend(RELOCATE, relocOp, &node->opID, sizeof(node->opID), tmpSet);
				node->getSet() -= tmpSet;

				SUPERBASE_PACKET * pRelOp = duplicatePacket(node);
				pRelOp->getSet() = tmpSet;
				pRelOp->opID = relocOp;
				pRelOp->parentOp = agentID;
				appendToOpList(pRelOp, prev);
				prev = pRelOp;
			}

			if (node == pLast)
				break;		// we've reached the end of the list
			node = node->pNext;
		}
	}

}
//--------------------------------------------------------------------------//
//
void OpAgent::onNewHost (void)
{
	bRealMigration = bHostMigration = true;

	//
	// we are still the client until all data packets have been processed
	//
	if (MGlobals::IsHost())
	{
		SUPERBASE_PACKET *node = pOpList;
		bRealMigration = (clientPlaybackBuffer!=0);		// assume migration if client playback is happening
		while (node)
		{
			bRealMigration |= node->isUserData();
			node = node->pNext;
		}
	}
}
//--------------------------------------------------------------------------//
//
bool OpAgent::testMigrationState (void)
{
	bool result = bHostMigration;

	if (result && CQFLAGS.bGameActive && clientPlaybackBuffer==0 && MGlobals::IsHost())
	{
		SUPERBASE_PACKET *node = pOpList;
		result = false;

		while (node)
		{
			// a queued PROCESS packet is really a user data packet too!
			if ((result = (node->isUserData() || (node->hostCmd==PROCESS&&node->parentOp!=0))) != 0)
				break;
			node = node->pNext;
		}

		if (bHostMigration != result)		// migration is over!
		{
			addToHostBuffer(MIGRATIONCOMPLETE, allocOpID(), 0, 0, 0);
			if (CQFLAGS.bTraceMission != 0)
				OPPRINT0("Host Migration completed!\n");
			bHostMigration = result;
			if (bRealMigration)		// only send update if state has changed
			{
				bRealMigration = result;
				EVENTSYS->Send(CQE_HOST_MIGRATE, 0);
			}
			bRealMigration = result;
			enablePendingJumps();
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 OpAgent::New (void)
{
	reset();
	bMissionLoaded=true;
	if (CQFLAGS.bClientPlayback && NETOUTPUT!=0)
		startClientPlayback();
	onNewHost();
	return 1;
}
//--------------------------------------------------------------------------//
//
void OpAgent::startClientPlayback (void)
{
	// remember: file contains some data at the beginning that is not ours
	U32 fileSize = NETOUTPUT->GetFileSize() - NETOUTPUT->GetFilePosition();

	if (fileSize && (clientPlaybackBuffer = VirtualAlloc(0, fileSize+4, MEM_COMMIT, PAGE_READWRITE)) != 0)
	{
		DWORD dwRead;
		S32 * endmarker;
		NETOUTPUT->ReadFile(0, clientPlaybackBuffer, fileSize, &dwRead, 0);

		endmarker = (S32 *) ( ((char *)clientPlaybackBuffer) + fileSize);
		*endmarker = -1;		// this is the end of the buffer
		dwRead = PAGE_READWRITE;
		// client side now monkey's with the data (clientReceivePacket())
//		VirtualProtect(clientPlaybackBuffer, fileSize+4, PAGE_READONLY, &dwRead);
		clientPlaybackBufferOffset = 0;
	}
	else
		CQERROR1("Failed to allocate %d bytes for clientPlaybackBuffer!", fileSize);
}
//--------------------------------------------------------------------------//
//
void OpAgent::Close (void)
{
	reset();
}
//--------------------------------------------------------------------------//
//
struct RWPACKET_HEADER		// used in host packet
{
	U32			size:8;
	HOSTCMD		cmd:8;		
	U32			bCancelDisabled:1;
	U32			bDelayDependency:1;
	U32			bStalled:1;
	U32			bLongTerm:1;
	U32			opID;
	U32			groupID;
	U32			parentOp;
	U32			numObjects;
	U32			objects[1];
	//  objects
	// operands
};
//--------------------------------------------------------------------------//
//
struct RWTERMINATION_NODE
{
	U32 victimID;
	U32 attackerID;

	RWTERMINATION_NODE & operator = (const TERMINATION_NODE & node)
	{
		victimID = node.victimID;
		attackerID = node.attackerID;
		return *this;
	}

	TERMINATION_NODE * getNode (void) const
	{
		TERMINATION_NODE * node = new TERMINATION_NODE;
		node->attackerID = attackerID;
		node->victimID = victimID;
		node->pNext = 0;
		return node;
	}
};
struct RWCOMPLETION_NODE
{
	U32 agentID;
	U32 dwMissionID;

	RWCOMPLETION_NODE & operator = (const OPCOMPLETE_NODE & node)
	{
		agentID = node.agentID;
		dwMissionID = node.dwMissionID;
		return *this;
	}

	OPCOMPLETE_NODE * getNode (void) const
	{
		OPCOMPLETE_NODE * node = new OPCOMPLETE_NODE;
		node->agentID = agentID;
		node->dwMissionID = dwMissionID;
		node->pNext = 0;
		return node;
	}
};
struct RWRECALCPARENT_NODE
{
	U32 opID;

	RWRECALCPARENT_NODE & operator = (const RECALCPARENT_NODE & node)
	{
		opID = node.opID;
		return *this;
	}

	RECALCPARENT_NODE * getNode (void) const
	{
		RECALCPARENT_NODE * node = new RECALCPARENT_NODE;
		node->opID = opID;
		node->pNext = 0;
		return node;
	}
};
//--------------------------------------------------------------------------//
//
SUPERBASE_PACKET * OpAgent::createSuperPacket (const RWPACKET_HEADER * header)
{
	SUPERBASE_PACKET * result;
	// size = sizeof superpacket + objset + sizeof operands
	U32 opsize = header->size - sizeof(RWPACKET_HEADER) - ((header->numObjects-1)*sizeof(U32));
	U32 size = sizeof(SUPERBASE_PACKET) + sizeof(ObjSet) + opsize;

	result = (SUPERBASE_PACKET *) calloc(size, 1);

	// copy the "additional materials"
	memcpy(result->castBasePacket()+1, ((U8*)header)+header->size-opsize, opsize);
	result->pSet = (ObjSet *) ( ((U8 *)result) + size - sizeof(ObjSet));

	result->getSet().numObjects = header->numObjects;
	memcpy(result->getSet().objectIDs, header->objects, header->numObjects*sizeof(U32));

	result->dwSize = size;
	result->hostCmd = header->cmd;
	result->opID = header->opID;
	result->groupID = header->groupID;
	result->bCancelDisabled = header->bCancelDisabled;
	result->bDelayDependency = header->bDelayDependency;
	result->bStalled = header->bStalled;
	result->bLongTerm = header->bLongTerm;
	result->parentOp = header->parentOp;

	return result;
}
//--------------------------------------------------------------------------//
//
void OpAgent::removeFromOpList (U32 victimID)
{
	SUPERBASE_PACKET *node = pOpList;
	MPartNC part = OBJLIST->FindObject(victimID);

	while (node)
	{
		if (node->hostCmd != PLATFORMDEATH)		// platform death is allowed after regular death
		{
			ObjSet & set = node->getSet();
			if (set.removeObject(victimID))
			{
				// cancel any actions
				if (IsMaster())
				{
					if (node->parentOp != 0)		// op never started
						addToHostBuffer(CANCELQUEUED, allocOpID(), victimID, &node->opID, sizeof(U32));
					else
						addToHostBuffer(CANCEL, allocOpID(), victimID, &node->opID, sizeof(U32));
				}
				else
					CQBOMB2("Removing object 0x%X with pending op #%d.", victimID, node->opID);
				onRemovalFromOp(part);		// cleared cached value for pending op
				recalcDependencies(node->opID);
			}
		}
		node = node->pNext;
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::updateSyncTick (void)
{
	const U32 currentTick = GetTickCount();
	if (syncTick == 0)
		syncTick = currentTick;

	int maxLoops = NUM_BINS;
	while (currentTick - syncTick > SYNC_PERIOD)
	{
		currentBin = (currentBin + 1) % NUM_BINS;
		syncBin[currentBin] = 0;
		opdataBin[currentBin] = 0;
		harvestBin[currentBin] = 0;
		syncTick += SYNC_PERIOD;
		if (--maxLoops<=0)
		{
			syncTick=0;
			break;
		}
	}

	syncThroughput = 0;
	opdataThroughput = 0;
	harvestThroughput = 0;

	maxLoops = NUM_BINS;
	while (maxLoops-- > 0)
	{
		syncThroughput += syncBin[maxLoops];
		opdataThroughput += opdataBin[maxLoops];
		harvestThroughput += harvestBin[maxLoops];
	}

	//
	// try to increase bandwidth usage
	//
	if (syncBackoffTick == 0)
		syncBackoffTick = currentTick;
	if (syncRandMask > DEFAULT_RAND_MASK && currentTick - syncBackoffTick > BACKOFF_PERIOD)
	{
		syncRandMask /= 2;
		syncBackoffTick = currentTick;
		if (CQFLAGS.bTraceMission != 0)
			OPPRINT1("OPAGENT::sync rate << = %X\n", syncRandMask);
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::receivePartRename (PARTRENAMEPACKET & packet)
{
	IBaseObject * obj = OBJLIST->FindObject(packet.partID);
	MPartNC ship = obj;

	if (ship.isValid())
	{
		_localWideToAnsi(packet.name, ship->partName, sizeof(ship->partName));
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::notifyOnUserStop (HOSTCMD hostcmd, U32 agentID, const ObjSet & set)
{
	OBJPTR<IMissionActor> actor;
	U32 i;
	
	for (i = 0; i < set.numObjects; i++)
	{
		IBaseObject * obj = OBJLIST->FindObject(set.objectIDs[i]);

		if (obj && obj->QueryInterface(IMissionActorID, actor))
		{
			actor->OnStopRequest(agentID);
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::setRallyPoint (U32 dwMissionID, const NETGRIDVECTOR & vector)
{
	IBaseObject * obj = OBJLIST->FindObject(dwMissionID);
	if (obj)
	{
		OBJPTR<IPlatform> platform;

		if (obj->QueryInterface(IPlatformID, platform))
		{
			platform->SetRallyPoint(vector);
		}
	}
}
//--------------------------------------------------------------------------//
//
BOOL32 OpAgent::Load (struct IFileSystem * inFile)
{
	reset();
	bMissionLoaded=true;

	BOOL32 result = 1;
	HANDLE hFile=INVALID_HANDLE_VALUE;
	DAFILEDESC fdesc = "lastOpID";
	DWORD dwRead, dwBufferSize, dwOffset;
	SUPERBASE_PACKET *node, *prev = 0;
	U8 *buffer=0;

	if (inFile->SetCurrentDirectory("\\OpAgent") == 0)
		goto Done;

	if ((hFile = inFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
		goto Done;

	inFile->ReadFile(hFile, &lastOpID, sizeof(lastOpID), &dwRead, 0);
	inFile->CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;
	//
	// process termination list
	//
	fdesc.lpFileName = "TerminationList";
	if ((hFile = inFile->OpenChild(&fdesc)) != INVALID_HANDLE_VALUE)
	{
		dwBufferSize = inFile->GetFileSize(hFile);
		buffer = (U8 *) malloc(dwBufferSize);
		inFile->ReadFile(hFile, buffer, dwBufferSize, &dwRead, 0);
		CQASSERT(dwRead==dwBufferSize);
		TERMINATION_NODE * prev = 0;

		dwOffset=0;
		while (dwOffset < dwBufferSize)
		{
			const RWTERMINATION_NODE * tnode = (const RWTERMINATION_NODE *) (buffer+dwOffset);
			TERMINATION_NODE * node = tnode->getNode();
			
			if (prev)
				prev->pNext = node;
			else
				pHitList = node;

			prev = node;
			dwOffset += sizeof(RWTERMINATION_NODE);
		}

		::free(buffer);
		inFile->CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
	//
	// process opcompletion list
	//
	fdesc.lpFileName = "CompletionList";
	if ((hFile = inFile->OpenChild(&fdesc)) != INVALID_HANDLE_VALUE)
	{
		dwBufferSize = inFile->GetFileSize(hFile);
		buffer = (U8 *) malloc(dwBufferSize);
		inFile->ReadFile(hFile, buffer, dwBufferSize, &dwRead, 0);
		CQASSERT(dwRead==dwBufferSize);
		OPCOMPLETE_NODE * prev = 0;

		dwOffset=0;
		while (dwOffset < dwBufferSize)
		{
			const RWCOMPLETION_NODE * tnode = (const RWCOMPLETION_NODE *) (buffer+dwOffset);
			OPCOMPLETE_NODE * node = tnode->getNode();
			
			if (prev)
				prev->pNext = node;
			else
				pCompleteList = node;

			prev = node;
			dwOffset += sizeof(RWCOMPLETION_NODE);
		}

		::free(buffer);
		inFile->CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	//
	// process recalc list
	//
	fdesc.lpFileName = "RecalcList";
	if ((hFile = inFile->OpenChild(&fdesc)) != INVALID_HANDLE_VALUE)
	{
		dwBufferSize = inFile->GetFileSize(hFile);
		buffer = (U8 *) malloc(dwBufferSize);
		inFile->ReadFile(hFile, buffer, dwBufferSize, &dwRead, 0);
		CQASSERT(dwRead==dwBufferSize);
		RECALCPARENT_NODE * prev = 0;

		dwOffset=0;
		while (dwOffset < dwBufferSize)
		{
			const RWRECALCPARENT_NODE * tnode = (const RWRECALCPARENT_NODE *) (buffer+dwOffset);
			RECALCPARENT_NODE * node = tnode->getNode();
			
			if (prev)
				prev->pNext = node;
			else
				pRecalcList = node;

			prev = node;
			dwOffset += sizeof(RWRECALCPARENT_NODE);
		}

		::free(buffer);
		inFile->CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	
	
	fdesc.lpFileName = "PendingOps";

	if ((hFile = inFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
		goto Done;

	dwBufferSize = inFile->GetFileSize(hFile);
	if (dwBufferSize!=0)
	{
		buffer = (U8 *) malloc(dwBufferSize);
		inFile->ReadFile(hFile, buffer, dwBufferSize, &dwRead, 0);
		CQASSERT(dwRead==dwBufferSize);

		dwOffset=0;
		while (dwOffset < dwBufferSize)
		{
			const RWPACKET_HEADER * header = (const RWPACKET_HEADER	*) (buffer+dwOffset);
			node = createSuperPacket(header);

			if (prev)
				prev->pNext = node;
			else
				pOpList = node;

			prev = node;
			dwOffset += header->size;
		}
	}

Done:
	if (CQFLAGS.bClientPlayback && NETOUTPUT!=0)
		startClientPlayback();
	onNewHost();

	result = 1;
	inFile->SetCurrentDirectory("\\");
	::free(buffer);
	if (hFile != INVALID_HANDLE_VALUE)
		inFile->CloseHandle(hFile);
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 OpAgent::Save (IFileSystem * outFile)
{
	BOOL32 result = 0;
	HANDLE hFile=INVALID_HANDLE_VALUE;
	DAFILEDESC fdesc = "lastOpID";
	DWORD dwWritten;
	char buffer[1024];

	outFile->CreateDirectory("\\OpAgent");
	if (outFile->SetCurrentDirectory("\\OpAgent") == 0)
		goto Done;

	RecursiveDelete(outFile);

	fdesc.dwDesiredAccess |= GENERIC_WRITE;
	fdesc.dwShareMode = 0;
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if ((hFile = outFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
		goto Done;

	outFile->WriteFile(hFile, &lastOpID, sizeof(lastOpID), &dwWritten, 0);
	outFile->CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;

	fdesc.lpFileName = "PendingOps";

	if ((hFile = outFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
		goto Done;

	{
		SUPERBASE_PACKET *node = pOpList;
		while (node)
		{
			RWPACKET_HEADER	* header = (RWPACKET_HEADER	*) buffer;

			header->size = sizeof(*header)+ node->getOperandSize() + ((node->getSet().numObjects-1) * sizeof(U32));
			header->cmd = node->hostCmd;
			header->opID = node->opID;
			header->groupID = node->groupID;
			header->bCancelDisabled = node->bCancelDisabled;
			header->bDelayDependency = node->bDelayDependency;
			header->bStalled =  node->bStalled;
			header->bLongTerm = node->bLongTerm;
			header->parentOp = node->parentOp;
			header->numObjects = node->getSet().numObjects;
			memcpy(header->objects, node->getSet().objectIDs, header->numObjects*sizeof(U32));
			memcpy(((U8*)header)+header->size-node->getOperandSize(), node->getOperands(), node->getOperandSize());
			
			if (outFile->WriteFile(hFile, header, header->size, &dwWritten, 0) == 0)
				goto Done;

			node = node->pNext;
		}
	}

	outFile->CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;

	if (pHitList)
	{
		fdesc.lpFileName = "TerminationList";

		if ((hFile = outFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
			goto Done;

		TERMINATION_NODE * node = pHitList;
		RWTERMINATION_NODE tnode;
		while (node)
		{
			tnode = *node;		// convert it to flat structure
			outFile->WriteFile(hFile, &tnode, sizeof(tnode), &dwWritten, 0);
			node = node->pNext;
		}

		outFile->CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	if (pCompleteList)
	{
		fdesc.lpFileName = "CompletionList";

		if ((hFile = outFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
			goto Done;

		OPCOMPLETE_NODE * node = pCompleteList;
		RWCOMPLETION_NODE tnode;
		while (node)
		{
			tnode = *node;		// convert it to flat structure
			outFile->WriteFile(hFile, &tnode, sizeof(tnode), &dwWritten, 0);
			node = node->pNext;
		}

		outFile->CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	if (pRecalcList)
	{
		fdesc.lpFileName = "RecalcList";

		if ((hFile = outFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
			goto Done;

		RECALCPARENT_NODE * node = pRecalcList;
		RWRECALCPARENT_NODE tnode;
		while (node)
		{
			tnode = *node;		// convert it to flat structure
			outFile->WriteFile(hFile, &tnode, sizeof(tnode), &dwWritten, 0);
			node = node->pNext;
		}

		outFile->CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	result = 1;
Done:
	outFile->SetCurrentDirectory("\\");
	if (hFile != INVALID_HANDLE_VALUE)
		outFile->CloseHandle(hFile);
	return result;
}
//--------------------------------------------------------------------------//
//
void OpAgent::ResetSyncItems (void)
{
	//
	// reset money
	//
	for(U32 i = 1; i < MAX_PLAYERS+1 ;++i)
	{
		netGas[i] = MGlobals::GetCurrentGas(i);
		netMetal[i] = MGlobals::GetCurrentMetal(i);
		netCrew[i] = MGlobals::GetCurrentCrew(i);
		netTotalComPts[i] = MGlobals::GetCurrentTotalComPts(i);
		netUsedComPts[i] = MGlobals::GetCurrentUsedComPts(i);
	}
	//
	// reset visibility
	//
	SUBPACKET_VISIBILITY vpacket;
	memset(vpacket.visibility, 0, sizeof(vpacket.visibility));
	vpacket.get();
	memcpy(netVisibility, vpacket.visibility, sizeof(netVisibility));
	//
	// reset alliance
	//
	SUBPACKET_ALLIANCE apacket;
	apacket.get();
	memcpy(netAlliance, apacket.alliance, sizeof(netAlliance));
	//
	// reset scores
	//
	U8 tmpBuffer[SCOREBUFFER_SIZE];
	U32 size;
	size = MGlobals::GetGameScores(tmpBuffer);
	CQASSERT(size <= sizeof(tmpBuffer));
	memcpy(scoreBuffer, tmpBuffer, size);		// update our local copy

	//
	// update the zone
	//
	if (ZONESCORE!=0)
	{
		int i;
		for (i = 1; i <= MAX_PLAYERS; i++)
		{
			U32 slots[MAX_PLAYERS+1];
			U32 numSlots;

			if ((numSlots = MGlobals::GetSlotIDForPlayerID(i, slots)) != 0)
			{
				U32 allyMask = MGlobals::GetAllyMask(i);
				U32 j;
				for (j = 0; j < numSlots; j++)
					ZONESCORE->SetPlayerTeam(MGlobals::GetZoneSeatFromSlot(slots[j]), allyMask);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
SUPERBASE_PACKET * OpAgent::createSuperPacket (const BASE_PACKET & packet, U32 baseSize, const U32 objectID[MAX_SELECTED_UNITS])
{
	SUPERBASE_PACKET * result;
	U32 size = baseSize + sizeof(ObjSet) + SUPERBASE_OVERHEAD;

	result = (SUPERBASE_PACKET *) calloc(size, 1);

	memcpy(result->castBasePacket(), &packet, baseSize);
	result->pSet = (ObjSet *) ( ((U8 *)result) + baseSize + SUPERBASE_OVERHEAD);
	
	U32 numObjects = (result->dwSize - baseSize) / sizeof(U32);
	CQASSERT2(numObjects > 0 && numObjects <= MAX_SELECTED_UNITS, "numObjects=%d, type=%d", numObjects, packet.type);
	
	if (numObjects==1 && MGlobals::GetPlayerFromPartID(objectID[0]) == MGlobals::GetGroupID())
	{
		IGroup * group = OBJLIST->FindGroupObject(objectID[0]);
		CQASSERT(group!=0);

		if (group)
			result->pSet->numObjects = group->GetObjects(result->pSet->objectIDs);
	}
	else // either single select or new group
	{
		result->pSet->numObjects = numObjects;
		memcpy(result->pSet->objectIDs, objectID, numObjects * sizeof(U32));
	}

	return result;
}
//--------------------------------------------------------------------------//
//
SUPERBASE_PACKET * OpAgent::duplicatePacket (SUPERBASE_PACKET *packet)
{
	SUPERBASE_PACKET * result;
	U32 size = ((U8 *) ((&packet->getSet())+1)) - ((U8 *)packet);
		
	result = (SUPERBASE_PACKET *) malloc(size);

	memcpy(result, packet, size);
	result->pNext = 0;
	result->pSet = (ObjSet *) (((U8 *)(result+1))+packet->getOperandSize());

	return result;
}
//--------------------------------------------------------------------------//
//
SUPERBASE_PACKET * OpAgent::createPacket (HOSTCMD cmd, U32 operandSize)
{
	SUPERBASE_PACKET * result;
	U32 size = sizeof(SUPERBASE_PACKET) + operandSize + sizeof(ObjSet);
		
	result = (SUPERBASE_PACKET *) calloc(size, 1);

	result->pSet = (ObjSet *) (((U8 *)(result+1))+operandSize);
	result->hostCmd = cmd;

	return result;
}
//--------------------------------------------------------------------------//
// add packet to the command list
//
void OpAgent::addPacket (SUPERBASE_PACKET * packet)
{
	SUPERBASE_PACKET *node = pCommandList, *prev=0;

	while (node)
	{
		prev = node;
		node = node->pNext;
	}
	//
	// append packet to the end of the list
	// 
	if (prev)
		prev->pNext = packet;
	else
		pCommandList = packet;
}
//--------------------------------------------------------------------------//
//
void OpAgent::reset (void)
{
	SUPERBASE_PACKET *node = pCommandList;

	while (node)
	{
		pCommandList = node->pNext;
		::free(node);
		node = pCommandList;
	}

	node = pOpList;
	while (node)
	{
		pOpList = node->pNext;
		::free(node);
		node = pOpList;
	}

	{
		TERMINATION_NODE * node = pHitList;

		while (node)
		{
			pHitList = node->pNext;
			delete node;
			node = pHitList;
		}
	}

	{
		OPCOMPLETE_NODE * node = pCompleteList;

		while (node)
		{
			pCompleteList = node->pNext;
			delete node;
			node = pCompleteList;
		}
	}

	{
		RECALCPARENT_NODE * node = pRecalcList;

		while (node)
		{
			pRecalcList = node->pNext;
			delete node;
			node = pRecalcList;
		}
	}

	//	randomizer.reset();
	memset(syncBin, 0, sizeof(syncBin));
	memset(opdataBin, 0, sizeof(opdataBin));
	memset(harvestBin, 0, sizeof(harvestBin));
	currentBin = syncThroughput = syncTick = 0;
	syncRandMask = DEFAULT_RAND_MASK;
	syncBackoffTick = 0;
	totalDataSent = 0;

	lastOpID = 0;
	bufferOffset = 0;

	for(U32 i = 1; i < MAX_PLAYERS+1 ;++i)
	{
		netGas[i] = MGlobals::GetCurrentGas(i);
		netMetal[i] = MGlobals::GetCurrentMetal(i);
		netCrew[i] = MGlobals::GetCurrentCrew(i);
		netTotalComPts[i] = MGlobals::GetCurrentTotalComPts(i);
		netUsedComPts[i] = MGlobals::GetCurrentUsedComPts(i);
	}

	memset(scoreBuffer, 0, sizeof(scoreBuffer));
	bRealMigration = bHostMigration = false;

	if (clientPlaybackBuffer)
		VirtualFree(clientPlaybackBuffer, 0, MEM_RELEASE);		// used for testing network code on client side
	clientPlaybackBuffer = 0;
	clientPlaybackBufferOffset = 0;

	memset(netVisibility, -1, sizeof(netVisibility));
	memset(netAlliance, -1, sizeof(netAlliance));
	bMissionLoaded=false;
}
//--------------------------------------------------------------------------//
//
SUPERBASE_PACKET * OpAgent::findQueuedDockCommand (U32 dwMissionID)
{
	SUPERBASE_PACKET *node = pOpList, *result=0;

	while (node)
	{
		if ((node->hostCmd == DOCKFLAGSHIP||node->hostCmd==UNDOCKFLAGSHIP) && node->getSet().contains(dwMissionID))
			result = node;
		node = node->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
// Filter dockship commands too. unit should issue an undock command before the dock command.
// This keeps the original dockship safely out of the new dockship operation.
//
void OpAgent::filterCommandsForAdmiral (SUPERBASE_PACKET * packet)
{
	if (packet->type != PT_USRBUILD && packet->type != PT_FORMATIONATTACK && packet->type != PT_USRFORMATIONMOVE && packet->type != PT_USRUNDOCKFLAGSHIP && packet->type != PT_FLEETDEF && packet->type != PT_ADMIRALTACTICCHANGE&& packet->type != PT_ADMIRALFORMATIONCHANGE)
	{
		ObjSet & set = packet->getSet();
		U32 i = set.numObjects;

		while (i-- > 0)
		{
			if (TESTADMIRAL(set.objectIDs[i]))
			{
				VOLPTR(IAdmiral) admiral = OBJLIST->FindObject(set.objectIDs[i]);

				if (admiral)
				{
					SUPERBASE_PACKET * pDock = findQueuedDockCommand(set.objectIDs[i]);
					HOSTCMD cmd = (pDock) ? pDock->hostCmd : NOTHING;
					bool bDocked = admiral->IsDocked() != 0;

					// let the command go through if 
					// 1) docked, but have a queued undock command, or
					// 2) undocked, and we don't have a queued dock command
					if (!((bDocked && cmd==UNDOCKFLAGSHIP) || (bDocked==0 && cmd!=DOCKFLAGSHIP)))
					{
						if (CQFLAGS.bTraceMission)
							OPPRINT2("CMD type %d filtered for admiral 0x%08X\r\n", packet->type, set.objectIDs[i]);

						if (packet->type == PT_USRDOCKFLAGSHIP)
							set.numObjects = 0;		// cancel the whole thing
						else
							set.removeObject(set.objectIDs[i]);
					}
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
// add packet to the op list
//
void OpAgent::sendCommand (SUPERBASE_PACKET * packet)
{
	//
	// cancel any previous commands for this group
	//
	SUPERBASE_PACKET *node = pOpList, *prev=0;

	// do not cancel previous commands if user bit is set
	if (packet->userBits!=0)
	{
		while (node)
		{
			prev = node;
			node = node->pNext;
		}

		if (isStanceLike(packet->type)==0 && packet->type != PT_USRBUILD && packet->type != PT_USRRALLY)
			packet->parentOp = findParentOp(packet->getSet());
	}
	else
	{
		ObjSet set;

		while (node)
		{
			set = node->getSet() & packet->getSet();

			if (set.numObjects > 0)
			{
				if (node->bCancelDisabled)
				{
					if (packet->type == PT_USRSTOP)
						notifyOnUserStop(node->hostCmd, node->opID, set);
					packet->getSet() -= set;
				}
				else
				if (node->hostCmd == JUMP && node->castJump()->bEnabled)
				{
					//
					// split the group, if possible
					//
					if (set.numObjects < packet->getTrueSetSize())		// some commands have extra members
					{
						SUPERBASE_PACKET * newpacket = duplicatePacket(packet);
						newpacket->getSet() -= set;		// = group that is already through the gate
						packet->getSet() = set;
						sendCommand(newpacket);			// this command should not get blocked
					}

					if (packet->parentOp==0)
						packet->parentOp = node->opID;
				}
				else
				{
					if (node->parentOp==0)	// is it active?
						notifySetofCancel(node->opID, set);
					sendCancel(node->opID, set, node->parentOp!=0);
					node->getSet() -= set;
					onRemovalFromOp(set);
					recalcDependencies(node->opID);
				}
			}

			prev = node;
			node = node->pNext;
		}
	}

	//
	// filter commands for docked admirals
	//
	filterCommandsForAdmiral(packet);

	if (packet->getSet().numObjects > 0)
		sendHostCommand(packet, prev);
	else
		::free(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::processCommands (void)
{
	//
	// process incoming commands
	//
	SUPERBASE_PACKET *node = pCommandList;

	while (node)
	{
		pCommandList = node->pNext;
		node->pNext = 0;

		validateSet(node->getSet());			// remove dead guys here!
		if (node->getSet().numObjects > 0)
		{
			sendCommand(node);
		}
		else
			::free(node);

		node = pCommandList;
	}

	if ((rand() & syncRandMask) == syncRandMask)
		sendSyncNuggets();
	sendSyncMessages();
	if ((rand() & syncRandMask) == syncRandMask)
		sendSyncMoney();
	sendSyncVisibility();
	sendSyncAlliance();
	if ((updateCounter & 63) == 0)		// update stats every 17 seconds
		sendSyncScores();
}
//--------------------------------------------------------------------------//
//
void OpAgent::processHitList (void)
{
	TERMINATION_NODE * node = pHitList;
	OBJPTR<IAttack> attacker;
	OBJPTR<IMissionActor> actor;
	IBaseObject * obj;

	while (node)
	{
		pHitList = node->pNext;
		node->pNext = 0;

		if ((obj = OBJLIST->FindObject(node->victimID)) != 0)
			if (obj->QueryInterface(IMissionActorID, actor))
			{
				// not the special code for "no explosion"
				if (node->attackerID != 0x80000000)
				{
					if ((obj = OBJLIST->FindObject(node->attackerID)) != 0)
						if (obj->QueryInterface(IAttackID, attacker))
						{
							attacker->ReportKill(node->victimID);
						}
				}
	
				//if this is not the second part of a jump plat
				bool bDontSendDead = false;
				if((actor.Ptr()->objClass == OC_PLATFORM) && (node->victimID &0x10000000))
				{
					VOLPTR(IJumpPlat) jumpPlat;
					if((jumpPlat=actor.Ptr()) != 0)
					{
						bDontSendDead = true;
					}
				}
				if(!bDontSendDead)
					MScript::RunProgramsWithEvent(CQPROGFLAG_OBJECTDESTROYED, node->victimID);

				EVENTSYS->Send(CQE_OBJECT_DESTROYED, (void *) node->victimID);

				actor->PreSelfDestruct();
				actor->SelfDestruct(node->attackerID != 0x80000000);
			
				removeFromOpList(node->victimID);

				if (IsMaster())
				{
					ObjSet set;
					set.numObjects = 1;
					set.objectIDs[0] = node->victimID;
					bufferedSend(ENABLEDEATH, allocOpID(), &node->attackerID, sizeof(node->attackerID), set);
				}

				if (CQFLAGS.bTraceMission != 0)
					OPPRINT2("Unit 0x%X terminated by 0x%X\n", node->victimID, node->attackerID);
			}

		delete node;
		node = pHitList;
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::processOperationCompletion (void)
{
	OPCOMPLETE_NODE * node = pCompleteList, *pNext;
	pCompleteList = 0;		// watch out for recursion

	while (node)
	{
		pNext = node->pNext;
		
		operationCompleted(node->agentID, node->dwMissionID);

		delete node;
		node = pNext;
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::processRecalculation (void)
{
	RECALCPARENT_NODE * node = pRecalcList, *pNext;
	pRecalcList = 0;		// watch out for recursion

	while (node)
	{
		pNext = node->pNext;

		SUPERBASE_PACKET * packet = findOp(node->opID);
		if (packet && packet->getSet().numObjects)
		{
			if (packet->parentOp)
			{
				CQASSERT(packet->isUserData()==false && packet->hostCmd != ADDTO);
				packet->parentOp = packet->opID;		// force a recalculation of THIS operation as well
			}
			onOpCompleted(packet, node->opID);
		}
		
		delete node;
		node = pNext;
	}
}
//--------------------------------------------------------------------------//
// if we have extra bandwidth, send sync packets if needed
//
void OpAgent::sendSyncMessages (void)
{
	IBaseObject * obj = OBJLIST->GetTargetList();
	VOLPTR(IMissionActor) actor;
	char buffer[256];
	ObjSet set;
	int dataSent=0;

	if (NETPACKET->TestLowPrioritySend(150) == 0)
	{
		if (syncRandMask < MAX_RAND_MASK && CQFLAGS.bTraceMission != 0)
			OPPRINT1("OPAGENT::sync rate <<. = %X\n", syncRandMask);
		syncRandMask = MAX_RAND_MASK;	// dramatically reduce amount of sync data
		syncBackoffTick = GetTickCount();
	}

	while (obj)
	{
		set.numObjects = 1;
		set.objectIDs[0] = obj->GetPartID();
		CQASSERT(set.objectIDs[0] != 0);
		if ((syncRandMask & rand()) == syncRandMask)
		{
			MPartNC part = obj;
			SUPERBASE_PACKET * node = findFirstAgent(part, set.objectIDs[0]);

			// do not perform updates for units that are jumping
			if (node==0 || node->hostCmd != JUMP || node->parentOp!=0)
			{
				if ((actor = obj) != 0)
				{
					int size;

					if ((size = actor->GetGeneralSyncData(buffer)) != 0)
					{
						CQASSERT(size+sizeof(U32) <= sizeof(buffer));

						U32 parentOp = 0;
						bool bNoDelayAction = false;
						
						if (node==0 || node->parentOp != 0)		// object has not yet started next command
						{
							parentOp = part->lastOpCompleted;
						}
						else
						{
							bNoDelayAction = true;		// action in progress
							parentOp = node->opID;		// current active op?

							// add the members of the process to the set, so that client will receive 
							// this packet at the right time
							if (node->isProcess())
							{
								set += node->getSet();
								CQASSERT(set.objectIDs[0] == obj->GetPartID());
							}
						}

						U32 oldTotal = totalDataSent;
						((U32 *)(buffer+size))[0] = parentOp;
						bufferedSend((bNoDelayAction) ? OBJSYNC : OBJSYNC2, allocOpID(), buffer, size+sizeof(U32), set);

						if (CQFLAGS.bTraceMission)
							OPPRINT5("Sending %s for unit 0x%X, #%d->#%d [%d bytes]\n", (bNoDelayAction) ? "OBJSYNC" : "OBJSYNC2",
									set.objectIDs[0], lastOpID, parentOp, size+sizeof(U32));
						
						U32 dataAmount = totalDataSent - oldTotal;
						syncBin[currentBin] += dataAmount;		// update sync data throughput
						syncThroughput += dataAmount;
						dataSent += dataAmount;
					}
				}
			}
		}

		obj = obj->nextTarget;
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendSyncMoney (void)
{
	SUBPACKET_MONEY mpacket;
	mpacket.get(netGas,netMetal,netCrew,netTotalComPts,netUsedComPts);

	// is update needed? (have we sent packet before, has money values changed?)
	if (mpacket.hasValue())
	{
		// test for bandwidth or shutdown
		if (CQFLAGS.bGameActive==0 || NETPACKET->TestLowPrioritySend(bufferOffset+sizeof(mpacket)) > sizeof(mpacket))
		{
			U32 oldTotal = totalDataSent;
			addToHostBuffer(SYNCMONEY, allocOpID(), 0, &mpacket, sizeof(mpacket));
			U32 dataAmount = totalDataSent - oldTotal;
			syncBin[currentBin] += dataAmount;		// update sync data throughput
			syncThroughput += dataAmount;

			mpacket.updateNetValues(netGas,netMetal,netCrew,netTotalComPts,netUsedComPts);
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendSyncVisibility (void)
{
	SUBPACKET_VISIBILITY vpacket;
	vpacket.get();

	// is update needed? (have we sent packet before, has visibility values changed?)
	if (vpacket != netVisibility)
	{
		// test for bandwidth or shutdown
		if (CQFLAGS.bGameActive==0 || NETPACKET->TestLowPrioritySend(bufferOffset+sizeof(vpacket)) > sizeof(vpacket))
		{
			U32 oldTotal = totalDataSent;
			addToHostBuffer(SYNCVISIBILITY, allocOpID(), 0, &vpacket, sizeof(vpacket));
			U32 dataAmount = totalDataSent - oldTotal;
			syncBin[currentBin] += dataAmount;		// update sync data throughput
			syncThroughput += dataAmount;

			memcpy(netVisibility, vpacket.visibility, sizeof(netVisibility));
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendSyncAlliance (void)
{
	SUBPACKET_ALLIANCE apacket;
	apacket.get();

	// is update needed? (have we sent packet before, has visibility values changed?)
	if (apacket != netAlliance)
	{
		// test for bandwidth or shutdown
		if (CQFLAGS.bGameActive==0 || NETPACKET->TestLowPrioritySend(bufferOffset+sizeof(apacket)) > sizeof(apacket))
		{
			U32 oldTotal = totalDataSent;
			addToHostBuffer(SYNCALLIANCE, allocOpID(), 0, &apacket, sizeof(apacket));
			U32 dataAmount = totalDataSent - oldTotal;
			syncBin[currentBin] += dataAmount;		// update sync data throughput
			syncThroughput += dataAmount;
			memcpy(netAlliance, apacket.alliance, sizeof(netAlliance));
			//
			// update the zone
			//
			if (ZONESCORE!=0)
			{
				int i;
				for (i = 1; i <= MAX_PLAYERS; i++)
				{
					U32 slots[MAX_PLAYERS+1];
					U32 numSlots;

					if ((numSlots = MGlobals::GetSlotIDForPlayerID(i, slots)) != 0)
					{
						U32 allyMask = MGlobals::GetAllyMask(i);
						U32 j;
						for (j = 0; j < numSlots; j++)
							ZONESCORE->SetPlayerTeam(MGlobals::GetZoneSeatFromSlot(slots[j]), allyMask);
					}
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendSyncScores (void)
{
	U8 tmpBuffer[SCOREBUFFER_SIZE];
	U32 size;

	// is update needed?
	size = MGlobals::GetGameScores(tmpBuffer);
	CQASSERT(size <= sizeof(tmpBuffer));

	if (memcmp(tmpBuffer, scoreBuffer, size) != 0)	// data changed?
	{
		// test for bandwidth or shutdown
		if (CQFLAGS.bGameActive==0 || NETPACKET->TestLowPrioritySend(bufferOffset+size) > S32(size))
		{
			U32 oldTotal = totalDataSent;
			addToHostBuffer(SYNCSCORES, allocOpID(), 0, tmpBuffer, size);
			U32 dataAmount = totalDataSent - oldTotal;
			syncBin[currentBin] += dataAmount;		// update sync data throughput
			syncThroughput += dataAmount;

			memcpy(scoreBuffer, tmpBuffer, size);		// update our local copy
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendSyncStats (void)
{
	U32 i;
	U8 buffer[500];
	SYNCSTATS * pPacket = (SYNCSTATS *) buffer;

	for (i = 1; i <= MAX_PLAYERS; i++)
	{
		if (MGlobals::IsPlayerInGame(i))
		{
			pPacket->SYNCSTATS::SYNCSTATS();		// construct the packet
			U32 statSize = MGlobals::GetGameStats(i, pPacket->stats);
			pPacket->dwSize = sizeof(SYNCSTATS) + statSize;
			CQASSERT(pPacket->dwSize <= 500);
			pPacket->playerID = i;
			pPacket->updateCount = MGlobals::GetUpdateCount();

			NETPACKET->Send(0, NETF_ALLREMOTE, pPacket);
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendSyncNuggets (void)
{
	char buffer[256];
	U32 dataSize = NUGGETMANAGER->GetSyncData(buffer);
	CQASSERT(dataSize <= sizeof(buffer));

	if (dataSize)
	{
		U32 oldTotal = totalDataSent;
		addToHostBuffer(SYNCNUGGETS, allocOpID(), 0, buffer, dataSize);
		U32 dataAmount = totalDataSent - oldTotal;
		syncBin[currentBin] += dataAmount;		// update sync data throughput
		syncThroughput += dataAmount;
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::Update (void)
{
	if (CQFLAGS.bGameActive)
	{
		updateCounter++;

		updateOpList();

		processOperationCompletion();

		processRecalculation();		// recalculate dependencies for operations that have partially completed
		
		// process the hitlist after the data list so that data/process packets can be 
		// handled prior to destruction.
		processHitList();

		if (HOSTID == PLAYERID)
		{
			if (clientPlaybackBuffer)
				updateClientPlayback();
			if (testMigrationState()==false)		// return new migration state
			{
				processCommands();
			}
			flushSendBuffer();
		}
	}
}
//--------------------------------------------------------------------------//
// assumes playback buffer is valid
//
void OpAgent::updateClientPlayback (void)
{
	// offset the time by 15 seconds +/-
	U32 currentCount = (CQFLAGS.bForceEarlyDelivery) ? (30 * 15) : 0;
	const U32 * pNewCount;
	if (CQFLAGS.bForceLateDelivery)
		currentCount -= (30 * 15);
	currentCount += MGlobals::GetUpdateCount();

	// see if we are playing back at a different speed
	S32 speed = DEFAULTS->GetDefaults()->gameSpeed;
	if (DEFAULTS->GetDefaults()->bConstUpdateRate)
		speed = 0;
	S32 origSpeed = MGlobals::GetGameSettings().gameSpeed;

	if (speed != origSpeed)
	{
		// current / old
		double mul = pow(2, (speed * (1/5.0))) / pow(2, (origSpeed * (1/5.0)));
		currentCount = F2LONG(S32(currentCount) * mul);
	}

	pNewCount = (const U32 *) (((char *)clientPlaybackBuffer) + clientPlaybackBufferOffset);

	while (pNewCount[0] < currentCount && S32(currentCount)>0)
	{
		BASE_PACKET * packet;
		packet = (BASE_PACKET *) (((char *)clientPlaybackBuffer) + clientPlaybackBufferOffset + sizeof(U32));

		clientReceivePacket(packet);

		clientPlaybackBufferOffset += sizeof(U32) + packet->dwSize;	// skip to the next packet
		pNewCount = (const U32 *) (((char *)clientPlaybackBuffer) + clientPlaybackBufferOffset);
	}

	if (S32(pNewCount[0]) == -1)		// end of the buffer reached
	{
		VirtualFree(clientPlaybackBuffer, 0, MEM_RELEASE);
		SCROLLTEXT->SetTextString(L"[DEBUG] Playback buffer exhausted!");
		clientPlaybackBuffer = 0;
		clientPlaybackBufferOffset = 0;
		onNewHost();
	}
}
//--------------------------------------------------------------------------//
//
U32 OpAgent::GetNumAgents (void) const
{
	SUPERBASE_PACKET *node = pOpList;
	U32 result = 0;

	while (node)
	{
		result++;
		node = node->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void OpAgent::FlushSyncData (void)
{
	CQASSERT(MGlobals::IsHost());

	sendSyncStats();
	sendSyncMoney();
	sendSyncVisibility();
	sendSyncAlliance();
	flushSendBuffer();
}
//--------------------------------------------------------------------------//
// return true if on host machine AND there no pending ops from old host
// if dwMissionID==0, see if there are ANY pending ops from old host
//
bool OpAgent::IsMaster (void)
{
	return (bRealMigration==false && MGlobals::IsHost()); 
}
//--------------------------------------------------------------------------//
// find a ENABLEDEATH packet in the queue
//
bool OpAgent::HasPendingDeath (U32 dwMissionID)
{
	SUPERBASE_PACKET *node = pOpList;

	while (node)
	{
		if (node->hostCmd == ENABLEDEATH && node->getSet().contains(dwMissionID))
			break;
		node = node->pNext;
	}

	return (node != 0);
}
//--------------------------------------------------------------------------//
//
void OpAgent::SwitchPendingDeath (U32 oldMissionID, U32 newMissionID)
{
	CQASSERT(IsMaster()==0);

	SUPERBASE_PACKET *node = pOpList;

	while (node)
	{
		if (node->hostCmd == ENABLEDEATH && node->getSet().contains(oldMissionID))
		{
			node->getSet().removeObject(oldMissionID);
			node->getSet().objectIDs[node->getSet().numObjects++] = newMissionID;
			if (CQFLAGS.bTraceMission)
				OPPRINT3("Switching ENABLEDEATH #%d, 0x%X -> 0x%X\n", node->opID, oldMissionID, newMissionID);
			break;
		}
		node = node->pNext;
	}
}
//--------------------------------------------------------------------------//
//
U32 OpAgent::GetDataThroughput (U32 * pSyncMask, U32 * pSyncData, U32 * pOpData, U32 * pHarvest)
{
	if (pSyncMask)
		*pSyncMask = syncRandMask;
	if (pSyncData)
		*pSyncData = syncThroughput;
	if (pOpData)
		*pOpData = opdataThroughput;
	if (pHarvest)
		*pHarvest = harvestThroughput;
	return totalDataSent;
}
//--------------------------------------------------------------------------//
// find a PROCESS packet in the queue
//
U32 OpAgent::findPendingBirth (U32 dwMissionID)
{
	SUPERBASE_PACKET *node = pOpList;
	U32 result = 0;

	while (node)
	{
		if ((node->isProcess() || node->hostCmd==ADDTO) && node->getSet().contains(dwMissionID))
		{
			// if process has started, but target doesn't exist, there is a likely queued ADDTO
			// process should not have started yet, or our target should exist!?
			if (node->parentOp!=0)
			{
				result = node->opID;
				break;
			}
		}
		node = node->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void OpAgent::SignalStaleGroupObject (U32 dwGroupID)
{
	CQASSERT(IsMaster());
	addToHostBuffer(DELETEGROUP, allocOpID(), dwGroupID, 0, 0);
}
//--------------------------------------------------------------------------//
//
bool OpAgent::HasPendingOp (U32 dwMissionID)
{
	return (findFirstAgent(dwMissionID) != 0);
}
//--------------------------------------------------------------------------//
//
bool OpAgent::HasPendingOp (IBaseObject * obj)
{
	MPartNC part = obj;

	if (part.isValid())
	{
		if (part->bPendingOpValid)
			return part->pendingOp != 0;
		else
			return (findFirstAgent(part, part->dwMissionID) != 0);
	}
	return false;
}
//--------------------------------------------------------------------------//
//
U32 OpAgent::GetWorkingOp (IBaseObject * obj)
{
	SUPERBASE_PACKET * node = findFirstAgent(obj, obj->GetPartID());
	U32 result = 0;

	if (node)
	{
		if (node->parentOp == 0)		// is it running!?
		{
			result = node->opID;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void OpAgent::FlushOpQueueForUnit (IBaseObject * obj)
{
	CQASSERT(IsMaster());
	
	removeFromOpList(obj->GetPartID());
}
//--------------------------------------------------------------------------//
//
bool OpAgent::HasActiveJump (const struct ObjSet & set)
{
	SUPERBASE_PACKET *node = pOpList;
	bool result = 0;

	while (node)
	{
		if (node->hostCmd == JUMP)
		{
			if (node->parentOp == 0 && node->castJump()->bEnabled == true && (node->getSet() & set).numObjects>0)
			{
				result = true;
				break;
			}
		}
		node = node->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
bool OpAgent::EnumerateQueuedMoveAgents (const struct ObjSet & set, IAgentEnumerator * callback)
{
	SUPERBASE_PACKET *node = pOpList;
	bool result = true;
	IAgentEnumerator::NODE anode;

	while (node)
	{
		if (node->hostCmd == MOVETO)
		{
			if (set == node->getSet())
			{
				anode.opID = node->opID;
				anode.hostCmd = node->hostCmd;
				anode.pSet = & node->getSet();
				anode.targetPosition = node->castMove()->position;
				anode.targetSystemID = node->castMove()->position.systemID;
				anode.dwFabArchetype = 0;

				if ((result = callback->EnumerateAgent(anode)) == false)
					break;
			}
		}
		else
		if (node->hostCmd == PREJUMP)
		{
			if (set == node->getSet())
			{
				HSTPREJUMP * prejump = node->castPreJump();
				IBaseObject * gate = OBJLIST->FindObject(prejump->jumpgateID);

				if (gate)
				{
					anode.opID = node->opID;
					anode.hostCmd = node->hostCmd;
					anode.pSet = & node->getSet();
					anode.targetPosition = gate->GetTransform().translation;
					anode.targetSystemID = gate->GetSystemID();
					anode.dwFabArchetype = 0;

					if ((result = callback->EnumerateAgent(anode)) == false)
						break;
				}
			}

		}
		node = node->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
bool OpAgent::EnumerateFabricateAgents (U32 playerID, IAgentEnumerator * callback)
{
	SUPERBASE_PACKET *node = pOpList;
	bool result = true;
	IAgentEnumerator::NODE anode;
	anode.targetPosition.zero();

	while (node)
	{
		if (node->hostCmd == FABRICATE || node->hostCmd == FABRICATE_POS || node->hostCmd == FABRICATE_JUMP)
		{
			if ((PLAYERID_MASK & node->getSet().objectIDs[0]) == playerID)
			{
				anode.opID = node->opID;
				anode.hostCmd = node->hostCmd;
				anode.pSet = & node->getSet();
				anode.targetSystemID = node->getDestSystemID();
				anode.dwFabArchetype = node->getFabArchetype();
				if(node->hostCmd == FABRICATE)
				{
					anode.fabSlotID = node->getFabSlotID();
					anode.fabTargetID = node->getFabTargetID();
				}
				else if(node->hostCmd == FABRICATE_POS)
				{
					anode.fabTargetID = 0;
					anode.fabSlotID = 0;
					anode.targetPosition = node->getFabPosition();
				}
				else
				{
					anode.fabTargetID = node->getFabTargetID();
					anode.fabSlotID = 0;
				}

				if ((result = callback->EnumerateAgent(anode)) == false)
					break;
			}
		}
		node = node->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
bool OpAgent::IsFabricating (IBaseObject * obj)
{
	SUPERBASE_PACKET * node = findFirstAgent(obj, obj->GetPartID());

	if (node && node->parentOp==0 && node->bCancelDisabled)
	{
		switch (node->hostCmd)
		{
		case FABRICATE:
		case FABRICATE_POS:
		case FABRICATE_JUMP:
			return true;
		}
	}

	return false;
}
//--------------------------------------------------------------------------//
//
bool OpAgent::IsNotInterruptable (IBaseObject * obj)
{
	SUPERBASE_PACKET * node = findFirstAgent(obj, obj->GetPartID());

	return (node!=0 && node->parentOp==0 && node->bCancelDisabled);
}
//--------------------------------------------------------------------------//
//
bool OpAgent::GetOperationSet (U32 agentID, const struct ObjSet ** ppSet)
{
	SUPERBASE_PACKET * node = findOp(agentID);

	if (node)
	{
		*ppSet = &node->getSet();
		return true;
	}
	else
		*ppSet = 0;

	return false;
}
//--------------------------------------------------------------------------//
//
void OpAgent::TerminatePlayerUnits (U32 playerID)
{
	CQASSERT(IsMaster());
	IBaseObject * obj = OBJLIST->GetTargetList();

	CQFLAGS.bInsidePlayerResign = 1;

	if (CQFLAGS.bTraceMission)
		OPPRINT1("Terminating player %d's units...\r\n", playerID);

	while (obj)
	{
		const U32 dwMissionID = obj->GetPartID();
		// exclude child objects from destruction
		if ((dwMissionID & (PLAYERID_MASK|SUBORDID_MASK)) == playerID)
		{
			objectTerminated(obj->GetPartID(), 0);
		}

		obj = obj->nextTarget;
	}

	processHitList();
	flushSendBuffer();

	CQFLAGS.bInsidePlayerResign = 0;
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendCancel (U32 opID, const ObjSet & cancelSet, bool bQueuedCancel)
{
	onRemovalFromOp(cancelSet);
	bufferedSend(bQueuedCancel ? CANCELQUEUED : CANCEL, allocOpID(), &opID, sizeof(opID), cancelSet);
}
//--------------------------------------------------------------------------//
// called on load(), or we become the host
//
void OpAgent::enablePendingJumps (void)
{
	SUPERBASE_PACKET *node = pOpList;

	// remove ops that have completed
	while (node)
	{
		if (node->hostCmd == JUMP)
		{
			if (node->parentOp == 0 && node->castJump()->bEnabled == false)
				doCommand(node);
		}
		node = node->pNext;
	}
}
//--------------------------------------------------------------------------//
//
SUPERBASE_PACKET * OpAgent::findLastJump (U32 dwMissionID)
{
	SUPERBASE_PACKET *node = pOpList, *result=0;

	while (node)
	{
		if (node->hostCmd == JUMP)
		{
			if (node->getSet().contains(dwMissionID))
				result = node;
		}
		node = node->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
// find the first non-data packet for unit, skipping data packets if there are any.
//
SUPERBASE_PACKET * OpAgent::findFirstAgent (const MPartNC & part, U32 dwMissionID)
{
	SUPERBASE_PACKET *node = 0;

	if (part.isValid() && part->bPendingOpValid)
	{
		if (part->pendingOp)
		{
			node = findOp(part->pendingOp);
			CQASSERT(node);
			CQASSERT(node->getSet().contains(part->dwMissionID));
		}
	}
	else
	{
		node = pOpList;
		while (node)
		{
			if (node->isUserData() == false && node->getSet().contains(dwMissionID))
				break;

			node = node->pNext;
		}

		if (part.isValid())
		{
			if (node && node->isUserData() == false)
			{
				part->pendingOp = node->opID;
				part->bPendingOpValid = true;
			}
			else
			{
				// don't record data as a pending op
				part->pendingOp = 0;
				part->bPendingOpValid = true;
				node = 0;
			}
		}
	}

	return node;
}
//--------------------------------------------------------------------------//
//
void OpAgent::notifySetofCancel (U32 agentID, const ObjSet & _set)
{
	OBJPTR<IMissionActor> actor;
	U32 i;
	const ObjSet set = _set;		// defensive coding, in case user tries to complete the Op
	onRemovalFromOp(set);
	
	for (i = 0; i < set.numObjects; i++)
	{
		IBaseObject * obj = OBJLIST->FindObject(set.objectIDs[i]);
		if (obj)
		{
			if (obj->QueryInterface(IMissionActorID, actor))
				actor->OnOperationCancel(agentID);
		}
	}
}
//--------------------------------------------------------------------------//
// everyone is in the same system, might still need a jumpgate
// packet->hostCmd may be MOVETO or AOEATTACK, or FABRICATE, or HARVEST, or ESCORT, or DOCKFLAGSHIP, or UNDOCKFLAGSHIP
//
void OpAgent::sendHostMoveCommandFromSystem (SUPERBASE_PACKET * packet, U32 systemID, U32 playerID)
{
	const U32 destSystemID = packet->getDestSystemID();
	SUPERBASE_PACKET * prev = 0, *node = pOpList;

	while (node)
	{
		prev = node;
		node = node->pNext;
	}

	if (destSystemID == 0)		// invalid destination (e.g. harvest target is gone)
	{
		::free(packet);
		packet = 0;
	}
	else
	if (destSystemID == systemID)
	{
		if (packet->hostCmd != HOSTJUMP)
		{
			//
			// simple case: in-system move
			// 
			packet->opID = allocOpID();
			packet->groupID = bufferedSend(packet->hostCmd, packet->opID, packet->getOperands(), packet->getOperandSize(), packet->getSet(),packet->ctrlBits);
			appendToOpList(packet, prev);
			if (packet->parentOp==0)
				doCommand(packet);		// if invalid, clears the set
		}
		else
		{
			::free(packet);
			packet = 0;
		}
	}
	else
	{
		//
		// else we need to use a jumpgate first
		//
		U32 list[16];
		U32 i=0;				// allow start system==0 to fail silently (fleet probe)
		U32 pathLength = (systemID==0) ? 0xFFFFFFFF : SECTOR->GetShortestPath(systemID, destSystemID, list, playerID);
		const U32 origNumMembers = packet->getSet().numObjects;

		packet->getSet().numObjects = packet->getTrueSetSize();			// some sets contain extra members

		if (pathLength == 0xFFFFFFFF)	// path is invalid, disregard command
		{
			::free(packet);
			packet = 0;
		}
		else
		{
			CQASSERT(pathLength <= 16);
			U32 parentOp = packet->parentOp;

			while (i+1 < pathLength)
			{
				IBaseObject * obj = OBJLIST->FindObject(packet->getSet().objectIDs[0]);
				IBaseObject * startObj = SECTOR->GetJumpgateTo(list[i], list[i+1],obj->GetPosition());
				SUPERBASE_PACKET * newpacket = createPacket(PREJUMP, sizeof(HSTPREJUMP)-sizeof(BASE_PACKET));
				HSTPREJUMP * pre = newpacket->castPreJump();
			
				CQASSERT(startObj->GetSystemID() == list[i]);

				pre->jumpgateID = startObj->GetPartID();
				pre->bUserInitiated = 1;
				pre->ctrlBits = packet->castBasePacket()->ctrlBits;
				newpacket->parentOp = parentOp;
				newpacket->opID = allocOpID();
				newpacket->getSet() = packet->getSet();
				newpacket->groupID = bufferedSend(PREJUMP, newpacket->opID, newpacket->getOperands(), newpacket->getOperandSize(), newpacket->getSet(),newpacket->ctrlBits);
				appendToOpList(newpacket, prev);
				if (newpacket->parentOp==0)	// did original packet have a parent?
					doCommand(newpacket);		// if invalid, clears the set
				prev = newpacket;
				parentOp = newpacket->opID;
				//
				// now we need to use the jumpgate
				//
				newpacket = createPacket(JUMP, sizeof(HSTJUMP)-sizeof(BASE_PACKET));
				HSTJUMP * jump = newpacket->castJump();
				jump->jumpgateID = startObj->GetPartID();
				newpacket->parentOp = parentOp;
				newpacket->opID = allocOpID();
				newpacket->getSet() = packet->getSet();
				newpacket->groupID = bufferedSend(JUMP, newpacket->opID, newpacket->getOperands(), newpacket->getOperandSize(), newpacket->getSet(),newpacket->ctrlBits);
				appendToOpList(newpacket, prev);
				prev = newpacket;
				parentOp = newpacket->opID;

				i++;
			}

			if (packet->hostCmd != HOSTJUMP)
			{
				//
				// simple case: in-system move
				// 
				packet->getSet().numObjects = origNumMembers;			// set should include the target in the command
				packet->parentOp = parentOp;
				packet->opID = allocOpID();
				packet->groupID = bufferedSend(packet->hostCmd, packet->opID, packet->getOperands(), packet->getOperandSize(), packet->getSet(),packet->ctrlBits);
				appendToOpList(packet, prev);
			}
			else
			{
				::free(packet);
				packet = 0;
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostFormationMoveCommand (SUPERBASE_PACKET * packet)
{
	USRFORMATIONMOVE * move = packet->castFormationMove();
	IBaseObject * obj;
	ObjSet & set = packet->getSet();

	CQASSERT(set.numObjects == 1);

	if ((obj = OBJLIST->FindObject(set.objectIDs[0])) != 0)
	{
		OBJPTR<IAdmiral> admiral;
		if (obj->QueryInterface(IAdmiralID, admiral))
		{
			admiral->HandleMoveCommand(move->position);
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostFormationAttackCommand (SUPERBASE_PACKET * packet)
{
	FORMATIONATTACK * move = packet->castFormationAttack();
	IBaseObject * obj;
	ObjSet & set = packet->getSet();

	CQASSERT(set.numObjects == 1);

	if ((obj = OBJLIST->FindObject(set.objectIDs[0])) != 0)
	{
		OBJPTR<IAdmiral> admiral;
		if (obj->QueryInterface(IAdmiralID, admiral))
		{
			admiral->HandleAttackCommand(move->targetID,move->destSystemID);
		}
	}
}
//--------------------------------------------------------------------------//
// packet->hostCmd may be MOVETO or AOEATTACK or FABRICATE, or HARVEST, or ESCORT,
//
void OpAgent::sendHostMoveCommand (SUPERBASE_PACKET * packet)
{
	// opID is not assigned yet!

	//
	// subdivide group into each system, do move op on the subgroup
	//

	//
	// is group spread out over multiple systems?
	//
	U32 i;
	U32 systemID=0;
	U32 playerID=0;
	ObjSet set = packet->getSet();
	ObjSet newset;
	IBaseObject * obj;

	CQASSERT(set.numObjects!=0);

	while (set.numObjects)
	{
		systemID = newset.numObjects = 0;
		// find a system to compare with
		while (set.numObjects > 0)
		{
			obj = OBJLIST->FindObject(set.objectIDs[0]);
			if (obj)
			{
				systemID = getRealSystemID(obj);
				playerID = obj->GetPlayerID();
				newset.objectIDs[newset.numObjects++] = set.objectIDs[0];
				set.removeObject(set.objectIDs[0]);
				break;
			}
			else
				set.removeObject(set.objectIDs[0]);
		}
		// now move everyone into new set who matches this systemID
		i = 0;
		while (i < set.numObjects)
		{
			obj = OBJLIST->FindObject(set.objectIDs[i]);
			if (obj)
			{
				if (systemID == getRealSystemID(obj))
				{
					newset.objectIDs[newset.numObjects++] = set.objectIDs[i];
					set.removeObject(set.objectIDs[i]);
				}
				else
					i++;
			}
			else
				set.removeObject(set.objectIDs[i]);
		}
	
		if (newset.numObjects == 0)
		{
			CQASSERT(set.numObjects==0);
			::free(packet);
		}
		else	// simple case, everyone is in same system
		if (newset.numObjects == packet->getSet().numObjects)
		{
			sendHostMoveCommandFromSystem(packet, systemID, playerID);
		}
		else
		if (set.numObjects)  // there are more items left
		{
			SUPERBASE_PACKET * newpacket = duplicatePacket(packet);
			newpacket->getSet() = newset;
			sendHostMoveCommandFromSystem(newpacket, systemID, playerID);
		}
		else // no more items left, reuse original packet
		{
			packet->getSet() = newset;
			sendHostMoveCommandFromSystem(packet, systemID, playerID);
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostBuildCommand (SUPERBASE_PACKET * packet)
{
	USRBUILD * build = packet->castBuild();
	IBaseObject * obj;
	ObjSet & set = packet->getSet();

	CQASSERT(set.numObjects == 1);

	if ((obj = OBJLIST->FindObject(set.objectIDs[0])) != 0)
	{
		OBJPTR<IBuildQueue> fab;
		if (obj->QueryInterface(IBuildQueueID, fab))
		{
			fab->BuildQueue(build->dwArchetypeID, static_cast<IBuildQueue::COMMANDS>(build->cmd));
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostStanceChange (SUPERBASE_PACKET * packet)
{
	STANCE_PACKET * stance = packet->castStance();
	IBaseObject * obj;
	ObjSet & set = packet->getSet();
	OBJPTR<IAttack> attack;
	U32 i;

	for (i = 0; i < set.numObjects; i++)
	{
		if ((obj = OBJLIST->FindObject(set.objectIDs[i])) != 0)
		{
			if (obj->QueryInterface(IAttackID, attack))
			{
				attack->SetUnitStance(stance->stance);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostSupplyStanceChange (SUPERBASE_PACKET * packet)
{
	SUPPLY_STANCE_PACKET * stance = packet->castSupplyStance();
	IBaseObject * obj;
	ObjSet & set = packet->getSet();
	OBJPTR<ISupplier> supplier;
	U32 i;

	for (i = 0; i < set.numObjects; i++)
	{
		if ((obj = OBJLIST->FindObject(set.objectIDs[i])) != 0)
		{
			if (obj->QueryInterface(ISupplierID, supplier))
			{
				supplier->SetSupplyStance(stance->stance);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostHarvestStanceChange (SUPERBASE_PACKET * packet)
{
	HARVEST_STANCE_PACKET * stance = packet->castHarvestStance();
	IBaseObject * obj;
	ObjSet & set = packet->getSet();
	OBJPTR<IHarvestBuilder> hBuilder;
	U32 i;

	for (i = 0; i < set.numObjects; i++)
	{
		if ((obj = OBJLIST->FindObject(set.objectIDs[i])) != 0)
		{
			if (obj->QueryInterface(IHarvestBuilderID, hBuilder))
			{
				hBuilder->SetHarvestStance(stance->stance);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostHarvestAutoModeChange (SUPERBASE_PACKET * packet)
{
	HARVEST_AUTOMODE_PACKET * stance = packet->castHarvestAutoMode();
	IBaseObject * obj;
	ObjSet & set = packet->getSet();
	OBJPTR<IHarvest> harvest;
	U32 i;

	for (i = 0; i < set.numObjects; i++)
	{
		if ((obj = OBJLIST->FindObject(set.objectIDs[i])) != 0)
		{
			if (obj->QueryInterface(IHarvestID, harvest))
			{
				harvest->SetAutoHarvest(stance->nuggetType);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostFighterStanceChange (SUPERBASE_PACKET * packet)
{
	FIGHTER_STANCE_PACKET * stance = packet->castFighterStance();
	IBaseObject * obj;
	ObjSet & set = packet->getSet();
	OBJPTR<IAttack> attacker;
	U32 i;

	for (i = 0; i < set.numObjects; i++)
	{
		if ((obj = OBJLIST->FindObject(set.objectIDs[i])) != 0)
		{
			if (obj->QueryInterface(IAttackID, attacker))
			{
				attacker->SetFighterStance(stance->stance);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostAdmiralTacticChange (SUPERBASE_PACKET * packet)
{
	ADMIRAL_TACTIC_PACKET * stance = packet->castAdmiralTactic();
	IBaseObject * obj;
	ObjSet & set = packet->getSet();
	OBJPTR<IAdmiral> admiral;
	U32 i;

	for (i = 0; i < set.numObjects; i++)
	{
		if ((obj = OBJLIST->FindObject(set.objectIDs[i])) != 0)
		{
			if (obj->QueryInterface(IAdmiralID, admiral))
			{
				admiral->SetAdmiralTactic(stance->stance);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostAdmiralFormationChange (SUPERBASE_PACKET * packet)
{
	ADMIRAL_FORMATION_PACKET * stance = packet->castAdmiralFormation();
	IBaseObject * obj;
	ObjSet & set = packet->getSet();
	OBJPTR<IAdmiral> admiral;
	U32 i;

	for (i = 0; i < set.numObjects; i++)
	{
		if ((obj = OBJLIST->FindObject(set.objectIDs[i])) != 0)
		{
			if (obj->QueryInterface(IAdmiralID, admiral))
			{
				U32 archID = admiral->GetKnownFormation(stance->slotID);
				if(archID)
					admiral->SetFormation(archID);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostSpecialAbility (SUPERBASE_PACKET * packet)
{
	USRSPABILITY * spPacket = packet->castSpecialAbility();
	VOLPTR(IToggle) toggle;
	VOLPTR(IAttack) attack;
	ObjSet & set = packet->getSet();
	U32 i;
	bool bToggleOff = true;
	bool bToggleHere = false;

	for (i = 0; i < set.numObjects; i++)
	{
		if ((attack = OBJLIST->FindObject(set.objectIDs[i])) != 0)
		{
			if((toggle = attack.Ptr()) && toggle->IsToggle())
			{
				if(toggle->CanToggle())
				{
					bToggleHere = true;
					if(!(toggle->IsOn()))
						bToggleOff = false;
				}
			}
			else
				attack->DoSpecialAbility(spPacket->specialID);
		}
	}

	if(bToggleHere)
	{
		for (i = 0; i < set.numObjects; i++)
		{
			if ((attack = OBJLIST->FindObject(set.objectIDs[i])) != 0)
			{
				if((toggle = attack.Ptr()) && toggle->IsToggle())
				{
					if(toggle->CanToggle())
					{
						if(bToggleOff && toggle->IsOn())
							attack->DoSpecialAbility(spPacket->specialID);
						else if((!bToggleOff) && (!(toggle->IsOn())))
							attack->DoSpecialAbility(spPacket->specialID);
					}
				}
			}
		}
	}

}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostCloakChange (SUPERBASE_PACKET * packet)
{
	VOLPTR(IAttack) attack;
	VOLPTR(ICloak) cloak;
	ObjSet & set = packet->getSet();
	U32 i;
	bool bCloakOff = true;
		
	for (i = 0; i < set.numObjects; i++)
	{
		if ((cloak = OBJLIST->FindObject(set.objectIDs[i])) != 0)
		{
			if(cloak->CanCloak())
			{
				MPart part(cloak.Ptr());
				if(part.isValid())
				{
					if(part->caps.cloakOk)
					{
						if(!cloak.Ptr()->bCloaked)
							bCloakOff = false;
					}
				}
			}
		}
	}

	for (i = 0; i < set.numObjects; i++)
	{
		if ((attack = OBJLIST->FindObject(set.objectIDs[i])) != 0)
		{
			if(bCloakOff && attack.Ptr()->bCloaked)
				attack->DoCloak();
			else if((!bCloakOff) && (!(attack.Ptr()->bCloaked)))
				attack->DoCloak();
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostEjectArtifact (SUPERBASE_PACKET * packet)
{
	//unimplemented until the create ship network issue is sorted out
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostMimicChange (SUPERBASE_PACKET * packet)
{
	VOLPTR(IAttack) attack;
	ObjSet & set = packet->getSet();
	U32 i;
	const USRMIMIC * const pMimic = packet->castMimic();
	OBJPTR<IBaseObject> victim;

	OBJLIST->FindObject(pMimic->targetID, set.objectIDs[0] & PLAYERID_MASK, victim);
	
	for (i = 0; i < set.numObjects; i++)
	{
		if ((attack = OBJLIST->FindObject(set.objectIDs[i])) != 0)
			attack->DoSpecialAbility(victim);
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendDockCommand (SUPERBASE_PACKET *packet, SUPERBASE_PACKET * prev)
{
	IBaseObject * obj = OBJLIST->FindObject(packet->getSet().objectIDs[0]);
	IBaseObject * obj2 = OBJLIST->FindObject(packet->getSet().objectIDs[1]);
	if (obj && obj2 && packet->getSet().numObjects==2)
	{
		U32 victimSystem = getRealSystemID(obj2) & ~HYPER_SYSTEM_MASK;
		U32 admiralSystem = getRealSystemID(obj) & ~HYPER_SYSTEM_MASK;
		//
		// move the victim to our system
		//
		if ((victimSystem & ~HYPER_SYSTEM_MASK) != (admiralSystem & ~HYPER_SYSTEM_MASK))
		{
			SUPERBASE_PACKET * newpacket = createPacket(MOVETO, sizeof(USRMOVE)-sizeof(BASE_PACKET));
			USRMOVE * move = newpacket->castMove();
			move->position.init(obj->GetGridPosition(), admiralSystem);
			newpacket->getSet().objectIDs[0] = packet->getSet().objectIDs[1];
			newpacket->getSet().numObjects = 1;
			newpacket->parentOp = packet->parentOp;

			sendHostMoveCommandFromSystem(newpacket, victimSystem, obj2->GetPlayerID());
			prev = newpacket;
			while (prev->pNext)
				prev = prev->pNext;
			packet->parentOp = prev->opID;
		}

		packet->opID = allocOpID();
		packet->groupID = bufferedSend(DOCKFLAGSHIP, packet->opID, NULL, 0, packet->getSet());
		appendToOpList(packet, prev);
		if (packet->parentOp==0)
			doCommand(packet);		// if invalid, clears the set
	}
	else
	{
		::free(packet);
		packet=0;
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendUndockCommand (SUPERBASE_PACKET *packet, SUPERBASE_PACKET * prev)
{
	IBaseObject * obj = OBJLIST->FindObject(packet->getSet().objectIDs[0]);
	IBaseObject * obj2 = OBJLIST->FindObject(packet->getSet().objectIDs[1]);
	if (obj && obj2 && packet->getSet().numObjects==2)
	{
		packet->opID = allocOpID();
		packet->hostCmd = UNDOCKFLAGSHIP;
		packet->groupID = bufferedSend(UNDOCKFLAGSHIP, packet->opID, NULL, 0, packet->getSet());
		appendToOpList(packet, prev);
		prev = packet;

		if (packet->parentOp==0)
			doCommand(packet);		// if invalid, clears the set
	}
	else
	{
		::free(packet);
		packet=0;
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::killSomeUnits (SUPERBASE_PACKET * packet)
{
	const ObjSet & set = packet->getSet();
	U32 i;

	for (i = 0; i < set.numObjects; i++)
		THEMATRIX->ObjectTerminated(set.objectIDs[i], 0);
}
//--------------------------------------------------------------------------//
// ensure only one unit per repair command
//
void OpAgent::sendShipRepair (SUPERBASE_PACKET * packet)
{
	ObjSet & set = packet->getSet();

	while (set.numObjects > 1)
	{
		SUPERBASE_PACKET * newpacket = duplicatePacket(packet);
		newpacket->getSet().numObjects = 1;
		set -= newpacket->getSet();

		sendHostMoveCommand(newpacket);
	}

	sendHostMoveCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostJumpCommand (SUPERBASE_PACKET * packet)
{
	IBaseObject * gate = OBJLIST->FindObject(packet->castUserJump()->jumpgateID);
	CQASSERT(gate);
	SUPERBASE_PACKET * newpacket = createPacket(PREJUMP, sizeof(HSTPREJUMP)-sizeof(BASE_PACKET));
	SUPERBASE_PACKET * jmppacket = createPacket(JUMP, sizeof(HSTJUMP)-sizeof(BASE_PACKET));

	HSTPREJUMP * pre = newpacket->castPreJump();
	pre->jumpgateID = gate->GetPartID();
	pre->bUserInitiated = 1;
	pre->ctrlBits = packet->castBasePacket()->ctrlBits;

	//
	// remove members from the set who are already in the destination system
	//
	{
		ObjSet & set = packet->getSet();
		U32 numObjects=set.numObjects;
		IBaseObject * destGate = SECTOR->GetJumpgateDestination(gate);
		CQASSERT(destGate);
		U32 destSystemID = destGate->GetSystemID();
	
		while (numObjects-- > 0)
		{
			IBaseObject * obj = OBJLIST->FindObject(set.objectIDs[numObjects]);

			if (obj == 0 || getRealSystemID(obj) == destSystemID)
			{
				set.removeObject(set.objectIDs[numObjects]);
			}
		}

		// is there anyone left to carry out the command?
		if (set.numObjects == 0)
		{
			::free(packet);
			::free(newpacket);
			::free(jmppacket);
			return;
		}
	}

	newpacket->getSet() = packet->getSet();
	sendHostMoveCommand(packet);		// go to system which contains the jumpgate
	packet = pOpList;

	SUPERBASE_PACKET * prev = 0;

	while (packet)
	{
		prev = packet;
		packet = packet->pNext;
	}
	// prev = last member of the list
	
	//
	// verify that everyone is in the right system (remove those who aren't)
	//
	{
		ObjSet & set = newpacket->getSet();
		U32 numObjects = set.numObjects;
		const U32 systemID = gate->GetSystemID();
	
		while (numObjects-- > 0)
		{
			IBaseObject * obj = OBJLIST->FindObject(set.objectIDs[numObjects]);

			if (obj == 0 || getRealSystemID(obj) != systemID)
			{
				set.removeObject(set.objectIDs[numObjects]);		// a previous jump must have failed
			}
		}

		// is there anyone left to carry out the command?
		if (set.numObjects == 0)
		{
			::free(newpacket);
			::free(jmppacket);
			return;
		}
	}
	
	jmppacket->getSet() = newpacket->getSet();
	
	//
	// now go to the other system
	//
	newpacket->parentOp = findParentOp(newpacket->getSet());
	newpacket->opID = allocOpID();
	newpacket->groupID = bufferedSend(PREJUMP, newpacket->opID, newpacket->getOperands(), newpacket->getOperandSize(), newpacket->getSet(),newpacket->ctrlBits);
	appendToOpList(newpacket, prev);
	if (newpacket->parentOp==0)	// did original packet have a parent?
		doCommand(newpacket);		// if invalid, clears the set
	prev = newpacket;
	//
	// now we need to use the jumpgate
	//
	jmppacket->parentOp = newpacket->opID;
	newpacket = jmppacket;
	jmppacket=0;
	HSTJUMP * jump = newpacket->castJump();
	jump->jumpgateID = gate->GetPartID();
	newpacket->opID = allocOpID();
	newpacket->groupID = bufferedSend(JUMP, newpacket->opID, newpacket->getOperands(), newpacket->getOperandSize(), newpacket->getSet());
	appendToOpList(newpacket, prev);
	prev = newpacket;
}
//--------------------------------------------------------------------------//
//
void OpAgent::sendHostCommand (SUPERBASE_PACKET * packet, SUPERBASE_PACKET * prev)
{
	switch (packet->type)
	{
	case PT_USRMOVE:
		packet->hostCmd = MOVETO;
		sendHostMoveCommand(packet);
		break;
	case PT_USRFORMATIONMOVE:
		packet->hostCmd = FORMATIONMOVETO;
		sendHostFormationMoveCommand(packet);
		break;
	case PT_FORMATIONATTACK:
		packet->hostCmd = FORMATIONATTACKCMD;
		sendHostFormationAttackCommand(packet);
		break;
	case PT_USRJUMP:
		packet->hostCmd = HOSTJUMP;
		sendHostJumpCommand(packet);
		break;
	case PT_USRAOEATTACK:
		packet->hostCmd = AOEATTACK;
		sendHostMoveCommand(packet);
		break;
	case PT_USEARTIFACTTARGETED:
		packet->hostCmd = ARTIFACTTARGETED;
		sendHostMoveCommand(packet);
		break;
	case PT_USRWORMATTACK:
		packet->hostCmd = WORMATTACK;
		sendHostMoveCommand(packet);
		break;
	case PT_USRPROBE:
		packet->opID = allocOpID();
		packet->hostCmd = PROBE;
		packet->groupID = bufferedSend(PROBE, packet->opID, packet->getOperands(), packet->getOperandSize(), packet->getSet(),packet->ctrlBits);
		appendToOpList(packet, prev);
		if (packet->parentOp==0)
			doCommand(packet);		// if invalid, clears the set
		break;
	case PT_USRRECOVER:
		packet->hostCmd = RECOVER;
		sendHostMoveCommand(packet);
		break;
	case PT_USRDROPOFF:
		packet->hostCmd = DROP_OFF;
		sendHostMoveCommand(packet);
		break;
	case PT_USRMIMIC:
		sendHostMimicChange(packet);
		::free(packet);
		break;
	case PT_USRCREATEWORMHOLE:
		packet->opID = allocOpID();
		packet->hostCmd = CREATEWORMHOLE;
		packet->groupID = bufferedSend(CREATEWORMHOLE, packet->opID, packet->getOperands(), packet->getOperandSize(), packet->getSet());
		appendToOpList(packet, prev);
		if (packet->parentOp==0)
			doCommand(packet);		// if invalid, clears the set
		break;
	case PT_USRATTACK:
		{
			packet->hostCmd = ATTACK;
			USRATTACK * pAttack = packet->castAttack();
			if (pAttack->destSystemID != 0)
			{
				IBaseObject * obj = OBJLIST->FindObject(pAttack->targetID);
				if (obj == 0 || obj->GetSystemID() != pAttack->destSystemID)
				{
					::free(packet);		// target has changed systems, don't go after him
					break;
				}
			}

			sendHostMoveCommand(packet);
		}
		break;
	case PT_USRSPATTACK:
		{
			packet->hostCmd = SPATTACK;
			USRATTACK * pAttack = packet->castAttack();
			if (pAttack->destSystemID != 0)
			{
				IBaseObject * obj = OBJLIST->FindObject(pAttack->targetID);
				if (obj == 0 || obj->GetSystemID() != pAttack->destSystemID)
				{
					::free(packet);		// target has changed systems, don't go after him
					break;
				}
			}
			sendHostMoveCommand(packet);
		}
		break;
	case PT_USRSTOP:
		packet->opID = allocOpID();
		packet->hostCmd = STOP;
		packet->groupID = bufferedSend(STOP, packet->opID, packet->getOperands(), packet->getOperandSize(), packet->getSet());
		appendToOpList(packet, prev);
		if (packet->parentOp==0)
			doCommand(packet);		// if invalid, clears the set
		break;
	case PT_USRBUILD:
		sendHostBuildCommand(packet);
		::free(packet);
		break;
	case PT_STANCECHANGE:
		sendHostStanceChange(packet);
		::free(packet);
		break;
	case PT_SUPPLYSTANCECHANGE:
		sendHostSupplyStanceChange(packet);
		::free(packet);
		break;
	case PT_HARVESTSTANCECHANGE:
		sendHostHarvestStanceChange(packet);
		::free(packet);
		break;
	case PT_HARVESTERAUTOMODE:
		sendHostHarvestAutoModeChange(packet);
		::free(packet);
		break;
	case PT_FIGHTERSTANCECHANGE:
		sendHostFighterStanceChange(packet);
		::free(packet);
		break;
	case PT_ADMIRALTACTICCHANGE:
		sendHostAdmiralTacticChange(packet);
		::free(packet);
		break;
	case PT_ADMIRALFORMATIONCHANGE:
		sendHostAdmiralFormationChange(packet);
		::free(packet);
		break;
	case PT_FLEETDEF:
		packet->opID = allocOpID();
		packet->hostCmd = FLEETDEF;
		packet->groupID = bufferedSend(FLEETDEF, packet->opID, packet->getOperands(), packet->getOperandSize(), packet->getSet());
		appendToOpList(packet, prev);
		if (packet->parentOp==0)
			doCommand(packet);		// if invalid, clears the set
		break;
	case PT_USRFAB:
		packet->hostCmd = FABRICATE;
		sendHostMoveCommand(packet);
		break;
	case PT_USRFABJUMP:
		packet->hostCmd = FABRICATE_JUMP;
		sendHostMoveCommand(packet);
		break;	
	case PT_USRFABPOS:
		packet->hostCmd = FABRICATE_POS;
		sendHostMoveCommand(packet);
		break;	
	case PT_USRHARVEST:
		{
			U32 oldTotal = totalDataSent;
			packet->hostCmd = HARVEST;
			sendHostMoveCommand(packet);
		
			U32 dataAmount = totalDataSent - oldTotal;
			harvestBin[currentBin] += dataAmount;		// update harvest throughput
			harvestThroughput += dataAmount;
		}
		break;
	case PT_USRRALLY:
		packet->opID = allocOpID();
		packet->hostCmd = RALLY;
		packet->groupID = bufferedSend(RALLY, packet->opID, packet->getOperands(), packet->getOperandSize(), packet->getSet());
		setRallyPoint(packet->groupID, *((NETGRIDVECTOR *)packet->getOperands()));
		::free(packet);
		break;
	case PT_USRESCORT:
		packet->hostCmd = ESCORT;
		sendHostMoveCommand(packet);
		break;
	case PT_USRDOCKFLAGSHIP:
		packet->hostCmd = DOCKFLAGSHIP;
		sendDockCommand(packet, prev);
		break;
	case PT_USRUNDOCKFLAGSHIP:
		packet->hostCmd = UNDOCKFLAGSHIP;
		sendUndockCommand(packet, prev);
		break;
	case PT_USRRESUPPLY:
		packet->opID = allocOpID();
		packet->hostCmd = RESUPPLY;
		packet->groupID = bufferedSend(RESUPPLY, packet->opID, packet->getOperands(), packet->getOperandSize(), packet->getSet(),packet->ctrlBits);
		appendToOpList(packet, prev);
		if (packet->parentOp==0)
			doCommand(packet);		// if invalid, clears the set
		break;
	case PT_USRFABREPAIR:
		packet->hostCmd = FABREPAIR;
		sendHostMoveCommand(packet);
		break;
	case PT_USRSHIPREPAIR:
		packet->hostCmd = SHIPREPAIR;
		sendShipRepair(packet);
		break;
	case PT_USRCAPTURE:
		packet->hostCmd = CAPTURE;
		sendHostMoveCommand(packet);
		break;
	case PT_USRSPABILITY:
		sendHostSpecialAbility(packet);
		::free(packet);
		break;
	case PT_USRCLOAK:
		sendHostCloakChange(packet);
		::free(packet);
		break;
	case PT_USR_EJECT_ARTIFACT:
		sendHostEjectArtifact(packet);
		::free(packet);
		break;
	case PT_USRFABSALVAGE:
		packet->hostCmd = SALVAGE;
		sendHostMoveCommand(packet);
		break;
	case PT_PATROL:
		packet->opID = allocOpID();
		packet->hostCmd = PATROL;
		packet->groupID = bufferedSend(PATROL, packet->opID, packet->getOperands(), packet->getOperandSize(), packet->getSet(),packet->ctrlBits);
		appendToOpList(packet, prev);
		if (packet->parentOp==0)
			doCommand(packet);		// if invalid, clears the set
		break;
	case PT_USRKILLUNIT:
		killSomeUnits(packet);
		::free(packet);
		break;

	default:
		CQBOMB1("Unhandled packet type received: %d", packet->type);
	}
	
	//
	// append packet to the end of the list
	// 
}
//--------------------------------------------------------------------------//
// send a packet, return group ID
//
U32 OpAgent::bufferedSend (HOSTCMD cmd, U32 opID, void *operands, U32 operandSize, const ObjSet & set, bool controlState)
{
	//
	// see if there is already a set defined
	//
	U32 groupID;

	if(controlState)
		cmd = (HOSTCMD)(((U32)cmd) | CONTROL_ON_FLAG);
	
	if (set.numObjects <= 1)
	{
		groupID = set.objectIDs[0];
	}
	else  // find or create a group
	{
		IBaseObject * obj = OBJLIST->FindGroupObject(set.objectIDs, set.numObjects);

		if (obj)
		{
			groupID = obj->GetPartID();
		}
		else // must create a group
		{
			obj = ARCHLIST->CreateInstance("Group!!Default");
			CQASSERT(obj);
			OBJPTR<IGroup> group;

			obj->QueryInterface(IGroupID, group);
			groupID = MGlobals::CreateNewGroupPartID();
			group->InitGroup(set.objectIDs, set.numObjects, groupID);
			OBJLIST->AddGroupObject(obj);

			addToHostBuffer(NEWGROUP, 0, groupID, set.objectIDs, set.numObjects * sizeof(U32));
		}
	}
	//
	// now we have a group defined
	//
	if (set.numObjects > 0)
		addToHostBuffer(cmd, opID, groupID, operands, operandSize);

	return groupID;
}
//--------------------------------------------------------------------------//
//
void OpAgent::addToHostBuffer (HOSTCMD cmd, U32 opID, U32 groupID, const void * operands, U32 operandSize)
{
	CQASSERT(opID!=0 || cmd==NEWGROUP);
	//
	// if buffer is empty, write the header
	//
	if (bufferOffset == 0)
	{
		BASE_PACKET * packet = (BASE_PACKET *) packetBuffer;

		memset(packet, 0, sizeof(*packet));
		packet->type = PT_HOSTUPDATE;
		bufferOffset += sizeof(*packet);
		totalDataSent += sizeof(*packet);
	}

	//
	// is there enough room in the buffer for the new data?
	//
	U32 size = sizeof(HOST_SUBPACKET_HEADER) + operandSize;

	if (size + bufferOffset > MAX_PACKET_SIZE)
	{
		flushSendBuffer();

		BASE_PACKET * packet = (BASE_PACKET *) packetBuffer;

		memset(packet, 0, sizeof(*packet));
		packet->type = PT_HOSTUPDATE;
		bufferOffset += sizeof(*packet);

		CQASSERT(size + bufferOffset <= MAX_PACKET_SIZE);
	}

	HOST_SUBPACKET_HEADER * header = (HOST_SUBPACKET_HEADER *) (packetBuffer + bufferOffset);

	header->size = size;
	CQASSERT(header->size == size);
	header->cmd = cmd;
	header->groupID = groupID;

	if (operands)
		memcpy(header+1, operands, operandSize);

	bufferOffset += size;
	totalDataSent += size;
}
//--------------------------------------------------------------------------//
// actually send the data
//
void OpAgent::flushSendBuffer (void)
{
	if (bufferOffset>0)
	{
		BASE_PACKET * packet = (BASE_PACKET *) packetBuffer;
		
		packet->dwSize = bufferOffset;
		NETPACKET->Send(0, NETF_ALLREMOTE, packet);

		bufferOffset = 0;		// buffer is now empty

		if (CQFLAGS.bClientPlayback==0 && NETOUTPUT!=0)
		{
			// write data to the file
			DWORD dwWritten;
			U32 updateCount = MGlobals::GetUpdateCount();

			NETOUTPUT->WriteFile(0, &updateCount, sizeof(updateCount), &dwWritten, 0);
			NETOUTPUT->WriteFile(0, packet, packet->dwSize, &dwWritten, 0);
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::updateOpList (void)
{
	SUPERBASE_PACKET *node = pOpList, *prev=0;

#if 0
	verifyDependencies();
#endif

	// remove ops that have completed
	while (node)
	{
		if (node->getSet().numObjects==0)    //		if (node->parentOp==0 && node->bCompleted)
		{
			U32 opID = node->opID;

			/*
			if (CQFLAGS.bTraceMission != 0)
			{
				if (node->isUserData()==false)
				{
					if (node->hostCmd == PROCESS)
						OPPRINT1("PROCESS #%d completed.\n", opID);
					else
						OPPRINT1("Agent #%d completed.\n", opID);
				}
			}
			*/

			if (prev)
			{
				prev->pNext = node->pNext;
				::free(node);
				node = prev->pNext;
			}
			else
			{
				pOpList = node->pNext;
				::free(node);
				node = pOpList;
			}
			onOpCompleted(NULL, opID);
		}
		else
		{
			prev = node;
			node = node->pNext;
		}
	}
}
//--------------------------------------------------------------------------//
// pOp is already in this list
//
void OpAgent::findParentOp (SUPERBASE_PACKET *pOp)
{
	SUPERBASE_PACKET *node = pOpList;

	while (node && node != pOp)
	{
		if ((pOp->getSet() & node->getSet()).numObjects > 0)
		{
			pOp->parentOp = node->opID;
			break;
		}
		node = node->pNext;
	}
}
//--------------------------------------------------------------------------//
// find pending conflicts for an ADD packet
//
SUPERBASE_PACKET * OpAgent::findConflictingOpForAdd (SUPERBASE_PACKET *pAdd)
{
	OpAgent * const THEMATRIX = ((OpAgent *)::THEMATRIX);
	SUPERBASE_PACKET *node = THEMATRIX->pOpList;
	U32 processID = pAdd->parentOp;		// assume parentOp has been set correctly beforehand
	SUPERBASE_PACKET *pProcessOp = THEMATRIX->findOp(processID);
	CQASSERT(pProcessOp);

	while (node && node != pAdd)
	{
		if (node->opID != processID && (pAdd->getSet() & node->getSet()).numObjects > 0)
		{
			pAdd->parentOp = node->opID;
			return node;
		}
		node = node->pNext;
	}

	return 0;
}
//--------------------------------------------------------------------------//
//
U32 OpAgent::findParentOp (const ObjSet & set)
{
	U32 result=0;
	SUPERBASE_PACKET *node = pOpList;

	while (node)
	{
		if ((set & node->getSet()).numObjects > 0)
		{
			result = node->opID;
			break;
		}
		node = node->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
// search for anyone waiting on this op, 
//
void OpAgent::onOpCompleted (SUPERBASE_PACKET *pCompletedOp, U32 opID)
{
	if (opID)
	{
		SUPERBASE_PACKET *node = pOpList;

		while (node)
		{
			if (node->parentOp == opID)
			{
				// stalled packets should be activated directly after the parent
				CQASSERT(node->bStalled==0);
				node->parentOp = 0;

				// some kinds of packets depend specifically on one parent
				if (node->isUserData()==false)
					findParentOp(node);
				else
				if (node->hostCmd != ENABLEDEATH)		// death packets have a custom way to determine parent
				{
					// make sure the op really completed
					if (pCompletedOp && pCompletedOp->getSet().numObjects)
						node->parentOp = opID;		// restore parentOp
				}

				if (node->hostCmd == ADDTO && node->getAddToTarget() == node->parentOp)
				{
					node->parentOp = 0;		// let the ADDTO command figure things out
				}

				if (node->parentOp == 0)
					doCommand(node);
			}

			node = node->pNext;
		}
	}
}
//--------------------------------------------------------------------------//
//
SUPERBASE_PACKET * OpAgent::findOp (U32 opID) const
{
	if (opID == 0)
		return 0;

	SUPERBASE_PACKET *node = pOpList;

	while (node)
	{
		if (node->opID == opID)
			break;
		node = node->pNext;
	}

	return node;
}
//--------------------------------------------------------------------------//
//
SUPERBASE_PACKET * OpAgent::findDataDependency (SUPERBASE_PACKET *pOp)
{
	SUPERBASE_PACKET * result = 0;

	//
	// find the last data packet that has us as the parentOp
	// ENABLEDEATH can depend on the start of a PROCESS, or data packets
	// NOTE: Do not think that a NULL return value means there is no pending data for an op!
	//   Only execute dependent data if the result == data packet.
	//
	SUPERBASE_PACKET * node = pOp->pNext;
	while (node)
	{
		// stalled packets behave like data packets for dependency checking
		if (node->getSet().numObjects>0 && (node->bStalled || node->isUserData() || node->hostCmd==ADDTO))
		{
			if (node->parentOp == pOp->opID || (node->hostCmd==ADDTO && ((U32 *)node->getOperands())[0]==pOp->opID))
			{
				ObjSet & set = node->getSet();
				
				// multiple dependency case
				// if set is not equal, block further searching. else if set is equal, execute the op
				// sets don't have to match if we aren't executing a process
				// exceptions to the rule:
				//    #1: ENABLEDEATH+CANCEL are executed when all data packets in front
				//			 of it have been processed. (i.e. sets don't have to match)
				//    #2: ADDTO: set contains the match set + first object to add. Consider the sets equal if the
				//			 compared set does NOT contain the new object & the sets are otherwise equal.
				//    #3: OBJSYNC: if bDelayDependency is true && our destination object is still a member, return NULL. (i.e. keep waiting)
				//    #4: Stalled packets began life dependent on some other op, but later switch to depend on a process
				//           that will (hopefully) create a target object.

				if (node->bDelayDependency)
				{
					CQASSERT(node->hostCmd==OBJSYNC);
					if (pOp->getSet().contains(set.objectIDs[0]) == 0)
						result = node;
					// else nothing (return NULL)
				}
				else
				if (node->hostCmd == CANCEL)
				{
					result = node;
				}
				else
				if (node->hostCmd == ENABLEDEATH)
				{
					// block until unit is removed from the op (kinda like bDelayDependency)
					if (pOp->getSet().contains(set.objectIDs[0]) == 0)
						result = node;
				}
				else
				if (node->bStalled)
				{
					result = node;
				}
				else
				if (node->hostCmd == ADDTO)
				{
					// don't execute unless we are finding data dependencies for our process
					if (node->parentOp == pOp->opID)
					{
						node->parentOp = ((U32 *)node->getOperands())[0];
						if (findConflictingOpForAdd(node) == 0)		// if no matching op's ahead of us
						{
							OpAgent * const THEMATRIX = ((OpAgent *)::THEMATRIX);
							SUPERBASE_PACKET * pProcess = THEMATRIX->findOp(node->parentOp);
							CQASSERT(pProcess);		// very bad if process were to terminate early
							if (pProcess->parentOp==0)			// wait until process is running
							{
								if (pProcess->getSet().contains(set.objectIDs[0]) == 0)		// doesn't already contain the guy we want to add
								{
									ObjSet tmpSet = pProcess->getSet();

									// set may be full because we're not supposed to be here yet
									if (tmpSet.numObjects < MAX_SELECTED_UNITS)
									{
										tmpSet.objectIDs[tmpSet.numObjects++] = set.objectIDs[0];	// add the object

										if (tmpSet == set)
											result = node;
									}
								}
							}
						}
					}
				}
				else
				if (pOp->isProcess()==false || pOp->getSet() == set)
				{
					result = node;
				}
				break;
			}
		}

		node = node->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
// don't destroy units until all pending items have been dispatched
//
SUPERBASE_PACKET * OpAgent::findDependentItemForDeath (SUPERBASE_PACKET * pDeath, U32 objectID)
{
	SUPERBASE_PACKET * node = pOpList, *result=0;

	while (node)
	{
		if (node == pDeath)
			break;
		if (node->getSet().contains(objectID))
		{
			result = node;
			break;
		}

		node = node->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//	remove dead guys from the set
//
void OpAgent::validateSet (ObjSet & set)
{
	U32 i = 0;
	while (i < set.numObjects)
	{
		IBaseObject * obj;
		if (set.objectIDs[i] && (obj = OBJLIST->FindObject(set.objectIDs[i])) != 0)
		{
			VOLPTR(IPlatform) platform = obj;
			if (platform==0 || platform->IsReallyDead()==0)		// remove dead platforms from the set
				i++;
			else
				set.removeObject(set.objectIDs[i]);
		}
		else
			set.removeObject(set.objectIDs[i]);
	}
}
//--------------------------------------------------------------------------//
// for clients only
#define SIZEDIFF 10
void OpAgent::clientReceivePacket (BASE_PACKET * packet)
{
	U8 * const buffer = (U8 *) packet;
	HOST_SUBPACKET_HEADER * hostheader;
	SUBPACKET_HEADER * header;
	U32 offset = sizeof(BASE_PACKET);
	const U32 maxSize = packet->dwSize;
	SUBPACKET_HEADER savedMemory;

	CQASSERT(sizeof(SUBPACKET_HEADER) - sizeof(HOST_SUBPACKET_HEADER) == SIZEDIFF);

	while (offset < maxSize)
	{
		hostheader = (HOST_SUBPACKET_HEADER *) (buffer + offset);
		header = (SUBPACKET_HEADER *) (buffer + offset - SIZEDIFF);
		savedMemory = *header;		// save current memory contents
		//
		// convert host-side header into client side,
		//
		header->size = hostheader->size + SIZEDIFF;
		header->cmd = static_cast<HOSTCMD> (hostheader->cmd);
		if (header->cmd==NEWGROUP)
			header->opID = 0;
		else
			header->opID = ++lastOpID;
		// don't need to copy header->groupID, since it's in the same location

		switch (header->cmd & (~CONTROL_ON_FLAG))
		{
		case NEWGROUP:
			clientCreateNewGroup(header);
			break;
		case DELETEGROUP:
			clientDeleteGroup(header);
			break;
		case CANCEL:
			clientCancelOp(header);
			break;
		case MOVETO:
			clientMoveTo(header);
			break;
		case ATTACK:
			clientAttack(header);
			break;
		case SPATTACK:
			clientSPAttack(header);
			break;
		case PREJUMP:
			clientPrejump(header);
			break;
		case JUMP:
			clientJump(header);
			break;
		case ENABLEJUMP:
			clientEnableJump(header);
			break;
		case OBJSYNC:
		case OBJSYNC2:
			clientObjSync(header);
			break;
		case STOP:
			clientStop(header);
			break;
		case ENABLEDEATH:
			clientEnableDeath(header);
			break;
		case AOEATTACK:
			clientAOEAttack(header);
			break;
		case ARTIFACTTARGETED:
			clientArtifactTargeted(header);
			break;
		case WORMATTACK:
			clientWormAttack(header);
			break;
		case PROBE:
			clientProbe(header);
			break;
		case RECOVER:
			clientRecover(header);
			break;
		case DROP_OFF:
			clientDropOff(header);
			break;
		case PROCESS:
			clientCreateProcess(header);
			break;
		case OPDATA:
		case OPDATA2:
			clientOpData(header);
			break;
		case FABRICATE:
			clientFabricate(header);
			break;
		case FABRICATE_JUMP:
			clientFabricateJump(header);
			break;
		case FABRICATE_POS:
			clientFabricatePos(header);
			break;
		case SYNCMONEY:
			clientSyncMoney(header);
			break;
		case SYNCVISIBILITY:
			clientSyncVisibility(header);
			break;
		case SYNCALLIANCE:
			clientSyncAlliance(header);
			break;
		case SYNCSCORES:
			clientSyncScores(header);
			break;
		case SYNCNUGGETS:
			clientSyncNuggets(header);
			break;
		case HARVEST:
			clientHarvest(header);
			break;
		case ADDTO:
			clientAddTo(header);
			break;
		case RALLY:
			clientRally(header);
			break;
		case ESCORT:
			clientEscort(header);
			break;
		case MIGRATIONCOMPLETE:
			clientMigrationComplete(header);
			break;
		case DOCKFLAGSHIP:
			clientDock(header);
			break;
		case UNDOCKFLAGSHIP:
			clientUndock(header);
			break;
		case RESUPPLY:
			clientResupply(header);
			break;
		case FABREPAIR:
			clientFabRepair(header);
			break;
		case SHIPREPAIR:
			clientShipRepair(header);
			break;
		case CAPTURE:
			clientCapture(header);
			break;
		case SALVAGE:
			clientFabSalvage(header);
			break;
		case PATROL:
			clientPatrol(header);
			break;
		case FLEETDEF:
			clientSetFleetDef(header);
			break;
		case CREATEWORMHOLE:
			clientCreateWormhole(header);
			break;
		case NUGGETDATA:
			clientNuggetData(header);
			break;
		case NUGGETDEATH:
			clientNuggetDeath(header);
			break;
		case PLATFORMDEATH:
			clientPlatformDeath(header);
			break;
		case RELOCATE:
			clientRelocateOp(header);
			break;
		case CANCELQUEUED:
			clientCancelQueued(header);
			break;

		default:
			CQBOMB0("Unhandled client packet");
		}

		*header = savedMemory;		// restore the original data
		offset += hostheader->size;
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientSyncStats (SYNCSTATS * packet)
{
	U32 statSize = packet->dwSize - sizeof(SYNCSTATS);

	MGlobals::SetGameStats(packet->playerID, packet->stats, statSize);
	MGlobals::SetUpdateCount(packet->updateCount);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientCreateNewGroup (SUBPACKET_HEADER *header)
{
	IBaseObject * obj = ARCHLIST->CreateInstance("Group!!Default");
	CQASSERT(obj);
	OBJPTR<IGroup> group;
	const U32 *pObjIDs = (const U32 *) (header+1);
	U32 numObjects;

	numObjects = (header->size - sizeof(SUBPACKET_HEADER)) / sizeof(U32);

	obj->QueryInterface(IGroupID, group);
	group->InitGroup(pObjIDs, numObjects, header->groupID);
	OBJLIST->AddGroupObject(obj);
	MGlobals::SetLastPartID(header->groupID);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientDeleteGroup (SUBPACKET_HEADER *header)
{
	IGroup * group = OBJLIST->FindGroupObject(header->groupID);
	CQASSERT(group);

	if (CQFLAGS.bTraceMission != 0)
		OPPRINT1("Marking group 0x%X stale\n", header->groupID);

	group->EnableDeath();
}
//--------------------------------------------------------------------------//
//
void OpAgent::initSetFromGroupID (ObjSet & set, U32 groupID)
{
	if (MGlobals::GetPlayerFromPartID(groupID) == MGlobals::GetGroupID())
	{
		IGroup * group = OBJLIST->FindGroupObject(groupID);
		if (group==0)
		{
			OpAgent * const THEMATRIX = ((OpAgent *)::THEMATRIX);
			CQBOMB2("Invalid GroupID '%X' received! (Last valid op was #%d)", groupID, THEMATRIX->lastOpID);
			set.numObjects = 1;
			set.objectIDs[0] = groupID;
		}
		else
		{
			set.numObjects = group->GetObjects(set.objectIDs);
		}
	}
	else // single unit
	{
		set.numObjects = 1;
		set.objectIDs[0] = groupID;
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientCancelOp (SUBPACKET_HEADER *header)
{
	U32 * pCancelOp = (U32 *) (header+1);		// get our data
	CQASSERT(header->size == sizeof(SUBPACKET_HEADER) + sizeof(U32));
	SUPERBASE_PACKET * node = findOp(*pCancelOp);

	if (node)
	{
		// queue it up, like the rest
		SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

		packet->parentOp = node->opID;
		clientAppendOp(packet);

		SUPERBASE_PACKET * dependent = findDataDependency(node);
		if (dependent == packet)		// make sure there aren't any packets in between us
			packet->parentOp = 0;
		
		if (packet->parentOp == 0)
			doCommand(packet);
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientRelocateOp (SUBPACKET_HEADER *header)
{
	U32 * pTargetOp = (U32 *) (header+1);		// get our data
	SUPERBASE_PACKET * node = findOp(*pTargetOp);
	ObjSet set;

	CQASSERT(node && "Queued command terminated early!?");

	initSetFromGroupID(set, header->groupID);

	if (node->parentOp == 0)
	{
		if (node->isProcess() && node->bCancelDisabled)
		{
			if (CQFLAGS.bTraceMission)
			{
				OPPRINT0("Relocaten Process already in action!?: ");
				node->print();
			}
			CQASSERT(node->isProcess() == 0);		// can't have a running process here!
		}
		notifySetofCancel(node->opID, set);
	}
	else
		onRemovalFromOp(set);
	node->getSet() -= set;
	recalcDependencies(node->opID);

	SUPERBASE_PACKET * pRelOp = duplicatePacket(node);
	pRelOp->getSet() = set;
	pRelOp->opID = header->opID;
	pRelOp->parentOp = 0;

	clientAppendOp(pRelOp);

	if (pRelOp->parentOp == 0)
		doCommand(pRelOp);			// this should never happen
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientCancelQueued (SUBPACKET_HEADER *header)
{
	U32 * pTargetOp = (U32 *) (header+1);		// get our data
	SUPERBASE_PACKET * node = findOp(*pTargetOp);

	if (node)
	{
		ObjSet set;
		initSetFromGroupID(set, header->groupID);

		if (node->parentOp == 0)
		{
			notifySetofCancel(node->opID, set);		// calls onRemovalFromOp()
		}
		else
			onRemovalFromOp(set);
		node->getSet() -= set;
		recalcDependencies(node->opID);
	}
}
//--------------------------------------------------------------------------//
//
SUPERBASE_PACKET * OpAgent::clientCreateSuperPacket (const SUBPACKET_HEADER * header)
{
	SUPERBASE_PACKET * result;
	U32 size = header->size - sizeof(SUBPACKET_HEADER) + sizeof(BASE_PACKET) + sizeof(ObjSet) + SUPERBASE_OVERHEAD;

	result = (SUPERBASE_PACKET *) calloc(size, 1);

	// copy the "additional materials"
	memcpy(result->castBasePacket()+1, header+1, header->size - sizeof(SUBPACKET_HEADER));
	result->pSet = (ObjSet *) ( ((U8 *)result) + size - sizeof(ObjSet));

	initSetFromGroupID(result->getSet(), header->groupID);
	result->opID = header->opID;
	result->ctrlBits = ((header->cmd & CONTROL_ON_FLAG) != 0);
	result->hostCmd = (HOSTCMD)(header->cmd & (~CONTROL_ON_FLAG));
	result->groupID = header->groupID;

	return result;
}
//--------------------------------------------------------------------------//
// data packets need to depend on the last non-data packet issued. This ensures that 
// cancel packets are scheduled after the last data dependency. (Cancel packets are
// only sent for non-data packets.)
//
// non-data packets need to depend on the first matching packet. If we were to match against the last
// packet, and that last packet gets cancelled before the first completed, then we would be activated early.
//
void OpAgent::clientAppendOp (SUPERBASE_PACKET * packet)
{
	// 
	// first see if there is a parent op
	//
	
	SUPERBASE_PACKET *node = pOpList, *prev=0;

	// this must be valid, so that others can use us as a parent later
	CQASSERT(packet->opID != 0);	// is this a valid id?

	if (packet->parentOp==0 && packet->isUserData()==0)  // if we don't have a parent already, look for one
	{
		ObjSet packetSet = packet->getSet();
		while (node)
		{
			if ((packetSet & node->getSet()).numObjects > 0)
			{
				packet->parentOp = node->opID;
				break;  // we depend only on the first packet (see comments, above)
			}
			prev = node;
			node = node->pNext;
		}

		//
		// if we failed to find a parent, and the packet is directed toward a subordinate object,
		// then check against the root object too
		if (packet->parentOp == 0 && packetSet.numObjects>0 && (packetSet.objectIDs[0] & SUBORDID_MASK) != 0)
		{
			node = pOpList;
			prev = 0;

			packetSet.objectIDs[0] &= ~SUBORDID_MASK;
			while (node)
			{
				if ((packetSet & node->getSet()).numObjects > 0)
				{
					packet->parentOp = node->opID;
					break;  // we depend only on the first packet (see comments, above)
				}
				prev = node;
				node = node->pNext;
			}
		}
	}

	//
	// now find the end of the list
	//
	while (node)
	{
		prev = node;
		node = node->pNext;
	}

	appendToOpList(packet, prev);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientMoveTo (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientAttack (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientSPAttack (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientPrejump (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientJump (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)	// this should always be non-zero
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientEnableJump (SUBPACKET_HEADER *header)
{
	U32 * pOp = (U32 *) (header+1);
	SUPERBASE_PACKET * node = findOp(*pOp);

	if (node)
	{
		CQASSERT(node->hostCmd == JUMP);
		CQASSERT(node->castJump()->bEnabled == false);

		node->castJump()->bEnabled = true;

		if (node->parentOp == 0)	// this should always be non-zero
			doCommand(node);
	}
	else
		CQTRACEM0("Discarding ENABLEJUMP packet!");
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientObjSync (SUBPACKET_HEADER *header)
{
	U8 * buffer = (U8 *) (header+1);		// get user data
	U32 size = header->size - sizeof(SUBPACKET_HEADER) - sizeof(U32);		// size of user data
	U32 parentOp = ((U32 *)(buffer+size))[0];
	SUPERBASE_PACKET * node = findOp(parentOp);		// find the parent op

	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);
	packet->hostCmd = OBJSYNC;

	clientAppendOp(packet);
	if (node)
		packet->parentOp = node->opID;
	else
		packet->parentOp = 0;

	CQASSERT(node==0 || node->hostCmd!=JUMP || header->cmd == OBJSYNC2);	// can't have a ACTIVE jump for a parent

	// if cmd == OBJSYNC2, host was idle when data was sent. wait until all local ops complete.
	// note: this means that data can depend on a running op
	if (header->cmd == OBJSYNC2)
		packet->bDelayDependency = true;

	if (node && node->parentOp==0)  // if our parent is already running, execute now
	{
		SUPERBASE_PACKET * dependency = findDataDependency(node);
		if (dependency == packet)		// make sure there aren't any packets in between us
			packet->parentOp = 0;
	}
	
	if (packet->parentOp == 0)
	{
		if (OBJLIST->FindObject(packet->getSet().objectIDs[0]) == 0)
		{
			if (CQFLAGS.bTraceMission != 0)
				OPPRINT1("OBJSYNC packet received for 0x%X, trying to fix it up...\n", packet->getSet().objectIDs[0]);
			//
			// this object might be a special weapon, use the same check to queue behind creation of parent object
			//
			SUPERBASE_PACKET * node = findFirstAgent(MPartNC(), packet->getSet().objectIDs[0] & ~SUBORDID_MASK);

			if (node && node->hostCmd==ADDTO)
			{
				U32 * pOp = (U32 *) (node->getOperands());		// get op that will create us

				if (findOp(*pOp))
				{
					packet->parentOp = *pOp;
					packet->bDelayDependency = false;		// must be not false, so as to not block other SYNC data
					if (CQFLAGS.bTraceMission != 0)
						OPPRINT2("      OBJSYNC packet #%d dependent on #%d\n", packet->opID, *pOp);
				}
			}
			else
			if (node && node->hostCmd==PROCESS)
			{
				packet->parentOp = node->opID;
				packet->bDelayDependency = false;		// must be not false, so as to not block other SYNC data
				if (CQFLAGS.bTraceMission != 0)
					OPPRINT2("      OBJSYNC packet #%d dependent on #%d\n", packet->opID, node->opID);
			}
			else
			if (node)
			{
				CQERROR0("      Found a node, but not an ADDTO or PROCESS\r\n");
			}
		}
	}

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientOpData (SUBPACKET_HEADER *header)
{
	U8 * buffer = (U8 *) (header+1);		// get user data
	U32 size = header->size - sizeof(SUBPACKET_HEADER) - sizeof(U32);		// size of user data
	U32 agentID = ((U32 *)(buffer+size))[0];
	SUPERBASE_PACKET * node = findOp(agentID);		// find the parent op

	if (node)		
	{
		SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

		packet->hostCmd = OPDATA;
		packet->parentOp = node->opID;
		clientAppendOp(packet);

		CQASSERT(agentID != packet->opID);  // should not be creating a new operation

		if (node && node->parentOp==0)  // if our parent is already running, execute now
		{
			SUPERBASE_PACKET * dependency = findDataDependency(node);
			if (dependency == packet)		// make sure there aren't any packets in between us
				packet->parentOp = 0;
			/*
			else
			if (CQFLAGS.bTraceMission != 0)
				OPPRINT1("Middle dependency detected for OPDATA packet, PROCESS #%d. Queuing it up.\n", node->opID);
			*/
		}

		if (packet->parentOp == 0)
			doCommand(packet);
	}
	else
	if (header->cmd == OPDATA)
		CQBOMB1("Can't find PROCESS #%d for OPDATA packet", agentID);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientCreateProcess (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientStop (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientEnableDeath (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * node = findDependentItemForDeath(NULL, header->groupID);		// find the parent op

	if (node)		// data is still pending, schedule for later
	{
		// queue it up, like the rest
		SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

		if (node->isUserData())
		{
			if ((packet->parentOp = node->parentOp) == 0)		// slim chance that parent might be 0
				packet->parentOp = node->opID;
		}
		else
			packet->parentOp = node->opID;
		packet->opID = header->opID;
		clientAppendOp(packet);

		if (packet->parentOp == 0)
			doCommand(packet);

		if (CQFLAGS.bTraceMission != 0)
			OPPRINT2("Queuing DEATH for Part 0x%X, parentOp=#%d.\n", header->groupID, node->opID);
	}
	else
	{
		U32 attackerID = ((U32 *)(header+1))[0];
		objectTerminated(header->groupID, attackerID);

		if (CQFLAGS.bTraceMission != 0)
			OPPRINT1("Immediate DEATH for Part 0x%X\n", header->groupID);
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientAOEAttack (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientArtifactTargeted (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientWormAttack (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientProbe (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientRecover (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientDropOff (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientCreateWormhole (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}//--------------------------------------------------------------------------//
//
void OpAgent::clientFabricate (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientFabricateJump (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientFabricatePos (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
// data in this case should be used directly instead of converting to a packet
void OpAgent::clientSyncMoney (SUBPACKET_HEADER *header)
{
	SUBPACKET_MONEY * pMoney = (SUBPACKET_MONEY *) (header+1);

	pMoney->set(netGas,netMetal,netCrew,netTotalComPts,netUsedComPts);		// set the values in the global structure
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientSyncVisibility (SUBPACKET_HEADER *header)
{
	SUBPACKET_VISIBILITY * pVis = (SUBPACKET_VISIBILITY *) (header+1);

	memcpy(netVisibility, pVis->visibility, sizeof(netVisibility));
	pVis->set();		// set the values in the global structure
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientSyncAlliance (SUBPACKET_HEADER *header)
{
	SUBPACKET_ALLIANCE * pAlly = (SUBPACKET_ALLIANCE *) (header+1);

	memcpy(netAlliance, pAlly->alliance, sizeof(netAlliance));
	pAlly->set();		// set the values in the global structure
	//
	// update the zone
	//
	if (ZONESCORE!=0)
	{
		int i;
		for (i = 1; i <= MAX_PLAYERS; i++)
		{
			U32 slots[MAX_PLAYERS+1];
			U32 numSlots;

			if ((numSlots = MGlobals::GetSlotIDForPlayerID(i, slots)) != 0)
			{
				U32 allyMask = MGlobals::GetAllyMask(i);
				U32 j;
				for (j = 0; j < numSlots; j++)
					ZONESCORE->SetPlayerTeam(MGlobals::GetZoneSeatFromSlot(slots[j]), allyMask);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientSyncScores (SUBPACKET_HEADER *header)
{
	U8 *pTmp = (U8 *) (header+1);
	U32 size = header->size - sizeof(*header);

	CQASSERT(size <= SCOREBUFFER_SIZE);
	memcpy(scoreBuffer, pTmp, size);
	MGlobals::SetGameScores(pTmp, size);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientSyncNuggets (SUBPACKET_HEADER *header)
{
	U8 *pTmp = (U8 *) (header+1);
	U32 size = header->size - sizeof(*header);

	NUGGETMANAGER->PutSyncData(pTmp, size);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientNuggetData (SUBPACKET_HEADER *header)
{
	U8 *pTmp = (U8 *) (header+1);
	U32 size = header->size - sizeof(*header);

	NUGGETMANAGER->ReceiveNuggetData(pTmp, size);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientNuggetDeath (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientPlatformDeath (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientHarvest (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientAddTo (SUBPACKET_HEADER *header)
{
	U32 * pOp = (U32 *) (header+1);		// get user data
	U32 agentID = *pOp;
	SUPERBASE_PACKET * node = findOp(agentID);		// find the parent op

	if (node)		
	{
		SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);
		CQASSERT(node->isProcess());

		packet->parentOp = agentID;
		clientAppendOp(packet);

		// if our process is already running, and there are no middle dependencies, do some more checking...
		if (node->parentOp==0)
		{
			SUPERBASE_PACKET * dependency = findDataDependency(node);
			if (dependency == packet)		// make sure there aren't any DATA packets in between us
				packet->parentOp = 0;
		}

		if (packet->parentOp == 0)
			doCommand(packet);
	}
	else
		CQBOMB1("Can't find PROCESS #%d for ADDTO packet!", node->opID);
}
//--------------------------------------------------------------------------//
// not tightly synchronized, but should not matter
//
void OpAgent::clientRally (SUBPACKET_HEADER *header)
{
	setRallyPoint(header->groupID, *((NETGRIDVECTOR *)(header+1)));
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientSetFleetDef (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientEscort (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientDock (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientUndock (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientMigrationComplete (SUBPACKET_HEADER *header)
{
	// ignore migration packets if we are playing back from log file
	if (clientPlaybackBuffer == 0)
	{
		bRealMigration = bHostMigration = false;
		if (CQFLAGS.bTraceMission != 0)
			OPPRINT0("Host Migration completed!\n");
		EVENTSYS->Send(CQE_HOST_MIGRATE, 0);
	}
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientResupply (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientFabRepair (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientShipRepair (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientCapture (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientFabSalvage (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void OpAgent::clientPatrol (SUBPACKET_HEADER *header)
{
	SUPERBASE_PACKET * packet = clientCreateSuperPacket(header);

	clientAppendOp(packet);

	if (packet->parentOp == 0)
		doCommand(packet);
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doCommand (void)
{
	if (getSet().numObjects>0)
	{
		switch (hostCmd)
		{
		case MOVETO:
			doMove();
			break;
		case ATTACK:
			doAttack();
			break;
		case SPATTACK:
			doSpecialAttack();
			break;
		case PREJUMP:
			doPrejump();
			break;
		case JUMP:
			doJump();
			break;
		case STOP:
			doStop();
			break;
		case OBJSYNC:
			doObjSync();
			break;
		case ENABLEDEATH:
			doDeath();
			break;
		case AOEATTACK:
			doAOEAttack();
			break;
		case ARTIFACTTARGETED:
			doArtifactTargeted();
			break;
		case WORMATTACK:
			doWormAttack();
			break;
		case PROBE:
			doProbe();
			break;
		case RECOVER:
			doRecover();
			break;
		case DROP_OFF:
			doDropOff();
			break;
		case CREATEWORMHOLE:
			doCreateWormhole();
			break;
		case PROCESS:
			doProcess();
			break;
		case OPDATA:
			doOpData();
			break;
		case CANCEL:
			doCancel();
			break;
		case FABRICATE:
			doFabricate();
			break;
		case FABRICATE_JUMP:
			doFabricateJump();
			break;
		case FABRICATE_POS:
			doFabricatePos();
			break;
		case HARVEST:
			doHarvest();
			break;
		case ADDTO:
			doAdd();
			break;
		case ESCORT:
			doEscort();
			break;
		case DOCKFLAGSHIP:
			doDock();
			break;
		case UNDOCKFLAGSHIP:
			doUndock();
			break;
		case RESUPPLY:
			doResupply();
			break;
		case FABREPAIR:
			doFabRepair();
			break;
		case SHIPREPAIR:
			doShipRepair();
			break;
		case CAPTURE:
			doCapture();
			break;
		case SALVAGE:
			doFabSalvage();
			break;
		case PATROL:
			doPatrol();
			break;
		case NUGGETDEATH:
			doNuggetDeath();
			break;
		case PLATFORMDEATH:
			doPlatformDeath();
			break;
		case FLEETDEF:
			doFleetDef();
			break;

		default:
			CQBOMB0("Unknown command type!?");
			break;
		}	// end switch
	}
}
//--------------------------------------------------------------------------//
// assigns groupID to units, converts the set back into object pointers
//
typedef IBaseObject *OBJARRAY[MAX_SELECTED_UNITS];
static void assignGroupID (const ObjSet & set, OBJARRAY  & obj, U32 groupID)
{
	U32 i;
	MPartNC part;

	memset(obj, 0, sizeof(obj));
	CQASSERT(sizeof(obj) == sizeof(IBaseObject *) * MAX_SELECTED_UNITS);
	for (i = 0; i < set.numObjects; i++)
	{
		if ((obj[i] = OBJLIST->FindObject(set.objectIDs[i])) != 0)
		{
			if ((part = obj[i]).isValid())
			{
				part->groupID = groupID;
				part->groupIndex = i;

				OpAgent::onRemovalFromOp(part);		// clear cached op, object may not actually support the command
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
static void DEBUG_traceCommand (const SUPERBASE_PACKET * packet)
{
	if (CQFLAGS.bTraceMission != 0)
	{
		OPPRINT0("Dispatching agent: ");
		packet->print();
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doAttack (void)
{
	OBJPTR<IAttack> attack;
	ObjSet & set = getSet();
	U32 i;
	const USRATTACK * const pAttack = castAttack();
	const bool bUserGenerated = pAttack->bUserGenerated;
	OBJPTR<IBaseObject> victim;
	OBJLIST->FindObject(pAttack->targetID, set.objectIDs[0] & PLAYERID_MASK, victim);

	if (pAttack->targetID && victim==0 && (parentOp = ((OpAgent *)THEMATRIX)->findPendingBirth(pAttack->targetID)) != 0)
	{
		bStalled = true;
	}
	else
	{
		const ObjSet tmpSet = set;
		IBaseObject * obj[MAX_SELECTED_UNITS];
		assignGroupID(set, obj, groupID);
		DEBUG_traceCommand(this);
		bStalled = false;

		// must deliver message to object even if victim is NULL because unit may need to send data
		// on the operation for closure.

		for (i = 0; i < tmpSet.numObjects; i++)
		{
			if (obj[i]==0 || obj[i]->QueryInterface(IAttackID, attack)==0)
				set.removeObject(tmpSet.objectIDs[i]);
			else
			{
				attack->Attack(victim, opID, bUserGenerated);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doSpecialAttack (void)
{
	OBJPTR<IAttack> attack;
	ObjSet & set = getSet();
	U32 i;
	const USRATTACK * const pAttack = castAttack();
	OBJPTR<IBaseObject> victim;
	OBJLIST->FindObject(pAttack->targetID, set.objectIDs[0] & PLAYERID_MASK, victim);

	if (pAttack->targetID && victim==0 && (parentOp = ((OpAgent *)THEMATRIX)->findPendingBirth(pAttack->targetID)) != 0)
	{
		bStalled = true;
	}
	else
	{
		const ObjSet tmpSet = set;
		IBaseObject * obj[MAX_SELECTED_UNITS];
		assignGroupID(set, obj, groupID);
		DEBUG_traceCommand(this);
		bStalled = false;
		
		for (i = 0; i < tmpSet.numObjects; i++)
		{
			if (obj[i]==0 || obj[i]->QueryInterface(IAttackID, attack)==0)
				set.removeObject(tmpSet.objectIDs[i]);
			else
				attack->SpecialAttack(victim, opID);
		}
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doMove (void)
{
	OBJPTR<IGotoPos> gotopos;
	ObjSet & set = getSet();
	U32 i;
	const ObjSet tmpSet = set;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);
	GRIDVECTOR gpos = (GRIDVECTOR)( castMove()->position);
	
	for (i = 0; i < tmpSet.numObjects; i++)
	{
		if (obj[i]==0 || obj[i]->QueryInterface(IGotoPosID, gotopos)==0)
			set.removeObject(tmpSet.objectIDs[i]);
		else
			gotopos->GotoPosition(gpos, opID, ctrlBits);
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doPrejump (void)
{
	OBJPTR<IGotoPos> gotopos;
	ObjSet & set = getSet();
	U32 i;
	HSTPREJUMP * pre = castPreJump();
	IBaseObject * jumpgate = OBJLIST->FindObject(pre->jumpgateID);
	CQASSERT(jumpgate);
	const ObjSet tmpSet = set;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);

	for (i = 0; i < tmpSet.numObjects; i++)
	{
		if (obj[i]==0 || obj[i]->QueryInterface(IGotoPosID, gotopos)==0)
			set.removeObject(tmpSet.objectIDs[i]);
		else
			gotopos->PrepareForJump(jumpgate, pre->bUserInitiated, opID, ctrlBits);
	}
}
//--------------------------------------------------------------------------//
//
bool setIgnoreJump(ObjSet & set)
{
	for(U32 count = 0; count < set.numObjects; ++count)
	{
		IBaseObject * obj = OBJLIST->FindObject(set.objectIDs[count]);
		if(obj)
		{
			if(!obj->effectFlags.canIgnoreInhibitors())
			{
				return false;
			}
		}
	}
	return true;
}

void SUPERBASE_PACKET::doJump (void)
{
	HSTJUMP * const jmpPtr = castJump();
	OpAgent * const THEMATRIX = ((OpAgent *)::THEMATRIX);
	
	// can be host and have jump already enabled after a host migration
	if (THEMATRIX->IsMaster() && jmpPtr->bEnabled==false)
	{
		//
		// test to see if we have reached unit limits
		//
		ObjSet & set = getSet();

		if (set.numObjects > 0)
		{
			IBaseObject * jumpgate = OBJLIST->FindObject(jmpPtr->jumpgateID);
			CQASSERT(jumpgate);
			IBaseObject * ingate = SECTOR->GetJumpgateDestination(jumpgate);
			CQASSERT(ingate);
//			U32 systemID = ingate->GetSystemID();

			OBJPTR<IJumpGate> gate;
			jumpgate->QueryInterface(IJumpGateID,gate);
			if(!(gate->PlayerCanJump(MGlobals::GetPlayerFromPartID(set.objectIDs[0]))|| setIgnoreJump(set) ))
			{
				SUPERBASE_PACKET * node = pNext;

				THEMATRIX->sendCancel(opID, set, true);		// calls onRemovalFromOp()

				//
				// cancel all dependent actions for this set
				//
				while (node)
				{
					ObjSet tmpSet = node->getSet() & set;
					if (tmpSet.numObjects > 0)
					{
						THEMATRIX->sendCancel(node->opID, tmpSet, node->parentOp!=0);
						if (node->parentOp==0)	// is it active?
							THEMATRIX->notifySetofCancel(node->opID, tmpSet);
						node->getSet() -= tmpSet;
						THEMATRIX->recalcDependencies(node->opID);
					}

					node = node->pNext;
				}
				set.numObjects = 0;
				return;
			}
			
			jmpPtr->bEnabled = true;
			//
			// send ENABLED message to the clients
			THEMATRIX->bufferedSend(ENABLEJUMP, THEMATRIX->allocOpID(), &opID, sizeof(U32), getSet());
		}
	}

	if (jmpPtr->bEnabled)
	{
		OBJPTR<IGotoPos> gotopos;
		ObjSet & set = getSet();
		U32 i;
		IBaseObject * jumpgate = OBJLIST->FindObject(jmpPtr->jumpgateID);
		CQASSERT(jumpgate);
		IBaseObject * ingate = SECTOR->GetJumpgateDestination(jumpgate);
		RECT rect;
		Vector vec;
		SINGLE heading;
		const ObjSet tmpSet = set;
		IBaseObject * obj[MAX_SELECTED_UNITS];
		assignGroupID(set, obj, groupID);
		DEBUG_traceCommand(this);
	
		SECTOR->GetSystemRect(ingate->GetSystemID(), &rect);
		vec.x = rect.right/2;
		vec.y = rect.top / 2;
		vec.z = 0;
		vec -= ingate->GetPosition();
		vec.normalize();
		heading = TRANSFORM::get_yaw(vec);
		vec *= 5000;
		vec += ingate->GetPosition();

		for (i = 0; i < tmpSet.numObjects; i++)
		{
			if (obj[i]==0 || obj[i]->QueryInterface(IGotoPosID, gotopos)==0)
				set.removeObject(tmpSet.objectIDs[i]);
			else
			{
				if (tmpSet.numObjects > 1)
					gotopos->UseJumpgate(jumpgate, ingate, vec+formation[i], heading, 500, opID);
				else
					gotopos->UseJumpgate(jumpgate, ingate, vec, heading, 500, opID);

			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doObjSync (void)
{
	ObjSet & set = getSet();

	if (set.numObjects)
	{
		VOLPTR(IMissionActor) actor = OBJLIST->FindObject(set.objectIDs[0]);
		OpAgent * const THEMATRIX = ((OpAgent *)::THEMATRIX);
		void * const operands = getOperands();

		if (actor)
		{
			//
			// Determine if this is an on-time delivery
			//
			U32 * targetOp = (U32 *) (((U8 *)operands) + getOperandSize() - sizeof(32));
			bool bLateDelivery = false;

			if (bDelayDependency==0)
			{
				//
				// unit must be currently working on the target operation
				//
				SUPERBASE_PACKET * pTarget = THEMATRIX->findOp(*targetOp);

				if (pTarget == NULL || pTarget->getSet().contains(set.objectIDs[0]) == false)
					bLateDelivery = true;
			}
			else
			{
				// firstFirstAgent() for unit should not be running, and lastOpCompleted == *targetOp
				MPartNC part = actor.Ptr();
				SUPERBASE_PACKET * pWorking = THEMATRIX->findFirstAgent(part, set.objectIDs[0]);
				CQASSERT(pWorking==0 || pWorking->isUserData()==0);

				if ((pWorking && pWorking->parentOp==0) || part->lastOpCompleted != *targetOp)
					bLateDelivery = true;
			}

			if (bLateDelivery && CQFLAGS.bTraceMission)
				OPPRINT3("Late delivery of OBJSYNC #%d->#%d for unit 0x%X\n", opID, *targetOp, set.objectIDs[0]);

			DEBUG_traceCommand(this);
			
			actor->PutGeneralSyncData(operands, getOperandSize() - sizeof(U32), bLateDelivery);		// subtract our piggyback data
		}
		else
		{
			CQBOMB4("SyncPckt #%d arrived for unknown object 0x%X, GameActive=%d, UpdateCount=%d\n", opID, set.objectIDs[0], CQFLAGS.bGameActive, MGlobals::GetUpdateCount());
		}
	}

	set.numObjects = 0;		// this command is done!
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doOpData (void)
{
	ObjSet & set = getSet();

	if (set.numObjects)
	{
		IBaseObject * obj = OBJLIST->FindObject(set.objectIDs[0]);
		OBJPTR<IMissionActor> actor;

		if (obj && obj->QueryInterface(IMissionActorID, actor))
		{
			U8 * buffer = (U8 *) getOperands();
			U32 size = getOperandSize() - sizeof(U32);		// subtract out piggyback data
			U32 agentID = ((U32 *)(buffer+size))[0];

			DEBUG_traceCommand(this);
			actor->ReceiveOperationData(agentID, (size)?buffer:0, size);
		}
		else
		if (MGlobals::IsHost()==false)
			CQBOMB1("OPDATA packet arrived for dead object 0x%X", set.objectIDs[0]);
	}

	set.numObjects = 0;		// this command is done!
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doProcess (void)
{
	ObjSet & set = getSet();

	if (set.numObjects)
	{
		IBaseObject * obj = OBJLIST->FindObject(set.objectIDs[0]);
		OBJPTR<IMissionActor> actor;

		if (obj && obj->QueryInterface(IMissionActorID, actor))
		{
			U8 * buffer = (U8 *) getOperands();
			U32 size = getOperandSize();

			DEBUG_traceCommand(this);
			actor->OnOperationCreation(opID, (size)?buffer:0, size);
			return;	// do not mark command as done
		}
		else
		if (MGlobals::IsHost()==false)
			CQBOMB1("PROCESS packet arrived for dead object 0x%X", set.objectIDs[0]);
	}

	set.numObjects = 0;		// this command is done!
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doCancel (void)
{
	OpAgent * const THEMATRIX = ((OpAgent *)::THEMATRIX);
	U32 * pCancelOp = (U32 *) getOperands();
	CQASSERT(getOperandSize() == sizeof(U32));
	SUPERBASE_PACKET * node = THEMATRIX->findOp(*pCancelOp);
	ObjSet & set = getSet();

	if (node)
	{
		DEBUG_traceCommand(this);
		if (node->parentOp==0)	// if op was started
			THEMATRIX->notifySetofCancel(node->opID, set);
		node->getSet() -= set;
		THEMATRIX->onRemovalFromOp(set);
		THEMATRIX->recalcDependencies(node->opID);
	}
	else
		CQTRACEM1("Op already dead for queued CANCEL packet, agent #%d", *pCancelOp);

	set.numObjects = 0;		// this command is done!
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doStop (void)
{
	OBJPTR<IAttack> attack;
	OBJPTR<IPlatform> platform;
	ObjSet & set = getSet();
	const ObjSet tmpSet = set;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);

	U32 i;
	for (i = 0; i < tmpSet.numObjects; i++)
	{
		if (obj[i]==0)
			set.removeObject(tmpSet.objectIDs[i]);
		else
		{
			if (obj[i]->QueryInterface(IAttackID, attack))
				attack->CancelAttack();
			if (obj[i]->QueryInterface(IPlatformID, platform))
				platform->StopActions();	
		}
	}

	set.numObjects = 0;		// this command is done!
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doDeath (void)
{
	OpAgent * const THEMATRIX = ((OpAgent *)::THEMATRIX);
	ObjSet & set = getSet();

	if (set.numObjects)
	{
		SUPERBASE_PACKET * node = THEMATRIX->findDependentItemForDeath(this, set.objectIDs[0]);		// find the parent op

		if (node)		// data is still pending, schedule for later
		{
			if (node->isUserData())
			{
				if ((parentOp = node->parentOp) == 0)		// slim chance that parent might be 0
					parentOp = node->opID;
			}
			else
				parentOp = node->opID;

			if (CQFLAGS.bTraceMission != 0)
				OPPRINT1("ReQueuing DEATH for Part 0x%X\n", set.objectIDs[0]);
		}
		else
		{
			U32 attackerID = ((U32 *)getOperands())[0];
			CQASSERT(getOperandSize() == sizeof(U32));
			THEMATRIX->objectTerminated(set.objectIDs[0], attackerID);

			DEBUG_traceCommand(this);

			set.numObjects = 0;		// this command is done!
		}
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doAOEAttack (void)
{
	OBJPTR<IAttack> attack;
	ObjSet & set = getSet();
	U32 i;
	GRIDVECTOR vec = castAOEAttack()->position;
	bool bSpecial = castAOEAttack()->bSpecial;
	const ObjSet tmpSet = set;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);
	
	for (i = 0; i < tmpSet.numObjects; i++)
	{
		if (obj[i]==0 || obj[i]->QueryInterface(IAttackID, attack)==0)
			set.removeObject(tmpSet.objectIDs[i]);
		else
		{
			if(bSpecial)
				attack->SpecialAOEAttack(vec, opID);
			else
				attack->AttackPosition(vec, opID);
		}
	}
}//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doArtifactTargeted (void)
{
	ObjSet & set = getSet();
	const ObjSet tmpSet = set;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);

	VOLPTR(IArtifactHolder) holder = obj[0];
	if(holder)
	{
		IBaseObject * target = OBJLIST->FindObject(castArtifactTargeted()->targetID);
		holder->UseArtifactOn(target,opID);
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doWormAttack (void)
{
	OBJPTR<IAttack> attack;
	ObjSet & set = getSet();
	U32 i;
	const USRWORMATTACK * const pAttack = castWormAttack();
	OBJPTR<IBaseObject> victim;
	OBJLIST->FindObject(pAttack->targetID, set.objectIDs[0] & PLAYERID_MASK, victim);

	CQASSERT(victim);//jumpgates should always exist.

	const ObjSet tmpSet = set;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);
	bStalled = false;
	
	for (i = 0; i < tmpSet.numObjects; i++)
	{
		if (obj[i]==0 || obj[i]->QueryInterface(IAttackID, attack)==0)
			set.removeObject(tmpSet.objectIDs[i]);
		else
			attack->WormAttack(victim, opID);
	}
}//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doProbe (void)
{
	OBJPTR<IAttack> attack;
	ObjSet & set = getSet();
	U32 i;
	GRIDVECTOR vec = (GRIDVECTOR)(castProbe()->position);
	const ObjSet tmpSet = set;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);
	
	for (i = 0; i < tmpSet.numObjects; i++)
	{
		if (obj[i]==0 || obj[i]->QueryInterface(IAttackID, attack)==0)
			set.removeObject(tmpSet.objectIDs[i]);
		else
			attack->MultiSystemAttack(vec,castProbe()->position.systemID, opID);
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doRecover (void)
{
	OBJPTR<IRecoverShip> recover;
	ObjSet & set = getSet();
	U32 i;
	OBJPTR<IBaseObject> target;
	OBJLIST->FindObject(castRecover()->targetID, set.objectIDs[0] & PLAYERID_MASK, target);
	const ObjSet tmpSet = set;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);
	
	for (i = 0; i < tmpSet.numObjects; i++)
	{
		if (obj[i]==0 || obj[i]->QueryInterface(IRecoverShipID, recover)==0)
			set.removeObject(tmpSet.objectIDs[i]);
		else
			recover->RecoverWreck(target, opID);
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doDropOff (void)
{
	OBJPTR<IRecoverShip> recover;
	ObjSet & set = getSet();
	U32 i;
	OBJPTR<IBaseObject> target;
	OBJLIST->FindObject(castDropOff()->targetID, set.objectIDs[0] & PLAYERID_MASK, target);

	const ObjSet tmpSet = set;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);
	
	for (i = 0; i < tmpSet.numObjects; i++)
	{
		if (obj[i]==0 || obj[i]->QueryInterface(IRecoverShipID, recover)==0)
			set.removeObject(tmpSet.objectIDs[i]);
		else
			recover->ReturnReck(target, opID);
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doCreateWormhole (void)
{
	OBJPTR<IAttack> attack;
	ObjSet & set = getSet();
	U32 i;
	const ObjSet tmpSet = set;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);
	const USRCREATEWORMHOLE * const pCreateWormhole = castCreateWormhole();
	
	for (i = 0; i < tmpSet.numObjects; i++)
	{
		if (obj[i]==0 || obj[i]->QueryInterface(IAttackID, attack)==0)
			set.removeObject(tmpSet.objectIDs[i]);
		else
			attack->DoCreateWormhole(pCreateWormhole->systemID,opID);
	}
}
//--------------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doFabricate (void)
{
	OBJPTR<IFabricator> fab;
	ObjSet & set = getSet();
	const ObjSet tmpSet = set;
	USRFAB * pFab = castFab();
	U32 i;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);

	for (i = 0; i < tmpSet.numObjects; i++)
	{
		if (obj[i]==0 || obj[i]->QueryInterface(IFabricatorID, fab)==0)
			set.removeObject(tmpSet.objectIDs[i]);
		else
		{
			if (tmpSet.numObjects > 1)
				fab->BeginConstruction(pFab->planetID, pFab->slotID, pFab->dwArchetypeID, opID);
			else
				fab->BeginConstruction(pFab->planetID, pFab->slotID, pFab->dwArchetypeID, opID);
		}
	}
}
//--------------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doFabricateJump (void)
{
	OBJPTR<IFabricator> fab;
	ObjSet & set = getSet();
	const ObjSet tmpSet = set;
	USRFABJUMP * pFab = castFabJump();
	U32 i;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);

	for (i = 0; i < tmpSet.numObjects; i++)
	{
		if (obj[i]==0 || obj[i]->QueryInterface(IFabricatorID, fab)==0)
			set.removeObject(tmpSet.objectIDs[i]);
		else
		{
			if (tmpSet.numObjects > 1)
				fab->BeginConstruction(OBJLIST->FindObject(pFab->jumpgateID), pFab->dwArchetypeID, opID);
			else
				fab->BeginConstruction(OBJLIST->FindObject(pFab->jumpgateID), pFab->dwArchetypeID, opID);
		}
	}
}
//--------------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doFabricatePos (void)
{
	OBJPTR<IFabricator> fab;
	ObjSet & set = getSet();
	const ObjSet tmpSet = set;
	USRFABPOS * pFab = castFabPos();
	U32 i;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);

	for (i = 0; i < tmpSet.numObjects; i++)
	{
		if (obj[i]==0 || obj[i]->QueryInterface(IFabricatorID, fab)==0)
			set.removeObject(tmpSet.objectIDs[i]);
		else
		{
			if (tmpSet.numObjects > 1)
				fab->BeginConstruction(pFab->position, pFab->dwArchetypeID, opID);
			else
				fab->BeginConstruction(pFab->position, pFab->dwArchetypeID, opID);
		}
	}
}
//--------------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doHarvest (void)
{
	OBJPTR<IHarvest> harvest;
	ObjSet & set = getSet();
	const ObjSet tmpSet = set;
	USRHARVEST * pHarvest = castHarvest();
	OBJPTR<IBaseObject> victim;
	OBJLIST->FindObject(pHarvest->targetID, set.objectIDs[0] & PLAYERID_MASK, victim);
	if(!victim)
		NUGGETMANAGER->FindNugget(pHarvest->targetID,set.objectIDs[0] & PLAYERID_MASK, victim,IBaseObjectID);

	if (pHarvest->targetID && victim==0 && (parentOp = ((OpAgent *)THEMATRIX)->findPendingBirth(pHarvest->targetID)) != 0)
	{
		bStalled = true;
	}
	else
	{
		U32 i;
		IBaseObject * obj[MAX_SELECTED_UNITS];
		assignGroupID(set, obj, groupID);
		DEBUG_traceCommand(this);
		bStalled = false;

		for (i = 0; i < tmpSet.numObjects; i++)
		{
			if (obj[i]==0 || obj[i]->QueryInterface(IHarvestID, harvest)==0)
				set.removeObject(tmpSet.objectIDs[i]);
			else
			{
				harvest->BeginHarvest(victim, opID, pHarvest->bAutoSelected);		// watch out! victim can be NULL
			}
		}
	}
}
//--------------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doEscort (void)
{
	OBJPTR<IAttack> attack;
	OBJPTR<ISupplier> supplier;
	ObjSet & set = getSet();
	U32 i;
	USRESCORT * defend = castEscort();
	const ObjSet tmpSet = set;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);
	OBJPTR<IBaseObject> target;
	OBJLIST->FindObject(defend->targetID, set.objectIDs[0] & PLAYERID_MASK, target);

	if (defend->targetID && target==0 && (parentOp = ((OpAgent *)THEMATRIX)->findPendingBirth(defend->targetID)) != 0)
	{
		bStalled = true;
	}
	else
	{
		bStalled = false;

		if (defend->targetID == 0)
		{
			set.numObjects = 0;		// don't know how this would happen
		}
		else
		{
			for (i = 0; i < tmpSet.numObjects; i++)
			{
				if (obj[i]==0 || (obj[i]->QueryInterface(IAttackID, attack)==0 && obj[i]->QueryInterface(ISupplierID, supplier)==0))
					set.removeObject(tmpSet.objectIDs[i]);
				else
				{
					if (attack)
						attack->Escort(target, opID);
					else
						supplier->SetSupplyEscort(opID, target);
				}
			}
		}
	}
}
//--------------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doDock (void)
{
	OBJPTR<IAdmiral> admiral;
	ObjSet & set = getSet();
	U32 targetID = (set.numObjects == 2) ? set.objectIDs[1] : 0;
	IBaseObject * target = OBJLIST->FindObject(targetID);
	
	if (targetID && target==0 && (parentOp = ((OpAgent *)THEMATRIX)->findPendingBirth(targetID)) != 0)
	{
		bStalled = true;
	}
	else
	{
		bStalled = false;
		const ObjSet tmpSet = set;
		IBaseObject * obj[MAX_SELECTED_UNITS];
		assignGroupID(set, obj, groupID);
		DEBUG_traceCommand(this);
		CQASSERT(set.numObjects <= 2);

		if (obj[0]==0 || obj[0]->QueryInterface(IAdmiralID, admiral)==0)
			set.numObjects = 0;
		else
			admiral->DockFlagship(target, opID);
	}
}
//--------------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doUndock (void)
{
	OBJPTR<IAdmiral> admiral;
	ObjSet & set = getSet();
	const ObjSet tmpSet = set;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);
	CQASSERT(set.numObjects <= 2);

	if (obj[0]==0 || obj[0]->QueryInterface(IAdmiralID, admiral)==0)
		set.numObjects = 0;
	else
		admiral->UndockFlagship(opID, set.objectIDs[1]);
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doAdd (void)
{
	OpAgent * const THEMATRIX = ((OpAgent *)::THEMATRIX);
	U32 * pOp = (U32 *) (getOperands());		// get our data
	CQASSERT(getOperandSize() == sizeof(U32));
	SUPERBASE_PACKET * node = THEMATRIX->findOp(*pOp);
	CQASSERT(node);
	CQASSERT(node->isProcess());
	ObjSet & set = node->getSet();
	CQASSERT(getSet().numObjects > 0);
	U32 objectID = getSet().objectIDs[0];
	
	// we have completed all tasks, switch over to PROCESS as our parent
	parentOp = node->opID;	

	if (node->parentOp==0)	// if process is already running
	{
		if (THEMATRIX->findDataDependency(node) == this)
		{
			DEBUG_traceCommand(this);
			
			CQASSERT(set.contains(objectID) == false);
			CQASSERT(set.numObjects < MAX_SELECTED_UNITS && "Client is WAY behind!");
			set.objectIDs[set.numObjects++] = objectID;

			IBaseObject * obj = OBJLIST->FindObject(objectID);
			if(obj)
			{
				OBJPTR<IMissionActor> actor;
				if (obj->QueryInterface(IMissionActorID, actor))
					actor->OnAddToOperation(node->opID);
			}

			OpAgent::onAdditionToOp(set, node->opID);
			getSet().numObjects = 0;
			
			SUPERBASE_PACKET * dependent = THEMATRIX->findDataDependency(node);		// find first dependency

			while (dependent)
			{
				dependent->parentOp = 0;
				THEMATRIX->doCommand(dependent);			// recursion, should only go 1 deep
				dependent = THEMATRIX->findDataDependency(node);		// find first dependency
			}
		}
		else
		{
			/*
			if (CQFLAGS.bTraceMission != 0)
				OPPRINT2("ADDTO #%d switching parents to the PROCESS #%d. Waiting on middle dependency.\n", opID, parentOp);
			*/
		}
	}
	/*
	else
	if (CQFLAGS.bTraceMission != 0)
		OPPRINT2("ADDTO #%d switching parents to the PROCESS #%d. PROCESS hasn't started yet.\n", opID, parentOp);
	*/
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doResupply (void)
{
	OBJPTR<ISupplier> supplier;
	ObjSet & set = getSet();
	U32 i;
	const USRRESUPPLY * const pSupply = castResupply();
	OBJPTR<IBaseObject> victim;
	OBJLIST->FindObject(pSupply->targetID, set.objectIDs[0] & PLAYERID_MASK, victim);

	if (pSupply->targetID && victim==0 && (parentOp = ((OpAgent *)THEMATRIX)->findPendingBirth(pSupply->targetID)) != 0)
	{
		bStalled = true;
	}
	else
	{
		bStalled = false;
		const ObjSet tmpSet = set;
		IBaseObject * obj[MAX_SELECTED_UNITS];
		assignGroupID(set, obj, groupID);
		DEBUG_traceCommand(this);

		for (i = 0; i < tmpSet.numObjects; i++)
		{
			if (obj[i]==0 || obj[i]->QueryInterface(ISupplierID, supplier)==0)
				set.removeObject(tmpSet.objectIDs[i]);
			else
				supplier->SupplyTarget(opID, victim);
		}
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doFabRepair (void)
{
	OBJPTR<IFabricator> fab;
	ObjSet & set = getSet();
	U32 i;
	const USRFABREPAIR * const pRepair = castFabRepair();
	OBJPTR<IBaseObject> victim;
	OBJLIST->FindObject(pRepair->targetID, set.objectIDs[0] & PLAYERID_MASK, victim);

	if (pRepair->targetID && victim==0 && (parentOp = ((OpAgent *)THEMATRIX)->findPendingBirth(pRepair->targetID)) != 0)
	{
		bStalled = true;
	}
	else
	{
		bStalled = false;
		const ObjSet tmpSet = set;
		IBaseObject * obj[MAX_SELECTED_UNITS];
		assignGroupID(set, obj, groupID);
		DEBUG_traceCommand(this);

		for (i = 0; i < tmpSet.numObjects; i++)
		{
			if (obj[i]==0 || obj[i]->QueryInterface(IFabricatorID, fab)==0)
				set.removeObject(tmpSet.objectIDs[i]);
			else
				fab->BeginRepair(opID, victim);
		}
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doShipRepair (void)
{
	OBJPTR<IRepairee> ship;
	ObjSet & set = getSet();
	U32 i;
	const USRSHIPREPAIR * const pRepair = castShipRepair();
	OBJPTR<IBaseObject> victim;
	OBJLIST->FindObject(pRepair->targetID, set.objectIDs[0] & PLAYERID_MASK, victim);

	if (pRepair->targetID && victim==0 && (parentOp = ((OpAgent *)THEMATRIX)->findPendingBirth(pRepair->targetID)) != 0)
	{
		bStalled = true;
	}
	else
	{
		bStalled = false;
		const ObjSet tmpSet = set;
		IBaseObject * obj[MAX_SELECTED_UNITS];
		assignGroupID(set, obj, groupID);
		DEBUG_traceCommand(this);

		for (i = 0; i < tmpSet.numObjects; i++)
		{
			if (obj[i]==0 || obj[i]->QueryInterface(IRepaireeID, ship)==0)
				set.removeObject(tmpSet.objectIDs[i]);
			else
				ship->RepairYourselfAt(victim, opID);
		}
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doCapture (void)
{
	OBJPTR<ITroopship> troop;
	ObjSet & set = getSet();
	U32 i;
	const USRCAPTURE * const pCapture = castCapture();
	OBJPTR<IBaseObject> victim;
	OBJLIST->FindObject(pCapture->targetID, set.objectIDs[0] & PLAYERID_MASK, victim);

	if (pCapture->targetID && victim==0 && (parentOp = ((OpAgent *)THEMATRIX)->findPendingBirth(pCapture->targetID)) != 0)
	{
		bStalled = true;
	}
	else
	{
		const ObjSet tmpSet = set;
		IBaseObject * obj[MAX_SELECTED_UNITS];
		assignGroupID(set, obj, groupID);
		DEBUG_traceCommand(this);
		bStalled = false;

		for (i = 0; i < tmpSet.numObjects; i++)
		{
			if (obj[i]==0 || obj[i]->QueryInterface(ITroopshipID, troop)==0)
				set.removeObject(tmpSet.objectIDs[i]);
			else
				troop->Capture(victim, opID);
		}
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doFabSalvage (void)
{
	OBJPTR<IFabricator> fab;
	ObjSet & set = getSet();
	U32 i;
	const USRFABSALVAGE * const pSalvage = castFabSalvage();
	OBJPTR<IBaseObject> victim;
	OBJLIST->FindObject(pSalvage->targetID, set.objectIDs[0] & PLAYERID_MASK, victim);

	if (pSalvage->targetID && victim==0 && (parentOp = ((OpAgent *)THEMATRIX)->findPendingBirth(pSalvage->targetID)) != 0)
	{
		bStalled = true;
	}
	else
	{
		bStalled = false;
		const ObjSet tmpSet = set;
		IBaseObject * obj[MAX_SELECTED_UNITS];
		assignGroupID(set, obj, groupID);
		DEBUG_traceCommand(this);

		for (i = 0; i < tmpSet.numObjects; i++)
		{
			if (obj[i]==0 || obj[i]->QueryInterface(IFabricatorID, fab)==0)
				set.removeObject(tmpSet.objectIDs[i]);
			else
				fab->BeginDismantle(opID, victim);
		}
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doPatrol (void)
{
	OBJPTR<IGotoPos> gotopos;
	ObjSet & set = getSet();
	U32 i;
	const PATROL_PACKET * const pGoto = castPatrol();

	const ObjSet tmpSet = set;
	IBaseObject * obj[MAX_SELECTED_UNITS];
	assignGroupID(set, obj, groupID);
	DEBUG_traceCommand(this);
	bStalled = false;

	for (i = 0; i < tmpSet.numObjects; i++)
	{
		if (obj[i]==0 || obj[i]->QueryInterface(IGotoPosID, gotopos)==0)
			set.removeObject(tmpSet.objectIDs[i]);
		else
			gotopos->Patrol(pGoto->patrolStart, pGoto->patrolEnd, opID);
	}
}
//--------------------------------------------------------------------------//
//
//
void SUPERBASE_PACKET::doFleetDef (void)
{
	VOLPTR(IAdmiral) admiral;
	ObjSet & set = getSet();

	if (set.numObjects && (admiral = OBJLIST->FindObject(set.objectIDs[0])) != 0)
	{
		admiral->SetFleetMembers(set.objectIDs, set.numObjects);
	}

	OpAgent::onRemovalFromOp(set);
	set.numObjects = 0;
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doNuggetDeath (void)
{
	ObjSet & set = getSet();
	if (set.numObjects)
	{
		DEBUG_traceCommand(this);
		NUGGETMANAGER->ReceiveNuggetDeath(set.objectIDs[0], (getOperandSize() ? getOperands() : NULL), getOperandSize());
		set.numObjects = 0;
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::doPlatformDeath (void)
{
	ObjSet & set = getSet();
	if (set.numObjects)
	{
		DEBUG_traceCommand(this);
		VOLPTR(IPlatform) platform = OBJLIST->FindObject(set.objectIDs[0]);
		CQASSERT(platform); //tried to kill a pltform that was not there
		platform->ReceiveDeathPacket();
		OpAgent::onRemovalFromOp(set);
		set.numObjects = 0;
	}
}
//--------------------------------------------------------------------------//
//
void SUPERBASE_PACKET::print (void) const
{
	OPPRINT4("OpID = %d   %s %s%s\n  ", opID, 
		((bCancelDisabled)?"CANCELDISABLED":""), 
		((bStalled)?"STALLED":""), 
		((bLongTerm)?"LONGTERM":"") );

	switch (hostCmd)
	{
	case NOTHING:
		OPPRINT0("NOTHING\n");
		break;
	case CANCEL:
		OPPRINT1("CANCEL -> AGENT #%d\n", ((U32 *) getOperands())[0]);
		break;
	case NEWGROUP:
		OPPRINT0("NEWGROUP\n");
		break;
	case ENABLEJUMP:
		OPPRINT0("ENABLEJUMP\n");
		break;
	case MOVETO:
		OPPRINT0("MOVETO\n");
		break;
	case ATTACK:
		OPPRINT3("ATTACK  ->  0x%X \"%s\"%s\n", castAttack()->targetID, OpAgent::getPartName(castAttack()->targetID), (castAttack()->bUserGenerated)? "[USR]":"");
		break;
	case SPATTACK:
		OPPRINT2("SPATTACK  ->  0x%X \"%s\"\n", castAttack()->targetID, OpAgent::getPartName(castAttack()->targetID));
		break;
	case PREJUMP:
		OPPRINT0("PREJUMP\n");
		break;
	case JUMP:
		OPPRINT0("JUMP\n");
		break;
	case OBJSYNC:
		OPPRINT0("OBJSYNC\n");
		break;
	case STOP:
		OPPRINT0("STOP\n");
		break;
	case ENABLEDEATH:
		OPPRINT0("ENABLEDEATH\n");
		break;
	case AOEATTACK:
		OPPRINT0("AOEATTACK\n");
		break;
	case ARTIFACTTARGETED:
		OPPRINT0("ARTIFACTTARGETED\n");
		break;
	case WORMATTACK:
		OPPRINT0("WORMATTACK\n");
		break;
	case PROBE:
		OPPRINT0("PROBE\n");
		break;
	case RECOVER:
		OPPRINT0("RECOVER\n");
		break;
	case DROP_OFF:
		OPPRINT0("DROP_OFF\n");
		break;
	case OPDATA:
		OPPRINT1("OPDATA -> PROCESS #%d\n", getOpDataTarget());
		break;
	case PROCESS:
		OPPRINT0("PROCESS\n");
		break;
	case FABRICATE:
		OPPRINT0("FABRICATE\n");
		break;
	case FABRICATE_JUMP:
		OPPRINT0("FABRICATE_JUMP\n");
		break;
	case FABRICATE_POS:
		OPPRINT0("FABRICATE_POS\n");
		break;
	case SYNCMONEY:
		OPPRINT0("SYNCMONEY\n");
		break;
	case SYNCVISIBILITY:
		OPPRINT0("SYNCVISIBILITY\n");
		break;
	case SYNCALLIANCE:
		OPPRINT0("SYNCALLIANCE\n");
		break;
	case HARVEST:
		OPPRINT2("HARVEST  ->  0x%X \"%s\"\n", castHarvest()->targetID, OpAgent::getPartName(castHarvest()->targetID));
		break;
	case ADDTO:
		OPPRINT1("ADDTO -> PROCESS #%d\n", ((U32 *) getOperands())[0]);
		break;
	case ESCORT:
		OPPRINT1("DEFEND  ->  0x%X\n", castEscort()->targetID);
		break;
	case DOCKFLAGSHIP:
		OPPRINT1("DOCKFLAGSHIP  ->  0x%X\n", getSet().objectIDs[1]);
		break;
	case UNDOCKFLAGSHIP:
		OPPRINT1("UNDOCKFLAGSHIP  ->  0x%X\n", getSet().objectIDs[1]);
		break;
	case RESUPPLY:
		OPPRINT2("RESUPPLY  ->  0x%X \"%s\"\n", castResupply()->targetID, OpAgent::getPartName(castResupply()->targetID));
		break;
	case FABREPAIR:
		OPPRINT2("FABREPAIR ->  0x%X \"%s\"\n", castFabRepair()->targetID, OpAgent::getPartName(castFabRepair()->targetID));
		break;
	case SHIPREPAIR:
		OPPRINT2("SHIPREPAIR AT 0x%X \"%s\"\n", castShipRepair()->targetID, OpAgent::getPartName(castShipRepair()->targetID));
		break;
	case CAPTURE:
		OPPRINT2("CAPTURE  ->  0x%X \"%s\"\n", castCapture()->targetID, OpAgent::getPartName(castCapture()->targetID));
		break;
	case SALVAGE:
		OPPRINT2("SALVAGE ->  0x%X \"%s\"\n", castFabSalvage()->targetID, OpAgent::getPartName(castFabSalvage()->targetID));
		break;
	case PATROL:
		OPPRINT0("PATROL\n");
		break;
	case NUGGETDEATH:
		OPPRINT0("NUGGETDEATH\n");
		break;
	case PLATFORMDEATH:
		OPPRINT0("PLATFORMDEATH\n");
		break;
	case RELOCATE:
		OPPRINT1("RELOCATE -> PROCESS #%d\n", getRelocateTarget());
		break;
	case FLEETDEF:
		OPPRINT0("FLEETDEF\n");
		break;

	default:
		OPPRINT0("???\n");
		break;
	}

	OPPRINT1("  Parent = %d\n", parentOp);

	const ObjSet & set = getSet();
	U32 i;		
	OPPRINT1("  Set has %d members:\n", set.numObjects);
	
	for (i = 0; i < set.numObjects; i++)
		OPPRINT1("    0x%X\n", set.objectIDs[i]);
}
//--------------------------------------------------------------------------//
//
U32 SUPERBASE_PACKET::getDestSystemID (void) const
{
	switch (hostCmd)
	{
	case CAPTURE:
		{
			IBaseObject * obj = OBJLIST->FindObject(castCapture()->targetID);
			if (obj)
			{
				return obj->GetSystemID() & ~HYPER_SYSTEM_MASK;
			}
		}
		break;

	case ARTIFACTTARGETED:
	case ATTACK:
	case SPATTACK:
	case WORMATTACK:
		{
			IBaseObject * obj = OBJLIST->FindObject(castAttack()->targetID);
			if (obj)
			{
				return obj->GetSystemID() & ~HYPER_SYSTEM_MASK;
			}
		}
		break;

	case MOVETO:
	case AOEATTACK:
		CQASSERT(castMove()->position.systemID && castMove()->position.systemID <= MAX_SYSTEMS);
		return castMove()->position.systemID;
	case FABRICATE:
		{
			IBaseObject * obj = OBJLIST->FindObject(castFab()->planetID);
			CQASSERT(obj);
			return obj->GetSystemID();
		}
		break;
	case FABRICATE_JUMP:
		{
			IBaseObject * obj = OBJLIST->FindObject(castFabJump()->jumpgateID);
			CQASSERT(obj);
			return obj->GetSystemID();
		}
		break;
	case FABRICATE_POS:
		{
			return castFabPos()->position.systemID;
		}
		break;
	case HARVEST:
		{
			IBaseObject * obj = OBJLIST->FindObject(castHarvest()->targetID);
			if(!obj)
				obj = NUGGETMANAGER->FindNugget(castHarvest()->targetID);
			if (obj)
				return obj->GetSystemID();
			else
				return 0;
		}
		break;
	case ESCORT:
		{
			USRESCORT * defend = castEscort();

			if (defend->targetID)
			{
				IBaseObject * obj = OBJLIST->FindObject(defend->targetID);
				if (obj)
				{
					return obj->GetSystemID() & ~HYPER_SYSTEM_MASK;
				}
				else
					return 0;
			}
			else
				return 0;
		}
		break;

	case DOCKFLAGSHIP:
		{
			if (getSet().numObjects>1)
			{
				IBaseObject * obj = OBJLIST->FindObject(getSet().objectIDs[getSet().numObjects-1]);
				if (obj)
				{
					OpAgent * const THEMATRIX = ((OpAgent *)::THEMATRIX);
					return THEMATRIX->getRealSystemID(obj);
				}
				else
					return 0;
			}
			else
				return 0;
		}
		break;

	case UNDOCKFLAGSHIP:
		return castUndock()->position.systemID;

	case FABREPAIR:
	case SHIPREPAIR:
	case SALVAGE:
		{
			USRFABREPAIR * repair = castRepair();
			IBaseObject * obj = OBJLIST->FindObject(repair->targetID);

			if (obj)
				return obj->GetSystemID() & ~HYPER_SYSTEM_MASK;
			else
				return 0;
		}
		break;
	case RECOVER:
	case DROP_OFF:
		{
			USRRECOVER * recover = castRecover();

			IBaseObject * obj = OBJLIST->FindObject(recover->targetID);

			if (obj)
				return obj->GetSystemID() & ~HYPER_SYSTEM_MASK;
			else
				return 0;
		}
		break;

	case HOSTJUMP:
		{
			IBaseObject * obj = OBJLIST->FindObject(castUserJump()->jumpgateID);
			CQASSERT(obj);
			return obj->GetSystemID();
		}
		break;

	default:
		CQBOMB0("Can't get systemID from this packet type");
		break;
	}
	return 0;
}
//-----------------------------------------------------------------------------//
//
int OpAgent::getNumHiddenAdmiralsInSet (const ObjSet & set, U32 systemID)
{
	int result = 0;
	U32 i;
	MPart part;

	for (i = 0; i < set.numObjects; i++)
	{
		if ((part = OBJLIST->FindObject(set.objectIDs[i])).isValid())
		{
			if (part.obj->GetSystemID() == systemID)
				result--;		// don't count object twice
			else
			if (part->admiralID != 0)
				result++;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
// get the real systemID, accounting for jumps in progress
//
U32 OpAgent::getRealSystemID (IBaseObject * obj)
{
	SUPERBASE_PACKET * jumppacket = findLastJump(obj->GetPartID());

	if (jumppacket)
	{
		U32 jumpID = jumppacket->castJump()->jumpgateID;
		IBaseObject * jumpgate = OBJLIST->FindObject(jumpID);
		U32 systemID = 0;
		CQASSERT(jumpgate);
		if (jumpgate)
		{
			jumpgate = SECTOR->GetJumpgateDestination(jumpgate);
			systemID = jumpgate->GetSystemID();
		}
		return systemID;
	}
	else
	{
		return (obj->GetSystemID() & ~HYPER_SYSTEM_MASK);
	}
}
//--------------------------------------------------------------------------//
//
const char * OpAgent::getPartName (U32 dwMissionID)
{
	static char defAnswer[1] = "";
	MPart part = OBJLIST->FindObject(dwMissionID);

	if (part.isValid())
		return part->partName;
	else
		return defAnswer;
}
//--------------------------------------------------------------------------//
//
void OpAgent::printOplist (void)
{
	SUPERBASE_PACKET *node = pOpList;

	OPPRINT0("------------OpAgent Trace out --------------------\n");

	if (node==0)
	{
		OPPRINT0("Empty list\n");
	}
	else
	while (node)
	{
		node->print();
		node = node->pNext;
	}

	OPPRINT0("\n");
}
//--------------------------------------------------------------------------//
//
struct _opagent : GlobalComponent
{
	OpAgent * agent;

	virtual void Startup (void)
	{
		THEMATRIX = agent = new DAComponent<OpAgent>;
		AddToGlobalCleanupList(&THEMATRIX);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise(agent->getBase(), &agent->eventHandle);
			FULLSCREEN->SetCallbackPriority(agent, EVENT_PRIORITY_MISSION);
		}
	}
};

static _opagent agent;

//------------------------------------------------------------------------------------//
//---------------------------------------End OpAgent.cpp------------------------------//
//------------------------------------------------------------------------------------//

