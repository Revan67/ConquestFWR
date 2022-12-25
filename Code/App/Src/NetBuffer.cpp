//--------------------------------------------------------------------------//
//                                                                          //
//                             NetBuffer.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

   $Header: /Conquest/App/Src/NetBuffer.cpp 57    9/13/01 10:01a Tmauer $

	
   1) Buffering for network.
   2) Simulate latency
   3) Calculate latency, tickOffset

*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>


#include "NetBuffer.h"
#include "NetPacket.h"
#include "Startup.h"
#include "UserDefaults.h"
#include "CQTrace.h"

#include <IProfileParser.h>
#include <TComponent.h>
#include <TSmartPointer.h>
#include <Heapobj.h>
#include <EventSys.h>

#include <dplobby.h>

#ifdef FINAL_RELEASE
#define SILENCE_TRACE
#endif

#ifdef SILENCE_TRACE

#define NETPRINT0(exp) ((void)0)
#define NETPRINT1(exp,p1) ((void)0)
#define NETPRINT2(exp,p1,p2) ((void)0)
#define NETPRINT3(exp,p1,p2,p3) ((void)0)
#define NETPRINT4(exp,p1,p2,p3,p4) ((void)0)
#define NETPRINT5(exp,p1,p2,p3,p4,p5) ((void)0)
#define NETPRINT6(exp,p1,p2,p3,p4,p5,p6) ((void)0)

#else	//  !FINAL_RELEASE

#define NETPRINT0(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "[%5d] "##exp"\r\n", NETBUFFER->GetHostTick())
#define NETPRINT1(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "[%5d] "##exp"\r\n", NETBUFFER->GetHostTick(), p1)
#define NETPRINT2(exp,p1,p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "[%5d] "##exp"\r\n", NETBUFFER->GetHostTick(), p1, p2)
#define NETPRINT3(exp,p1,p2,p3) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "[%5d] "##exp"\r\n", NETBUFFER->GetHostTick(), p1, p2, p3)
#define NETPRINT4(exp,p1,p2,p3,p4) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "[%5d] "##exp"\r\n", NETBUFFER->GetHostTick(), p1, p2, p3, p4)
#define NETPRINT5(exp,p1,p2,p3,p4,p5) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "[%5d] "##exp"\r\n", NETBUFFER->GetHostTick(), p1, p2, p3, p4, p5)
#define NETPRINT6(exp,p1,p2,p3,p4,p5,p6) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "[%5d] "##exp"\r\n", NETBUFFER->GetHostTick(), p1, p2, p3, p4, p5, p6)

#endif //  !FINAL_RELEASE

#define HEAP_SIZE			  0x10000			// amount of memory allocated for buffering
#define MAX_MESSAGE_SIZE  	  500				// max allowable size of a packet
#define SYNC_SAMPLES		  10				// number of samples needed to complete the test
#define SYNC_PERIOD			  300				// time (msec) between sending SYNC messages
#define NUM_BINS			  16
#define THROUGHPUT_PERIOD	  (1000	/ NUM_BINS)	// time between throughput measurements

#define DEFAULT_BANDWIDTH	  2800				// 2800 bytes per second
#define MAX_BANDWIDTH		  15000				// max user configurable bandwidth
#define MAX_LOCAL_BANDWIDTH   15000
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct SUPERBASE_PACKET
{
	struct SUPERBASE_PACKET * pNext;
	DWORD					  dwSize;
	DPID					  fromID;
	PACKET_TYPE type    : PACKETTYPEBITS;
	U32			userBits : PACKETUSERBITS;
	U32			ctrlBits : PACKETCTRLBITS;
	U32			timeStamp : PACKETTIMEBITS;
	U16						  sequenceID, ackID;
};
//--------------------------------------------------------------------------//
//
#pragma pack (push, 4)
struct PLAYER_NODE
{
	struct PLAYER_NODE *	pNext;
	DPID					playerID;
	U32						tickOffset;
	U32						avgLatency;
	U32						samplesReceived;
	U32						maxThroughput;
	U32						checkSum;

	__int64					l64Accum;
	long					lMinOffset;
	long					lMaxOffset;

	void * operator new (size_t size)
	{
		return heap->ClearAllocateMemory(size, "NetBuffer");
	}

	void operator delete (void * ptr)
	{
		heap->FreeMemory(ptr);
	}

	static IHeap * heap;
};
IHeap * PLAYER_NODE::heap;
#pragma pack ( pop )
//--------------------------------------------------------------------------//
//
struct SYNC_PACKET : BASE_PACKET
{
	U32 localTick;
	U32 remoteTick;
	U32 extraLag;		// time spent in remote machine's "InBox"
	U32 maxThroughput;	// tell remote player about our limit
	U32 checkSum;		// used to detect cheating
};

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE NetBuffer : public INetBuffer
{
	BEGIN_DACOM_MAP_INBOUND(NetBuffer)
	DACOM_INTERFACE_ENTRY(INetBuffer)
	END_DACOM_MAP()

	//--------------------
	// data members
	//--------------------

	COMPTR<IHeap> heap;
	U32 startingTick, currentTick, updateTick, syncTick, throughputTick;		
	U32 minLatency;
	U32 percentLost;	// 0 to 256
	U32 maxBandwidth, TCPBandwidth, LANBandwidth;	// bytes per second
	U32 throughputBin[NUM_BINS], currentBin;
	SUPERBASE_PACKET * pPacketList, *pPacketListEnd;
	BASE_PACKET * pHolding;
	PLAYER_NODE	* pPlayerList;
	bool bEnableLimiting;		// limit bandwidth to maxBandwidth

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	NetBuffer (void);

	~NetBuffer (void)
	{
		free(pHolding);
		pHolding = 0;

		reset();
	}
	
	/* INetBuffer methods */

	virtual BOOL32 __stdcall Initialize (void);

	virtual void __stdcall Send (DPID idTo, DWORD dwFlags, const BASE_PACKET * packet);

	virtual BOOL32 __stdcall Receive (DPID idFrom, DWORD dwFlags,	BASE_PACKET ** packet);

	virtual BOOL32 __stdcall FreePacket (BASE_PACKET * packet);

	virtual void __stdcall SetMinLatency (U32 minLatency);	// in msec
	
	virtual void __stdcall SetPacketLoss (U32 percentLost);	// range from 0 to 100

	virtual void __stdcall SetMaxBandwidth (const GUID & guid, bool bLAN);	// depends on connection type

	virtual void __stdcall SetTCPNetworkPerformance(bool bHighSpeed);

	virtual BOOL32 __stdcall SyncPlayer (DPID playerID);

	virtual SYNC_RESULT __stdcall GetLatency (DPID playerID, U32 * latency);

	virtual U32 __stdcall GetPacketAge (const BASE_PACKET *packet);

	virtual U32 __stdcall GetThroughput (void);

	virtual bool __stdcall TestPacketSend (U32 packetSize);		//return true if enough bandwidth to send a packet

	virtual bool __stdcall TestFTPSend (U32 packetSize);		//return true if enough bandwidth to send FTP packet

	virtual U32 __stdcall GetSessionThroughput (void);	// get lowest common throughput of connected users

	virtual void __stdcall EnableThroughputLimiting (bool bEnable);	// default is TRUE

	virtual U32 __stdcall GetChecksumForPlayer (DPID playerID);

	virtual void __stdcall DestroyPlayer (DPID playerID);

	virtual U32 __stdcall GetHostTick (void);

	/* NetBuffer methods */

 	void reset (void);

	void update (void);

	BOOL32 useHolding (void);

	PLAYER_NODE * findPlayer (DPID dpid);

	BOOL32 receive (DPID idFrom, DWORD dwFlags,	BASE_PACKET ** packet);

	BOOL32 syncPlayers (void);

	void handleSyncMsg (SYNC_PACKET * pPacket);

	void handleSystemMessages (DPMSG_GENERIC * pPacket);

	void sendToSelf (BASE_PACKET * packet);

	void readProfile (void);

	BOOL32 randomPacketLoss (void)
	{
		return (U32(rand() & 255) < percentLost);
	}

	U32 setTick (void)
	{
		return (currentTick = GetTickCount() - startingTick);
	}

	bool isSysMsg (BASE_PACKET * packet)
	{
		return (PLAYERID != DPID_SYSMSG && packet->fromID == DPID_SYSMSG);
	}

	bool isSysMsg (SUPERBASE_PACKET * packet)
	{
		return (PLAYERID != DPID_SYSMSG && packet->fromID == DPID_SYSMSG);
	}

	bool testSend (U32 packetSize)
	{
		return (bEnableLimiting==0 || (GetThroughput() + packetSize <= maxBandwidth));
	}


	IDAComponent * getBase (void)
	{
		return (INetBuffer *) this;
	}
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
NetBuffer::NetBuffer (void)
{
	DAHEAPDESC hdesc;

	hdesc.heapSize = HEAP_SIZE;
	hdesc.flags    = DAHEAPFLAG_PRIVATE | DAHEAPFLAG_NOMSGS;
	DACOM->CreateInstance(&hdesc, heap);
	PLAYER_NODE::heap = heap;
	heap->SetErrorHandler(0);

	pHolding = (BASE_PACKET *) calloc(MAX_MESSAGE_SIZE+sizeof(DWORD)+sizeof(DPID), 1);

	startingTick = GetTickCount() - 1;
	currentTick = 1;
	bEnableLimiting = true;
	TCPBandwidth = maxBandwidth = DEFAULT_BANDWIDTH;
	LANBandwidth = MAX_BANDWIDTH;
	readProfile();
}
//-------------------------------------------------------------------------//
//
BOOL32 NetBuffer::Initialize (void)
{
	reset();
	return 1;
}
//--------------------------------------------------------------------------//
//
void NetBuffer::Send (DPID idTo, DWORD dwFlags, const BASE_PACKET * _packet)
{
	BASE_PACKET * packet = const_cast<BASE_PACKET *>(_packet);

	CQASSERT(_packet->dwSize >= sizeof(BASE_PACKET));

	packet->timeStamp = setTick() >> PACKETTIMEBITSR;
	if (packet->dwSize > MAX_MESSAGE_SIZE)
	{
		CQBOMB2("NetBuffer::Attempt to send %d byte message. LIMIT=%d", packet->dwSize, MAX_MESSAGE_SIZE);
	}

	if (throughputTick == 0)
		throughputTick = currentTick;
	int maxLoops = NUM_BINS;
	while (currentTick - throughputTick >= THROUGHPUT_PERIOD)
	{
		currentBin = (currentBin + 1) % NUM_BINS;
		throughputBin[currentBin] = 0;
		throughputTick += THROUGHPUT_PERIOD;
		if (--maxLoops<=0)
		{
			throughputTick=0;
			break;
		}
	}

	if (PLAYERID == idTo)		// sending a packet to self
	{
		sendToSelf(packet);
		goto Done;
	}

	if (testSend(_packet->dwSize) == false)
	{
		if (CQFLAGS.bTraceNetwork)
			NETPRINT1("Send(): Packet dropped because of bandwidth. SEQ=%d", _packet->sequenceID);
		goto Done;
	}

	if (DPLAY)
	{
			// don't need to send fromID and size
		HRESULT dpResult = DPLAY->SendEx(PLAYERID, idTo, dwFlags, (&packet->dwSize)+2, packet->dwSize-(sizeof(DWORD)+sizeof(DPID)),
				0,0,NULL,NULL);
		if (SUCCEEDED(dpResult) || dpResult==DPERR_PENDING)
			throughputBin[currentBin] += packet->dwSize-sizeof(DWORD)-sizeof(DPID);
		else
		if (dpResult == DPERR_UNSUPPORTED && dwFlags!=0)
		{
			NETPACKET->OnGuaranteedDeliveryFailure();
			Send(idTo, 0, _packet);
		}
		else
		if (CQFLAGS.bTraceNetwork)
			NETPRINT1("DPLAY::SendEx() returned error 0x%X", dpResult);
	}

Done:
	return;
}
//--------------------------------------------------------------------------//
//
BOOL32 NetBuffer::Receive (DPID idFrom, DWORD dwFlags,	BASE_PACKET ** packet)
{
	BOOL32 result = 0, bWasEmpty;

	setTick();
	bWasEmpty = (pPacketList == 0);
	result = receive(idFrom, dwFlags, packet);

	update();	// receive more packets

	// if failed to read a packet the first time, try again
	if (result == 0 && bWasEmpty && pPacketList)
		result = receive(idFrom, dwFlags, packet);

	if (result && isSysMsg(*packet))
		handleSystemMessages((DPMSG_GENERIC *)(&((*packet)->dwSize) + 2));

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 NetBuffer::FreePacket (BASE_PACKET * packet)
{
	SUPERBASE_PACKET * tmp = (SUPERBASE_PACKET *) (((C8 *)packet) - sizeof(SUPERBASE_PACKET *));

	if (tmp->pNext)
		_localprintf("%s(%d) : WARNING! Invalid packet freed.", __FILE__, __LINE__);

	return heap->FreeMemory(tmp);
}
//--------------------------------------------------------------------------//
//
void NetBuffer::SetMinLatency (U32 _minLatency)
{
	minLatency = _minLatency;
	DEFAULTS->GetDefaults()->minLatency = minLatency;
}
//--------------------------------------------------------------------------//
//
void NetBuffer::SetPacketLoss (U32 _percentLost)
{
	if (_percentLost > 100)
		_percentLost = 100;
	DEFAULTS->GetDefaults()->packetLossPercent = _percentLost;

	percentLost = (_percentLost * 256) / 100;
}
//--------------------------------------------------------------------------//
//
void NetBuffer::SetMaxBandwidth (const GUID & guid, bool bLAN)
{
	if (IsEqualGUID(guid, DPSPGUID_SERIAL) || IsEqualGUID(guid, DPSPGUID_MODEM) )
		maxBandwidth = DEFAULT_BANDWIDTH;
	else
	if (bLAN)
		maxBandwidth = LANBandwidth;
	else
		maxBandwidth = TCPBandwidth;
}
//--------------------------------------------------------------------------//
//
void NetBuffer::SetTCPNetworkPerformance(bool bHighSpeed)
{
	if(bHighSpeed)
		TCPBandwidth = MAX_BANDWIDTH;
	else
		TCPBandwidth = DEFAULT_BANDWIDTH;
}
//--------------------------------------------------------------------------//
//
BOOL32 NetBuffer::SyncPlayer (DPID playerID)
{
	if (playerID)
	{
		PLAYER_NODE * pNode = findPlayer(playerID);

		if (pNode)
		{
			pNode->samplesReceived = 0;
			pNode->l64Accum = 0;
			pNode->lMinOffset = 0x7FFFFFFF;
			pNode->lMaxOffset = 0x80000000;
			return 1;
		}

		return 0;
	}
	else // do this for all players
	{
		PLAYER_NODE * pNode = pPlayerList;

		while (pNode)
		{
			pNode->samplesReceived = 0;
			pNode->l64Accum = 0;
			pNode->lMinOffset = 0x7FFFFFFF;
			pNode->lMaxOffset = 0x80000000;
			pNode = pNode->pNext;
		}

		return 1;
	}
}
//--------------------------------------------------------------------------//
//
U32 NetBuffer::GetChecksumForPlayer (DPID playerID)
{
	PLAYER_NODE * pNode = findPlayer(playerID);
	U32 result = 0;

	if (pNode)
		result = pNode->checkSum;

	return result;
}
//--------------------------------------------------------------------------//
//
SYNC_RESULT NetBuffer::GetLatency (DPID playerID, U32 * latency)
{
	if (playerID)
	{
		SYNC_RESULT result = SR_INVALID;
		PLAYER_NODE * pNode = findPlayer(playerID);

		if (pNode)
		{
			*latency = pNode->avgLatency;
			result = (pNode->samplesReceived >= SYNC_SAMPLES) ? SR_SUCCESS : SR_INPROGRESS;
		}

		return result;
	}
	else // do this for all players
	if (pPlayerList)
	{
		SYNC_RESULT result = SR_SUCCESS;
		PLAYER_NODE * pNode = pPlayerList;
		U32 cumLatency=0, numPlayers=0;

		while (pNode)
		{
			cumLatency += pNode->avgLatency;
			numPlayers++;
			if (pNode->samplesReceived < SYNC_SAMPLES)
				result = SR_INPROGRESS;
			pNode = pNode->pNext;
		}

		if (numPlayers)
			*latency = (cumLatency / numPlayers);
		else
			*latency = 0;

		return result;
	}
	else
		return SR_INVALID;
}
//--------------------------------------------------------------------------//
//
U32 NetBuffer::GetPacketAge (const BASE_PACKET *packet)
{
	return setTick() - (packet->timeStamp << PACKETTIMEBITSR);
}
//--------------------------------------------------------------------------//
//
U32 NetBuffer::GetThroughput (void)
{
	int i; 
	U32 sum=0;

	for (i = 0; i < NUM_BINS; i++)
		sum += throughputBin[i];

	return sum;
}
//--------------------------------------------------------------------------//
// return true if enough bandwidth to send packet
//
bool NetBuffer::TestFTPSend (U32 packetSize)
{
	setTick();
	
	if (throughputTick == 0)
		throughputTick = currentTick;
	int maxLoops = NUM_BINS;
	while (currentTick - throughputTick >= THROUGHPUT_PERIOD)
	{
		currentBin = (currentBin + 1) % NUM_BINS;
		throughputBin[currentBin] = 0;
		throughputTick += THROUGHPUT_PERIOD;
		if (--maxLoops<=0)
		{
			throughputTick=0;
			break;
		}
	}

	U32 t = (bEnableLimiting==0) ? MAX_LOCAL_BANDWIDTH : GetSessionThroughput();
	U32 r = GetThroughput() + 256;		// leave some room for regular packets

	if (pPlayerList && pPlayerList->pNext==0)  // if only 1 remote player
		t = (t * 3) / 4;

	return (r + packetSize <= t);
}
//--------------------------------------------------------------------------//
//
bool NetBuffer::TestPacketSend (U32 packetSize)
{
	setTick();
	
	if (throughputTick == 0)
		throughputTick = currentTick;
	int maxLoops = NUM_BINS;
	while (currentTick - throughputTick >= THROUGHPUT_PERIOD)
	{
		currentBin = (currentBin + 1) % NUM_BINS;
		throughputBin[currentBin] = 0;
		throughputTick += THROUGHPUT_PERIOD;
		if (--maxLoops<=0)
		{
			throughputTick=0;
			break;
		}
	}

	U32 t = (bEnableLimiting==0) ? MAX_LOCAL_BANDWIDTH : GetSessionThroughput();
	U32 r = GetThroughput();

	if (pPlayerList && pPlayerList->pNext==0)  // if only 1 remote player
		t = (t * 3) / 4;

	return (r + packetSize <= t);
}
//--------------------------------------------------------------------------//
//
U32 NetBuffer::GetHostTick (void)
{
	if (HOSTID==PLAYERID)
		return setTick();
	else
	{
		PLAYER_NODE * pNode = findPlayer(HOSTID);

		if (pNode)
			return setTick() - pNode->tickOffset;
		else
			return 0;
	}
}
//--------------------------------------------------------------------------//
// return the min of throughput's of all connected players
//
U32 NetBuffer::GetSessionThroughput (void)
{
	U32 result = maxBandwidth;
	PLAYER_NODE *pNode = pPlayerList;

	if (pNode)
	{
		U32 numPlayers=0;

		while (pNode)
		{
			numPlayers++;
			if (pNode->maxThroughput && pNode->maxThroughput < result)
				result = pNode->maxThroughput;
			pNode = pNode->pNext;
		}

		result *= numPlayers;
		result = __min(maxBandwidth, result);
	}
	
	return result;
}
//--------------------------------------------------------------------------//
//
void NetBuffer::EnableThroughputLimiting (bool bEnable)
{
	bEnableLimiting = bEnable;
}
//--------------------------------------------------------------------------//
//
void NetBuffer::reset (void)
{
	// free the packets in the buffer
	SUPERBASE_PACKET * tmp;
	
	while (pPacketList)
	{
		tmp = pPacketList->pNext;
		heap->FreeMemory(pPacketList);
		pPacketList = tmp;
	}
	pPacketListEnd = 0;
	if (pHolding)
		pHolding->dwSize = 0;

	PLAYER_NODE	* tmp2;
	while (pPlayerList)
	{
		tmp2 = pPlayerList->pNext;
		heap->FreeMemory(pPlayerList);
		pPlayerList = tmp2;
	}

	// testing!!  (should be empty heap at this point)
	if (heap->GetLargestBlock() != heap->GetHeapSize())
		_localprintf("%s(%d) : WARNING!  Internal memory leak detected.\n", __FILE__,__LINE__);

	memset(throughputBin, 0, sizeof(throughputBin));
	maxBandwidth = DEFAULT_BANDWIDTH;
}
//--------------------------------------------------------------------------//
// read all waiting messages into the holding tank
//
void NetBuffer::update (void)
{
	DPID idFrom, idTo;
	DWORD dwDataSize = MAX_MESSAGE_SIZE;

	if (syncTick == 0 || currentTick - syncTick > SYNC_PERIOD)
	{
		if (syncPlayers())		// if we actually sent a message
			syncTick = currentTick;
		else
			syncTick = 0;
	}

	if (throughputTick == 0)
		throughputTick = currentTick;
	int maxLoops = NUM_BINS;
	while (currentTick - throughputTick >= THROUGHPUT_PERIOD)
	{
		currentBin = (currentBin + 1) % NUM_BINS;
		throughputBin[currentBin] = 0;
		throughputTick += THROUGHPUT_PERIOD;
		if (--maxLoops<=0)
		{
			throughputTick=0;
			break;
		}
	}

	if (pHolding->dwSize && useHolding() == 0)	// have a leftover block, but no room at the inn
		goto Done;
	
	while (DPLAY && PLAYERID && DPLAY->Receive(&idFrom, &idTo, DPRECEIVE_ALL, (&pHolding->dwSize)+2, &dwDataSize) == DP_OK)
	{
		throughputBin[currentBin] += dwDataSize;

		pHolding->dwSize = dwDataSize+sizeof(DWORD)+sizeof(DPID);
		pHolding->fromID = idFrom;

		if (isSysMsg(pHolding) == 0)
		{
			switch (pHolding->type)
			{
			case PT_PLAYER_SYNC:
				handleSyncMsg((SYNC_PACKET *)pHolding);
				pHolding->dwSize = 0;		// packet was used
				break;
			default:
				if (randomPacketLoss())
				{
					if (CQFLAGS.bTraceNetwork)
						NETPRINT1("RCV SEQ=%d Dropped randomly", pHolding->sequenceID);
					pHolding->dwSize = 0;		// packet was used
				}
				break;
			}
		}

		if (pHolding->dwSize && useHolding() == 0)  // have a leftover block, but no room at the inn
			goto Done;

		dwDataSize = MAX_MESSAGE_SIZE;
	}

Done:
	updateTick = currentTick;		// remember the last time we were updated
}
//--------------------------------------------------------------------------//
// sending a packet to self, put it in the queue
//
void NetBuffer::sendToSelf (BASE_PACKET * packet)
{
	if (pHolding->dwSize && useHolding() == 0)	// have a leftover block, but no room at the inn
	{
		CQERROR1("SendToSelf(): dropped packet %d", packet->sequenceID);
		goto Done;
	}

	memcpy(pHolding, packet, packet->dwSize);
	// count local packets in bandwidth calc?
//	throughputBin[currentBin] += 2 * (packet->dwSize-sizeof(DWORD)-sizeof(DPID));
	pHolding->fromID = PLAYERID;

Done:
	return;
}
//--------------------------------------------------------------------------//
// attempt to allocate memory large enough for the holding buffer
//
BOOL32 NetBuffer::useHolding (void)
{
	BOOL32 result = 0;
	SUPERBASE_PACKET * pNode = (SUPERBASE_PACKET *) heap->AllocateMemory(pHolding->dwSize + sizeof(SUPERBASE_PACKET *));

	if (pNode)
	{
		memcpy(&pNode->dwSize, pHolding, pHolding->dwSize);		// copy the block
		pNode->pNext = 0;
		if (pPacketListEnd)	// if list is not empty
		{
			pPacketListEnd->pNext = pNode;
			pPacketListEnd = pNode;
		}
		else  // list is empty
			pPacketList = pPacketListEnd = pNode;

		if (isSysMsg(pNode) == 0)		// if not a system message
		{
			PLAYER_NODE * tmp = findPlayer(pNode->fromID);

			if (tmp && tmp->samplesReceived > SYNC_SAMPLES)
			{
				pNode->timeStamp += (tmp->tickOffset >> PACKETTIMEBITSR);
				if (pNode->timeStamp > (currentTick>>PACKETTIMEBITSR))
					pNode->timeStamp = currentTick>>PACKETTIMEBITSR;
			}
			else
				pNode->timeStamp = currentTick>>PACKETTIMEBITSR;
		}

		pHolding->dwSize = 0;		// mark it unused
		result = 1;
	}

	if (result==0)
	{
		if (CQFLAGS.bTraceNetwork)
			NETPRINT0("useHolding(): out of memory");
	}

	return result;
}
//--------------------------------------------------------------------------//
//
PLAYER_NODE * NetBuffer::findPlayer (DPID dpid)
{
	PLAYER_NODE	* tmp = pPlayerList;

	while (tmp && tmp->playerID != dpid)
		tmp = tmp->pNext;

	if (tmp == 0 && dpid!=PLAYERID)
	{
		if (CQFLAGS.bTraceNetwork)
			CQTRACE11("findPlayer() failed on %08x", dpid);
	}

	return tmp;
}
//--------------------------------------------------------------------------//
// retrieve a matching packet from the list
// return TRUE if returned a packet
//
BOOL32 NetBuffer::receive (DPID idFrom, DWORD dwFlags, BASE_PACKET ** packet)
{
	BOOL32 result = 0;
	SUPERBASE_PACKET * pNode = pPacketList, *pPrev=0;

	while (pNode)
	{
		if ((dwFlags & DPRECEIVE_FROMPLAYER)==0 || idFrom == pNode->fromID)
		{
			//
			// check against minimum latency
			//
			if (minLatency==0 || isSysMsg(pNode) || (currentTick - (pNode->timeStamp>>PACKETTIMEBITSR) >= minLatency))
			{
				*packet = (BASE_PACKET *) (((C8 *)pNode) + sizeof(SUPERBASE_PACKET *));

				if ((dwFlags & DPRECEIVE_PEEK)==0)	// remove packet from the list
				{
					if (pPrev == 0)	// removed the first element
					{
						if ((pPacketList = pNode->pNext) == 0)
							pPacketListEnd = 0;			// list is now empty
					}
					else	// removed from the middle or end
					{
						if ((pPrev->pNext = pNode->pNext) == 0)
							pPacketListEnd = pPrev;		// removed from the end of list
					}
					pNode->pNext = 0;
				}

				result = 1;
				break;
			}
			else
			if (isSysMsg(pNode)==0 && pNode->sequenceID != 0)
			{
				if (CQFLAGS.bTraceNetwork)
					NETPRINT2("Holding packet %d from player %08x", pNode->sequenceID, pNode->fromID);
			}

		}

		pPrev = pNode;
		pNode = pNode->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------
//  called periodically, returns TRUE when it did some work
//
BOOL32 NetBuffer::syncPlayers (void)
{
	PLAYER_NODE *pNode;
	BOOL32 result=0;
	SYNC_PACKET packet;

	if ((pNode = pPlayerList) == 0 || PLAYERID == 0)
		goto Done;		// nothing to do

	packet.dwSize = sizeof(SYNC_PACKET);
	packet.fromID = PLAYERID;
	packet.type   = PT_PLAYER_SYNC;
	packet.timeStamp = currentTick>>PACKETTIMEBITSR;
	packet.localTick = currentTick;
	packet.remoteTick = 0;
	packet.extraLag = 0;
	packet.maxThroughput = maxBandwidth;
	// encode the checksum into the sync packet
	packet.sequenceID = rand();
	EVENTSYS->Send(CQE_GET_MULTIPLAYER_VER, &packet.checkSum);
	packet.checkSum ^= PLAYERID;
	packet.checkSum += packet.sequenceID;
	packet.ackID += ~(packet.checkSum + maxBandwidth);

	while (pNode)
	{
		if (pNode->samplesReceived < SYNC_SAMPLES)
		{
			Send(pNode->playerID, 0, &packet);
			result = 1;
		}
		
		pNode = pNode->pNext;
	}

Done:
	return result;
}
//--------------------------------------------------------------------------
//
void NetBuffer::handleSyncMsg (SYNC_PACKET * pPacket)
{
	if (PLAYERID == 0)
		goto Done;	// can't do anything yet, don't have a player

	if (pPacket->remoteTick==0)  // someone else is pinging us
	{
		pPacket->remoteTick = currentTick;
		pPacket->extraLag = (currentTick - updateTick) / 2;		// estimate time it sat in our queue
		Send(pPacket->fromID, 0, pPacket);		// echo the packet back to caller

		PLAYER_NODE * pNode = findPlayer(pPacket->fromID);

		if (pNode == 0 && (pNode = new PLAYER_NODE) != 0)
		{
			// add a new player to the list
			pNode->pNext = pPlayerList;
			pPlayerList = pNode;
			pNode->playerID = pPacket->fromID;
			pNode->lMinOffset = 0x7FFFFFFF;
			pNode->lMaxOffset = 0x80000000;

			if (CQFLAGS.bTraceNetwork)
				NETPRINT1("Adding player %08x", pPacket->fromID);
			EVENTSYS->Send(CQE_NETADDPLAYER, (void *)pPacket->fromID);
		}
		if (pNode)
		{
			pNode->maxThroughput = pPacket->maxThroughput;
			// attempt to decode the checksum info
			U32 ack = U16(~pPacket->ackID);
			if (ack == U16(pPacket->checkSum + pPacket->maxThroughput))
			{
				U32 checkSum = pPacket->checkSum - pPacket->sequenceID;
				checkSum ^= pNode->playerID;
				pNode->checkSum = checkSum;
			}
		}
	}
	else	// response to our message
	{
		PLAYER_NODE *pNode;

		// verify msg
		if (pPacket->dwSize != sizeof(SYNC_PACKET))
			goto Done;	// bogus!

		if (pPacket->extraLag > 500)		// unreliable
			goto Done;

		if ((pNode = findPlayer(pPacket->fromID)) == 0)
			goto Done;

		if (pNode->samplesReceived < SYNC_SAMPLES)
		{
			long lApprox;
			// approx our tick when at remote side
			lApprox = (pPacket->localTick + pPacket->extraLag + currentTick) / 2;

			lApprox -= pPacket->remoteTick;

			pNode->l64Accum += lApprox;
			pNode->samplesReceived++;
			if (lApprox < pNode->lMinOffset)
				pNode->lMinOffset = lApprox;
			if (lApprox > pNode->lMaxOffset)
				pNode->lMaxOffset = lApprox;

			if (pNode->samplesReceived == SYNC_SAMPLES)
			{
				// remove the lowest and highest

				__int64 avg = pNode->l64Accum;

				avg -= ((__int64)pNode->lMinOffset + (__int64)pNode->lMaxOffset);

				// take the average

				pNode->tickOffset = long(avg / (SYNC_SAMPLES-2));
				pNode->samplesReceived++;
				//
				// calculate the latency on this message
				//
				pPacket->timeStamp += pNode->tickOffset>>PACKETTIMEBITSR;
				pPacket->remoteTick += pNode->tickOffset;

				if (currentTick < pPacket->remoteTick)
					pNode->avgLatency = minLatency;
				else
					pNode->avgLatency = currentTick - pPacket->remoteTick + minLatency;
			}
		}
	}
Done:
	return;
}
//----------------------------------------------------------------------------//
//
void NetBuffer::handleSystemMessages (DPMSG_GENERIC * _pPacket)
{
	switch (_pPacket->dwType)
	{
	case DPSYS_CREATEPLAYERORGROUP:
		{
			DPMSG_CREATEPLAYERORGROUP * pPacket = (DPMSG_CREATEPLAYERORGROUP *) _pPacket;

			if (pPacket->dwPlayerType == DPPLAYERTYPE_PLAYER)
			{
				if (CQFLAGS.bTraceNetwork || CQFLAGS.bTraceMission)
					NETPRINT1("DPSYS_CREATEPLAYERORGROUP 0x%08X", pPacket->dpId);
				PLAYER_NODE * pNode = findPlayer(pPacket->dpId);

				if (pNode == 0 && (pNode = new PLAYER_NODE) != 0)
				{
					// add a new player to the list
					pNode->pNext = pPlayerList;
					pPlayerList = pNode;
					pNode->playerID = pPacket->dpId;
					pNode->lMinOffset = 0x7FFFFFFF;
					pNode->lMaxOffset = 0x80000000;

					EVENTSYS->Send(CQE_NETADDPLAYER, (void *)pPacket->dpId);
				}
			}
		}
		break;

	case DPSYS_DESTROYPLAYERORGROUP:
		{
			DPMSG_DESTROYPLAYERORGROUP * pPacket = (DPMSG_DESTROYPLAYERORGROUP *) _pPacket;

			if (pPacket->dwPlayerType == DPPLAYERTYPE_PLAYER)
			{
				if (CQFLAGS.bTraceNetwork || CQFLAGS.bTraceMission)
					NETPRINT1("DPSYS_DESTROYPLAYERORGROUP 0x%08X", pPacket->dpId);
				if (pPacket->dpId == PLAYERID)  // the host has killed us!
				{
					EVENTSYS->Send(CQE_NETDELETEPLAYER, (void *)PLAYERID);
				}
				else
				{
					PLAYER_NODE	* pNode = pPlayerList, *pPrev=0;
		
					while (pNode)
					{
						if (pNode->playerID == pPacket->dpId)
						{
							if (pPrev)
								pPrev->pNext = pNode->pNext;
							else
								pPlayerList = pNode->pNext;
							delete pNode;
							EVENTSYS->Send(CQE_NETDELETEPLAYER, (void *)pPacket->dpId);
							break;
						}
						pPrev = pNode;
						pNode = pNode->pNext;
					}
				}
			}
		}
		break;  // end case DPSYS_DESTROYPLAYERORGROUP

	case DPSYS_SESSIONLOST:
		{
			if (CQFLAGS.bTraceNetwork || CQFLAGS.bTraceMission)
				NETPRINT0("Network session was lost!");
			CQERROR0("DPSYS_SESSIONLOST received. DPLAY failure.");
			// delete all players
			PLAYER_NODE	* pNode = pPlayerList;

			while (pNode)
			{
				pPlayerList = pNode->pNext;
				EVENTSYS->Send(CQE_NETDELETEPLAYER, (void *)pNode->playerID);

				delete pNode;
				pNode = pPlayerList;
			}
		} // end case DPSYS_SESSIONLOST
		break;
	}
}
//--------------------------------------------------------------------------//
//
void NetBuffer::DestroyPlayer (DPID playerID)
{
	if (DPLAY)
	{
		HRESULT hresult = DPLAY->DestroyPlayer(playerID);
		if (hresult != DP_OK)
		{
			if (CQFLAGS.bTraceNetwork)
				NETPRINT1("DPLAY::DestroyPlayer() returned error code 0x%X", hresult);
		}
	}
	if (playerID == PLAYERID)  // the host has killed us!
	{
		if (DPLAY)
			DPLAY->Close();		// close the connection so that host migration will work

		// free the packets in the buffer
		SUPERBASE_PACKET * tmp, *saveList=0;
		pPacketListEnd = 0;
		
		while (pPacketList)
		{
			tmp = pPacketList->pNext;
			if (pPacketList->fromID == PLAYERID)	// add to the saved list
			{
				if (saveList==0)
				{
					saveList = pPacketListEnd = pPacketList;
				}
				else
				{
					pPacketListEnd->pNext = pPacketList;
					pPacketListEnd = pPacketList;
				}
				pPacketList->pNext = 0;
			}
			else
			{
				heap->FreeMemory(pPacketList);
			}
			pPacketList = tmp;
		}
		pPacketList = saveList;		// don't delete messages from PLAYERID
		if (pHolding && pHolding->dwSize && pHolding->fromID != PLAYERID)
			pHolding->dwSize = 0;
		
		EVENTSYS->Send(CQE_NETDELETEPLAYER, (void *)PLAYERID);
	}
	else
	{
		PLAYER_NODE	* pNode = pPlayerList, *pPrev=0;

		while (pNode)
		{
			if (pNode->playerID == playerID)
			{
				if (pPrev)
					pPrev->pNext = pNode->pNext;
				else
					pPlayerList = pNode->pNext;
				delete pNode;
				if (CQFLAGS.bTraceNetwork)
					NETPRINT1("Deleting player %08x", playerID);
				EVENTSYS->Send(CQE_NETDELETEPLAYER, (void *)playerID);
				break;
			}
			pPrev = pNode;
			pNode = pNode->pNext;
		}
	}
}
//--------------------------------------------------------------------------//
//
void NetBuffer::readProfile (void)
{
	COMPTR<IProfileParser> parser;
	HANDLE hSection;
	C8 buffer[MAX_PATH];

	if (DACOM->QueryInterface("IProfileParser", parser) != GR_OK)
		goto Done;

	if ((hSection = parser->CreateSection("Network")) == 0)
		goto Done;

/*	if (parser->ReadKeyValue(hSection, "HighSpeedInternet", buffer, sizeof(buffer)) != 0)
	{
		if (strncmp(buffer, "1", 1) == 0 || strnicmp(buffer, "on", 2)==0 || strnicmp(buffer, "true", 4)==0)
			TCPBandwidth = MAX_BANDWIDTH;
	}
*/	
	if (parser->ReadKeyValue(hSection, "LowSpeedLAN", buffer, sizeof(buffer)) != 0)
	{
		if (strncmp(buffer, "1", 1) == 0 || strnicmp(buffer, "on", 2)==0 || strnicmp(buffer, "true", 4)==0)
			LANBandwidth = MAX_BANDWIDTH/3;
	}

	CQFLAGS.bDPDelivery = 0;		// defaults to off
	if (parser->ReadKeyValue(hSection, "GuaranteedDelivery", buffer, sizeof(buffer)) != 0)
	{
		if (strncmp(buffer, "1", 1) == 0 || strnicmp(buffer, "on", 2)==0 || strnicmp(buffer, "true", 4)==0)
			CQFLAGS.bDPDelivery = 1;
	}

	parser->CloseSection(hSection);

Done:
	return;
}
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//
struct _netbuffer : GlobalComponent
{
	virtual void Startup (void)
	{
		CQASSERT(PT_LAST <= (1 << (PACKETTYPEBITS-1)));
		NETBUFFER = new DAComponent<NetBuffer>;
		AddToGlobalCleanupList((IDAComponent **) &NETBUFFER);
	}

	virtual void Initialize (void)
	{
		NETBUFFER->SetMinLatency(DEFAULTS->GetDefaults()->minLatency);
		NETBUFFER->SetPacketLoss(DEFAULTS->GetDefaults()->packetLossPercent);
		NETBUFFER->SetTCPNetworkPerformance(DEFAULTS->GetDefaults()->bNetworkBandwidth);
	}
};

static _netbuffer startup;

//----------------------------------------------------------------------------//
//-----------------------------End NetBuffer.cpp------------------------------//
//----------------------------------------------------------------------------//
