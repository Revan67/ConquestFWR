#ifndef SFX_H
#define SFX_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                   SFX.h                                  //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Sbarton $

   $Header: /Conquest/App/Src/SFX.h 11    10/13/00 6:44p Sbarton $

					   High level sound effect manager. 
   Unlike the wav streams, sound fx sounds can be played again and again. 
   Calling the Open method guarantees that the sample is loaded into memory
   and is ready to play at a moment's notice. 
   Also, because the number of samples that can be mixed at one time is finite,
   the SFX manager limits the number of effects actually sent to DSOUND, based on
   a highly secret priority scheme.

*/
//--------------------------------------------------------------------------//
//
/*
	virtual void __stdcall Preload (SFX::ID id) = 0;
	//------------------------------
	//
	void ISFX::Preload (SFX::ID id);
		INPUT:
			SFX::ID: Enumerated value specifying the sample to open. (See SFXID.h)
		RETURNS:
			Non-zero if successful.
		OUTPUT:
			Loads the associated sound effect into memory, if it wasn't already loaded.
		NOTES:
			This method is useful for class factories.
	//------------------------------
	//
	HSOUND ISFX::Open (SFX::ID id);
		INPUT:
			SFX::ID: Enumerated value specifying the sample to open. (See SFXID.h)
		RETURNS:
			Handle to a sound. NULL if the method fails.
		OUTPUT:
			Loads the associated sound effect into memory, if it wasn't already loaded.
			Prepares it for playback.
		NOTES:
			You can open multiple handles for a sound effect if you will need to play
			the same sound more than once at a time.

  

*/

#ifndef DACOM_H
#include <DACOM.h>
#endif

namespace SFX
{
enum ID;
}


// typedef struct SoundEffect *HSOUND;
typedef struct SoundEffect *PEFFECT;
class HSOUND
{
	PEFFECT ptr;

	explicit HSOUND (PEFFECT _ptr)		// watch out for refcount problem
	{
		ptr = _ptr;
	}

public:

	HSOUND (void)
	{
		ptr = 0;
	}

	HSOUND (const HSOUND & new_ptr);

	~HSOUND (void)
	{
		free();
	}

	const HSOUND & operator = (const HSOUND & new_ptr);

	const HSOUND & operator = (int i)
	{
		CQASSERT(i==0);		// only allowed to set to NULL
		free();
		return *this;
	}

	operator PEFFECT (void) const
	{
		return ptr;
	}

	PEFFECT operator -> (void) const
	{
	 	return ptr;
	}

	void free (void);

	friend struct Sfx;
};


struct GT_GLOBAL_SOUNDS;

//------------------------------------
// flags for ISFX::Play
//------------------------------------
#define SFXPLAYF_LOOPING	0x00000001
#define SFXPLAYF_NORESTART	0x00000002		// do not restart sample from beginning if already playing
#define SFXPLAYF_NOFOG		0x00000004		// play through the fog
#define SFXPLAYF_NODROP		0x00000008		// play sound at 0 volume rather than dropping, when off screen



struct ISFX : IDAComponent
{
	virtual BOOL32 __stdcall Preload (SFX::ID id) = 0;

	virtual HSOUND __stdcall Open (SFX::ID id) = 0;

	virtual BOOL32 __stdcall CloseHandle (HSOUND & hSound) = 0;

	virtual BOOL32 __stdcall AddRef (const HSOUND & hSound) = 0;

	virtual BOOL32 __stdcall Play (const HSOUND & hSound, U32 systemID=0, const class Vector * pos=0, U32 flags=0) = 0;

	virtual BOOL32 __stdcall Stop (const HSOUND & hSound) = 0;

	virtual BOOL32 __stdcall SetVolume (const HSOUND & hSound, SINGLE volume) = 0;

	virtual BOOL32 __stdcall GetVolume (const HSOUND & hSound, SINGLE * volume) const = 0;

	virtual void __stdcall Enable (BOOL32 bEnable) = 0;

	virtual void __stdcall SetSFXVolume (SINGLE volume) = 0;

	virtual const GT_GLOBAL_SOUNDS & __stdcall GetGlobalSounds (void) = 0;

	virtual U32 DEBUG_GetNumHandles (void) = 0;
};

//--------------------------------------------------------------------------//
//
inline void HSOUND::free (void)
{
	if (ptr && SFXMANAGER)
	{
		SFXMANAGER->CloseHandle(*this);
	}
}
//--------------------------------------------------------------------------//
//
inline const HSOUND & HSOUND::operator = (const HSOUND & new_ptr)
{
	free();
	if ((ptr = new_ptr.ptr) != 0)
		SFXMANAGER->AddRef(*this);
	return *this;
}
//--------------------------------------------------------------------------//
//
inline HSOUND::HSOUND (const HSOUND & new_ptr)
{
	if ((ptr = new_ptr.ptr) != 0)
		SFXMANAGER->AddRef(*this);
}
//--------------------------------------------------------------------------//
//---------------------------------END SFX.h--------------------------------//
//--------------------------------------------------------------------------//
#endif