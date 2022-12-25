#ifndef MUSICMANAGER_H
#define MUSICMANAGER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             MusicManager.h                               //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

   $Header: /Conquest/App/Src/MusicManager.h 7     4/18/01 8:58a Tmauer $

   High level music player. Streams .WAV files in the background.
*/
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

struct IMusicManager : IDAComponent
{
	virtual void __stdcall Enable (BOOL32 bEnable) = 0;

	virtual void __stdcall SetVolume (SINGLE volume) = 0;

	virtual void __stdcall Load (struct IFileSystem * inFile) = 0;

	virtual void __stdcall LoadFinish () = 0;
	
	virtual void __stdcall Save (struct IFileSystem * outFile) = 0;

	virtual void __stdcall InitPlaylist (enum M_RACE race) = 0;

	virtual void __stdcall PlayMusic (const char * fileName, bool bSmoothTransition=true) = 0;

	virtual void __stdcall GetMusicFileName (struct M_STRING & string) = 0;
};
















//--------------------------------------------------------------------------//
//--------------------------END MusicManager.h------------------------------//
//--------------------------------------------------------------------------//
#endif