#ifndef MSCRIPT_H
#define MSCRIPT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               MScript.h								    //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Include/MScript.h 187   7/06/01 11:08a Tmauer $
*/
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// struct used by script dll's to register a program
//
#undef CQEXTERN
#ifdef BUILD_MISSION
#define CQEXTERN __declspec(dllexport)
#define MEXTERN  __declspec(dllexport)
#else
#define CQEXTERN __declspec(dllimport)
#define MEXTERN  __declspec(dllimport)
#endif

#ifndef DMISSIONENUM_H
#include "DMissionEnum.h"
#endif

#ifndef DMTECHNODE_H
#include "DMTechNode.h"
#endif

//#ifndef ISPLAYERAI_H
//#include "ISPlayerAI.h"
//#endif

struct MPartRef;
//---------------------------------------------------------------------------
// Here are the EventFlags. These flags determine which game events cause a program instance to be created.
// By default, a program is only created by explicitly creating one.
// Can be any combination of the following:

#define CQPROGFLAG_STARTBRIEFING		0x00000001		// briefing is starting
#define CQPROGFLAG_STARTMISSION			0x00000002		// mission is starting
#define CQPROGFLAG_EDITOR				0x00000004		// editor command (for testing)
#define CQPROGFLAG_OBJECTDESTROYED		0x00000008		// a ship or platfrom has been destroyed
#define CQPROGFLAG_OBJECTCONSTRUCTED	0x00000010		// a ship has finished being constructed
#define CQPROGFLAG_OBJECTDEPLOYED		0x00000020		// a satalite has been deployed
#define CQPROGFLAG_RECOVERY_PICKUP		0x00000040		// a harvester has just picked up a recovery object
#define CQPROGFLAG_RECOVERY_DROPOFF		0x00000080		// a harvester droped off a recovery object

#define CQPROGFLAG_ENEMYSIGHTED			0x00000100		// An enemy sighted comm has happened...
#define CQPROGFLAG_FORBIDEN_JUMP		0x00000200		// the user tried to order a ship though a frobidden jumnpgate
#define CQPROGFLAG_UNITHIT				0x00000400		// A unit has been hit.  will only be triggered if the bAllEventsOn flag is set for the object
#define CQPROGFLAG_UNDERATTACK			0x00000800		// A unit is under attack
#define CQPROGFLAG_TROOPSHIPED			0x00001000		// Someone was troopshiped.
#define CQPROGFLAG_HOTKEY_1				0x00001000		// hotkey 1.
#define CQPROGFLAG_HOTKEY_2				0x00002000		// hotkey 2.
#define CQPROGFLAG_HOTKEY_3				0x00004000		// hotkey 3.
#define CQPROGFLAG_HOTKEY_4				0x00008000		// hotkey 4.
#define CQPROGFLAG_HOTKEY_5				0x00010000		// hotkey 5.


//---------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE CQBaseProgram
{
	virtual void Initialize (U32 eventFlags, const MPartRef & part) = 0;

	virtual bool Update (void) = 0;		// return false to discontinue the program

	virtual void Destroy (void) = 0;
};

//---------------------------------------------------------------------------
//
typedef CQBaseProgram * (__cdecl * CQPROGFACTORY) (void);

struct CQSCRIPTENTRY
{	
	const CQSCRIPTENTRY * pNext;
	const char *progName;
	const char *saveLoadStruct;
	S16   saveOffset;			// offset within class to save/load struct
	U16   saveSize;				// size of the save structure
	U32   eventFlags;			// See eventFlag documentation, above
	CQPROGFACTORY factory;
};

//---------------------------------------------------------------------------
//
struct CQSCRIPTDATADESC
{
	U32 size;					// size of the struct
	void * preprocessData;		// pointer to preprocessed data def
	U32 preprocessSize;			// size of data def
	const char *typeName;		// save struct type
	void * data;				// address of save struct
	U32 typeSize;				// size of the save struct
	struct HINSTANCE__ * hLocal;				// handle to the localization module
};

//---------------------------------------------------------------------------
//
struct CQBRIEFINGITEM
{
	U32 size;

	CQBRIEFINGITEM (void)
	{
		size = sizeof(CQBRIEFINGITEM);
	}
	char szFileName[32];
	char szTypeName[32];
	U32  slotID;
	U32  dwTimer;
	bool bHighlite;
	bool bContinueAnimating;
	bool bLoopAnimation;
};

//---------------------------------------------------------------------------
//
struct CQBRIEFINGTELETYPE
{
	wchar_t * pString;
	COLORREF color;
	U32 lifeTime;
	U32 textTime;
	bool bMuted;
};

//---------------------------------------------------------------------------
//
struct SPLASHINFO
{
	char vfxName[64];
	SINGLE speed;
	bool bAllowExit;
};

//----------------------------------------------------------------------------
//
// The structs used to define a Conquest player AI

enum AI_DIFFICULTY
{
	CAKEWALK,
	EASY,
	MEDIUM,
	HARD,
	LILBASTARD
};

enum AI_STRATEGY
{
	TERRAN_CORVETTE_RUSH,
	TERRAN_FORWARD_BUILD,
	TERRAN_DREADNOUGHTS,
	MANTIS_FRIGATE_RUSH,
	MANTIS_FORTRESS,
	MANTIS_FORWARD_BUILD,
	MANTIS_SWARM,
	SOLARIAN_FORWARD_BUILD,
	SOLARIAN_FORGERS,
	SOLARIAN_DENY
};

struct AIPersonality
{
	AIPersonality (void);

	U32				size;						// size of the structure, for run-time verification		
	AI_DIFFICULTY	difficulty;
	
	struct 
	{
		U32			bBuildPlatforms			:	1;	//toggles fabrication of platforms. true means it does.
		U32			bBuildLightGunboats		:	1;	//on/off light gunboat construction. See MGlobals.cpp
		U32			bBuildMediumGunboats	:	1;	//on/off med gunboat construction. See MGlobals.cpp
		U32			bBuildHeavyGunboats		:	1;	//on/off big gunboat construction. See MGlobals.cpp
		U32			bHarvest				:	1;	//on/off harvesters being built and sent to nebulas etc
		U32			bScout					:	1;	//on/off ships are sent to scout undiscovered terrain
		U32			bResearchTechs			:	1;	//on/off researching techs like Hull, Weapons, etc.
		U32			bBuildAdmirals			:	1;	//on/off building fleet admirals
		U32			bLaunchOffensives		:	1;	//on/off aggressively attack enemies, or stay at home
		U32			bVisibilityRules		:	1;	//on/off never act on something that can't be seen
		U32			bUnitDependencyRules	:	1;	//on/off build ships w/o the necessary plats
		U32			bCorvsOrMissiles		:	1;	//for corvette rush, use corvs or mcruisers
		U32			bResignationPossible	:	1;  //should the AI be allowed to resign if it wants to
		U32			bSendTaunts				:	1;	//should the AI send taunts to opponents
		U32			bUseSpecialWeapons		:	1;	//should the AI send taunts to opponents
		U32			bRandomBit1				:	1;
		U32			bRandomBit2				:	1;
		U32			bRandomBit3				:	1;
		U32			bRandomBit4				:	1;
		U32			bRandomBit5				:	1;
		U32			bRandomBit6				:	1;
		U32			bRandomBit7				:	1;
		U32			bExtraBits				:	10;
	} buildMask;
	
	S32				nOffenseVsDefense;			// 0-100 offvsdef setting currently affects nothing
	S8				nHarvestEscorts;			// 0-5   number of gunboats to use to escort each harvester
	SINGLE			fHarvestersPerRefinery;		// 0-5	 2.0 or more is a lot, 0.8 seems good MEANINGLESS
	S8				nMaxHarvesters;				// 0-50	 10 is probably good
	S8				nNumFabricators;			// 0-10	 2 is good
	S8				nNumFleets;					// 1-5   1, 2, 3 good
	U8				uNumScouts;					// 0-30	 num spyships to build and use to scout
	S8				nNumTroopships;				// 0-10	 num troopships to try to have in operation
	S8				nNumMinelayers;				// 0-10	 num minelayers to try to have in operation
	S8				nFabricateEscorts;			// 0-10  num gunboats to accompany each fabricator for safety
	S8				nGunboatsPerSupplyShip;		// 0-50  I usually use 8, meaning 1 supship for evry 8 gboats 
	S8				nCautiousness;				// 1-10  affects sending of fabs/harvesters to enemy terr.  at 10 they are most unlikely to send them
	S8				nBuildPatience;				// 0-10  0 is most patient, 10 the least.  At 0, the AI will wait for enough resources to be accumulated to build what it REALLY wants.  At 10, it will settle for whatever it has the resources to build when the fabricator becomes idle, just for speed-building
	S8				nShipyardPatience;			// 0-20  0 is most patient, 20 builds whatever it can
	S32				nPanicThreshold;
};
//-------------------------------------------------------------
//
inline AIPersonality::AIPersonality (void)
{
	size = sizeof(*this);
	difficulty = HARD;
	memset(&buildMask, -1, sizeof(buildMask));		// all bits enabled by default
	buildMask.bSendTaunts = false;
	buildMask.bResignationPossible = false;
	buildMask.bVisibilityRules = false;
	//buildMask.bUnitDependencyRules = false;
	nOffenseVsDefense = 50;
	nHarvestEscorts = 0;							//number of gunboats to use to escort each harvester
	fHarvestersPerRefinery = 0.8;
	nMaxHarvesters = 10;
	nNumFabricators = 2;
	nNumFleets = 4;
	uNumScouts = 1;
	nNumTroopships = 0;  //troopships not built is default behavior, turn this on, TURN ON THE FUN
	nNumMinelayers = 1;  //1 is temp for testing
	nFabricateEscorts = 0;
	nGunboatsPerSupplyShip = 8;
	nCautiousness = 2;
	nBuildPatience = 5;
	nShipyardPatience = 5;
	nPanicThreshold = 1500;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//

//---------------------------------------------------------------------------
//
struct MScript
{
	/*
	 *	Called by DLLMain in the script DLL, not intended for use by script writer.
	 */
    MEXTERN static void RegisterScriptProgram (CQSCRIPTENTRY * pTable);

	/*
	 *	Called by DLLMain in the script DLL, not intended for use by script writer.
	 */
	MEXTERN static void SetScriptData (const CQSCRIPTDATADESC & desc);

	/*
	 * Called to trigger all programs of a certain event type
	 */ 
	MEXTERN static void RunProgramsWithEvent (U32 eventType, const MPartRef & part, DWORD extendendInfo = 0);

	/*
	 * Called to trigger all programs of a certain event type
	 */ 
	MEXTERN static void RunProgramsWithEvent (U32 eventType, U32 dwMissionID, DWORD extendendInfo = 0);

	/*
	 * Called to trigger all programs of a certain event type
	 */ 
	MEXTERN static void RunProgramsWithEvent (U32 eventType);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void RunProgramByName (const char * progName, const MPartRef & part);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void RunProgramByName (const char * progName, U32 dwMissionID);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static DWORD GetExtendedEventInfo (void);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static MPartRef GetExtendedEventPartRef (void);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static MPartRef CreatePart (const char *pArchetype, const MPartRef & location, U32 playerID, const char * partName = NULL);

	/*
	 *  Documentation goes here!
	 *  WARNING: This method is slow. Avoid using it if possible.
	 */
	MEXTERN static MPartRef GetPartByName (const char *szPartName);
	
	/*
	 *  Documentation goes here!
	 */
	MEXTERN static MPartRef GetPartByID (U32 dwMissionID);

	/*
	 *  This Method returns the first MPartRef in the object list
	 */
	MEXTERN static MPartRef GetFirstPart (void);

	/*
	 *  This Method returns the next MPartRef in the object list or an invalid part if it is the end of the list
	 */
	MEXTERN static MPartRef GetNextPart (const MPartRef & part);

	/*
	 *  This Method returns the Jump PLAT on the other side of the wormhole for the given Jump PLAT
	 */
	MEXTERN static MPartRef GetJumpgateSibling (const MPartRef & part);

	/*
	 *  This Method returns the System ID which the given Jump PLAT's wormhole takes you to
	 */
	MEXTERN static U32 GetJumpgateDestination (const MPartRef & part);

	MEXTERN static MPartRef GetJumpgateWormhole (const MPartRef & part);

	/*
	 *  This Method returns the Jumpgate on the other side of the wormhole for the given jumpgate
	 */
	MEXTERN static MPartRef GetWormholeSibling (const MPartRef & part);

	/*
	 *  This Method returns the System ID which the given Jumpgate wormhole takes you to
	 */
	MEXTERN static U32 GetWormholeDestination (const MPartRef & part);

	/*
	 *  This Method returns the ID of the part towing the passed in part
	 */

	MEXTERN static MPartRef GetTowerID( const MPartRef & part );

	/*
	 *  Sets the part's name.  Wheee !
	 */
	MEXTERN static void SetPartName (const MPartRef & part, const char * name);

	MEXTERN static void SetReady (const MPartRef & part, bool bSetting);

	/*
	 *  This method returns the cost of the archetype name supplied.
	 */
	MEXTERN static struct ResourceCost GetResourceCost(const char * name);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void EnableMouse (bool bEnable);

	MEXTERN static bool IsMouseEnabled (void);

	/*
	 *  Turns the Briefing Control Dialog on or off
	 */
	MEXTERN static void EnableBriefingControls (bool bEnable);

	MEXTERN static void EnableRegenMode (bool bEnable);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void SetAvailiableTech (TECHNODE node);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static struct TECHNODE GetPlayerTech (U32 playerID);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void SetPlayerTech (U32 playerID, TECHNODE node);

    /*
	 *	Get a counted string from a stringID
	 *
	 */
	MEXTERN static wchar_t * GetCountedString (U32 stringID);

	/*
	 *  Seaches for a specific part (by name), and returns "true" if found
	 */
	MEXTERN static bool IsPartValid (const char *szPartName);


/* Player controls */
	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void SetAllies (U32 playerID, U32 allyID, bool bEnable);

	/*
	 *  Gets the players view of his ally
	 */
	MEXTERN static bool GetAllies (U32 playerID, U32 allyID);

	/*
	 *  return true if both parties are allied
	 */
	MEXTERN static bool AreAllies (U32 playerID1, U32 playerID2);

	/*
	 * Returns true if given player has any platforms left
	 */
	MEXTERN static bool PlayerHasPlatforms(U32 playerID);

	/*
	 * Returns true if given player has any platforms left in a system
	 */
	MEXTERN static bool PlayerHasPlatformsInSystem (U32 playerID, U32 systemID);

	/*
	 * Returns true if given player has any units left
	 */
	MEXTERN static bool PlayerHasUnits (U32 playerID);


/* Mission Controls */

	/*
	 *  End The Mission in Victory
	 */
	MEXTERN static void EndMissionVictory (U32 missionCompletedID = 0);

	/*
	 *  End The Mission in Defeat
	 */
	MEXTERN static void EndMissionDefeat (void);

	/*
	 *  End The Mission and Show Splash Screen
	 */
	MEXTERN static void EndMissionSplash (const char * vfxName,SINGLE speed, bool bAllowExit);

/* Camera Controls */

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void EnableMovieMode (bool bEnable);
	
	/*
	 *  for switching to a specific camera object
	 *  MOVIE_CAMERA::ZERO is a good default value for flags
	 */
	MEXTERN static void ChangeCamera (const MPartRef & camera, SINGLE time, S32 flags);

	/* remove this one
	 * for moving to a normal location(vied just like it was in the game)
	 * MEXTERN static void MoveCamera(S32 xLoc, S32 yLoc, S32 zLoc,U32 systemID,SINGLE time = 0.0, S32 flags = MOVIE_CAMERA::ZERO);
	 */
	
	/*
     *  For moving to an object from the regular game view.
	 *  Good defaults: time = 0.0, flags = MOVIE_CAMERA::ZERO
	 */
	MEXTERN static void MoveCamera (const MPartRef & location, SINGLE time, S32 flags);

	/*
	 *	Sets the source ship to be used with an upcomming ChangeCamera call
	 */
	MEXTERN static void CameraSourceShip (const MPartRef & sourceShip, SINGLE xLoc, SINGLE yLoc, SINGLE zLoc);

	/*
  	 *  Sets the target ship to be used with an upcomming Change Camera call
	 */
	MEXTERN static void CameraTargetShip (const MPartRef & targetShip);

	/*
	 *	Clears the Queue of all Camera calls
	 */
	MEXTERN static void ClearCameraQueue (void);

	/*
	 *  Documentation goes here!
	 *  Saves the current position of the camera
	 */
	MEXTERN static void SaveCameraPos (void);

	/*
	 *  Creates a ChangeCamera call that used the saved camera
	 *	good defaults: time = 0.0, flags = MOVIE_CAMERA::ZERO
     *		documentation should go here!
	*/
	MEXTERN static void LoadCameraPos (SINGLE time, S32 flags);

/* Object Generator Controls */

	/*
	 *  set how often a generator will create a ship
	 *  Documentation goes here!
	 */
	MEXTERN static void SetGenFrequency (const MPartRef & generator, SINGLE mean, SINGLE minDiference);

	/*
	 *  set the type of ship to create
	 */
	MEXTERN static void SetGenType (const MPartRef & generator, const char * partType);

	/*
	 *  turn the generator on or off
	 */
	MEXTERN static void GenEnable (const MPartRef & generator, bool bEnable);

	/*
	 *  explicitly create a ship with the generator
	 */
	MEXTERN static MPartRef GenForceGeneration (const MPartRef & generator);

/* Trigger Controls */

	/*
	 *  turn a trigger on or off
	 */
	MEXTERN static void EnableTrigger (const MPartRef & trigger, bool bEnable);

	/*
	 * 	default value for bAddTo is false
	 * //set the filter for a trigger
	 */
	MEXTERN static void SetTriggerFilter (const MPartRef & trigger, U32 number, U32 flags, bool bAddTo);

	/*
	 *   set the range for a trigger
	 */
	MEXTERN static void SetTriggerRange (const MPartRef & trigger, SINGLE range);

	/*
	 *   set the program to run for a trigger
	 */
	MEXTERN static void SetTriggerProgram (const MPartRef & trigger, const char * progName);

	/*
	 *  find the last object that triggered the trigger
	 */
	MEXTERN static MPartRef GetLastTriggerObject (const MPartRef & trigger);

/* general part methods */

	/*
	 *  Removes a part from the game (no Explosion) 
	 */
	MEXTERN static void RemovePart (const MPartRef & part);

	/*
	 *  Makes an object explode
	 */
	MEXTERN static void DestroyPart (const MPartRef & part);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static bool IsDead (const MPartRef & part);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static bool IsDeadToPlayer (const MPartRef & part, U32 playerID);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void SetHullPoints (const MPartRef & part, U32 amount);

	/*
	 *  Documentation goes here!
	 */
    MEXTERN static void SetMaxHullPoints(const MPartRef & part, U32 amount);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void SetSupplies (const MPartRef & part, U32 amount);
	
	/*
	 *  Documentation goes here!
	 */
	MEXTERN static bool IsVisibleToPlayer (const MPartRef & part, U32 playerID);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void SetVisibleToPlayer (const MPartRef & part, U32 playerID);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void ClearVisibility (const MPartRef & part);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static bool IsPlatform (const MPartRef & part);

	/*
	 *  Documentation goes here!
	 */
    MEXTERN static bool IsGunboat (const MPartRef & part);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static bool IsSelected (const MPartRef & part);


	MEXTERN static void SelectUnit (const MPartRef & part);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void EnableSelection (const MPartRef & part, bool selectable);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void EnableJumpCap (const MPartRef & part, bool jumpOk);
	/*
	 *  Documentation goes here!
	 */

	MEXTERN static void EnableMoveCap (const MPartRef & part, bool moveOk);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void EnableAttackCap (const MPartRef & part, bool attackOk);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void EnableSpecialAttackCap (const MPartRef & part, bool attackOk);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void EnableSpecialEOACap (const MPartRef & part, bool attackOk);

	/*
	 *  Changes the dwMissionID of part as a side-effect.
	 */
	MEXTERN static void MakeDerelict (MPartRef & part);

	/*
	 * Makes the object invincible
	 */
	MEXTERN static void MakeInvincible (MPartRef & part,bool value);

	/*
	 * Makes the no longer auto targeted
	 */
	MEXTERN static void MakeNonAutoTarget (MPartRef & part,bool value);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void MakeNonDerelict (const MPartRef & part);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void MakeJumpgateInvisible (const MPartRef & part, bool value);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void SwitchPlayerID (const MPartRef & part, U32 newPlayerID);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static bool WasPart(const MPartRef & oldPart, U32 newPartID);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void EnableGenerateAllEvents (const MPartRef & part, bool bEnable);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void EnablePartnameDisplay (const MPartRef & part, bool bEnable);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static bool IsIdle (const MPartRef & part);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static bool IsSelectedUnique (const MPartRef & part);

	/*
	 *  returns the distance bettween two objects return -1 if one object does not exist or are in different systems.
	 */
	MEXTERN static SINGLE DistanceTo (const MPartRef & part1, const MPartRef & part2);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static bool IsFabBuilding (const MPartRef & part, U32 buildID);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static U32 GetFabBuildID (char * buildType);

	/*
	 *	Documentation goes here!
	 */
	MEXTERN static void EnableJumpgate (const MPartRef & part, bool jumpOn);

	/*
	 *	Documentation goes here!
	 */
	MEXTERN static void EnableSystem (U32 systemID, bool bEnable, bool bVisible = true);

	MEXTERN static U32 GetCurrentSystem (void);

	/*
	 *	Forces the visibily of all jump gates between system A and B so you are garanteed a path
	 */
	MEXTERN static void ClearPath (U32 systemA, U32 systemB, U32 playerID);

	/*
	 * Find the nearest enemy even a system away
	 */
	MEXTERN static MPartRef FindNearestEnemy (const MPartRef & part, bool bMultiSystem, bool bVisible);

	/*
	 *	Returns the number of empty build slots around a planet
	 */
	MEXTERN static U32 GetAvailableSlots (const MPartRef & part);

	MEXTERN static bool IsPlayerOnPlanet (const MPartRef & planet, U32 playerID);

	MEXTERN static void TeraformPlanet (const MPartRef & part, const char * planetType, SINGLE changeTime);


/* wormhole anim stuff */

	MEXTERN static U32 CreateWormBlast(const MPartRef & target, SINGLE gridRadius, U32 playerID, bool suckOut);

	MEXTERN static void FlashWormBlast(U32 wormID);

	MEXTERN static void CloseWormBlast(U32 wormID);

/* orders */
	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void OrderTeleportTo (const MPartRef & part, const MPartRef & target);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void OrderMoveTo (const MPartRef & part, const MPartRef & target, bool bQueueOn = false);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void OrderUseJumpgate (const MPartRef & part, const MPartRef & jumpgate, bool bQueueOn = false);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void OrderAttack (const MPartRef & part, const MPartRef & target, bool user_command = true);

	MEXTERN static void OrderAttackPosition (const MPartRef & part, const MPartRef & target, bool user_command = true);

    MEXTERN static void OrderEscort (const MPartRef & part, const MPartRef & target );

	/*
	 *  Supports special attacks, Area of effect, Mimic, and Probe
	 */
	MEXTERN static void OrderSpecialAttack (const MPartRef & part, const MPartRef & target);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void OrderSpecialAbility (const MPartRef & part);

	MEXTERN static void OrderDoWormhole(const MPartRef & part, U32 systemID);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void OrderBuildUnit (const MPartRef & part, char * buildName, bool queUnits = false );

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void OrderBuildPlatform (const MPartRef & part,MPartRef & target,char * buildName);

	/*
	 *  Order a harvester to harvest in a particular area
	 */
	MEXTERN static void OrderHarvest (const MPartRef & part, const MPartRef & target);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void OrderCancel (const MPartRef & part);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void SetStance (const MPartRef & part, UNIT_STANCE stance);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static UNIT_STANCE GetStance (const MPartRef & part);

	/*
	 *  Set a platform's rally point
	 */
	MEXTERN static void SetRallyPoint (const MPartRef & platform, const MPartRef & point);

	MEXTERN static void GiveCommandKit (const MPartRef & admiral, char * kitName);

	MEXTERN static const char * GetFormationName(const MPartRef & admiral);

/* group orders here */
	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void OrderMoveTo (const struct MGroupRef & group, const MPartRef & target, bool bQueueOn = false);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void OrderAttack (const struct MGroupRef & group, const MPartRef & target, bool user_command = true);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void OrderUseJumpgate (const struct MGroupRef & group, const MPartRef & jumpgate, bool bQueueOn = false);

/* fog of war */

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void ClearHardFog (const MPartRef & target, SINGLE range);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void MakeAreaVisible (U32 playerID, const MPartRef & target, SINGLE range);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void EnableFogOfWar (bool bEnable);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static SINGLE GetHardFogCleared (U32 playerID, U32 systemID);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void SetSpectatorMode (bool bSetting);

/* game Speed */

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void PauseGame (bool bPause);
	
	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void SetGameSpeed (S32 speed);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static S32 GetGameSpeed (void);

/* audio video text */

	MEXTERN static void BriefingSubtitle(U32 soundHandle, U32 subtitle);
	
	MEXTERN static bool IsAudioThere (const char * fileName);
	/*
	 *  Documentation goes here!
	 */
	MEXTERN static U32 PlayAudio (const char * fileName, U32 subtitle = 0);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static U32 PlayAudio (const char * fileName, const MPartRef & speaker, U32 subtitle = 0);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static U32 PlayCommAudio (const char * fileName,const MPartRef & speaker, U32 subtitle = 0);

	/*
	 *  filename: Name of the .WAV file to play. The method looks for a .TXT file with the same 
	 *      base name for the magpie descriptor file. Both files go in the MSPEECH directory.
	 *  animType: Name of a GT_ANIMATION type found in Gendata.db. Ask SeanB about how to create/use one.
	 *  x, y: Screen coordinates for animation, assuming 640x480 mode.
	 */
	MEXTERN static U32 PlayAnimatedMessage (const char * filename, const char * animType, S32 x, S32 y, U32 subtitle);
	/*
	 *  filename: Name of the .WAV file to play. The method looks for a .TXT file with the same 
	 *      base name for the magpie descriptor file. Both files go in the MSPEECH directory.
	 *  animType: Name of a GT_ANIMATION type found in Gendata.db. Ask SeanB about how to create/use one.
	 *  x, y: Screen coordinates for animation, assuming 640x480 mode.
	 *  speaker: Highlights an object while an animation is playing.
	 */
	MEXTERN static U32 PlayAnimatedMessage (const char * filename, const char * animType, S32 x, S32 y, const MPartRef & speaker, U32 subtitle);

	MEXTERN static void PlayFullScreenVideo (const char * filename);

	/*
	 *  Documentation goes here!
	 */
	MEXTERN static void __stdcall PlayMusic (const char * fileName);

	MEXTERN static void __stdcall GetMusicFileName (struct M_STRING & string);

	/*
	 *	Stop all media streams (audio/video)
	 */
	MEXTERN static void FlushStreams (void);

	/*
	 *	Documentation goes here!
	 */
	MEXTERN static BOOL32 IsStreamPlaying (U32 movie_handle);

	/*
	 *	Stop an audio/video stream
	 */
	MEXTERN static void EndStream (U32 movie_handle);


/* teletype control */
	/*
	 * lifetime = total time that control is on screen (in milliseconds)
	 * textTime = time it takes for all the text to get displayed (should be less than lifetime)
	 * lifetime = 10000 and textTime = 5000 are good defaults
	 */
	MEXTERN static U32 PlayTeletype (U32 stringID, U32 left, U32 top, U32 right, U32 bottom, COLORREF color, U32 lifetime, U32 textTime, 
									 bool bMuted, bool bCentered = false);

	MEXTERN static BOOL32 IsTeletypePlaying (U32 teletypeID);

	MEXTERN static BOOL32 IsCustomBriefingAnimationDone (U32 slotID);

	/*
	 *  End all teletyping that is going on
	 */
	MEXTERN static void FlushTeletype (void); 

    MEXTERN static U32 GetScriptStringLength( U32 stringID );

/* draw functions */

	/*
	 *  Draw a line from a 2D point to a 2D point
	 *  lifetime = total time that control is on the screen
	 *	travelTime = time it takes for the line to grow from (x1,y1) to (x2,y2) 
	 */
	MEXTERN static void DrawLine (U32 x1, U32 y1, U32 x2, U32 y2, COLORREF color, U32 lifetime, U32 travelTime);

	/*
	 *  draw a line from a 2D point to an object
	 *  
	 */
	MEXTERN static void DrawLine (U32 x, U32 y, const MPartRef & target, COLORREF color, U32 lifetime, U32 travelTime);

	/*
	 *  Draw a line from an object to another object
	 *  
	 */
	MEXTERN static void DrawLine (const MPartRef & object, const MPartRef & target, COLORREF color, U32 lifetime, U32 travelTime);

/* highlight stuff */

	/*
	 *	Highlight a sector in the sector map.
	 *  
	 */
	MEXTERN static void AlertSector (U32 systemID);

	/*
	 *  Highlight an object in the systemMap and optionaly in the sector map
	 *  
	 */
	MEXTERN static void AlertObjectInSysMap (const MPartRef & object, bool bSectorToo = true);

	/*
	 *  
	 */
	MEXTERN static void AlertMessage (const MPartRef & object, char * filename);

	/*
	 *  returns the ID of the anim, you will need to store this if you want to stop it.
	 */
	MEXTERN static U32 StartAlertAnim (const MPartRef & object);

	/*
	 *  Give it the anim id from the StartAlertAnim
	 */
	MEXTERN static void StopAlertAnim (U32 animID);

	/*
	 *  
	 */
	MEXTERN static void FlustAlertAnims ();

	/*
	 *  
	 */
	MEXTERN static void StartFlashUI (U32 hotkeyID);

	/*
	 *  
	 */
	MEXTERN static void StopFlashUI (U32 hotkeyID);

/* resource stuff */

	/*
	 *	Documentation goes here!
	 */
	MEXTERN static void SetGas (U32 playerID, U32 amount);

	/*
	 *	Documentation goes here!
	 */
	MEXTERN static U32 GetGas (U32 playerID);

	/*
	 *	Documentation goes here!
	 */
	MEXTERN static void SetMaxGas (U32 playerID, U32 amount);

	/*
	 *	Documentation goes here!
	 */
	MEXTERN static void SetMetal (U32 playerID, U32 amount);

	/*
	 *	Documentation goes here!
	 */
	MEXTERN static U32 GetMetal (U32 playerID);

	/*
	 *	Documentation goes here!
	 */
	MEXTERN static void SetMaxMetal (U32 playerID, U32 amount);
	
	/*
	 *	Documentation goes here!
	 */
	MEXTERN static void SetCrew (U32 playerID, U32 amount);

	/*
	 *	Documentation goes here!
	 */
	MEXTERN static U32 GetCrew (U32 playerID);

	/*
	 *	Documentation goes here!
	 */
	MEXTERN static void SetMaxCrew (U32 playerID, U32 amount);

	/*
	 *	Documentation goes here!
	 */
	MEXTERN static U32 GetTotalCommandPoints (U32 playerID);

	MEXTERN static void SetTotalCommandPoints (U32 playerID, U32 newTotal);

	/*
	 *	Documentation goes here!
	 */
	MEXTERN static S32 GetFreeCommandPoints (U32 playerID);

	/*
	 *	Documentation goes here!
	 */
	MEXTERN static U32 GetUsedCommandPoints (U32 playerID);

	MEXTERN static U32 GetMaxCommandPoints (U32 playerID);

	MEXTERN static void SetMaxCommandPoitns (U32 playerID, U32 cpMax);

	/* Mission Objectives methods */

	MEXTERN static void OpenObjectiveMenu (U32 numObjectives, U32 * objectiveArray);

	MEXTERN static void SetMissionName (U32 stringID);

	MEXTERN static void SetMissionID (U32 missionID);

	MEXTERN static void SetMissionDescription (U32 stringID);

	MEXTERN static void AddToObjectiveList (U32 stringID, bool bSecondary = false);

	MEXTERN static void RemoveFromObjectiveList (U32 stringID);

	MEXTERN static void MarkObjectiveCompleted (U32 stringID);

	MEXTERN static void MarkObjectiveFailed (U32 stringID);

	MEXTERN static bool IsObjectiveCompleted (U32 stringID);

    MEXTERN static bool IsObjectiveInList(U32 stringID);

	/* Briefing Menu methods */

	/*
	 *	These functions should only be called when the briefing menu is up.  They make sure our briefing stuff get's played
	 *	in the right spot on the briefing dialog.
	 * 
	 */
	MEXTERN static U32 PlayBriefingTalkingHead (const CQBRIEFINGITEM & item);

	MEXTERN static U32 PlayBriefingAnimation (const CQBRIEFINGITEM & item);

	MEXTERN static U32 PlayBriefingTeletype (U32 stringID, COLORREF color, U32 lifetime, U32 textTime, bool bMuted);

	MEXTERN static void FreeBriefingSlot (S32 slotID);


	/* AI controlling methods */   //JeffP

	MEXTERN static void EnableEnemyAI (U32 playerID, bool bOn, M_RACE race);

	MEXTERN static void EnableEnemyAI (U32 playerID, bool bOn, const char * szPlayerAIType);

	MEXTERN static void SetEnemyCharacter (U32 playerID, U32 stringNameID);

	MEXTERN static void SetEnemyAIRules (U32 playerID, const AIPersonality & rules);

	MEXTERN static void SetEnemyAITarget (U32 playerID, const MPartRef & object, U32 range, U32 systemID = 0);

	MEXTERN static void EnableAIForPart (const MPartRef & object, bool bEnable);	

	MEXTERN static void LaunchOffensive(U32 playerID, UNIT_STANCE stance = US_ATTACK);

	/* Resource modifying stuff from Jeff */

	MEXTERN static void ClearResources (U32 systemID);

	// you can pass in -1 for a resource value to NOT change that resource amount
	MEXTERN static void SetResourcesOnPlanet (MPartRef & planet, S32 gas, S32 metal, S32 crew);

    MEXTERN static S32 GetCrewOnPlanet (const MPartRef & planet);

    MEXTERN static S32 GetGasOnPlanet (const MPartRef & planet);

    MEXTERN static S32 GetMetalOnPlanet (const MPartRef & planet);

	/* color setting / getting methods */

	MEXTERN static void SetColorTable (COLORREF colors[9]);

	MEXTERN static void ResetColorTable (void);

private:

	static void Save (struct IFileSystem * outFile);

	// load the script DLL, 
	// then load the programs. (Run the "Start" program if no programs are loaded)
	// inFile can be NULL if you just want to load the script
	static void Load (struct IFileSystem * inFile = 0);

	static void LoadScript (void);

	static void Close  (void);

	static void Update (void);	// called at AI update rate

	static void Notify (U32 message, void * param);
// friends
	friend struct Mission;
};


//-------------------------------------------------------------------


#endif

