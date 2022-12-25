#ifndef DTROOPSHIP_H
#define DTROOPSHIP_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DTroopship.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DTroopship.h 13    7/21/00 4:37p Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef DSHIPSAVE_H
#include "DShipSave.h"
#endif

enum TSHIP_NET_COMMANDS
{
	TSHIP_CANCEL,
	TSHIP_APPROVED,
	TSHIP_SUCCESS
};
//----------------------------------------------------------------
//
struct BASE_TROOPSHIP_SAVELOAD
{
	__hexview U32 dwTargetID;
	U32 attackAgentID;
	bool bTakeoverApproved;
};
//----------------------------------------------------------------
//
#ifndef _ADB
#define TROOPSHIP_SAVELOAD _GSTL
#endif
struct TROOPSHIP_SAVELOAD : SPACESHIP_SAVELOAD
{
	BASE_TROOPSHIP_SAVELOAD	troopSaveLoad;
};
//----------------------------------------------------------------
//
struct TROOPPOD_SAVELOAD
{
	__hexview U32 dwTargetID;
};
#endif