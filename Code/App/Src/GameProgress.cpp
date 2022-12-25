//--------------------------------------------------------------------------//
//                                                                          //
//                      GameProgress.cpp                                    //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//																			//
//	NOTE:  Ignore stupid stl warnings                                       //
//--------------------------------------------------------------------------//
/*
   $Author: Sbarton $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "IGameProgress.h"
#include "Startup.h"
#include "UserDefaults.h"

#include <FileSys.h>
#include <TSmartPointer.h>

#define NAME_REG_KEY	"CQPlayerName"
#define GPF_FILE		"player.gpf"

static bool g_bForcedIntroMovie = false;
static U32  g_tempMissionsCompleted = 0;

struct PROGRESS_DATA
{
	U32 missionsCompleted;
	U32 moviesSeen;
	U32 missionsSeen;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE GameProgress : public IGameProgress
{
	BEGIN_DACOM_MAP_INBOUND(GameProgress)
	DACOM_INTERFACE_ENTRY(IGameProgress)
	END_DACOM_MAP()

	COMPTR<IFileSystem> fileSystem;
	PROGRESS_DATA progressData;
	char szPlayerName[128];

	GameProgress(void)
	{
		init();
	}

	~GameProgress(void);
	
    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IGameProgress methods */

	virtual U32  GetMissionsCompleted (void);

	virtual void SetMissionCompleted (U32 missionID);

	virtual void SetMissionsSeen (U32 missionID);

	virtual U32  GetMissionsSeen (void);

	virtual U32  GetMoviesSeen (void);

	virtual void SetMovieSeen (U32 movieID);

	virtual void ReInitialize (void);

	virtual void ForcedIntroMovie (void)
	{
		g_bForcedIntroMovie = true;
	}

	virtual void SetTempMissionsCompleted (U32 missionID);

	/* GameProgress methods */

	void init (void);

	bool loadGameProgress (void);

	bool saveGameProgress (void);

	bool setFileSystem (void);

	IDAComponent * GetBase (void)
	{
		return (IGameProgress*) this;
	}
};
//--------------------------------------------------------------------------//
//
GameProgress::~GameProgress(void)
{
}
//----------------------------------------------------------------------------
//
void GameProgress::init (void)
{
	memset(&progressData, 0, sizeof(PROGRESS_DATA));
	g_tempMissionsCompleted = 0;
}
//----------------------------------------------------------------------------
//
bool GameProgress::setFileSystem (void)
{
	char buffer[128];

	U32 res = DEFAULTS->GetStringFromRegistry(NAME_REG_KEY, szPlayerName, sizeof(szPlayerName));

	if (res && szPlayerName[0])
	{
		wsprintf(buffer, "SavedGame\\%s", szPlayerName);
	
		DAFILEDESC fdesc = buffer;
		
		if (DACOM->CreateInstance(&fdesc, fileSystem) != GR_OK)
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}
//--------------------------------------------------------------------------//
//
bool GameProgress::loadGameProgress (void)
{
	if (setFileSystem() == false)
	{
		CQBOMB0("Could not create the file system in GameProgress");
		return false;
	}

	// open up the file for reading
	if (fileSystem)
	{
		char szFile[128];
		DAFILEDESC fdesc;
		COMPTR<IFileSystem> file;
	
		DWORD dwRead;

		wsprintf(szFile, GPF_FILE);
		fdesc.lpFileName = szFile;
		fdesc.lpImplementation = "DOS";
		fdesc.dwDesiredAccess = GENERIC_READ;
		fdesc.dwShareMode = 0;  // no sharing
		fdesc.dwCreationDistribution = OPEN_EXISTING;

		if (fileSystem->CreateInstance(&fdesc, file) != GR_OK)
		{
			// will fail the first time we play our game
			return false;
		}

		file->ReadFile(0, &progressData, sizeof(progressData), &dwRead, 0);
	}
	return true;
}
//--------------------------------------------------------------------------//
//
bool GameProgress::saveGameProgress (void)
{
	if (setFileSystem() == false)
	{
		CQBOMB0("Could not create the file system in GameProgress");
		return false;
	}

	// save off our structure into a file, create the file if it is not there
	if (fileSystem)
	{
		char szFile[128];
		DAFILEDESC fdesc;
		COMPTR<IFileSystem> file;
	
		DWORD dwWritten;

		wsprintf(szFile, GPF_FILE);
		fdesc.lpFileName = szFile;
		fdesc.lpImplementation = "DOS";
		fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
		fdesc.dwShareMode = 0;  // no sharing
		fdesc.dwCreationDistribution = CREATE_ALWAYS;

		if (fileSystem->CreateInstance(&fdesc, file) != GR_OK)
		{
			CQBOMB0("Could not create save file in GameProgress");
			return false;
		}

		file->WriteFile(0, &progressData, sizeof(progressData), &dwWritten, 0);
	}

	return true;
}
//--------------------------------------------------------------------------//
//
U32 GameProgress::GetMissionsCompleted (void)
{
	// the player may have changed names
	setFileSystem();
	
	// open the file for reading purposes
	// create the file if it is not there and set it's value to zero

	// there should be only one file of the type in the SAVEDIR directory

	// actually - we should just return a bitfield telling us what we've seen
	if (loadGameProgress() == false)
	{
		// try to create the file and save off whatever we have in progressData
		saveGameProgress();
	}
	
	return progressData.missionsCompleted | g_tempMissionsCompleted;
}
//--------------------------------------------------------------------------//
//
void GameProgress::SetMissionCompleted (U32 missionID)
{
	// set the missionID'th bit and save off
	progressData.missionsCompleted |= 1 << missionID;

	saveGameProgress();
}
//--------------------------------------------------------------------------//
//
void GameProgress::SetTempMissionsCompleted (U32 missionID)
{
	g_tempMissionsCompleted |= 1 << missionID;
}
//--------------------------------------------------------------------------//
//
void GameProgress::SetMissionsSeen (U32 missionID)
{
	progressData.missionsSeen |= 1 << missionID;

	saveGameProgress();
}
//--------------------------------------------------------------------------//
//
U32 GameProgress::GetMissionsSeen (void)
{
	// the player may have changed names
	setFileSystem();
	
	// open the file for reading purposes
	// create the file if it is not there and set it's value to zero

	// there should be only one file of the type in the SAVEDIR directory

	// actually - we should just return a bitfield telling us what we've seen
	if (loadGameProgress() == false)
	{
		// try to create the file and save off whatever we have in progressData
		saveGameProgress();
	}
	
	return progressData.missionsSeen;
}
//--------------------------------------------------------------------------//
//
U32 GameProgress::GetMoviesSeen (void)
{
	//  just return a bitfield telling us what we've seen
	if (loadGameProgress() == false)
	{
		// try to create the file and save off whatever we have in progressData
		saveGameProgress();
	}
	
	if (g_bForcedIntroMovie)
	{
		U32 moviebits = progressData.moviesSeen;
		moviebits |= 1;
		return moviebits;
	}

	return progressData.moviesSeen;
}
//--------------------------------------------------------------------------//
//
void GameProgress::SetMovieSeen (U32 movieID)
{
	// set the movieID'th bit and save off
	progressData.moviesSeen |= 1 << movieID;

	saveGameProgress();
}
//--------------------------------------------------------------------------//
//
void GameProgress::ReInitialize (void)
{
	init();
	loadGameProgress();
}
//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//
struct _gameProgress : GlobalComponent
{
	virtual void Startup (void)
	{
		GAMEPROGRESS = new DAComponent<GameProgress>;
		AddToGlobalCleanupList((IDAComponent **) &GAMEPROGRESS);
	}

	virtual void Initialize (void)
	{
	}
};

static _gameProgress startup;

//--------------------------------------------------------------------------//
//--------------------------End ScrollingText.cpp---------------------------//
//--------------------------------------------------------------------------//