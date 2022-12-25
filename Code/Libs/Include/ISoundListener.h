//--------------------------------------------------------------------------//
//                                                                          //
//                              ISoundListener.h                             //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Rmarr $
*/

/*
   This header file describes the SoundListener interface used by the Digital Anvil Sound Manager.  
   The Sound Manager provides a simple way to add sounds to an app.  The app implements this interface, 
   provides a list of sounds to play, implementing the ISoundSource interface for the sounds, and calls 
   update periodically causing Sound Manager to play the sounds.  Sound Manager handles all of the buffer
   creation and management using DirectSound, allows the user to set EAX and A3D properties if desired, 
   and supports DirectSound3D.

   //-----------------------------
   // ISoundListener Methods
   //-----------------------------

	GENRESULT COMAPI get_ear_orientation(Vector *back, Vector *up)
		INPUT/OUPUT:
			back: ptr to vector representing the back (-front) direction of the listener.
			up: ptr to vector representing the up direction of the listener.
			
		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the SoundManager
			default, top(0,0,1) and front(0,1,0), will be used).  

	GENRESULT COMAPI get_ear_position(Vector *position)
		INPUT/OUPUT:
			position: ptr to the Vector position of the listener.

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the DSound
			default, top(0,0,0), will be used).  

	GENRESULT COMAPI get_ear_velocity(Vector *velocity)
		INPUT/OUPUT:
			velocity: ptr to the Vector velocity of the listener.

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the DSound
			default, (0,0,0), will be used).  

	GENRESULT COMAPI get_ear_distance_factor(SINGLE *distance)
		INPUT/OUPUT:
			distance: ptr to the SINGLE distance factor of the listener.

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the DSound
			default, 1.0, will be used).  

		NOTES:
			All internal SoundManager calculations are based on units of meters.  If the app uses some
			other units, this specifies the number of units in 1 meter.

	GENRESULT COMAPI get_ear_doppler_factor(SINGLE *doppler)
		INPUT/OUPUT:
			doppler: ptr to the SINGLE doppler factor of the listener. Valid range is between 0.0 and 10.0.

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the DSound
			default, 1.0, will be used).  

		NOTES:
			This value determines how much/little doppler is applied to sounds.  1.0 is the normal amount.

	GENRESULT COMAPI get_ear_rolloff_factor(SINGLE *rolloff)
		INPUT/OUPUT:
			rolloff: ptr to the SINGLE rolloff factor of the listener. Valid range is between 0.0 and 10.0.

		RETURNS:
			GR_OK is returning a value, GR_NOT_IMPLEMENTED if not returning a value (the DSound
			default, 1, will be used).  

		NOTES:
			This value determines how the sounds attenuate based on distance.  1.0 is the normal amount.

*/

#ifndef ISOUNDLISTENER_H
#define ISOUNDLISTENER_H

#ifndef DACOM_H
#include "dacom.h"
#endif

#include "typedefs.h"

class Vector;
class Matrix;

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define ISOUNDLISTENER_VERSION 1
#define IID_ISoundListener MAKE_IID("ISoundListener", 1)
struct ISoundListener : public IDAComponent
{
	virtual GENRESULT COMAPI get_ear_orientation(Vector *back, Vector *up) = 0;
	virtual GENRESULT COMAPI get_ear_position(Vector *position) = 0;
	virtual GENRESULT COMAPI get_ear_velocity(Vector *velocity) = 0;
	virtual GENRESULT COMAPI get_ear_distance_factor(SINGLE *distance) = 0;
	virtual GENRESULT COMAPI get_ear_doppler_factor(SINGLE *doppler) = 0;
	virtual GENRESULT COMAPI get_ear_rolloff_factor(SINGLE *rolloff) = 0;
};

//----------------------------------------------------------------------------------
//------------------------END ISoundListener.h---------------------------------------------
//----------------------------------------------------------------------------------

#endif