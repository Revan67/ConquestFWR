#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              SoundManager.h                              //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/SoundManager.h 21    10/23/00 10:51p Jasony $

   High level audio player. Streams .WAV files in the background.
   Handles volume controls, and other inconvenient details of stream management
*/
//--------------------------------------------------------------------------//
//
/*
	Documentation would go here.			
*/

#ifndef DACOM_H
#include <DACOM.h>
#endif


struct ISoundManager : IDAComponent
{
	// parent == 0 means get sound from SPEECH directory
	virtual U32 PlayCommMessage (const char * filename, struct IFileSystem * parent = 0, IBaseObject* obj = NULL, SINGLE volume = 1.0) = 0;

	virtual U32 PlayChatMessage (const char * filename, struct IFileSystem * parent, SINGLE volume = 1.0) = 0;

	// uses MSPEECH directory
	virtual U32 PlayAudioMessage (const char * filename, IBaseObject * obj = NULL, SINGLE volume = 1.0) = 0;

	// uses MSPEECH directory
	virtual U32 PlayAnimatedMessage (const char * filename, const char * animType, S32 x, S32 y, IBaseObject* obj = NULL, SINGLE volume = 1.0) = 0;

	// uses MOVIE directory
	virtual U32 PlayMovie (const char * filename, S32 x, S32 y, SINGLE volume = 1.0) = 0;

	// uses MOVIE directory
	virtual U32 PlayMovie (const char * filename, const RECT &rc, SINGLE volume = 1.0) = 0;

	virtual BOOL32 IsPlaying (U32 soundID) const = 0;

	virtual BOOL32 StopPlayback (U32 soundID) = 0;

	virtual void FlushStreams (void) = 0;

	virtual bool IsChatEnabled (void) = 0;

	virtual void GetMusicVolumeLevel (S32& level, bool& bMuted) = 0;		

	virtual void SetMusicVolumeLevel (const S32& level, bool bMuted) = 0; 

	virtual void GetSfxVolumeLevel (S32& level, bool& bMuted) = 0;

	virtual void SetSfxVolumeLevel (const S32& level, bool bMuted) = 0;

	virtual void GetCommVolumeLevel (S32& level, bool& bMuted) = 0;

	virtual void SetCommVolumeLevel (const S32& level, bool bMuted) = 0;

	virtual void GetChatVolumeLevel (S32& level, bool& bMuted) = 0;

	virtual void SetChatVolumeLevel (const S32& level, bool bMuted) = 0;

	virtual void TweekMusicVolume (void) = 0;

	virtual U32 GetLastStreamID (void) = 0;
	virtual void SetLastStreamID (U32 id) = 0;

	// transfers ownership of anim to caller.. DO NOT DELETE ANIM UNTIL STREAM HAS ENDED!
	virtual GENRESULT GetAnimation (U32 soundID, struct IAnimate ** ppAnim) = 0;
	// returns number of movies blit'ed
	virtual int BltMovies (void) = 0;
};


//----------------------------------------------------------------------------//
//--------------------------END SoundManager.h--------------------------------//
//----------------------------------------------------------------------------//
#endif