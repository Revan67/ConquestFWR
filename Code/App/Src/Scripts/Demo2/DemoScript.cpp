//--------------------------------------------------------------------------//
//                                                                          //
//                                DemoScript.cpp                            //
//                                                                          //
//               COPYRIGHT (C) 2004 Warthog Texas, INC.                     //
//                                                                          //
//--------------------------------------------------------------------------//
/*
MEXTERN static void AlertObjectInSysMap (const MPartRef & object, bool bSectorToo = true);
MEXTERN static U32 StartAlertAnim (const MPartRef & object);
*/
//--------------------------------------------------------------------------//

#include <ScriptDef.h>
#include <DLLScriptMain.h>
#include "DataDef.h"
#include "Hotkeys.h"
#define MOTextColor				RGB(63, 199, 208)

CQSCRIPTDATA(MissionData, data);

//Briefing

CQSCRIPTPROGRAM(Demo_Briefing, Demo_Briefing_Data,0);

void Demo_Briefing::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	state = Begin;
	strcpy(empty_anim.szFileName, "empty.wav");
	strcpy(empty_anim.szTypeName, "Animate!!Blackwell");
	empty_anim.slotID = 0;
	empty_anim.dwTimer = 0;
	empty_anim.bHighlite = false;
	empty_anim.bContinueAnimating = false;
	empty_anim.bLoopAnimation = true;

	strcpy(empty_anim.szTypeName, "Animate!!Fuzz");
	empty_anim.slotID = 0;
	empty_anim.dwTimer = 5;
	//MScript::PlayBriefingAnimation(empty_anim);

	strcpy(empty_anim.szTypeName, "Animate!!Fuzz");
	empty_anim.slotID = 1;
	empty_anim.dwTimer = 5;
	//MScript::PlayBriefingAnimation(empty_anim);

	strcpy(empty_anim.szTypeName, "Animate!!Fuzz");
	empty_anim.slotID = 2;
	empty_anim.dwTimer = 5;
	//MScript::PlayBriefingAnimation(empty_anim);

	strcpy(empty_anim.szTypeName, "Animate!!Fuzz");
	empty_anim.slotID = 3;
	empty_anim.dwTimer = 5;
	//MScript::PlayBriefingAnimation(empty_anim);

}

bool Demo_Briefing::Update (void)
{
	count = count + 1;
	switch(state)
	{
	case(Begin):
		{
			if(count >= 15)
			{
				//Stage0_1Var = MScript::PlayBriefingTeletype(IDS_Demo_Ad_1, MOTextColor, 21000, 19000, false);

				strcpy(empty_anim.szTypeName, "Animate!!Radiowave");
				strcpy(empty_anim.szFileName, "bcast_01.wav");
				empty_anim.slotID = 0;
				empty_anim.dwTimer = 15;
				Stage0_1Var = MScript::PlayBriefingTalkingHead(empty_anim);

				state = Stage0_1;
				count = 0;
				return true;
			}
			else
				return true;


		}
		break;
	case(Stage0_1):
		{
			if(!MScript::IsStreamPlaying(Stage0_1Var)) //!MScript::IsTeletypePlaying(Stage0_1Var))
			{
				//Stage0_2Var = MScript::PlayBriefingTeletype(IDS_Demo_Ad_2, MOTextColor, 26000, 22000, false);

				strcpy(empty_anim.szTypeName, "Animate!!Radiowave");
				strcpy(empty_anim.szFileName, "bcast_02.wav");
				empty_anim.slotID = 0;
				empty_anim.dwTimer = 15;
				Stage0_2Var = MScript::PlayBriefingTalkingHead(empty_anim);
				state = Stage0_2;
				count = 0;
				return true;
			}
			else
				return true;
		}
		break;
	case(Stage0_2):
		{
			if(!MScript::IsStreamPlaying(Stage0_2Var) && count >= 5)
			{
				MScript::PlayBriefingTeletype(IDS_Demo_Location, MOTextColor, 8000, 2000, false);
				strcpy(empty_anim.szTypeName, "Animate!!Fuzz");
				strcpy(empty_anim.szFileName, "empty.wav");
				empty_anim.slotID = 0;
				empty_anim.dwTimer = 5;
				MScript::PlayBriefingAnimation(empty_anim);
				state = Stage1;
				count = 0;
				return true;
			}
			else
				return true;
		}
		break;
	case(Stage1):
		{
			if(count >= 120)
			{
				Stage1Var = MScript::PlayBriefingTeletype(IDS_Demo_TT1, MOTextColor, 16000, 14000, false);

				strcpy(empty_anim.szTypeName, "Animate!!Blackwell");
				strcpy(empty_anim.szFileName, "comm_blackwell_01.wav");
				empty_anim.dwTimer = 0;
				empty_anim.slotID = 0;
				MScript::PlayBriefingTalkingHead(empty_anim);
				state = Stage1_5;
				count = 0;
				return true;
			}
			else
				return true;
		}
		break;
	case(Stage1_5):
		{
			if(count >= 180)
			{
				strcpy(empty_anim.szFileName, "empty.wav");
				strcpy(empty_anim.szTypeName, "Animate!!Espionage");
				empty_anim.slotID = 1;
				empty_anim.dwTimer = 15;
				MScript::PlayBriefingAnimation(empty_anim);
				state = Stage1_6;
				count = 0;
				return true;
			}
			else
				return true;
		}
		break;
	case(Stage1_6):
		{
			if(count >= 30)
			{
				strcpy(empty_anim.szFileName, "empty.wav");
				strcpy(empty_anim.szTypeName, "Animate!!Espionage2");
				empty_anim.slotID = 2;
				empty_anim.dwTimer = 15;
				MScript::PlayBriefingAnimation(empty_anim);
				state = Stage1_7;
				count = 0;
				return true;
			}
			else
				return true;
		}
		break;
	case(Stage1_7):
		{
			if(count >= 30)
			{
				strcpy(empty_anim.szFileName, "empty.wav");
				strcpy(empty_anim.szTypeName, "Animate!!Espionage3");
				empty_anim.slotID = 3;
				empty_anim.dwTimer = 15;
				MScript::PlayBriefingAnimation(empty_anim);
				state = Stage2;
				count = 0;
				return true;
			}
			else
				return true;
		}
		break;
	case(Stage2):
		{
			if(!MScript::IsTeletypePlaying(Stage1Var)) //count >= 180)
			{
				Stage2Var = MScript::PlayBriefingTeletype(IDS_Demo_TT2, MOTextColor, 16000, 14000, false);
				strcpy(empty_anim.szTypeName, "Animate!!Fuzz");
				empty_anim.slotID = 1;
				empty_anim.dwTimer = 5;
				MScript::PlayBriefingAnimation(empty_anim);
				empty_anim.slotID = 2;
				MScript::PlayBriefingAnimation(empty_anim);
				empty_anim.slotID = 3;
				MScript::PlayBriefingAnimation(empty_anim);
				state = Stage2_5;
				count = 0;
				return true;
			}
			else
				return true;
		}
		break;
	case(Stage2_5):
		{
			if(count >= 100)
			{
				strcpy(empty_anim.szFileName, "empty.wav");
				strcpy(empty_anim.szTypeName, "Animate!!Celareon");
				empty_anim.slotID = 1;
				empty_anim.dwTimer = 1;
				MScript::PlayBriefingAnimation(empty_anim);
				state = Stage3;
				count = 0;
				return true;
			}
			else
				return true;
		}
		break;
	case(Stage3):
		{
			if(!MScript::IsTeletypePlaying(Stage2Var)) //count >= 180)
			{
				Stage3Var = MScript::PlayBriefingTeletype(IDS_Demo_TT3, MOTextColor, 20000, 4000, false);
				strcpy(empty_anim.szTypeName, "Animate!!Fuzz");
				empty_anim.slotID = 0;
				empty_anim.dwTimer = 5;
				MScript::PlayBriefingAnimation(empty_anim);
				empty_anim.slotID = 1;
				empty_anim.dwTimer = 5;
				MScript::PlayBriefingAnimation(empty_anim);

				state = Stage4;
				count = 0;
				return true;
			}
			else
				return true;
		}
		break;
	case(Stage4):
		{
			if(!MScript::IsTeletypePlaying(Stage3Var)) //count >= 300)
			{
				strcpy(empty_anim.szTypeName, "Animate!!Fuzz");
				empty_anim.slotID = 0;
				empty_anim.dwTimer = 5;
				MScript::PlayBriefingAnimation(empty_anim);
				empty_anim.slotID = 1;
				empty_anim.dwTimer = 5;
				MScript::PlayBriefingAnimation(empty_anim);

				state = Finish;
				count = 0;
				return true;
			}
			else
				return true;
		}
		break;
	case(Finish):
		{
			return false;
		}
		break;
	}
	return true;
}

//Programs

CQSCRIPTPROGRAM(MyProg, MyProg_Save,CQPROGFLAG_STARTMISSION);

void MyProg::Initialize (U32 eventFlags, const MPartRef & part)
{
	data.IsWin = false;

	data.Cel_Plat_1 = MScript::GetPartByName("#5D01#Bunker");
	data.Cel_Plat_2 = MScript::GetPartByName("#5D21#Eutromil");
	data.Cel_Plat_3 = MScript::GetPartByName("#5D91#Sentinel Tower");
	data.Cel_Plat_4 = MScript::GetPartByName("#5D71#Sentinel Tower");
	data.Cel_Plat_5 = MScript::GetPartByName("#5D41#Eutromil");
	data.Cel_Plat_6 = MScript::GetPartByName("#5E51#Sentinel Tower");
	data.Cel_Plat_7 = MScript::GetPartByName("#5D31#Eutromil");

	MScript::MakeInvincible(data.Cel_Plat_1, true);
	MScript::MakeInvincible(data.Cel_Plat_2, true);
	MScript::MakeInvincible(data.Cel_Plat_3, true);
	MScript::MakeInvincible(data.Cel_Plat_4, true);
	MScript::MakeInvincible(data.Cel_Plat_5, true);
	MScript::MakeInvincible(data.Cel_Plat_6, true);
	MScript::MakeInvincible(data.Cel_Plat_7, true);

	data.Nova_Bomb = MScript::GetPartByName("#3B261#Space Station");
	data.HQ = MScript::GetPartByName("#591#Headquarters");
	data.Temp_HQ= MScript::GetPartByName("#33671#Space Station");

	data.mission_state = MissionStart;
	state = Begin;

	//MScript::SetSpectatorMode(true);

	MScript::MakeAreaVisible (1, MScript::GetPartByName("#E980#"), 3);
	MScript::MakeAreaVisible (1, MScript::GetPartByName("#115A0#"), 3);
	MScript::MakeAreaVisible (1, MScript::GetPartByName("#2B9B0#"), 3);
	MScript::MakeAreaVisible (1, MScript::GetPartByName("#A9B0#"), 3);

	MScript::MakeAreaVisible (1, MScript::GetPartByName("Diablo"), 4);
	MScript::MakeAreaVisible (1, MScript::GetPartByName("Phobos"), 4);
	MScript::MakeAreaVisible (1, MScript::GetPartByName("Mephisto"), 4);
	MScript::MakeAreaVisible (1, MScript::GetPartByName("Shiar"), 4);
	MScript::MakeAreaVisible (1, MScript::GetPartByName("Baal"), 4);

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
//		TECHTREE::M_RES_XGRAVWELL |
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
//		TECHTREE::RES_TANKER2  |
		TECHTREE::RES_TENDER1 |
		TECHTREE::RES_TENDER2 |
		TECHTREE::RES_FIGHTER1 //|
//		TECHTREE::RES_FIGHTER2
        );

	data.missionTech.race[1].common = (TECHTREE::COMMON) (
//		TECHTREE::RES_WEAPONS1  |
//		TECHTREE::RES_WEAPONS2  |
		TECHTREE::RES_SUPPLY1  |
//		TECHTREE::RES_SUPPLY2  |
//		TECHTREE::RES_HULL1  |
		TECHTREE::RES_ENGINE1 //|
//		TECHTREE::RES_SHIELDS1
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
	//	TECHTREE::RES_WEAPONS2  |
	//	TECHTREE::RES_WEAPONS3  |
	//	TECHTREE::RES_WEAPONS4  |
	//	TECHTREE::RES_WEAPONS5  |
		TECHTREE::RES_SUPPLY1  |
		TECHTREE::RES_SUPPLY2  |
	//	TECHTREE::RES_SUPPLY3  |
	//	TECHTREE::RES_SUPPLY4  |
	//	TECHTREE::RES_SUPPLY5  |
		TECHTREE::RES_HULL1  |
		TECHTREE::RES_HULL2  |
	//	TECHTREE::RES_HULL3  |
	//	TECHTREE::RES_HULL4  |
	//	TECHTREE::RES_HULL5  |
		TECHTREE::RES_ENGINE1 |
		TECHTREE::RES_ENGINE2 |
		TECHTREE::RES_ENGINE3 |
		TECHTREE::RES_ENGINE4 |
		TECHTREE::RES_ENGINE5 //|
	//	TECHTREE::RES_SHIELDS1 |
	//	TECHTREE::RES_SHIELDS2 |
	//	TECHTREE::RES_SHIELDS3 |
	//	TECHTREE::RES_SHIELDS4 |
	//	TECHTREE::RES_SHIELDS5
        );

    MScript::SetPlayerTech(1, data.missionTech );
    MScript::SetPlayerTech(2, data.missionTech );
    MScript::SetPlayerTech(3, data.missionTech );	
	
	MPartRef Admiral = MScript::GetPartByName("Admiral Steele");
	MScript::GiveCommandKit(Admiral, "CK!!T_Halsey");
	MScript::GiveCommandKit(Admiral, "CK!!T_Tactical");
	MScript::MoveCamera(MScript::GetPartByName("Minbari"), 0.0, MOVIE_CAMERA_JUMP_TO);
	MScript::EnableMovieMode(true);

	MScript::MakeInvincible(MScript::GetPartByName("#266B2#Scout Carrier"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#266A2#Scout Carrier"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#26692#Scout Carrier"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#26642#Scout Carrier"), true);

	MScript::MakeInvincible(MScript::GetPartByName("#26632#Frigate"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#26622#Frigate"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#26652#Frigate"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#26662#Frigate"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#26672#Frigate"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#26682#Frigate"), true);

	MScript::MakeInvincible(MScript::GetPartByName("#26722#Hive Carrier"), true);

	MScript::MakeInvincible(MScript::GetPartByName("#2CF21#ESP Coil"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#2CF11#ESP Coil"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#2CF01#ESP Coil"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#2CF41#ESP Coil"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#2CF31#ESP Coil"), true);

	MScript::MakeInvincible(MScript::GetPartByName("#26701#Hydrofoil"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#266E1#Hydrofoil"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#266F1#Hydrofoil"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#26711#Hydrofoil"), true);
	MScript::MakeInvincible(MScript::GetPartByName("#27A61#Hydrofoil"), true);

	MScript::EnableAIForPart(MScript::GetPartByName("#266B2#Scout Carrier"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#266A2#Scout Carrier"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#26692#Scout Carrier"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#26642#Scout Carrier"), true);

	MScript::EnableAIForPart(MScript::GetPartByName("#26632#Frigate"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#26622#Frigate"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#26652#Frigate"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#26662#Frigate"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#26672#Frigate"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#26682#Frigate"), true);

	MScript::EnableAIForPart(MScript::GetPartByName("#26722#Hive Carrier"), true);

	MScript::EnableAIForPart(MScript::GetPartByName("#2CF21#ESP Coil"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#2CF11#ESP Coil"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#2CF01#ESP Coil"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#2CF41#ESP Coil"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#2CF31#ESP Coil"), true);

	MScript::EnableAIForPart(MScript::GetPartByName("#26701#Hydrofoil"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#266E1#Hydrofoil"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#266F1#Hydrofoil"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#26711#Hydrofoil"), true);
	MScript::EnableAIForPart(MScript::GetPartByName("#27A61#Hydrofoil"), true);
	
	count = 0;

	MScript::SetGameSpeed(0);

}

bool MyProg::Update (void)
{

	count = count + 1;

	switch (state)
	{
		case Begin:
			{
				MPartRef LightShipyard1 = MScript::GetPartByName("#641#Light Shipyard");
				MPartRef LightShipyard2 = MScript::GetPartByName("#651#Light Shipyard");
				MPartRef HeavyShipyard1 = MScript::GetPartByName("#611#Heavy Shipyard");
				MPartRef HeavyShipyard2 = MScript::GetPartByName("#621#Heavy Shipyard");

				MPartRef Niad1 = MScript::GetPartByName("#A62#Niad");
				MPartRef Niad2 = MScript::GetPartByName("#A72#Niad");
				MPartRef Niad3 = MScript::GetPartByName("#A82#Niad");
				MPartRef Niad4 = MScript::GetPartByName("#A92#Niad");
				MPartRef Niad5 = MScript::GetPartByName("#AA2#Niad");
                

				for(int i1 = 0; i1 < 4; i1++)
				{
					MScript::OrderBuildUnit(LightShipyard1, "GBOAT!!T_Corvette", true);
                    MScript::OrderBuildUnit(LightShipyard1, "GBOAT!!T_Missile Cruiser", true);
				}

				for(int i2 = 0; i2 < 4; i2++)
				{
					MScript::OrderBuildUnit(LightShipyard2, "GBOAT!!T_Corvette", true);
					MScript::OrderBuildUnit(LightShipyard2, "GBOAT!!T_Missile Cruiser", true);
				}

				for(int i3 = 0; i3 < 2; i3++)
				{
					MScript::OrderBuildUnit(HeavyShipyard1, "GBOAT!!T_Lancer Cruiser", true);
					MScript::OrderBuildUnit(HeavyShipyard1, "GBOAT!!T_Lancer Cruiser", true);
                	MScript::OrderBuildUnit(HeavyShipyard1, "GBOAT!!T_Midas Battleship", true);
                	MScript::OrderBuildUnit(HeavyShipyard1, "GBOAT!!T_Dreadnought", true);
				}

				for(int i4 = 0; i4 < 2; i4++)
				{
					MScript::OrderBuildUnit(HeavyShipyard2, "GBOAT!!T_Lancer Cruiser", true);
					MScript::OrderBuildUnit(HeavyShipyard2, "GBOAT!!T_Lancer Cruiser", true);
                	MScript::OrderBuildUnit(HeavyShipyard2, "GBOAT!!T_Midas Battleship", true);
                	MScript::OrderBuildUnit(HeavyShipyard2, "GBOAT!!T_Dreadnought", true);
				}

				for(int i5 = 0; i5 < 3; i5++)
				{
					MScript::OrderBuildUnit(Niad1, "GBOAT!!M_Hive Carrier", true);
					MScript::OrderBuildUnit(Niad1, "GBOAT!!M_Frigate", true);
					MScript::OrderBuildUnit(Niad1, "GBOAT!!M_Scout Carrier", true);

					MScript::OrderBuildUnit(Niad2, "GBOAT!!M_Scout Carrier", true);
					MScript::OrderBuildUnit(Niad2, "GBOAT!!M_Hive Carrier", true);
					MScript::OrderBuildUnit(Niad2, "GBOAT!!M_Frigate", true);

					MScript::OrderBuildUnit(Niad3, "GBOAT!!M_Frigate", true);
					MScript::OrderBuildUnit(Niad3, "GBOAT!!M_Scout Carrier", true);
					MScript::OrderBuildUnit(Niad3, "GBOAT!!M_Hive Carrier", true);
					
					MScript::OrderBuildUnit(Niad4, "GBOAT!!M_Frigate", true);
					MScript::OrderBuildUnit(Niad4, "GBOAT!!M_Scout Carrier", true);
					MScript::OrderBuildUnit(Niad4, "GBOAT!!M_Hive Carrier", true);

					MScript::OrderBuildUnit(Niad5, "GBOAT!!M_Scout Carrier", true);
					MScript::OrderBuildUnit(Niad5, "GBOAT!!M_Hive Carrier", true);
					MScript::OrderBuildUnit(Niad5, "GBOAT!!M_Frigate", true);
				}

				MScript::SetRallyPoint(LightShipyard1, MScript::GetPartByName("#18860#"));
				MScript::SetRallyPoint(LightShipyard2, MScript::GetPartByName("#18870#"));
				MScript::SetRallyPoint(HeavyShipyard1, MScript::GetPartByName("#18840#"));
				MScript::SetRallyPoint(HeavyShipyard2, MScript::GetPartByName("#18850#"));

				MScript::SetRallyPoint(Niad1, MScript::GetPartByName("#BEA0#"));
				MScript::SetRallyPoint(Niad2, MScript::GetPartByName("#BEB0#"));
				MScript::SetRallyPoint(Niad3, MScript::GetPartByName("#BEC0#"));
				MScript::SetRallyPoint(Niad4, MScript::GetPartByName("#BED0#"));
				MScript::SetRallyPoint(Niad5, MScript::GetPartByName("#BEE0#"));

//				MScript::MakeJumpgateInvisible(MScript::GetPartByName("Gate3"), true);

				MScript::OrderBuildPlatform(MScript::GetPartByName("#1111#Fabricator "), MScript::GetPartByName("#8290#"), "PLATGUN!!T_IonCannon");
				MScript::OrderBuildPlatform(MScript::GetPartByName("#1101#Fabricator "), MScript::GetPartByName("#82A0#"), "PLATGUN!!T_IonCannon");

				MScript::OrderHarvest(MScript::GetPartByName("#1151#Harvester"), MScript::GetPartByName("#7040#"));
				MScript::OrderHarvest(MScript::GetPartByName("#1141#Harvester"), MScript::GetPartByName("#7040#"));
				MScript::OrderHarvest(MScript::GetPartByName("#1131#Harvester"), MScript::GetPartByName("#7070#"));
				MScript::OrderHarvest(MScript::GetPartByName("#1121#Harvester"), MScript::GetPartByName("#7070#"));
				MScript::OrderHarvest(MScript::GetPartByName("#11A1#Harvester"), MScript::GetPartByName("#7050#"));
				MScript::OrderHarvest(MScript::GetPartByName("#11B1#Harvester"), MScript::GetPartByName("#7050#"));
				MScript::OrderHarvest(MScript::GetPartByName("#1181#Harvester"), MScript::GetPartByName("#7060#"));
				MScript::OrderHarvest(MScript::GetPartByName("#1191#Harvester"), MScript::GetPartByName("#7060#"));
				MScript::OrderHarvest(MScript::GetPartByName("#1161#Harvester"), MScript::GetPartByName("#7030#"));
				MScript::OrderHarvest(MScript::GetPartByName("#1171#Harvester"), MScript::GetPartByName("#7030#"));
				
				MScript::OrderHarvest(MScript::GetPartByName("#27AC2#Siphon"), MScript::GetPartByName("#27A90#"));
				MScript::OrderHarvest(MScript::GetPartByName("#27AD2#Siphon"), MScript::GetPartByName("#27A90#"));
				MScript::OrderHarvest(MScript::GetPartByName("#27AE2#Siphon"), MScript::GetPartByName("#27A80#"));
				MScript::OrderHarvest(MScript::GetPartByName("#27AF2#Siphon"), MScript::GetPartByName("#27A80#"));
				MScript::OrderHarvest(MScript::GetPartByName("#27B02#Siphon"), MScript::GetPartByName("#27AA0#"));
				MScript::OrderHarvest(MScript::GetPartByName("#27B12#Siphon"), MScript::GetPartByName("#27AA0#"));
				MScript::OrderHarvest(MScript::GetPartByName("#27B22#Siphon"), MScript::GetPartByName("#27AB0#"));
				MScript::OrderHarvest(MScript::GetPartByName("#27B32#Siphon"), MScript::GetPartByName("#27AB0#"));

				MPartRef MantisTrigger = MScript::GetPartByName("#A9B0#");

				MScript::EnableTrigger(MantisTrigger, true);

				MScript::SetTriggerProgram(MantisTrigger, "Demo_MantisAttack");
			
				//initialize Win and Defeat Conditions
//				MScript::RunProgramByName("Demo_WinConditions", MPartRef());
				MScript::RunProgramByName("Demo_DefeatConditions", MPartRef());
				
				MScript::SetGameSpeed(0);

				//MScript::EnableMovieMode(true);
				MScript::MoveCamera(MScript::GetPartByName("Admiral Steele"), 0.0, MOVIE_CAMERA_JUMP_TO);
				MScript::SaveCameraPos();
                MScript::ChangeCamera (MScript::GetPartByName("Fleet_Camera_1"), 0, MOVIE_CAMERA_SLIDE_TO);
				//Admiral Steele
				state = PreBriefing1;
			}

			break;

		case PreBriefing1:
		{
			MScript::EnableMovieMode(true);
			

			PreStream1 = MScript::PlayAudio("bcast_01.wav");

			MScript::ClearCameraQueue();

			MScript::ChangeCamera (MScript::GetPartByName("Fleet_Camera_1"), 2, MOVIE_CAMERA_SLIDE_TO);
			MScript::ChangeCamera (MScript::GetPartByName("Planet_1"), 5, MOVIE_CAMERA_QUEUE);
			MScript::ChangeCamera (MScript::GetPartByName("Planet_1"), 1, MOVIE_CAMERA_QUEUE);
			MScript::ChangeCamera (MScript::GetPartByName("Planet_2"), 5, MOVIE_CAMERA_QUEUE);
			MScript::ChangeCamera (MScript::GetPartByName("Planet_2"), 1, MOVIE_CAMERA_QUEUE);
			MScript::ChangeCamera (MScript::GetPartByName("Planet_3"), 5, MOVIE_CAMERA_QUEUE);
			MScript::ChangeCamera (MScript::GetPartByName("Planet_3"), 1, MOVIE_CAMERA_QUEUE);
			MScript::ChangeCamera (MScript::GetPartByName("Planet_4"), 5, MOVIE_CAMERA_QUEUE);
			MScript::ChangeCamera (MScript::GetPartByName("Planet_4"), 1, MOVIE_CAMERA_QUEUE);
//			MScript::ChangeCamera (MScript::GetPartByName("Start_Camera"), 9, MOVIE_CAMERA_QUEUE);
			count = 0;
			state = PreBriefing2;
			return true;
		}break;

		case PreBriefing2:
		{
			if(!MScript::IsStreamPlaying(PreStream1))
			{
				MScript::AlertObjectInSysMap(MScript::GetPartByName("#22F90#"));
				MScript::SetMissionID(0);
				MScript::SetMissionName(IDS_Demo_Name);
				MScript::SetMissionDescription(IDS_Demo_Desc);
				MScript::AddToObjectiveList(IDS_Demo_Obj_1);

				PreStream2 = MScript::PlayAnimatedMessage("comm_blackwell_01.wav", "Animate!!Blackwell2", 550, 25, IDS_Demo_Obj_1);
				
				MScript::ClearCameraQueue();

				MScript::ChangeCamera (MScript::GetPartByName("Planet_4"), 3, MOVIE_CAMERA_SLIDE_TO);
				MScript::ChangeCamera (MScript::GetPartByName("Hybrid_Close_2"), 7, MOVIE_CAMERA_QUEUE);
				MScript::ChangeCamera (MScript::GetPartByName("Hybrid_Close_2"), 2, MOVIE_CAMERA_QUEUE);
				MScript::ChangeCamera (MScript::GetPartByName("Fleet_Camera_1"), 7, MOVIE_CAMERA_QUEUE);
				MScript::ChangeCamera (MScript::GetPartByName("Fleet_Camera_1"), 1, MOVIE_CAMERA_QUEUE);
				MScript::ChangeCamera (MScript::GetPartByName("Cel_Planet_1"), 0, MOVIE_CAMERA_QUEUE | MOVIE_CAMERA_JUMP_TO);
				MScript::ChangeCamera (MScript::GetPartByName("Cel_Planet_1"), 2, MOVIE_CAMERA_QUEUE);
				MScript::ChangeCamera (MScript::GetPartByName("Cel_Base"), 7, MOVIE_CAMERA_QUEUE);
				MScript::ChangeCamera (MScript::GetPartByName("Cel_Base"), 2, MOVIE_CAMERA_QUEUE);
				MScript::ChangeCamera (MScript::GetPartByName("Start_Camera"), 0, MOVIE_CAMERA_QUEUE | MOVIE_CAMERA_JUMP_TO);
				state = Tutor0;
				return true;
			}
			return true;

		}break;

		case Briefing:
		{
			if(count >= 5)
			{
				//Play Objective
				MScript::SetMissionID(0);
				MScript::SetMissionName(IDS_Demo_Name);
				MScript::SetMissionDescription(IDS_Demo_Desc);
				MScript::AddToObjectiveList(IDS_Demo_Obj_1);
				StreamB = MScript::PlayAnimatedMessage("comm_blackwell_01_5.wav", "Animate!!Blackwell2", 550, 25, IDS_Demo_Obj_1);
				//MScript::DrawLine(550, 25, MScript::GetPartByName("#35FD1#Infiltrator"), RGB(63, 199, 208), 7000, 2000);
				MScript::AlertObjectInSysMap(MScript::GetPartByName("#22F90#"));
				state = Tutor0;
				count = 0;

				return true;
			}
			else
				return true;
		}
		break;

		case Tutor0:
		{
			if(!MScript::IsStreamPlaying(PreStream2))
			{
				MScript::EnableMovieMode(false);
				MScript::PauseGame(true);
				MScript::LoadCameraPos(0, 0);
				MScript::ChangeCamera (MScript::GetPartByName("Start_Camera"), 0, MOVIE_CAMERA_JUMP_TO);
				Stream1 = MScript::PlayAnimatedMessage("demo2_tut_01.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				
				MScript::MakeInvincible(MScript::GetPartByName("#266B2#Scout Carrier"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#266A2#Scout Carrier"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#26692#Scout Carrier"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#26642#Scout Carrier"), false);

				MScript::MakeInvincible(MScript::GetPartByName("#26632#Frigate"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#26622#Frigate"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#26652#Frigate"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#26662#Frigate"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#26672#Frigate"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#26682#Frigate"), false);

				MScript::MakeInvincible(MScript::GetPartByName("#26722#Hive Carrier"), false);

				MScript::MakeInvincible(MScript::GetPartByName("#2CF21#ESP Coil"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#2CF11#ESP Coil"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#2CF01#ESP Coil"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#2CF41#ESP Coil"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#2CF31#ESP Coil"), false);

				MScript::MakeInvincible(MScript::GetPartByName("#26701#Hydrofoil"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#266E1#Hydrofoil"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#266F1#Hydrofoil"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#26711#Hydrofoil"), false);
				MScript::MakeInvincible(MScript::GetPartByName("#27A61#Hydrofoil"), false);

				state = Tutor1;
				return true;
			}
			return true;
		}

		case Tutor1:
		{
			if(!MScript::IsStreamPlaying(Stream1))
			{
				Stream2 = MScript::PlayAnimatedMessage("demo2_tut_02.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream2 = MScript::PlayAudio("demo2_tut_02.wav");
				state = Tutor2;

				MScript::EnableJumpgate(MScript::GetJumpgateWormhole(MScript::GetPartByName("#37B1#Jump Gate")), false);

				MScript::DrawLine(170, 15, 170, 1, RGB(208, 199, 63), 6000, 0);
				MScript::DrawLine(639, 15, 639, 1, RGB(208, 199, 63), 6000, 0);
				MScript::DrawLine(170, 15, 639, 15, RGB(208, 199, 63), 6000, 0);
				MScript::DrawLine(170, 1, 639, 1, RGB(208, 199, 63), 6000, 0);

				return true;
			}
			return true;
		}
		break;
		case Tutor2:
		{
			if(!MScript::IsStreamPlaying(Stream2))
			{
				Stream3 = MScript::PlayAnimatedMessage("demo2_tut_03.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream3 = MScript::PlayAudio("demo2_tut_03.wav");
				state = Tutor3;		

				MScript::DrawLine(230, 355, 310, 355, RGB(208, 199, 63), 4000, 0);
				MScript::DrawLine(230, 479, 310, 479, RGB(208, 199, 63), 4000, 0);
				MScript::DrawLine(230, 355, 230, 479, RGB(208, 199, 63), 4000, 0);
				MScript::DrawLine(310, 355, 310, 479, RGB(208, 199, 63), 4000, 0);

				return true;
			}
			return true;
		}
		break;
		case Tutor3:
		{
			if(!MScript::IsStreamPlaying(Stream3))
			{
				Stream4 = MScript::PlayAnimatedMessage("demo2_tut_04.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream4 = MScript::PlayAudio("demo2_tut_04.wav");
				state = Tutor4;	

				MScript::DrawLine(1, 355, 230, 355, RGB(208, 199, 63), 5000, 0);
				MScript::DrawLine(1, 479, 230, 479, RGB(208, 199, 63), 5000, 0);
				MScript::DrawLine(1, 355, 1, 479, RGB(208, 199, 63), 5000, 0);
				MScript::DrawLine(230, 355, 230, 479, RGB(208, 199, 63), 5000, 0);

				return true;
			}
			return true;
		}
		break;
		case Tutor4:
		{
			if(!MScript::IsStreamPlaying(Stream4))
			{
				Stream5 = MScript::PlayAnimatedMessage("demo2_tut_05.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream5 = MScript::PlayAudio("demo2_tut_05.wav");
				state = Tutor5;			

				MScript::DrawLine(340, 320, 340, 479, RGB(208, 199, 63), 5000, 0);
				MScript::DrawLine(495, 320, 495, 479, RGB(208, 199, 63), 5000, 0);
				MScript::DrawLine(340, 320, 495, 320, RGB(208, 199, 63), 5000, 0);
				MScript::DrawLine(340, 479, 495, 479, RGB(208, 199, 63), 5000, 0);

				MScript::DrawLine(495, 330, 495, 479, RGB(208, 199, 63), 5000, 0);
				MScript::DrawLine(639, 330, 639, 479, RGB(208, 199, 63), 5000, 0);
				MScript::DrawLine(495, 330, 639, 330, RGB(208, 199, 63), 5000, 0);
				MScript::DrawLine(495, 479, 639, 479, RGB(208, 199, 63), 5000, 0);

				return true;
			}
			return true;
		}
		break;
		case Tutor5:
		{
			if(!MScript::IsStreamPlaying(Stream5))
			{
				Stream6 = MScript::PlayAnimatedMessage("demo2_tut_06.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream6 = MScript::PlayAudio("demo2_tut_06.wav");
				state = Tutor6;
//				MScript::StartFlashUI(IDH_FORM_FLEET);
				MScript::PauseGame(false);
				return true;
			}
			return true;
		}
		break;
		case Tutor6:
		{
			if(!MScript::IsStreamPlaying(Stream6) && MScript::IsSelected(MScript::GetPartByName("#1CAC1#Dreadnought")))
			{
				Stream7 = MScript::PlayAnimatedMessage("demo2_tut_07.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream7 = MScript::PlayAudio("demo2_tut_07.wav");
				state = Tutor7;
				MScript::StartFlashUI(IDH_FORM_FLEET);
				MScript::RunProgramByName("Demo_DetectFormFormation", MPartRef());
				return true;
			}
			return true;
		}
		break;
		case Tutor7:
		{
			//MPartRef Admiral = MScript::GetPartByName("Admiral Steele");

			MPartRef Admiral = MScript::GetPartByName("#1CAC1#Dreadnought");

			if(!MScript::IsStreamPlaying(Stream7) && Admiral->fleetID != 0) // && MScript::IsSelected(Admiral))
			{
				Stream8 = MScript::PlayAnimatedMessage("demo2_tut_08.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream8 = MScript::PlayAudio("demo2_tut_08.wav");
				state = Teletype;		
				MScript::StopFlashUI(IDH_FORM_FLEET);
				return true;
			}
			return true;
		}
		break;

		case Teletype:
		{

			if(count >= 30)
			{
				//Play Teletype
				TT = MScript::PlayTeletype(IDS_Demo_Obj_1, 150, 175, 500, 300, RGB(63, 199, 208), 8000, 2000, false);
				Alert = MScript::StartAlertAnim(MScript::GetPartByName("#37B1#Jump Gate"));
				MScript::EnableJumpgate(MScript::GetJumpgateWormhole(MScript::GetPartByName("#37B1#Jump Gate")), true);
				state = Tutor8;
				MScript::PauseGame(false);
				//MScript::EnableMovieMode(false);
				count = 0;
				return true;
			}
			return true;
		}
		break;

		case Tutor8:
		{
			if(!MScript::IsStreamPlaying(Stream8))
			{
				MScript::FlushStreams();
				Stream9 = MScript::PlayAnimatedMessage("demo2_tut_09.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream9 = MScript::PlayAudio("demo2_tut_09.wav");
				state = Finish;			
				return true;
			}
			return true;
		}
		break;

		case Finish:
		{
			if(!MScript::IsStreamPlaying(Stream9))
			{
				count = 0;
				//MScript::StartFlashUI(IDH_ADMIRAL_TACTIC_DEFEND);
				MScript::StopAlertAnim(Alert);
				return false;
			}
		}
	}
	return true;
}

CQSCRIPTPROGRAM(Demo_DetectFormFormation, Demo_DetectFormFormation_Save,0);

void Demo_DetectFormFormation::Initialize (U32 eventFlags, const MPartRef & part)
{}
bool Demo_DetectFormFormation::Update (void)
{
	MPartRef Admiral = MScript::GetPartByName("#1CAC1#Dreadnought");

	if(MScript::IsSelected(Admiral) && Admiral->fleetID != 0)
	{
		MScript::StopFlashUI(IDH_FORM_FLEET);
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
	count = 0;
	//Activate Mantis AI
	MScript::EnableEnemyAI(2, true, "MANTIS_FORTRESS");

	AIPersonality airules;

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

	MScript::EnableTrigger(part, false);
}

bool Demo_MantisAttack::Update (void)
{
	count = count + 1;
	switch(state)
	{
		case Begin:
		{
			if (count >= 10)
			{
				//Group 1
				MPartRef Frigate1_1 = MScript::GetPartByName("#A862#Frigate");
				MPartRef Frigate2_1 = MScript::GetPartByName("#A872#Frigate");
				MPartRef Scarab1_1 = MScript::GetPartByName("#A7B2#Scarab");
				MPartRef Tiamat1_1 = MScript::GetPartByName("#A732#Tiamat");
				
				//Group 2
				MPartRef Frigate1_2 = MScript::GetPartByName("#A852#Frigate");
				MPartRef Frigate2_2 = MScript::GetPartByName("#A842#Frigate");
				MPartRef Scarab1_2 = MScript::GetPartByName("#A7C2#Scarab");
				MPartRef Tiamat1_2 = MScript::GetPartByName("#A722#Tiamat");
				
				//Group 3
				MPartRef Frigate1_3 = MScript::GetPartByName("#A8C2#Frigate");
				MPartRef Frigate2_3 = MScript::GetPartByName("#A8D2#Frigate");
				MPartRef Scarab1_3 = MScript::GetPartByName("#A762#Scarab");
				MPartRef Tiamat1_3 = MScript::GetPartByName("#A752#Tiamat");

				//Group 4
				MPartRef Frigate1_4 = MScript::GetPartByName("#A802#Frigate");
				MPartRef Frigate2_4 = MScript::GetPartByName("#A812#Frigate");
				MPartRef Scarab1_4 = MScript::GetPartByName("#A7E2#Scarab");
				MPartRef Tiamat1_4 = MScript::GetPartByName("#A712#Tiamat");

				//Group 5
				MPartRef Frigate1_5 = MScript::GetPartByName("#A8B2#Frigate");
				MPartRef Frigate2_5 = MScript::GetPartByName("#A8A2#Frigate");
				MPartRef Scarab1_5 = MScript::GetPartByName("#A782#Scarab");
				MPartRef Tiamat1_5 = MScript::GetPartByName("#A742#Tiamat");

				//Group Waypoints
				MPartRef Group1WP = MScript::GetPartByName("#A990#");
				MPartRef Group2WP = MScript::GetPartByName("#A950#");
				MPartRef Group3WP = MScript::GetPartByName("#A960#");
				MPartRef Group4WP = MScript::GetPartByName("#A980#");
				MPartRef Group5WP = MScript::GetPartByName("#A970#");

				//Issue Move Command Group 1
				MScript::OrderMoveTo(Frigate1_1, Group1WP, false);
				MScript::OrderMoveTo(Frigate2_1, Group1WP, false);
				MScript::OrderMoveTo(Scarab1_1, Group1WP, false);
				MScript::OrderMoveTo(Tiamat1_1, Group1WP, false);

				//Issue Attack Command Group 1
				MScript::SetStance(Frigate1_1, US_ATTACK);
				MScript::SetStance(Frigate2_1, US_ATTACK);
				MScript::SetStance(Scarab1_1, US_ATTACK);
				MScript::SetStance(Tiamat1_1, US_ATTACK);

				//Issue Move Command Group 2
				MScript::OrderMoveTo(Frigate1_2, Group2WP, false);
				MScript::OrderMoveTo(Frigate2_2, Group2WP, false);
				MScript::OrderMoveTo(Scarab1_2, Group2WP, false);
				MScript::OrderMoveTo(Tiamat1_2, Group2WP, false);
				
				//Issue Attack Command Group 2
				MScript::SetStance(Frigate1_2, US_ATTACK);
				MScript::SetStance(Frigate2_2, US_ATTACK);
				MScript::SetStance(Scarab1_2, US_ATTACK);
				MScript::SetStance(Tiamat1_2, US_ATTACK);

				//Issue Move Command Group 3
				MScript::OrderMoveTo(Frigate1_3, Group3WP, false);
				MScript::OrderMoveTo(Frigate2_3, Group3WP, false);
				MScript::OrderMoveTo(Scarab1_3, Group3WP, false);
				MScript::OrderMoveTo(Tiamat1_3, Group3WP, false);

				//Issue Attack Command Group 3	
				MScript::SetStance(Frigate1_3, US_ATTACK);
				MScript::SetStance(Frigate2_3, US_ATTACK);
				MScript::SetStance(Scarab1_3, US_ATTACK);
				MScript::SetStance(Tiamat1_3, US_ATTACK);

				//Issue Move Command Group 4
				MScript::OrderMoveTo(Frigate1_4, Group4WP, false);
				MScript::OrderMoveTo(Frigate2_4, Group4WP, false);
				MScript::OrderMoveTo(Scarab1_4, Group4WP, false);
				MScript::OrderMoveTo(Tiamat1_4, Group4WP, false);

				//Issue Attack Command Group 4
				MScript::SetStance(Frigate1_4, US_ATTACK);
				MScript::SetStance(Frigate2_4, US_ATTACK);
				MScript::SetStance(Scarab1_4, US_ATTACK);
				MScript::SetStance(Tiamat1_4, US_ATTACK);

				//Issue Move Command Group 5
				MScript::OrderMoveTo(Frigate1_5, Group5WP, false);
				MScript::OrderMoveTo(Frigate2_5, Group5WP, false);
				MScript::OrderMoveTo(Scarab1_5, Group5WP, false);
				MScript::OrderMoveTo(Tiamat1_5, Group5WP, false);

				//Issue Attack Command Group 5
				MScript::SetStance(Frigate1_5, US_ATTACK);
				MScript::SetStance(Frigate2_5, US_ATTACK);
				MScript::SetStance(Scarab1_5, US_ATTACK);
				MScript::SetStance(Tiamat1_5, US_ATTACK);
							
				//Play Blackwell 
				MScript::FlushStreams();
				StreamB = MScript::PlayAnimatedMessage("comm_blackwell_02.wav", "Animate!!Blackwell2", 550, 25, IDS_Demo_Obj_1);

				//Enable next comm
				MPartRef CommTrigger = MScript::GetPartByName("#39D50#");

				MScript::EnableTrigger(CommTrigger, true);

				MScript::SetTriggerProgram(CommTrigger, "Demo_ConvoyComm");
				MScript::RunProgramByName("Demo_DetectHalseyFormation", MPartRef());
				MScript::SelectUnit(MScript::GetPartByName("Admiral Steele"));
				state = Tutor0;
				
				return true;
			}
			return true;

		}break;
		case Tutor0:
		{
			if(!MScript::IsStreamPlaying(StreamB))
			{
				state = Tutor0_5;
				MScript::FlushStreams();
				Stream1 = MScript::PlayAnimatedMessage("demo2_tut_10a.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				MScript::SelectUnit(MScript::GetPartByName("Admiral Steele"));
				//Stream1 = MScript::PlayAudio("demo2_tut_10a.wav");
				//MScript::StartFlashUI(IDH_FORMATION_1);
				MScript::PauseGame(true);
				count = 0;
				return true;
			}
			return true;
		}
		case Tutor0_5:
		{
			if (count >= 40)
			{
				MScript::StartFlashUI(IDH_FORMATION_1);
				state = Tutor1;
				return true;
			}
			return true;
		}
		case Tutor1:
		{	
			if(!MScript::IsStreamPlaying(Stream1))
			{
				state = Tutor2;
				Stream2 = MScript::PlayAnimatedMessage("demo2_tut_10b.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream2 = MScript::PlayAudio("demo2_tut_10b.wav");
				MScript::SelectUnit(MScript::GetPartByName("Admiral Steele"));
				MScript::StartFlashUI(IDH_FORMATION_2);
				MScript::StopFlashUI(IDH_FORMATION_1);
				return true;
			}
			return true;
		}break;
		
		case Tutor2:
		{
			if(!MScript::IsStreamPlaying(Stream2))
			{
				state = Tutor3;
				Stream3 = MScript::PlayAnimatedMessage("demo2_tut_10c.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream3 = MScript::PlayAudio("demo2_tut_10c.wav");
				MScript::SelectUnit(MScript::GetPartByName("Admiral Steele"));
				MScript::StartFlashUI(IDH_FORMATION_4);
				MScript::StopFlashUI(IDH_FORMATION_2);
				return true;
			}
			return true;
			
		}break;

		case Tutor3:
		{
			if(!MScript::IsStreamPlaying(Stream3))
			{
				state = Tutor4;
				Stream4 = MScript::PlayAnimatedMessage("demo2_tut_10d.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream4 = MScript::PlayAudio("demo2_tut_10d.wav");
				MScript::SelectUnit(MScript::GetPartByName("Admiral Steele"));
				MScript::StartFlashUI(IDH_FORMATION_5);
				MScript::StopFlashUI(IDH_FORMATION_4);
				return true;
			}
			return true;
			
		}break;

		case Tutor4:
		{
			if(!MScript::IsStreamPlaying(Stream4))
			{
				state = Tutor5;
				Stream5 = MScript::PlayAnimatedMessage("demo2_tut_10e.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream5 = MScript::PlayAudio("demo2_tut_10e.wav");
				MScript::SelectUnit(MScript::GetPartByName("Admiral Steele"));
				MScript::StartFlashUI(IDH_FORMATION_3);
				MScript::StopFlashUI(IDH_FORMATION_5);
				return true;
			}
			return true;
			
		}break;

		case Tutor5:
		{
			if(!MScript::IsStreamPlaying(Stream5))
			{
				state = Tutor6;
				Stream6 = MScript::PlayAnimatedMessage("demo2_tut_10f.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream6 = MScript::PlayAudio("demo2_tut_10f.wav");
				return true;
			}
			return true;
			
		}break;

		case Tutor6:
		{
			if(!MScript::IsStreamPlaying(Stream6))
			{
				Stream7 = MScript::PlayAnimatedMessage("demo2_tut_11.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream7 = MScript::PlayAudio("demo2_tut_11.wav");
				state = Finish;
				data.Mantis_WP_Alert = MScript::StartAlertAnim(MScript::GetPartByName("#442D0#"));
				MScript::StopFlashUI(IDH_FORMATION_2);
				MScript::StopFlashUI(IDH_FORMATION_3);
				MScript::StopFlashUI(IDH_FORMATION_4);
				MScript::StopFlashUI(IDH_FORMATION_5);
				MScript::PauseGame(false);
				return true;
			}
			return true;
		}
		break;

		case Tutor7:
		{
			if(!MScript::IsStreamPlaying(Stream7))
			{
				Stream8 = MScript::PlayAnimatedMessage("demo2_tut_12.wav", "Animate!!Blackwell2", 550, 25, IDS_Demo_Obj_1);
				//Stream8 = MScript::PlayAudio("demo2_tut_12.wav");
				state = Finish;
				return true;
			}
			return true;
		}
		break;

		case Finish:
			{
				if(!MScript::IsStreamPlaying(Stream7))
				{
					//Start Basilisk Attack
					MScript::RunProgramByName("Demo_BasiliskAttack", MPartRef());
					MScript::StopAlertAnim(Alert);
					return false;
				}
				return true;
			}

        }

		return true;
}

CQSCRIPTPROGRAM(Demo_ConvoyComm, Demo_ConvoyComm_Save,0);
void Demo_ConvoyComm::Initialize (U32 eventFlags, const MPartRef & part)
{	
	state = Begin;
	MScript::EnableTrigger(part, false);
}

bool Demo_ConvoyComm::Update (void)
{	
	switch(state)
	{
	case Begin:
		{
			MScript::FlushStreams();
			StreamB = MScript::PlayAnimatedMessage("comm_blackwell_03.wav", "Animate!!Blackwell2", 550, 25, IDS_Demo_Obj_1);
			state = Tutor0;
			return true;
		}break;
	case Tutor0:
		{
			if(!MScript::IsStreamPlaying(StreamB))
			{
				MScript::FlushStreams();
				Stream1 = MScript::PlayAnimatedMessage("demo2_tut_13.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream1 = MScript::PlayAudio("demo2_tut_13.wav");
				MScript::StopAlertAnim(data.Mantis_WP_Alert);
				data.Celareon_WP_Alert = MScript::StartAlertAnim(MScript::GetPartByName("#388D0#"));
				MScript::PauseGame(true);
				state = Tutor1;
				return true;
			}
			return true;
		}
	case Tutor1:
		{
			if(!MScript::IsStreamPlaying(Stream1))
			{
				MScript::FlushStreams();
				Stream2 = MScript::PlayAnimatedMessage("demo2_tut_14.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream2 = MScript::PlayAudio("demo2_tut_14.wav");
				MScript::PauseGame(false);
				MScript::SelectUnit(MScript::GetPartByName("Admiral Steele"));
				MScript::StartFlashUI(IDH_FORMATION_4);
				MScript::RunProgramByName("Demo_DetectConvoyFormation", MPartRef());
				state = Finish;
				return true;
			}
		}break;
	case Finish:
		{
			
			if(!MScript::IsStreamPlaying(Stream2))
			{
		//		MScript::PlayAnimatedMessage("comm_procyo_01.wav", "Animate!!Vivac2", 550, 25, IDS_Demo_Alert_2);
		//		MScript::PlayTeletype(IDS_Demo_Alert_2, 150, 175, 500, 300, RGB(63, 199, 208), 8000, 2000, false);
				MScript::StopFlashUI(IDH_FORMATION_4);
				return false;
			}
		}break;
    }
	return true;
}

CQSCRIPTPROGRAM(Demo_DetectConvoyFormation, Demo_DetectConvoyFormation_Save,0);

void Demo_DetectConvoyFormation::Initialize (U32 eventFlags, const MPartRef & part)
{	count = 0;}

bool Demo_DetectConvoyFormation::Update (void)
{	
	MPartRef Admiral = MScript::GetPartByName("Admiral Steele");
	char formation[] = "FORMATION!!T_Convoy";

	if (strcmp(MScript::GetFormationName(Admiral),formation) == 0)
	{
		MScript::StopFlashUI(IDH_FORMATION_4);
		return false;
	}
	return true;
}

CQSCRIPTPROGRAM(Demo_DetectHalseyFormation, Demo_DetectHalseyFormation_Save,0);

void Demo_DetectHalseyFormation::Initialize (U32 eventFlags, const MPartRef & part)
{	count = 0;}

bool Demo_DetectHalseyFormation::Update (void)
{	
	MPartRef Admiral = MScript::GetPartByName("Admiral Steele");
	char formation[] = "FORMATION!!T_Halsey";

	if (strcmp(MScript::GetFormationName(Admiral),formation) == 0)
	{
		MScript::StopFlashUI(IDH_FORMATION_3);
		return false;
	}
	return true;
}

CQSCRIPTPROGRAM(Demo_DetectDefenseFormation, Demo_DetectDefenseFormation_Save,0);

void Demo_DetectDefenseFormation::Initialize (U32 eventFlags, const MPartRef & part)
{	count = 0;}

bool Demo_DetectDefenseFormation::Update (void)
{	
	MPartRef Admiral = MScript::GetPartByName("Admiral Steele");
	char formation[] = "FORMATION!!T_PlatDefence";

	if (strcmp(MScript::GetFormationName(Admiral),formation) == 0)
	{
		MScript::StopFlashUI(IDH_FORMATION_5);
		return false;
	}
	return true;
}

CQSCRIPTPROGRAM(Demo_BasiliskAttack, Demo_BasiliskAttack_Save,0);

void Demo_BasiliskAttack::Initialize (U32 eventFlags, const MPartRef & part)
{	count = 0;}

bool Demo_BasiliskAttack::Update (void)
{	
	count = count + 1;

	if (count >= 60)
	{	
		MPartRef Basilisk1_1 = MScript::GetPartByName("#E53#Basilisk");
		MPartRef Basilisk1_2 = MScript::GetPartByName("#CD3#Basilisk");
		MPartRef Basilisk1_3 = MScript::GetPartByName("#DD3#Basilisk");
		MPartRef Basilisk1_4 = MScript::GetPartByName("#CC3#Basilisk");
		MPartRef Basilisk1_5 = MScript::GetPartByName("#DB3#Basilisk");
		MPartRef Basilisk1_6 = MScript::GetPartByName("#CB3#Basilisk");
		MPartRef Basilisk1_7 = MScript::GetPartByName("#ED3#Basilisk");

		//Targets
		MPartRef Target1 = MScript::GetPartByName("#BE90#");
		MPartRef Target2 = MScript::GetPartByName("#BEA0#");
		MPartRef Target3 = MScript::GetPartByName("#BE80#");
		MPartRef Target4 = MScript::GetPartByName("#BEE0#");
		MPartRef Target5 = MScript::GetPartByName("#BEB0#");
		MPartRef Target6 = MScript::GetPartByName("#BED0#");
		MPartRef Target7 = MScript::GetPartByName("#BEC0#");

		MScript::OrderSpecialAttack(Basilisk1_1, Target1);
		MScript::OrderSpecialAttack(Basilisk1_2, Target2);
		MScript::OrderSpecialAttack(Basilisk1_3, Target3);
		MScript::OrderSpecialAttack(Basilisk1_4, Target4);
		MScript::OrderSpecialAttack(Basilisk1_5, Target5);
		MScript::OrderSpecialAttack(Basilisk1_6, Target6);
		MScript::OrderSpecialAttack(Basilisk1_7, Target7);
		MScript::SetStance(Basilisk1_1, US_ATTACK);
		MScript::SetStance(Basilisk1_2, US_ATTACK);
		MScript::SetStance(Basilisk1_3, US_ATTACK);
		MScript::SetStance(Basilisk1_4, US_ATTACK);
		MScript::SetStance(Basilisk1_5, US_ATTACK);
		MScript::SetStance(Basilisk1_6, US_ATTACK);
		MScript::SetStance(Basilisk1_7, US_ATTACK);

		//Set up Surprise Attack
		MPartRef SurpriseTrigger = MScript::GetPartByName("#22F90#");
		MPartRef WarningTrigger = MScript::GetPartByName("#388D0#");

		MScript::SetTriggerProgram(SurpriseTrigger, "Demo_SurpriseAttack");
		MScript::EnableTrigger(SurpriseTrigger, true);

//		MScript::SetTriggerProgram(WarningTrigger, "Demo_BasiliskWarning");
//		MScript::EnableTrigger(WarningTrigger, true);
		return false;
	}
	return true;
}

CQSCRIPTPROGRAM(Demo_BasiliskAttack2, Demo_BasiliskAttack2_Save,0);

void Demo_BasiliskAttack2::Initialize (U32 eventFlags, const MPartRef & part)
{	
	state = Begin;
	count = 0;
}

bool Demo_BasiliskAttack2::Update (void)
{
	switch(state)
	{
	case Begin:
		{
		MScript::StopAlertAnim(data.Celareon_WP_Alert);
		Stream1 = MScript::PlayAnimatedMessage("comm_procyo_01.wav", "Animate!!Vivac2", 550, 25, IDS_Demo_Obj_1);
		//Stream1 = MScript::PlayAudio("comm_procyo_01.wav");
		MScript::RunProgramByName("Demo_DetectDefenseFormation", MPartRef());
		//MScript::PauseGame(true);

		MPartRef Spawn = MScript::GetPartByName("#21D33#Adder");

		MPartRef Basilisk2_1 = MScript::GetPartByName("#E43#Basilisk");
		MPartRef Basilisk2_2 = MScript::GetPartByName("#C83#Basilisk");
		MPartRef Basilisk2_3 = MScript::GetPartByName("#DC3#Basilisk");
		MPartRef Basilisk2_4 = MScript::GetPartByName("#C93#Basilisk");
		MPartRef Basilisk2_5 = MScript::GetPartByName("#DA3#Basilisk");
		MPartRef Basilisk2_6 = MScript::GetPartByName("#CA3#Basilisk");
		MPartRef Basilisk2_7 = MScript::GetPartByName("#EC3#Basilisk");

		MPartRef Target1 = MScript::GetPartByName("#BE90#");
		MPartRef Target2 = MScript::GetPartByName("#BEA0#");
		MPartRef Target3 = MScript::GetPartByName("#BE80#");
		MPartRef Target4 = MScript::GetPartByName("#BEE0#");
		MPartRef Target5 = MScript::GetPartByName("#BEB0#");
		MPartRef Target6 = MScript::GetPartByName("#BED0#");
		MPartRef Target7 = MScript::GetPartByName("#BEC0#");

		MPartRef Basalisk1 = MScript::CreatePart("GBOAT!!V_Basalisk", Spawn, 3, "Basalisk");
		MScript::OrderSpecialAttack(Basalisk1, Target1);
		MScript::SetStance(Basalisk1, US_ATTACK);

		MPartRef Basalisk2 = MScript::CreatePart("GBOAT!!V_Basalisk", Spawn, 3, "Basalisk");
		MScript::OrderSpecialAttack(Basalisk2, Target2);
		MScript::SetStance(Basalisk2, US_ATTACK);

		MPartRef Basalisk3 = MScript::CreatePart("GBOAT!!V_Basalisk", Spawn, 3, "Basalisk");
		MScript::OrderSpecialAttack(Basalisk3, Target3);
		MScript::SetStance(Basalisk3, US_ATTACK);

		MPartRef Basalisk4 = MScript::CreatePart("GBOAT!!V_Basalisk", Spawn, 3, "Basalisk");
		MScript::OrderSpecialAttack(Basalisk4, Target4);
		MScript::SetStance(Basalisk4, US_ATTACK);

		MPartRef Basalisk5 = MScript::CreatePart("GBOAT!!V_Basalisk", Spawn, 3, "Basalisk");
		MScript::OrderSpecialAttack(Basalisk5, Target5);
		MScript::SetStance(Basalisk5, US_ATTACK);

		MPartRef Basalisk6 = MScript::CreatePart("GBOAT!!V_Basalisk", Spawn, 3, "Basalisk");
		MScript::OrderSpecialAttack(Basalisk6, Target6);
		MScript::SetStance(Basalisk6, US_ATTACK);

		MPartRef Basalisk7 = MScript::CreatePart("GBOAT!!V_Basalisk", Spawn, 3, "Basalisk");
		MScript::OrderSpecialAttack(Basalisk7, Target7);
		MScript::SetStance(Basalisk7, US_ATTACK);

		MScript::OrderSpecialAttack(Basilisk2_1, Target1);
		MScript::OrderSpecialAttack(Basilisk2_2, Target2);
		MScript::OrderSpecialAttack(Basilisk2_3, Target3);
		MScript::OrderSpecialAttack(Basilisk2_4, Target4);
		MScript::OrderSpecialAttack(Basilisk2_5, Target5);
		MScript::OrderSpecialAttack(Basilisk2_6, Target6);
		MScript::OrderSpecialAttack(Basilisk2_7, Target7);
		MScript::SetStance(Basilisk2_1, US_ATTACK);
		MScript::SetStance(Basilisk2_2, US_ATTACK);
		MScript::SetStance(Basilisk2_3, US_ATTACK);
		MScript::SetStance(Basilisk2_4, US_ATTACK);
		MScript::SetStance(Basilisk2_5, US_ATTACK);
		MScript::SetStance(Basilisk2_6, US_ATTACK);
		MScript::SetStance(Basilisk2_7, US_ATTACK);
		
		state = Tutor1;

		return true;
		}break;
	case Tutor1:
		{
			if(!MScript::IsStreamPlaying(Stream1))
			{
				Stream2 = MScript::PlayAnimatedMessage("demo2_tut_15.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_1);
				//Stream2 = MScript::PlayAudio("demo2_tut_15.wav");
				MScript::SelectUnit(MScript::GetPartByName("Admiral Steele"));
				MScript::StartFlashUI(IDH_FORMATION_5);
				MScript::PauseGame(true);
				state = Finish;
				return true;
			}
			return true;
		}break;

	case Finish:
		{
			if(!MScript::IsStreamPlaying(Stream2))
			{
				MScript::StopFlashUI(IDH_FORMATION_5);
				MScript::PauseGame(false);
				return false;
			}
			return true;
		}break;
	}
	return true;
}

CQSCRIPTPROGRAM(Demo_BasiliskWarning, Demo_BasiliskWarning_Save,0);

void Demo_BasiliskWarning::Initialize (U32 eventFlags, const MPartRef & part)
{	
	state = Begin;
	MPartRef WarningTrigger = MScript::GetPartByName("#388D0#");
	MScript::EnableTrigger(WarningTrigger, false);
	count = 0;
}

bool Demo_BasiliskWarning::Update (void)
{
	count = count + 1;
	if(count >= 15)
	{
        MScript::PlayAnimatedMessage("comm_procyo_01.wav", "Animate!!Vivac2", 550, 25, IDS_Demo_Alert_2);
		MScript::PlayTeletype(IDS_Demo_Alert_2, 150, 175, 500, 300, RGB(63, 199, 208), 8000, 2000, false);
		return false;
	}
}

CQSCRIPTPROGRAM(Demo_HQDestroyed, Demo_HQDestroyed_Save,0);

void Demo_HQDestroyed::Initialize (U32 eventFlags, const MPartRef & part)
{	
	state = Begin;
	target = MScript::GetPartByName("#591#Headquarters");
}

bool Demo_HQDestroyed::Update (void)
{
	switch(state)
	{
	case Begin:
		{
            if(MScript::IsDead(target))
				{
				//	MScript::PlayTeletype(IDS_Demo_Alert_1, 150, 175, 500, 300, RGB(63, 199, 208), 8000, 2000, false);
					StreamB = MScript::PlayAnimatedMessage("comm_blackwell_05.wav", "Animate!!Blackwell2", 550, 25, IDS_Demo_Alert_1);
					state = Tutor0;				
					return true;
				}
				return true;
         }break;
	case Tutor0:
		{
			if(!MScript::IsStreamPlaying(StreamB))
			{
				MScript::SelectUnit(data.Nova_Bomb);
				Stream1 = MScript::PlayAnimatedMessage("demo2_tut_17.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Alert_1);
				//Stream1 = MScript::PlayAudio("demo2_tut_17.wav");
				MScript::MoveCamera(data.Nova_Bomb, 2.0, MOVIE_CAMERA_JUMP_TO);
				MScript::AlertObjectInSysMap(data.Nova_Bomb, true);
				data.Nova_Alert = MScript::StartAlertAnim(data.Nova_Bomb);
				MScript::StartFlashUI(IDH_SPECIAL_ABILITY);
				MScript::PauseGame(true);
				state = Tutor1;
				return true;
			}
			return true;
		}
	case Tutor1:
		{
			if(!MScript::IsStreamPlaying(Stream1))
			{
				MScript::PauseGame(false);
				state = Finish;
				return false;
			}
			return true;
		}break;
	}
	return true;
}

CQSCRIPTPROGRAM(Demo_SurpriseAttack, Demo_SurpriseAttack_Save,0);

void Demo_SurpriseAttack::Initialize (U32 eventFlags, const MPartRef & part)
{	
	state = Time;
	MScript::EnableTrigger(part, false);
	MScript::RunProgramByName("Demo_WinConditions", MPartRef());
	MScript::RunProgramByName("Demo_BasiliskAttack2", MPartRef());
	count = 0;
}

bool Demo_SurpriseAttack::Update (void)
{
	count = count + 1;
	switch(state)
	{
	case Time:
		{
			if (count >= 150)
			{
				MScript::RunProgramByName("Demo_HQDestroyed", MPartRef());
				MScript::RunProgramByName("Demo_SurpriseAttackWave1", MPartRef());
				count = 0;
				state = Begin;
				return true;
			}
			return true;
		}break;

	case Begin:
		{
			if (count >= 30)
			{
				MScript::MarkObjectiveCompleted(IDS_Demo_Obj_1);
				MScript::AddToObjectiveList(IDS_Demo_Obj_2);
				StreamB = MScript::PlayAnimatedMessage("comm_blackwell_04.wav", "Animate!!Blackwell2", 550, 25, IDS_Demo_Obj_2);
				MScript::PlayTeletype(IDS_Demo_Obj_2, 150, 175, 500, 300, RGB(63, 199, 208), 8000, 2000, false);
				state = Tutor0;
				return true;
			}
			return true;
		}break;
	case Tutor0:
		{
			if(!MScript::IsStreamPlaying(StreamB))
				{
					Stream1 = MScript::PlayAnimatedMessage("demo2_tut_16.wav", "Animate!!Benson2", 550, 25, IDS_Demo_Obj_2);
					//Stream1 = MScript::PlayAudio("demo2_tut_16.wav");
					MScript::PauseGame(true);
					MScript::DrawLine(495, 330, 495, 479, RGB(208, 199, 63), 7000, 0);
					MScript::DrawLine(639, 330, 639, 479, RGB(208, 199, 63), 7000, 0);
					MScript::DrawLine(495, 330, 639, 330, RGB(208, 199, 63), 7000, 0);
					MScript::DrawLine(495, 479, 639, 479, RGB(208, 199, 63), 7000, 0);
					state = Tutor1;
					return true;
				}
				return true;
		}
	case Tutor1:
		{
			if(!MScript::IsStreamPlaying(Stream1))
			{
				MScript::PauseGame(false);
				return false;
			}
			return true;
		}break;
	}
	return true;
}
//--------------------------------------------------------------------------//
//--Trigger Scripts-------------------------------------------------------//
//--Surprise Attack-------------------------------------------------------//
//--------------------------------------------------------------------------//
// Left Gate =  #E980#
// Right Gate = #115A0#

//Vyrium Attack Wave from the Left Jump Hole to Urth
CQSCRIPTPROGRAM(Demo_VyriumWave1, Demo_VyriumWave1_Save,0);

void Demo_VyriumWave1::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#E980#"), 1, 1, true);
}

bool Demo_VyriumWave1::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("Urth");
		MPartRef gate = MScript::GetPartByName("#E980#");

		MScript::FlashWormBlast(tempWormID);
		for(int i = 0; i < 7; i++)
		{
            MScript::CreatePart("GBOAT!!V_Adder", gate, 3, "Adder");
			MScript::OrderMoveTo(MScript::GetPartByName("Adder"), target, false);
			MScript::SetStance(MScript::GetPartByName("Adder"), US_ATTACK);
		}
		for(int j = 0; j < 3; j++)
		{
			MScript::CreatePart("GBOAT!!V_Leviathin", gate, 3, "Leviathan");
			MScript::OrderMoveTo(MScript::GetPartByName("Leviathan"), target, false);
			MScript::SetStance(MScript::GetPartByName("Leviathan"), US_ATTACK);
		}		
		MScript::CloseWormBlast(tempWormID);

	   	return false;
	}
	return true;
}

//Vyrium Attack Wave from the Left Jump Hole to Centauri
CQSCRIPTPROGRAM(Demo_VyriumWave2, Demo_VyriumWave2_Save,0);

void Demo_VyriumWave2::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#E980#"), 1, 1, true);
}

bool Demo_VyriumWave2::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("Centauri");
		MPartRef gate = MScript::GetPartByName("#E980#");

		MScript::FlashWormBlast(tempWormID);
		for(int i = 0; i < 5; i++)
		{
            MScript::CreatePart("GBOAT!!V_Adder", gate, 3, "Adder");
			MScript::OrderMoveTo(MScript::GetPartByName("Adder"), target, false);
			MScript::SetStance(MScript::GetPartByName("Adder"), US_ATTACK);
		}
		for(int j = 0; j < 2; j++)
		{
			MScript::CreatePart("GBOAT!!V_Leviathin", gate, 3, "Leviathan");
			MScript::OrderMoveTo(MScript::GetPartByName("Leviathan"), target, false);
			MScript::SetStance(MScript::GetPartByName("Leviathan"), US_ATTACK);
		}		
		MScript::CloseWormBlast(tempWormID);

	   	return false;
	}
	return true;
}

//Vyrium Attack Wave from the Right Jump Hole to Minbari
CQSCRIPTPROGRAM(Demo_VyriumWave3, Demo_VyriumWave3_Save,0);

void Demo_VyriumWave3::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#115A0#"), 1, 1, true);
}

bool Demo_VyriumWave3::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("Minbari");
		MPartRef gate = MScript::GetPartByName("#115A0#");

		MScript::FlashWormBlast(tempWormID);
		for(int i = 0; i < 5; i++)
		{
            MScript::CreatePart("GBOAT!!V_Adder", gate, 3, "Adder");
			MScript::OrderMoveTo(MScript::GetPartByName("Adder"), target, false);
			MScript::SetStance(MScript::GetPartByName("Adder"), US_ATTACK);
		}
		for(int j = 0; j < 2; j++)
		{
			MScript::CreatePart("GBOAT!!V_Leviathin", gate, 3, "Leviathan");
			MScript::OrderMoveTo(MScript::GetPartByName("Leviathan"), target, false);
			MScript::SetStance(MScript::GetPartByName("Leviathan"), US_ATTACK);
		}		
		MScript::CloseWormBlast(tempWormID);

	   	return false;
	}
	return true;
}

//Vyrium Attack Wave from the Right Jump Hole to Charon Prime
CQSCRIPTPROGRAM(Demo_VyriumWave4, Demo_VyriumWave4_Save,0);

void Demo_VyriumWave4::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#115A0#"), 1, 1, true);
}

bool Demo_VyriumWave4::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("Charon Prime");
		MPartRef gate = MScript::GetPartByName("#115A0#");

		MScript::FlashWormBlast(tempWormID);
		for(int i = 0; i < 5; i++)
		{
            MScript::CreatePart("GBOAT!!V_Adder", gate, 3, "Adder");
			MScript::OrderMoveTo(MScript::GetPartByName("Adder"), target, false);
			MScript::SetStance(MScript::GetPartByName("Adder"), US_ATTACK);
		}
		for(int j = 0; j < 2; j++)
		{
			MScript::CreatePart("GBOAT!!V_Leviathin", gate, 3, "Leviathan");
			MScript::OrderMoveTo(MScript::GetPartByName("Leviathan"), target, false);
			MScript::SetStance(MScript::GetPartByName("Leviathan"), US_ATTACK);
		}		
		MScript::CloseWormBlast(tempWormID);

	   	return false;
	}
	return true;
}

//Basalisk Attack from the Right Jump Hole
CQSCRIPTPROGRAM(Demo_VyriumBasaliskWave1, Demo_VyriumBasaliskWave1_Save,0);

void Demo_VyriumBasaliskWave1::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#115A0#"), 1, 1, true);
}

bool Demo_VyriumBasaliskWave1::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("#FED0#");
		MPartRef gate = MScript::GetPartByName("#115A0#");
		
		MScript::FlashWormBlast(tempWormID);

		for(int i = 0; i < 2; i++)
		{
			MScript::CreatePart("GBOAT!!V_Basalisk", gate, 3, "Basalisk");
			MScript::OrderSpecialAttack(MScript::GetPartByName("Basalisk"), target);
			MScript::SetStance(MScript::GetPartByName("Basalisk"), US_ATTACK);
		}

		MScript::CloseWormBlast(tempWormID);
		return false;
	}
	return true;
}

//Basalisk Attack from the Right Jump Hole
CQSCRIPTPROGRAM(Demo_VyriumBasaliskWave2, Demo_VyriumBasaliskWave2_Save,0);

void Demo_VyriumBasaliskWave2::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#115A0#"), 1, 1, true);
}

bool Demo_VyriumBasaliskWave2::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("#FEE0#");
		MPartRef gate = MScript::GetPartByName("#115A0#");
		
		MScript::FlashWormBlast(tempWormID);

		for(int i = 0; i < 2; i++)
		{
			MScript::CreatePart("GBOAT!!V_Basalisk", gate, 3, "Basalisk");
			MScript::OrderSpecialAttack(MScript::GetPartByName("Basalisk"), target);
			MScript::SetStance(MScript::GetPartByName("Basalisk"), US_ATTACK);
		}

		MScript::CloseWormBlast(tempWormID);
		return false;
	}
	return true;
}

//Basalisk Attack from the Right Jump Hole
CQSCRIPTPROGRAM(Demo_VyriumBasaliskWave3, Demo_VyriumBasaliskWave3_Save,0);

void Demo_VyriumBasaliskWave3::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#115A0#"), 1, 1, true);
}

bool Demo_VyriumBasaliskWave3::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("#FF10#");
		MPartRef gate = MScript::GetPartByName("#115A0#");
		
		MScript::FlashWormBlast(tempWormID);

		for(int i = 0; i < 2; i++)
		{
			MScript::CreatePart("GBOAT!!V_Basalisk", gate, 3, "Basalisk");
			MScript::OrderSpecialAttack(MScript::GetPartByName("Basalisk"), target);
			MScript::SetStance(MScript::GetPartByName("Basalisk"), US_ATTACK);
		}

		MScript::CloseWormBlast(tempWormID);
		return false;
	}
	return true;
}

//Basalisk Attack from the Right Jump Hole
CQSCRIPTPROGRAM(Demo_VyriumBasaliskWave4, Demo_VyriumBasaliskWave4_Save,0);

void Demo_VyriumBasaliskWave4::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#115A0#"), 1, 1, true);
}

bool Demo_VyriumBasaliskWave4::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("#FF30#");
		MPartRef gate = MScript::GetPartByName("#115A0#");
		
		MScript::FlashWormBlast(tempWormID);

		for(int i = 0; i < 2; i++)
		{
			MScript::CreatePart("GBOAT!!V_Basalisk", gate, 3, "Basalisk");
			MScript::OrderSpecialAttack(MScript::GetPartByName("Basalisk"), target);
			MScript::SetStance(MScript::GetPartByName("Basalisk"), US_ATTACK);
		}

		MScript::CloseWormBlast(tempWormID);
		return false;
	}
	return true;
}

//Basalisk Attack from the Left Jump Hole
CQSCRIPTPROGRAM(Demo_VyriumBasaliskWave5, Demo_VyriumBasaliskWave5_Save,0);

void Demo_VyriumBasaliskWave5::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#E980#"), 1, 1, true);
}

bool Demo_VyriumBasaliskWave5::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("#FEF0#");
		MPartRef gate = MScript::GetPartByName("#E980#");
		
		MScript::FlashWormBlast(tempWormID);

		for(int i = 0; i < 2; i++)
		{
			MScript::CreatePart("GBOAT!!V_Basalisk", gate, 3, "Basalisk");
			MScript::OrderSpecialAttack(MScript::GetPartByName("Basalisk"), target);
			MScript::SetStance(MScript::GetPartByName("Basalisk"), US_ATTACK);
		}

		MScript::CloseWormBlast(tempWormID);
		return false;
	}
	return true;
}

//Basalisk Attack from the Left Jump Hole
CQSCRIPTPROGRAM(Demo_VyriumBasaliskWave6, Demo_VyriumBasaliskWave6_Save,0);

void Demo_VyriumBasaliskWave6::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#E980#"), 1, 1, true);
}

bool Demo_VyriumBasaliskWave6::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("#FF00#");
		MPartRef gate = MScript::GetPartByName("#E980#");
		
		MScript::FlashWormBlast(tempWormID);

		for(int i = 0; i < 2; i++)
		{
			MScript::CreatePart("GBOAT!!V_Basalisk", gate, 3, "Basalisk");
			MScript::OrderSpecialAttack(MScript::GetPartByName("Basalisk"), target);
			MScript::SetStance(MScript::GetPartByName("Basalisk"), US_ATTACK);
		}

		MScript::CloseWormBlast(tempWormID);
		return false;
	}
	return true;
}

//Basalisk Attack from the Left Jump Hole
CQSCRIPTPROGRAM(Demo_VyriumBasaliskWave7, Demo_VyriumBasaliskWave7_Save,0);

void Demo_VyriumBasaliskWave7::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#E980#"), 1, 1, true);
}

bool Demo_VyriumBasaliskWave7::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("#FFE0#");
		MPartRef gate = MScript::GetPartByName("#E980#");
		
		MScript::FlashWormBlast(tempWormID);

		for(int i = 0; i < 2; i++)
		{
			MScript::CreatePart("GBOAT!!V_Basalisk", gate, 3, "Basalisk");
			MScript::OrderSpecialAttack(MScript::GetPartByName("Basalisk"), target);
			MScript::SetStance(MScript::GetPartByName("Basalisk"), US_ATTACK);
		}

		MScript::CloseWormBlast(tempWormID);
		return false;
	}
	return true;
}

//Crotal Attack from Left Jump Hole to Minbari
CQSCRIPTPROGRAM(Demo_VyriumCrotalWave1, Demo_VyriumCrotalWave1_Save,0);

void Demo_VyriumCrotalWave1::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#E980#"), 1, 1, true);
}

bool Demo_VyriumCrotalWave1::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("Minbari");
		MPartRef gate = MScript::GetPartByName("#E980#");
		
		MScript::FlashWormBlast(tempWormID);

		for(int i = 0; i < 3; i++)
		{
            MScript::CreatePart("GBOAT!!V_Crotal", gate, 3, "Crotal");
			MScript::OrderAttackPosition(MScript::GetPartByName("Crotal"), target, true);
			MScript::SetStance(MScript::GetPartByName("Crotal"), US_ATTACK);
		}

		MScript::CloseWormBlast(tempWormID);
	   	return false;
	}
	return true;
}

//Crotal Attack from Left Jump Hole to Charon Prime
CQSCRIPTPROGRAM(Demo_VyriumCrotalWave2, Demo_VyriumCrotalWave2_Save,0);

void Demo_VyriumCrotalWave2::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#E980#"), 1, 1, true);
}

bool Demo_VyriumCrotalWave2::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("Charon Prime");
		MPartRef gate = MScript::GetPartByName("#E980#");
		
		MScript::FlashWormBlast(tempWormID);

		for(int i = 0; i < 3; i++)
		{
            MScript::CreatePart("GBOAT!!V_Crotal", gate, 3, "Crotal");
			MScript::OrderAttackPosition(MScript::GetPartByName("Crotal"), target, true);
			MScript::SetStance(MScript::GetPartByName("Crotal"), US_ATTACK);
		}

		MScript::CloseWormBlast(tempWormID);
	   	return false;
	}
	return true;
}

//Crotal Attack from Left Jump Hole to Terran Jump Hole
CQSCRIPTPROGRAM(Demo_VyriumCrotalWave3, Demo_VyriumCrotalWave3_Save,0);

void Demo_VyriumCrotalWave3::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#E980#"), 1, 1, true);
}

bool Demo_VyriumCrotalWave3::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("#12B70#");
		MPartRef gate = MScript::GetPartByName("#E980#");
		
		MScript::FlashWormBlast(tempWormID);

		for(int i = 0; i < 3; i++)
		{
            MScript::CreatePart("GBOAT!!V_Crotal", gate, 3, "Crotal");
			MScript::OrderAttackPosition(MScript::GetPartByName("Crotal"), target, true);
			MScript::SetStance(MScript::GetPartByName("Crotal"), US_ATTACK);
		}

		MScript::CloseWormBlast(tempWormID);
	   	return false;
	}
	return true;
}

//Crotal Attack from Right Jump Hole to Urth
CQSCRIPTPROGRAM(Demo_VyriumCrotalWave4, Demo_VyriumCrotalWave4_Save,0);

void Demo_VyriumCrotalWave4::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#115A0#"), 1, 1, true);
}

bool Demo_VyriumCrotalWave4::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("Urth");
		MPartRef gate = MScript::GetPartByName("#115A0#");
		
		MScript::FlashWormBlast(tempWormID);

		for(int i = 0; i < 3; i++)
		{
            MScript::CreatePart("GBOAT!!V_Crotal", gate, 3, "Crotal");
			MScript::OrderAttackPosition(MScript::GetPartByName("Crotal"), target, true);
			MScript::SetStance(MScript::GetPartByName("Crotal"), US_ATTACK);
		}

		MScript::CloseWormBlast(tempWormID);
	   	return false;
	}
	return true;
}

//Crotal Attack from Right Jump Hole to Centauri
CQSCRIPTPROGRAM(Demo_VyriumCrotalWave5, Demo_VyriumCrotalWave5_Save,0);

void Demo_VyriumCrotalWave5::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#115A0#"), 1, 1, true);
}

bool Demo_VyriumCrotalWave5::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("Centauri");
		MPartRef gate = MScript::GetPartByName("#115A0#");
		
		MScript::FlashWormBlast(tempWormID);

		for(int i = 0; i < 3; i++)
		{
            MScript::CreatePart("GBOAT!!V_Crotal", gate, 3, "Crotal");
			MScript::OrderAttackPosition(MScript::GetPartByName("Crotal"), target, true);
			MScript::SetStance(MScript::GetPartByName("Crotal"), US_ATTACK);
		}

		MScript::CloseWormBlast(tempWormID);
	   	return false;
	}
	return true;
}

//Cobra Planet Kill from Left Jump Hole to Charon Prime, Home World
CQSCRIPTPROGRAM(Demo_VyriumCobraWave1, Demo_VyriumCobraWave1_Save,0);

void Demo_VyriumCobraWave1::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#E980#"), 1, 1, true);
}

bool Demo_VyriumCobraWave1::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("Charon Prime");
		MPartRef gate = MScript::GetPartByName("#E980#");

		MScript::FlashWormBlast(tempWormID);

		MScript::CreatePart("GBOAT!!V_Cobra_PlanetKill", gate, 3, "Cobra");
		MScript::OrderSpecialAttack(MScript::GetPartByName("Cobra"), target);
		MScript::MakeInvincible(MScript::GetPartByName("Cobra"), true);
		MScript::SetStance(MScript::GetPartByName("Cobra"), US_ATTACK);

		MScript::CloseWormBlast(tempWormID);
		return false;
	}
	return true;
}

//Cobra Planet Kill from Right Jump Hole to Minbari, Temp Home World
CQSCRIPTPROGRAM(Demo_VyriumCobraWave2, Demo_VyriumCobraWave2_Save,0);

void Demo_VyriumCobraWave2::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	tempWormID = MScript::CreateWormBlast(MScript::GetPartByName("#115A0#"), 1, 1, true);
}

bool Demo_VyriumCobraWave2::Update (void)
{	
	count = count+1;

	if (count >= 10)
	{
		MPartRef target = MScript::GetPartByName("Urth");
		MPartRef gate = MScript::GetPartByName("#115A0#");

		MScript::FlashWormBlast(tempWormID);

		MScript::CreatePart("GBOAT!!V_Cobra_PlanetKill", gate, 3, "Cobra");
		MScript::OrderSpecialAttack(MScript::GetPartByName("Cobra"), target);
		//MScript::MakeInvincible(MScript::GetPartByName("Cobra"), true);
		MScript::SetStance(MScript::GetPartByName("Cobra"), US_ATTACK);

		MScript::CloseWormBlast(tempWormID);
		return false;
	}
	return true;
}

//Initial Attack, sends Basilisks and Crotals
CQSCRIPTPROGRAM(Demo_SurpriseAttackWave1, Demo_SurpriseAttackWave1_Save,0);

void Demo_SurpriseAttackWave1::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	MScript::RunProgramByName("Demo_VyriumBasaliskWave1", MPartRef());
	MScript::RunProgramByName("Demo_VyriumBasaliskWave2", MPartRef());
	MScript::RunProgramByName("Demo_VyriumBasaliskWave3", MPartRef());
	MScript::RunProgramByName("Demo_VyriumBasaliskWave4", MPartRef());
	MScript::RunProgramByName("Demo_VyriumBasaliskWave5", MPartRef());
	MScript::RunProgramByName("Demo_VyriumBasaliskWave6", MPartRef());
	MScript::RunProgramByName("Demo_VyriumBasaliskWave7", MPartRef());

}

bool Demo_SurpriseAttackWave1::Update (void)
{	
	count = count+1;

	if (count >= 60)
	{
		MScript::RunProgramByName("Demo_VyriumCrotalWave1", MPartRef());
		MScript::RunProgramByName("Demo_VyriumCrotalWave2", MPartRef());
		MScript::RunProgramByName("Demo_VyriumWave3", MPartRef());
		MScript::RunProgramByName("Demo_SurpriseAttackWave2", MPartRef());
		return false;
	}
	return true;
}

//Major Wave Attack, sends Adders and Leviathans
CQSCRIPTPROGRAM(Demo_SurpriseAttackWave2, Demo_SurpriseAttackWave2_Save,0);

void Demo_SurpriseAttackWave2::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
}

bool Demo_SurpriseAttackWave2::Update (void)
{	
	count = count+1;

	if (count >= 60)
	{
		MScript::RunProgramByName("Demo_VyriumCrotalWave4", MPartRef());
		MScript::RunProgramByName("Demo_VyriumCrotalWave5", MPartRef());
		MScript::RunProgramByName("Demo_VyriumWave4", MPartRef());
		MScript::RunProgramByName("Demo_SurpriseAttackWave3", MPartRef());
		return false;
	}
	return true;
}

//Major Wave Attack, sends Adders and Leviathans
CQSCRIPTPROGRAM(Demo_SurpriseAttackWave3, Demo_SurpriseAttackWave3_Save,0);

void Demo_SurpriseAttackWave3::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
}

bool Demo_SurpriseAttackWave3::Update (void)
{	
	count = count+1;

	if (count >= 60)
	{
		MScript::RunProgramByName("Demo_VyriumCrotalWave3", MPartRef());
		MScript::RunProgramByName("Demo_VyriumWave2", MPartRef());
		MScript::RunProgramByName("Demo_SurpriseAttackWave4", MPartRef());
		return false;
	}
	return true;
}

//Major Wave Attack, sends Adders and Leviathans
CQSCRIPTPROGRAM(Demo_SurpriseAttackWave4, Demo_SurpriseAttackWave4_Save,0);

void Demo_SurpriseAttackWave4::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
}

bool Demo_SurpriseAttackWave4::Update (void)
{	
	count = count+1;

	if (count >= 60)
	{
		MScript::RunProgramByName("Demo_VyriumWave1", MPartRef());
		MScript::RunProgramByName("Demo_SurpriseAttackWave5", MPartRef());
		return false;
	}
	return true;
}

//Major Wave Attack, sends Adders and Leviathans
CQSCRIPTPROGRAM(Demo_SurpriseAttackWave5, Demo_SurpriseAttackWave5_Save,0);

void Demo_SurpriseAttackWave5::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
	//MScript::PlayAnimatedMessage("comm_blackwell_05.wav", "Animate!!Blackwell2", 550, 25, IDS_Demo_Obj_2);
	MScript::RunProgramByName("Demo_VyriumCobraWave1", MPartRef());
}

bool Demo_SurpriseAttackWave5::Update (void)
{	
	count = count+1;

	if (count >= 60)
	{
		MScript::RunProgramByName("Demo_VyriumCobraWave2", MPartRef());
		return false;
	}
	return true;
}

//Win Conditions for the Mission:  This monitors the trigger of the Nova Bomb
CQSCRIPTPROGRAM(Demo_WinConditions, Demo_WinConditions_Save,0);

void Demo_WinConditions::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
}

bool Demo_WinConditions::Update (void)
{	
	if(!MScript::IsDead(data.Nova_Bomb))
		if (!data.Nova_Bomb->caps.specialAbilityOk	)
		{
			data.IsWin = true;
			MScript::StopFlashUI(IDH_SPECIAL_ABILITY);
			MScript::StopAlertAnim(data.Nova_Alert);
			MScript::RunProgramByName("Demo_WinConditions2", MPartRef());
			MScript::RunProgramByName("Demo_WinConditions3_Audio", MPartRef());
			return false;
		}
	return true;
	//return false;
}

//Win Conditions for the Mission:  This monitors the actual destruction of the Nova Bomb
CQSCRIPTPROGRAM(Demo_WinConditions2, Demo_WinConditions2_Save,0);

void Demo_WinConditions2::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
}

bool Demo_WinConditions2::Update (void)
{	
	count+=ELAPSED_TIME;

	if(count >= 22)
	{
		MScript::PauseGame(true);
		MScript::PlayFullScreenVideo("boom.bik");
		MScript::EndMissionSplash("VFXShape!!Splash",10000,true);
		return false;
	}return true;
	/*

	if(MScript::IsDead(data.Nova_Bomb))
		{
			MScript::RunProgramByName("Demo_CountdownTimer", MPartRef());
			return false;
		}
	return true;
	//return false;*/

}

//Win Conditions for the Mission:  This plays the final audio cue
CQSCRIPTPROGRAM(Demo_WinConditions3_Audio, Demo_WinConditions3_Audio_Save,0);

void Demo_WinConditions3_Audio::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
}

bool Demo_WinConditions3_Audio::Update (void)
{	
	count = count + 1;
	if(count >= 15)
		{
			MScript::PlayAnimatedMessage("comm_blackwell_06.wav", "Animate!!Blackwell2", 550, 25, IDS_Demo_Alert_1);
			return false;
		}
	return true;
	//return false;
}

//Countdown Timer for the Mission
CQSCRIPTPROGRAM(Demo_CountdownTimer, Demo_CountdownTimer_Save,0);

void Demo_CountdownTimer::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
}

bool Demo_CountdownTimer::Update (void)
{	
	count = count + 1;
    if (count >= 30)
	{
		MScript::EndMissionSplash("VFXShape!!Splash",10000,true);
		return false;
	}
	return true;
}

//Defeat Conditions for the Mission
CQSCRIPTPROGRAM(Demo_DefeatConditions, Demo_DefeatConditions_Save,0);

void Demo_DefeatConditions::Initialize (U32 eventFlags, const MPartRef & part)
{
	count = 0;
}

bool Demo_DefeatConditions::Update (void)
{	
	if (data.IsWin == true)
		return false;

	if ((MScript::IsDead(data.Cel_Plat_1)&&
		 MScript::IsDead(data.Cel_Plat_2)&&
		 MScript::IsDead(data.Cel_Plat_3)&&
		 MScript::IsDead(data.Cel_Plat_4)&&
		 MScript::IsDead(data.Cel_Plat_5)&&
		 MScript::IsDead(data.Cel_Plat_6)&&
		 MScript::IsDead(data.Cel_Plat_7)) ||
//		(MScript::IsDead(data.HQ) &&
//		 MScript::IsDead(data.Temp_HQ)) ||
		 MScript::IsDead(data.Nova_Bomb)
		 )
	{
			MScript::EndMissionDefeat();
			return false;
	}
	return true;
}

CQSCRIPTPROGRAM(Demo_TestGrid, Demo_TestGrid_Save, CQPROGFLAG_HOTKEY_1);

void Demo_TestGrid::Initialize (U32 eventFlags, const MPartRef & part)
{
	for(U32 x = 0; x < 640; x = x + 10)
		MScript::DrawLine(x, 0, x, 480, RGB(63, 199, 208), 20000, 2000);
	for(U32 y = 0; y < 480; y = y + 10)
        MScript::DrawLine(0, y, 640, y, RGB(63, 199, 208), 20000, 2000);
}

bool Demo_TestGrid::Update (void)
{	

	return true;
}

CQSCRIPTPROGRAM(Demo_TestGrid2, Demo_TestGrid2_Save, CQPROGFLAG_HOTKEY_2);

void Demo_TestGrid2::Initialize (U32 eventFlags, const MPartRef & part)
{

	MScript::DrawLine(0, 15, 640, 15, RGB(63, 199, 208), 20000, 2000);
	MScript::DrawLine(0, 320, 640, 320, RGB(63, 199, 208), 20000, 2000);
	MScript::DrawLine(0, 330, 640, 330, RGB(63, 199, 208), 20000, 2000);
	MScript::DrawLine(0, 350, 640, 350, RGB(63, 199, 208), 20000, 2000);
	MScript::DrawLine(0, 355, 640, 355, RGB(63, 199, 208), 20000, 2000);

    MScript::DrawLine(170, 0, 170, 480, RGB(63, 199, 208), 20000, 2000);
    MScript::DrawLine(230, 0, 230, 480, RGB(63, 199, 208), 20000, 2000);
    MScript::DrawLine(310, 0, 310, 480, RGB(63, 199, 208), 20000, 2000);
    MScript::DrawLine(340, 0, 340, 480, RGB(63, 199, 208), 20000, 2000);
	MScript::DrawLine(342, 0, 342, 480, RGB(63, 199, 208), 20000, 2000);
    MScript::DrawLine(495, 0, 495, 480, RGB(63, 199, 208), 20000, 2000);
}

bool Demo_TestGrid2::Update (void)
{	

	return true;
}

CQSCRIPTPROGRAM(Demo_TestGrid3, Demo_TestGrid3_Save, CQPROGFLAG_HOTKEY_3);

void Demo_TestGrid3::Initialize (U32 eventFlags, const MPartRef & part)
{

	MScript::DrawLine(1, 355, 230, 355, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(1, 479, 230, 479, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(1, 355, 1, 479, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(230, 355, 230, 479, RGB(63, 199, 208), 20000, 0);

	MScript::DrawLine(230, 355, 310, 355, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(230, 479, 310, 479, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(230, 355, 230, 479, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(310, 355, 310, 479, RGB(63, 199, 208), 20000, 0);

	MScript::DrawLine(310, 350, 310, 479, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(342, 350, 342, 479, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(310, 350, 342, 350, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(310, 479, 342, 479, RGB(63, 199, 208), 20000, 0);

	MScript::DrawLine(340, 320, 340, 479, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(495, 320, 495, 479, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(340, 320, 495, 320, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(340, 479, 495, 479, RGB(63, 199, 208), 20000, 0);

	MScript::DrawLine(495, 330, 495, 479, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(639, 330, 639, 479, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(495, 330, 639, 330, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(495, 479, 639, 479, RGB(63, 199, 208), 20000, 0);

	MScript::DrawLine(170, 15, 170, 1, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(639, 15, 639, 1, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(170, 15, 639, 15, RGB(63, 199, 208), 20000, 0);
	MScript::DrawLine(170, 1, 639, 1, RGB(63, 199, 208), 20000, 0);

/*

	//Horizontal Lines
	MScript::DrawLine(0, 15, 640, 15, RGB(63, 199, 208), 20000, 2000);
	MScript::DrawLine(0, 320, 640, 320, RGB(63, 199, 208), 20000, 2000);
	MScript::DrawLine(0, 330, 640, 330, RGB(63, 199, 208), 20000, 2000);
	MScript::DrawLine(0, 350, 640, 350, RGB(63, 199, 208), 20000, 2000);
	MScript::DrawLine(0, 355, 640, 355, RGB(63, 199, 208), 20000, 2000);

	//Vertical Lines
    MScript::DrawLine(170, 0, 170, 480, RGB(63, 199, 208), 20000, 2000);
    MScript::DrawLine(230, 0, 230, 480, RGB(63, 199, 208), 20000, 2000);
    MScript::DrawLine(310, 0, 310, 480, RGB(63, 199, 208), 20000, 2000);
    MScript::DrawLine(340, 0, 340, 480, RGB(63, 199, 208), 20000, 2000);
	MScript::DrawLine(342, 0, 342, 480, RGB(63, 199, 208), 20000, 2000);
    MScript::DrawLine(495, 0, 495, 480, RGB(63, 199, 208), 20000, 2000);
	*/
}

bool Demo_TestGrid3::Update (void)
{	

	return true;
}

CQSCRIPTPROGRAM(Demo_Camera1, Demo_Camera1_Save, CQPROGFLAG_HOTKEY_4);

void Demo_Camera1::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript::EnableMovieMode(true);

	MScript::PlayAudio("bcast_01.wav");

	MScript::ClearCameraQueue();

	MScript::ChangeCamera (MScript::GetPartByName("Planet_1"), 7, MOVIE_CAMERA_SLIDE_TO);
	MScript::ChangeCamera (MScript::GetPartByName("Planet_2"), 8, MOVIE_CAMERA_QUEUE);
	MScript::ChangeCamera (MScript::GetPartByName("Planet_3"), 8, MOVIE_CAMERA_QUEUE);
	MScript::ChangeCamera (MScript::GetPartByName("Planet_4"), 8, MOVIE_CAMERA_QUEUE);
	MScript::ChangeCamera (MScript::GetPartByName("Start_Camera"), 8, MOVIE_CAMERA_QUEUE);

}

bool Demo_Camera1::Update (void)
{	return true;}

CQSCRIPTPROGRAM(Demo_Camera2, Demo_Camera2_Save, CQPROGFLAG_HOTKEY_5);

void Demo_Camera2::Initialize (U32 eventFlags, const MPartRef & part)
{
	MScript::EnableMovieMode(true);

	MScript::PlayAnimatedMessage("comm_blackwell_01.wav", "Animate!!Blackwell2", 550, 25, IDS_Demo_Obj_1);
	
	MScript::ChangeCamera (MScript::GetPartByName("Start_Camera"), 5, MOVIE_CAMERA_SLIDE_TO);
	MScript::ChangeCamera (MScript::GetPartByName("Hybrid_Close_1"), 6, MOVIE_CAMERA_QUEUE);
	MScript::ChangeCamera (MScript::GetPartByName("Hybrid_Close_2"), 6, MOVIE_CAMERA_QUEUE);
	MScript::ChangeCamera (MScript::GetPartByName("Fleet_Camera_1"), 6, MOVIE_CAMERA_QUEUE);
	MScript::ChangeCamera (MScript::GetPartByName("Cel_Base"), 5, MOVIE_CAMERA_QUEUE | MOVIE_CAMERA_JUMP_TO);
	MScript::ChangeCamera (MScript::GetPartByName("Cel_Planet_1"), 5, MOVIE_CAMERA_QUEUE);
	MScript::ChangeCamera (MScript::GetPartByName("Cel_Planet_2"), 5, MOVIE_CAMERA_QUEUE);
	MScript::ChangeCamera (MScript::GetPartByName("Cel_Base"), 5, MOVIE_CAMERA_QUEUE);
	MScript::ChangeCamera (MScript::GetPartByName("Fleet_Camera_1"), 4, MOVIE_CAMERA_QUEUE | MOVIE_CAMERA_JUMP_TO);
	MScript::ChangeCamera (MScript::GetPartByName("Start_Camera"), 4, MOVIE_CAMERA_QUEUE);
}

bool Demo_Camera2::Update (void)
{	return true;}
//--------------------------------------------------------------------------//
//--Test Scripts-------------------------------------------------------//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//--End ScriptTest.cpp-------------------------------------------------------//

