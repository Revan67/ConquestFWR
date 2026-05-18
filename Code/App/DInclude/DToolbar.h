#ifndef DTOOLBAR_H
#define DTOOLBAR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DToolBar.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DToolbar.h 87    5/07/01 9:21a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DHOTBUTTON_H
#include "DHotButton.h"
#endif

#ifndef DSTATIC_H
#include "DStatic.h"
#endif

#ifndef DPROGRESSSTATIC_H
#include "DProgressStatic.h"
#endif

#ifndef DEDIT_H
#include "DEdit.h"
#endif

#ifndef DHOTSTATIC_H
#include "DHotStatic.h"
#endif

#ifndef DSHIPSILBUTTON_H
#include "DShipSilButton.h"
#endif

#ifndef DICON_H
#include "DIcon.h"
#endif

#ifndef DQUEUECONTROL_H
#include "DQueueControl.h"
#endif

#ifndef DTABCONTROL_H
#include "DTabControl.h"
#endif

#define TB_RACES 3
#define NUM_SPECIAL_ORDERS 12

//---------------------------------------------------------------------------
//
struct GT_TOOLBAR
{
	//
	// required (compile-time) members
	// 
	char vfxShapeType[GT_PATH];		// name of the vfxshape type -- not the filename
	char vfxToolBar[TB_RACES][GT_PATH];

	RECT contextRect;				// relative to toolbar
	RECT sysmapRect[TB_RACES];
	RECT sectorMapRect[TB_RACES];
	U32 topBarX,topBarY;

	//
	// runtime members
	//

	struct COMMON
	{
		HOTBUTTON_DATA rotateLeft, returnDefaultView, rotateRight, minimize, restore, options, chat, go, starMap, exitSysMap,
			hpDiplomacy, hpResearch,hpFleetOfficer,hpIndustrial,hpIdleCivilian,missionObjectives;
		STATIC_DATA gas,metal,crew;
		STATIC_DATA commandPts,shipclass;
		ICON_DATA inSupply,notInSupply;
	} common[TB_RACES];

//	BUILDBUTTON_DATA test;
//	EDIT_DATA testEdit;

 	struct NONE
	{
		STATIC_DATA nothing;
	} none;

	struct FABRICATOR
	{
		M_OBJCLASS type;		// for debugging
		HOTBUTTON_DATA sell,stop,repair;
		EDIT_DATA shipname;
		STATIC_DATA hull;
		TABCONTROL_DATA fabTab;
		struct TAB1
		{
			BUILDBUTTON_DATA plat0,plat1,plat2,plat3,plat4,plat5,plat6,plat7,plat8,plat9,plat10,plat11,plat12,plat13,plat14,plat15;
		}basicTab;
		struct TAB2
		{
			BUILDBUTTON_DATA plat16,plat17,plat18,plat19,plat20,plat21,plat22,plat23,plat24,plat25,plat26,plat27,plat28,plat29,plat30,plat31;
		}advancedTab;
		struct TAB4
		{
			BUILDBUTTON_DATA plat32,plat33,plat34,plat35,plat36,plat37,plat38,plat39,plat40,plat41,plat42,plat43,plat44,plat45,plat46,plat47;
		}defenceTab;
		struct TAB3
		{
			HOTSTATIC_DATA techarmor,techengine,techsheild,techsensors;
		}statisticsTab;
	} fabricator,M_Weaver,S_Forger;

	struct LINDUSTRIAL
	{
		M_OBJCLASS type;		// for debugging
		ICON_DATA inSupply,notInSupply;
		HOTBUTTON_DATA rally,stop;
		STATIC_DATA hull,metalStorage,gasStorage,crewStorage,location,disabledText;
		BUILDBUTTON_DATA build0,build1,build2,build3,build4,build5,build6,build7,build8,build9;
		QUEUECONTROL_DATA buildQueue;
	} lindustrial,hq,hindustrial,
		M_Cocoon,M_Niad,S_Acropolis,S_Pavilion,
		S_Sanctum,S_GreaterPavilion;

	struct GENERIC
	{	
		M_OBJCLASS type;		// for debugging
		ICON_DATA inSupply,notInSupply;
		STATIC_DATA hull,supplies,location,disabledText;
	} turret;

	
	struct RESEARCH
	{
		M_OBJCLASS type;		// for debugging
		ICON_DATA inSupply,notInSupply;
		HOTBUTTON_DATA stop;
		STATIC_DATA hull,supplies,metalStorage,gasStorage,crewStorage,location,disabledText;
		RESEARCHBUTTON_DATA research0,research1,research2,research3,research4,research5,research6,research7,research8,research9;
		QUEUECONTROL_DATA buildQueue;
	} M_Plantation,proplab,ballistics,outpost,advHull,awsLab,lrsensor,
		hanger,weapons,displacement,
		M_BlastFurnace,M_ExplosivesRange,M_CarrionRoost,M_BioForge,
		M_FusionMill,M_CarpacePlant,M_HybridCenter,M_PlasmaSpitter,
		S_HelionVeil,S_XenoChamber,S_Anvil,S_MunitionsAnnex,
		M_EyeStock,M_MutationColony,S_TurbineDock,S_Bunker;

	struct BUILD_RES
	{
		M_OBJCLASS type;		// for debugging
		ICON_DATA inSupply,notInSupply;
		HOTBUTTON_DATA stop,rally;
		STATIC_DATA hull,metalStorage,gasStorage,crewStorage,location,disabledText;
		RESEARCHBUTTON_DATA research0,research1,research2,research3,research4,research5,research6,research7,research8,research9,research10,research11,research12,research13,research14,research15;
		BUILDBUTTON_DATA build0,build1,build2,build3,build4,build5;
		QUEUECONTROL_DATA buildQueue;
	}M_Thripid,academy,refinery,M_Collector,M_GreaterCollector,
		M_WarlordTraining,S_SentinalTower,S_Citidel,T_HeavyRefinery,T_SuperHeavyRefinery,S_Oxidator;

	struct FLEET
	{
		M_OBJCLASS type;		// for debugging
		HOTBUTTON_DATA escort,patrol,stop;
		HOTBUTTON_DATA tacticPeace,tacticStandGround, tacticDefend, tacticSeek;
		TABCONTROL_DATA fabTab;
		struct TAB1
		{
			MULTIHOTBUTTON_DATA admiralHead;
			STATIC_DATA o_namearea,o_kills,o_hull;
			HOTBUTTON_DATA order1,order2,order3,order4,order5,order6;
			TABCONTROL_DATA weaponTab;
			struct W_TAB1
			{
				HOTBUTTON_DATA terran0,terran1,terran2,terran3,terran4;
			}terranTab;
			struct W_TAB2
			{
				HOTBUTTON_DATA mantis0,mantis1,mantis2,mantis3,mantis4;
			}mantisTab;
			struct W_TAB3
			{
				HOTBUTTON_DATA solarian0,solarian1,solarian2,solarian3,solarian4,solarian5;
			}solarianTab;
		}orderTab;
		struct TAB2
		{
			SHIPSILBUTTON_DATA ship0,ship1,ship2,ship3,ship4,ship5,ship6,ship7,ship8,ship9,ship10,ship11,ship12,ship13,ship14,ship15,ship16,ship17,ship18,ship19,ship20,ship21;
		}silTab;
		struct TAB3
		{
			STATIC_DATA namearea,hull,kills;
			HOTSTATIC_DATA techarmor,techsupply,techengine,techsheild,techweapon,techsensors,techspecial;
		}statTab;
	} fleet[TB_RACES];
	
	struct WARTURRET
	{
		M_OBJCLASS type;
		ICON_DATA inSupply,notInSupply;
		STATIC_DATA hull,supplies,location;
		HOTBUTTON_DATA fighterStanceNormal,fighterStancePatrol;
		HOTSTATIC_DATA techarmor,techsupply,techsheild,techweapon,techsensors,techspecial;
	}WarTurret[TB_RACES];

	struct INDIVIDUAL
	{	
		M_OBJCLASS type;		// for debugging
		HOTBUTTON_DATA patrol,stop,escort,stanceAttack,stanceDefend,stanceStand,stanceStop,
			supplyStanceAuto,supplyStanceNoAuto,supplyStanceResupplyOnly,cloak;
		HOTBUTTON_DATA fighterStanceNormal,fighterStancePatrol;
		MULTIHOTBUTTON_DATA specialweapon;
		EDIT_DATA namearea;
		STATIC_DATA hull,supplies,kills,metal,gas;
		HOTSTATIC_DATA techarmor,techsupply,techengine,techsheild,techweapon,techsensors,techspecial;
	} individual[TB_RACES];

	struct GROUP
	{
		M_OBJCLASS type;		// for debugging
		MULTIHOTBUTTON_DATA specialweapon;
		HOTBUTTON_DATA escort,patrol,stop,cloak;
		HOTBUTTON_DATA stanceAttack,stanceDefend,stanceStand,stanceStop,
			supplyStanceAuto,supplyStanceNoAuto,supplyStanceResupplyOnly;
		HOTBUTTON_DATA fighterStanceNormal,fighterStancePatrol;
		SHIPSILBUTTON_DATA ship0,ship1,ship2,ship3,ship4,ship5,ship6,ship7,ship8,ship9,ship10,ship11,ship12,ship13,ship14,ship15,ship16,ship17,ship18,ship19,ship20,ship21;
	} group[TB_RACES];
	
};

#endif
