#ifndef DJUMPSAVE_H
#define DJUMPSAVE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DJumpSave.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DJumpSave.h 15    11/06/00 1:26p Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DMBASEDATA_H
#include "DMBaseData.h"
#endif

//----------------------------------------------------------------
//
struct JUMPGATE_SAVELOAD
{
	U32 id,exit_gate_id;
	U32 ownerID;
	bool bLocked;
	SINGLE time_until_last_jump;

	/* mission data */
	MISSION_SAVELOAD mission;

	/* physics data */
	TRANS_SAVELOAD trans_SL;

//	M_STRING	exitGate;
	U8  exploredFlags;
	U8	marks;
	U8  visMarks;
	U8	shadowVisabilityFlags;
	bool bJumpAllowed;
	bool bInvisible;	// used by mission scripts
};

//----------------------------------------------------------------
//

struct JUMPGATE_VIEW
{
	MISSION_SAVELOAD *	mission;
	struct BASIC_INSTANCE *	rtData;
	bool bJumpAllowed;
};

#endif
