//--------------------------------------------------------------------------//
//                                                                          //
//                              ISound.h                                     //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Rmarr $
*/

/*
   This header file describes the SoundListener interface used by the Digital Anvil Sound Manager.  
   The Sound Manager provides a simple way to add sounds to an app.  The app implements the ISoundSource 
   interface, provides a list of sounds to play, implementing this interface for the sounds, and calls 
   update periodically causing Sound Manager to play the sounds.  Sound Manager handles all of the buffer
   creation and management using DirectSound, allows the user to set EAX and A3D properties if desired, 
   and supports DirectSound3D.

   //-----------------------------
   // ISoundSource Methods
   //-----------------------------

	U32 get_start_time()
		INPUT:
			NONE

		RETURNS:
			U32 start time for this sound

		NOTES:
			The start time should initially be set by calling SoundManager::get_current_time_ms() to ensure
			that the times are in sync with SoundManager.

	bool COMAPI is_on()
		INPUT:
			NONE

		RETURNS:
			Bool specifying whether this sound is on (should be playing or not).

	bool COMAPI is_3D()
		INPUT:
			NONE

		RETURNS:
			Bool specifying whether this sound is a 3D sound.

		NOTES:
			SoundManager skips all of the extra 3D processing for non 3D sounds, setting
			only pan and attenuation for the sound. If a sound is not meant to be 3D, this 
			should return false.  If the app wants to stop processing the 3D properties of
			a 3D sound, the sound can return false for as long as the 3D portion needs to
			be ignored.

	bool COMAPI is_looping()
		INPUT:
			NONE

		RETURNS:
			Bool specifying whether this sound should be looped.
	
	SOUND_ARCH_INDEX get_archetype()
		INPUT:
			NONE

		RETURNS:
			SOUND_ARCH_INDEX specifying which archetype this sound is an instance of.

	GENRESULT COMAPI get_attenuation(SINGLE *)
		INPUT/OUTPUT:
			Ptr to the SINGLE attenuation value.

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the SoundManager
			default, 0,  will be used).  

		NOTES:
			This value represents decibel attenuation between -100.0 and 0.0 . A value of 0.0 is full
			volume, -100.0 is effectively silence.

	GENRESULT COMAPI get_frequency(SINGLE *)
		INPUT/OUTPUT:
			Ptr to the SINGLE frequency value.

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the SoundManager
			default, 1.0,  will be used).  

		NOTES:
			This value represents the amount to scale the original frequency (the sample rate of the
			archetype) by. 0.5 would be half speed, 1.0 normal, and 2.0 would be twice as fast. The
			archetype must be created with the SM_ENABLE_FREQUENCY_CONTROL flag specified or SoundManager
			will be unable to control the frequency.

	GENRESULT COMAPI get_pan(S32 *)
		INPUT/OUTPUT:
			Ptr to the SINGLE pan value.

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the DSound
			default, 0 - centered,  will be used).  

		NOTES:
			This value represents the amount to pan the sound volume. The range is -100.0 to 100.0 .
			A value of -100.0 is full left (the right would be silent), 0.0 is center (both left and
			right at full volume), and 100.0 would be full right (left would be silent).

	GENRESULT COMAPI get_position(Vector *)
		INPUT/OUTPUT:
			Ptr to the Vector position of the sound.

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the DSound
			default (0,0,0) will be used).  

	GENRESULT COMAPI get_min_distance(SINGLE *)
		INPUT/OUTPUT:
			Ptr to the SINGLE min distance for the sound.

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the DSound
			default, 1, will be used).  

		NOTES:
			The sound is at full volume at min distance and closer to the listener than min distance
			will not cause the sound to get any louder.  At twice min distance, the volume of the sound
			will be halfed, at 4x, halfed again, etc.

	GENRESULT COMAPI get_max_distance(SINGLE *) = 0;  // sound does not get quiter farther than max from the listener
		INPUT/OUTPUT:
			Ptr to the SINGLE max distance for the sound.

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the DSound
			default, DS3D_DEFAULTMAXDISTANCE ~ infinite, will be used).  

		NOTES:
			The sound volume stops decreasing at max distance.  If the archetype of the sound was created
			with the SM_MUTE_3D_AT_MAX_DISTANCE flag specified (which is included in the default, 
			SM_ARCHETYPE_DEFAULT) the sound should be inaudible	beyond max distance.

	GENRESULT COMAPI get_cone_angles(DWORD *insideAngle, DWORD *outsideAngle)
		INPUT/OUTPUT:
			insideAngle: Ptr to the DWORD angle for the inner cone
			outsideAngle: Ptr to the DWORD angle for the outer cone

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the DSound
			default, 360, will be used for both).  

		NOTES:
			Similiar to min and max distance, the sound will become quiter as the listener approaches the outer
			cone and louder as the listener approaches the inner cone.  Inside the inner cone the sound is at full
			volume and does not get louder.  Outside the outer cone, the sound will be attenuated by the value returned
			by get_cone_outside_attenuation().

	GENRESULT COMAPI get_cone_orientation(Vector *)
			Ptr to the Vector orientation of the sound cone. 

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the DSound
			default (0,0,0) will be used).  

		NOTES:
			This is the vector representing the direction of the sound.

	GENRESULT COMAPI get_cone_outside_attenuation(SINGLE *) = 0; // attenuation of sound beyond outside cone - within inside cone, attenuation is normal attenuation as set by get_attenuation  between cones the attenuation is scaled between these two volumes
		INPUT/OUTPUT:
			Ptr to the SINGLE attenuation amount for the sound when the listener is outside the outer cone.

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the DSound
			default, -100.0 - silence , will be used).  

		NOTES:
	GENRESULT COMAPI get_velocity(Vector *)
		INPUT/OUTPUT:
			Ptr to the Vector velocity of the sound.

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the SoundManager
			default (0,0,0) will be used).  

		NOTES:
			If the velocity is returnded, doppler effect will be automatically calculated for the sound
			if it available for the sound.

	GENRESULT COMAPI get_reverb_mix(SINGLE *)
		INPUT/OUTPUT:
			Ptr to the SINGLE reverb mix amount for the sound.

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the SoundManager
			default, 0.0 (no reverb), will be used).  

		NOTES:
			This allows the reverb amount set by SoundManager::set_master_reverb to be controlled on a
			per instance basis.
	
	GENRESULT COMAPI get_sound_mode(S32 *) = 0; // mode of 3D sound: disable, normal, or headRelative (= absolute values are automatically updated when listener moves)
		INPUT/OUTPUT:
			Ptr to an S32 specifying the sound mode for this sound.
		
		RETURNS
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the SoundManager
			default, normal , will be used).  

		NOTES:
			If the mode is disabled, SoundManager continues to process the sounds 3D properties, but 
			DSound3D ignores them.  If headRelative is specified, the sound is considered to be relative to
			the head and its coordinates are automatically adjusted with the listener.

	GENRESULT COMAPI get_apply_mode(S32 *)
		INPUT/OUTPUT:
			Ptr to an S32 specifying the sound apply mode for this sound.
		
		RETURNS
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the SoundManager
			default, deferred , will be used).  

		NOTES:
			This determines when to apply any changes made to properties: immediate or deferred.  SoundManager
			currently ignores this and always uses deferred mode.  Immediate immediately applies the new
			sound data (causing DSound to flush its mixing buffers and remix ahead for each sound with this
			mode specified) deferred applies all changes at once causing only one remix.
*/

#ifndef ISOUND_H
#define ISOUND_H

#ifndef DACOM_H
#include "dacom.h"
#endif

#include "typedefs.h"
class Vector;

typedef S32 SOUND_ARCH_INDEX;
const SOUND_ARCH_INDEX SM_INVALID_ARCHETYPE = -1;

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define ISOUNDSOURCE_VERSION 1
#define IID_ISoundSource MAKE_IID("ISoundSource", 1)
//********************************************************************************************
struct ISoundSource : public IDAComponent
{
	virtual U32 get_start_time() = 0;
	virtual bool COMAPI is_on() = 0;
	virtual bool COMAPI is_3D() = 0;
	virtual bool COMAPI is_looping() = 0;
	virtual SOUND_ARCH_INDEX get_archetype() = 0;
	virtual GENRESULT COMAPI get_attenuation(SINGLE *) = 0;
	virtual GENRESULT COMAPI get_frequency(SINGLE *) = 0;
	virtual GENRESULT COMAPI get_pan(S32 *) = 0;
	virtual GENRESULT COMAPI get_position(Vector *) = 0;
	virtual GENRESULT COMAPI get_min_distance(SINGLE *) = 0;
	virtual GENRESULT COMAPI get_max_distance(SINGLE *) = 0;
	virtual GENRESULT COMAPI get_cone_angles(DWORD *insideAngle, DWORD *outsideAngle) = 0; 
	virtual GENRESULT COMAPI get_cone_orientation(Vector *) = 0;
	virtual GENRESULT COMAPI get_cone_outside_attenuation(SINGLE *) = 0;
	virtual GENRESULT COMAPI get_velocity(Vector *) = 0;
	virtual GENRESULT COMAPI get_reverb_mix(SINGLE *) = 0; 
	virtual GENRESULT COMAPI get_sound_mode(S32 *) = 0;
	virtual GENRESULT COMAPI get_apply_mode(S32 *) = 0;
};

//----------------------------------------------------------------------------------
//------------------------END ISoundSource.h---------------------------------------------
//----------------------------------------------------------------------------------
#endif

