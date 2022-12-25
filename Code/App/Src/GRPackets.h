#ifndef GRPACKETS_H
#define GRPACKETS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              GRPackets.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/GRPackets.h 9     11/08/00 3:18p Jasony $
*/			    
//--------------------------------------------------------------------------//
//

#ifndef NETPACKET_H
#include "NetPacket.h"
#endif

#ifndef USERDEFAULTS_H
#include "UserDefaults.h"
#endif

#ifndef DCQGAME_H
#include <DCQGAME.h>
#endif

//--------------------------------------------------------------------------//
//
enum GR_TYPE
{
	GAMEDESC,
	MAPNAME,
	CLIENT,
	STARTDOWNLOAD,
	DOWNLOADCOMPLETE,
	INITGAME,
	INITCOMPLETE,
	GAMEREADY,
	CANCELLOAD,
	GRCHAT
};
//--------------------------------------------------------------------------//
//
struct GR_PACKET : BASE_PACKET
{
	GR_TYPE subtype;

	GR_PACKET (GR_TYPE newtype)
	{
		type = PT_GAMEROOM;
		subtype = newtype;
		dwSize = sizeof(*this);
	}
};
//--------------------------------------------------------------------------//
//
struct START_PACKET : GR_PACKET
{
	USER_DEFAULTS userDefaults;
	CQGAMETYPES::_CQGAME  cqgame;
	U32 checkSum, randomSeed;

	START_PACKET (void) : GR_PACKET(STARTDOWNLOAD)
	{
		checkSum = randomSeed = 0;
		dwSize = sizeof(*this);
	}
};

#endif