#ifndef DUNITSOUNDS_H
#define DUNITSOUNDS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DUnitSounds.h    							//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DUnitSounds.h 24    9/21/00 11:33a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#error You must include DBaseData.h first!
#endif

namespace UNITSOUNDS
{
enum PRIORITY
{
	LOW,
	MEDIUM,
	HIGH,
	ADMIRAL
};

union SPEECH
{
	struct SPACESHIP
	{
		M_STRING underAttack;
		M_STRING alliedAttack;
		M_STRING enemySighted;
		M_STRING selected;
		M_STRING destructionDenied;	// unit is too busy to blow up!
		M_STRING move;
		M_STRING aggravated;
		M_STRING constructComplete;
		M_STRING death;
	};

	struct RESUSER
	{
		M_STRING notEnoughGas;
		M_STRING notEnoughMetal;
		M_STRING notEnoughCrew;
		M_STRING notEnoughCommandPoints;
	};

	struct GUNBOAT : SPACESHIP
	{
		M_STRING attacking;
		M_STRING suppliesout;
		M_STRING specialAttack;
	} gunboat;
	
	struct HARVESTER : SPACESHIP
	{
		M_STRING harvestResource;
		M_STRING planetDepleted;
		M_STRING planetDepletedRedeploy;
	} harvester;
	
	struct FABRICATOR : SPACESHIP, RESUSER
	{
		M_STRING buildImposible;
		M_STRING constructStarted;
	} fabricator;
	
	struct MINELAYER : SPACESHIP
	{
	} minelayer;

	struct SUPPLYSHIP : SPACESHIP
	{
		M_STRING resupplyShips;
	} supplyship;

	struct TROOPSHIP : SPACESHIP
	{
		M_STRING attacking;
		M_STRING attackSuccess;
		M_STRING attackFailed;

	} troopship;

	struct FLAGSHIP : SPACESHIP
	{
		M_STRING attacking;
		M_STRING fleetdamage50;		//Fleet has taken more than 50% damage
		M_STRING fleetdamage75;		//Fleet has taken more than 75% damage
		M_STRING supplieslow;		//Fleet supplies are generally under 50%
		M_STRING suppliesout;		//Fleet supplies are out on some ships
		M_STRING battlegood;		//Fleet will have no problems with enemy	
		M_STRING battlemoderate;	//Fleet might win or lose battle
		M_STRING battlebleak;		//Fleet will have no chance to win battle
		M_STRING shipleaving;		//Command ship changing Flagships
		M_STRING admiralondeck;		//When Command ship attaches to Flagship
		M_STRING flagshipintrouble;	//When Flagship/Command Ship is taking lots of damage
		M_STRING transferfailed;	//When Flagship/Command Ship is taking lots of damage
		M_STRING scuttle;			//you'll never take me alive!!
	} flagship;

	struct PLATFORM
	{
		M_STRING underAttack;
		M_STRING alliedAttack;
		M_STRING enemySighted;
		M_STRING selected;
		M_STRING destructionDenied;	// unit is too busy to blow up!
		M_STRING constructComplete;
		M_STRING constructStarted;
		M_STRING buildDelayed;
		M_STRING research;
		M_STRING researchCompleted;
		M_STRING upgradeCompleted;
	};
	
	struct GENPLATFORM : PLATFORM, RESUSER
	{
	} platform;

	struct GUNPLAT : PLATFORM
	{
		M_STRING attacking;
		M_STRING suppliesout;
	} gunPlat;

};	// end union SPEECH

}  // end namespace UNITSOUNDS

typedef UNITSOUNDS::SPEECH MT_UNITSPEECH;

//---------------------------------------------------------------------------//
//
#endif
