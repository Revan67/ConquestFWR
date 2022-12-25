//--------------------------------------------------------------------------//
//                                                                          //
//                                 SFX.cpp                                  //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

   $Header: /Conquest/App/Src/SFX.cpp 35    10/14/00 2:14a Jasony $

*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "SFX.h"
#include <SFXID.h>
#include "EventPriority.h"
#include "UserDefaults.h"
#include "Resource.h"
#include "Startup.h"
#include "CQTrace.h"
#include "BaseHotRect.h"
#include "Sector.h"
#include "FogOfWar.h"
#include "Camera.h"
#include "GenData.h"

#include <IConnection.h>
#include <TComponent.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <EventSys.h>
#include <FileSys.h>
#include <3DMath.h>

#include <mmsystem.h>
#include <dsound.h>
#include <mmreg.h>
#include <msacm.h>

#define SFXNOTIFY 1				// hook into event system
#define MAX_SOUNDS 10			// number of sounds that can be played at one time
#define MAX_DUP_SOUNDS 2		// number of duplicate instances that can be played at one time
#define MAX_DUP_LSOUNDS 1		// number of duplicate looping instances that can be played at one time
#define SFXPLAYF_POSVALID	0x00000010		// user supplied a valid position
#define SFXPLAYF_OFFSCREEN	0x00000020		// same system, not on camera
#define SFXPLAYF_DISTANT	0x00000040		// lower priority than just offscreen
#define OFFSCREEN_SCALE   (0.2F)

static char szKey[] = "SFX";

#define CHUNK_NAME(d0,d1,d2,d3) ((long(d3)<<24)+(long(d2)<<16)+(d1<<8)+d0)

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
// NOTE: Real volume given to DSOUND = 
// globalVolume * SoundEffect.volume * SFX_ENTRY.volume;


//
// one instance for each member of the SFX::ID enumeration
//
struct SFX_ENTRY
{
	SINGLE volume;
	SINGLE cutoff;
	U32    dwCutoff;		// cutoff * bufferSize
	U32    dwBufferSize;
	const char * filename;
	COMPTR<IDirectSoundBuffer> lpDSBuffer;
	U32 usage;
	// add other things here?

	SFX_ENTRY (void)
	{
		filename = 0;
		volume = 0;
		cutoff = 0;
		usage = 0;
		dwCutoff = dwBufferSize = 0;
	}	
};

//
// instance created when user opens a sound effect
//
struct SoundEffect
{
	struct SoundEffect * next, * prev;
	SFX::ID id;			// which sample?
	SINGLE volume;		// user requested volume
	U32	   flags;		// looping?
	Vector pos;
	U32	   systemID;
	U32	   userRefCount;	// how many references are there to this object?
	struct SoundInstance * instance;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	SoundEffect (void)
	{
		volume = 1.0;
	}

	~SoundEffect (void);
};
//--------------------------------------------------------------------------//
//
struct SoundInstance
{
  	struct SoundInstance *pNext, *pPrev;
	COMPTR<IDirectSoundBuffer> lpDSBuffer;
	SoundEffect * owner;
	SFX::ID id;			// which sample?

	SoundInstance (void)
	{
		pNext = pPrev = 0;
		owner = 0;
	}

	~SoundInstance (void)
	{
		if (owner)
			owner->instance = 0;
	}
};
//--------------------------------------------------------------------------//
//
SoundEffect::~SoundEffect (void)
{
	if (instance)
		instance->owner = 0;
}


#define VERIFY(x)	if (x.ptr == 0)	return 0
			
	
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
#if SFXNOTIFY
struct DACOM_NO_VTABLE Sfx : ISFX, IEventCallback
#else
struct DACOM_NO_VTABLE Sfx : ISFX
#endif
{
	BEGIN_DACOM_MAP_INBOUND(Sfx)
	DACOM_INTERFACE_ENTRY(ISFX)
#if SFXNOTIFY
	DACOM_INTERFACE_ENTRY(IEventCallback)
#endif
	END_DACOM_MAP()

#if SFXNOTIFY
	U32 eventHandle;
#endif
	BOOL32 bActive;

	U32 currentSystemID;
	BOOL32 bCameraMoved;
	bool bGameActive;			// have we been updated when game was active?
	bool bLowPhysicalMemory;	// true is machine has less than 128M physical memory
	//------------------------

	SFX_ENTRY *table;
	SINGLE globalVolume;
	U32 dwLast;

	SoundEffect * effectList;
	SoundInstance instances[MAX_SOUNDS];
	SoundInstance *instanceList;			// a pointer to the latest sound instance to begin playing

	//-------------------------
	// sound vectoring
	//-------------------------
	SINGLE top, left, bottom, right;		// world coordinates of view area, updated when camera moves
				
	//------------------------

	const GT_GLOBAL_SOUNDS * globalSounds;

	//------------------------

	Sfx (void);

	~Sfx (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ISFX methods */

	DEFMETHOD_(void,Enable) (BOOL32 bEnable);

	BOOL32 __stdcall Preload (SFX::ID id);

	DEFMETHOD_(HSOUND,Open) (SFX::ID id);

	DEFMETHOD_(BOOL32,CloseHandle) (HSOUND & hSound);

	virtual BOOL32 __stdcall AddRef (const HSOUND & hSound);

	DEFMETHOD_(BOOL32,Play) (const HSOUND & hSound, U32 systemID, const class Vector * pos, U32 flags);

	DEFMETHOD_(BOOL32,Stop) (const HSOUND & hSound);

	DEFMETHOD_(BOOL32,SetVolume) (const HSOUND & hSound, SINGLE volume);

	DEFMETHOD_(BOOL32,GetVolume) (const HSOUND & hSound, SINGLE * volume) const;

	DEFMETHOD_(void,SetSFXVolume) (SINGLE volume);

	DEFMETHOD_(const GT_GLOBAL_SOUNDS &,GetGlobalSounds) (void);

	virtual U32 DEBUG_GetNumHandles (void);

	/* IEventCallback methods */

#if SFXNOTIFY
	DEFMETHOD(Notify) (U32 message, void *param = 0);
#endif

	/* Sfx methods */

	void update (void);

	bool loadData (void);

	bool load (SFX::ID id);

	void refreshInstanceList (void);

	SoundInstance * findInstanceEntry (SFX::ID id, U32 flags);		// return NULL on error

	bool loadSoundInstance (SoundInstance * instance, SFX::ID id);

	void dumpUnusedBuffers (void);

	IDAComponent * getBase (void)
	{
		return static_cast<ISFX *>(this);
	}

	S32 getDSVolume (SINGLE volume)
	{
		return ConvertFloatToDSound(volume);
	}

	S32 readLong (HANDLE handle)
	{
		DWORD dwBytesRead;
		S32 result;

		SFXDIR->ReadFile(handle, &result, sizeof(result), &dwBytesRead, 0);

		return result;
	}

	inline SINGLE calculateVolumeForPosition (SINGLE normalX, SINGLE normalY, U32 & flags);

	inline SINGLE calculatePanForPosition (SINGLE normalX);
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
Sfx::Sfx (void)
{
	globalVolume = 1.0;
	bCameraMoved = 1;

	//
	// set up the circular instance array
	//
	instanceList = &instances[0];

	if (MAX_SOUNDS==1)
	{
		instances[0].pPrev = &instances[0];
		instances[0].pNext = &instances[0];
	}
	else
	if (MAX_SOUNDS>1)
	{
		int i;
		for (i = 1; i < MAX_SOUNDS-1; i++)
		{
			instances[i].pPrev = &instances[i-1];
			instances[i].pNext = &instances[i+1];
		}

		instances[0].pPrev = &instances[MAX_SOUNDS-1];
		instances[0].pNext = &instances[1];
		instances[MAX_SOUNDS-1].pPrev = &instances[MAX_SOUNDS-2];
		instances[MAX_SOUNDS-1].pNext = &instances[0];
	}

	MEMORYSTATUS memoryStatus;
	GlobalMemoryStatus(&memoryStatus);

	if (memoryStatus.dwTotalPhys < 0x5000000)
		bLowPhysicalMemory = true;
}
//--------------------------------------------------------------------------//
//
Sfx::~Sfx (void)
{
#if SFXNOTIFY
	if (FULLSCREEN)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}
#endif
	SoundEffect * node;
	while (effectList)
	{
		node = effectList->next;
		delete effectList;
		effectList = node;
	}

	if (table)
	{
		delete [] table;
		table = 0;
	}
}
//--------------------------------------------------------------------------//
//
bool Sfx::load (SFX::ID id)
{
	bool result = 0;
	HANDLE hFile=INVALID_HANDLE_VALUE;		// handle to buffered version of file
	S32 chunk_length, chunk_name;
	U32 riff_size;
	U32 dwBytesRead;
	DSBUFFERDESC dsdesc;
	HRESULT hr;
	DAFILEDESC fdesc = table[id].filename;
	WAVEFORMATEX  * pInputWaveFormat=0, outputWaveFormat;
	MMRESULT mmr;
	HACMSTREAM hACMStream=0;
	ACMSTREAMHEADER  acmHeader;
	memset(&acmHeader, 0, sizeof(acmHeader));
	memset(&outputWaveFormat, 0, sizeof(outputWaveFormat));

	S32 data_size;
	COMPTR<IDirectSoundBuffer> lpDSBuffer;
	void * pSndMemory;
	U32 dwSndBufferLen;


	if ((hFile = SFXDIR->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQFILENOTFOUND(fdesc.lpFileName);
		table[id].filename = 0;	// prevent further attempt to load this file 
		goto Done;
	}

	if ((chunk_name = readLong(hFile)) != CHUNK_NAME('R','I','F','F'))
		goto Done;
	riff_size = readLong(hFile);
	if ((chunk_name = readLong(hFile)) != CHUNK_NAME('W','A','V','E'))
		goto Done;
	while ((chunk_name = readLong(hFile)) != CHUNK_NAME('f','m','t',' '))
	{
		chunk_length = (readLong(hFile) + 1) & ~1;
		if (SFXDIR->GetFilePosition(hFile) + chunk_length >= riff_size)
			goto Done;
		SFXDIR->SetFilePointer(hFile, chunk_length, 0, FILE_CURRENT);	// skip over this chunk
	}
	chunk_length = (readLong(hFile) + 1) & ~1;
	pInputWaveFormat = (WAVEFORMATEX *) calloc(__max(chunk_length, sizeof(WAVEFORMATEX)), 1);

	SFXDIR->ReadFile(hFile, pInputWaveFormat, chunk_length, &dwBytesRead, 0);
	SFXDIR->SetFilePointer(hFile, chunk_length - dwBytesRead, 0, FILE_CURRENT);	// skip to next chuck
	
	while ((chunk_name = readLong(hFile)) != CHUNK_NAME('d','a','t','a'))
	{
		chunk_length = (readLong(hFile) + 1) & ~1;
		if (SFXDIR->GetFilePosition(hFile) + chunk_length >= riff_size)
			goto Done;
		SFXDIR->SetFilePointer(hFile, chunk_length, 0, FILE_CURRENT);	// skip to next chunk
	}
	data_size = readLong(hFile);		// number of bytes of the sample

	// are we going to need decompression ?
	if (pInputWaveFormat->wFormatTag != WAVE_FORMAT_PCM)
	{
		outputWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	
		mmr = acmFormatSuggest(NULL, pInputWaveFormat, &outputWaveFormat, sizeof(WAVEFORMATEX), ACM_FORMATSUGGESTF_WFORMATTAG);
		if (mmr) 
		{
			CQERROR2("ACM error #%08X on file '%s'", mmr, table[id].filename);
			goto Done;
		}

		mmr = acmStreamOpen(&hACMStream, NULL, pInputWaveFormat, &outputWaveFormat, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME);

		if (mmr) 
		{
			CQERROR2("ACM error #%08X on file '%s'", mmr, table[id].filename);
			goto Done;
		}

		// now prepare the acm header

		acmHeader.cbSrcLength = data_size;
		acmHeader.cbStruct = sizeof(acmHeader);
		mmr = acmStreamSize(hACMStream, acmHeader.cbSrcLength, &acmHeader.cbDstLength, ACM_STREAMSIZEF_SOURCE);
		acmHeader.cbDstLength *= 2;		// for safety

		if ((acmHeader.pbSrc = (U8 *) VirtualAlloc(0, acmHeader.cbSrcLength, MEM_COMMIT, PAGE_READWRITE)) == 0)
		{
			CQTRACE10("VirtualAlloc failed");
			goto Done;
		}
		if ((acmHeader.pbDst = (U8 *) VirtualAlloc(0, acmHeader.cbDstLength, MEM_COMMIT, PAGE_READWRITE)) == 0)
		{
			CQTRACE10("VirtualAlloc failed");
			goto Done;
		}

		mmr = acmStreamPrepareHeader(hACMStream, &acmHeader, 0);
		if (mmr) 
		{
			CQERROR2("ACM error #%08X on file '%s'", mmr, table[id].filename);
			goto Done;
		}

		acmHeader.cbSrcLengthUsed = 0;	// nothing in the buffer
		acmHeader.cbDstLengthUsed = 0;

		SFXDIR->ReadFile(hFile, acmHeader.pbSrc, data_size, &dwBytesRead, 0);

		acmHeader.cbSrcLengthUsed = dwBytesRead;

		mmr = acmStreamConvert(hACMStream, &acmHeader, 0);
		if (mmr) 
		{
			CQERROR2("ACM error #%08X on file '%s'", mmr, table[id].filename);
			goto Done;
		}

		data_size = acmHeader.cbDstLengthUsed;
	}
	else	// no compression needed
	{
		outputWaveFormat = *pInputWaveFormat;
	}

	memset(&dsdesc, 0, sizeof(dsdesc));
	dsdesc.dwSize = sizeof(dsdesc);
	dsdesc.lpwfxFormat = & outputWaveFormat;		
	dsdesc.dwFlags = DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLPAN|DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_STATIC;
	dsdesc.dwBufferBytes = data_size;
	hr = DSOUND->CreateSoundBuffer(&dsdesc, lpDSBuffer, 0);
	if (hr != DS_OK)
	{
		CQTRACE11("DSound error #%08X",hr);
		goto Done;
	}
	lpDSBuffer->SetVolume(getDSVolume(table[id].volume * globalVolume));

	result = true;
	table[id].lpDSBuffer = lpDSBuffer;
	table[id].dwCutoff = table[id].cutoff * data_size;
	table[id].dwBufferSize = data_size;
	SNDMEMORYUSED += data_size;

	//
	// fill the sound buffer with good things
	//
	if (lpDSBuffer->Lock(0, data_size, (void **) &pSndMemory, &dwSndBufferLen, 0, 0, 0) == DS_OK)
	{
		if (hACMStream)
		{
			memcpy(pSndMemory, acmHeader.pbDst, dwSndBufferLen);
		}
		else
		{
			SFXDIR->ReadFile(hFile, pSndMemory, dwSndBufferLen, &dwBytesRead, 0);
		}
		lpDSBuffer->Unlock(pSndMemory, dwSndBufferLen,0,0);
	}

Done:
	if (hACMStream)		// compression data
	{
		if (acmHeader.fdwStatus != 0)
			acmStreamUnprepareHeader(hACMStream, &acmHeader, 0);
		if (acmHeader.pbSrc)
		{
			VirtualFree(acmHeader.pbSrc, 0, MEM_RELEASE);		// multithreaded release
			acmHeader.pbSrc = 0;
		}
		if (acmHeader.pbDst)
		{
			VirtualFree(acmHeader.pbDst, 0, MEM_RELEASE);
			acmHeader.pbDst = 0;
		}
		acmStreamClose(hACMStream, 0);
		hACMStream = 0;
	}
	if (result==0)
		table[id].filename = 0;	// prevent further attempt to load this file 
	::free(pInputWaveFormat);
	SFXDIR->CloseHandle(hFile);
	return result;
}
//--------------------------------------------------------------------------//
//
HSOUND Sfx::Open (SFX::ID id)
{
	PEFFECT result = 0;
	//
	// verify things
	//
	if ((U32)id >= dwLast || id == 0)
		goto Done;
	if (table[id].filename == 0)
		goto Done;
	if (DSOUND == 0 || SFXDIR == 0)
		goto Done;

	if (table[id].lpDSBuffer == 0 && load(id)==0)
		goto Done;

	result = new SoundEffect;
	result->id = id;
	result->flags = 0;
	result->userRefCount = 1;
	if ((result->next = effectList) != 0)
		effectList->prev = result;
	effectList = result;

Done:
	return HSOUND(result);		// WARNING: Make sure the destructor is not called here!
}
//--------------------------------------------------------------------------//
//
BOOL32 Sfx::Preload (SFX::ID id)
{
	BOOL32 result = 0;

	if (bLowPhysicalMemory)
		goto Done;
	//
	// verify things
	//
	if ((U32)id >= dwLast || id == 0)
		goto Done;
	if (table[id].filename == 0)
		goto Done;
	if (DSOUND == 0 || SFXDIR == 0)
		goto Done;

	if (table[id].lpDSBuffer == 0 && load(id)==0)
		goto Done;

	result = 1;
Done:
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 Sfx::CloseHandle (HSOUND & hSound)
{
	VERIFY(hSound);
	
	if (hSound->userRefCount-- == 1)
	{
		// remove the node from the list
		if (effectList == hSound)
		{
			if ((effectList = hSound->next) != 0)
				effectList->prev = 0;
		}
		else
		{
			if (hSound->prev)
				hSound->prev->next = hSound->next;
			if (hSound->next)
				hSound->next->prev = hSound->prev;
		}
		
		//
		// stop looping samples
		//
		if ((hSound->flags & SFXPLAYF_LOOPING) && hSound->instance)
			hSound->instance->lpDSBuffer->Stop();

		hSound->next = 0;
		delete hSound.ptr;
	}

	hSound.ptr = 0;

	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 Sfx::AddRef (const HSOUND & hSound)
{
	VERIFY(hSound);
	
	hSound->userRefCount++;
	return 1;
}
//--------------------------------------------------------------------------//
//
U32 Sfx::DEBUG_GetNumHandles (void)
{
	U32 result = 0;
	SoundEffect * node = effectList;

	while (node)
	{
		result++;
		node = node->next;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
inline SINGLE Sfx::calculateVolumeForPosition (SINGLE normalX, SINGLE normalY, U32 & flags)
{
	if (normalY > 0)
		normalY *= 1.5;
	else
		normalY *= 0.75;
	SINGLE x = fabs(normalX);
	SINGLE y = fabs(normalY);

	if (x > 1.0F || y > 1.0F)
		flags |= SFXPLAYF_OFFSCREEN;

	x = __max(x, y);

	if ((x -= 1.0) <= 0)
		return 1.0;
	if ((x = ((2.0F - x)/2.0F)) < OFFSCREEN_SCALE)
	{
		flags |= SFXPLAYF_DISTANT;
//		if (flags & SFXPLAYF_LOOPING)
//			return 0;		// otherwise we would always play looping sounds
		return OFFSCREEN_SCALE;
	}
	return x;
}
//--------------------------------------------------------------------------//
//
inline SINGLE Sfx::calculatePanForPosition (SINGLE normalX)
{
	SINGLE x = fabs(normalX);

	if ((x -= 0.70) <= 0)
		return 1.0;		// no pan
	if ((x = ((1.0F - x)/1.0F)) < 0)
		return 0;		
	return x;
}
//--------------------------------------------------------------------------//
//
BOOL32 Sfx::Play (const HSOUND & hSound, U32 systemID, const class Vector * pos, U32 flags)
{
	VERIFY(hSound);
	BOOL32 bInSystem=1;
	BOOL32 bNewOwner=0;
	SINGLE volumeScale=1.0F;		// scale for distance
	S32 dsPanValue = 0;	 // no pan by default

	if (bActive==0)
		return 1;

	flags &= ~(SFXPLAYF_OFFSCREEN|SFXPLAYF_DISTANT);

	if (pos)
	{
		flags |= SFXPLAYF_POSVALID;
		hSound->pos = *pos;
		hSound->systemID = systemID;
		//CQASSERT(systemID && "Invalid systemID passed to SFX::Play()");
		if (systemID == currentSystemID && bGameActive)
		{
			//
			// check to see if sound is "visible" to the player
			//
			if ( ((flags & SFXPLAYF_NOFOG)!=0 && FOGOFWAR->CheckHardFogVisibilityForThisPlayer(systemID, *pos) == 0) || 
				 ((flags & SFXPLAYF_NOFOG)==0 && FOGOFWAR->CheckVisiblePosition(*pos) == 0) )
			{
				bInSystem=0;
			}
			else // on camera ?
			{
				const class Transform *trans = CAMERA->GetInverseWorldTransform();  /* world to rotated_world transform */
				Vector tpos = trans->rotate_translate(*pos);

				SINGLE midx = (left + right) / 2;
				SINGLE midy = (top + bottom) / 2;
				SINGLE normalX, normalY;

				normalX = 2 * (SINGLE(tpos.x) - midx) / (right - left);	
				normalY = 2 * (SINGLE(tpos.y) - midy) / (top - bottom);

				if ((volumeScale = calculateVolumeForPosition(normalX, normalY, flags)) == 0)
					bInSystem = 0;
				else
				{
					dsPanValue = getDSVolume(calculatePanForPosition(normalX));
					if (normalX > 0)
						dsPanValue = -dsPanValue;		// positive pan means "take away from left channel"
				}
			}
		}
		else
			bInSystem=0;
	}

	//
	// treat sound special if user doesn't want it dropped
	//
	if (bInSystem==0 && (flags & SFXPLAYF_NODROP))
	{
		flags |= (SFXPLAYF_OFFSCREEN|SFXPLAYF_DISTANT);
		volumeScale = 0;		// don't actually make a sound
		bInSystem = 1;
	}

	hSound->flags = (flags & ~SFXPLAYF_NORESTART);

	//
	// if we are in the wrong system, but have a sample owned
	//
	if (bInSystem==0 && hSound->instance)
	{
		CQASSERT(hSound->instance->lpDSBuffer != 0);
		hSound->instance->lpDSBuffer->Stop();
		CQASSERT(hSound->instance->owner->instance == hSound->instance);
		hSound->instance->owner = 0;
		hSound->instance = 0;
	}

	if (bInSystem && hSound->instance == 0)
	{
		SoundInstance * instance;
		//
		// create a sound buffer for us to use
		//
		refreshInstanceList();
		instance = findInstanceEntry(hSound->id, flags);		// return NULL on error

		if (instance)
		{
			//
			// remove previous owner
			//
			if (instance->owner)
			{
				CQASSERT(instance->owner != hSound);	// can't take over ownership from self!
				instance->owner->instance = 0;
				instance->owner = 0;
			}

			if (loadSoundInstance(instance, hSound->id))
			{
				instance->owner = hSound;
				hSound->instance = instance;
				bNewOwner = 1;		// ownership changed hands

				if (instance != instanceList)
				{
					//
					// remove instance from current position
					//
					instance->pPrev->pNext = instance->pNext;
					instance->pNext->pPrev = instance->pPrev;
					//
					// put instance at the front of the list
					//
					instance->pPrev = instanceList->pPrev;
					instance->pNext = instanceList;
					instanceList->pPrev->pNext = instance;
					instanceList->pPrev = instance;
					instanceList = instance;
				}
			}
		}
	}

	if (bInSystem && hSound->instance)
	{
		SINGLE volume;

//		if (bNewOwner && (flags & SFXPLAYF_NORESTART)==0)
		if ((flags & SFXPLAYF_NORESTART)==0)
		{
			hSound->instance->lpDSBuffer->Stop();
			hSound->instance->lpDSBuffer->SetCurrentPosition(0);
		}
		volume = table[hSound->id].volume * globalVolume * hSound->volume * volumeScale;
		hSound->instance->lpDSBuffer->SetVolume(getDSVolume(volume));
		hSound->instance->lpDSBuffer->SetPan(dsPanValue);
		hSound->instance->lpDSBuffer->Play(0,0,(flags & SFXPLAYF_LOOPING)?DSBPLAY_LOOPING:0);
	}

	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 Sfx::Stop (const HSOUND & hSound)
{
	VERIFY(hSound);

	hSound->flags = 0;
	if (hSound->instance)
		hSound->instance->lpDSBuffer->Stop();
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 Sfx::SetVolume (const HSOUND & hSound, SINGLE volume)
{
	VERIFY(hSound);

	if (volume > 1.0)
		volume = 1.0;
	else
	if (volume < 0)
		volume = 0;
	hSound->volume = volume;
	if (hSound->instance)		// force update of volume on next update
		bCameraMoved=1;
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 Sfx::GetVolume (const HSOUND & hSound, SINGLE * volume) const
{
	VERIFY(hSound);

	*volume = hSound->volume;
	return 1;
}
//--------------------------------------------------------------------------//
//
#if SFXNOTIFY
GENRESULT Sfx::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_MISSION_CLOSE:
		if (param!=0)
			dumpUnusedBuffers();
		break;
	case CQE_CAMERA_MOVED:
		bCameraMoved=1;
		break;
	case CQE_UPDATE:
		if (bCameraMoved && bActive)
		{
			bCameraMoved=0;
			update();
		}
		break;
	};

	return GR_OK;
}
#endif
//--------------------------------------------------------------------------//
//
void Sfx::SetSFXVolume (SINGLE volume)
{
	if (globalVolume != volume)
	{
		globalVolume = volume;
		bCameraMoved=1;				// force update of volume on next update
	}
}
//--------------------------------------------------------------------------//
//
const GT_GLOBAL_SOUNDS & Sfx::GetGlobalSounds (void)
{
	if (globalSounds==0)
	{
		globalSounds = (GT_GLOBAL_SOUNDS *) GENDATA->GetArchetypeData("GlobalSounds");
		CQASSERT(globalSounds);
	}

	return *globalSounds;
}
//--------------------------------------------------------------------------//
//
void Sfx::Enable (BOOL32 bEnable)
{
	if (bEnable == bActive)
		return;				// no change

	bActive = bEnable;

	if (bActive)
	{
		bCameraMoved = 1;	// update everything next frame
	}
	else
	{
		//
		// deactivate all samples
		//
		int i;

		for (i = 0; i < MAX_SOUNDS; i++)
		{
			if (instances[i].lpDSBuffer!=0)
			{
				instances[i].lpDSBuffer->Stop();
				if (instances[i].owner)
				{
					CQASSERT(instances[i].owner->instance == &instances[i]);
					instances[i].owner->instance = 0;
					instances[i].owner = 0;
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
// happens when camera position changes
//
void Sfx::update (void)
{
	currentSystemID = SECTOR->GetCurrentSystem();
	refreshInstanceList();
	{
		Vector topl, topr, bottomr, bottoml;		// world coordinates of view area, updated when camera moves

		CAMERA->PaneToPoints(topl, topr, bottomr, bottoml);
		SINGLE viewHeight = topl.y - bottoml.y;		// should be positive value

		bottom = bottoml.y;
		top = bottom + viewHeight;
		left = (topl.x + bottoml.x) / 2;
		right = (topr.x + bottomr.x) / 2;
	}
	//
	// update all looping&active samples
	//
	SoundEffect * node = effectList;
	while (node)
	{
		if (node->instance || (node->flags & SFXPLAYF_LOOPING) != 0)
		{
			HSOUND hSound(node);	// doesn't add a refcount
			if (node->flags & SFXPLAYF_POSVALID)
				Play(hSound, node->systemID, &node->pos, node->flags|SFXPLAYF_NORESTART);
			else
				Play(hSound, 0, 0, node->flags|SFXPLAYF_NORESTART);
			hSound.ptr = 0;			// prevent decriment of refcount
		}
		node = node->next;
	}

	bGameActive = (CQFLAGS.bGameActive!=0);
}

struct sfxchunk
{
	U32 id;
	float volume;
	char filename[32];
	float cutoff;
};
//--------------------------------------------------------------------------//
//
bool Sfx::loadData (void)
{
	bool result = 0;
	U32 numChunks=0, i;
	HRSRC hRes;
	const sfxchunk * chunk=0;

	if (SFXDIR == 0)
		goto Done;

	if ((hRes = FindResource(hResource, MAKEINTRESOURCE(IDR_SFX1), "SFX")) != 0)
	{
		HGLOBAL hGlobal;

		if ((hGlobal = LoadResource(hResource, hRes)) != 0)
		{
			U32 dataSize = SizeofResource(hResource, hRes);
			U8 * pData = (U8 *) LockResource(hGlobal);
			chunk = (sfxchunk *) (pData + 4);
			numChunks = (dataSize - 4) / sizeof(sfxchunk);
			CQASSERT(((dataSize - 4) % sizeof(sfxchunk)) == 0);
			dwLast = ((U32 *)pData)[0];
			CQASSERT(dwLast);
		}
	}

	CQASSERT(chunk!=0 && "SFX resource data is missing!");

	table = new SFX_ENTRY[dwLast];

	for (i = 0; i < numChunks; i++)
	{
		if (chunk[i].id >= dwLast || chunk[i].id == 0)
		{
			CQBOMB1("Bad SFX ID %d Found in SFX table.", chunk[i].id);
			goto Done;
		}
		table[chunk[i].id].volume = chunk[i].volume;
		table[chunk[i].id].filename = chunk[i].filename;
		table[chunk[i].id].cutoff = chunk[i].cutoff;
	}

	result = true;

Done:
	return result;
}
//--------------------------------------------------------------------------//
// Remove ownership of buffers that are not playing, or have passed the cutoff point.
//
void Sfx::refreshInstanceList (void)
{
	int i;

	for (i = 0; i < MAX_SOUNDS; i++)
	{
		// sound might not be playing because we are paused, so don't flush looping sounds
		if (instances[i].owner && instances[i].lpDSBuffer!=0 && (instances[i].owner->flags & SFXPLAYF_LOOPING) == 0)
		{
			DWORD dwStatus;
			if (instances[i].lpDSBuffer->GetStatus(&dwStatus)==DS_OK)
			{
				if ((dwStatus & DSBSTATUS_PLAYING) == 0)
				{
					CQASSERT(instances[i].owner->instance == &instances[i]);
					instances[i].owner->instance = 0;
					instances[i].owner = 0;
				}
				else // see if we are in the cutoff zone
				if ((dwStatus & DSBSTATUS_LOOPING) == 0)		// don't check looping sounds
				{
					U32 pos;
					if (instances[i].lpDSBuffer->GetCurrentPosition(&pos, NULL) == DS_OK)
					{
						if (pos > table[instances[i].id].dwCutoff)
						{
							CQASSERT(instances[i].owner->instance == &instances[i]);
							instances[i].owner->instance = 0;
							instances[i].owner = 0;
						}
					}
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
// return NULL on error, 
// #1 unowned instance matching our id
// #2 any unowned instance (limit usage to MAX_DUP_SOUNDS)
// #3 owned instance matching our id, not looping
// #4 any owned instance, not looping 
// If low priority, do not override high priority sounds
//
SoundInstance * Sfx::findInstanceEntry (SFX::ID id, U32 flags)
{
	SoundInstance * result, *matchingUnowned, *anyUnowned, *matchingOwned, *anyOwned;
	int count=MAX_SOUNDS;
	SoundInstance *instance = &instances[rand() % MAX_SOUNDS];		// start on a random effect
	
	result = matchingUnowned = anyUnowned = matchingOwned = anyOwned = 0;

	while (count-- > 0)
	{
		if (instance->owner == 0)
		{
			if (instance->id == id)
				matchingUnowned = instance;
			else
			if (table[id].usage < U32((flags&SFXPLAYF_LOOPING)?MAX_DUP_LSOUNDS:MAX_DUP_SOUNDS))
				anyUnowned = instance;
		}
		else // owned
		if ((flags & SFXPLAYF_OFFSCREEN)==0)	 // not low priority
		{
			// if we are both looping, and victim is offscreen
			if ((flags & SFXPLAYF_LOOPING)!=0 && (instance->owner->flags & (SFXPLAYF_LOOPING|SFXPLAYF_OFFSCREEN)) == (SFXPLAYF_LOOPING|SFXPLAYF_OFFSCREEN))
			{
				if (instance->id == id)
					matchingOwned = instance;
			}
			else
			if ((instance->owner->flags & (SFXPLAYF_LOOPING|SFXPLAYF_OFFSCREEN)) == SFXPLAYF_OFFSCREEN)
			{
				if (instance->id == id)
					matchingOwned = instance;
				else
				if (table[id].usage < U32((flags&SFXPLAYF_LOOPING)?MAX_DUP_LSOUNDS:MAX_DUP_SOUNDS))
					anyOwned = instance;
			}
		}
		else  // we are low priority
		if ((flags & SFXPLAYF_DISTANT)==0)		// not lowest priority though
		{
			// if we are both looping, and victim is distant
			if ((flags & SFXPLAYF_LOOPING)!=0 && (instance->owner->flags & (SFXPLAYF_LOOPING|SFXPLAYF_DISTANT)) == (SFXPLAYF_LOOPING|SFXPLAYF_DISTANT))
			{
				if (instance->id == id)
					matchingOwned = instance;
			}
			else
			if ((instance->owner->flags & (SFXPLAYF_LOOPING|SFXPLAYF_DISTANT)) == SFXPLAYF_DISTANT)
			{
				if (instance->id == id)
					matchingOwned = instance;
				else
				if (table[id].usage < U32((flags&SFXPLAYF_LOOPING)?MAX_DUP_LSOUNDS:MAX_DUP_SOUNDS))
					anyOwned = instance;
			}
		}

		instance = instance->pNext;
	}

	if ((result = matchingUnowned) == 0)
		if ((result = anyUnowned) == 0)
			if ((result = matchingOwned) == 0)
				result = anyOwned;

	return result;
}
//--------------------------------------------------------------------------//
//
bool Sfx::loadSoundInstance (SoundInstance * instance, SFX::ID id)
{
	if (instance->id != id)
	{
		if (instance->lpDSBuffer!=0)	// do we need to flush existing buffer?
		{
			table[instance->id].usage--;
			CQASSERT(S32(table[instance->id].usage) >= 0);
			instance->lpDSBuffer.free();
			instance->id = SFX::INVALID;
		}

		if (table[id].usage==0)		// is no one else using this sample?
		{
			if (table[id].lpDSBuffer==0 && load(id)==0)
				goto Done;
#ifdef _DEBUG
			table[id].lpDSBuffer->AddRef();
			U32 ref = table[id].lpDSBuffer->Release();
			if (ref != 1)
				CQBOMB0("Ref count leak!");
#endif
			instance->lpDSBuffer = table[id].lpDSBuffer;
		}
		else // need to duplicate the buffer
		{
			HRESULT hr;
			hr = DSOUND->DuplicateSoundBuffer(table[id].lpDSBuffer, instance->lpDSBuffer);
			if (hr != DS_OK)
			{
				instance->id = SFX::INVALID;
				CQTRACE11("DSound error #%08X", hr);
				goto Done;
			}
		}
		table[id].usage++;
		instance->id = id;
	}

Done:
	return (instance->lpDSBuffer.ptr!=0);
}
//--------------------------------------------------------------------------//
//
void Sfx::dumpUnusedBuffers (void)
{
	U32 i;

	for (i = 0; i < dwLast; i++)
	{
		if (table[i].usage==0)		// is no one else using this sample?
		{
			table[i].lpDSBuffer.free();
			SNDMEMORYUSED -= table[i].dwBufferSize;
			table[i].dwBufferSize = 0;
		}
	}

	if (S32(SNDMEMORYUSED) < 0)
		SNDMEMORYUSED = 0;		// just to be sure, should never happen
}
//--------------------------------------------------------------------------//
//
struct _sfx : GlobalComponent
{
	Sfx * manager;

	virtual void Startup (void)
	{
		SFXMANAGER = manager = new DAComponent<Sfx>;
		AddToGlobalCleanupList((IDAComponent **)&SFXMANAGER);
	}

	virtual void Initialize (void)
	{
#if SFXNOTIFY
		COMPTR<IDAConnectionPoint> connection;

		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(manager->getBase(), &manager->eventHandle);
		FULLSCREEN->SetCallbackPriority(manager,EVENT_PRIORITY_SFX);
#endif
		manager->loadData();
	}
};

static _sfx startup;

//--------------------------------------------------------------------------//
//-------------------------END SFX.cpp--------------------------------------//
//--------------------------------------------------------------------------//
