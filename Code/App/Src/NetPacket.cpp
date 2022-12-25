//--------------------------------------------------------------------------//
//                                                                          //
//                              NetPacket.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/NetPacket.cpp 113   8/23/01 1:53p Tmauer $

    Packet manager that handles packet-loss. Packets that come through here
	have guaranteed delivery.
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "NetPacket.h"
#include "Resource.h"
#include "NetBuffer.h"
#include "Startup.h"
#include "CQTrace.h"
#include "WindowManager.h"
#include "DrawAgent.h"
#include "GRPackets.h"

#include <FileSys.h>
#include <TComponent.h>
#include <TSmartPointer.h>
#include <Heapobj.h>
#include <EventSys.h>
#include <IConnection.h>

#if 0
#include "DBHotkeys.h"
#endif

#include <dplobby.h>
#include <dplay.h>

#include <stdio.h>

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

#endif

#define HEAP_SIZE			0x10000			// amount of memory for buffering
#define BLOCKED_TIMEOUT		5000
#define SEND_NACK_TIMEOUT	250				// max time to wait before sending an nack packet (in mseconds)
#define SEND_TIMEOUT		2000			// max time to wait before sending an ack packet (in mseconds)
#define RECEIVE_TIMEOUT		(1000*15)		// time to wait until assuming remote player has paused (in mseconds)
#define MAX_HOST_CLOGGED_PACKETS 40		// number of packets allowed to be unack'ed before waiting
#define MAX_CLIENT_CLOGGED_PACKETS 10	// number of packets allowed to be unack'ed before waiting

//
// pause/connection quality stuff
//
#define PAUSE_STARTUP_COST	(30*1000)	// time it costs to begin pause
#define PAUSE_TIME_ALLOWED  (4*60*1000)	// 4 minutes of pause per session
#define PAUSE_BOOT_TIME		(5*60*1000)	// after minutes of pause, boot the player

#define DLG_UPDATE		(WM_USER+1)

namespace __NETPACKET
{
static IHeap * netheap;
	
//--------------------------------------------------------------------------//
//
struct PAUSE_PACKET : BASE_PACKET
{
	bool bPause;
	U32  dpid;			// machine that is pausing/unpausing

	PAUSE_PACKET (bool _bPause, DPID _dpid)
	{
		dwSize = sizeof(*this);
		type = PT_PAUSE;
		bPause = _bPause;
		dpid = _dpid;
	}
};
//--------------------------------------------------------------------------//
//
struct TURTLE_PACKET : BASE_PACKET
{
	bool bTurtled;
	U32  dpid;			// machine that is turtled

	TURTLE_PACKET (bool _bTurtled, DPID _dpid)
	{
		dwSize = sizeof(*this);
		type = PT_TURTLE;
		bTurtled = _bTurtled;
		dpid = _dpid;
	}
};
//--------------------------------------------------------------------------//
//
struct PAUSEWARNING_PACKET : BASE_PACKET
{
	U32 dpid;

	PAUSEWARNING_PACKET (void)
	{
		dwSize = sizeof(*this);
		type = PT_PAUSEWARNING;
	}
};
//--------------------------------------------------------------------------//
//
struct NEWHOST_PACKET : BASE_PACKET
{
	U32  updateCount;

	NEWHOST_PACKET (U32 count)
	{
		updateCount = count;
		dwSize = sizeof(*this);
		type = PT_HOST;
	}
};
//--------------------------------------------------------------------------//
//
struct HOSTPEND_PACKET : BASE_PACKET
{
	U32  updateCount;
	U8   data[];

	HOSTPEND_PACKET (U32 count)
	{
		updateCount = count;
		type = PT_HOSTPEND;
	}
};
//--------------------------------------------------------------------------//
// assumed to be the same structure as HOSTPEND_PACKET
struct HOSTPENDACK_PACKET : BASE_PACKET
{
	U32  updateCount;
	U8   data[];
	
	HOSTPENDACK_PACKET (U32 count)
	{
		updateCount = count;
		type = PT_HOSTPENDACK;
	}
};
//--------------------------------------------------------------------------//
//
struct SUPERBASE_PACKET
{
	U32						  refCount;

	DWORD					  dwSize;
	DPID					  fromID;
	PACKET_TYPE type    : PACKETTYPEBITS;
	U32			userBits : PACKETUSERBITS;
	U32			ctrlBits : PACKETCTRLBITS;
	U32			timeStamp : PACKETTIMEBITS;
	U16						  sequenceID, ackID;

	operator const BASE_PACKET * (void)
	{
		return (const BASE_PACKET *) (&dwSize);
	}

	void operator delete (void * ptr)
	{
		netheap->FreeMemory(ptr);
	}

	void release (void)
	{
		CQASSERT(refCount);
		if (--refCount == 0)
			delete this;
	}
};
//--------------------------------------------------------------------------//
//
SUPERBASE_PACKET * convertToSuperPacket (const BASE_PACKET * packet)
{
	SUPERBASE_PACKET * result = (SUPERBASE_PACKET *) (((U8 *) packet) - sizeof(U32));
	return result;
}
//--------------------------------------------------------------------------//
//
struct PACKET_NODE
{
	struct PACKET_NODE * pNext;
	struct SUPERBASE_PACKET * pPacket;
	U16 sequenceID;

	void * operator new (size_t size)
	{
		return netheap->ClearAllocateMemory(size, "PACKET_NODE");
	}

	void operator delete (void * ptr)
	{
		netheap->FreeMemory(ptr);
	}

	~PACKET_NODE (void)
	{
		if (pPacket)
			pPacket->release();
	}
};
//--------------------------------------------------------------------------//
//
struct NETPLAYER
{
	struct NETPLAYER *	pNext;
	
	DPID	playerID;
	U32		receiveTimeout;			// time we received a ANY ack packet. 0 if never received a packet
	U32		sendTimeout;			// time we sent a packet. 0 if never sent a packet
	bool    bPaused:1;              // remote player has paused game
	bool	bTurtled:1;				// remote player has turtled (detected bad connection to a player)
	bool	bWaitingOnFlush:1;		// true if we are waiting on all ack's before continuing
	bool	bMarkedForDeath:1;		// player needs to be destroyed
	bool	bReceivedHostAck:1;		// player has acknowledged us becoming the host
	U16		waitingForAckID;		// packet we need to get and ack for before continuing

	//
	//		keep track of total pause time for session
	//
	U32		totalPauseTime;			// in mseconds
	bool	bPauseWarningIssued:1;	// true if we warned the player that he is out of pause time
	bool	bPauseWarningReceived:1;	// true if we have been warned about being out of pause time
	bool	bResentThisFrame:1;			// already resent a packet once this frame
	//
	// data items for send
	//
	PACKET_NODE * sentPackets;
	U16		lastAck;			// last packet that was acknowledged by remote side
	U16		lastSendID;			// last packet sequenceID used in a send
	//
	// data items for receive
	//
	PACKET_NODE * receivedPackets;
	U16		lastReceiveID;		// last (in order) packet sequenceID received, processed through to event system

	wchar_t   name[PLAYERNAMESIZE];
	//
	// node methods
	//

	void * operator new (size_t size)
	{
		return netheap->ClearAllocateMemory(size, "NetPlayerNode");
	}

	void operator delete (void * ptr)
	{
		netheap->FreeMemory(ptr);
	}

	~NETPLAYER (void)
	{
		reset();
	}

	void reset (void);

	int receiveAck (U16 ack);	// returns number of messages that were ack'ed

	int resend (int numMessages);

	int dispatchPackets (void);

	wchar_t * getPlayerName (void)
	{
		if (name[0] == 0)
		{
			if (playerID == 0)
				wcscpy(name, L"LOCAL");
			else
			{
				//
				// get player name from the id
				//
				DWORD size = 0;
				name[0] = 0;
				DPLAY->GetPlayerName(playerID, NULL, &size);
				if (size)
				{
					DPNAME * pName = (DPNAME *) malloc(size);
					if (DPLAY->GetPlayerName(playerID, pName, &size) == DP_OK)
						wcsncpy(name, pName->lpszShortName, PLAYERNAMESIZE-1);
					::free(pName);
				}
			}
		}

		return name;
	}

	static PACKET_NODE * addToList (PACKET_NODE * & list, const BASE_PACKET * pPacket);

	static PACKET_NODE * addToList (PACKET_NODE * & list, SUPERBASE_PACKET * pPacket);

	static PACKET_NODE * findPacket (PACKET_NODE * list, U16 sequence);

	static PACKET_NODE * findPacketPrev (PACKET_NODE * list, U16 sequence, PACKET_NODE * &pPrev);

	static int countPackets (PACKET_NODE * list);		// return number of elements in list

	static S32 compare (U32 id1, U32 id2)		// returns results of compare, checking for wrap-around
	{
		return S16(id1 - id2);
	}
};
//--------------------------------------------------------------------------//
//
inline static void guardedSend (DPID dpid, DWORD dwFlags, const BASE_PACKET * pPacket)
{
//	INetBuffer * const NETBUFFER = ::NETBUFFER;
//	if (NETBUFFER->TestPacketSend(pPacket->dwSize))
	NETBUFFER->Send(dpid, dwFlags, pPacket);
}
//--------------------------------------------------------------------------//
//
PACKET_NODE * NETPLAYER::addToList (PACKET_NODE * & list, const BASE_PACKET * pPacket)
{
	SUPERBASE_PACKET * pNode = (SUPERBASE_PACKET *) netheap->AllocateMemory(pPacket->dwSize + sizeof(U32));

	memcpy(&pNode->dwSize, &pPacket->dwSize, pPacket->dwSize);
	pNode->refCount = 0;

	return addToList(list, pNode);
}
//--------------------------------------------------------------------------//
//
PACKET_NODE * NETPLAYER::addToList (PACKET_NODE * & list, SUPERBASE_PACKET * pPacket)
{
	PACKET_NODE * node = new PACKET_NODE;

	pPacket->refCount++;
	node->pPacket = pPacket;
	node->pNext = list;
	list = node;

	return node;
}
//--------------------------------------------------------------------------//
//
PACKET_NODE * NETPLAYER::findPacket (PACKET_NODE * list, U16 sequence)
{
	while (list)
	{
		if (list->sequenceID == sequence)
			break;
		list = list->pNext;
	}
	return list;
}
//--------------------------------------------------------------------------//
//
PACKET_NODE * NETPLAYER::findPacketPrev (PACKET_NODE * list, U16 sequence, PACKET_NODE * &pPrev)
{
	pPrev = 0;
	while (list)
	{
		if (list->sequenceID == sequence)
			break;
		pPrev = list;
		list = list->pNext;
	}
	return list;
}
//--------------------------------------------------------------------------//
//
int NETPLAYER::countPackets (PACKET_NODE * list)		// return number of elements in list
{
	int result = 0;

	while (list)
	{
		result++;
		list = list->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
// flush both lists
//
void NETPLAYER::reset (void)
{
	PACKET_NODE * pNode;

	while ((pNode = sentPackets) != 0)
	{
		sentPackets = pNode->pNext;
		delete pNode;
	}

	while ((pNode = receivedPackets) != 0)
	{
		receivedPackets = pNode->pNext;
		delete pNode;
	}

	memset(this, 0, sizeof(*this));
}
//--------------------------------------------------------------------------//
//
int NETPLAYER::receiveAck (U16 ack)
{
	int result = 0;
	// if we haven't already got that acknowledgement
	if (compare(ack, lastAck) > 0)
	{
		PACKET_NODE * pNode=sentPackets, *pPrev = 0;
		
		while (pNode)
		{
			if (compare(ack, pNode->sequenceID) >= 0)
			{
				if (pPrev)	// removing from within list
				{
					pPrev->pNext = pNode->pNext;
					delete pNode;
					pNode = pPrev->pNext;
				}
				else	// removing first element
				{
					sentPackets = pNode->pNext;
					delete pNode;
					pNode = sentPackets;
				}
				result++;
			}
			else
			{
				pPrev = pNode;
				pNode = pNode->pNext;
			}
		}
		lastAck = ack;
	}

	return result;
}
//--------------------------------------------------------------------------//
// resend # messages to remote side
//
int NETPLAYER::resend (int numMessages)
{
	U16 i;
	int result=0;

	for (i = lastAck+1; result < numMessages && compare(lastSendID, i) >= 0; i++)
	{
		PACKET_NODE * node;
		
		if ((node = findPacket(sentPackets, i)) != 0)
		{
			const BASE_PACKET * pResend = *node->pPacket;

			node->pPacket->sequenceID = i;
			node->pPacket->ackID = lastReceiveID;
			if (CQFLAGS.bTraceNetwork)
				NETPRINT4("Resend packet %d to player %S.  type=%d, dwSize=%d", i, getPlayerName(), pResend->type, pResend->dwSize);
			guardedSend(playerID, 0, pResend);
			sendTimeout = 0;
			bResentThisFrame=true;
			result++;
		}
		else
		if (CQFLAGS.bTraceNetwork)
			NETPRINT2("findPacket failed on packet %d to player %S", i, getPlayerName());
	}

	return result;
}
//--------------------------------------------------------------------------//
// send received packets to the local event system
//
int NETPLAYER::dispatchPackets (void)
{
	PACKET_NODE * node, *pPrev;
	int result = 0;
		
	while ((node = findPacketPrev(receivedPackets, lastReceiveID+1, pPrev)) != 0)
	{
		lastReceiveID++;

		EVENTSYS->Send(CQE_NETPACKET, (void *)((const BASE_PACKET *)*(node->pPacket)));  // cast the super_packet into base_packet

		if (pPrev)	// removing from within list
		{
			pPrev->pNext = node->pNext;
			delete node;
			node = 0;
		}
		else	// removing first element
		{
			receivedPackets = node->pNext;
			delete node;
			node = 0;
		}
		result++;
	}

	return result;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE NetPacket : public INetPacket, IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(NetPacket)
	DACOM_INTERFACE_ENTRY(INetPacket)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	//--------------------
	// data members
	//--------------------

	COMPTR<IHeap> heap;
	U32 eventHandle;			    // handle to event callback
	bool bRemotePlayerTurtled;	// true if remote player has turtled
	bool bRemotePlayerBlocked;	// true if our connection to a player is blocked
	bool bRemotePlayerPaused;
	bool bDeadPlayers;				// true if there are dead players in the list
	U32	 currentTick;				// in milliseconds
	int  numPlayers;				// number of elements in the list (includes local player)
	int  iRecursion;
	bool bHasFocus;
	bool bBooted;					// we were booted from a game
	bool bLocalPaused;			// true if local player has paused the game
	bool bProposedHost;			// received a PT_HOST packet, haven't gotten acks yet
	bool bActiveApp;
#if 0
	bool DEBUG_bDoException;
#endif
	U32  dwSendFlags;
	U32  gameStatePacketsReceived;
	U32  dwHostLatency;				// age of packet, last time we received one from the host

	// thread data
	HANDLE hThread;
	volatile bool bThreadCancel;
	volatile bool bMessageReceived;

	COMPTR<IDrawAgent> turtleShape;

	NETPLAYER * pPlayerList;

	SUPERBASE_PACKET * pLastHostPacket;
	//--------------------

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	NetPacket (void);

	~NetPacket (void);
	
	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* INetPacket methods */

	virtual void __stdcall Send (DPID idTo, DWORD dwFlags, const BASE_PACKET * packet);

	virtual int __stdcall GetPacketsInTransit (DPID idTo, DWORD dwFlags) const;

	virtual int __stdcall TestLowPrioritySend (U32 packetSize);	

	virtual int GetNumHosts (void);	// returns number of computers attached to session (including local machine)

	virtual int __stdcall StallPacketDelivery (bool bStall);

	virtual U32 GetPauseTimeForPlayer (DPID dpID);		// return pause time used in msecs

	virtual bool HasPauseWarningBeenReceived (void);	

	virtual U32 GetTimeUntilBooting (DPID * dpid, bool * paused);

	virtual void DEBUG_print (struct IDebugFontDrawAgent * DEBUGFONT);

	virtual void OnGuaranteedDeliveryFailure (void);

	virtual bool EnumeratePlayers (IPlayerStateCallback * callback, void * context);

	/* NetPacket methods */

	void init (void);	// called once at startup

	void onNetStartup (void);

	void onNetShutdown (void);

	void receivePackets (void);	

	void receiveSystemPacket (DPMSG_GENERIC * packet);

	void receiveGamePacket (BASE_PACKET * packet);

	void onAddPlayer (DPID param);

	void deleteDeadPlayers (void);

	void createLocalPlayer (void);

	void reset (void);

	void onUpdate (U32 dt);	// in msecs

	void onLocalPause (bool bLocalPause);

	void send (DPID idTo, const BASE_PACKET * packet);

	void broadcast (DWORD dwFlags, const BASE_PACKET * packet);

	void receiveAck (NETPLAYER * player, const BASE_PACKET *packet);

	bool isSysMsg (BASE_PACKET * packet)
	{
		return (PLAYERID != DPID_SYSMSG && packet->fromID == DPID_SYSMSG);
	}

	NETPLAYER * findPlayer (DPID dpid) const;

	void on3DEnable (bool bEnable);

	void onGameActive (bool bActivate);

	void forceNetTurtle (void);

	void handleSessionLost (void);		// post messages to myself about host changing

	void handleBooting (void);

	void handleHostBooting (void);

	void handleHostPending (void);

	void handleKeepAlive (S32 dt);

	void receiveUpdateFromClient (const HOSTPENDACK_PACKET * packet);

	void sendHostPendAck (U32 dpid);

	bool checkReceiveTimeout (NETPLAYER * player)
	{
		if (HOSTID==PLAYERID || player->playerID==HOSTID)
		{
			if (player->receiveTimeout==0)
			{
				player->receiveTimeout = currentTick;
				return false;
			}
			else 
			if (player->playerID==HOSTID && player->countPackets(player->sentPackets) > MAX_CLIENT_CLOGGED_PACKETS)
				return true;
			else
			if (player->playerID!=HOSTID && player->countPackets(player->sentPackets) > MAX_HOST_CLOGGED_PACKETS)
				return true;
			else
				return (currentTick - player->receiveTimeout > RECEIVE_TIMEOUT);
		}
		else
			return false;		// peer-peer connection
	}

	bool checkSendNackTimeout (NETPLAYER * player)
	{
		if (HOSTID==PLAYERID || player->playerID==HOSTID)
		{
			if (player->sendTimeout==0)
			{
				player->sendTimeout = currentTick;
				return false;
			}
			else	// if we have detected missing packets, inform the remote side
				return (player->receivedPackets!=0 && currentTick - player->sendTimeout > SEND_NACK_TIMEOUT);
		}
		else
			return false;		// peer-peer connection
	}

	bool checkSendTimeout (NETPLAYER * player)
	{
		if (HOSTID==PLAYERID || player->playerID==HOSTID)
		{
			if (player->sendTimeout==0)
			{
				player->sendTimeout = currentTick;
				return false;
			}
			else
				return (currentTick - player->sendTimeout > SEND_TIMEOUT);
		}
		else
			return false;		// peer-peer connection
	}

	bool checkBlockedTimeout (NETPLAYER * player)
	{
		// don't check for blocked-to-myself case
		if (PLAYERID!=player->playerID && (HOSTID==PLAYERID || player->playerID==HOSTID))
		{
			if (player->receiveTimeout==0)
			{
				player->receiveTimeout = currentTick;
				return false;
			}
			else
				return (currentTick - player->receiveTimeout > BLOCKED_TIMEOUT);
		}
		else
			return false;		// peer-peer connection
	}

	// called when we are the proposed new host
	bool checkHostPendSendTimeout (NETPLAYER * player)
	{
		if (player->sendTimeout==0)
		{
			player->sendTimeout = currentTick;
			return false;
		}
		else
			return (currentTick - player->sendTimeout > SEND_TIMEOUT/2);
	}

	bool isPeerPacketType (const BASE_PACKET * packet)		// received without guaranteed delivery
	{
		switch (packet->type)
		{
		case PT_GAMEROOM:
			{
				GR_PACKET * grpacket = (GR_PACKET *) packet;
				if (grpacket->subtype == GRCHAT)
					return true;
			}
			break;
		case PT_FILE_TRANSFER:
		case PT_NETTEXT:
		case PT_RESIGN:
		case PT_RESIGNACK:
		case PT_HOSTPEND:
		case PT_HOSTPENDACK:
			return true;
		}

		return false;
	}

	bool isEveryonePaused (void) const;

	void clearPauseValues (void);

	IDAComponent * getBase (void)
	{
		return static_cast<INetPacket *>(this);
	}

	static BOOL __stdcall dlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
	static void __stdcall getPlayerName (DPID dpid, wchar_t * buffer, U32 bufferSize);

	// thread stuff
	void stopPollingThread (void);
	void startPollingThread (void);
	void pollingThreadMain (void);
	static DWORD WINAPI threadProc (LPVOID lpParameter)
	{
		struct NetPacket * _this = (NetPacket *) lpParameter;

		_this->pollingThreadMain();
		return 0;
	}
};
//--------------------------------------------------------------------------//
//
NetPacket::NetPacket (void)
{
	DAHEAPDESC hdesc;

	hdesc.heapSize = HEAP_SIZE;
	hdesc.growSize = HEAP_SIZE;
	hdesc.flags    = DAHEAPFLAG_PRIVATE | DAHEAPFLAG_DEBUGFILL | DAHEAPFLAG_GROWHEAP;
	DACOM->CreateInstance(&hdesc, heap);
	netheap = heap;
	netheap->SetErrorHandler(ICQImage::STANDARD_DUMP);
	
	createLocalPlayer();
	bHasFocus = 1;
}
//--------------------------------------------------------------------------//
//
NetPacket::~NetPacket (void)
{
	COMPTR<IDAConnectionPoint> connection;

	reset();

	if (GS && GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);
}
//--------------------------------------------------------------------------//
//
void NetPacket::stopPollingThread (void)
{
	if (hThread)
	{
		bThreadCancel = true;
		WaitForSingleObject(hThread, INFINITE);
		::CloseHandle(hThread);
		hThread=0;
	}
}
//--------------------------------------------------------------------------//
//
void NetPacket::startPollingThread (void)
{
	DWORD dwThreadID;

	CQASSERT(hThread==0);

	bThreadCancel = false;

	hThread = CreateThread(0,4096, threadProc, (LPVOID)this, 0, &dwThreadID);
	CQASSERT(hThread);
}
//--------------------------------------------------------------------------//
//
void NetPacket::pollingThreadMain (void)
{
	bMessageReceived = true;

	while (bThreadCancel==false)
	{
		if (bMessageReceived)	// only post one message at a time
		{
			bMessageReceived = false;
			PostMessage(hMainWindow, CQE_NETPACKET_KEEPALIVE, 100, 0);
		}
		Sleep(100);
	}
}
//-------------------------------------------------------------------
// receive notifications from event system
//
GENRESULT NetPacket::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;
	iRecursion++;

	switch (message)
	{
#if 0
	case CQE_DEBUG_HOTKEY:
		if (U32(param) == IDH_DEBUG_EXCEPTION)
		{
			if (HOSTID==PLAYERID)
				DEBUG_bDoException = true;
		}
		break;
#endif

	case WM_ACTIVATEAPP:
		if ((bActiveApp = (msg->wParam!=0)) == 0)
		{
			if (CQFLAGS.bGameActive && HOSTID && PLAYERID==HOSTID && hThread==0 && numPlayers>1)
				startPollingThread();
		}
		else
		if (hThread)
			stopPollingThread();
		break;

	case CQE_NETPACKET_KEEPALIVE:
		bMessageReceived = true;
		if (CQFLAGS.bGameActive && HOSTID && PLAYERID==HOSTID && hThread!=0 && numPlayers>1)
			handleKeepAlive(msg->wParam);
		break;

	case CQE_MISSION_CLOSE:
		if (param!=0)
			heap->HeapMinimize();
		break;

	case CQE_MISSION_ENDING:
		bLocalPaused = false;
		break;

	case CQE_NETPACKET:
		{
			const BASE_PACKET * packet = (const BASE_PACKET *) param;
			switch (packet->type)
			{
			case PT_HOST:
				{
					const NEWHOST_PACKET * pNewhost = (const NEWHOST_PACKET *) packet;
					DWORD dwOldHost = HOSTID;
					HOSTID = packet->fromID;
					if (dwOldHost != HOSTID)
						EVENTSYS->Send(CQE_NEWHOST, (void *)dwOldHost);
					if (CQFLAGS.bTraceNetwork || CQFLAGS.bTraceMission)
					{
						NETPRINT1("-----------------------NEWHOST NEWHOST NEWHOST [0x%X]--------------------------------", HOSTID);
						NETPRINT2("NEWHOST: Rcv #%d, My #%d", pNewhost->updateCount, gameStatePacketsReceived);
					}

					if (pNewhost->updateCount != gameStatePacketsReceived)
					{
						bBooted = true;
						NETBUFFER->DestroyPlayer(PLAYERID);		// drop ourselves from the session

						if (CQFLAGS.bTraceNetwork || CQFLAGS.bTraceMission)
							NETPRINT0("BOOTING myself, received different amount of packets!");
						CQERROR0("Host migration failed at the application level.");
					}
				}
				break;
			case PT_PAUSEWARNING:
				{
					const PAUSEWARNING_PACKET * pWarning = (const PAUSEWARNING_PACKET *) packet;
					if (packet->fromID == HOSTID)
					{
						NETPLAYER * player = findPlayer(pWarning->dpid);
						if (player)
						{
							player->bPauseWarningReceived = true;
							if (player->totalPauseTime < PAUSE_TIME_ALLOWED)
								player->totalPauseTime = PAUSE_TIME_ALLOWED;
							if (pWarning->dpid == PLAYERID)
								EVENTSYS->Send(CQE_PAUSE_WARNING, 0);
						}
					}
				}
				break;

			case PT_PAUSE:
				{
					const PAUSE_PACKET * pPause = (const PAUSE_PACKET *) (const PAUSE_PACKET *) packet;
					if (pPause->dpid == 0 && pPause->fromID != 0)	// pause sent from client
					{
						if (HOSTID==PLAYERID)		// make sure we are the host
						{
							PAUSE_PACKET newPause(pPause->bPause, pPause->fromID);

							Send(0, NETF_ECHO | NETF_ALLREMOTE, &newPause);
						}
					}
					else
					if (pPause->fromID == HOSTID)
					{
						NETPLAYER * player = findPlayer(pPause->dpid);

						if (player)
						{
							bool bOldPause = player->bPaused;
							if ((player->bPaused = pPause->bPause) != 0 && bOldPause==0)
							{
								if (player->totalPauseTime < PAUSE_TIME_ALLOWED)
									player->totalPauseTime += PAUSE_STARTUP_COST;
							}

							if (isEveryonePaused())
							{
								clearPauseValues();
							}
						}
					}
				}
				break;

			case PT_TURTLE:
				{
					const TURTLE_PACKET * pTurtle = (const TURTLE_PACKET *) packet;
					if (packet->fromID == HOSTID)
					{
						NETPLAYER * player = findPlayer(pTurtle->dpid);
						if (player)
						{
							player->bTurtled = pTurtle->bTurtled;
						}
					}
				}
				break;

			case PT_RESIGN:
				{
					// send an ack
					BASE_PACKET response;
					response.dwSize = sizeof(response);
					response.type = PT_RESIGNACK;
					Send(packet->fromID, 0, &response);
				}
				break;

			case PT_HOSTUPDATE:
				// if this is a game state packet from the host, inc counter
				if (packet->fromID == HOSTID)
				{
					gameStatePacketsReceived++;
					if (pLastHostPacket)
						pLastHostPacket->release();
					pLastHostPacket = convertToSuperPacket(packet);		// save a copy of this packet
					pLastHostPacket->refCount++;
				}
				break;
			
			case PT_HOSTPENDACK:
				if (bProposedHost)
				{
					const HOSTPENDACK_PACKET * pAck = (const HOSTPENDACK_PACKET *) packet;
					NETPLAYER * player = findPlayer(pAck->fromID);
					if (player)
					{
						if (CQFLAGS.bTraceNetwork || CQFLAGS.bTraceMission)
							NETPRINT3("HOSTPENDACK rcv from %S, Rcv #%d, My #%d", player->getPlayerName(), pAck->updateCount, gameStatePacketsReceived);
						player->bReceivedHostAck = true;
						// need to reset when host or client is off by 1						
						bool bReset = ( (gameStatePacketsReceived+1 == pAck->updateCount) || (gameStatePacketsReceived == pAck->updateCount+1) );
						if (gameStatePacketsReceived+1 == pAck->updateCount)	// client is one ahead of us
						{
							receiveUpdateFromClient(pAck);
						}

						if (bReset)
						{
							//  clear ack bits and start over
							NETPLAYER * player = pPlayerList;
							while (player)
							{
								player->bReceivedHostAck = false;
								player = player->pNext;
							}

						}
					}
				}
				break;

			case PT_HOSTPEND:
				if (findPlayer(HOSTID) == 0)		// only acknowledge it if host has died
				{
					const HOSTPENDACK_PACKET * pAck = (const HOSTPENDACK_PACKET *) packet;
					if (CQFLAGS.bTraceNetwork || CQFLAGS.bTraceMission)
					{
						NETPLAYER * player = findPlayer(packet->fromID);
						if (player)
							NETPRINT3("HOSTPEND rcv from %S, Rcv #%d, My #%d", player->getPlayerName(), pAck->updateCount, gameStatePacketsReceived);
					}
					if (gameStatePacketsReceived+1 == pAck->updateCount)	// client is one ahead of us
						receiveUpdateFromClient(pAck);
					sendHostPendAck(packet->fromID);
				}
				else
				{
					if (CQFLAGS.bTraceNetwork || CQFLAGS.bTraceMission)
					{
						NETPLAYER * player = findPlayer(packet->fromID);
						if (player)
							NETPRINT1("HOSTPEND rcv from %S, IGNORED.", player->getPlayerName());
					}
				}
				break;
			}  // end switch(packet->type)
		}
		break;

	case CQE_KILL_FOCUS:
		bHasFocus = 0;
		break;
	case CQE_SET_FOCUS:
		bHasFocus = 1;
		break;
	case CQE_GAME_ACTIVE:
		CQASSERT(iRecursion==1);
		onGameActive(param!=0);
		break;

	case CQE_NETSTARTUP:
		CQASSERT(iRecursion==1);
		onNetStartup();
		break;
	
	case CQE_NETSHUTDOWN:
		CQASSERT(iRecursion==1);
		onNetShutdown();
		break;

	case CQE_NETADDPLAYER:
		onAddPlayer((DPID)param);
		break;

	case CQE_NETDELETEPLAYER:
		{
			NETPLAYER * player = findPlayer(DPID(param));
			if (player)
			{
				if (HOSTID==PLAYERID && player->playerID == PLAYERID)
					handleHostBooting();
				else
				{
					if (player->playerID == PLAYERID && CQFLAGS.bGameActive)
						bBooted = true;
					player->bMarkedForDeath = bDeadPlayers = true;
				}
			}
		}
		break;

	case CQE_UPDATE:
		if (iRecursion==1)
			onUpdate(U32(param) >> 10);
		break;

	case CQE_LOCALPAUSED:
		onLocalPause(param!=0);
		break;

	case CQE_DPLAY_MSGWAITING:
		if (iRecursion==1)
			onUpdate(msg->wParam);	// make sure packets are processed
		break;

	case CQE_START3DMODE:
		on3DEnable(true);
		break;

	case CQE_ENDFRAME:
		if (bRemotePlayerBlocked)
		{
			U16 width, height;
			S32 x;
			
			turtleShape->GetDimensions(width, height);
			x = SCREENRESX - width;
			turtleShape->Draw(0, x, 0);
		}
		break;
	}

	iRecursion--;
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void NetPacket::handleKeepAlive (S32 dt)
{
	NETPLAYER * player = findPlayer(PLAYERID);
	if (player)
	{
		if (dt > 0)
			player->totalPauseTime += dt;

		if (player->bMarkedForDeath==false)
		{
			if (player->totalPauseTime > PAUSE_BOOT_TIME)
			{
				// boot the player!
				stopPollingThread();
				if (CQFLAGS.bTraceNetwork || CQFLAGS.bTraceMission)
					NETPRINT1("Pause time exceeded, booting player: %S", player->getPlayerName());
				NETBUFFER->DestroyPlayer(player->playerID);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
bool NetPacket::isEveryonePaused (void) const
{
	NETPLAYER * player = pPlayerList;
	bool result = true;

	while (player)
	{
		if (player->bPaused==0 && player->bPauseWarningIssued==0 && player->bPauseWarningReceived==0)
		{
			result = false;
			break;
		}
		player = player->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void NetPacket::clearPauseValues (void)
{
	NETPLAYER * player = pPlayerList;
	while (player)
	{
		player->bPauseWarningIssued = player->bPauseWarningReceived = 0;
		player->totalPauseTime = 0;
		player = player->pNext;
	}
}
//--------------------------------------------------------------------------//
//
void NetPacket::receivePackets (void)
{
	BASE_PACKET * pBasePacket;

	while (NETBUFFER->Receive(0, 0,	&pBasePacket))
	{
		if (isSysMsg(pBasePacket))
			receiveSystemPacket((DPMSG_GENERIC *) ((C8 *)pBasePacket + (sizeof(DWORD)*2)));
		else
		if (isPeerPacketType(pBasePacket))		// no guaranteed delivery
		{
			if (CQFLAGS.bTraceNetwork)
				NETPRINT1("RCV PEER Packet type=%#X", pBasePacket->type);
			EVENTSYS->Send(CQE_NETPACKET, pBasePacket); 
		}
		else
			receiveGamePacket(pBasePacket);

		NETBUFFER->FreePacket(pBasePacket);
	}
}
//--------------------------------------------------------------------------//
//
void NetPacket::Send (DPID idTo, DWORD dwFlags, const BASE_PACKET * packet)
{
	CQASSERT((dwFlags & ~(NETF_ALLREMOTE|NETF_ECHO)) == 0 && "Invalid flags passed to Send()");
	CQASSERT(packet->timeStamp==0);
	CQASSERT(packet->sequenceID==0);
	CQASSERT(packet->ackID==0);
	CQASSERT(packet->dwSize >= sizeof(BASE_PACKET));
	
	if ((dwFlags & (NETF_ALLREMOTE|NETF_ECHO)) == 0)
	{
		CQASSERT(idTo || PLAYERID==0);
		CQASSERT1(idTo==HOSTID || HOSTID==PLAYERID || isPeerPacketType(packet), "Attempt to send packet type %d to peer.", packet->type);
		send(idTo, packet);
	}
	else
	{
		if (PLAYERID != 0)	// active session
		{
			CQASSERT1(HOSTID==PLAYERID || packet->type==PT_HOST || isPeerPacketType(packet), "Attempt to send packet type %d to peer.", packet->type);
			broadcast(dwFlags, packet);
		}
		else // not networked
		{
			if (dwFlags & NETF_ECHO)
				send(PLAYERID, packet);
		}
	}
}
//--------------------------------------------------------------------------//
//
int NetPacket::GetPacketsInTransit (DPID idTo, DWORD dwFlags) const
{
	int result = 0;

	CQASSERT((dwFlags & ~(NETF_ALLREMOTE|NETF_ECHO)) == 0 && "Invalid flags passed to GetPacketsInTransit()");

	if ((dwFlags & (NETF_ALLREMOTE|NETF_ECHO)) == 0)
	{
		CQASSERT(idTo || PLAYERID==0);
		NETPLAYER * pNode = findPlayer(idTo);

		if (pNode)
			result = pNode->compare(pNode->lastSendID, pNode->lastAck);
	}
	else
	{
		if (PLAYERID != 0)	// active session
		{
			NETPLAYER * player = pPlayerList;

			while (player)
			{
				if (player->playerID != PLAYERID || (dwFlags & NETF_ECHO))
					result += player->compare(player->lastSendID, player->lastAck);
				player = player->pNext;
			}
		}
		else // not networked
		{
			if (dwFlags & NETF_ECHO)
			{
				NETPLAYER * pNode = findPlayer(PLAYERID);
		
				if (pNode)
					result = pNode->compare(pNode->lastSendID, pNode->lastAck);
			}
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
int NetPacket::TestLowPrioritySend (U32 packetSize)
{
	int result = 0;

	if (numPlayers < 2 || NETBUFFER->TestPacketSend(packetSize*(numPlayers-1)))
	{
		// allow no more than 4 seconds of back-logged packets
		const S32 maxSavedData = NETBUFFER->GetSessionThroughput()*4;
		CQASSERT(maxSavedData < HEAP_SIZE);
		result = heap->GetAvailableMemory() - (HEAP_SIZE - maxSavedData);
		if (result < 0)
			result = 0;
	}

	return result;
}
//--------------------------------------------------------------------------//
int NetPacket::GetNumHosts (void)
{
	return numPlayers;
}
//--------------------------------------------------------------------------//
//
int NetPacket::StallPacketDelivery (bool bStall)
{
	if (bStall)
		iRecursion++;
	else
		iRecursion--;

	CQASSERT(iRecursion>=0);

	return iRecursion;
}
//--------------------------------------------------------------------------//
//
U32 NetPacket::GetPauseTimeForPlayer (DPID dpID)
{
	NETPLAYER * player = findPlayer(dpID);
	S32 result;

	result = (player) ? (PAUSE_BOOT_TIME - player->totalPauseTime) : 0;
	if (result < 0)
		result = 0;

	return result;
}
//--------------------------------------------------------------------------//
//
U32 NetPacket::GetTimeUntilBooting (DPID * dpid, bool * paused)
{
	NETPLAYER * player = pPlayerList;
	U32 maxUserID=0;
	U32 maxPauseTime=0;
	bool maxUserPaused = 0;
	U32 result = 0;

	if (dpid)
		*dpid = 0;
	if (paused)
		*paused = 0;

	while (player)
	{
		if (player->bPaused || player->bTurtled)
		{
			if (player->totalPauseTime > maxPauseTime)
			{
				maxUserID = player->playerID;
				maxPauseTime = player->totalPauseTime;
				maxUserPaused = player->bPaused;
			}
		}

		player = player->pNext;
	}
	
	if (maxUserID)
	{
		S32 time = PAUSE_BOOT_TIME - maxPauseTime;
		if (time <= 0)
			time = 1;
		result = time;
		if (dpid)
			*dpid = maxUserID;
		if (paused)
			*paused = maxUserPaused;
	}
	
	return result;
}
//--------------------------------------------------------------------------//
//
bool NetPacket::EnumeratePlayers (IPlayerStateCallback * callback, void * context)
{
	NP_PLAYERINFO info;
	bool result = true;
	NETPLAYER * player = pPlayerList;

	while (player)
	{
		info.dpid = player->playerID;
		info.bootTime = PAUSE_BOOT_TIME - player->totalPauseTime;
		if (info.bootTime <= 0)
			info.bootTime = 1;
		info.szPlayerName = player->getPlayerName();
		info.dwFlags = (numPlayers < 2) ? NPPI_HOST : 0;
		info.dwFlags |= (player->bPaused) ? NPPI_PAUSED : 0;
		info.dwFlags |= (player->bTurtled) ? NPPI_TURTLED : 0;

		if ((result = callback->EnumeratePlayer(info, context)) == 0)
			break;

		player = player->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void NetPacket::getPlayerName (DPID dpid, wchar_t * buffer, U32 bufferSize)
{
	U32 size = 0;
	DPLAY->GetPlayerName(dpid, NULL, &size);

	buffer[0] = 0;
	if (size)
	{
		DPNAME * pName = (DPNAME *) malloc(size);
		if (DPLAY->GetPlayerName(dpid, pName, &size) == DP_OK)
			wcsncpy(buffer, pName->lpszShortName, bufferSize / sizeof(wchar_t));
		::free(pName);
	}
}
//--------------------------------------------------------------------------//
//
void NetPacket::DEBUG_print (struct IDebugFontDrawAgent * DEBUGFONT)
{
	if (PLAYERID != 0)
	{
		char buffer[256];
		wchar_t wbuffer[64];
		int y = 30;
		const int x = SCREENRESX-170;
		int count = 0;
		const int inc = DEBUGFONT->GetFontHeight();
		const U32 DCOLOR = RGB(200,200,200);
		const U32 DCOLOR2 = RGB(200,100,100);

		sprintf(buffer, "NETWORKING      %d bps", NETBUFFER->GetThroughput());
		DEBUGFONT->StringDraw(NULL, x, y, buffer, DCOLOR2);
		y += inc;
		sprintf(buffer, "DPID: 0x%08X HOSTID: 0x%08X", PLAYERID, HOSTID);
		DEBUGFONT->StringDraw(NULL, x, y, buffer, DCOLOR2);
		y += inc;
		sprintf(buffer, "HEAP: %d of %d", heap->GetAvailableMemory(), heap->GetHeapSize());
		DEBUGFONT->StringDraw(NULL, x, y, buffer, DCOLOR2);
		y += inc;
		if (PLAYERID!=HOSTID)
		{
			sprintf(buffer, "LATENCY TO HOST: %d msec", dwHostLatency);
			DEBUGFONT->StringDraw(NULL, x, y, buffer, DCOLOR2);
			y += inc;
		}

		NETPLAYER * player = pPlayerList;
		while (player)
		{
			getPlayerName(player->playerID, wbuffer, sizeof(wbuffer));
			if (wbuffer[0])
			{
				sprintf(buffer, "%S [%X]%s", wbuffer, player->playerID, (player->playerID==HOSTID)?" HOST":"");
				DEBUGFONT->StringDraw(NULL, x, y, buffer, (count&1)?DCOLOR2:DCOLOR);
				y += inc;
			}

			sprintf(buffer, "   FLAGS: %s%s%s",
				(player->bPaused)?"[PAUSED]":"",
				(player->bTurtled)?"[TURTLED]":"",
				(player->bWaitingOnFlush)?"[FLUSH]":"");
			DEBUGFONT->StringDraw(NULL, x, y, buffer, (count&1)?DCOLOR2:DCOLOR);
			y += inc;
			
			sprintf(buffer, "   SEND: %d, ACK: %d, #pcks %d", player->lastSendID, player->lastAck, player->countPackets(player->sentPackets));
			DEBUGFONT->StringDraw(NULL, x, y, buffer, (count&1)?DCOLOR2:DCOLOR);
			y += inc;

			if (PLAYERID==HOSTID || player->playerID==HOSTID)
			{
				sprintf(buffer, "   RCV: %d, #pcks %d", player->lastReceiveID, player->countPackets(player->receivedPackets));
				DEBUGFONT->StringDraw(NULL, x, y, buffer, (count&1)?DCOLOR2:DCOLOR);
				y += inc;
			}

			player = player->pNext;
			count++;
		}
	}
}
//--------------------------------------------------------------------------//
//
void NetPacket::OnGuaranteedDeliveryFailure (void)
{
	if (CQFLAGS.bTraceNetwork)
		NETPRINT0("Guaranteed Delivery not supported. Switching to internal method.");
	CQFLAGS.bDPDelivery = 0;
	dwSendFlags = 0;
}
//--------------------------------------------------------------------------//
//
bool NetPacket::HasPauseWarningBeenReceived (void)
{
	NETPLAYER * player = findPlayer(PLAYERID);
	bool result;

	result = (player) ? player->bPauseWarningReceived : 0;

	return result;
}
//--------------------------------------------------------------------------//
//
void NetPacket::onNetStartup (void)
{
	if (bDeadPlayers)
	{
		deleteDeadPlayers();
		bDeadPlayers = false;
	}

	createLocalPlayer();

	pPlayerList->playerID = PLAYERID;
	gameStatePacketsReceived = 0;
	dwSendFlags = (CQFLAGS.bDPDelivery) ? (DPSEND_GUARANTEED | DPSEND_ASYNC | DPSEND_NOSENDCOMPLETEMSG) : 0;
}
//--------------------------------------------------------------------------//
//
void NetPacket::reset (void)
{
	stopPollingThread();

	NETPLAYER * pNode = pPlayerList, *pNext;

	while (pNode)
	{
		pNext = pNode->pNext;
		delete pNode;
		pNode = pNext;
		numPlayers--;
	}
	pPlayerList = 0;
	CQASSERT(numPlayers==0);

	if (pLastHostPacket)
	{
		pLastHostPacket->release();
		pLastHostPacket = 0;
	}

	if (heap->GetLargestBlock() != heap->GetHeapSize())
		_localprintf("%s(%d) : WARNING!  Internal memory leak detected.\n", __FILE__,__LINE__);
}
//--------------------------------------------------------------------------//
//
void NetPacket::onNetShutdown (void)
{
	stopPollingThread();

	//
	// delete remote players
	//
	NETPLAYER * player = findPlayer(PLAYERID);

	if (player)
		player->playerID = 0;

	NETPLAYER * pNode = pPlayerList, *pNext;

	while (pNode)
	{
		pNext = pNode->pNext;
		if (pNode != player)
		{
			pNode->bMarkedForDeath = bDeadPlayers = true;
		}
		pNode = pNext;
	}

	if (pLastHostPacket)
	{
		pLastHostPacket->release();
		pLastHostPacket = 0;
	}
}
//--------------------------------------------------------------------------//
//
void NetPacket::receiveSystemPacket (DPMSG_GENERIC * packet)
{
	if (CQFLAGS.bTraceNetwork)
		NETPRINT1("RCV System Packet type=%#X", packet->dwType);

	switch (packet->dwType)
	{
	case DPSYS_SESSIONLOST:
		if (HOSTID != PLAYERID)
			handleSessionLost();		// post messages to myself about host changing
		break;

		/*
	case DPSYS_HOST:		// tell everyone else that I'm in charge
		{
			NEWHOST_PACKET packet(gameStatePacketsReceived);

			if (CQFLAGS.bTraceNetwork)
				NETPRINT1("Sending PT_HOST packet. Packets=%d", gameStatePacketsReceived);
			Send(0, NETF_ALLREMOTE|NETF_ECHO, &packet);
			SendZoneHostChange();
		}
		break;
		*/

	case DPSYS_HOST:
		bProposedHost = true;		// begin migration
		break;
	}
}
//--------------------------------------------------------------------------//
//
void NetPacket::receiveGamePacket (BASE_PACKET * packet)
{
	NETPLAYER * player = findPlayer(packet->fromID);

	if (player)
	{
		receiveAck (player, packet);

		if (packet->type == PT_ACK || packet->type == PT_NACK)
		{
			// nothing more to do
		}
		else
		if (player->compare(packet->sequenceID, player->lastReceiveID) > 0)		// throw away repeats
		{
			PACKET_NODE * node;
		
			// make sure we haven't already received this one
			if ((node = player->findPacket(player->receivedPackets, packet->sequenceID)) == 0)
			{
				node = player->addToList(player->receivedPackets, packet);
				node->sequenceID = packet->sequenceID;
				//
				// check for out-of-order delivery
				//
				bool bInOrderPacket = (player->compare(packet->sequenceID, player->lastReceiveID) == 1);

				if (CQFLAGS.bTraceNetwork)
					NETPRINT3("RCV packet %d %s from player %S", packet->sequenceID, (bInOrderPacket)?"":"(OutOfOrder)", player->getPlayerName());
				if (bInOrderPacket)
					player->dispatchPackets();
				int count = player->countPackets(player->receivedPackets);
				// if we received the packet we were waiting on, but still have other blocking packets,
				// or we received an out-of-order packet with no other packets in the queue
				if ((bInOrderPacket && count > 0) || count == 1)
				{
					// we lost a packet!
					BASE_PACKET packet;

					packet.type = PT_NACK;
					packet.dwSize = sizeof(packet);
					packet.ackID = player->lastReceiveID;
					if (CQFLAGS.bTraceNetwork)
						NETPRINT2("SND NACK of packet %d to player %S", player->lastReceiveID, player->getPlayerName());
					guardedSend(player->playerID, 0, &packet);
					player->sendTimeout = 0;
				}
			}
			else
			{
				if (CQFLAGS.bTraceNetwork)
					NETPRINT2("RCV packet %d (duplicate) from player %S", packet->sequenceID, player->getPlayerName());
			}
		}
		else
		{
			if (CQFLAGS.bTraceNetwork)
				NETPRINT2("RCV packet %d (duplicate - very old) from player %S", packet->sequenceID, player->getPlayerName());
		}
	}
	else
	{
		if (CQFLAGS.bTraceNetwork)
			NETPRINT1("RCV packet %d from UNKNOWN player!", packet->sequenceID);
	}
}
//----------------------------------------------------------------------------------
// WARNING: this can be called from within an update()
//
void NetPacket::onAddPlayer (DPID param)
{
	if (pPlayerList && pPlayerList->playerID==0)
		pPlayerList->playerID = PLAYERID;

	if (param != PLAYERID)
	{
		CQASSERT (findPlayer(param) == 0);
		NETPLAYER * player;

		player = new NETPLAYER;
		player->playerID = param;
		player->pNext = pPlayerList;
		pPlayerList = player;
		numPlayers++;

		if (CQFLAGS.bTraceNetwork)
			NETPRINT2("Adding player 0x%08X %s", param, player->getPlayerName());
		EVENTSYS->Send(CQE_ADDPLAYER, (void *)param);
	}

	if (HOSTID == PLAYERID)		// are we the host
	{
		NEWHOST_PACKET packet(gameStatePacketsReceived);
		Send(param, 0, &packet);		// tell the new player about us
	}
}
//----------------------------------------------------------------------------------
//
void NetPacket::deleteDeadPlayers (void)
{
	NETPLAYER * player = pPlayerList, *pPrev = 0;

	while (player)
	{
		if (player->bMarkedForDeath)
		{
			numPlayers--;
			U32 id = player->playerID;
			if (pPrev)	// deleting from inside list
			{
				pPrev->pNext = player->pNext;
				delete player;
				player = pPrev->pNext;
			}
			else // deleting first node
			{
				pPlayerList = player->pNext;
				delete player;
				player = pPlayerList;
			}
			if (CQFLAGS.bTraceNetwork)
				NETPRINT1("Deleting player 0x%08X", id);
			EVENTSYS->Send(CQE_DELETEPLAYER, (void *)id);
		}
		else
		{
			pPrev = player;
			player = player->pNext;
		}
	}
}
//----------------------------------------------------------------------------------
//
void NetPacket::createLocalPlayer (void)
{
	CQASSERT(pPlayerList==0 || pPlayerList->pNext==0);		// only local player exists
	CQASSERT(pPlayerList==0 || pPlayerList->playerID==0 || pPlayerList->playerID==PLAYERID);	// only local player exists

	if (pPlayerList)
		pPlayerList->reset();
	else
	{
		pPlayerList = new NETPLAYER;
		numPlayers++;
	}
}
//----------------------------------------------------------------------------------
//
void NetPacket::forceNetTurtle (void)
{
	if (CQFLAGS.bGameActive)
	{
		NETPLAYER * player = pPlayerList;

		while (player)
		{
			// don't mark host's local player for flush
			if (player->playerID!=HOSTID || PLAYERID!=HOSTID)
			{
				// not already waiting and we have unack'ed packets
				if (player->bWaitingOnFlush==false && player->lastAck != player->lastSendID)
				{
					player->waitingForAckID = player->lastSendID;
					player->bWaitingOnFlush=true;
				}
			}

			player = player->pNext;
		}
	}
}
//----------------------------------------------------------------------------------
// called when HOSTID!=PLAYERID,
// forge some messages from old host
//
void NetPacket::handleSessionLost (void)
{
	CQASSERT(HOSTID!=PLAYERID);

	onNetStartup();		// ensure that only one player remains
	CQASSERT(pPlayerList && pPlayerList->pNext==0);		// only one player left

	{
		DWORD dwOldHost = HOSTID;
		HOSTID = PLAYERID;
		if (dwOldHost != HOSTID)
			EVENTSYS->Send(CQE_NEWHOST, (void *)dwOldHost);
	}
}
//----------------------------------------------------------------------------------
// we've been booted!
//
void NetPacket::handleBooting (void)
{
	//
	// delete remote players
	//
	NETPLAYER * player = findPlayer(PLAYERID);
	NETPLAYER * pNode = pPlayerList;

	while (pNode)
	{
		if (pNode != player)
		{
			pNode->bMarkedForDeath = bDeadPlayers = true;
		}
		pNode = pNode->pNext;
	}

	handleSessionLost();
}
//----------------------------------------------------------------------------------
// called when the host has been booted from his own session
void NetPacket::handleHostBooting (void)
{
	CQASSERT(HOSTID==PLAYERID);
	NETPLAYER * player = findPlayer(PLAYERID);
	NETPLAYER * pNode = pPlayerList;

	while (pNode)
	{
		if (pNode != player)
		{
			pNode->bMarkedForDeath = bDeadPlayers = true;
		}
		pNode = pNode->pNext;
	}
}
//----------------------------------------------------------------------------------
// dt is in milliseconds
//
void NetPacket::onUpdate (U32 dt)
{
	currentTick += dt;	
	bool bExtBlockedPlayer = false;
	NETPLAYER * player;
	const bool bOldNetPaused = (bRemotePlayerTurtled||bRemotePlayerPaused);		// save old state
	
	bRemotePlayerTurtled = false;
	bRemotePlayerBlocked = false;
	bRemotePlayerPaused = false;

	// clear some flags
	player = pPlayerList;
	while (player)
	{
		player->bResentThisFrame = 0;
		player = player->pNext;
	}

	receivePackets();

	if (bBooted)
	{
		if (HOSTID!=PLAYERID)
			handleBooting();
		bBooted = false;
	}

	if (bDeadPlayers)
	{
		deleteDeadPlayers();
		bDeadPlayers = false;
	}

	if (bProposedHost)
		handleHostPending();

	const bool bEveryonePaused = isEveryonePaused();

	player = pPlayerList;
	while (player)
	{
		bExtBlockedPlayer = false;
		//
		// check for blocked condition
		//
		if (CQFLAGS.bGameActive && player->playerID != PLAYERID)
		{
			if (player->bPaused)
			{
				player->bWaitingOnFlush = bRemotePlayerPaused = true;
				player->waitingForAckID = player->lastSendID;
			}
			else  
			if (player->bWaitingOnFlush)
			{
				if (NETPLAYER::compare(player->lastAck,player->waitingForAckID) < 0) // we have unack'ed messages
				{
					bExtBlockedPlayer = true;
				}
				else
					player->bWaitingOnFlush = false;
			}
			else
			if (checkReceiveTimeout(player))
			{
				bExtBlockedPlayer = true;
				if (NETPLAYER::compare(player->lastAck, player->lastSendID) < 0)	// if we have unack'ed messages
				{
					player->bWaitingOnFlush = true;
					player->waitingForAckID = player->lastSendID;
				}
			}
		}

		if (checkBlockedTimeout(player))
		{
			bRemotePlayerBlocked = true;
		}

		if (checkSendNackTimeout(player))
		{
			// we lost a packet!
			BASE_PACKET packet;

			packet.type = PT_NACK;
			packet.dwSize = sizeof(packet);
			packet.ackID = player->lastReceiveID;
			packet.sequenceID = player->lastSendID;
			if (CQFLAGS.bTraceNetwork)
				NETPRINT2("SND NACK of packet %d to player %S", player->lastReceiveID, player->getPlayerName());
			guardedSend(player->playerID, 0, &packet);
			player->sendTimeout = 0;
		}
		else
		if (checkSendTimeout(player))
		{
			// don't bother sending acks if we are host, remote machine is host and there are no unack'd packets
			if (PLAYERID!=HOSTID || player->playerID!=HOSTID || player->sentPackets!=0)
			{
				//
				// we haven't sent anything in a while, send am ACK packet
				//
				BASE_PACKET packet;

				packet.type = PT_ACK;
				packet.dwSize = sizeof(packet);
				packet.ackID = player->lastReceiveID;
				packet.sequenceID = player->lastSendID;

				if (CQFLAGS.bTraceNetwork)
					NETPRINT2("SND ACK of packet %d to player %S", player->lastReceiveID, player->getPlayerName());
				guardedSend(player->playerID, 0, &packet);
				player->sendTimeout = 0;
			}
		}

		if ((bEveryonePaused==0 && player->bPaused) || player->bTurtled)
			player->totalPauseTime += dt;

		// issue a pause warning if player is over the time limit
		if (HOSTID==PLAYERID && player->totalPauseTime > PAUSE_TIME_ALLOWED)
		{
			if (player->bMarkedForDeath==false)
			{
				if (player->totalPauseTime > PAUSE_BOOT_TIME)
				{
					// boot the player!
					if (CQFLAGS.bTraceNetwork || CQFLAGS.bTraceMission)
						NETPRINT1("Pause time exceeded, booting player: %S", player->getPlayerName());
					NETBUFFER->DestroyPlayer(player->playerID);
				}
				else
				if (player->bPauseWarningIssued==false)
				{
					PAUSEWARNING_PACKET packet;
					packet.dpid = player->playerID;
					Send(0, NETF_ALLREMOTE | NETF_ECHO, &packet);		// you've been warned!
				}
				player->bPauseWarningIssued = true;
			}
		}

		if (HOSTID == PLAYERID && player->playerID!=PLAYERID)
		{
			if (player->bTurtled != bExtBlockedPlayer)
			{
				TURTLE_PACKET packet(bExtBlockedPlayer, player->playerID);
				Send(0, NETF_ALLREMOTE, &packet);		// player has turtled!
				player->bTurtled = bExtBlockedPlayer;
			}
		}
		else
		if (HOSTID != PLAYERID && player->playerID==HOSTID)
		{
			player->bTurtled = bExtBlockedPlayer;
		}

		if (player->bTurtled && CQFLAGS.bGameActive)
			bRemotePlayerTurtled = true;

		player = player->pNext;
	} // end while (player)

	// cannot bring up pause menu while in front-end menus
	if (CQFLAGS.bGameActive)
	{
		if (bOldNetPaused != (bRemotePlayerTurtled||bRemotePlayerPaused))			// has remote pause state changed?
		{
			int code = (bRemotePlayerTurtled||bRemotePlayerPaused) ? 1 : 0;
			EVENTSYS->Send(CQE_NETPAUSED, (void*)code);
		}
	}
}
//----------------------------------------------------------------------------------
//
void NetPacket::onLocalPause (bool _bLocalPause)
{
	bLocalPaused = _bLocalPause;

	PAUSE_PACKET packet(_bLocalPause, 0);
	Send(HOSTID, 0, &packet);
}
//----------------------------------------------------------------------------------
//
NETPLAYER * NetPacket::findPlayer (DPID dpid) const
{
	NETPLAYER * result = pPlayerList;

	while (result && result->playerID != dpid)
		result = result->pNext;

	if (result==0)
	{
		if (CQFLAGS.bTraceNetwork)
			NETPRINT1("findPlayer failed on 0x%08X", dpid);
	}

	return result;
}
//----------------------------------------------------------------------------------
// send to specific person
//
void NetPacket::send (DPID idTo, const BASE_PACKET * _packet)
{
	if (isPeerPacketType(_packet))
	{
		if (CQFLAGS.bTraceNetwork)
			NETPRINT1("SND PEER packet to player 0x%08X", idTo);
		NETBUFFER->Send(idTo, dwSendFlags, _packet);
	}
	else
	{
		NETPLAYER * player = findPlayer(idTo);
		if (player)
		{
			PACKET_NODE * node = player->addToList(player->sentPackets, _packet);

			player->lastSendID++;
			node->sequenceID = node->pPacket->sequenceID = player->lastSendID;
			node->pPacket->ackID = player->lastReceiveID;
			{
				if (CQFLAGS.bTraceNetwork)
					NETPRINT2("SND %d to player %S", node->pPacket->sequenceID, player->getPlayerName());
				guardedSend(idTo, dwSendFlags, *(node->pPacket));
				player->sendTimeout = 0;
			}
		}
	}
}
//----------------------------------------------------------------------------------
// broadcast to everyone, and maybe to self as well
//
void NetPacket::broadcast (DWORD dwFlags, const BASE_PACKET * _packet)
{
	NETPLAYER * player = pPlayerList;
	SUPERBASE_PACKET * packet = 0;
	
	CQASSERT((dwFlags & NETF_ALLREMOTE) != 0);

	while (player)
	{
		if (player->playerID != PLAYERID || (dwFlags & NETF_ECHO))
		{
			if (isPeerPacketType(_packet))
			{
				if (CQFLAGS.bTraceNetwork)
					NETPRINT1("SND PEER packet to player %S", player->getPlayerName());
				NETBUFFER->Send(player->playerID, dwSendFlags, _packet);
			}
			else
			{
				PACKET_NODE * node;

				if (packet==0)
				{
					node = player->addToList(player->sentPackets, _packet);
					packet = node->pPacket;
				}
				else
				{
					node = player->addToList(player->sentPackets, packet);	// reuse the same packet data
				}

				player->lastSendID++;
				node->sequenceID = packet->sequenceID = player->lastSendID;
				packet->ackID = player->lastReceiveID;
				{
					if (CQFLAGS.bTraceNetwork)
						NETPRINT2("SND %d to player %S", packet->sequenceID, player->getPlayerName());
					guardedSend(player->playerID, dwSendFlags, *packet);
					player->sendTimeout = 0;
				}
#if 0
				if (DEBUG_bDoException)
					player = NULL;		// this will cause an exception on the next line
#endif
			
			}
		}
		player = player->pNext;
	}
}
//----------------------------------------------------------------------------------
// receive an acknowledgement from remote side
//
void NetPacket::receiveAck (NETPLAYER * player, const BASE_PACKET *packet)
{
	int i;

	i = player->receiveAck(packet->ackID);

	if (CQFLAGS.bTraceNetwork)
	{
		if (i)
			NETPRINT3("RCV %sACK %d from player %S", (packet->type == PT_NACK)?"N":"", packet->ackID, player->getPlayerName());
		else
		if (packet->type == PT_ACK || packet->type == PT_NACK)
			NETPRINT3("RCV Duplicate %sACK %d from player %S", (packet->type == PT_NACK)?"N":"", packet->ackID, player->getPlayerName());
	}
	
	// don't resend if we already sent something this frame
	if (packet->type == PT_NACK && player->bResentThisFrame==0)
		player->resend(1);
	else
	if (CQFLAGS.bTraceNetwork)
	{
		if (packet->type == PT_NACK)
			NETPRINT1("Chose not to resend %d", player->lastAck+1);
	}


	player->receiveTimeout = 0;

	//
	// if packet->sequenceID is not the last packet we received && our receive list is empty,
	//    NOTE: We may have lost the packet that tells us who the host is (on startup or after
	//	  a host migration. If this happens, no timeouts can occur. We should send back a 
	//    packet now in that case.
	//    we've missed some packets from remote side. Send a NACK.
	//
	if (player->playerID != PLAYERID)		// don't send NACK's to yourself (infinite loop)
	{
		if (packet->type == PT_ACK || packet->type == PT_NACK)
		{
			if (packet->sequenceID != player->lastReceiveID)
			{
				// we lost a packet!
				BASE_PACKET packet;

				packet.type = PT_NACK;
				packet.dwSize = sizeof(packet);
				packet.ackID = player->lastReceiveID;
				packet.sequenceID = player->lastSendID;
				if (CQFLAGS.bTraceNetwork)
					NETPRINT3("SND NACK (I sent you %d) of packet %d to player %S", player->lastSendID, player->lastReceiveID, player->getPlayerName());
				guardedSend(player->playerID, 0, &packet);
				player->sendTimeout = 0;
			}
		}
	}

	//
	// for debugging, remember the age of packets from host
	//

	if (packet->fromID == HOSTID)
		dwHostLatency = NETBUFFER->GetPacketAge(packet);
}
//----------------------------------------------------------------------------------
//
void NetPacket::handleHostPending (void)
{
	NETPLAYER * player = pPlayerList;
	bool bAllAcked = true;		// if this stays true, pending state is done

	while (player)
	{
		if (player->playerID != PLAYERID)
		{
			if (player->bReceivedHostAck == false)
			{
				bAllAcked = false;

				if (checkHostPendSendTimeout(player))
				{
					//
					// send a proposed host message
					//
					char buffer[512];
					HOSTPEND_PACKET * packet = (HOSTPEND_PACKET *) buffer;
					packet->HOSTPEND_PACKET::HOSTPEND_PACKET(gameStatePacketsReceived);
					
					if (pLastHostPacket)
					{
						memcpy(packet->data, &pLastHostPacket->dwSize, pLastHostPacket->dwSize);
						packet->dwSize = sizeof(HOSTPEND_PACKET) + pLastHostPacket->dwSize;
					}
					else
					{
						packet->dwSize = sizeof(HOSTPEND_PACKET);
					}
					guardedSend(player->playerID, 0, packet);
					player->sendTimeout = 0;
					if (CQFLAGS.bTraceNetwork)
						NETPRINT2("Sending HOSTPEND(%d) to player %S", gameStatePacketsReceived, player->getPlayerName());
				}
			}
		}

		player = player->pNext;
	}

	//
	//	if everyone has acknowledged the pending change, send host change packet
	//
	if (bAllAcked)
	{
		NEWHOST_PACKET packet(gameStatePacketsReceived);

		if (CQFLAGS.bTraceNetwork)
			NETPRINT1("Sending PT_HOST packet. Packets=%d", gameStatePacketsReceived);
		Send(0, NETF_ALLREMOTE|NETF_ECHO, &packet);
		SendZoneHostChange();

		bProposedHost = false;
	}

}
//----------------------------------------------------------------------------------
//
void NetPacket::receiveUpdateFromClient (const HOSTPENDACK_PACKET * packet)
{
	const BASE_PACKET * pBase = (const BASE_PACKET *) packet->data;
	CQASSERT(packet->dwSize == sizeof(HOSTPENDACK_PACKET) || pBase->type == PT_HOSTUPDATE);

	if (packet->dwSize > sizeof(HOSTPENDACK_PACKET))
	{
		SUPERBASE_PACKET * super = (SUPERBASE_PACKET *) netheap->AllocateMemory(pBase->dwSize + sizeof(SUPERBASE_PACKET) - sizeof(BASE_PACKET));
		super->refCount = 1;
		memcpy(&super->dwSize, pBase, pBase->dwSize);
		super->fromID = HOSTID;

		EVENTSYS->Send(CQE_NETPACKET, (void *)((const BASE_PACKET *)*(super)));  // cast the super_packet into base_packet
		super->release();
	}
}
//----------------------------------------------------------------------------------
//
void NetPacket::sendHostPendAck (U32 dpid)
{
	//
	// send a proposed host ack
	//
	char buffer[512];
	HOSTPENDACK_PACKET * packet = (HOSTPENDACK_PACKET *) buffer;
	NETPLAYER * player = findPlayer(dpid);
	packet->HOSTPENDACK_PACKET::HOSTPENDACK_PACKET(gameStatePacketsReceived);
	
	if (pLastHostPacket)
	{
		memcpy(packet->data, &pLastHostPacket->dwSize, pLastHostPacket->dwSize);
		packet->dwSize = sizeof(HOSTPENDACK_PACKET) + pLastHostPacket->dwSize;
	}
	else
	{
		packet->dwSize = sizeof(HOSTPENDACK_PACKET);
	}
	guardedSend(dpid, 0, packet);
	if (player)
	{
		player->sendTimeout = 0;
		if (CQFLAGS.bTraceNetwork)
			NETPRINT2("Sending HOSTPENDACK(%d) to player %S", gameStatePacketsReceived, player->getPlayerName());
	}
}
//----------------------------------------------------------------------------------
//
void NetPacket::on3DEnable (bool bEnable)
{
	CreateDrawAgent("TurtleNet.bmp", TEXTURESDIR, DA::BMP, 0, turtleShape);
}
//----------------------------------------------------------------------------------
//
void NetPacket::onGameActive (bool bActivate)
{
	if (bActivate==0)
	{
	}
}
//----------------------------------------------------------------------------------
//
void NetPacket::init (void)	// called once at startup
{
	COMPTR<IDAConnectionPoint> connection;

	if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Advise(getBase(), &eventHandle);
}
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//
struct _netpacket : GlobalComponent
{
	NetPacket * net;

	virtual void Startup (void)
	{
		NETPACKET = net = new DAComponent<NetPacket>;
		AddToGlobalCleanupList(&NETPACKET);
	}

	virtual void Initialize (void)
	{
		net->init();
		netheap->SetErrorHandler(FDUMP);
	}
};

static _netpacket startup;

}  // end namespace __NETPACKET
//----------------------------------------------------------------------------------
//---------------------------End NetPacket.cpp--------------------------------------
//----------------------------------------------------------------------------------
