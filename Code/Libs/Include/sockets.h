// **************************************************** *
// Sockets.h/.cpp
//  socket manager and socket wrapper
//
//  Copyright (C) Digital Anvil, Inc. 1998 (tlp)
// **************************************************** *

#ifndef SOCKETS_H
#define SOCKETS_H

#include "typedefs.h"
#include "msgbuffer.h"

#include <winsock2.h>

//---------------------------------------------------------------------------
//for symbols exported by the dll that contains code common
//to both the server and the client.

#if (BUILDING_SOCKETS)
	#define SOCKETS_EXP __declspec (dllexport)
#else
	#define SOCKETS_EXP __declspec (dllimport)
#endif

//---------------------------------------------------------------------------
struct socket_stats
{
	char desc[32];		// descripition of the object type

	char src[32],		// src connection
		 dest[32];		// dest connection

	char state[32];		// state of the buffer

	int amt_sent;		// total amt sent
	int amt_recvd;		// total amt recvd

	// resets everything but the description
	void reset( void )
	{
		*src= *dest= *state= 0;
		amt_sent= amt_recvd= 0;
	}

	socket_stats( void )
	{
		*desc= 0;
		reset();
	}

	socket_stats & operator =( const socket_stats & m )
	{
		strncpy( desc, m.desc, 32 );
		strncpy( dest, m.dest, 32 );
		strncpy( src, m.src, 32 );
		strncpy( state, m.state, 32 );

		amt_sent= m.amt_sent;
		amt_recvd= m.amt_recvd;

		return (*this);
	}
};

//---------------------------------------------------------------------------
// BaseSocket
// a somewhat generic interface for transmitting data
// return style modeled after sockets

class BaseSocket
{
public:
	SOCKETS_EXP BaseSocket( void );
	SOCKETS_EXP virtual ~BaseSocket( void );
	
	SOCKETS_EXP void get_stats ( socket_stats * metrics ) const; 
	SOCKETS_EXP void set_stats ( const socket_stats & metrics ); 

	// open allocates local resources
	// it is expected that descendent classes (like sockets)
	// will have specialised connction functions;
	SOCKETS_EXP virtual bool Open( void ) =0;
	SOCKETS_EXP virtual void Close( void ) =0;

	// use datawaiting to poll for pending data
	// return:
	// positive when:
		// a connection has pending data when
		// data exists for reading _or_
		// when remote connection has closed
	// zero when open and no incoming data exists
	// negative on error
	SOCKETS_EXP virtual int DataWaiting( void ) =0;
	
	// non-blocking recv
	// return:
	// positive or zero number of bytes read; negative on error
	// recv'd data is appended to buffer object
	// disregarding (but not altering) current read/write position
	SOCKETS_EXP virtual int RecvBuffer( BufferObjectIO *, void * from_addr=NULL ) =0;

	// non-blocking send
	// return:
	// positive or zero number of bytes sent; negative on error
	// sends the whole buffer object disregarding 
	// (but not altering) current read/write position	
	SOCKETS_EXP virtual int SendBuffer( const BufferObjectIO &, const void * to_addr=NULL ) =0;

protected:
	// (decendents whom) add their contribution to this properly win big cash prizes
	socket_stats metrics;

private:
friend class SocketManager;	
	BaseSocket * next, * prev;
	
private:
	// unless a descendent class overrides
	// the sockets cant be copied around
	void operator =( const BaseSocket & ) { };
};


//---------------------------------------------------------------------------
// SocketManager
// tracks the creation, deletion of all sockets

class SocketManager
{
public:
	// return number of messaging buffers currently in use
	SOCKETS_EXP int count( void ) const;
	SOCKETS_EXP int GetStats( socket_stats * allocated_structs, int num_structs );

	SocketManager(void);

private:
friend class BaseSocket;

	void link( BaseSocket * );
	void unlink( BaseSocket * );

private:		
	BaseSocket * first_entry, * last_entry;
	int cnt;
};

// access to dll static socket manager instance
SocketManager * SocketManager_Acquire( void );

//---------------------------------------------------------------------------
// Inet Sockets

// state flags
#define SOCK_STATE_FBOUND	   (1 << 0)
#define SOCK_STATE_FCONNECTED   (1 << 1)
#define SOCK_STATE_FOPENED	  (1 << 2)
#define SOCK_STATE_FLISTENING   (1 << 3)

// state flag override
#define SOCK_STATE_CLOSED	   (0)

class Socket : public BaseSocket
{
public:
	SOCKETS_EXP Socket( int socktype=0 );
	SOCKETS_EXP virtual ~Socket(void);

	SOCKETS_EXP virtual bool Open( void );
	SOCKETS_EXP virtual void Close( void );

	SOCKETS_EXP virtual int DataWaiting( void );

	// to/from_addr interpreted as sockaddr_in
	SOCKETS_EXP virtual int RecvBuffer( BufferObjectIO *, void * from_addr=NULL );
	SOCKETS_EXP virtual int SendBuffer( const BufferObjectIO &, const void * to_addr=NULL );
   
	// attach socket to a local port
	SOCKETS_EXP bool Bind( const U16 localport = INADDR_ANY );

	// remote host can be in dotted dec or domain name style
	SOCKETS_EXP bool Connect( const char * remotehost, const U16 remoteport );

	// blocking
	// zero or postive number of bytes sent
	// negative winsock return code on error
	// TCP send does not return till all has beeen sent
	SOCKETS_EXP int Send( const U8 * data, const int data_size, const sockaddr_in * to_addr=NULL );

	// blocking
	// postive number of bytes read
	// negative winsock return code on error
	// 0 if connection closed
	SOCKETS_EXP int Recv( U8 * data, const int data_size, sockaddr_in * from_addr=NULL );
	
	// SOCK_DGRAM, SOCK_STREAM
	SOCKETS_EXP int SocketType(void) const;

	// return -1 on error;
	// on multihomed hosts this will return 0 if Bind() specified 
	// INADDR_ANY and socket remains unconnected
	SOCKETS_EXP int LocalPort(void) const;
	SOCKETS_EXP int RemotePort(void) const;

	// return \0 on error; 0.0.0.0 if unknown
	// this probably shouldnt be returning a pointer
	// to memory inside the dll; oh well
	SOCKETS_EXP const char * RemoteHost( void ) const;

	// returns state flags
	SOCKETS_EXP int Status(void) const;

protected:
	SOCKET sock;
	const int socktype;
	sockaddr_in remote, local;
	int state;
	
	// zero out all internal data
	void Clear();

	// record a bound address into the local sockaddr  
	bool RecordBinding( void );

private:	
	// used by data waiting;
	// dont want to create this on the
	// stack and cant make static due to
	// m-thread issues
	fd_set fds;
};

//---------------------------------------------------------------------------
// TCP sockets

class SocketTCP : public Socket
{
public:
	// pass true to allow the internal msgs can grow
	SOCKETS_EXP SocketTCP(bool growable_frames=false);
	
	// start this socket listening for new connections
	SOCKETS_EXP bool BeginListen( void );

	// accept a new connection
	// record the connection into the passed existing socket
	SOCKETS_EXP bool Accept( SocketTCP * socket );
	
	// TCP streams into an internal buffer for Recv
	// and only returns success after whole message
	// has been recvd; prepends a length of size U16
	// on to each message to this chunk streaming
	SOCKETS_EXP virtual int RecvBuffer( BufferObjectIO *, void * from_addr=NULL  );
	SOCKETS_EXP virtual int SendBuffer( const BufferObjectIO &, const void * to_addr=NULL  );
	
private: 
	// cancels the streaming of the current incoming msg
	void ResetRecv( void );

	WriteBuffer WriteMsg;
	ReadBuffer ReadMsg;

	bool readingsize;
	
	U8 * read_ptr;
	int amt_left;
	int final_size;
};

//---------------------------------------------------------------------------
// UDP sockets

class SocketUDP : public Socket
{
public:
	SOCKETS_EXP SocketUDP(void);
	
	SOCKETS_EXP virtual int RecvBuffer( BufferObjectIO *, void * from_addr=NULL  );
	SOCKETS_EXP virtual int SendBuffer( const BufferObjectIO &, const void * to_addr=NULL  );
};

//---------------------------------------------------------------------------
// Broadcast UDP socket
//
// implements a special non-blocking subset of udp 
// that allows a socket to put itself in the recv state, 
// necessary to queue broadcasts,
// without blocking normal execution of the recving thread.

class SocketBroadcast : public Socket
{
public:
	SOCKETS_EXP SocketBroadcast( void );

	SOCKETS_EXP virtual bool Open( void );

	SOCKETS_EXP virtual int SendBuffer( const BufferObjectIO & outfrom, const void * to_addr );
	SOCKETS_EXP virtual int RecvBuffer( BufferObjectIO *, void * from_addr=NULL  );
	
	SOCKETS_EXP int BroadcastBuffer( const BufferObjectIO &, U16 port );
};


//---------------------------------------------------------------------------
// Local Node Socket

// implementation for the "send/recv buffer interface"
// for communication between instanced endpoints in the same application
// without using winsock
// 
// each local node can talk into a specific channel
// from one side or the other; 

class LocalNode : public BaseSocket
{
public:
	enum channel_side
	{
		SERVER_SIDE,
		CLIENT_SIDE
	};

	// there can
	SOCKETS_EXP LocalNode( int which_channel, channel_side side );
	SOCKETS_EXP virtual ~LocalNode( void );

	// local resource allocation done at construction time
	SOCKETS_EXP virtual bool Open( void );
	SOCKETS_EXP virtual void Close( void ); 
	
	SOCKETS_EXP virtual int DataWaiting( void );

	// to/from_addr ignored
	SOCKETS_EXP virtual int RecvBuffer( BufferObjectIO *, void * from_addr= NULL );
	SOCKETS_EXP virtual int SendBuffer( const BufferObjectIO &, const void * to_addr= NULL  );

private:
	ReadBuffer * in;
	WriteBuffer * out;

	bool client;
	int channel;
	int nextmsgsize;
};

SOCKETS_EXP bool LocalNode_ClientConnected( int channel );
SOCKETS_EXP bool LocalNode_ServerConnected( int channel );

#endif // SOCKETS_H
