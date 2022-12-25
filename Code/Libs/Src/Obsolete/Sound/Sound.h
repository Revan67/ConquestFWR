//--------------------------------------------------------------------------//
//                                                                          //
//                                Sound.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /tools/objview/Sound.h 1     3/10/98 5:47p Gboswood $
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

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define ISOUND_VERSION 1
#define IID_ISound MAKE_IID("ISound", 1)

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