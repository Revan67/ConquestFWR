//--------------------------------------------------------------------------//
//                                                                          //
//                             SoundManager.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/SplitDir.cpp 1     7/18/01 2:44p Tmauer $

   Manager the volume level of various sounds in the game
*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "ISplitDir.h"

struct DirNode
{
	DirNode * next;
	COMPTR<IFileSystem> fileSystem;
};

//--------------------------------------------------------------------------//
//
struct SplitDir: public ISplitDir
{
	virtual void AddDir (IFileSystem * fileSys);
	
	virtual void Empty ();

	virtual void CreateInstance(DACOMDESC *descriptor, void **instance);

	virtual ISearchPath * GetSearchPath();
};

//--------------------------------------------------------------------------//
//
struct _soundman : GlobalComponent
{
	SoundMan * soundMan;

	virtual void Startup (void)
	{
		SOUNDMANAGER = soundMan = new DAComponent<SoundMan>;
		AddToGlobalCleanupList((IDAComponent **) &SOUNDMANAGER);
	}

	virtual void Initialize (void)
	{
		soundMan->init();
	}
};

static _soundman soundman;

//------------------------------------------------------------------------------------//
//-----------------------------End SoundManager.cpp-----------------------------------//
//------------------------------------------------------------------------------------//
