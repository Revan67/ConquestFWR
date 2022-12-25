//--------------------------------------------------------------------------//
//                                                                          //
//                              TestScript.cpp                              //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

   $Header: /Conquest/App/Src/TestScript.cpp 11    11/08/00 3:20p Jasony $

*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>
#include "CQTrace.h"

#include "CQGame.h"
#include "UserDefaults.h"
#include "Mission.h"
#include "MScroll.h"
#include "InProgressAnim.h"

#include <HeapObj.h>
#include <FileSys.h>
#include <EventSys.h>
#include <TSmartPointer.h>

using namespace CQGAMETYPES;

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
void __stdcall EnableLogging (const FULLCQGAME & cqgame, U32 seed)
{
	DAFILEDESC fdesc = "cqnet.log";
	DWORD dwWritten;
//	FULLCQGAME * _cqgame = const_cast<FULLCQGAME *>(&cqgame);

	if (NETOUTPUT != 0)
	{
		NETOUTPUT->Release();
		NETOUTPUT = 0;
	}
	else
	{
		AddToGlobalCleanupList(&NETOUTPUT);
	}

	fdesc.dwDesiredAccess |= GENERIC_WRITE;
	fdesc.dwShareMode = 0;
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	fdesc.lpImplementation = "DOS";

	if (DACOM->CreateInstance(&fdesc, (void **)&NETOUTPUT) != GR_OK)
		CQBOMB1("Could not create network log file '%s'", fdesc.lpFileName);

	NETOUTPUT->WriteFile(0, &seed, sizeof(seed), &dwWritten, 0);
//	U32 oldVersion = cqgame.version;
//	_cqgame->version = GetBuildVersion();
	NETOUTPUT->WriteFile(0, &cqgame, sizeof(FULLCQGAME), &dwWritten, 0);
//	_cqgame->version = oldVersion;
	CQASSERT(dwWritten == sizeof(FULLCQGAME));
}
//------------------------------------------------------------------------------//
//------------------------------------------------------------------------------//
//
void __stdcall RunTestScript (const char *cmd_line_script)
{
	FULLCQGAME cqgame;
	bool bClient = (stricmp(cmd_line_script, "client")==0);
	DAFILEDESC fdesc = "cqnet.log";
	U32 seed=5050;

	memset(&cqgame, 0, sizeof(cqgame));
	cqgame.activeSlots = MAX_PLAYERS;
	cqgame.bHostBusy = 1;
	cqgame.localSlot = 0;
	cqgame.slot[0].type = COMPUTER;
	cqgame.slot[0].state = READY;
	cqgame.slot[0].race = TERRAN;
	cqgame.slot[0].color = YELLOW;
	cqgame.slot[0].team = NOTEAM;
	cqgame.slot[0].dpid = 0;

	cqgame.slot[1].type = COMPUTER;
	cqgame.slot[1].state = READY;
	cqgame.slot[1].race = TERRAN;
	cqgame.slot[1].color = RED;
	cqgame.slot[1].team = NOTEAM;
	cqgame.slot[1].dpid = 0;

	const U32 mversion = GetBuildVersion();

	if (DEFAULTS->GetDataFromRegistry("LobbyOptions", static_cast<CQGAME *>(&cqgame), sizeof(CQGAME)) != sizeof(CQGAME) ||
		cqgame.version != mversion)
	{
		cqgame.version = mversion;
		cqgame.gameType = KILL_UNITS;
//		cqgame.gameSpeed = NORMAL;
		cqgame.gameSpeed = 0;
		cqgame.money = MEDIUM_MONEY;
		cqgame.mapSize = SMALL_MAP;
		cqgame.terrain = LIGHT_TERRAIN;
		cqgame.units = UNITS_MINIMAL;
	}

	if (bClient)
	{
		DWORD dwRead;

		if (DACOM->CreateInstance(&fdesc, (void **)&NETOUTPUT) != GR_OK)
			CQBOMB1("Could not open network log file '%s'", fdesc.lpFileName);

		NETOUTPUT->ReadFile(0, &seed, sizeof(seed), &dwRead, 0);
		NETOUTPUT->ReadFile(0, &cqgame, sizeof(cqgame), &dwRead, 0);

		CQASSERT(dwRead == sizeof(cqgame));

		if (cqgame.version != mversion)
			CQBOMB0("Cannot use log file, CQGAME structure has changed.");

		DEFAULTS->SetDataInRegistry("LobbyOptions", static_cast<CQGAME *>(&cqgame), sizeof(CQGAME));

		CQFLAGS.bClientPlayback = 1;
	}
	else
	{
		DWORD dwWritten;

		fdesc.dwDesiredAccess |= GENERIC_WRITE;
		fdesc.dwShareMode = 0;
		fdesc.dwCreationDistribution = CREATE_ALWAYS;
		fdesc.lpImplementation = "DOS";

		if (DACOM->CreateInstance(&fdesc, (void **)&NETOUTPUT) != GR_OK)
			CQBOMB1("Could not create network log file '%s'", fdesc.lpFileName);

		NETOUTPUT->WriteFile(0, &seed, sizeof(seed), &dwWritten, 0);
		NETOUTPUT->WriteFile(0, &cqgame, sizeof(cqgame), &dwWritten, 0);
		CQASSERT(dwWritten == sizeof(cqgame));
	}

	AddToGlobalCleanupList(&NETOUTPUT);

	ChangeInterfaceRes(IR_IN_GAME_RESOLUTION);

	COMPTR<IPANIM> ipAnim;
	MISSION->StartProgressAnim(ipAnim);
	
	if (cqgame.mapType == RANDOM_MAP)
		MISSION->GenerateMultiplayerMap(cqgame, seed);
	else
	{
		char buffer[256];
		COMPTR<IFileSystem> missionDir;
		_localWideToAnsi(cqgame.szMapName, buffer, sizeof(buffer));
		missionDir = (cqgame.mapType==SELECTED_MAP)  ? MPMAPDIR : SAVEDIR;

		if (MISSION->Load(buffer,missionDir) == 0)  // if load failed
			CQBOMB1("Load of mission file '%s' failed.", buffer);
	}

	MISSION->InitializeNetGame(cqgame, seed);
	DEFAULTS->GetDefaults()->gameSpeed = cqgame.gameSpeed;

	CQFLAGS.bGameActive = 1;
	EVENTSYS->Send(CQE_GAME_ACTIVE, (void*)1);
	MSCROLL->SetActive(1);

	ipAnim.free();
	CQDoModal(0);
}
//------------------------------------------------------------------------------//
//-----------------------------End Testscript.cpp-------------------------------//
//------------------------------------------------------------------------------//

