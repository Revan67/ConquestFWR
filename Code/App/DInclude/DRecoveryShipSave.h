#ifndef DRECOVERYSHIPSAVE_H
#define DRECOVERYSHIPSAVE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DRecoveryShipSave.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DRecoveryShipSave.h 2     11/22/99 1:43p Tmauer $
*/			    
//--------------------------------------------------------------------------//
#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#ifndef DSHIPSAVE_H
#include "DShipSave.h"
#endif

//----------------------------------------------------------------
//
struct REC_SAVELOAD		// template code save structure
{
};
//----------------------------------------------------------------
//
struct BASE_RECOVERYSHIP_SAVELOAD	 // leaf class's save structure
{
	enum REC_MODES
	{
		MOVING_TO_RECOVERY,
		RECOVERING,
		TOWING
	}mode;
	U32 recoveryTargetID;
	SINGLE recoverTimer;
};
//----------------------------------------------------------------
//
struct RECOVERYSHIP_SAVELOAD : SPACESHIP_SAVELOAD		// structure written to disk
{
	BASE_RECOVERYSHIP_SAVELOAD  baseSaveLoad;
	REC_SAVELOAD rec_SL;
};

//----------------------------------------------------------------
//

#endif
