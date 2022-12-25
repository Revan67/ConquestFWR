#ifndef __ICHANNEL_H
#define __ICHANNEL_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               IChannel.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Rmarr $

   $Header: /Conquest/Libs/Include/ichannel.h 5     4/28/00 11:57p Rmarr $
*/
//--------------------------------------------------------------------------//

#include "animTypes.h"

struct IFileSystem;
struct IChannel2;

typedef unsigned int CHANNEL_INSTANCE_INDEX;

const CHANNEL_INSTANCE_INDEX INVALID_CHANNEL_INSTANCE_INDEX = -1;

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define ICHANNEL_VERSION 1
#define IID_IChannel MAKE_IID("IChannel", 1)

//

struct DACOM_NO_VTABLE IChannel : public IDAComponent
{
	//the newly created channel is guaranteed to have it's playback head placed at time = 0;
	virtual CHANNEL_INSTANCE_INDEX COMAPI create_channel_instance (IChannel2*, const Animation::Target&) = 0;
	virtual CHANNEL_INSTANCE_INDEX COMAPI create_channel_instance (Animation::IVirtualChannel * ctrl, unsigned int data_type, const Animation::Target&) = 0;

	virtual GENRESULT COMAPI destroy_channel_instance (CHANNEL_INSTANCE_INDEX idx) = 0;

	//EMAURER increments ref count of output parm. Release () should be called on IChannel2.
	virtual GENRESULT COMAPI get_channel_archetype (CHANNEL_INSTANCE_INDEX idx, IChannel2*&) = 0;

	virtual bool COMAPI start (CHANNEL_INSTANCE_INDEX idx, Animation::StartParms* parms) = 0;
	virtual bool COMAPI stop (CHANNEL_INSTANCE_INDEX idx) = 0;

	virtual float COMAPI get_current_time (CHANNEL_INSTANCE_INDEX idx) const = 0;

	//EMAURER Animation::BEGIN or END can be used as a parameter here.
	virtual bool COMAPI set_current_time (CHANNEL_INSTANCE_INDEX idx, float time) = 0;

	virtual void COMAPI set_weight(CHANNEL_INSTANCE_INDEX idx, float weight) = 0;
	virtual float COMAPI get_weight(CHANNEL_INSTANCE_INDEX idx) const = 0;

	virtual void COMAPI change_position(CHANNEL_INSTANCE_INDEX idx, const class Vector & p) = 0;
	virtual void COMAPI change_orientation(CHANNEL_INSTANCE_INDEX idx, const class Matrix & R) = 0;

	virtual bool COMAPI channel_in_use(CHANNEL_INSTANCE_INDEX idx) const = 0;
	virtual Animation::Target COMAPI get_channel_target(CHANNEL_INSTANCE_INDEX idx) const = 0;

// SPECIAL-PURPOSE function used by IAnimation::get_root_offset(). Needs to be fleshed out for
// general use.
	virtual void COMAPI adjust_start_position(CHANNEL_INSTANCE_INDEX idx, const Vector & dp) = 0;

	virtual GENRESULT COMAPI is_forward (CHANNEL_INSTANCE_INDEX idx, bool&) const = 0;
};

#endif