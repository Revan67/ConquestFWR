// **************************************************** *
// NetLink.h
//
// declarations for 
//  class NetAddr
//  class NetLink
//  class RendezvousPoint
//  namespace RendezvousQuery
//
//  Copyright (C) Digital Anvil, Inc. 1998 (tlp)
// **************************************************** *

#ifndef NETLINK_H
#define NETLINK_H

#include "typedefs.h"
#include "stddat.h"
#include "msgbuffer.h"
#include "sockets.h"

//---------------------------------------------------------------------------
// NetAddr
// a string that holds info about a communication endpoint

class SOCKETS_EXP NetAddr
{
public:
	enum addrtype
	{
		INVALID=0,
		LOCALPASS, 	// "localpass"
		TCP			// "tcpip, <hostname>:<portnumber>"; 
						// a hostname of 'local' ( ie. netaddr == "tcpip, local:<portnumber>') 
						// tells the socket layers to insert the caller's machine's name
	};

	NetAddr(void);
	NetAddr( const char * hoststring );

// output
	// type evaluates the internal string
	addrtype type( void ) const;
	bool addr2tcp( char * host, U16 * port ) const;
	
// input
	void tcp2addr( struct sockaddr_in * );
	void tcp2addr( const char * host, const U16 dstport );
	
// access
	operator char*();
	operator const char*() const;

// copy
	const NetAddr & operator=( const NetAddr & s );
	
private:
	char addr[128];
};

//---------------------------------------------------------------------------
// helper classes;
// shows how to setup netaddrs

class NetAddr_TCP : public NetAddr
{
public:
	NetAddr_TCP ( char * host, U16 port ) { tcp2addr( host, port ); }
	NetAddr_TCP ( struct sockaddr_in * sai ) { tcp2addr( sai ); }
};

class NetAddr_LocalPass : public NetAddr
{
public:
	NetAddr_LocalPass( void ) { strcpy((char*) (*this), "localpass"); }
};

//---------------------------------------------------------------------------
// NetLink
// a communications channel

// status flags; combined to form state
#define LINK_CLOSED		0
#define LINK_CONNECTING	1
#define LINK_ACCEPTING	2
#define LINK_OPEN		3

class NetLink
{
public:
	SOCKETS_EXP NetLink(void);
	SOCKETS_EXP virtual ~NetLink(void);

	//EMAURER needed to manage which heap objects of this type are allocated from.
	//This is a symptom of the use of different heaps by the sockets.dll
	//and the client of this module.
	SOCKETS_EXP void* operator new (size_t);
	SOCKETS_EXP void operator delete (void*);
	

	// link_accept called by rendezvouspoint's pollincoming;
	// not usually needed externally;
	SOCKETS_EXP bool link_accept( int localid, int remoteid, 
									NetAddr::addrtype addr_type,
									BaseSocket * system, BaseSocket * update);

	// connect this link to the remote address
	SOCKETS_EXP bool link_connect( const class NetAddr );

	// terminate the connection
	SOCKETS_EXP void close( void );

	// see #defines above
	SOCKETS_EXP int Status( void );

// flush and poll functions provide a heartbeat for connecting/ accepting sockets;
// both sets should be called whenever a link isnt closed
	SOCKETS_EXP bool FlushUpdate( void );
	SOCKETS_EXP bool FlushSystemMsg( void );

	// note: poll sends internal information during inital connection handshaking
	SOCKETS_EXP int PollSystem( BufferObjectIO * into );
	SOCKETS_EXP int PollUpdate( BufferObjectIO * into );

	// combined update/system msg utility functions
	SOCKETS_EXP bool FlushMessages( void );
	SOCKETS_EXP int  PollMessages( BufferObjectIO * into );

// ids; this player's and the remote player's id.
// for right now player that held the rendevous point always an id == -1
// if connecting local id not valid until link fully open
// if accepting local and remote id valid on RendvezvousPoint::OpenIncoming() success
	SOCKETS_EXP int LocalID( void );
	SOCKETS_EXP int RemoteID( void );

// stats
	SOCKETS_EXP SINGLE curr_latency( void );

	SOCKETS_EXP void SystemStats ( socket_stats * fill );
	SOCKETS_EXP void UpdateStats ( socket_stats * fill );
   
	// do not expect data queued before the link's status is 
	// labeled open to get to the remote side
	WriteBuffer systemMsg;
	WriteBuffer updateMsg;

private:
	int state;

	NetAddr::addrtype addrtype;
	BaseSocket * System;
	BaseSocket * Update;

	S32 localid, remoteid;

	// last received timestamp; 
	// used for redundent and out of order checking
	SINGLE laststamp;
	
	SINGLE echotime, 
		   recvtime;

	bool   ping, // ping the remote connection
		   pong; // remote connecting pinging this

	// current average round trip time
	SINGLE avglatency;
};

//---------------------------------------------------------------------------
// RendezvousPoint
// server or super-peer held connection point
// for joining players.

// game desc is not meant to be extensible
// endian problems could come into existance
struct GameDesc
{
	// might make this a list of addresses
	NetAddr addr;
	
	char name[16];  // this part used to compare connection requests internally
	char desc[16];		  // these fields can be used by the app
	U16  version;		   // to accept or reject connections....

	U16 max_connections,
		num_connections;

	GameDesc( void )
	{
		*name= 0;
		*desc= 0;
		version=0;
		max_connections=
		num_connections=0;
	};
};

class RendezvousPoint
{
public:
	SOCKETS_EXP RendezvousPoint( const GameDesc & );
	SOCKETS_EXP virtual ~RendezvousPoint(void);
	
	//EMAURER needed to manage which heap objects of this type are allocated from.
	//This is a symptom of the use of different heaps by the sockets.dll
	//and the client of this module.
	SOCKETS_EXP void* operator new (size_t);
	SOCKETS_EXP void operator delete (void*);
	
	// open the connection port 
	// listening at the GameDesc.addr's port
	SOCKETS_EXP bool OpenRendezvous( void );

	// close does not deletes the link list
	// only the destructors does
	SOCKETS_EXP void CloseRendezvous( void );
	
	// open the next pending connection if any
	// 1 == new connection; new link at passed var
	// 0 == no new connection
	//-1 == error
	// dont delete this pointer.
	SOCKETS_EXP int PollIncoming( NetLink ** link );

	SOCKETS_EXP int numlinks( void ) const;

	SOCKETS_EXP NetLink * link( const int id );
	
private:
	GameDesc gamedesc;
	
	SocketTCP * connectTCP;
	SocketBroadcast * connectUDP;
	bool connectLocal;

	// class to insure initialisation of 
	// netlink pointers in their dynamic array
	struct pNetLink
	{
		pNetLink() { p = NULL; }
		pNetLink( NetLink * _p ) : p(_p) {}
		
		operator NetLink*() { return p; }
		NetLink * operator->() { return p; }
		NetLink * operator=(NetLink * _p ) { p=_p; return p; }
	private:
		NetLink * p;
	};
	
	// using pointer array to safe the recording 
	// of pointers and to still allow indexed lookups
	// probably no longer needs to be dynamic
	DynamicArray<pNetLink> linklist;
	int numalloced; // track how many valid pointers there are in the dynamic array
};

//---------------------------------------------------------------------------
// Rendezvous Query functions;
// keeping the functions seperated by kind for now 
// still to come: MASTER_SERVER and DPLAY; 
//
// query's dont wont exactly like normal enums
// they call on existing enumeration (passing a game desc ptr)
// or after 0.5 seconds has passed (passing NULL)
// to provide a display heartbeat;
//
// they only stop "enumerating" if the callback returns false

namespace RendezvousQuery
{
	typedef bool (*QueryEnumCallback) ( const GameDesc *, void * context );
	
	// the query uses the game name and the port section
	// of addr to filter/find servers.
	SOCKETS_EXP bool QueryLAN( const GameDesc &, QueryEnumCallback, void * context );
}

#endif // NETLINK_H
