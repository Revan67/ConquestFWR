#ifndef DSHIPSAVE_H
#define DSHIPSAVE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DShipSave.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DShipSave.h 135   8/24/01 2:48p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DINSTANCE_H
#include "DInstance.h"		// for SAVELOAD, animation
#endif

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DMBASEDATA_H
#include "DMBaseData.h"
#endif

#ifndef DBUILDSAVE_H
#include "DBuildSave.h"
#endif

#ifndef DREPAIRSAVE_H
#include "DRepairSave.h"
#endif

#ifndef DNUGGET_H
#include "DNugget.h"
#endif

#ifdef _ADB
#define mutable
#endif

struct BASE_FABRICATOR_SAVELOAD;
struct BASE_FLAGSHIP_SAVELOAD;
struct BASE_TROOPSHIP_SAVELOAD;
struct BASE_SUPPLYSHIP_SAVELOAD;
//----------------------------------------------------------------
//
//----------------------------------------------------------------
//
struct WARP_SAVELOAD
{
	enum WARP_STAGE
	{
		WS_NONE=0,
		WS_WARP_IN,
		WS_LIMBO,
		WS_WARP_OUT,
		WS_PRE_WARP
	};

	WARP_STAGE warpStage;
	Vector warpInVector;
	float warpSpeed;
	float warpRadius;
	SINGLE warpTimer;  
	U32 targetGateID;
	U32 inTargetGateID;
	U32 warpAgentID;
	SINGLE stop_speed;
	SINGLE stop_heading;
	SINGLE releaseTime;
};
//----------------------------------------------------------------
//
#define MAX_PATH_SIZE 4
struct SPACESHIP_SAVELOAD
{
	struct TOBJMOVE
	{
		//
		// move info
		//
		GRIDVECTOR goalPosition, currentPosition, jumpToPosition;
		GRIDVECTOR pathList[MAX_PATH_SIZE];
		SINGLE cruiseDepth;		// target Z to reach when moving
		SINGLE cruiseSpeed;		// 0 = use default
		SINGLE groupAcceleration;  // 0 = use default
		SINGLE mockRotationAngle;
		U32    moveAgentID, jumpAgentID;

		//
		// override data
		//
		U32 overrideAttackerID;		// unit that is pushing us around
		enum 
		{ 
			OVERRIDE_NONE,
			OVERRIDE_PUSH,
			OVERRIDE_DESTABILIZE,
			OVERRIDE_ORIENT
		} overrideMode;
		union
		{
			SINGLE overrideSpeed;
			SINGLE overrideYaw;
		};
		GRIDVECTOR overrideDest;	// place we are being pushed

		//
		// patrol data
		//
		GRIDVECTOR patrolVectors[2];
		S8		   patrolIndex;

		//
		// state flags 
		//
		U8   pathLength;		// number of valid elements in pathList, (valid only when bMoveActive is true)
		bool bMoveActive:1;
		bool bAutoMovement:1;
		bool bCompletionAllowed:1;	// host has already completed the move, ok for us to complete too
		bool bRotatingBeforeMove:1;	// true if doing a rotation in preparation for a move
		bool bSyncNeeded:1;			// true after completing a move
		bool bFinalMove:1;			// close to target position, pass final flag to terrain map
		bool bPathOverflow:1;		// returned path overflowed buffer
		bool bMockRotate:1;
		bool bPatroling:1;

		//
		// rocking data
		//
		bool bRollUp:1;
		bool bAltUp:1;

		bool slowMove:1;
	} tobjmove;
	
	// basic physics data
	TRANS_SAVELOAD   trans_SL;

	// build data
	BUILD_SAVELOAD build_SL;

	//repair data
	REPAIR_SAVELOAD repair_SL;

	// warping data

	WARP_SAVELOAD warp_SL;

	// cloaking data

	CLOAK_SAVELOAD cloak_SL;

	DAMAGE_SAVELOAD damage_SL;

	// spaceship data
	U32 firstNuggetID;

	/* mission data */
	MISSION_SAVELOAD mission;
};

enum MINEMODES
{
	MLAY_IDLE,
	MLAY_MOVING_TO_POS,
	MLAY_WAIT_CLIENT,
	MLAY_ROTATING_TO_POS,
	MLAY_LAYING,
};

struct BASE_MINELAYER_SAVELOAD
{
	struct GRIDVECTOR targetMinePos;	
	SINGLE layingTime;
	U32 workingID;
	U32 minefieldMissionID;
	bool bRotating:1;
	MINEMODES mode;
};

struct MINELAYER_SAVELOAD : SPACESHIP_SAVELOAD 
{
	BASE_MINELAYER_SAVELOAD mineLayerSaveLoad;
};
//----------------------------------------------------------------
//
struct BASE_GUNBOAT_SAVELOAD
{
	__hexview U32 dwTargetID;
	U32 attackAgentID;
	U32 escortAgentID;
	__hexview U32 dwEscortID;			// unit that we are defending
	UNIT_STANCE unitStance;
	FighterStance fighterStance;
	GRIDVECTOR  defendPivot;
	U32 launcherAgentID;
	U32 launcherID;

	bool bWaitingForAdmiral:1;
	bool bRepairUnderway:1;
	bool bUserGenerated:1;
	bool bSpecialAttack:1;
	bool bArtifactUse:1;
	bool bDefendPivotValid:1;
};
//----------------------------------------------------------------
//
#ifndef _ADB
#define GUNBOAT_SAVELOAD _GSL
#endif
struct GUNBOAT_SAVELOAD : SPACESHIP_SAVELOAD
{
	BASE_GUNBOAT_SAVELOAD	gunSaveLoad;
};
//----------------------------------------------------------------
//
struct BASE_RECONPROBE_SAVELOAD
{
	SINGLE probeTimer,totalTime;
	Vector goal;
	U32 workingID;
	bool bMoving:1;
	bool bJumping:1;
	bool bGone:1;
	bool bLauncherDelete:1;
	bool bNoMoreSync:1;
};
//----------------------------------------------------------------
//
struct RECONPROBE_SAVELOAD : SPACESHIP_SAVELOAD
{
	BASE_RECONPROBE_SAVELOAD	baseSaveLoad;
};
//----------------------------------------------------------------
//
struct BASE_TORPEDO_SAVELOAD
{
	SINGLE tTimer,totalTime;
	bool bClearing;
	Vector clearPos;
	__hexview U32 targetID;
	U32 ownerID;
};
//----------------------------------------------------------------
//
struct TORPEDO_SAVELOAD : SPACESHIP_SAVELOAD
{
	BASE_TORPEDO_SAVELOAD	baseSaveLoad;
};
//----------------------------------------------------------------
//
enum HARVEST_MODE
{
	HAR_NO_MODE_AI_ON,
	HAR_NO_MODE_AI_OFF,
	HAR_NO_MODE_CLIENT_WAIT_CANCEL,

	//-----------------------
	HAR_MOVING_TO_REFINERY,
	HAR_AT_REFINERY_CLIENT,
	HAR_MOVING_TO_READY_REFINERY_CLIENT,
	HAR_MOVING_TO_READY_REFINERY_HOST,

	HAR_WAITING_TO_DOCK,
	HAR_WAIT_DOCKING_CANCEL_CLIENT,
	HAR_DOCKING_REFINERY,
	HAR_DOCKED_TO_REFINERY,

	//-----------------------

	HAR_WAIT_FOR_NUGGET_BEGIN_CLIENT,
	HAR_MOVING_TO_NUGGET,
	HAR_ROTATING_TO_NUGGET,
	HAR_WAIT_NUGGET_START,
	HAR_MOVING_TO_READY_NUGGET_CLIENT,
	HAR_MOVING_TO_READY_NUGGET_HOST,
	HAR_ROTATING_TO_READY_NUGGET_CLIENT,
	HAR_ROTATING_TO_READY_NUGGET_HOST,
	HAR_WAIT_NUGGET_ARRIVAL,
	HAR_NUGGETING,
	HAR_WAIT_NUGGET_CANCEL_CLIENT,

	//-----------------------
	
	HAR_MOVE_TO_RECOVERY,
	HAR_ROTATING_TO_RECOVERY,
	HAR_RECOVERING,
	HAR_MOVE_TO_RECOVERY_DROP

};
//----------------------------------------------------------------
//
struct BASE_HARVEST_SAVELOAD
{	
	U8 gas;
	U8 metal;

	__hexview U32 targetPartID;
	__hexview U32 recoverPartID;
	U32 workingID;
	U32 posibleWorkingID;
	NETGRIDVECTOR harvestVector;

	Vector recoverPos;
	SINGLE recoverYaw;
	SINGLE recoverTime;
	SINGLE harvestRemainder;

	HARVEST_MODE mode;
	M_NUGGET_TYPE nuggetType;

	bool bNuggeting:1;
	bool bLockingPlatform:1;
	bool bHostParking:1;
	bool bDockingWithGas:1;
	bool bRotating:1;
	bool bTowingShip:1;
	bool bSendIdle:1;
	bool bNuggetCancelOp:1;
};
//----------------------------------------------------------------
//
struct HARVEST_SAVELOAD : SPACESHIP_SAVELOAD
{
	BASE_HARVEST_SAVELOAD baseSaveLoad;
};
//----------------------------------------------------------------
//
struct SPACESHIP_VIEW
{
	MISSION_SAVELOAD * mission;
	BASIC_INSTANCE	  *	rtData;
	S16 gamma;
	SINGLE contrast;
	union SHIPDATA
	{
		BASE_GUNBOAT_SAVELOAD		* gunboat;
		BASE_MINELAYER_SAVELOAD		* minelayer;
		BASE_RECONPROBE_SAVELOAD	* reconprobe;
		BASE_TORPEDO_SAVELOAD		* torpedo;
		BASE_HARVEST_SAVELOAD		* harvester;
		BASE_FABRICATOR_SAVELOAD	* fabricator;
		BASE_FLAGSHIP_SAVELOAD		* flagship;
		BASE_TROOPSHIP_SAVELOAD		* troopship;
		BASE_SUPPLYSHIP_SAVELOAD	* supplyship;
		void						* nothing;
	} shipData;
};


#endif
