//--------------------------------------------------------------------------//
//                                                                          //
//                                Sound.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Conquest/Libs/Include/Sound.h 1     11/18/99 12:52p Rmarr $
*/			    
//--------------------------------------------------------------------------//

#ifndef SOUND_H
#define SOUND_H

#ifndef DACOM_H
#include "dacom.h"
#endif


typedef S32 INSTANCE_INDEX;			// also defined in Engine.h, ObjNode.h
class Vector;
typedef void * HANDLE;

struct ISound : public IDAComponent
{
//
// Object state access functions.
//
	virtual SINGLE COMAPI get_master_volume  () const = 0;

	virtual SINGLE COMAPI get_volume(INSTANCE_INDEX object) const = 0;
	
//
// Object state modification functions.
//
	virtual void COMAPI set_ear_position(const Vector&) = 0;

	virtual void COMAPI set_master_volume(SINGLE volume) = 0;

	virtual void COMAPI set_volume(INSTANCE_INDEX object, SINGLE volume) = 0;

	virtual void COMAPI play	(INSTANCE_INDEX object) = 0;
	virtual void COMAPI stop	(INSTANCE_INDEX object) = 0;
	virtual void COMAPI resume	(INSTANCE_INDEX object) = 0;

};

//----------------------------------------------------------------------------------
//------------------------END Sound.h---------------------------------------------
//----------------------------------------------------------------------------------

#endif