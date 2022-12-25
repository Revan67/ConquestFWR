//--------------------------------------------------------------------------//
//                                                                          //
//                                DemoScript.cpp                            //
//                                                                          //
//               COPYRIGHT (C) 2004 Warthog Texas, INC.                     //
//                                                                          //
//--------------------------------------------------------------------------//
/*

*/
//--------------------------------------------------------------------------//

#include <ScriptDef.h>
#include <DLLScriptMain.h>
#include "DataDef.h"

MPartRef Mantis = MScript::GetPartByName("#802#Cocoon");
bool hotkey = true;
bool hotkey3 = true;
bool hotkey4 = true;

CQSCRIPTDATA(MissionData, data);

//Programs

CQSCRIPTPROGRAM(MyProg, MyProg_Save,CQPROGFLAG_STARTMISSION);

void MyProg::Initialize (U32 eventFlags, const MPartRef & part)
{

	data.mission_state = MissionStart;
	state = Begin;

	MScript::SetSpectatorMode(true);
//	MScript::EnableFogOfWar(false);
//	MScript::CreatePart("FSHIP!!T_Halsey", MScript::GetPartByName("#4050#"), 1);

	data.missionTech.InitLevel( TECHTREE::FULL_TREE );

	data.missionTech.race[0].build = (TECHTREE::BUILDNODE) (
		TECHTREE::RES_REFINERY_GAS1 |
		TECHTREE::RES_REFINERY_GAS2 |
		TECHTREE::RES_REFINERY_METAL1 |
		TECHTREE::RES_REFINERY_METAL2 |
        TECHTREE::TDEPEND_HEADQUARTERS | 
        TECHTREE::TDEPEND_REFINERY | 
        TECHTREE::TDEPEND_LIGHT_IND |
        TECHTREE::TDEPEND_JUMP_INHIBITOR |
        TECHTREE::TDEPEND_LASER_TURRET |
        TECHTREE::TDEPEND_BALLISTICS |
        TECHTREE::TDEPEND_TENDER |
		TECHTREE::TDEPEND_REPAIR |
		TECHTREE::TDEPEND_HEAVY_IND |
		TECHTREE::TDEPEND_ADVHULL |
		TECHTREE::TDEPEND_OUTPOST |
		TECHTREE::TDEPEND_HANGER  |
		TECHTREE::TDEPEND_SENSORTOWER  |
		TECHTREE::TDEPEND_ACADEMY  |
		TECHTREE::TDEPEND_HEAVY_IND |
		TECHTREE::TDEPEND_SPACE_STATION |
		TECHTREE::TDEPEND_PROPLAB  |
		TECHTREE::TDEPEND_AWSLAB  |
		TECHTREE::TDEPEND_DISPLACEMENT
        );

	data.missionTech.race[0].tech = (TECHTREE::TECHUPGRADE) (
        TECHTREE::T_SHIP__FABRICATOR | 
        TECHTREE::T_SHIP__HARVEST |
		TECHTREE::T_SHIP__SUPPLY |
        TECHTREE::T_SHIP__CORVETTE | 
        TECHTREE::T_SHIP__MISSILECRUISER |
		TECHTREE::T_SHIP__BATTLESHIP |
		TECHTREE::T_SHIP__TROOPSHIP |
  		TECHTREE::T_SHIP__CARRIER  |
		TECHTREE::T_SHIP__INFILTRATOR  |
		TECHTREE::T_SHIP__LANCER   |
		TECHTREE::T_SHIP__DREADNOUGHT   |
		TECHTREE::T_RES_TROOPSHIP1 |
		TECHTREE::T_RES_TROOPSHIP2 |
		TECHTREE::T_RES_TROOPSHIP3 |
		TECHTREE::T_RES_MISSLEPACK1 |
		TECHTREE::T_RES_MISSLEPACK2 |
		TECHTREE::T_RES_XCHARGES |
		TECHTREE::T_RES_XPROBE  |
		TECHTREE::T_RES_XCLOAK  |
		TECHTREE::T_RES_XVAMPIRE  |
		TECHTREE::T_RES_XSHIELD
        );

	data.missionTech.race[0].common_extra = (TECHTREE::COMMON_EXTRA) (
        TECHTREE::RES_SENSORS1  |
        TECHTREE::RES_SENSORS2  |
		TECHTREE::RES_TANKER1  |
		TECHTREE::RES_TANKER2  |
		TECHTREE::RES_TENDER1 |
		TECHTREE::RES_TENDER2 |
		TECHTREE::RES_FIGHTER1 |
		TECHTREE::RES_FIGHTER2 |
		TECHTREE::RES_FIGHTER3
        );

	data.missionTech.race[0].common = (TECHTREE::COMMON) (
		TECHTREE::RES_WEAPONS1  |
		TECHTREE::RES_WEAPONS2  |
		TECHTREE::RES_WEAPONS3  |
		TECHTREE::RES_WEAPONS4  |
		TECHTREE::RES_SUPPLY1 |
		TECHTREE::RES_SUPPLY2  |
		TECHTREE::RES_SUPPLY3  |
		TECHTREE::RES_SUPPLY4  |
		TECHTREE::RES_HULL1  |
		TECHTREE::RES_HULL2  |
		TECHTREE::RES_HULL3  |
		TECHTREE::RES_HULL4  |
		TECHTREE::RES_ENGINE1 |
		TECHTREE::RES_ENGINE2 |
		TECHTREE::RES_ENGINE3 |
		TECHTREE::RES_ENGINE4 |
		TECHTREE::RES_SHIELDS1 |
		TECHTREE::RES_SHIELDS2 |
		TECHTREE::RES_SHIELDS3 |
		TECHTREE::RES_SHIELDS4
        );


	data.missionTech.race[1].tech = (TECHTREE::TECHUPGRADE) (
//		TECHTREE::M_RES_XCAMO |
		TECHTREE::M_RES_XGRAVWELL |
//		TECHTREE::M_RES_XRCLOUD |
//		TECHTREE::M_RES_REPULSOR |
		TECHTREE::M_RES_EXPLODYRAM1  |
		TECHTREE::M_RES_EXPLODYRAM2  |
		TECHTREE::M_RES_LEECH1  |
		TECHTREE::M_RES_LEECH2
        );


	data.missionTech.race[1].common_extra = (TECHTREE::COMMON_EXTRA) (
        TECHTREE::RES_SENSORS1  |
        TECHTREE::RES_SENSORS2  |
        TECHTREE::RES_SENSORS3  |
        TECHTREE::RES_SENSORS4  |
		TECHTREE::RES_TANKER1  |
		TECHTREE::RES_TANKER2  |
		TECHTREE::RES_TENDER1 |
		TECHTREE::RES_TENDER2 |
		TECHTREE::RES_FIGHTER1 |
		TECHTREE::RES_FIGHTER2
        );

	data.missionTech.race[1].common = (TECHTREE::COMMON) (
		TECHTREE::RES_WEAPONS1  |
		TECHTREE::RES_WEAPONS2  |
		TECHTREE::RES_SUPPLY1  |
		TECHTREE::RES_SUPPLY2  |
		TECHTREE::RES_HULL1  |
		TECHTREE::RES_ENGINE1 |
		TECHTREE::RES_SHIELDS1
        );

	data.missionTech.race[3].common_extra = (TECHTREE::COMMON_EXTRA) (
        TECHTREE::RES_SENSORS1  |
        TECHTREE::RES_SENSORS2  |
        TECHTREE::RES_SENSORS3  |
        TECHTREE::RES_SENSORS4  |
		TECHTREE::RES_TANKER1  |
		TECHTREE::RES_TANKER2  |
		TECHTREE::RES_TENDER1 |
		TECHTREE::RES_TENDER2 |
		TECHTREE::RES_FIGHTER1 |
		TECHTREE::RES_FIGHTER2 |
		TECHTREE::RES_FIGHTER3
        );

	data.missionTech.race[3].common = (TECHTREE::COMMON) (
		TECHTREE::RES_WEAPONS1  |
		TECHTREE::RES_WEAPONS2  |
		TECHTREE::RES_WEAPONS3  |
		TECHTREE::RES_WEAPONS4  |
		TECHTREE::RES_WEAPONS5  |
		TECHTREE::RES_SUPPLY1  |
		TECHTREE::RES_SUPPLY2  |
		TECHTREE::RES_SUPPLY3  |
		TECHTREE::RES_SUPPLY4  |
		TECHTREE::RES_SUPPLY5  |
		TECHTREE::RES_HULL1  |
		TECHTREE::RES_HULL2  |
		TECHTREE::RES_HULL3  |
		TECHTREE::RES_HULL4  |
		TECHTREE::RES_HULL5  |
		TECHTREE::RES_ENGINE1 |
		TECHTREE::RES_ENGINE2 |
		TECHTREE::RES_ENGINE3 |
		TECHTREE::RES_ENGINE4 |
		TECHTREE::RES_ENGINE5 |
		TECHTREE::RES_SHIELDS1 |
		TECHTREE::RES_SHIELDS2 |
		TECHTREE::RES_SHIELDS3 |
		TECHTREE::RES_SHIELDS4 |
		TECHTREE::RES_SHIELDS5
        );

    MScript::SetPlayerTech(1, data.missionTech );
    MScript::SetPlayerTech(2, data.missionTech );
    MScript::SetPlayerTech(3, data.missionTech );	


}

bool MyProg::Update (void)
{

	AIPersonality airules;

	switch (state)
	{
		case Begin:
			{
				MPartRef NavalAcademy = MScript::GetPartByName("#11211#Naval Academy");
				MPartRef LightShipyard = MScript::GetPartByName("#B261#Light Shipyard");
				MPartRef HeavyShipyard = MScript::GetPartByName("#B251#Heavy Shipyard");
				MPartRef Fabricator = MScript::GetPartByName("#2C01#Fabricator ");
				MPartRef IonTarget = MScript::GetPartByName("#127E0#");

				MScript::OrderBuildUnit(NavalAcademy, "FSHIP!!T_Halsey", true);
				MScript::OrderBuildUnit(LightShipyard, "GBOAT!!T_Missile Cruiser", true);
				MScript::OrderBuildUnit(LightShipyard, "GBOAT!!T_Missile Cruiser", true);
				MScript::OrderBuildUnit(LightShipyard, "GBOAT!!T_Missile Cruiser", true);
			//	MScript::OrderBuildUnit(LightShipyard, "GBOAT!!T_Corvette", true);
				MScript::OrderBuildUnit(HeavyShipyard, "GBOAT!!T_Lancer Cruiser", true);
				MScript::OrderBuildUnit(HeavyShipyard, "GBOAT!!T_Lancer Cruiser", true);
				MScript::OrderBuildPlatform(Fabricator, IonTarget, "PLATGUN!!T_IonCannon");

				MScript::SetRallyPoint(NavalAcademy, MScript::GetPartByName("#4050#"));
				MScript::SetRallyPoint(LightShipyard, MScript::GetPartByName("#11200#"));
				MScript::SetRallyPoint(HeavyShipyard, MScript::GetPartByName("#11200#"));
				
				//MScript::CreatePart("FSHIP!!T_Halsey", MScript::GetPartByName("#4050#"), 1);

//				MScript::MakeJumpgateInvisible(MScript::GetPartByName("Gate3"), true);
				MScript::EnableJumpgate(MScript::GetPartByName("Gate3"), false);

				MScript::OrderHarvest(MScript::GetPartByName("#2BE1#Harvester"), MScript::GetPartByName("#AB50#"));
				MScript::OrderHarvest(MScript::GetPartByName("#2BF1#Harvester"), MScript::GetPartByName("#AB60#"));
				MScript::OrderHarvest(MScript::GetPartByName("#14AA1#Harvester"), MScript::GetPartByName("#AB50#"));
				MScript::OrderHarvest(MScript::GetPartByName("#14AB1#Harvester"), MScript::GetPartByName("#AB60#"));
				MScript::OrderHarvest(MScript::GetPartByName("#14AC1#Harvester"), MScript::GetPartByName("#AB50#"));
				MScript::OrderHarvest(MScript::GetPartByName("#14AD1#Harvester"), MScript::GetPartByName("#AB60#"));
				MScript::OrderHarvest(MScript::GetPartByName("#14AE1#Harvester"), MScript::GetPartByName("#AB50#"));
				MScript::OrderHarvest(MScript::GetPartByName("#14AF1#Harvester"), MScript::GetPartByName("#AB60#"));

				MScript::EnableEnemyAI(2, true, "MANTIS_FORTRESS");

				airules.difficulty = MEDIUM;
				airules.buildMask.bBuildPlatforms = false;
				airules.buildMask.bBuildLightGunboats = true;
				airules.buildMask.bBuildMediumGunboats = true;
				airules.buildMask.bBuildHeavyGunboats = true;
				airules.buildMask.bHarvest = true;
				airules.buildMask.bScout = false;
				airules.buildMask.bBuildAdmirals = false;
				airules.buildMask.bLaunchOffensives = false;
				airules.buildMask.bVisibilityRules = true;
				airules.nGunboatsPerSupplyShip = 10;
				airules.nNumMinelayers = 0;
				airules.uNumScouts = 0;
				airules.nNumFabricators = 0;
				airules.fHarvestersPerRefinery = 2;
				airules.nMaxHarvesters = 5;
				airules.nPanicThreshold = 2000;

				MScript::SetEnemyAIRules(2, airules);

				MPartRef MantisTrigger = MScript::GetPartByName("#CC00#");

				MScript::SetTriggerProgram(MantisTrigger, "Demo_MantisAttack");

				state = Finish;
			}
			break;

		case VyriumGoto:
			
			if (MScript::IsDead(Mantis) && hotkey == true)
			{
				state = Finish;
			}
			else if (hotkey == false)
				state = Finish;
			break;

		case Finish:
			return false;
	}
	return true;
}


//--------------------------------------------------------------------------//
//--Trigger Scripts-------------------------------------------------------//
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(Demo_MantisAttack, Demo_MantisAttack_Save,0);

void Demo_MantisAttack::Initialize (U32 eventFlags, const MPartRef & part)
{	
	state = Begin;
	MScript::EnableTrigger(part, false);
/*
	target = MScript::GetLastTriggerObject(part);

	if( target == Mantis1 )
	{
		MScript::EnableTrigger(part, false);
		state = Begin;
	}
	else
	{	state = Finish;	}
*/
}

bool Demo_MantisAttack::Update (void)
{
//	switch(state)
//	{
//		case Begin:
//		{
			MPartRef Tiamat1 = MScript::GetPartByName("#2612#Tiamat");
			MPartRef Tiamat2 = MScript::GetPartByName("#25F2#Tiamat");
			MPartRef Tiamat3 = MScript::GetPartByName("#2622#Tiamat");
			MPartRef Tiamat4 = MScript::GetPartByName("#2602#Tiamat");
			MPartRef Scarab1 = MScript::GetPartByName("#25E2#Scarab");
			MPartRef Scarab2 = MScript::GetPartByName("#2632#Scarab");
			MPartRef Scarab3 = MScript::GetPartByName("#25C2#Scarab");
			MPartRef Hive1 = MScript::GetPartByName("#15742#Hive Carrier");
			MPartRef Hive3 = MScript::GetPartByName("#15762#Hive Carrier");
			MPartRef Frigate1 = MScript::GetPartByName("#225D2#Frigate");
			MPartRef Frigate2 = MScript::GetPartByName("#225E2#Frigate");
			MPartRef Frigate3 = MScript::GetPartByName("#225F2#Frigate");
			MPartRef Frigate4 = MScript::GetPartByName("#22602#Frigate");
			MPartRef Frigate5 = MScript::GetPartByName("#22612#Frigate");
			MPartRef Frigate6 = MScript::GetPartByName("#22622#Frigate");
			MPartRef Frigate7 = MScript::GetPartByName("#22632#Frigate");
			MPartRef Frigate8 = MScript::GetPartByName("#22642#Frigate");

			MPartRef Tiamat1WP = MScript::GetPartByName("#CBD0#");
			MPartRef Tiamat2WP = MScript::GetPartByName("#CBE0#");
			MPartRef Tiamat3WP = MScript::GetPartByName("#CBA0#");
			MPartRef Tiamat4WP = MScript::GetPartByName("#CBC0#");
			MPartRef Scarab1WP = MScript::GetPartByName("#CBF0#");
			MPartRef Scarab2WP = MScript::GetPartByName("#CB90#");
			MPartRef Scarab3WP = MScript::GetPartByName("#CB80#");
			MPartRef Hive1WP = MScript::GetPartByName("#15730#");
			MPartRef Hive3WP = MScript::GetPartByName("#CB60#");
			MPartRef Frigate1WP = MScript::GetPartByName("#CB50#");
			MPartRef Frigate2WP = MScript::GetPartByName("#CB60#");

			MScript::OrderMoveTo(Tiamat1, Tiamat1WP, false);
			MScript::OrderMoveTo(Tiamat2, Tiamat2WP, false);
			MScript::OrderMoveTo(Tiamat3, Tiamat3WP, false);
			MScript::OrderMoveTo(Tiamat4, Tiamat4WP, false);
			MScript::OrderMoveTo(Scarab1, Scarab1WP, false);
			MScript::OrderMoveTo(Scarab2, Scarab2WP, false);
			MScript::OrderMoveTo(Scarab3, Scarab3WP, false);
			MScript::OrderMoveTo(Hive1, Hive1WP, false);
			MScript::OrderMoveTo(Hive3, Hive3WP, false);
			MScript::OrderMoveTo(Frigate1, Frigate1WP, false);
			MScript::OrderMoveTo(Frigate2, Frigate1WP, false);
			MScript::OrderMoveTo(Frigate3, Frigate1WP, false);
			MScript::OrderMoveTo(Frigate4, Frigate1WP, false);
			MScript::OrderMoveTo(Frigate5, Frigate2WP, false);
			MScript::OrderMoveTo(Frigate6, Frigate2WP, false);
			MScript::OrderMoveTo(Frigate7, Frigate2WP, false);
			MScript::OrderMoveTo(Frigate8, Frigate2WP, false);

			MScript::SetStance(Tiamat1, US_ATTACK);
			MScript::SetStance(Tiamat2, US_ATTACK);
			MScript::SetStance(Tiamat3, US_ATTACK);
			MScript::SetStance(Tiamat4, US_ATTACK);
			MScript::SetStance(Scarab1, US_ATTACK);
			MScript::SetStance(Scarab2, US_ATTACK);
			MScript::SetStance(Scarab3, US_ATTACK);
			MScript::SetStance(Hive1, US_ATTACK);
			MScript::SetStance(Hive3, US_ATTACK);
			MScript::SetStance(Frigate1, US_ATTACK);
			MScript::SetStance(Frigate2, US_ATTACK);
			MScript::SetStance(Frigate3, US_ATTACK);
			MScript::SetStance(Frigate4, US_ATTACK);
			MScript::SetStance(Frigate5, US_ATTACK);
			MScript::SetStance(Frigate6, US_ATTACK);
			MScript::SetStance(Frigate7, US_ATTACK);
			MScript::SetStance(Frigate8, US_ATTACK);

			state = Finish;
//		}
//	}
	return false;
}


CQSCRIPTPROGRAM(Demo_LeviathanAttack, Demo_LeviathanAttack_Save,0);

void Demo_LeviathanAttack::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MPartRef Leviathan1 = MScript::GetPartByName("#F03#Leviathan");
	target = MScript::GetLastTriggerObject(part);

	if (MScript::IsDead(Leviathan1))
	{	state = Finish;	}
	else if( target == Leviathan1 )
	{
		MScript::EnableTrigger(part, false);
		state = Begin;
	}
	else
	{	state = Finish;	}
}

bool Demo_LeviathanAttack::Update (void)
{
	switch(state)
	{
		case Begin:
		{
			MPartRef Leviathan1 = MScript::GetPartByName("#F03#Leviathan");

			MPartRef Target = MScript::GetPartByName("#60#Earth Planet");

			MScript::OrderSpecialAttack(Leviathan1, Target);

			MScript::MakeInvincible(Leviathan1, false);

		//	MScript::SetStance(Leviathan1, US_ATTACK);


			MPartRef Leviathan2 = MScript::GetPartByName("#FB93#Leviathan");
			MPartRef Leviathan3 = MScript::GetPartByName("#FBC3#Leviathan");
			MPartRef Leviathan4 = MScript::GetPartByName("#FBA3#Leviathan");

			MPartRef Leviathan2WP = MScript::GetPartByName("#FBF0#");
			MPartRef Leviathan3WP = MScript::GetPartByName("#FC00#");
			MPartRef Leviathan4WP = MScript::GetPartByName("#FC10#");

			MScript::OrderMoveTo(Leviathan2, Leviathan2WP, false);
			MScript::OrderMoveTo(Leviathan3, Leviathan3WP, false);
			MScript::OrderMoveTo(Leviathan4, Leviathan4WP, false);

			MScript::SetStance(Leviathan2, US_ATTACK);
			MScript::SetStance(Leviathan3, US_ATTACK);
			MScript::SetStance(Leviathan4, US_ATTACK);

			state = Finish;
		}
	}
	return false;
}

CQSCRIPTPROGRAM(Demo_Crotal1Attack, Demo_Crotal1Attack_Save,0);

void Demo_Crotal1Attack::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MPartRef Crotal1 = MScript::GetPartByName("#DD3#Crotal");
	target = MScript::GetLastTriggerObject(part);
	count = 0;

	if (MScript::IsDead(Crotal1))
	{	state = Finish;	}
	else if( target == Crotal1 )
	{
		MScript::EnableTrigger(part, false);
		state = Begin;
	}
	else
	{
		state = Finish;
	}
}

bool Demo_Crotal1Attack::Update (void)
{

	switch(state)
	{
		case Begin:
		{
			MPartRef Crotal1 = MScript::GetPartByName("#DD3#Crotal");

			MPartRef Target = MScript::GetPartByName("#31F0#");

			MScript::OrderAttackPosition(Crotal1, Target, true);

			MScript::MakeInvincible(Crotal1, false);

			state = Finish;

		}
	}
	return false;

}



CQSCRIPTPROGRAM(Demo_Crotal2Attack, Demo_Crotal2Attack_Save,0);

void Demo_Crotal2Attack::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MPartRef Crotal2 = MScript::GetPartByName("#DE3#Crotal");
	target = MScript::GetLastTriggerObject(part);
	count = 0;

	if (MScript::IsDead(Crotal2))
	{	state = Finish;	}
	else if( target == Crotal2 )
	{
		MScript::EnableTrigger(part, false);
		state = Begin;
	}
	else
	{
		state = Finish;
	}
}

bool Demo_Crotal2Attack::Update (void)
{
	switch(state)
	{
		case Begin:
		{
			MPartRef Crotal2 = MScript::GetPartByName("#DE3#Crotal");

			MPartRef Target = MScript::GetPartByName("#3200#");

			MScript::OrderAttackPosition(Crotal2, Target, true);

			MScript::MakeInvincible(Crotal2, false);

			state = Finish;

		}
	}
	return false;

}

CQSCRIPTPROGRAM(Demo_Crotal3Attack, Demo_Crotal3Attack_Save,0);

void Demo_Crotal3Attack::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MPartRef Crotal3 = MScript::GetPartByName("#E03#Crotal");
	target = MScript::GetLastTriggerObject(part);
	count = 0;

	if (MScript::IsDead(Crotal3))
	{	state = Finish;	}
	else if( target == Crotal3 )
	{
		MScript::EnableTrigger(part, false);
		state = Begin;
	}
	else
	{
		state = Finish;
	}
}

bool Demo_Crotal3Attack::Update (void)
{

	switch(state)
	{
		case Begin:
		{
			MPartRef Crotal3 = MScript::GetPartByName("#E03#Crotal");

			MPartRef Target = MScript::GetPartByName("#3210#");

			MScript::OrderAttackPosition(Crotal3, Target, true);
		
			MScript::MakeInvincible(Crotal3, false);
			
			state = Finish;

		}
	}
	return false;

}

CQSCRIPTPROGRAM(Demo_Crotal4Attack, Demo_Crotal4Attack_Save,0);

void Demo_Crotal4Attack::Initialize (U32 eventFlags, const MPartRef & part)
{
	MPartRef Crotal4 = MScript::GetPartByName("#E13#Crotal");
	target = MScript::GetLastTriggerObject(part);
	count = 0;

	if (MScript::IsDead(Crotal4))
	{	state = Finish;	}
	else if( target == Crotal4 )
	{
		MScript::EnableTrigger(part, false);
		state = Begin;
	}
	else
	{
		state = Finish;
	}
}

bool Demo_Crotal4Attack::Update (void)
{
	switch(state)
	{
		case Begin:
		{
			MPartRef Crotal4 = MScript::GetPartByName("#E13#Crotal");

			MPartRef Target = MScript::GetPartByName("#3220#");

			MScript::OrderAttackPosition(Crotal4, Target, true);
			
			MScript::MakeInvincible(Crotal4, false);

			state = Finish;

		}
	}
	return false;

}

CQSCRIPTPROGRAM(Demo_Adder1Attack, Demo_Adder1Attack_Save,0);

void Demo_Adder1Attack::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MPartRef Adder1 = MScript::GetPartByName("#BAA3#Adder");
	MPartRef Adder2 = MScript::GetPartByName("#BAB3#Adder");

	target = MScript::GetLastTriggerObject(part);
	count = 0;

	if(( target == Adder1 ) || ( target == Adder2 ))
	{
		MScript::EnableTrigger(part, false);
		state = Begin;
	}
	else
	{
		state = Finish;
	}
}

bool Demo_Adder1Attack::Update (void)
{

	switch(state)
	{
		case Begin:
		{
			MPartRef Adder1 = MScript::GetPartByName("#BAA3#Adder");
			MPartRef Adder2 = MScript::GetPartByName("#BAB3#Adder");

			MScript::SetStance(Adder1, US_ATTACK);
			MScript::SetStance(Adder2, US_ATTACK);

			MScript::MakeInvincible(Adder1, false);
			MScript::MakeInvincible(Adder2, false);

			state = Finish;

		}
	}
	return false;

}

CQSCRIPTPROGRAM(Demo_Adder2Attack, Demo_Adder2Attack_Save,0);

void Demo_Adder2Attack::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MPartRef Adder1 = MScript::GetPartByName("#BAC3#Adder");
	MPartRef Adder2 = MScript::GetPartByName("#BAD3#Adder");

	target = MScript::GetLastTriggerObject(part);
	count = 0;

	if(( target == Adder1 ) || ( target == Adder2 ))
	{
		MScript::EnableTrigger(part, false);
		state = Begin;
	}
	else
	{
		state = Finish;
	}
}

bool Demo_Adder2Attack::Update (void)
{

	switch(state)
	{
		case Begin:
		{
			MPartRef Adder1 = MScript::GetPartByName("#BAC3#Adder");
			MPartRef Adder2 = MScript::GetPartByName("#BAD3#Adder");

			MScript::SetStance(Adder1, US_ATTACK);
			MScript::SetStance(Adder2, US_ATTACK);

			MScript::MakeInvincible(Adder1, false);
			MScript::MakeInvincible(Adder2, false);

			state = Finish;

		}
	}
	return false;

}

CQSCRIPTPROGRAM(Demo_Adder3Attack, Demo_Adder3Attack_Save,0);

void Demo_Adder3Attack::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MPartRef Adder1 = MScript::GetPartByName("#BAE3#Adder");
	MPartRef Adder2 = MScript::GetPartByName("#BAF3#Adder");

	target = MScript::GetLastTriggerObject(part);
	count = 0;

	if(( target == Adder1 ) || ( target == Adder2 ))
	{
		MScript::EnableTrigger(part, false);
		state = Begin;
	}
	else
	{
		state = Finish;
	}
}

bool Demo_Adder3Attack::Update (void)
{

	switch(state)
	{
		case Begin:
		{
			MPartRef Adder1 = MScript::GetPartByName("#BAE3#Adder");
			MPartRef Adder2 = MScript::GetPartByName("#BAF3#Adder");

			MScript::SetStance(Adder1, US_ATTACK);
			MScript::SetStance(Adder2, US_ATTACK);

			MScript::MakeInvincible(Adder1, false);
			MScript::MakeInvincible(Adder2, false);

			state = Finish;

		}
	}
	return false;

}

CQSCRIPTPROGRAM(Demo_Adder4Attack, Demo_Adder4Attack_Save,0);

void Demo_Adder4Attack::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MPartRef Adder1 = MScript::GetPartByName("#BB03#Adder");
	MPartRef Adder2 = MScript::GetPartByName("#BB13#Adder");

	target = MScript::GetLastTriggerObject(part);
	count = 0;

	if(( target == Adder1 ) || ( target == Adder2 ))
	{
		MScript::EnableTrigger(part, false);
		state = Begin;
	}
	else
	{
		state = Finish;
	}
}

bool Demo_Adder4Attack::Update (void)
{

	switch(state)
	{
		case Begin:
		{
			MPartRef Adder1 = MScript::GetPartByName("#BB03#Adder");
			MPartRef Adder2 = MScript::GetPartByName("#BB13#Adder");

			MScript::SetStance(Adder1, US_ATTACK);
			MScript::SetStance(Adder2, US_ATTACK);

			MScript::MakeInvincible(Adder1, false);
			MScript::MakeInvincible(Adder2, false);

			state = Finish;

		}
	}
	return false;

}

CQSCRIPTPROGRAM(Demo_PlanetInitiate, Demo_PlanetInitiate_Save,0);

void Demo_PlanetInitiate::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MScript::EnableTrigger(part, false);
	state = Begin;	
}

bool Demo_PlanetInitiate::Update (void)
{

	switch(state)
	{
		case Begin:
		{
			MPartRef Leviathan1 = MScript::GetPartByName("#FBB3#Leviathan");
			MPartRef Leviathan2 = MScript::GetPartByName("#FB93#Leviathan");
			MPartRef Leviathan3 = MScript::GetPartByName("#FBC3#Leviathan");
			MPartRef Leviathan4 = MScript::GetPartByName("#FBA3#Leviathan");
			MPartRef Cobra = MScript::GetPartByName("#FBD3#Cobra");

			MPartRef Leviathan1WP = MScript::GetPartByName("#FC20#");
			MPartRef Leviathan2WP = MScript::GetPartByName("#FBF0#");
			MPartRef Leviathan3WP = MScript::GetPartByName("#FC00#");
			MPartRef Leviathan4WP = MScript::GetPartByName("#FC10#");
			MPartRef CobraWP = MScript::GetPartByName("#FBE0#");

			MPartRef CobraTarget = MScript::GetPartByName("#60#Earth Planet");

			MScript::OrderMoveTo(Leviathan1, Leviathan1WP, false);
			MScript::OrderMoveTo(Leviathan2, Leviathan2WP, false);
			MScript::OrderMoveTo(Leviathan3, Leviathan3WP, false);
			MScript::OrderMoveTo(Leviathan4, Leviathan4WP, false);
			MScript::OrderSpecialAttack(Cobra, CobraTarget);

			MScript::MakeInvincible(Cobra, false);

			MScript::SetStance(Leviathan1, US_ATTACK);
			MScript::SetStance(Leviathan2, US_ATTACK);
			MScript::SetStance(Leviathan3, US_ATTACK);
			MScript::SetStance(Leviathan4, US_ATTACK);

			state = Finish;
		}
	}
	return false;

}

CQSCRIPTPROGRAM(Demo_PlanetAttack, Demo_PlanetAttack_Save,0);

void Demo_PlanetAttack::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MPartRef Cobra = MScript::GetPartByName("#FBD3#Cobra");

	target = MScript::GetLastTriggerObject(part);
	if( target == Cobra	)
	{
		MScript::EnableTrigger(part, false);
		state = Begin;
	}
	else
	{
		state = Finish;
	}
}

bool Demo_PlanetAttack::Update (void)
{

	switch(state)
	{
		case Begin:
		{

			MPartRef Cobra = MScript::GetPartByName("#FBD3#Cobra");

			MPartRef CobraTarget = MScript::GetPartByName("#60#Earth Planet");

			MScript::OrderSpecialAttack(Cobra, CobraTarget);

			MScript::MakeInvincible(Cobra, false);

			state = Finish;

		}
	}
	return false;

}

CQSCRIPTPROGRAM(Demo_PlanetTerraform, Demo_PlanetTerraform_Save,CQPROGFLAG_HOTKEY_1);

void Demo_PlanetTerraform::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MPartRef planet = MScript::GetPartByName("#60#Earth Planet");

	MScript::TeraformPlanet(planet, "Planet Earth!!Earth 1", 10);
}

bool Demo_PlanetTerraform::Update (void)
{	return false;	}


CQSCRIPTPROGRAM(Demo_PlanetDestroy, Demo_PlanetDestroy_Save,CQPROGFLAG_HOTKEY_2);

void Demo_PlanetDestroy::Initialize (U32 eventFlags, const MPartRef & part)
{	
	MPartRef planet = MScript::GetPartByName("#60#Earth Planet");

	MScript::TeraformPlanet(planet, "Planet Dead!!Dead 1", 10);
}

bool Demo_PlanetDestroy::Update (void)
{	return false;	}

CQSCRIPTPROGRAM(Demo_VyriumWave1, Demo_VyriumWave1_Save,CQPROGFLAG_HOTKEY_3);

void Demo_VyriumWave1::Initialize (U32 eventFlags, const MPartRef & part)
{	
		hotkey = false;

		if(hotkey3)
		{
			MPartRef planet = MScript::GetPartByName("#60#Earth Planet");

			MPartRef Crotal1 = MScript::GetPartByName("#DD3#Crotal");
			MPartRef Crotal2 = MScript::GetPartByName("#DE3#Crotal");
			MPartRef Crotal3 = MScript::GetPartByName("#E03#Crotal");
			MPartRef Crotal4 = MScript::GetPartByName("#E13#Crotal");

			MPartRef Adder1 = MScript::GetPartByName("#BAA3#Adder");
			MPartRef Adder2 = MScript::GetPartByName("#BAB3#Adder");
			MPartRef Adder3 = MScript::GetPartByName("#BAC3#Adder");
			MPartRef Adder4 = MScript::GetPartByName("#BAD3#Adder");
			MPartRef Adder5 = MScript::GetPartByName("#BAE3#Adder");
			MPartRef Adder6 = MScript::GetPartByName("#BAF3#Adder");
			MPartRef Adder7 = MScript::GetPartByName("#BB03#Adder");
			MPartRef Adder8 = MScript::GetPartByName("#BB13#Adder");

			MPartRef Adder11 = MScript::GetPartByName("#E5C3#Adder");
			MPartRef Adder12 = MScript::GetPartByName("#E5D3#Adder");
			MPartRef Adder13 = MScript::GetPartByName("#E623#Adder");
			MPartRef Adder21 = MScript::GetPartByName("#E5E3#Adder");
			MPartRef Adder22 = MScript::GetPartByName("#E633#Adder");
			MPartRef Adder23 = MScript::GetPartByName("#E5F3#Adder");
			MPartRef Adder31 = MScript::GetPartByName("#E603#Adder");
			MPartRef Adder32 = MScript::GetPartByName("#E613#Adder");
			MPartRef Adder33 = MScript::GetPartByName("#E643#Adder");

			MPartRef Leviathan1 = MScript::GetPartByName("#F03#Leviathan");

			MPartRef Crotal1Destination = MScript::GetPartByName("#32E0#");
			MPartRef Crotal2Destination = MScript::GetPartByName("#32D0#");
			MPartRef Crotal3Destination = MScript::GetPartByName("#32F0#");
			MPartRef Crotal4Destination = MScript::GetPartByName("#3300#");
			MPartRef Adder1Destination = MScript::GetPartByName("#C300#");
			MPartRef Adder2Destination = MScript::GetPartByName("#C300#");
			MPartRef Adder3Destination = MScript::GetPartByName("#C310#");
			MPartRef Adder4Destination = MScript::GetPartByName("#C310#");				
			MPartRef Adder5Destination = MScript::GetPartByName("#C320#");
			MPartRef Adder6Destination = MScript::GetPartByName("#C320#");
			MPartRef Adder7Destination = MScript::GetPartByName("#C330#");
			MPartRef Adder8Destination = MScript::GetPartByName("#C330#");
			MPartRef Adder11Destination = MScript::GetPartByName("#E6E0#");
			MPartRef Adder12Destination = MScript::GetPartByName("#E6E0#");
			MPartRef Adder13Destination = MScript::GetPartByName("#E6E0#");
			MPartRef Adder21Destination = MScript::GetPartByName("#E6F0#");
			MPartRef Adder22Destination = MScript::GetPartByName("#E6F0#");				
			MPartRef Adder23Destination = MScript::GetPartByName("#E6F0#");
			MPartRef Adder31Destination = MScript::GetPartByName("#E6D0#");
			MPartRef Adder32Destination = MScript::GetPartByName("#E6D0#");
			MPartRef Adder33Destination = MScript::GetPartByName("#E6D0#");

			MPartRef Leviathan1Destination = MScript::GetPartByName("#32C0#");

			MPartRef LeviathanTrigger = MScript::GetPartByName("#32C0#");
			MPartRef Crotal1Trigger = MScript::GetPartByName("#32E0#");
			MPartRef Crotal2Trigger = MScript::GetPartByName("#32D0#");
			MPartRef Crotal3Trigger = MScript::GetPartByName("#32F0#");
			MPartRef Crotal4Trigger = MScript::GetPartByName("#3300#");
			MPartRef Adder1Trigger = MScript::GetPartByName("#C300#");
			MPartRef Adder2Trigger = MScript::GetPartByName("#C310#");
			MPartRef Adder3Trigger = MScript::GetPartByName("#C320#");
			MPartRef Adder4Trigger = MScript::GetPartByName("#C330#");

			MScript::MakeInvincible(Crotal1, true);
			MScript::MakeInvincible(Crotal2, true);
			MScript::MakeInvincible(Crotal3, true);
			MScript::MakeInvincible(Crotal4, true);
			MScript::MakeInvincible(Adder1, true);
			MScript::MakeInvincible(Adder2, true);
			MScript::MakeInvincible(Adder3, true);
			MScript::MakeInvincible(Adder4, true);
			MScript::MakeInvincible(Adder5, true);
			MScript::MakeInvincible(Adder6, true);
			MScript::MakeInvincible(Adder7, true);
			MScript::MakeInvincible(Adder8, true);

			MScript::SetStance(Adder11, US_ATTACK);
			MScript::SetStance(Adder12, US_ATTACK);
			MScript::SetStance(Adder13, US_ATTACK);
			MScript::SetStance(Adder21, US_ATTACK);
			MScript::SetStance(Adder22, US_ATTACK);
			MScript::SetStance(Adder23, US_ATTACK);
			MScript::SetStance(Adder31, US_ATTACK);
			MScript::SetStance(Adder32, US_ATTACK);
			MScript::SetStance(Adder33, US_ATTACK);
			
			MScript::MakeInvincible(Leviathan1, true);

			MScript::OrderMoveTo(Crotal1, Crotal1Destination, false);
			MScript::OrderMoveTo(Crotal2, Crotal2Destination, false);
			MScript::OrderMoveTo(Crotal3, Crotal3Destination, false);
			MScript::OrderMoveTo(Crotal4, Crotal4Destination, false);

			MScript::OrderMoveTo(Adder1, Adder1Destination, false);
			MScript::OrderMoveTo(Adder2, Adder2Destination, false);
			MScript::OrderMoveTo(Adder3, Adder3Destination, false);
			MScript::OrderMoveTo(Adder4, Adder4Destination, false);
			MScript::OrderMoveTo(Adder5, Adder5Destination, false);
			MScript::OrderMoveTo(Adder6, Adder6Destination, false);
			MScript::OrderMoveTo(Adder7, Adder7Destination, false);
			MScript::OrderMoveTo(Adder8, Adder8Destination, false);
			MScript::OrderMoveTo(Adder11, Adder11Destination, false);
			MScript::OrderMoveTo(Adder12, Adder12Destination, false);
			MScript::OrderMoveTo(Adder13, Adder13Destination, false);
			MScript::OrderMoveTo(Adder21, Adder21Destination, false);
			MScript::OrderMoveTo(Adder22, Adder22Destination, false);
			MScript::OrderMoveTo(Adder23, Adder23Destination, false);
			MScript::OrderMoveTo(Adder31, Adder31Destination, false);
			MScript::OrderMoveTo(Adder32, Adder32Destination, false);
			MScript::OrderMoveTo(Adder33, Adder33Destination, false);

			MScript::OrderMoveTo(Leviathan1, Leviathan1Destination, false);

			MPartRef Leviathan5 = MScript::GetPartByName("#FBB3#Leviathan");
			MPartRef Leviathan5WP = MScript::GetPartByName("#FC20#");
			MScript::OrderMoveTo(Leviathan5, Leviathan5WP, false);
			MScript::SetStance(Leviathan5, US_ATTACK);

			MPartRef Basilisk1 = MScript::GetPartByName("#D33#Basilisk");
			MPartRef Basilisk2 = MScript::GetPartByName("#D53#Basilisk");
			MPartRef Basilisk3 = MScript::GetPartByName("#D63#Basilisk");
			MPartRef Basilisk4 = MScript::GetPartByName("#DA3#Basilisk");
			MPartRef Basilisk5 = MScript::GetPartByName("#E653#Basilisk");
			MPartRef Basilisk6 = MScript::GetPartByName("#E583#Basilisk");
			MPartRef Basilisk7 = MScript::GetPartByName("#E593#Basilisk");
			MPartRef Basilisk8 = MScript::GetPartByName("#E5A3#Basilisk");
			MPartRef Basilisk9 = MScript::GetPartByName("#E5B3#Basilisk");
			MPartRef Basilisk0 = MScript::GetPartByName("#E663#Basilisk");

			MPartRef Target1 = MScript::GetPartByName("#3190#");
			MPartRef Target2 = MScript::GetPartByName("#31A0#");
			MPartRef Target3 = MScript::GetPartByName("#31D0#");
			MPartRef Target4 = MScript::GetPartByName("#31C0#");
			MPartRef Target5 = MScript::GetPartByName("#E6A0#");
			MPartRef Target6 = MScript::GetPartByName("#E690#");
			MPartRef Target7 = MScript::GetPartByName("#E680#");
			MPartRef Target8 = MScript::GetPartByName("#E670#");
			MPartRef Target9 = MScript::GetPartByName("#E6B0#");
			MPartRef Target0 = MScript::GetPartByName("#E6C0#");

			MScript::OrderSpecialAttack(Basilisk1, Target1);
			MScript::OrderSpecialAttack(Basilisk2, Target2);
			MScript::OrderSpecialAttack(Basilisk3, Target3);
			MScript::OrderSpecialAttack(Basilisk4, Target4);
			MScript::OrderSpecialAttack(Basilisk5, Target5);
			MScript::OrderSpecialAttack(Basilisk6, Target6);
			MScript::OrderSpecialAttack(Basilisk7, Target7);
			MScript::OrderSpecialAttack(Basilisk8, Target8);
			MScript::OrderSpecialAttack(Basilisk9, Target9);
			MScript::OrderSpecialAttack(Basilisk0, Target0);

			MScript::SetStance(Basilisk1, US_ATTACK);
			MScript::SetStance(Basilisk2, US_ATTACK);
			MScript::SetStance(Basilisk3, US_ATTACK);
			MScript::SetStance(Basilisk4, US_ATTACK);
			MScript::SetStance(Basilisk5, US_ATTACK);
			MScript::SetStance(Basilisk6, US_ATTACK);
			MScript::SetStance(Basilisk7, US_ATTACK);
			MScript::SetStance(Basilisk8, US_ATTACK);
			MScript::SetStance(Basilisk9, US_ATTACK);
			MScript::SetStance(Basilisk0, US_ATTACK);

			MScript::SetTriggerProgram(LeviathanTrigger, "Demo_LeviathanAttack");
			MScript::SetTriggerProgram(Crotal1Trigger, "Demo_Crotal1Attack");
			MScript::SetTriggerProgram(Crotal2Trigger, "Demo_Crotal2Attack");
			MScript::SetTriggerProgram(Crotal3Trigger, "Demo_Crotal3Attack");
			MScript::SetTriggerProgram(Crotal4Trigger, "Demo_Crotal4Attack");
			MScript::SetTriggerProgram(Adder1Trigger, "Demo_Adder1Attack");
			MScript::SetTriggerProgram(Adder2Trigger, "Demo_Adder2Attack");
			MScript::SetTriggerProgram(Adder3Trigger, "Demo_Adder3Attack");
			MScript::SetTriggerProgram(Adder4Trigger, "Demo_Adder4Attack");
		
			hotkey3 = false;
		}

}

bool Demo_VyriumWave1::Update (void)
{	return false;	}

CQSCRIPTPROGRAM(Demo_VyriumWave2, Demo_VyriumWave2_Save,CQPROGFLAG_HOTKEY_4);

void Demo_VyriumWave2::Initialize (U32 eventFlags, const MPartRef & part)
{	
	hotkey = false;

	if(hotkey4)
	{
		MPartRef Cobra = MScript::GetPartByName("#FBD3#Cobra");

		MPartRef CobraWP = MScript::GetPartByName("#FBE0#");

		MPartRef CobraTarget = MScript::GetPartByName("#60#Earth Planet");

		MScript::OrderSpecialAttack(Cobra, CobraTarget);

		MScript::MakeInvincible(Cobra, true);

		state = Finish;
		
		hotkey4 = false;
	}
}

bool Demo_VyriumWave2::Update (void)
{	return false;	}

CQSCRIPTPROGRAM(Demo_EndGame, Demo_EndGame_Save,CQPROGFLAG_HOTKEY_5);

void Demo_EndGame::Initialize (U32 eventFlags, const MPartRef & part)
{	
	hotkey = false;
	MScript::EndMissionSplash("VFXShape!!Splash",10000,true);
//	MScript::EndMissionVictory(0);
}

bool Demo_EndGame::Update (void)
{	return false;	}


//--------------------------------------------------------------------------//
//--Test Scripts-------------------------------------------------------//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//--End ScriptTest.cpp-------------------------------------------------------//

