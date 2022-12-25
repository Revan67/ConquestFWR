//****************************************************************************
//*                                                                          *
//*  AUDIOMGR.H: DA COM audio services                                       *
//*                                                                          *
//*  Source compatible with 32-bit 80386 C/C++                               *
//*                                                                          *
//*  V1.00 of 1-Jan-98: Initial                                              *
//*                                                                          *
//*  32-bit protected-mode source compatible with Watcom 10.5/MSC 9.0        *
//*                                                                          *
//*  Author: John Miles                                                      *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  The audio manager provides a system-independent abstraction layer for   *
//*  allocation and performance of various audio data types.                 *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  Copyright (C) 1997 Digital Anvil, Inc.                                  *
//*                                                                          *
//****************************************************************************

#ifndef AUDIOMGR_H
#define AUDIOMGR_H

//
// General type definitions for portability
// 
#ifndef DACOM_H
#include "dacom.h"   // DA component object manager
#endif

#include "typedefs.h"
#include "DAVariant.h"

// need HWND
#ifndef _WINDOWS_
#error Windows.h required for this to compile!
#endif

#ifndef YES
#define YES 1
#endif

#ifndef NO
#define NO  0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE  0
#endif

//****************************************************************************
//*                                                                          *
//* AUDIOMGR class definition                                                *
//*                                                                          *
//****************************************************************************

//
// Forward declare the IDirectSound class so we don't have to
//	include the DirectSound include files here.
//

struct IDirectSound;
//typedef void *HWND;

//
// SOUND_ID = identifier for instance of playing audio sample
//
// A SOUND_ID is returned to the application by IAudioManager::play(), to
// allow the application to identify the playing sound for control purposes.
// SOUND_IDs are session-unique -- a new value is returned by each successive
// call to IAudioManager::play().  These unique SOUND_ID values have an
// effectively infinite lifetime.  In other words, they do not become 
// "invalid" when a sound finishes playing or is stopped by the application.  
// Calls to IAudioManager control functions with SOUND_IDs for obsolete 
// sounds are simply ignored.  SOUND_IDs do not have to be explicitly freed.
// 
// This means that the application does not have to perform any of its 
// own bookkeeping or handle/memory management on sound objects ("play and
// forget").
//

typedef U32 SOUND_ID;
#define INVALID_SOUND_ID 0xFFFFFFFF

//
// Identifiers for sound status
//

#define SOUND_STOPPED 0
#define SOUND_PLAYING 1
#define SOUND_DONE    2

//
// Bitmask identifiers for sound playback modes
//

#define SOUND_STEREO_ONLY 1  // Sound played as stereo (set relative levels in 2 speakers)
#define SOUND_DOLBY       2  // Generate Dolby sound (set relative levels in 3 speakers)
#define SOUND_PRO_LOGIC   4  // Generate Dolby sound (set relative levels in 4 speakers)
#define SOUND_STREAMABLE  8  // Stream this file (rather than loading it in its entirety)

//
// Speaker bitmask identifiers
//

#define SPKR_L_FRONT    1
#define SPKR_CENTER     2
#define SPKR_R_FRONT    4
#define SPKR_L_SURROUND 8
#define SPKR_R_SURROUND 16
#define SPKR_SURROUND   (SPKR_L_SURROUND | SPKR_R_SURROUND)
#define SPKR_LFE        32
#define SPKR_ALL        0xFF

struct DACOM_NO_VTABLE IAudioManager : public IDAComponent
{
   //
   // Starts and shuts down audio services.  The application should specify 
   // an OS-specific device ID for the desired digital and/or MIDI driver 
   // (normally 0 for the primary device in either case, -1U for no device), 
   // as well as the desired digital audio output format and rate.
   //
   // startup() returns TRUE if initialization was successful, else FALSE
   //
   // TODO: These are TEMPORARY functions -- make startup/shutdown work
   // in a more meaningful DACOM-like fashion?  e.g., perform startup when
   // CreateInstance() is called for the first time, and shutdown when
   // last reference is destroyed?
   //

   virtual BOOL32 COMAPI startup    (U32     digital_device_ID   = 0,
                                     U32     MIDI_device_ID  = 0,
                                     U32     data_cache_KB  = 4096,
                                     U32     maximum_handles = 32,
                                     U32     output_rate  = 22050,
                                     U32     output_bits  = 16,
                                     U32     output_channels  = 2,
									 HWND    hwnd=0) = 0;

   virtual void   COMAPI shutdown   (void) = 0;

   //
   // Precache a given sound object by its filename, so that it can be 
   // played with minimal startup latency
   //
   // If the sound file has already been cached, it will be reused without
   // further file access
   //
   // If a parent file system is specified, then *filename is ignored for 
   // purposes of caching the sound object.  It is used only to track the 
   // object by name, for subsequent playback initiation
   //

    virtual BOOL32         COMAPI precache(const  C8          *name, 
                                           S32                 mode,
                                           struct IFileSystem *parent = 0) = 0;

   //
   // Precache a given sound object based on a property list rather than a
   // filename
   //

    virtual BOOL32         COMAPI precache(const C8       *name,
                                           S32             mode,
                                           const PROPERTY *properties) = 0;
   
   //
   // Obtain handle to instance of sound, for control purposes
   //

   virtual SOUND_ID       COMAPI get_ID  (const C8 *name) = 0;

   //
   // Obtain handle to instance of sound based on property list, for control purposes
   //
   // This would be used for setting up a memory-based stream independent of any files,
   // for instance
   //

   virtual SOUND_ID       COMAPI get_ID  (const PROPERTY *properties) = 0;


   //
   // Initiate playback of specified sound from its beginning
   //

   virtual void           COMAPI play    (SOUND_ID sound,
                                          S32      ramp_milliseconds = 0) = 0;

   //
   // Stop playback of specified sound
   //

   virtual void           COMAPI stop    (SOUND_ID sound,
                                          S32      ramp_milliseconds = 0) = 0;

   //
   // Resume playback of specified sound from its current point
   //

   virtual void           COMAPI resume  (SOUND_ID sound,
                                          S32      ramp_milliseconds = 0) = 0;

   //
   // Get status of specified sound
   // Possible return codes are SOUND_STOPPED, SOUND_PLAYING, SOUND_DONE
   //

   virtual U32            COMAPI get_status(SOUND_ID sound);

   //
   // Set overall sound volume
   //
   // Position-derived volume scaling is applied after this control setting
   //
   // Level is specified in decibels, 0 = no attenuation (max volume), -96 =
   // max attenuation (min volume) for 16-bit audio
   //

   virtual void           COMAPI set_volume(SOUND_ID sound,
                                            SINGLE   volume,
                                            S32      ramp_milliseconds = 0) = 0;

   //
   // Set levels by source intensity and vector displacement
   //
   // (For use with mono sound effects via multi-speaker or 3D audio)
   //
   // falloff_3dB parameter = distance (vector magnitude) at which sound 
   // volume decreases by 3 dB (1/2 power)
   //
   // Listener is at (0,0,0), facing negative Z-axis
   //

   virtual void           COMAPI set_sound_bearing(SOUND_ID       sound,
                                                   const Vector  &bearing,
                                                   SINGLE         falloff_3dB) = 0;

   //
   // Set levels by explicitly controlling the speaker volume levels
   //
   // (For use with mono sound effects via multi-speaker audio only)
   //
   // A sound's levels may be controlled either by this function or by 
   // set_sound_bearing() -- not both at once.  When either
   // set_speaker_volume() or set_sound_bearing() is called on a given 
   // sound ID, that sound ID is marked for level control by that 
   // API function only.  Default volumes for all 6 speakers are set the 
   // first time set_speaker_volume() is called for a given sound ID.
   //
   // Level is specified in decibels, 0 = no attenuation (max volume), -96 =
   // max attenuation (min volume) for 16-bit audio
   //

   virtual void           COMAPI set_speaker_level (SOUND_ID      sound,
                                                    U32           spkr_bitmask,
                                                    SINGLE        level) = 0;

   //
   // Set relative playback rate percentage (e.g., for Doppler-effect control)
   //
   // 100% = normal rate
   //

   virtual void           COMAPI set_relative_rate (SOUND_ID      sound,
                                                    SINGLE        rate_percent) = 0;


   //
   // Set loop count
   //
   // 0=infinite, 1=one-shot, n=n iterations
   //
   // This is separate from the sub-file looping (e.g., for engine sounds) 
   // effect supported for .VOC files
   //

   virtual void           COMAPI set_loop_count    (SOUND_ID      sound,
                                                    S32           loop_cnt) = 0;

   //
   // Get pointer to DirectSound provider used by IAudioManager provider
   //
   // This allows the application to perform its own low-latency streaming,
   // movie playback, etc., without conflicting with IAudioManager
   // 
   // DirectSound application should cast this pointer to an LPDIRECTSOUND
   // pointer and call its Release() method when done with it
   // 
   // WARNING: Result will be NULL if underlying MSS layer configured to
   // use waveOut instead of DirectSound!
   // 

   virtual IDirectSound* COMAPI get_DirectSound_provider  (void) = 0;
};

#endif
