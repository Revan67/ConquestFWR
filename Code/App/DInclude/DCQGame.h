#ifndef DCQGAME_H
#define DCQGAME_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 DCQGame.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DCQGame.h 29    10/18/02 2:36p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
/*
	CQGAME defines the structure for data that needs to be collected before
	starting a multiplayer game.

 */
//--------------------------------------------------------------------------//
//
#ifdef _ADB
#define DWORD U32
#endif


#define PLAYERNAMESIZE 34			// enough to hold 32 characters plus null
#define MAPNAMESIZE	   128

typedef DWORD DPID;		// from dplay.h

namespace CQGAMETYPES
{
	enum DIFFICULTY
	{
		NODIFFICULTY,
		EASY,
		AVERAGE,
		HARD
	};

	enum TYPE
	{
		HUMAN,
		COMPUTER
	};

	enum COMP_CHALANGE
	{
		EASY_CH,
		AVERAGE_CH,
		HARD_CH,
		IMPOSIBLE_CH,
		NIGHTMARE_CH,
	};

	enum STATE
	{
		OPEN,		// slot can be used, but not active at this time
		CLOSED,		// host has disallowed this slot
		ACTIVE,		// slot is being used by computer or human player, has not accepted game rules yet
		READY		// slot is active and player has accepted rules
	};
	enum RACE
	{
		NORACE,
		TERRAN,
		MANTIS,
		SOLARIAN,
		VYRIUM,
	};
	enum COLOR
	{
		UNDEFINEDCOLOR,	// used for computer players
		YELLOW,
		RED,
		BLUE,
		PINK,
		GREEN,
		ORANGE,
		PURPLE,
		AQUA
	};
	enum TEAM
	{
		NOTEAM,
		_1,
		_2,
		_3,
		_4
	};

	struct SLOT
	{
		TYPE type:3;
		COMP_CHALANGE compChalange:4;
		STATE state:3;
		RACE race:4;
		COLOR color:5;
		TEAM team:4;
		U32  zoneSeat:3;
		DPID dpid;			// id of player, 0 if computer player
	};

	enum GAMETYPE			// need 2 bits
	{
		KILL_UNITS=-2,
		KILL_HQ_PLATS,
		MISSION_DEFINED,	// must be == 0
		KILL_PLATS_FABS
	};

	enum MONEY				// need 2 bits
	{
		LOW_MONEY=-2,
		MEDIUM_MONEY,
		HIGH_MONEY
	};
	enum MAPTYPE			// need 2 bits
	{
		SELECTED_MAP=-2,
		USER_MAP,			// from saved game dir
		RANDOM_MAP
	};
	enum MAPSIZE			// need 2 bits
	{
		SMALL_MAP=-2,
		MEDIUM_MAP,
		LARGE_MAP
	};
	enum TERRAIN			// need 2 bits
	{
		LIGHT_TERRAIN=-2,
		MEDIUM_TERRAIN,
		HEAVY_TERRAIN
	};
	enum STARTING_UNITS		// need 2 bits
	{
		UNITS_MINIMAL=-2,
		UNITS_MEDIUM,
		UNITS_LARGE
	};
	enum VISIBILITYMODE		// need 2 bits
	{
		VISIBILITY_NORMAL=-1,
		VISIBILITY_EXPLORED,
		VISIBILITY_ALL
	};

	enum RANDOM_TEMPLATE	//need 2 bits
	{
		TEMPLATE_NEW_RANDOM = -2,
		TEMPLATE_RANDOM,
		TEMPLATE_RING,
		TEMPLATE_STAR,
	};

	enum COMMANDLIMIT		// need 2 bits
	{
		COMMAND_LOW=-2,
		COMMAND_NORMAL,
		COMMAND_MID,
		COMMAND_HIGH
	};

	struct OPTIONS
	{
		U32 version;
		GAMETYPE gameType:3;
		S32 gameSpeed:5;	// need enough bits for -16 to 15
		U32 regenOn:1;
		U32 spectatorsOn:1;
		U32 lockDiplomacyOn:1;
		U32 numSystems:5;
		MONEY money:2;
		MAPTYPE mapType:2;
		RANDOM_TEMPLATE templateType:2;
		MAPSIZE mapSize:2;
		TERRAIN terrain:2;
		STARTING_UNITS units:2;
		VISIBILITYMODE visibility:2;
		COMMANDLIMIT  commandLimit:2;

#ifndef _ADB
		bool operator == (const OPTIONS & options)
		{
			return (memcmp(this, &options, sizeof(*this)) == 0);
		}

		bool operator != (const OPTIONS & options)
		{
			return (memcmp(this, &options, sizeof(*this)) != 0);
		}
#endif
	};

	struct _CQGAME
	{
		U32 activeSlots:8;			// valid from 1 to MAX_PLAYERS
		U32 bHostBusy:1;			// host is not on the final screen
		U32 startCountdown:4;	

		SLOT slot[MAX_PLAYERS];
	};

}  // end namespace CQGAMETYPES

//--------------------------------------------------------------------------//
//
struct CQGAME : CQGAMETYPES::_CQGAME, CQGAMETYPES::OPTIONS
{
};


#endif