#ifndef NETBUFFER_H
#define NETBUFFER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              NetBuffer.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

   $Header: /Conquest/App/Src/NetBuffer.h 9     9/13/01 10:01a Tmauer $
	
   1) Buffering for network.
   2) Simulate latency
   3) Calculate latency, tickOffset

*/
//--------------------------------------------------------------------------//
//							INetBuffer Documentation
//--------------------------------------------------------------------------//

/*
	//------------------------------
	//
	BOOL32 NetBuffer::Initialize (void);
		RETURNS:
			TRUE if succeeded, FALSE on error.
		OUTPUT:
			Initializes internal data structures. Can be called more than once during the
			lifetime of the object. 
		NOTES:
			You should call this method before starting a new network connection to make sure 
			all references to any previous sessions are flushed.

	//------------------------------
	//
	BOOL32 NetBuffer::Send (DPID idTo, DWORD dwFlags, const BASE_PACKET * packet);
		INPUT:
			idTo: DirectPlay ID of remote player to send a packet.
			dwFlags: Defined by DirectPlay API. (Also see DPlay.h, IDirectPlay3::Send)
				DPSEND_GUARANTEED: Send the message using a guaranteed send method.
								   Default is non-guaranteed.
				DPSEND_SIGNED:	   Send the message digitally signed to ensure authenticity.
				DPSEND_ENCRYPTED:  Send the message with encryption to ensure privacy.
			packet: Address of packet to send to remote machine.
				The 'dwSize' and 'type' members of the packet must be set before calling this method.
		RETURNS:
			TRUE if the message was sent, FALSE on failure.
		OUTPUT:
			Sets the 'fromID' and 'timeStamp' fields of the packet and sends the packet to the remote player.

	//------------------------------
	//
	BOOL32 NetBuffer::Receive (DPID idFrom, DWORD dwFlags,	BASE_PACKET ** packet);
		INPUT:
			idFrom: Specific DirectPlay player ID to receive a message from. This parameter is ignored unless
					DPRECEIVE_FROMPLAYER flag is set in 'dwFlags' parameter.
			dwFlags: Defined by DirectPlay API. (Also see DPlay.h, IDirectPlay3::Receive)
				DPRECEIVE_ALL:			Get the first message in the queue.
				DPRECEIVE_FROMPLAYER:	Get the first message in the queue from a specific player.
				DPRECEIVE_PEEK:			Get the message but don't remove it from the queue.
			packet: Address of a pointer that will be set to point to received packet.
		RETURNS:
			TRUE if a packet was received, FALSE if no packet was received.
		OUTPUT:
			Retrieves a packet waiting on the input queue, and returns a pointer to the packet. 
		NOTES: Memory for the packet is allocated by the NetBuffer. You should call the
			   FreePacket() method when you are done processing the packet. Failure to free 
			   packets will eventually cause the NetBuffer to run out of internal memory.
				
	//------------------------------
	//
	BOOL32 NetBuffer::FreePacket (BASE_PACKET * packet);
		INPUT:
			packet: Address of a packet retrieved from a call to Receive().
		RETURNS:
			TRUE if the packet's memory was properly deallocated.
			FALSE if the packet address was invalid.
		OUTPUT:
			Releases memory allocated for the packet within the NetBuffer's internal buffer.
		NOTES:
			Applications should call this method after processing a packet received from Receive().
			On return from this method, the memory pointed to by 'packet' is invalid, and should
			not be used.

	//------------------------------
	//
	void NetBuffer::SetMinLatency (U32 minLatency);	// in msec
		INPUT:
			minLatency: Specifies the minimum amount of time (in msecs) that a packet should
						sit in the incoming message queue before it can be received.
		NOTES:
			Minimum latency is used to simulate a slow network connection. The default minimum
			latency is 0, so packets can be received immediately. When latency is non-zero, packets
			are held in the buffer until they at least 'minLatency' milliseconds old. (The Receive()
			method will overlook messages in the buffer until they are old enough for the 
			application to actually receive them.)

	//------------------------------
	//
	BOOL32 NetBuffer::SyncPlayer (DPID playerID);
		INPUT:
			playerID: DirectPlay playerID of a remote player. 'playerID' == 0 is interpreted as 
					  meaning all players
		RETURNS:
			TRUE if the player ID is valid.
		OUTPUT:
			Restarts the process of determining the latency of the connection to the remote player. It
			may take a few seconds to determine the latency of a remote connection.


	//------------------------------
	//
	SYNC_RESULT NetBuffer::GetLatency (DPID playerID, U32 * latency);
		INPUT:
			playerID: DirectPlay playerID of a remote player. 'playerID' == 0 is interpreted as 
					  meaning all players
			latency: Address of value that will be set to the latency of the connection.
		RETURNS:
			SR_SUCCESS:		Returned latency value is accurate.
			SR_INVALID:		Invalid player ID specified.
			SR_INPROGRESS:	Returned latency value is a work-in-progress value, check back later.
		NOTES:
			The latency value returned is the actual latency of the connection, and is not affected by the
			simulated latency set by SetMinLatency().

	//------------------------------
	//
  	U32 NetBuffer::GetPacketAge (const BASE_PACKET *packet);
		INPUT:
			packet: Address of packet received from a remote player.
		RETURNS:
			Amount of real time (in milliseconds) that has elapsed since the message was sent
			by the remote player.

*/
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

typedef DWORD DPID, FAR *LPDPID;

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
enum SYNC_RESULT
{
	SR_SUCCESS,
	SR_INVALID,
	SR_INPROGRESS
};
struct BASE_PACKET;
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE INetBuffer : public IDAComponent
{
	virtual BOOL32 __stdcall Initialize (void) = 0;

	virtual void __stdcall Send (DPID idTo, DWORD dwFlags, const BASE_PACKET * packet) = 0;

	virtual BOOL32 __stdcall Receive (DPID idFrom, DWORD dwFlags,	BASE_PACKET ** packet) = 0;

	virtual BOOL32 __stdcall FreePacket (BASE_PACKET * packet) = 0;

	virtual void __stdcall SetMinLatency (U32 minLatency) = 0;	// in msec
	
	virtual void __stdcall SetPacketLoss (U32 percentLost) = 0;	// range from 0 to 100

	virtual void __stdcall SetMaxBandwidth (const GUID & guid, bool bLAN) = 0;	// depends on connection type

	virtual void __stdcall SetTCPNetworkPerformance(bool bHighSpeed) = 0;

	virtual BOOL32 __stdcall SyncPlayer (DPID playerID) = 0;

	virtual SYNC_RESULT __stdcall GetLatency (DPID playerID, U32 * latency) = 0;

	virtual U32 __stdcall GetPacketAge (const BASE_PACKET *packet) = 0;

	virtual U32 __stdcall GetThroughput (void) = 0;

	virtual bool __stdcall TestPacketSend (U32 packetSize) = 0;		//return true if enough bandwidth to send a packet

	virtual bool __stdcall TestFTPSend (U32 packetSize) = 0;		//return true if enough bandwidth to send FTP packet

	virtual U32 __stdcall GetSessionThroughput (void) = 0;	// get lowest common throughput of connected users

	virtual void __stdcall EnableThroughputLimiting (bool bEnable) = 0;	// default is TRUE

	virtual U32 __stdcall GetChecksumForPlayer (DPID playerID) = 0;

	virtual void __stdcall DestroyPlayer (DPID playerID) = 0;

	virtual U32 __stdcall GetHostTick (void) = 0;
};





//----------------------------------------------------------------------------//
//-------------------------------End NetBuffer.h------------------------------//
//----------------------------------------------------------------------------//
#endif