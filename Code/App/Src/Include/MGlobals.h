#ifndef MGLOBALS_H
#define MGLOBALS_H

//--------------------------------------------------------------------------//
//                                                                          //
//                               MGlobals.h								//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Include/MGlobals.h 117   10/18/02 2:36p Tmauer $
*/
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#define ADMIRAL_MASK    0x80000000
#define SUBORDID_MASK   0x7F000000
#define PLAYERID_MASK	0x0000000F
#define TESTADMIRAL(x)  ((x & (ADMIRAL_MASK|SUBORDID_MASK)) == ADMIRAL_MASK)
//---------------------------------------------------------------------------
//
enum UG_TYPE
{
	UG_FIGHTER,
	UG_SHIELDS
};
//---------------------------------------------------------------------------
//
struct MGlobals 
{
	MEXTERN static U32 GetUpgradeLevel (U32 playerID,enum UG_TYPE ug_type,S32 race);

	MEXTERN static void SetAlly (U32 playerID1, U32 playerID2, bool bAlly=true);	// one-way alliance

	MEXTERN static bool AreAllies (U32 playerID1, U32 playerID2);		// two-way alliance

	MEXTERN static U32 GetOneWayAllyMask (U32 playerID);	// returns 8-bit bitfield for two-way alliance

	MEXTERN static U32 GetAllyMask (U32 playerID);			// returns 8-bit bitfield for two-way alliance

	MEXTERN static U32 GetAllyData (U8 * buffer, U32 bufferSize);

	MEXTERN static void SetAllyData (const U8 * buffer, U32 bufferSize);
	
	MEXTERN static U32 GetVisibilityMask (U32 playerID);

	MEXTERN static void SetVisibilityMask (U32 playerID, U32 visibilityMask);

	MEXTERN static void SetPlayerVisibility (U32 playerID1, U32 player2);	// two way visibility (only called on host)

	MEXTERN static U32  GetThisPlayer (void);

	MEXTERN static U32 GetPlayerFromPartID (U32 dwMissionID);

	MEXTERN static U32 GetOwnerFromPartID (U32 dwMissionID);
	
	MEXTERN static bool IsHost (void);

	MEXTERN static U32 GetColorID (U32 playerID);

	MEXTERN static U32 CreateNewPartID (U32 playerID);	// only valid on server

	// used by subordinate objects of a mission object (e.g. fighters, nuggets)
	// can be called only after calling CreateInstance() for the base mission object (e.g. the carrier)
	MEXTERN static U32 CreateSubordinatePartID (void);

	MEXTERN static bool IsUpdateFrame (U32 dwMissionID);

	MEXTERN static struct IBaseObject * CreateInstance (PARCHETYPE pArchetype, U32 newMissionID);

	MEXTERN static void InitMissionData (IBaseObject * obj, U32 dwMissionID);

	MEXTERN static SINGLE GetBaseTargetingAccuracy (struct IBaseObject * shooter, struct IBaseObject * target, struct IBaseObject * admiral);

	MEXTERN static U32 GetEffectiveDamage (U32 amount, struct IBaseObject * shooter, struct IBaseObject * target, U32 shooterMissionID);

	MEXTERN static U32 GetIndExperienceLevel (U32 numKills);

	MEXTERN static U32 GetAdmiralExperienceLevel (U32 numKills);

	MEXTERN static SINGLE GetAIBonus (U32 playerID);

	MEXTERN static bool AdvancedAI (U32 playerID, bool expertOnly = false);

	MEXTERN static bool IsNightmareAI (U32 playerID);

	MEXTERN static U32 GetGroupID (void);

	// get next id in the sequence, does not create a new id!
	MEXTERN static U32 GetNextSubPartID (U32 dwMissionID);

	MEXTERN static bool CanTroopship (U32 playerID, U32 troopshipID, U32 targetID);

	MEXTERN static struct TECHNODE __stdcall GetCurrentTechLevel (U32 playerID);

	MEXTERN static void __stdcall SetCurrentTechLevel (struct TECHNODE techNode, U32 playerID);

	// tech flags of research currently under development
	MEXTERN static struct TECHNODE __stdcall GetWorkingTechLevel (U32 playerID);

	MEXTERN static void __stdcall SetWorkingTechLevel (struct TECHNODE techNode, U32 playerID);

	MEXTERN static struct TECHNODE GetTechAvailable();

	MEXTERN static void __stdcall SetTechAvailable (struct TECHNODE techNode);

	MEXTERN static void __stdcall UpgradeMissionObj(IBaseObject * missionObj);

	MEXTERN static SINGLE __stdcall GetHarvestUpgrade(IBaseObject * harvester);

	MEXTERN static SINGLE __stdcall GetTenderUpgrade(IBaseObject * tender);

	MEXTERN static SINGLE __stdcall GetFleetUpgrade(IBaseObject * fleetOfficer);
	
	MEXTERN static SINGLE __stdcall GetFighterUpgrade(U32 playerID, U32 race);

	// money methods
	MEXTERN static U32 GetCurrentGas (U32 playerID);

	MEXTERN static void SetCurrentGas (U32 playerID, U32 amount);

	MEXTERN static U32 GetCurrentMetal (U32 playerID);

	MEXTERN static void SetCurrentMetal (U32 playerID, U32 amount);

	MEXTERN static U32 GetCurrentCrew (U32 playerID);

	MEXTERN static void SetCurrentCrew (U32 playerID, U32 amount);

	MEXTERN static U32 GetCurrentTotalComPts (U32 playerID);

	MEXTERN static void SetCurrentTotalComPts (U32 playerID, U32 amount);

	MEXTERN static U32 GetCurrentUsedComPts (U32 playerID);

	MEXTERN static void SetCurrentUsedComPts (U32 playerID, U32 amount);

	MEXTERN static U32 GetMaxGas (U32 playerID);

	MEXTERN static void SetMaxGas (U32 playerID, U32 amount);

	MEXTERN static U32 GetMaxMetal (U32 playerID);

	MEXTERN static void SetMaxMetal (U32 playerID, U32 amount);

	MEXTERN static U32 GetMaxCrew (U32 playerID);

	MEXTERN static void SetMaxCrew (U32 playerID, U32 amount);

	MEXTERN static void ResetResourceMax ();

	// the following section is for game statistics
	MEXTERN static U32  GetNumUnitsBuilt (U32 playerID);
	MEXTERN static void SetNumUnitsBuilt (U32 playerID, U32 newValue);

	MEXTERN static U32  GetUnitsDestroyed (U32 playerID);
	MEXTERN static void SetUnitsDestroyed (U32 playerID, U32 newValue);

	MEXTERN static U32  GetUnitsLost (U32 playerID);
	MEXTERN static void SetUnitsLost (U32 playerID, U32 newValue);

	MEXTERN static U32  GetNumPlatformsBuilt (U32 playerID);
	MEXTERN static void SetNumPlatformsBuilt (U32 playerID, U32 newValue);

	MEXTERN static U32  GetNumAdmiralsBuilt (U32 playerID);
	MEXTERN static void SetNumAdmiralsBuilt (U32 playerID, U32 newValue);

	MEXTERN static U32  GetPlatformsDestroyed (U32 playerID);
	MEXTERN static void SetPlatformsDestroyed (U32 playerID, U32 newValue);

	MEXTERN static U32  GetPlatformsLost (U32 playerID);
	MEXTERN static void SetPlatformsLost (U32 playerID, U32 newValue);

	MEXTERN static U32  GetUnitsConverted (U32 playerID);
	MEXTERN static void SetUnitsConverted (U32 playerID, U32 newValue);

	MEXTERN static U32  GetPlatformsConverted (U32 playerID);
	MEXTERN static void SetPlatformsConverted (U32 playerID, U32 newValue);

	MEXTERN static U32  GetNumJumpgatesControlled (U32 playerID);
	MEXTERN static void SetNumJumpgatesControlled (U32 playerID, U32 newValue);

	MEXTERN static U32  GetGasGained (U32 playerID);
	MEXTERN static void SetGasGained (U32 playerID, U32 newValue);

	MEXTERN static U32  GetMetalGained (U32 playerID);
	MEXTERN static void SetMetalGained (U32 playerID, U32 newValue);

	MEXTERN static U32  GetCrewGained (U32 playerID);
	MEXTERN static void SetCrewGained (U32 playerID, U32 newValue);

	MEXTERN static U32 GetResearchCompleted (U32 playerID);
	MEXTERN static void   SetResearchCompleted (U32 playerID, U32 newValue);

	MEXTERN static SINGLE GetExploredSystemsRatio (U32 playerID);
	MEXTERN static void   SetExploredSystemsRatio (U32 playerID, SINGLE newValue);

	MEXTERN static bool IsGlobalLighting (void);
	MEXTERN static void EnableGlobalLighting (bool bEnable);

	MEXTERN static U32 GetPlayerNameBySlot (U32 slotID, wchar_t * buffer, U32 bufferSize);
	MEXTERN static U32 GetPlayerNameFromDPID (U32 dpid, wchar_t * buffer, U32 bufferSize);
	MEXTERN static U32 GetPlayerIDFromDPID (U32 dpID);
	MEXTERN static U32 GetPlayerIDFromSlot (U32 slotID);
	MEXTERN static U32 GetPlayerDPIDForPlayerID (U32 playerID, U32 * pDPID = 0);		// returns # of DPID's that match the playerID
	MEXTERN static U32 GetSlotIDFromDPID (U32 dpID);
	MEXTERN static bool IsPlayerInGame (U32 playerID);		// this doesn't change throughout game
	MEXTERN static bool HasPlayerResigned (U32 slotID);		// slot valid from 0 to MAX_PLAYERS-1
	MEXTERN static void SetPlayerResignedBySlot (U32 slotID);		// slot valid from 0 to MAX_PLAYERS-1
	MEXTERN static U32  GetZoneSeatFromSlot (U32 slotID);
	MEXTERN static void SetZoneSeatFromSlot (U32 slotID, U32 dwSeat);
	MEXTERN static U32 GetSlotIDForPlayerID (U32 playerID, U32 * pSlots = 0);		// returns # of slots that match the playerID

	MEXTERN static bool IsHostOnlyPlayerLeft (void);

	MEXTERN static void SetupComputerCharacter (U32 playerID, const wchar_t * szName);
	
	MEXTERN static const struct CQGAME & GetGameSettings (void);
	MEXTERN static void SetRegenMode (bool bEnable);
	MEXTERN static bool IsSinglePlayer (void);

	MEXTERN static U32 GetMaxControlPoints (U32 playerID);
	MEXTERN static void SetMaxControlPoints (U32 playerID, U32 newMax);


	MEXTERN static U32  GetScriptName (char * buffer, U32 bufferSize);
	MEXTERN static void SetScriptName (const char * buffer);

	MEXTERN static U32  GetTerrainFilename (char * buffer, U32 bufferSize);
	MEXTERN static void SetTerrainFilename (const char * buffer);

	MEXTERN static U32  GetUpdateCount (void);
	MEXTERN static void SetUpdateCount (U32 count);

	MEXTERN static enum M_RACE GetPlayerRace (U32 playerID);
	MEXTERN static void SetPlayerRace (const U32 playerID, const enum M_RACE race);

	static U32  GetGameStats (U32 playerID, void * buffer); //returns size of gameStats buffer
	static void SetGameStats (U32 playerID, const void * buffer, U32 bufferSize);

	static U32  GetGameScores (void * buffer); //returns size of gameScores buffer
	static void SetGameScores (const void * buffer, U32 bufferSize);

	static void SetPlayerScore (U32 playerID, U32 score);		//  only valid on host
	static U32  GetPlayerScore (U32 playerID);
	
	// save/load for global things that otherwise have no save/load needs
	static U32 GetLastStreamID (void);
	static void SetLastStreamID (U32 id);

	static U32 GetLastTeletypeID (void);
	static void SetLastTeletypeID (U32 id);

	static bool GetScriptUIControl (void);
	static void SetScriptUIControl (bool bScriptControl);

	static BOOL32 Save (IFileSystem * outFile);

	static BOOL32 Load (IFileSystem * inFile);

	static U32 GetFileDescription (const C8 * fileName, wchar_t * string, U32 bufferSize);

	static U32 GetFileDescription (IFileSystem * fileDir, const C8 * fileName, wchar_t * string, U32 bufferSize);

	static void SetFileDescription (const wchar_t * string);

	static U32 GetFileMaxPlayers (IFileSystem * fileDir, const C8 * fileName, U32 & maxPlayers);

	static U32 GetFileMaxPlayers (const C8 * fileName, U32 & maxPlayers);

	static void SetFileMaxPlayers (U32 maxPlayers);

	static BOOL32 QuickSave (IFileSystem * outFile);

	static BOOL32 New  (void);

	static BOOL32 Close  (void);

	/* Mission Objectives methods */

	static void SetMissionName (U32 stringID);
	static const U32 GetMissionName (void);

	static void SetMissionID (U32 missionID);
	static const U32 GetMissionID (void);

	static void SetMissionDescription (U32 stringID);
	static const U32 GetMissionDescription (void);

	static void AddToObjectiveList (U32 stringID, bool bSecondary);
	static void RemoveFromObjectiveList (U32 stringID);
	
	static void MarkObjectiveCompleted (U32 stringID);
	static const bool IsObjectiveCompleted (const U32 stringID);
	static const bool IsObjectiveSecondary (const U32 stringID);
	
	static void MarkObjectiveFailed (U32 stringID);
	static const bool IsObjectiveFailed (const U32 stringID);

    static const bool IsObjectiveInList (const U32 stringID);

	static const U32 GetNumberObjectives (void);
	static const U32 GetObjectiveStringID (const U32 index);


	// the following is for M_OBJCLASS recognition
	MEXTERN static bool IsHQ (enum M_OBJCLASS oclass);
	MEXTERN static bool IsPlatform (enum M_OBJCLASS oclass);
	MEXTERN static bool IsRefinery (enum M_OBJCLASS oclass);
	MEXTERN static bool IsShipyard (enum M_OBJCLASS oc);
	MEXTERN static bool IsRepairPlat (enum M_OBJCLASS oc);
	MEXTERN static bool IsTenderPlat (enum M_OBJCLASS oc);
	MEXTERN static bool IsMilitaryShip (enum M_OBJCLASS oclass);
	MEXTERN static bool IsObjectThreatening (enum M_OBJCLASS oc);
	MEXTERN static bool IsGunboat (enum M_OBJCLASS oclass);
	MEXTERN static bool IsLightGunboat (enum M_OBJCLASS oclass);
	MEXTERN static bool IsMediumGunboat (enum M_OBJCLASS oclass);
	MEXTERN static bool IsHeavyGunboat (enum M_OBJCLASS oclass);
	MEXTERN static bool IsCarrier (enum M_OBJCLASS oclass);
	MEXTERN static bool IsTroopship (enum M_OBJCLASS oclass);
	MEXTERN static bool IsFlagship (enum M_OBJCLASS oclass);
	MEXTERN static bool IsMinelayer (enum M_OBJCLASS oclass);
	MEXTERN static bool IsHarvester (enum M_OBJCLASS oclass);
	MEXTERN static bool IsSupplyShip (enum M_OBJCLASS oclass);
	MEXTERN static bool IsFabricator (enum M_OBJCLASS oclass);
	MEXTERN static bool IsJumpPlat (enum M_OBJCLASS oclass);
	MEXTERN static bool IsGunPlat (enum M_OBJCLASS oclass);
	MEXTERN static bool IsSeekerShip (enum M_OBJCLASS oclass);

	
private:

	static void AssignPlayers (const struct FULLCQGAME & cqgame, U32 randomSeed);

	static void AssignThisPlayer (const char * szPlayerName);

	static void SetLastPartID (U32 lastID);

	static U32 CreateNewGroupPartID (void);

	static U32 CreateNewJumpgatePartID (void);
	
	static void RemoveDPIDFromPlayerID (U32 dpid, U32 playerID);		// called when a player exits a game

	friend struct Mission;
	friend struct OpAgent;
	friend struct Jump;
};

//-------------------------------------------------------------------


#endif

