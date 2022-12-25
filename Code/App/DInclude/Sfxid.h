#ifndef SFXID_H
#define SFXID_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 SFXID_H                                  //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

   $Header: /Conquest/App/DInclude/Sfxid.h 30    10/24/00 2:10p Jasony $

				   Sound effect ID's for Conquest sound effects.
*/
//--------------------------------------------------------------------------//
//

namespace SFX
{

enum ID
{
	INVALID=0,
	
	BUTTONPRESS1,
	BUTTONPRESS2,
	inRange, 
	startConstruction, 
	endConstruction, 
	zeroMoney, 
	lightindustryButton, 
	heavyindustryButton, 
	hitechindustryButton, 
	hqButton, 
	researchButton, 
	planetDepleted,
	harvestRedeploy, 
	researchCompleted,

	JG_ENTER1,
	JG_ENTER2,
	JG_ARRIVE1,
	JG_ARRIVE2,
	JG_AMBIENCE,
	
	//TERRAN SFX
	T_ARC_CANNON,
	T_DEFENSEGUN1,
	T_SHIELDHIT,
	T_SHIELDFIZZOUT,
	T_SHIELDFIZZIN,
	T_LASERTURRET_SML,
	T_CORVETTE_IMPACT,
	T_LASERTURRET_MED,
	T_LASERTURRET_LGE,
	T_MISSILE,
	T_FLYBY1,
	T_FIGHTERFLYBY1,
	T_FIGHTERFLYBY2,
	T_FIGHTERFLYBY3,
	T_FIGHTERFLYBY4,
	T_FIGHTERLAUNCH1,
	T_FIGHTERLAUNCH2,
	T_FIGHTERLAUNCH3,
	T_FIGHTERLAUNCH4,
	T_CONSTRUCTION,
	T_POWERUP,
	T_LSATFIRE,
	T_LSATDEPLOY,
	T_FIGHTEREXPLO,
	T_HARVESTERUNLOAD,
	T_IMPACTCORVETTE,
	T_IMPACTLSAT,
	T_TROOPRELEASE,
	T_IMPACTMISSILE,
	T_IMPACTFIGHTER,
	T_FIGHTERFIRE1,
	T_FIGHTERFIRE2,
	T_FIGHTERFIRE3,
	T_FIGHTERFIRE4,
	T_PROBELAUNCH,
	T_FIGHTERKAMIKAZE,
	T_TEMPESTLAUNCH,
	T_IMPACTDREADNOUGHT,
	T_GRAVWELL,
	T_TRACTORDEBRIS,
	T_PROBEFIZZLE,
	T_AEGISSHIELD,
	T_IONCHARGE,
	T_TROOPASSAULT,

	//MANTIS SFX
	M_FIGHTERLAUNCH1,
	M_FIGHTERLAUNCH2,
	M_FIGHTERLAUNCH3,
	M_FIGHTERLAUNCH4,
	M_FIGHTERFLYBY1,
	M_FIGHTERFLYBY2,
	M_FIGHTERFLYBY3,
	M_FIGHTERFLYBY4,
	M_PLASMASMALL,
	M_PLASMAMED,
	M_CONSTRUCTION,
	M_FIGHTERBLAST,
	M_PLASMACHARGE,
	M_SELLINGUNIT,
	M_SHIELDHIT,
	M_REPELLCLOUD,
	S_CONSTRUCTION,
	S_BALLENERGY,
	S_BEAM_LG,
	S_BEAM_MED,
	S_DESTABILIZER,
	S_DISRUPTORDAM,
	S_ECLIPSE,
	S_FORGERDEPLOY,
	S_LAUNCHDESTAB,
	S_LAUNCHDISRUP,
	S_LEGIONAIRE,
	S_LEGIONSUCCESS,
	S_MINELAYER,
	S_REPULSOR,
	S_SELLINGUNIT,
	S_STRATUM,
	S_SYNTHESIS,
	S_UNLOAD,
	S_BEAM_MED2,
	S_SHIELDHIT,
	S_BEAM_SML,

	//Nebula & Terrain
	NEBULA_ION,
	NEBULA_IONSPARK,
	NEBULA_PLASMA,
	NEBULA_ARGON,
	NEBULA_HELIOUS,
	NEBULA_HYADES,
	ASTEROID_FIELD,
	NEBULA_CELSIUS,
	NEBULA_CYGNUS,
	NEBULA_LITHIUM,
	
		
	//Explosions
	EXPT_L_SUB,
	EXPT_L_FINAL,
	EXPT_M_SUB,
	EXPT_M_FINAL,
	EXPT_S_SUB,
	EXPT_S_FINAL,
	EXPM_RIPPLE1,
	EXPM_RIPPLE2,
	EXPM_RIPPLE3,
	EXPM_RIPPLE4,
	EXPM_L_SUB,
	EXPM_L_FINAL,
	EXPM_M_SUB,
	EXPM_M_FINAL,
	EXPM_S_SUB,
	EXPM_S_FINAL,
	EXPS_L_SUB,
	EXPS_L_FINAL,
	EXPS_M_SUB,
	EXPS_M_FINAL,
	EXPS_S_SUB,
	EXPS_S_FINAL,
	CONFIRMATION,
	EXPLOSION,
	EXPLOSION1,
	EXPLOSION2,
	EXPLOSION3,
	EXPLOSION4,
	SECEXPLODE1,
	SECEXPLODE2,
	SECEXPLODE3,
	SECEXPLODE4,
	SMALLEXPLOSION,
	TELETYPE,
	SPECIALDENIED,
	BEACON,

	LAST	// marks the end of the list
};

}  // end namespace SFX

//--------------------------------------------------------------------------//
//------------------------------END SFXID.h---------------------------------//
//--------------------------------------------------------------------------//
#endif 
