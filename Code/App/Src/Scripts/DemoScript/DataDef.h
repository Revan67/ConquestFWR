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

	MissionState mission_state;
};

//--------------------------------------------------------------------------//
//  PROGRAM DATA
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//

struct MyProg_Save
{

	enum
	{
		Begin,
		VyriumGoto,
		VyriumAttack,
		Finish
	} state;
};

struct Demo_SetTriggers_Save
{

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_MantisAttack_Save
{
	MPartRef target;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_LeviathanAttack_Save
{
	MPartRef target;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_Crotal1Attack_Save
{
	MPartRef target;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_Crotal2Attack_Save
{
	MPartRef target;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_Crotal3Attack_Save
{
	MPartRef target;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_Crotal4Attack_Save
{

	MPartRef target;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_Adder1Attack_Save
{
	MPartRef target;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_Adder2Attack_Save
{
	MPartRef target;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_Adder3Attack_Save
{
	MPartRef target;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_Adder4Attack_Save
{

	MPartRef target;
	int count;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_BasiliskAttack_Save
{
	MPartRef target;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_PlanetInitiate_Save
{
	MPartRef target;

	SINGLE timer;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_PlanetAttack_Save
{
	MPartRef target;

	enum
	{
		Begin,
		Finish
	} state;
};


struct Demo_PlanetTerraform_Save
{
	MPartRef target;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumWave1_Save
{
	MPartRef target;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_VyriumWave2_Save
{
	MPartRef target;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_EndGame_Save
{
	MPartRef target;

	enum
	{
		Begin,
		Finish
	} state;
};

struct Demo_PlanetDestroy_Save
{
	MPartRef target;

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