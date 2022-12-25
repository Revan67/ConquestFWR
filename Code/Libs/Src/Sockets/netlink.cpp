#include <assert.h>
#include "netlink.h"
#include "sockets.h"

#include <stdio.h>

static DWORD starttime = timeGetTime();
static float SystemTime(void) { return (float) ((timeGetTime() - starttime) / 1000.0); }

#define UFLAGS_NONE	 (0)
#define UFLAGS_PING	 (1<<0)
#define UFLAGS_PONG	 (1<<1)

#define LOCAL_CHANNEL_UDP 0
#define LOCAL_CHANNEL_TCP 1

//---------------------------------------------------------------------------
NetAddr::NetAddr(void) 
{ 
	*addr = '\0'; 
}

NetAddr::NetAddr( const char * hoststring ) 
{ 
	strncpy(addr, hoststring, 128); 
}

NetAddr::addrtype NetAddr::type( void ) const
{
	if (!addr)
		return INVALID;
		
	if (strstr(addr, "localpass") == addr)
		return LOCALPASS;

	if (strstr(addr, "tcpip") == addr)
		return TCP;

	return INVALID;
}

bool NetAddr::addr2tcp( char * dsthost, U16 * dstport ) const
{
	char comp[128];
	char * tok;
	
	if (type()!= TCP)
		return false;

	strncpy( comp, addr, 128 );

	if ((tok= strtok( comp, ",\t " ))==NULL)
		return false;
		
	if ((tok= strtok( NULL, ":\t " ))==NULL)
		return false;

	strcpy( dsthost, tok );

	if ((tok= strtok( NULL, "\t "))==NULL)
		return false;

	*dstport= (U16) atoi( tok );

	return true;
}

void NetAddr::tcp2addr( const char * host, const U16 dstport )
{
	_snprintf(addr, sizeof( NetAddr ), "tcpip, %s:%d", host, dstport);
}

void NetAddr::tcp2addr( sockaddr_in * sin )
{
	_snprintf(addr, sizeof( NetAddr ), "tcpip, %s:%d",
				 inet_ntoa( sin->sin_addr ),  
					 ntohs( sin->sin_port ));
}

NetAddr::operator char*() 
{ 
	return addr; 
}

NetAddr::operator const char*() const 
{ 
	return addr; 
}

const NetAddr & NetAddr::operator=( const NetAddr & s )
{
	if (this == &s) return (*this);
	
	strncpy(addr, s.addr, sizeof(addr));
	return (*this);
}

//---------------------------------------------------------------------------
NetLink::NetLink(void) :
		System(NULL),
		Update(NULL),
		remoteid(-1),
		localid(-1),
		state(LINK_CLOSED),
		addrtype( NetAddr::INVALID )
{
	// temporary till update array working
	updateMsg.allowoverflow= true;
	systemMsg.allowresize= true;
}

//EMAURER needed to manage which heap objects of this type are allocated from.
//This is a symptom of the use of different heaps by the sockets.dll
//and its client. The fact that the destructor is virtual means that the 
//delete operator that is used is the one that is in scope at the time of
//definition of the destructor. The delete found at that time can operate on a
//different heap than the the new operator used to allocate the object. So
//define a new and delete here and the same heap is guaranteed to be used.
//see c++ spec 12.5.4
void* NetLink::operator new (size_t s)
{
	return ::operator new (s);
}

void NetLink::operator delete (void* d)
{
	::delete (d);
}

// **************************************************** *
NetLink::~NetLink(void)
{
	close();
}

// **************************************************** *
int NetLink::Status( void )
{ 
	return state; 
}

// **************************************************** *
int NetLink::LocalID( void )
{ 
	return localid; 
}

// **************************************************** *
int NetLink::RemoteID( void )
{ 
	return remoteid; 
}

// **************************************************** *
SINGLE NetLink::curr_latency( void ) 
{ 
	return avglatency; 
}

// **************************************************** *
void NetLink::SystemStats ( socket_stats * fill )
{
	assert(System);
	System->get_stats( fill );
}

// **************************************************** *
void NetLink::UpdateStats ( socket_stats * fill )
{
	assert(Update);
	Update->get_stats( fill );
}

// **************************************************** *
bool NetLink::FlushMessages( void )
{
	return (FlushSystemMsg() && FlushUpdate());
}

// **************************************************** *
int NetLink::PollMessages( BufferObjectIO * into )
{
	int res;

	// pull out all system msgs first
	res= PollSystem( into );

	if (res != 0)
		return res;

	return PollUpdate( into );
}

// **************************************************** *
bool NetLink::FlushSystemMsg( void )
{
	if (state != LINK_OPEN)
	{
		// system msgs must be kept in 
		// queue dont clear sys msgs
		return (state != LINK_CLOSED);
	}
	
	if (systemMsg.BufferLength() > 0)
	{
		// might need to chunk this up a little( could be sending a lot here ) (tlp)
		if (System->SendBuffer( systemMsg ) <= 0)
			return false;

		systemMsg.Clear();
	}

	return true;
}

// **************************************************** *
// updates have pings and pongs attached to them
// to estimate the udp connection's latency 
bool NetLink::FlushUpdate( void )
{
	static WriteBuffer tempdate;

	if (state != LINK_OPEN)
	{
		// updates can be dropped
		// clear update msg
		updateMsg.Clear();

		return (state != LINK_CLOSED);
	}

	if ((updateMsg.BufferLength() > 0) || (pong))
	{
		tempdate.Clear();

	// send flags
		S8 flags = 0;

		if (ping) flags |= UFLAGS_PING;
		if (pong) flags |= UFLAGS_PONG;

		tempdate.WriteU8( flags );
		
		// "if (ping)" but always sending time at on UDP msgs
		// to detect out of order and redundent packets
		SINGLE stamptime= SystemTime();
		tempdate.WriteSingle( stamptime );
		// ping=false

		if (pong)
		{
			// subtract time the packet was kept
			// this implicitly assumes that the time 
			// step on client and server are the same
			tempdate.WriteSingle( echotime - (stamptime - recvtime) );
			pong= false;
		}				
	
		tempdate.WriteData( updateMsg.Buffer(), updateMsg.BufferLength() );

		if (Update->SendBuffer( tempdate ) <= 0)
			return false;

		updateMsg.Clear();
	}

	return true;
}

// **************************************************** *
int NetLink::PollSystem( BufferObjectIO * into )
{
	static BufferObjectIO msg;
	
	if (state == LINK_CLOSED)
		return -1;

	msg.Clear();

	int res= System->RecvBuffer( &msg );
	
	if (res <= 0) 
		return res;

	if (state == LINK_OPEN)
	{
		if (!((WriteBuffer*) into)->WriteData( msg.Buffer(), msg.BufferLength() ))
			return -1;

		return res;
	}
	else
	if (state == LINK_ACCEPTING)
	{
		U16 updateport= ((ReadBuffer&) msg).ReadU16();

		if ( addrtype == NetAddr::TCP )
		{
			SocketUDP * update = (SocketUDP *) Update;

			update->Open();
			update->Bind();

			update->Connect( ((SocketTCP *) System)->RemoteHost(), updateport );

			if (!(update->Status() & SOCK_STATE_FCONNECTED))
			{
				close();
				return -1;
			}

			((WriteBuffer&) msg).WriteU16 (update->LocalPort());
		}
		// end tcp specfic accept

		((WriteBuffer&) msg).WriteU16 (RemoteID());

		if (System->SendBuffer( msg ) <= 0)
		{
			close();
			return -1;
		}

		state= LINK_OPEN;
	}
	else
	if (state == LINK_CONNECTING)
	{
		if ( addrtype == NetAddr::TCP )
		{
			SocketUDP * update = (SocketUDP *) Update;

			U16 udpport= ((ReadBuffer&) msg).ReadU16();

			update->Connect( ((SocketTCP *) System)->RemoteHost(), udpport );

			if (!(update->Status() & SOCK_STATE_FCONNECTED))
			{
				close();
				return -1;
			}

		}
		// end tcp specific connect
		
		localid = ((ReadBuffer&) msg).ReadU16();

		state= LINK_OPEN;
	}

	return 0;
}

// **************************************************** *
int NetLink::PollUpdate( BufferObjectIO * into )
{	
	static ReadBuffer read;

	if (state != LINK_OPEN)
	{
		if (state == LINK_CLOSED)
			return -1;
		
		return 0;
	}

	while (1)
	{
		read.Clear();

		int res= Update->RecvBuffer( (BufferObjectIO*) &read );

		if (res <= 0) 
			return res;
			
		SINGLE currtime= SystemTime();

		U8 flags = read.ReadU8();
		SINGLE stampin= read.ReadSingle();
		
		// FIXME-TLP
		// this is all gonna lose precision eventually
		// maybe should make these calculations versus 
		// framecounters or known interval wrapping tickcounts....
		
		// a duplicate or out of order packet
		if ( stampin <= laststamp )
			continue;

		laststamp= stampin;

		// save the remote's current stamp
		// record when we got the stamp to
		// provide 'accurate' latency calcs
		if ( flags & UFLAGS_PING )
		{
			// saving laststamp off
			// in case another packet, non-ping, packet
			// is recv'd before pong gets sent
			echotime = laststamp;
			recvtime = currtime;
			pong= true;
		}

		// calculate latency
		if ( flags & UFLAGS_PONG )
		{
			SINGLE echoed= read.ReadSingle();
			SINGLE traveltime= currtime - echoed;
			
			avglatency= (0.8f * (avglatency)) + ((1.0f-0.8f) * traveltime);
		}

		// give the buffer to the caller
		// minus the packet header
		if (!((WriteBuffer*) into)->WriteData( read.Buffer(read.ReadPos()), read.ReadLeft() ))
			return -1;

		return read.ReadLeft();
	}
}

// **************************************************** *
bool NetLink::link_accept( int _local, int _remote, 
							NetAddr::addrtype _addrtype,
							BaseSocket * system, BaseSocket * update )
{ 
	assert( system && update );

	addrtype= _addrtype;
	System= system;
	Update= update;

	// the update channel (udp) needs to complete full handshaking
	// it is created and filled in during link's poll heartbeat
	state= LINK_ACCEPTING;

	avglatency= 0;
	laststamp= 0;

	pong= false;

	// note that this could be
	// turned on and off but it isnt
	// doing so... 
	// right now ping every chance we get

	// this is a hack so the local channel doesnt ping; 
	// dont mind since ping should be off by default and run externally

	if (update)
		ping =false;
	else
		ping= true;

	localid= _local;
	remoteid= _remote;

	updateMsg.Clear();
	systemMsg.Clear();

	return true;
}

// **************************************************** *
bool NetLink::link_connect( const NetAddr addr ) 
{ 
	state= LINK_CONNECTING;

	avglatency= 0;
	laststamp= 0;

	pong= false;

	// note that this could be turned on and off 
	// but it isnt doing so...
	// right now ping every chance we get
	ping= true;

	addrtype= addr.type();

	// FIXME? every link needs inital data push to start
	// prime the pollSystem pump;
	// local port's it in tcp; 
	// so using it as a dummy for everything
	U16 updateport=0;

	if (addrtype == NetAddr::LOCALPASS)
	{
		LocalNode* system = new LocalNode(LOCAL_CHANNEL_TCP, LocalNode::CLIENT_SIDE);
		System = (BaseSocket *) system;

		LocalNode* update = new LocalNode(LOCAL_CHANNEL_UDP, LocalNode::CLIENT_SIDE);
		Update = (BaseSocket *) update;

		if (!System || !System->Open() ||
			!Update || !Update->Open())
		{
			return false;
		}

		ping = false;
// this has to go through its connecting phase (tlp)
//		state= LINK_OPEN;
	}
	else
	if (addrtype == NetAddr::TCP)
	{
		char hostname[256];
		U16 port;

		if (!addr.addr2tcp( hostname, &port ))
		{
			close();
			return false;
		}

		SocketTCP * system =new SocketTCP(true);
		System = (BaseSocket *) system;

		SocketUDP * update =new SocketUDP();
		Update = (BaseSocket *) update;
		
		if (!System || !Update)
		{
			close();
			return false;
		}

		system->Open();
		system->Bind();

		if (!system->Connect( hostname, port ))
		{
		#ifdef _DEBUG
			assert(!"server running");
		#endif
			close();
			return false;
		}

		update->Open();

		// FIXME-TLP
		// allow the user to specify this port
		// perhaps through a gerneric configuration
		// description or ini file since there
		// is the potential for many different 
		// communication implementation
		if (!update->Bind())
		{
			close();
			return false;
		}

		// first half of connnection handshake
		// in return we expect the server local port
		// and our server defined id
		updateport= update->LocalPort();
	}

	updateMsg.Clear();
	systemMsg.Clear();

	systemMsg.WriteU16( updateport );

	if (System->SendBuffer( systemMsg ) <= 0)
	{
		close();
		return false;
	}

	systemMsg.Clear();
	
	return true;
}

// **************************************************** *
void NetLink::close( void )
{
	state= LINK_CLOSED;
	// remote and local ids not 
	// changed;  systems should use
	// state var to determine link state

	if(System) { delete System; System=0; }
	if(Update) { delete Update; Update=0; }
}

//---------------------------------------------------------------------------
// RendezvousPoint

RendezvousPoint::RendezvousPoint( const GameDesc & gd ) :
	connectTCP(NULL),
	connectUDP(NULL),
	connectLocal(false),
	numalloced(0)
{
	memcpy( &gamedesc, &gd, sizeof(GameDesc) );
}

//EMAURER needed to manage which heap objects of this type are allocated from.
//This is a symptom of the use of different heaps by the sockets.dll
//and its client. The fact that the destructor is virtual means that the 
//delete operator that is used is the one that is in scope at the time of
//definition of the destructor. The delete found at that time can operate on a
//different heap than the the new operator used to allocate the object. So
//define a new and delete here and the same heap is guaranteed to be used.
//see c++ spec 12.5.4
void* RendezvousPoint::operator new (size_t s)
{
	return ::operator new (s);
}

void RendezvousPoint::operator delete (void* d)
{
	::delete (d);
}

// **************************************************** *
RendezvousPoint::~RendezvousPoint( void )
{
	CloseRendezvous();

	for (int i=0; i< numalloced; i++)
		delete linklist[i];

	linklist.free();
}

// **************************************************** *
void RendezvousPoint::CloseRendezvous( void )
{
	if (connectTCP)
		delete connectTCP;
	if (connectUDP)
		delete connectUDP;

	// nothing special has to
	// be done to the connectLocal flag
}

// **************************************************** *
bool RendezvousPoint::OpenRendezvous( void )
{
	char host[256];
	U16 port;

	if (gamedesc.addr.type() == NetAddr::LOCALPASS)
	{
		connectLocal = true;
		return true;
	}
	else
	if (gamedesc.addr.type() == NetAddr::TCP)
	{
		if (!gamedesc.addr.addr2tcp( host, &port ))
		{
			assert(0 && "this should never have happened");
			return false;
		}

		if (stricmp(host, "local"))
			return false;

		OutputDebugString("RENDEZVOUS: opening listen function\n");
		
		if (connectTCP || connectUDP)
		{
			OutputDebugString("RENDEZVOUS: connect socket already open\n");
			return false;
		}

		if ((((connectTCP = new SocketTCP) == NULL)) || 
			 ((connectUDP = new SocketBroadcast) == NULL))
		{
			OutputDebugString("RENDEZVOUS: Couldnt create connect socket\n");
			return false;
		}
		
		connectTCP->Open();
		connectTCP->Bind( port );
		connectTCP->BeginListen();

		connectUDP->Open();
		connectUDP->Bind( port );

		return ( ((connectTCP->Status() & SOCK_STATE_FLISTENING) != 0) &&
				 ((connectUDP->Status() & SOCK_STATE_FBOUND) != 0) );
	}

	return false;
}

// **************************************************** *
int RendezvousPoint::PollIncoming( NetLink ** newclient )
{
	(*newclient)= NULL;

// check for udp based inquiries
	if (connectUDP)
	{
		while (1)
		{
			sockaddr_in addr;
			BufferObjectIO umsg( sizeof(GameDesc) );

			umsg.Clear();
			int res= connectUDP->RecvBuffer(&umsg, &addr);

			if (res ==0)
				break;

			if (res < 0)
				return -1;

			if (((unsigned int) umsg.BufferLength() != strlen(gamedesc.name) + 1) ||
				(strcmp((const char*) umsg.Buffer(), gamedesc.name)) )
			{
				continue;
			}

			umsg.Clear();
			WriteBuffer * write= (WriteBuffer*) &umsg;
			write->WriteString( gamedesc.name );
			write->WriteString( gamedesc.desc );
			write->WriteU16( gamedesc.version );

			write->WriteU16( gamedesc.max_connections );
			// there is no exact count available to the rendezvous
			// because closes can happen outside of it
			write->WriteU16( SocketManager_Acquire()->count() /2 -1);

			connectUDP->SendBuffer( umsg, &addr );
		}
	}

	NetAddr::addrtype addrtype= NetAddr::INVALID;
	BaseSocket * system=NULL, * update= NULL;

//
// look locally for connections
//
	if ((addrtype==NetAddr::INVALID) && connectLocal)
	{
		if (LocalNode_ClientConnected( LOCAL_CHANNEL_TCP ) && !LocalNode_ServerConnected( LOCAL_CHANNEL_TCP ))
		{
			LocalNode * connect_system= new LocalNode(LOCAL_CHANNEL_TCP, LocalNode::SERVER_SIDE);
			LocalNode * connect_update= new LocalNode(LOCAL_CHANNEL_UDP, LocalNode::SERVER_SIDE);

			if ( connect_system && connect_system->Open() &&
				 connect_update && connect_update->Open() )
			{
				system= connect_system;
				update= connect_update;
			}
			else
			{
				assert( 0 && "this is bad");
				delete connect_update;
				delete connect_system;
				return -1;
			}

			addrtype= NetAddr::LOCALPASS;
		}
	}

//
// look remotely for new connections
//
	if ((addrtype==NetAddr::INVALID) && connectTCP)
	{
		int res= connectTCP->DataWaiting();
		
		if (res < 0)
			return -1;

		if (res > 0)
		{
			OutputDebugString("CheckConnect: incoming connection...\n");
			
			SocketTCP * connect_system= new SocketTCP(true);
			SocketUDP * connect_update= new SocketUDP();
			
			if (!connectTCP->Accept( connect_system ) ||
				!connect_update)
			{
				OutputDebugString("CheckConnect: accept failed\n");
				delete connect_system;
				delete connect_update;
				return -1;
			}

			system= connect_system;
			update= connect_update;

			addrtype= NetAddr::TCP;
		}
	}

	if (addrtype != NetAddr::INVALID)
	{
		// find a free entry
		for (int i=0; i< numalloced; i++)
		{
			if (linklist[i]->Status() == LINK_CLOSED)
				break;
		}

		// possible implict array expansion
		if (linklist[i] == NULL)
		{
			// cap number of openings in the array
			if (numalloced >= gamedesc.max_connections)
			{
				OutputDebugString("CheckConnect: no more room for new players\n");
				delete system;
				delete update;
				return -1;
			}
			
			linklist[i]= new NetLink;

			if (!linklist[i])
			{
				assert( !"out of memory for new connection" );
				delete system;
				delete update;
				return -1;
			}

			++numalloced;
		}

		if (!linklist[i]->link_accept( -1, i, addrtype, system, update ))
		{
			OutputDebugString("CheckConnect: couldnt accept new player\n");
			delete system;
			delete update;
			return -1;
		}

		OutputDebugString("CheckConnect: connection initiated.\n");

		(*newclient)=linklist[i];
	
		return 1;
	}

	return 0;
}

// **************************************************** *
int RendezvousPoint::numlinks( void ) const
{
	return numalloced;
}

// **************************************************** *
NetLink * RendezvousPoint::link( const int id )
{
	assert( (id >=0) && (id < numalloced) );
	return linklist[ id ];
}

//---------------------------------------------------------------------------

namespace RendezvousQuery
{
	// query gives one null call back after succesfully finding a group
	// of lan servers in order to designate the end of group
	bool QueryLAN( const GameDesc & lookup_desc, QueryEnumCallback qecb, void * context )
	{
		char lookup_host[256];
		U16 lookup_port;
		lookup_desc.addr.addr2tcp( lookup_host, &lookup_port );

		SocketBroadcast sock;

		sock.Open();
		sock.Bind();

		if (!(sock.Status() & SOCK_STATE_FBOUND))
			return false;

		bool callback = true;
		BufferObjectIO umsg( sizeof(GameDesc ) );

		while ( callback )
		{
		// broadcast gamedesc
			// server only looks at name right now
			// so only send the name
			umsg.Clear();
			((WriteBuffer &)umsg).WriteString( lookup_desc.name );
			sock.BroadcastBuffer( umsg, lookup_port );

		// give the servers some time to respond 
			Sleep(1500);

			// read response
			GameDesc gamedesc;
			struct sockaddr_in addr;
			bool response= true;
			
			while ( response && callback )
			{
				umsg.Clear();
				int res= sock.RecvBuffer( &umsg, &addr );

				if (res < 0)
					return false;

				response = res > 0;

				if (!response)
				{
					callback = qecb( NULL, context );
				}
				else
				{
					ReadBuffer * read= (ReadBuffer*) &umsg;

					gamedesc.addr.tcp2addr( &addr );
					read->ReadString( gamedesc.name );
					read->ReadString( gamedesc.desc );

					gamedesc.version= read->ReadU16();
					gamedesc.max_connections= read->ReadU16();
					gamedesc.num_connections= read->ReadU16();
		
					callback = qecb( (const GameDesc *) &gamedesc, context );
				}
			}
		}

		return true;
	}
}
