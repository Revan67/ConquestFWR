#ifndef COMMPACKET_H
#define COMMPACKET_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               CommPacket.h                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/CommPacket.h 84    5/07/01 9:22a Tmauer $
*/
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#ifndef NETPACKET_H
#include "NetPacket.h"
#endif

#ifndef GRIDVECTOR_H
#include "GridVector.h"
#endif

#ifndef DBASEDATA_H
#include <DBaseData.h>
#endif

#ifndef FINAL_RELEASE
#ifndef COMMTRACK_H
#include "CommTrack.h"
#endif
#endif

const DWORD * __fastcall dwordsearch (DWORD len, DWORD value, const DWORD * buffer);

#pragma pack (push , 1)

//--------------------------------------------------------------------------//
//
struct USRMOVE : BASE_PACKET
{
	struct NETGRIDVECTOR position;

	USRMOVE (void)
	{
		type = PT_USRMOVE;
	}
};
//--------------------------------------------------------------------------//
//
struct USRFORMATIONMOVE : BASE_PACKET
{
	struct NETGRIDVECTOR position;

	USRFORMATIONMOVE (void)
	{
		type = PT_USRFORMATIONMOVE;
	}
};
//--------------------------------------------------------------------------//
//
struct FORMATIONATTACK : BASE_PACKET
{
	U32 targetID;
	U32 destSystemID;

	FORMATIONATTACK (void)
	{
		type = PT_FORMATIONATTACK;
	}
};
//--------------------------------------------------------------------------//
//
struct USRJUMP : BASE_PACKET
{
	U32 jumpgateID;
	
	USRJUMP (void)
	{
		type = PT_USRJUMP;
	}
};
//--------------------------------------------------------------------------//
//
struct USRATTACK : BASE_PACKET
{
	U32 targetID;
	U32 destSystemID;		// 0 = follow target to ANY remote system to attack it
	bool bUserGenerated;

	USRATTACK (void)
	{
		type = PT_USRATTACK;
		destSystemID = 0;
		bUserGenerated = 0;
	}
};
//--------------------------------------------------------------------------//
//
struct USRWORMATTACK : BASE_PACKET
{
	U32 targetID;

	USRWORMATTACK (void)
	{
		type = PT_USRWORMATTACK;
	}
};
//--------------------------------------------------------------------------//
//
struct USRAOEATTACK : BASE_PACKET
{
	struct NETGRIDVECTOR position;
	bool bSpecial;

	USRAOEATTACK (void)
	{
		type = PT_USRAOEATTACK;
		bSpecial = true;//for backward compatibility
	}
};
//--------------------------------------------------------------------------//
//
struct USEARTIFACTTARGETED : BASE_PACKET
{
	U32 targetID;

	USEARTIFACTTARGETED (void)
	{
		type = PT_USEARTIFACTTARGETED;
	}
};
//--------------------------------------------------------------------------//
//
struct USRPROBE : BASE_PACKET
{
	struct NETGRIDVECTOR position;

	USRPROBE (void)
	{
		type = PT_USRPROBE;
	}
};
//--------------------------------------------------------------------------//
//
struct USRRECOVER : BASE_PACKET
{
	U32 targetID;

	USRRECOVER (void)
	{
		type = PT_USRRECOVER;
	}
};
//--------------------------------------------------------------------------//
//
struct USRDROPOFF : BASE_PACKET
{
	U32 targetID;

	USRDROPOFF (void)
	{
		type = PT_USRDROPOFF;
	}
};
//--------------------------------------------------------------------------//
//
struct USRMIMIC : BASE_PACKET
{
	U32 targetID;

	USRMIMIC (void)
	{
		type = PT_USRMIMIC;
	}
};
//--------------------------------------------------------------------------//
//
struct USRCREATEWORMHOLE : BASE_PACKET
{
	U8 systemID;

	USRCREATEWORMHOLE (void)
	{
		type = PT_USRCREATEWORMHOLE;
	}
};
//--------------------------------------------------------------------------//
//
struct USRSTOP : BASE_PACKET
{
	USRSTOP (void)
	{
		type = PT_USRSTOP;
	}
};
//--------------------------------------------------------------------------//
//
struct USRBUILD : BASE_PACKET
{
	enum COMMAND
	{
		ADD,
		PAUSE,
		REMOVE,
		ADDIFEMPTY
	} cmd;	

	U32 dwArchetypeID;

	USRBUILD (void)
	{
		type = PT_USRBUILD;
		userBits = 1;		// concat this command
	}
};
//--------------------------------------------------------------------------//
// fabricate a structure
//
struct USRFAB : BASE_PACKET
{
	U32 dwArchetypeID;
	U32 planetID;
	U16	slotID;


	USRFAB (void)
	{
		type = PT_USRFAB;
	}
};
//--------------------------------------------------------------------------//
// fabricate a structure
//
struct USRFABJUMP : BASE_PACKET
{
	U32 dwArchetypeID;
	U32 jumpgateID;

	USRFABJUMP (void)
	{
		type = PT_USRFABJUMP;
	}
};
//--------------------------------------------------------------------------//
//
struct USRFABPOS : BASE_PACKET
{
	U32 dwArchetypeID;
	NETGRIDVECTOR position;

	USRFABPOS (void)
	{
		type = PT_USRFABPOS;
	}
};
//--------------------------------------------------------------------------//
//
struct USRHARVEST : BASE_PACKET
{
	U32 targetID;
	bool bAutoSelected;			// true if AI selected the target

	USRHARVEST (void)
	{
		type = PT_USRHARVEST;
	}
};
//--------------------------------------------------------------------------//
//
struct USRRALLY : BASE_PACKET
{
	NETGRIDVECTOR position;

	USRRALLY (void)
	{
		type = PT_USRRALLY;
		userBits = 1;		// concat this command
	}
};
//--------------------------------------------------------------------------//
//
struct USRESCORT : BASE_PACKET
{
	U32 targetID;

	USRESCORT (void)
	{
		type = PT_USRESCORT;
	}
};
//--------------------------------------------------------------------------//
//
struct USRDOCKFLAGSHIP : BASE_PACKET
{
	USRDOCKFLAGSHIP (void)
	{
		type = PT_USRDOCKFLAGSHIP;
	}
};
//--------------------------------------------------------------------------//
//
struct USRUNDOCKFLAGSHIP : BASE_PACKET
{
	struct NETGRIDVECTOR position;			// used for undocking

	USRUNDOCKFLAGSHIP (void)
	{
		type = PT_USRUNDOCKFLAGSHIP;
	}
};
//--------------------------------------------------------------------------//
//
struct USRRESUPPLY : BASE_PACKET
{
	U32 targetID;			// unit that will receive supplies

	USRRESUPPLY (void)
	{
		type = PT_USRRESUPPLY;
	}
};
//--------------------------------------------------------------------------//
//
struct USRSHIPREPAIR : BASE_PACKET
{
	U32 targetID;		// platform that will be performing the repair

	USRSHIPREPAIR (void)
	{
		type = PT_USRSHIPREPAIR;
	}
};
//--------------------------------------------------------------------------//
//
struct USRFABREPAIR : BASE_PACKET
{
	U32 targetID;	// platform/gunsat that will be repaired

	USRFABREPAIR (void)
	{
		type = PT_USRFABREPAIR;
	}
};
//--------------------------------------------------------------------------//
//
struct USRCAPTURE : BASE_PACKET
{
	U32 targetID;

	USRCAPTURE (void)
	{
		type = PT_USRCAPTURE;
	}
};
//--------------------------------------------------------------------------//
//
struct USRSPATTACK : BASE_PACKET
{
	U32 targetID;
	U32 destSystemID;		// 0 = follow target to ANY remote system to attack it

	USRSPATTACK (void)
	{
		type = PT_USRSPATTACK;
		destSystemID = 0;
	}
};
//--------------------------------------------------------------------------//
//
struct USRSPABILITY : BASE_PACKET
{
	U8 specialID;
	USRSPABILITY (void)
	{
		specialID = 0;
		type = PT_USRSPABILITY;
	}
};
//--------------------------------------------------------------------------//
//
struct USRCLOAK : BASE_PACKET
{
	USRCLOAK (void)
	{
		type = PT_USRCLOAK;
	}
};
//--------------------------------------------------------------------------//
//
struct USR_EJECT_ARTIFACT : BASE_PACKET
{
	USR_EJECT_ARTIFACT (void)
	{
		type = PT_USR_EJECT_ARTIFACT;
	}
};
//--------------------------------------------------------------------------//
//
struct USRFABSALVAGE : BASE_PACKET
{
	U32 targetID;	// platform/gunsat that will be salvaged

	USRFABSALVAGE (void)
	{
		type = PT_USRFABSALVAGE;
	}
};
//--------------------------------------------------------------------------//
//
struct USRKILLUNIT : BASE_PACKET
{
	USRKILLUNIT (void)
	{
		type = PT_USRKILLUNIT;
	}
};
//--------------------------------------------------------------------------//
//
struct PARTRENAMEPACKET : BASE_PACKET
{
	U32 partID;
	wchar_t name[32];		// size of a part name

	PARTRENAMEPACKET (void)
	{
		type = PT_PARTRENAME;
	}
};
//--------------------------------------------------------------------------//
//
#define MAX_CHAT_LENGTH 100
struct NETTEXT : BASE_PACKET
{
	U8 toID;		// mask of who we are sending it to
	U8 playerID;	
	wchar_t chatText[MAX_CHAT_LENGTH];
	      
	NETTEXT (void)
	{
		type = PT_NETTEXT;
		playerID = 0;
	}
};
//--------------------------------------------------------------------------//
//
struct STANCE_PACKET : BASE_PACKET
{
	UNIT_STANCE stance;

	STANCE_PACKET (void)
	{
		type = PT_STANCECHANGE;
	}
};
//--------------------------------------------------------------------------//
//
struct SUPPLY_STANCE_PACKET : BASE_PACKET
{
	enum SUPPLY_SHIP_STANCE stance;

	SUPPLY_STANCE_PACKET (void)
	{
		type = PT_SUPPLYSTANCECHANGE;
	}
};
//--------------------------------------------------------------------------//
//
struct HARVEST_STANCE_PACKET : BASE_PACKET
{
	enum HARVEST_STANCE stance;

	HARVEST_STANCE_PACKET (void)
	{
		type = PT_HARVESTSTANCECHANGE;
	}
};
//--------------------------------------------------------------------------//
//
struct HARVEST_AUTOMODE_PACKET : BASE_PACKET
{
	enum M_NUGGET_TYPE nuggetType;

	HARVEST_AUTOMODE_PACKET (void)
	{
		type = PT_HARVESTERAUTOMODE;
	}
};
//--------------------------------------------------------------------------//
//
struct FIGHTER_STANCE_PACKET : BASE_PACKET
{
	enum FighterStance stance;

	FIGHTER_STANCE_PACKET (void)
	{
		type = PT_FIGHTERSTANCECHANGE;
	}
};
//--------------------------------------------------------------------------//
//
struct ADMIRAL_TACTIC_PACKET : BASE_PACKET
{
	enum ADMIRAL_TACTIC stance;

	ADMIRAL_TACTIC_PACKET (void)
	{
		type = PT_ADMIRALTACTICCHANGE;
	}
};
//--------------------------------------------------------------------------//
//
struct ADMIRAL_FORMATION_PACKET : BASE_PACKET
{
	U8 slotID;

	ADMIRAL_FORMATION_PACKET (void)
	{
		type = PT_ADMIRALFORMATIONCHANGE;
	}
};

//--------------------------------------------------------------------------//
//
struct PATROL_PACKET : BASE_PACKET
{
	GRIDVECTOR patrolStart;
	GRIDVECTOR patrolEnd;
	
	PATROL_PACKET (void)
	{
		type = PT_PATROL;
	}
};
//--------------------------------------------------------------------------//
//
struct FLEETDEF_PACKET : BASE_PACKET
{
	FLEETDEF_PACKET (void)
	{
		type = PT_FLEETDEF;
	}
};
//--------------------------------------------------------------------------//
//
struct PLAYERLOST_PACKET : BASE_PACKET
{
	U8 playerID;

	PLAYERLOST_PACKET (void)
	{
		type = PT_PLAYERLOST;
	}
};
//--------------------------------------------------------------------------//
//
struct ALLIANCE_PACKET : BASE_PACKET
{
	U8 allianceFlags;
	U8 playerID;

	ALLIANCE_PACKET (void)
	{
		type = PT_ALLIANCEFLAGS;
	}
};
//--------------------------------------------------------------------------//
//
struct GIFT_PACKET : BASE_PACKET
{
	U8 recieveID;
	U8 giverID;
	U32 amount;
};
//--------------------------------------------------------------------------//
//
struct GIFTORE_PACKET : GIFT_PACKET
{
	GIFTORE_PACKET (void)
	{
		type = PT_GIFTORE;
	}
};
//--------------------------------------------------------------------------//
//
struct GIFTCREW_PACKET : GIFT_PACKET
{
	GIFTCREW_PACKET (void)
	{
		type = PT_GIFTCREW;
	}
};
//--------------------------------------------------------------------------//
//
struct GIFTGAS_PACKET : GIFT_PACKET
{
	GIFTGAS_PACKET (void)
	{
		type = PT_GIFTGAS;
	}
};
//--------------------------------------------------------------------------//
//
struct AIRESIGN_PACKET : BASE_PACKET
{
	U8 playerID;
	
	AIRESIGN_PACKET (void)
	{
		type = PT_AIRESIGN;
	}
};
//--------------------------------------------------------------------------//
//
#ifdef FINAL_RELEASE

template <class Type> 
struct USR_PACKET : Type
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(Type) + (numObjects*sizeof(U32));
	}
};

#else
//--------------------------------------------------------------------------//
//
inline void VerifySet (U32 objectID[MAX_SELECTED_UNITS], int numObjects)
{
	int i;

	for (i = 1; i < numObjects; i++)
	{
		if (dwordsearch(numObjects-i, objectID[i-1], objectID+i))
			CQBOMB1("Duplicate member 0x%X found in packet set.", objectID[i-1]);
	}
}
//--------------------------------------------------------------------------//
//
template <class Type> 
struct USR_PACKET : Type
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(Type) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		VerifySet(objectID, numObjects);
		if (bTrackCommand)
			COMMTRACK->AddCommand(type, objectID, numObjects);
	}
};
template <> struct USR_PACKET<USRMOVE>;
template <> struct USR_PACKET<USRFORMATIONMOVE>;
template <> struct USR_PACKET<FORMATIONATTACK>;
template <> struct USR_PACKET<FLEETDEF_PACKET>;
template <> struct USR_PACKET<USRAOEATTACK>;
template <> struct USR_PACKET<USEARTIFACTTARGETED>;
template <> struct USR_PACKET<STANCE_PACKET>;
template <> struct USR_PACKET<SUPPLY_STANCE_PACKET>;
template <> struct USR_PACKET<HARVEST_STANCE_PACKET>;
template <> struct USR_PACKET<HARVEST_AUTOMODE_PACKET>;
template <> struct USR_PACKET<FIGHTER_STANCE_PACKET>;
template <> struct USR_PACKET<ADMIRAL_TACTIC_PACKET>;
template <> struct USR_PACKET<ADMIRAL_FORMATION_PACKET>;
template <> struct USR_PACKET<USRCLOAK>;
template <> struct USR_PACKET<USR_EJECT_ARTIFACT>;
template <> struct USR_PACKET<USRMIMIC>;
template <> struct USR_PACKET<USRSPABILITY>;
#ifdef OBJLIST_H
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<USRMOVE> : USRMOVE
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(USRMOVE) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		CQASSERT(position.systemID && position.systemID <= MAX_SYSTEMS);
		VerifySet(objectID, numObjects);
		if (bTrackCommand && userBits==0)		// don't track queued move commands
			COMMTRACK->AddCommand(type, objectID, numObjects);
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<USRFORMATIONMOVE> : USRFORMATIONMOVE
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(USRFORMATIONMOVE) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		CQASSERT(position.systemID && position.systemID <= MAX_SYSTEMS);
		VerifySet(objectID, numObjects);
		if (bTrackCommand && userBits==0)		// don't track queued move commands
			COMMTRACK->AddCommand(type, objectID, numObjects);
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<FORMATIONATTACK> : FORMATIONATTACK
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(FORMATIONATTACK) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		CQASSERT(destSystemID && destSystemID <= MAX_SYSTEMS);
		VerifySet(objectID, numObjects);
		if (bTrackCommand && userBits==0)		// don't track queued move commands
			COMMTRACK->AddCommand(type, objectID, numObjects);
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<FLEETDEF_PACKET> : FLEETDEF_PACKET
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(FLEETDEF_PACKET) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT(TESTADMIRAL(objectID[0]) && OBJLIST->FindObject(objectID[0]));
		VerifySet(objectID, numObjects);
		if (bTrackCommand)
			COMMTRACK->AddCommand(type, objectID, numObjects);
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<USRAOEATTACK> : USRAOEATTACK
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(USRAOEATTACK) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		CQASSERT(position.systemID && position.systemID <= MAX_SYSTEMS);
		VerifySet(objectID, numObjects);
		if (bTrackCommand)
			COMMTRACK->AddCommand(type, objectID, numObjects);
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<USEARTIFACTTARGETED> : USEARTIFACTTARGETED
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(USEARTIFACTTARGETED) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		VerifySet(objectID, numObjects);
		if (bTrackCommand)
			COMMTRACK->AddCommand(type, objectID, numObjects);
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<STANCE_PACKET> : STANCE_PACKET
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(STANCE_PACKET) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		VerifySet(objectID, numObjects);
		// don't track stance packets
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<SUPPLY_STANCE_PACKET> : SUPPLY_STANCE_PACKET
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(SUPPLY_STANCE_PACKET) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		VerifySet(objectID, numObjects);
		// don't track stance packets
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<HARVEST_STANCE_PACKET> : HARVEST_STANCE_PACKET
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(HARVEST_STANCE_PACKET) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		VerifySet(objectID, numObjects);
		// don't track stance packets
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<HARVEST_AUTOMODE_PACKET> : HARVEST_AUTOMODE_PACKET
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(HARVEST_AUTOMODE_PACKET) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		VerifySet(objectID, numObjects);
		// don't track stance packets
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<FIGHTER_STANCE_PACKET> : FIGHTER_STANCE_PACKET
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(FIGHTER_STANCE_PACKET) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		VerifySet(objectID, numObjects);
		// don't track stance packets
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<ADMIRAL_TACTIC_PACKET> : ADMIRAL_TACTIC_PACKET
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(ADMIRAL_TACTIC_PACKET) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		VerifySet(objectID, numObjects);
		// don't track stance packets
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<ADMIRAL_FORMATION_PACKET> : ADMIRAL_FORMATION_PACKET
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(ADMIRAL_FORMATION_PACKET) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		VerifySet(objectID, numObjects);
		// don't track stance packets
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<USRCLOAK> : USRCLOAK
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(USRCLOAK) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		VerifySet(objectID, numObjects);
		// don't track cloaking packets
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<USR_EJECT_ARTIFACT> : USR_EJECT_ARTIFACT
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(USR_EJECT_ARTIFACT) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		VerifySet(objectID, numObjects);
		// don't track cloaking packets
	}
};

//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<USRMIMIC> : USRMIMIC
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(USRMIMIC) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		VerifySet(objectID, numObjects);
		// don't track mimic packets
	}
};
//--------------------------------------------------------------------------//
//
template <> struct USR_PACKET<USRSPABILITY> : USRSPABILITY
{
	U32 objectID[MAX_SELECTED_UNITS];

	void init (int numObjects, bool bTrackCommand=true)
	{
		dwSize = sizeof(USRSPABILITY) + (numObjects*sizeof(U32));
		CQASSERT(numObjects);
		CQASSERT(numObjects <= MAX_SELECTED_UNITS);
		CQASSERT((objectID[0]&PLAYERID_MASK)!=MGlobals::GetGroupID() || OBJLIST->FindGroupObject(objectID[0]));
		CQASSERT((objectID[0]&PLAYERID_MASK)==MGlobals::GetGroupID() || OBJLIST->FindObject(objectID[0]));
		VerifySet(objectID, numObjects);
		// don't track special ability packets
	}
};

#endif	  // end OBJLIST_H

#endif	  // end !FINAL_RELEASE
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
// enum PACKET_TYPE is now defined in Globals.h
//
//--------------------------------------------------------------------------//
//
/*
struct BASE_PACKET
{
	DWORD		dwSize;
	DPID		fromID;
	PACKET_TYPE type;
	U32			timeStamp;
	U16			sequenceID, ackID;

	BASE_PACKET (void)
	{
		memset(this, 0, sizeof(*this));
	}
};
*/

#pragma pack ( pop )

//---------------------------------------------------------------------------------//
//-------------------------------End CommPacket.h----------------------------------//
//---------------------------------------------------------------------------------//
#endif