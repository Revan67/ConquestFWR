#ifndef MAPGEN_H
#define MAPGEN_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              MapGen.h                                    //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MapGen.h 5     9/01/00 10:49a Sbarton $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include "DACOM.h"
#endif

#ifndef INPROGRESSANIM_H
#include "InProgressAnim.h"
#endif

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IMapGen : public IDAComponent
{
	virtual void GenerateMap (const struct FULLCQGAME & game, U32 seed, IPANIM * ipAnim) = 0;

	virtual U32 GetBestSystemNumber (const FULLCQGAME & game,U32 approxNumber) = 0;

	virtual U32 GetPosibleSystemNumbers (const FULLCQGAME & game, U32 * list) = 0;
};

//---------------------------------------------------------------------------
//-------------------------END MapGen.h--------------------------------------
//---------------------------------------------------------------------------
#endif
