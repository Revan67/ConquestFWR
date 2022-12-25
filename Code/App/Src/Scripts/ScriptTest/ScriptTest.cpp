//--------------------------------------------------------------------------//
//                                                                          //
//                                Script00T.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   TEST MISSION

   Created:  JeffP  4/18/00

*/
//--------------------------------------------------------------------------//

#include <ScriptDef.h>
#include <DLLScriptMain.h>
#include "DataDef.h"
#include "Local.h"
#include "stdlib.h"

#include "..\helper\helper.h"

//--------------------------------------------------------------------------//
//  DEFINES																	//
//--------------------------------------------------------------------------//


//--------------------------------------------------------------------------//
//  MISSION PROGRAM 														//
//--------------------------------------------------------------------------//

CQSCRIPTDATA(MissionData, TestMission_Data);

CQSCRIPTPROGRAM(TestMission_Start, TestMission_Start_Data, CQPROGFLAG_STARTMISSION);

void TestMission_Start::Initialize (U32 eventFlags, const MPartRef & part)
{
	TestMission_Data.state = Begin;
}

bool TestMission_Start::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//  OTHER PROGRAMS
//--------------------------------------------------------------------------//

CQSCRIPTPROGRAM(MissionWin, MissionWin_Data, 0);

void MissionWin::Initialize (U32 eventFlags, const MPartRef & part)
{
	TestMission_Data.state = Done;
	M_ EndMissionVictory();
}

bool MissionWin::Update (void)
{
	return false;
}

//--------------------------------------------------------------------------//
//--End ScriptTest.cpp-------------------------------------------------------//

