#ifndef NETPACKET_H
#define NETPACKET_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               NetPacket.h                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

   $Header: /Conquest/App/Src/NetPacket.h 14    8/23/01 1:53p Tmauer $
	
   Centralized place for receiving, dispatching packets

*/
//--------------------------------------------------------------------------//
//							INetPacket Documentation
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

struct IBaseObject;

typedef DWORD DPID, FAR *LPDPID;
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
// enum PACKET_TYPE is now defined in Globals.h
//
//--------------------------------------------------------------------------//
//
#define PACKETTYPEBITS  7
#define PACKETUSERBITS  1
#define PACKETCTRLBITS	1
#define PACKETTIMEBITSR	(PACKETTYPEBITS + PACKETUSERBITS + PACKETCTRLBITS)
#define PACKETTIMEBITS	(32 - PACKETTIMEBITSR)

struct BASE_PACKET
{
	DWORD		dwSize;
	DPID		fromID;
	PACKET_TYPE type    : PACKETTYPEBITS;
	U32			userBits : PACKETUSERBITS;
	U32			ctrlBits : PACKETCTRLBITS;
	U32			timeStamp : PACKETTIMEBITS;
	U16			sequenceID, ackID;

	BASE_PACKET (void)
	{
		memset(this, 0, sizeof(*this));
	}
};
//--------------------------------------------------------------------------//
// FLAGS to use in Send(), GetPacketsInTransit() method
//
#define NETF_ALLREMOTE		0x00000100		// send to all remote players (idTo is ignored)
#define NETF_ECHO			0x00000200		// send to self (idTo is ignored)

//--------------------------------------------------------------------------//
//
#define NPPI_HOST		0x00000001
#define NPPI_PAUSED		0x00000002
#define NPPI_TURTLED	0x00000004
struct NP_PLAYERINFO
{
	DWORD dwFlags;
	DPID  dpid;
	const wchar_t * szPlayerName;
	S32   bootTime;			// milleseconds until boot
};
//--------------------------------------------------------------------------//
//
struct IPlayerStateCallback
{
	// return TRUE to continue enumeration
	virtual bool EnumeratePlayer (struct NP_PLAYERINFO & info, void * context) = 0;
};
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE INetPacket : public IDAComponent
{
	virtual void __stdcall Send (DPID idTo, DWORD dwFlags, const BASE_PACKET * packet) = 0;

	virtual int __stdcall GetPacketsInTransit (DPID idTo, DWORD dwFlags) const = 0;

	//return true if enough bandwidth to send a low priority packet
	//returns the size of the largest low-priority packet that can be allowed
	virtual int __stdcall TestLowPrioritySend (U32 packetSize) = 0;
	
	virtual int GetNumHosts (void) = 0;	// returns number of computers attached to session (including local machine)

	virtual int __stdcall StallPacketDelivery (bool bStall) = 0;

	// only valid for remote machines
	virtual U32 GetPauseTimeForPlayer (DPID dpID) = 0;		// return pause time used in msecs

	// returns TRUE if local player has been told not to pause game anymore
	virtual bool HasPauseWarningBeenReceived (void) = 0;	

	// returns amount of time (in mseconds) before player is booted. (0 == nobody available for booting)
	// returns *dpid is set to dpid of player to be booted.
	// returns *paused == true if player has paused the game.
	virtual U32 GetTimeUntilBooting (DPID * dpid, bool * paused = 0) = 0;

	virtual void DEBUG_print (struct IDebugFontDrawAgent * DEBUGFONT) = 0;

	// signal to turn off guaranteed delivery, it's not going to work on this machine
	virtual void OnGuaranteedDeliveryFailure (void) = 0;

	virtual bool EnumeratePlayers (IPlayerStateCallback * callback, void * context) = 0;
};


//---------------------------------------------------------------------------------//
//--------------------------------End NetPacket.h----------------------------------//
//---------------------------------------------------------------------------------//

#endif