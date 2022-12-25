#ifndef DATADEF_H
#define DATADEF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DataDef.h                                 //
//                                                                          //
//               COPYRIGHT (C) 2004 Warthog Texas, INC.                     //
//                                                                          //
//--------------------------------------------------------------------------//


//--------------------------------------------------------------------------//

#ifndef US_TYPEDEFS
#include <typedefs.h>
#endif

#ifndef MPARTREF_H
#include <MPartRef.h>
#endif

#ifndef DMTECHNODE_H
#include <..\..\dinclude\DMTechNode.h>
#endif


//--------------------------------------------------------------------------//
//  MISSION DATA
//--------------------------------------------------------------------------//

enum MissionState
{
	Begin,
    Briefing,
	MissionStart,
	Done
};

struct MissionData
{
	TECHNODE missionTech;

	bool IsWin;

	MissionState mission_state;

	MPartRef Cel_Plat_1;
	MPartRef Cel_Plat_2;
	MPartRef Cel_Plat_3;
	MPartRef Cel_Plat_4;
	MPartRef Cel_Plat_5;
	MPartRef Cel_Plat_6;
	MPartRef Cel_Plat_7;

	MPartRef Nova_Bomb;
	MPartRef HQ;
	MPartRef Temp_HQ;

	U32 Procyo_check;
	U32 Nova_Alert;
	U32 Mantis_WP_Alert;
	U32 Celareon_WP_Alert;


};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct Demo_Briefing_Data
{
	int count;
	CQBRIEFINGITEM empty_anim;
	U32 Stage0_1Var;
	U32 Stage0_2Var;
	U32 Stage1Var;
	U32 Stage1_5Var;
	U32 Stage1_6Var;
	U32 Stage1_7Var;
	U32 Stage2Var;
	U32 Stage2_5Var;
	U32 Stage3Var;
	U32 Stage4Var;

	U32 streamID;

	enum
	{
		Begin,
		Stage0_1,
		Stage0_2,
		Stage1,
		Stage1_5,
		Stage1_6,
		Stage1_7,
		Stage2,
		Stage2_5,
		Stage3,
		Stage4,
		Finish
	} state;
};

struct MyProg_Save
{
	int count;
	U32 TT;
	U32 PreStream1;
	U32 PreStream2;
	U32 Stream1;
	U32 Stream2;
	U32 Stream3;
	U32 Stream4;
	U32 Stream5;
	U32 Stream6;
	U32 Stream7;
	U32 Stream8;
	U32 Stream9;
	U32 Stream10;
	U32 StreamB;
	U32 Alert;

	enum
	{
		Begin,
		PreBriefing1,
		PreBriefing2,
		Briefing,
		Teletype,
		VyriumGoto,
		VyriumAttack,
		Tutor0,
		Tutor1,
		Tutor2,
		Tutor3,
		Tutor4,
		Tutor5,
		Tutor6,
		Tutor7,
		Tutor8,
		Tutor9,
		Tutor10,
		Finish
	} state;
};

struct Demo_DetectInvincible_Save
{
	MPartRef target;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_DetectFormFormation_Save
{
	MPartRef target;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_DetectConvoyFormation_Save
{
	MPartRef target;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_DetectHalseyFormation_Save
{
	MPartRef target;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_DetectDefenseFormation_Save
{
	MPartRef target;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_MantisAttack_Save
{
	MPartRef target;
	int count;
	U32 Stream1;
	U32 Stream2;
	U32 Stream3;
	U32 Stream4;
	U32 Stream5;
	U32 Stream6;
	U32 Stream7;
	U32 Stream8;
	U32 Stream9;
	U32 StreamB;
	U32 Alert;

	enum
	{
		Begin,
		Tutor0,
		Tutor0_5,
		Tutor1,
		Tutor2,
		Tutor3,
		Tutor4,
		Tutor5,
		Tutor6,
		Tutor7,
		Tutor8,
		Tutor9,
		Finish
	} state;
};

struct Demo_ConvoyComm_Save
{
	MPartRef target;
	U32 Stream1;
	U32 Stream2;
	U32 StreamB;
	U32 Alert;

	enum
	{
		Begin,
		Tutor0,
		Tutor1,
		Finish
	} state;
};

struct Demo_BasiliskAttack_Save
{
	MPartRef target;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_BasiliskAttack2_Save
{
	MPartRef target;
	int count;
	U32 Stream1;
	U32 Stream2;

	enum
	{
		Begin,
		Tutor1,
		Finish
	} state;
};

struct Demo_SurpriseAttack_Save
{
	MPartRef target;
	int count;
	U32 Stream1;
	U32 StreamB;

	enum
	{
		Time,
		Begin,
		Tutor0,
		Tutor1,
		Finish
	} state;
};

struct Demo_VyriumWave1_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumWave2_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumWave3_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumWave4_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumBasaliskWave1_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumBasaliskWave2_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumBasaliskWave3_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumBasaliskWave4_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumBasaliskWave5_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumBasaliskWave6_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumBasaliskWave7_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumBasaliskWave8_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumCrotalWave1_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumCrotalWave2_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumCrotalWave3_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumCrotalWave4_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumCrotalWave5_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumCobraWave1_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumCobraWave2_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_BasiliskWarning_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_HQDestroyed_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;
	U32 Stream1;
	U32 StreamB;

	enum
	{
		Begin,
		Tutor0,
		Tutor1,
		Finish
	} state;
};

struct Demo_SurpriseAttackWave1_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_SurpriseAttackWave2_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_SurpriseAttackWave3_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_SurpriseAttackWave4_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_SurpriseAttackWave5_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_WinConditions_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_WinConditions2_Save
{
	MPartRef target;
	U32 tempWormID;
	SINGLE count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_WinConditions3_Audio_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_DefeatConditions_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_CountdownTimer_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_TestGrid_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_TestGrid2_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_TestGrid3_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_Camera1_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_Camera2_Save
{
	MPartRef target;
	U32 tempWormID;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};
//--------------------------------------------------------------------------//
//--------------------------------End DataDef.h-----------------------------//
//--------------------------------------------------------------------------//
#endif