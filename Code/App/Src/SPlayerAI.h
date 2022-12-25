#ifndef SPLAYERAI_H
#define SPLAYERAI_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                SPlayerAI.cpp                             //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/SPlayerAI.h 118   10/20/00 2:59a Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef ISPLAYERAI_H
#include "ISPlayerAI.h"
#endif

#ifndef HEAPOBJ_H
#include <Heapobj.h>
#endif

#ifndef OBJSET_H
#include "ObjSet.h"
#endif

#ifndef EVENTSYS_H
#include <EventSys.h>
#endif

#ifndef MPART_H
#include "Mpart.h"
#endif

#ifndef MSCRIPT_H
#include <MScript.h>
#endif

#include "IPlanet.h"
#include "Sector.h"
#include "DNebula.h"

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#define SCOUT_HOPS					8
#define MAX_SCOUT_ROUTES			10
#define MAX_DANGER_POINTS			50
#define MAX_IONCLOUD_POINTS			100
#define M_MAX_PLANETS				100
#define CLOAK_DISTANCE				6.0
#define DREADTHRESH					14
#define DREADNOUGHTSHIELD_RADIUS	6
#define SYNTH_THRESH				40
#define PORTAL_RADIUS				5  //  fix  solarian portal's effect radius
#define ASSIGNMENT_STALL			30
#define METALGAS_MULT				25
#define CREW_MULT					10
#define OH_SHIT						200000
#define ACTING_WAIT					16  //cannot be lower than 16
#define TROOPSHIP_PRI				400
#define SIGHTSHIP_PRI				320
#define MINELAYER_PRI				450
#define DISTTONEXTSYS				(10e5 / GRIDSIZE)
#define GRIDSQUARE_CONV_M			(64 / 16)

#define CQMAP_RANGE_SMALL			2
#define CQMAP_RANGE_MEDIUM			5
#define CQMAP_RANGE_LARGE			10
#define CQMAP_RANGE_HUGE			20
//
//--------------------------------------------------------------------------//
//

enum AI_TECH_TYPE
{
	NOTECH,
	T_BATTLESHIPCHARGE,
	T_CARRIERPROBE,
	T_DREADNOUGHTSHIELD,
	T_ENGINEUPGRADE1,
	T_ENGINEUPGRADE2,
	T_ENGINEUPGRADE3,
	T_ENGINEUPGRADE4,
	T_FIGHTER1,
	T_FIGHTER2,
	T_FIGHTER3, //10
	T_GAS1,
	T_GAS2,
	T_GAS3,
	T_HULLUPGRADE1,
	T_HULLUPGRADE2,
	T_HULLUPGRADE3,
	T_HULLUPGRADE4,
	T_LANCERVAMPIRE,
	T_MISSILECLOAKING,
	T_MISSILEPAK1, //20
	T_MISSILEPAK2,
	//T_MISSILEPAK3,
	T_ORE1,
	T_ORE2,
	T_ORE3,
	T_SENSOR1,
	T_SENSOR2,
	T_SHIELDUPGRADE1,
	T_SHIELDUPGRADE2,
	T_SHIELDUPGRADE3,
	T_SHIELDUPGRADE4, //30
	T_SUPPLYUPGRADE1,
	T_SUPPLYUPGRADE2,
	T_SUPPLYUPGRADE3,
	T_SUPPLYUPGRADE4,
	T_TANKER1,
	T_TANKER2,
	T_TENDER1,
	T_TENDER2,
	T_TROOPSHIP1,
	T_TROOPSHIP2, //40
	T_TROOPSHIP3,
	T_WEAPONUPGRADE1,
	T_WEAPONUPGRADE2,
	T_WEAPONUPGRADE3,
	T_WEAPONUPGRADE4,

	M_REPULSOR,
	M_CAMOFLAGE,
	M_ENGINE1,
	M_ENGINE2,
	M_ENGINE3,
	M_FIGHTER1,
	M_FIGHTER2,
	M_FIGHTER3,
	M_FIGHTER4,
	M_FIGHTER5,
	M_GRAVWELL,
	M_HULL1,
	M_HULL2,
	M_HULL3,
	M_LEECH1,
	M_LEECH2,
	M_RAM1,
	M_RAM2,
	M_REPELCLOUD,
	M_SENSOR1,
	M_SENSOR2,
	M_SENSOR3,
	M_SHIELD1,
	M_SHIELD2,
	M_SHIELD3,
	M_SUPPLY1,
	M_SUPPLY2,
	M_SUPPLY3,
	M_TANKER1,
	M_TANKER2,
	M_TANKER3,
	M_TENDER1,
	M_TENDER2,
	M_TENDER3,
	M_TROOPSHIP1,
	M_WEAPON1,
	M_WEAPON2,
	M_WEAPON3,

	S_DESTABILIZER,
	S_CLOAKER,
	S_ENGINE1,
	S_ENGINE2,
	S_ENGINE3,
	S_ENGINE4,
	S_ENGINE5,
	S_HULL1,
	S_HULL2,
	S_HULL3,
	S_HULL4,
	S_HULL5,
	S_LEGIONAIRE1,
	S_LEGIONAIRE2,
	S_LEGIONAIRE3,
	S_LEGIONAIRE4,
	S_MASSDISRUPTOR,
	S_ORE1,
	S_ORE2,
	S_ORE3,
	S_GAS1,
	S_GAS2,
	S_GAS3,
	S_TRACTOR,
	S_SENSOR1,
	S_SENSOR2,
	S_SENSOR3,
	S_SENSOR4,
	S_SENSOR5,
	S_SHIELD1,
	S_SHIELD2,
	S_SHIELD3,
	S_SHIELD4,
	S_SHIELD5,
	S_SUPPLY1,
	S_SUPPLY2,
	S_SUPPLY3,
	S_SUPPLY4,
	S_SUPPLY5,
	S_SYNTHESIS,
	S_TANKER1,
	S_TANKER2,
	//S_TANKER3,
	//S_TANKER4,
	//S_TANKER5,
	S_TENDER1,
	S_TENDER2,
	S_WEAPON1,
	S_WEAPON2,
	S_WEAPON3,
	S_WEAPON4,
	S_WEAPON5,

	AI_TECH_END

};	

enum E_POINT_TYPE
{
	DANGER_POINT,
	ION_CLOUD_POINT,
	FAB_POINT
};

/*
enum AI_NEBULA_TYPE
{
	NONEB,
	CYGNUS,
	LITHIUM,
	CELCIUS,
	HELIOS,
	HADES,
	ION,
	ANTIMATTER
};
*/

enum ASSIGNMENTTYPE
{
	NOASSIGNMENT,
	SCOUT,
	ESCORT,
	MINING_RUN,
	DEFEND,
	ATTACK,
	RESUPPLY,
	REPAIR_ASS
};

struct ASSIGNMENT
{
	ASSIGNMENT *	pNext;
	ASSIGNMENTTYPE	type;
	U32				maxUnits;
	U32				targetID;
	ObjSet			set;
	U32				systemID;
	S32				militaryPower;
	U32				uStallTime;
	bool			bHasAdmiral;
	bool			bFabricator;
	U32				supships;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	void Init(void)
	{
		pNext = NULL;
		type = NOASSIGNMENT;
		maxUnits = MAX_SELECTED_UNITS;
		targetID = 0;
		systemID = 0;
		militaryPower = 0;
		uStallTime = 0;
		bHasAdmiral = false;
		bFabricator = false;
		supships = 0;
	}
};

struct GridSquare
{
	int threat;
	int importance;
};

struct BasePriority
{
	int			nPriority;
	int			nExponentialDecrement;
	int			nAdditiveDecrement;
};

struct BuildPriority : public BasePriority
{
	int			nPlanetMultiplier;
	int			nSystemMultiplier;
	int			nNumSlots;
	M_OBJCLASS	prerequisite;
	bool		bMutation;
	//second prereq
	//third prereq
};

struct ShipPriority : public BasePriority
{
	M_OBJCLASS			facility;
	//prerequisites have been handled in that CanIBuild function which calls HasDependencies()
};

struct ResearchPriority : public BasePriority
{
	M_OBJCLASS			facility;
	bool				bAcquired;
	AI_TECH_TYPE		prerequisite;
	//second prereq
	//third prereq
};

struct BuildSite
{
	U32				planet;
	S32				slot;
	U32				fabID;
	DOUBLE			dist;
	NETGRIDVECTOR	pos;
	M_OBJCLASS		plattype;
};

//volatile, so never save off
struct PlanetHolding
{
	U32				planet;
	bool			bHasRefinery;
	U32				system;
	//int			typeofplanet;
};

struct ScoutRoute
{
	NETGRIDVECTOR	positions[SCOUT_HOPS];
	U32				numhops;
};

struct CQMapCircle
{
	NETGRIDVECTOR	pos;
	U32				range;
};

struct AINebula : public CQMapCircle
{
	NEBTYPE			nebulaType;
};

struct EnemyStatus
{
	NETGRIDVECTOR	HQLocation;
	U32				numUnits;
	U32				numMilitaryUnits;
	S32				TotalMilitaryPower;
};

struct InvalidLocation
{
	NETGRIDVECTOR	loc;
	U32				hits;
};

struct UnitActor
{
	U32				mID;
	U32				timer;
};

//--------------------------------------------------------------------------//
// things that get written to disk, no volatile items
//
struct PLAYERAI_SAVELOAD
{
	AI_STRATEGY		strategy;
	AIPersonality	DNA;

	bool			m_bOnOff;

	S32 m_Gas;
	S32 m_GasPerTurn;
	S32 m_Metal;
	S32 m_MetalPerTurn;
	S32 m_Crew;
	S32 m_CrewPerTurn;
	S32 m_ComPts;
	S32 m_ComPtsUsedUp;
	S32 updateCount;
	S32 m_Age;

	U32					UnitsOwned[M_ENDOBJCLASS];
	BuildPriority		BuildDesires[M_ENDOBJCLASS];
	ResearchPriority	ResearchDesires[AI_TECH_END];
	ShipPriority		ShipDesires[M_ENDOBJCLASS];
	EnemyStatus			Enemies[MAX_PLAYERS];
	GridSquare			m_Threats[16][16][MAX_SYSTEMS];
	InvalidLocation		m_OffLimits[10];
	UnitActor			m_ActingUnits[100];
	M_RACE				m_nRace;
	S32					m_TotalMilitaryUnits;
	S32					m_TotalMilitaryPower;
	S32					m_TotalImportance;
	PlanetHolding		m_OwnedPlanets[M_MAX_PLANETS];
	U32					m_PlanetsUnderMyControl;
	U32					m_AdmiralsOwned;

	S32					m_AttackWaveSize;
	U32					m_StrategicTargetID;
	U32					m_StrategicTargetRange;
	U32					m_StrategicTargetSystem;
	U32					m_ThreatResponse;

	//spacial information
	U32					m_LastSpacialPointActor;
	U32					m_NumDangerPoints;
	CQMapCircle			m_DangerPoints[MAX_DANGER_POINTS];
	U32					m_NumIonCloudPoints;
	AINebula			m_IonCloudPoints[MAX_IONCLOUD_POINTS];

	//keeping track of my yet-to-be-built decisions
	U32					m_NumFabPoints;
	BuildSite			m_FabPoints[MAX_DANGER_POINTS];
	bool				m_bAssignsInvalid;
	bool				m_bTechInvalid;
	AI_TECH_TYPE		m_WorkingTech;

	NETGRIDVECTOR		m_HQLocation;
	U32					m_RepairSite;
	ScoutRoute			m_ScoutRoutes[MAX_SCOUT_ROUTES];
	S32					m_nNumScoutRoutes;

	//mode data
	U32					m_Terminate;  //targets an enemy playerID for total extermination, set to 0 normally
	U32					m_ResignComing;
	bool				m_bKillHQ;
	bool				m_bRegenResources;
	bool				m_bSystemSupplied[MAX_SYSTEMS];
	U32					m_UnitTotals[MAX_PLAYERS];
	bool				m_bWorldSeen;
	bool				m_bShipRepair;
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE SPlayerAI : public ISPlayerAI, IEventCallback, PLAYERAI_SAVELOAD
{
	BEGIN_DACOM_MAP_INBOUND(SPlayerAI)
	DACOM_INTERFACE_ENTRY(ISPlayerAI)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	//--------------------------------
	// member data
	//--------------------------------
	
	U32 eventHandle;		// connection handle
	U32	m_ArchetypeIDs[M_ENDOBJCLASS];

	ASSIGNMENT * pAssignments;
	U32 playerID;
	OBJPTR<IBaseObject>	m_StrategicTarget;


	//--------------------------------
	// class methods
	//--------------------------------
	
	SPlayerAI (void);
	~SPlayerAI (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
	
	// IEventCallback methods
	GENRESULT __stdcall Notify (U32 message, void *param);

	// ISPlayerAI methods	
	virtual void Update (void);		// at 15 fps

	BOOL32 Load (struct IFileSystem * inFile);
	BOOL32 Save (struct IFileSystem * outFile);
	void ResolveResolveAssociations (void);

	void init (U32 playerID, M_RACE race = M_NO_RACE);

	// SPlayerAI overridable methods
	//
	// top-level functions
	virtual void onIdleUnit (IBaseObject * obj);
	virtual void evaluate (void);

	//platform building stuff
	virtual void						initBuildDesires(void);
	virtual void						initResearchDesires(bool bFirstTime = false);
	virtual void						initShipDesires(void);
	void								initArchetypeIDs(void);
	virtual void						UpdateBuildDesires(void);
	virtual void						UpdateResearchDesires(void);
	virtual void						UpdateShipDesires(void);
	virtual void						UpdateFinalSolution(void);
	S32									UpdateTechStatus(bool bFull = true);  //returns numtechs acquired
	virtual AI_TECH_TYPE				ChooseNextTech(int * fail);
	virtual M_OBJCLASS					ChooseNextBuild(int * fail, M_RACE race, bool bPlat, bool bFab = true);
	virtual M_OBJCLASS					ChooseNextShip(int *fail, M_RACE race, M_OBJCLASS facil);
	virtual U32							ChooseBuildSite(M_OBJCLASS plattype, PARCHETYPE pArchetype, 
										U32 dwFabID, U32 systemID, Vector position, 
										const Vector *translation, S32* slot, DOUBLE* dist,
										NETGRIDVECTOR* loc, bool bResourcesMatter = false);
	virtual NETGRIDVECTOR				ChooseFreeBuildSite(PARCHETYPE pArchetype, U32 dwFabID, U32 systemID, Vector position, bool wormgen = false);
	virtual NETGRIDVECTOR				ChooseAssignmentSite(ASSIGNMENTTYPE aType);
	void								IncreaseSlotPriorities(S32 slotnum, S32 amount);
	
	//Units-owned info
	void								addPartToUnitsOwned (MPart & part);
	void								resetUnitsOwned (void);
	void								addPlanet(U32 planet, U32 system);
	void								clearPlanets(void);
	void								removePlanet(U32 planet);
	int									GetPlanetIndex(U32 planet);
	bool								IsPlanetOwned(U32 planet);

	//stage-of-game info
	virtual bool						ReadyForMilitary(void);
	virtual bool						ReadyForBuilding(void);
	virtual bool						ReadyForResearch(void);
	virtual bool						ReadyForHarvest(void);

	//Unit-specific idle handling functions
	virtual void						doFabricator (MPart & part);
	virtual void						doHQ (MPart & part);
	virtual void						doRefinery (MPart & part);
	virtual	void						doHarvest (MPart & part);
	virtual void						doSupplyShip (MPart & part);
	virtual void						doMinelayer (MPart & part);
	virtual void						doTroopship (MPart & part);
	virtual void						doShipyard (MPart & part);
	virtual void						doNavalAcademy (MPart & part);
	virtual bool						doResupply (MPart & part);
	virtual bool						doShipRepair (MPart & part);
	virtual void						doRepair (MPart & part);
	virtual bool						doScouting (MPart & part);
	virtual void						doFlagShip (MPart & part);
	virtual void						doPortal (MPart & part);
	
	// Task monitoring functions
	virtual void						MonitorAssignments(void);
	virtual void						FieldGeneral (ASSIGNMENT * node);
	virtual bool						FieldCorporal(U32 unitID, ASSIGNMENT * my_ass);
	bool								IsActing (U32 uID);
	U32									AllInSystem (ASSIGNMENT * node);
	U32									AllInFleet (ASSIGNMENT * node);
	bool								AssignmentMemberLost(MPart & part, ASSIGNMENT * node);

	virtual void						doAttack (ASSIGNMENT * assign, UNIT_STANCE stance = US_ATTACK, IBaseObject * targ = NULL, bool bForce = false);
	virtual bool						ReissueAttack(ASSIGNMENT *assign, IBaseObject * targ);
	virtual S32							CalcWaveSize(void);
	virtual void						doUpgrade (MPart & part);
	virtual void						doResearch (MPart & part);
	virtual void						assignGunboat (MPart & part);
	virtual void						ensureAssignmentFor (MPart & part);



	/* support methods */
	void				SinglePassDataUpdate(void);
	bool				CanIBuild(U32 pArcheID, void *fail);
	bool				CanIRepair(MPart & part);  //for fabs repairing plats only
	bool				HasDependencies(M_OBJCLASS oc, M_OBJCLASS *failtype);
	DOUBLE 				ShouldIBuild(OBJPTR<IPlanet> planet, PARCHETYPE pArchetype, BuildSite & site);
	bool				SiteTaken(U32 planet, S32 slot);
	void				resetUpdateCount (void);
	void				resetAssignments (void);		// flush the assignment list

	ASSIGNMENT *		createShadowList (void);

	void				SpreadThreat(S32 x, S32 y, U32 sys, MPart & part, bool bThreat);
	void				AddSpacialPoint(E_POINT_TYPE type, NETGRIDVECTOR pt, U32 range, int objclass, U32 planet, S32 slot, NEBTYPE nebula = NEB_ANTIMATTER);
	void				RemoveSpacialPoint(E_POINT_TYPE type, NETGRIDVECTOR pt, U32 range);
	U32					GetDanger(NETGRIDVECTOR pt);
	U32					HowManyFabPoints(int oc);
	U32					HowManyResearchFabPoints(U32 mID);
	BuildSite *			GetFabPoint(U32 mID);
	
	void				addToShadow (U32 dwMissionID, ASSIGNMENT * pShadow);
	void				copyShadowData (ASSIGNMENT ** ppShadow);		// destroys the shadow list	
	void				cullAssignments (void);
	
	IBaseObject *		findClosestPlatform (MPart & part, bool (__cdecl *compare )(enum M_OBJCLASS), bool bForRepair = 0);
	IBaseObject *		findNearbySupplyShip(MPart & part, U32 *fail);
	IBaseObject *		findGunplat(U32 systemID, GRIDVECTOR position);
	IBaseObject *		findSupplyEscortee(U32 systemID, GRIDVECTOR position);
	void				removeFromAssignments (MPart & part);
	void				RemoveAssMember(ASSIGNMENT * cur, MPart & part);

	ASSIGNMENT *		findAssignment (MPart & part);
	static U32			getNumJumps (U32 startSystem, U32 endSystem, U32 playerID);
	ASSIGNMENT *		findAssignmentFor (MPart & part);
	U32					findScoutingRoute (MPart & part, NETGRIDVECTOR position[SCOUT_HOPS], bool bFlip = false);
	U32					repeatScoutingRoute (MPart & part, NETGRIDVECTOR positions[SCOUT_HOPS]);

	virtual IBaseObject *		findStrategicTarget (MPart & part, bool bNewTarget = false, bool bIgnoreDistance = false);
	IBaseObject *		GetJuicyAreaEffectTarget (MPart & part, U32 range, U32 radius); //range = minimum denses sensing, radius = firing range of sp. weapon
	U32					getNumGunplats (void);
	U32					GetNumEnemiesAround(MPart & part, int range = PORTAL_RADIUS);
	IBaseObject *		GetMimic(IBaseObject *base);

	void				addAssignment (ASSIGNMENT * assign);	// add top the end of the list
	U32					getNumAssignments (ASSIGNMENTTYPE atype);
	ASSIGNMENT *		findAssignment (ASSIGNMENTTYPE atype);
	void				AddAllUnits(ASSIGNMENT * assign);
	bool				TargetAssigned (U32 target);

	IBaseObject *		findHarvestTarget (MPart & part);
	IBaseObject *		GetNugget (U32 sysID, U32 harvsysID, GRIDVECTOR position);
	IBaseObject *		GetAttackInfo (IBaseObject * base, UNIT_STANCE * stance = NULL);  //returns the unit's attack target

	//useful, general functions
	M_RACE				GetRace(int objclass);
	M_RACE				GetTechRace(AI_TECH_TYPE tech);
	bool				IsMilitary(int objclass);
	bool				IsAlly(U32 mID);
	bool				IsFabricator(int objclass);
	bool				IsOfflimits(NETGRIDVECTOR pos);
	void				AddOfflimits(NETGRIDVECTOR pos);
	S32					GetImportance(MPart & part);
	S32					GetRefineryImportance(MPart & part);
	S32					GetPowerRating(MPart & part);
	U32					GetOffensivePowerRating(MPart & part);
	U32					GetTechArcheID(AI_TECH_TYPE i);
	AI_TECH_TYPE		CouldResearch(MPart & part);
	bool				HasAdmiral(ASSIGNMENT * assign);
	bool				DoIHave(AI_TECH_TYPE t);
	virtual void		ShouldIResign(void);

	//verbal communication
	void				SendChatMessage(wchar_t *buffer, U8 sendToMask);
	void				SendThreateningRemark(U32 pID);
	void				SendCelebratoryRemark(U32 pID);
	void				SendDisparagingRemark(void);
	bool				ShouldIComment(void);

	//Hooks   -- these virtuals override functions in ISPlayerAI, the parent object
	virtual void		SetStrategicTarget (IBaseObject *obj, U32 range, U32 systemID = 0);
	virtual void		SetPersonality (const struct AIPersonality & settings);
	virtual void		LaunchOffensive (UNIT_STANCE stance = US_ATTACK);
	virtual void		Activate (bool bOnOff);

	virtual const char * getSaveLoadName (void) const;

	IDAComponent * getBase (void)
	{
		return static_cast<ISPlayerAI *> (this);
	}
};
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//---------------------------END SPlayerAI.h---------------------------------//
//---------------------------------------------------------------------------//
#endif