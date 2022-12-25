#include "sockets.h"
#include <assert.h>
#include <stdio.h>

//---------------------------------------------------------------------------
// Socket Manager
//---------------------------------------------------------------------------

// control that binding time!
class SocketManager * SocketManager_Acquire( void )
{
	static SocketManager static_SocketManager;
	return &static_SocketManager;
}

//---------------------------------------------------------------------------
SocketManager::SocketManager(void)
{
	first_entry = NULL;
	last_entry  = NULL;
	cnt		 	= 0;
}

//---------------------------------------------------------------------------
int SocketManager::count(void) const
{
	return cnt;
}

//---------------------------------------------------------------------------
void SocketManager::link( BaseSocket * entry )
{
	BaseSocket * prev;

	prev = last_entry;
	last_entry = entry;

	if (prev == NULL)
	{
		first_entry = entry;
	}
	else
	{
		prev->next = entry;
	}

	entry->next = NULL;
	entry->prev = prev;

	++cnt;
}

//---------------------------------------------------------------------------
void SocketManager::unlink( BaseSocket * entry )
{
	if (entry->prev == NULL)
	{
		first_entry = entry->next;
	}
	else
	{
		entry->prev->next = entry->next;
	}

	if (entry->next == NULL)
	{
		last_entry = entry->prev;
	}
	else
	{
		entry->next->prev = entry->prev;
	}

	--cnt;
}

//---------------------------------------------------------------------------
int SocketManager::GetStats( socket_stats * list, int size )
{
	BaseSocket * trace = first_entry;

	for (int i=0; (i< size) && (trace); i++, trace= trace->next)
	{
		memcpy( &list[i], &trace->metrics, sizeof(socket_stats) );
	}

	return count();
}

//---------------------------------------------------------------------------
// Base Socket Class
//---------------------------------------------------------------------------

BaseSocket::BaseSocket( void )
{
	SocketManager_Acquire()->link(this);
}

BaseSocket::~BaseSocket( void )
{
	SocketManager_Acquire()->unlink(this);
}

void BaseSocket::get_stats ( socket_stats * met ) const
{
	(*met) = metrics;
}

void BaseSocket::set_stats ( const socket_stats & met )
{
	metrics = met;
}

//---------------------------------------------------------------------------
// Inet Socket Class
//---------------------------------------------------------------------------

// use WSAGetLastError to log
// whatever socket error just occured
int SocketError( void );

void QueryWinSock ( void );
void ReleaseWinSock ( void );

//---------------------------------------------------------------------------
Socket::Socket( int stype ) :
	socktype(stype)
{
	QueryWinSock();

	Clear();
}

//---------------------------------------------------------------------------
Socket::~Socket( void )
{
	Close();

	ReleaseWinSock();
}

//---------------------------------------------------------------------------
int Socket::SocketType(void) const
{
	return socktype;
}

//---------------------------------------------------------------------------
int Socket::LocalPort(void) const
{
	return ntohs(local.sin_port);
}

//---------------------------------------------------------------------------
int Socket::RemotePort(void) const
{
	return ntohs(remote.sin_port);
}

//---------------------------------------------------------------------------
int Socket::Status(void) const
{
	return state;
}

//---------------------------------------------------------------------------
const char * Socket::RemoteHost(void) const
{
	char * ra= inet_ntoa( remote.sin_addr );

	if (ra)
		return ra;
	else
		return '\0';
}

//---------------------------------------------------------------------------
void Socket::Clear( void )
{
	sock= INVALID_SOCKET;

	memset(&local, 0, sizeof(sockaddr_in));
	memset(&remote, 0, sizeof(sockaddr_in));

	metrics.reset();
	state= 0;

	strncpy( metrics.dest, "unconnected", 64 );
}

//---------------------------------------------------------------------------
bool Socket::RecordBinding( void )
{
// record local binding when it doesnt exist
	if (!local.sin_port)
	{
		int addrlen= sizeof(sockaddr_in);

		if (getsockname(sock, (struct sockaddr *) &local, &addrlen))
		{
			SocketError();
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------
bool Socket::Open( void )
{
	// socket in use
	if (sock != INVALID_SOCKET)
	{
		OutputDebugString("Socket::Open: socket in use");
		return false;
	}

	if ((sock = socket( AF_INET, socktype, 0)) == INVALID_SOCKET)
	{
		SocketError();
		return false;
	}

	state|= SOCK_STATE_FOPENED;

	return true;
}

//---------------------------------------------------------------------------
void Socket::Close( void )
{
	if (sock != INVALID_SOCKET)
		closesocket( sock );

	Clear();
}

//---------------------------------------------------------------------------
bool Socket::Bind( const U16 port )
{
	local.sin_family= AF_INET;
	local.sin_addr.s_addr= htonl(INADDR_ANY);
	local.sin_port= htons(port);

	if (bind(sock, (struct sockaddr *) &local, sizeof(sockaddr_in)))
	{
		SocketError();
		return false;
	}

	if (!RecordBinding())
	{
		SocketError();
		return false;
	}

	state|= SOCK_STATE_FBOUND;

	return true;
}

//---------------------------------------------------------------------------
bool Socket::Connect(const char * hostname, const U16 port)
{
// fill remote address structure
	remote.sin_family= AF_INET;
	remote.sin_port = htons(port);

	// translate dotted dec into an numbered address
	if ((remote.sin_addr.s_addr = inet_addr( hostname )) == htonl(INADDR_NONE))
	{
		// failed dotted dec; try passed string as host domain name
		struct hostent * phe= gethostbyname( hostname );

		if (phe)
			memcpy( &remote.sin_addr, phe->h_addr, phe->h_length );
		// failed host domain name lookup
		else
		{
			SocketError();
			return false;
		}
	}

// connect to remote socket
	if (connect(sock, (struct sockaddr *) &remote, sizeof(sockaddr_in)))
	{
		SocketError();
		return false;
	}

// local binding guarented now
	if (!RecordBinding( ))
	{
		Close();
		return false;
	}

	state|= SOCK_STATE_FCONNECTED;
	_snprintf( metrics.dest, 64, "%s:%d", RemoteHost(), RemotePort() );

// return success
	return true;
}

//---------------------------------------------------------------------------
int Socket::DataWaiting( void )
{
	static timeval timeout = { 0, 0 };

	FD_ZERO( &fds );
	FD_SET( sock, &fds );

	int ret= select( NULL, &fds, NULL, NULL, &timeout);

	if (ret < 0)
	{
		SocketError();
		return -1;
	}

	return ret;
}

//---------------------------------------------------------------------------
int Socket::Send( const U8 * data, const int data_size,
				  const sockaddr_in * to_addr )
{
	int amt;

	if (!to_addr)
	{
		amt = send( sock, (const char*) data, data_size, NULL );
	}
	else
	{
		amt = sendto( sock, (const char*) data,  data_size, NULL,
							(const struct sockaddr *) to_addr,
							sizeof(sockaddr_in));
	}

	if (amt < 0)
	{
		SocketError();
		return -1;
	}

	metrics.amt_sent+= amt;

	if (amt < data_size)
	{
		OutputDebugString("Socket::Send: couldnt send full packet");
		return -1;
	}

	return amt;
}

//---------------------------------------------------------------------------
// returns 0 if connection closed
// number of bytes read
// or negative on error
int Socket::Recv( U8 * data, int max_data_size,
				  sockaddr_in * from_addr )
{
	int amt;

	if (!from_addr)
	{
		amt= recv( sock, (char *) data, max_data_size, NULL );
	}
	else
	{
		int fromsize = sizeof(sockaddr_in);

		amt = recvfrom( sock, (char *) data, max_data_size, NULL,
							  (struct sockaddr *) from_addr, &fromsize);
	}

	if (amt < 0)
	{
		SocketError();
		return -1;
	}

	metrics.amt_recvd+= amt;

	return amt;
}

//---------------------------------------------------------------------------
// truely the socketsudp and tcp should be renamed and
// should have multiple inheritance through socket and
// msgbuffer; rather than single through socket
// and these should be removed.... oh well.
int Socket::RecvBuffer( BufferObjectIO *, void * from_addr )
{ return -1; }

int Socket::SendBuffer( const BufferObjectIO &, const void * to_addr )
{ return -1; }

//---------------------------------------------------------------------------
// SocketTCP Class
//---------------------------------------------------------------------------

SocketTCP::SocketTCP( bool resizeable ) :
	Socket( SOCK_STREAM )
{
	WriteMsg.allowresize= resizeable;
	ReadMsg.allowresize= resizeable;
	
	ResetRecv();
	strncpy( metrics.desc, "SocketTCP", 32);
}

//---------------------------------------------------------------------------
bool SocketTCP::BeginListen( void )
{
	if (listen( sock, SOMAXCONN ))
	{
		SocketError();
		return false;
	}

	state|= SOCK_STATE_FLISTENING;
	_snprintf( metrics.dest, 64, "local:%d", LocalPort() );

	return true;
}

//---------------------------------------------------------------------------
bool SocketTCP::Accept( SocketTCP * newsock )
{
	if (!newsock)
	{
		OutputDebugString("SocketTCP::Accept: called with NULL newsock");
		return false;
	}

// zero out dest socket
	newsock->Clear();

	int addrlen= sizeof(sockaddr_in);

// allow a new connection to occur
	newsock->sock= accept( sock,
						  (struct sockaddr *) &newsock->remote,
						  &addrlen );

	if (newsock->sock == INVALID_SOCKET)
	{
		SocketError();
		return false;
	}

// record new's local structure
	if (!newsock->RecordBinding())
	{
		return false;
	}

// record new's state and socket type
	newsock->state= SOCK_STATE_FOPENED|
							SOCK_STATE_FCONNECTED|
							SOCK_STATE_FBOUND;

	_snprintf( newsock->metrics.dest, 64, "%s:%d",
			   newsock->RemoteHost(),
			   newsock->RemotePort() );

	return true;
}

//---------------------------------------------------------------------------
void SocketTCP::ResetRecv(void )
{
	readingsize= true;
	amt_left= sizeof(U16);

	ReadMsg.Clear();
	read_ptr= ReadMsg.Buffer();
	// allow possibly endian ordered
	// u16 message length to be (later) read
	// off the begining of the message
	ReadMsg.SeekWrite( sizeof(U16) );
}

//---------------------------------------------------------------------------
int SocketTCP::RecvBuffer( BufferObjectIO * into, void * from_addr )
{
	int res;

	while ((res=DataWaiting()) > 0)
	{
		if (amt_left > 0)
		{
			int amt = Recv( read_ptr, amt_left, (sockaddr_in *) from_addr );

			// consider 0 data received an error;
			// the connection has reset.
			if (amt == 0)
				return -1;

			// error
			if (amt < 0)
			{
				// there wasnt enough room in the buffer
				if (WSAEMSGSIZE == WSAGetLastError())
					return 0;

				return amt;
			}

			read_ptr+= amt;

			if ((amt_left-= amt) > 0)
				continue;
		}

		// done reading size;
		// begin reading message body
		if (readingsize)
		{
			amt_left= final_size= ReadMsg.ReadU16();

			ReadMsg.Clear();

			if ( !ReadMsg.Resize( final_size ) )
				return 0;

			read_ptr= ReadMsg.Buffer();

			readingsize = false;
			continue;
		}
		// done reading message body;
		// return message in passed buffer
		// begin reading message headers again
		else
		{
			if (!into->Resize(final_size))
				return 0;

			memcpy( into->Buffer(into->BufferLength()), ReadMsg.Buffer(), final_size );

			// seek to the end of the buffer
			// thereby expanding it as necessary
			// for the above appended data
			int oldpos= into->WritePos();
			into->SeekWrite( into->BufferLength() + final_size );
			into->SeekWrite( oldpos );

			ResetRecv();

			return into->BufferLength();
		}
	}

	// message not ready yet
	return res;
}

//---------------------------------------------------------------------------
int SocketTCP::SendBuffer( const BufferObjectIO & outfrom, const void * to_addr )
{
	if (outfrom.BufferLength() <= 0)
		return 0;

	WriteMsg.Clear();

	if (!WriteMsg.Resize( outfrom.BufferLength() + 2 ))
		return -1;

	// add the length of the passed message into the sent message
	WriteMsg.WriteU16( outfrom.BufferLength() );
	WriteMsg.WriteData( outfrom.Buffer(), outfrom.BufferLength() );

	return Send( WriteMsg.Buffer(), WriteMsg.BufferLength(), (sockaddr_in*) to_addr );
}

//---------------------------------------------------------------------------
// SocketUDP Class
//---------------------------------------------------------------------------

SocketUDP::SocketUDP( void ) :
	Socket( SOCK_DGRAM )
{
	strncpy( metrics.desc, "SocketUDP", 32 );
}

//---------------------------------------------------------------------------
int SocketUDP::RecvBuffer( BufferObjectIO * into, void * from_addr )
{
	int res;

	if ((res= DataWaiting()) > 0)
	{
		int buffer_left= into->BufferLeft();

		// out of space
		if (buffer_left <= 0)
			return 0;

		// overriding const of buffer pointer but
		// very carefully managing data (below)
		// to simulate a correct write
		int amt = Recv( into->Buffer(into->BufferLength()), buffer_left, (sockaddr_in*) from_addr);

		// error
		if (amt < 0)
		{
			// there wasnt enough room in the buffer
			if (WSAEMSGSIZE == WSAGetLastError())
				return 0;

			return amt;
		}

		// seek to the end of the buffer
		// thereby expanding it as necessary
		// for the above appended data
		int oldpos= into->WritePos();
		into->SeekWrite( into->BufferLength() + amt );
		into->SeekWrite( oldpos );
	}

	return res;
}

//---------------------------------------------------------------------------
int SocketUDP::SendBuffer( const BufferObjectIO & outfrom, const void * to_addr )
{
	if (outfrom.BufferLength() <= 0)
		return 0;

	return Send( outfrom.Buffer(), outfrom.BufferLength(), (const sockaddr_in *) to_addr );
}

//---------------------------------------------------------------------------
// SocketBroadcast
//---------------------------------------------------------------------------

SocketBroadcast::SocketBroadcast( void ) :
	Socket( SOCK_DGRAM )
{
	strncpy( metrics.desc, "SocketBCAST", 32 );
}

//---------------------------------------------------------------------------
bool SocketBroadcast::Open( void )
{
	if (!Socket::Open())
		return false;

	// turn off blocking property
	unsigned long fionbio= 1;
	ioctlsocket( sock, FIONBIO, &fionbio );

	return true;
}

//---------------------------------------------------------------------------
int SocketBroadcast::BroadcastBuffer( const BufferObjectIO & buf, const U16 port )
{
	int optval;

	// turn on broadcasting ability
	optval= 1;
	if (setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (char*) &optval, sizeof(int)))
	{
		// SOCK_ERR
		SocketError();
		return -1;
	}

	sockaddr_in baddr;

	baddr.sin_family= AF_INET;
	baddr.sin_port  = htons(port);
	baddr.sin_addr.s_addr= htonl(INADDR_BROADCAST);

	int ret = SendBuffer( buf, &baddr );

	// turn off broadcasting
	optval=0;
	setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (char*) &optval, sizeof(int));

	return ret;
}

//---------------------------------------------------------------------------
int SocketBroadcast::SendBuffer( const BufferObjectIO & outfrom, const void * to_addr )
{
	if (outfrom.BufferLength() <= 0)
		return 0;

	return Send( outfrom.Buffer(), outfrom.BufferLength(), (const sockaddr_in *) to_addr );
}

//---------------------------------------------------------------------------
int SocketBroadcast::RecvBuffer( BufferObjectIO * into, void * from_addr )
{
	int buffer_left= into->BufferLeft();

	// out of space
	if (buffer_left <= 0)
		return 0;

	// overriding const of buffer pointer but
	// very carefully managing data (below)
	// to simulate a correct write
	int fromsize = sizeof(sockaddr_in);

	int amt = recvfrom( sock, (char *) into->Buffer(into->BufferLength()), buffer_left, NULL,
							  (struct sockaddr *) from_addr, &fromsize);

	// error
	if (amt < 0)
	{
		// there wasnt enough room in the buffer
		if (WSAEMSGSIZE == WSAGetLastError())
			return 0;

		// no message has arrived
		if (WSAEWOULDBLOCK == WSAGetLastError())
			return 0;

		return -1;
	}

	metrics.amt_recvd+= amt;

	// seek to the end of the buffer
	// thereby expanding it as necessary
	// for the above appended data
	int oldpos= into->WritePos();
	into->SeekWrite( into->BufferLength() + amt );
	into->SeekWrite( oldpos );

	return amt;
}


//---------------------------------------------------------------------------
// WinSock
// runs the winsock connection
//---------------------------------------------------------------------------

static winsock_refs = 0;

// Init; attach to winsock
// try obtaining winsock version 2.0
// on fail obtain winsock version 1.1
//
// only seen 2.0 fail when TCP/IP hasnt been setup
// in which case 1.1 fails too (tlp)
void QueryWinSock( void )
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err=0;

	if (winsock_refs == 0)
	{
		wVersionRequested = MAKEWORD( 2, 0 );

		if (! (err = WSAStartup( wVersionRequested, &wsaData )))
		{
			OutputDebugString("started winsock 2.0\n");
		}
		else
		{
			wVersionRequested = MAKEWORD( 1, 1 );

			if (! (err = WSAStartup( wVersionRequested, &wsaData )))
			{
				OutputDebugString("started winsock 1.1\n");
			}
			else
			{
				OutputDebugString("couldnt start winsock\n");
			}
		}
	}

	if (!err)
		winsock_refs++;
}

void ReleaseWinSock( void )
{
	if (winsock_refs > 0)
	{
		if ((--winsock_refs) == 0)
		{
			WSACleanup();
		}
	}
}
//---------------------------------------------------------------------------
// eror logging
//---------------------------------------------------------------------------
int SocketError( void )
{
	int err = WSAGetLastError();

	switch (err)
	{
		case 0: break;
		case WSAEINTR:		  OutputDebugString("WSAEINTR\n");	   break;
		case WSAEBADF:		  OutputDebugString("WSAEBADF\n");	   break;
		case WSAEACCES:		 OutputDebugString("WSAEACCES\n");	  break;
		case WSAEFAULT:		 OutputDebugString("WSAEFAULT\n");	  break;
		case WSAEINVAL:		 OutputDebugString("WSAEINVAL\n");	  break;
		case WSAEMFILE:		 OutputDebugString("WSAEMFILE\n");	  break;
		case WSAEWOULDBLOCK:	OutputDebugString("WSAEWOULDBLOCK\n");	 break;
		case WSAEINPROGRESS:	OutputDebugString("WSAEINPROGRESS\n");	 break;
		case WSAEALREADY:	   OutputDebugString("WSAEALREADY\n");		break;
		case WSAENOTSOCK:	   OutputDebugString("WSAENOTSOCK\n");		break;
		case WSAEDESTADDRREQ:   OutputDebugString("WSAEDESTADDRREQ\n");	break;
		case WSAEMSGSIZE:	   OutputDebugString("WSAEMSGSIZE\n");		break;
		case WSAEPROTOTYPE:	 OutputDebugString("WSAEPROTOTYPE\n");	  break;
		case WSAENOPROTOOPT:	OutputDebugString("WSAENOPROTOOPT\n");	 break;
		case WSAEPROTONOSUPPORT:OutputDebugString("WSAEPROTONOSUPPORT\n"); break;
		case WSAESOCKTNOSUPPORT:OutputDebugString("WSAESOCKTNOSUPPORT\n"); break;
		case WSAEOPNOTSUPP:	 OutputDebugString("WSAEOPNOTSUPP\n");	  break;
		case WSAEPFNOSUPPORT:   OutputDebugString("WSAEPFNOSUPPORT\n");	break;
		case WSAEAFNOSUPPORT:   OutputDebugString("WSAEAFNOSUPPORT\n");	break;
		case WSAEADDRINUSE:	 OutputDebugString("WSAEADDRINUSE\n");	  break;
		case WSAEADDRNOTAVAIL:  OutputDebugString("WSAEADDRNOTAVAIL\n");   break;
		case WSAENETDOWN:	   OutputDebugString("WSAENETDOWN\n");		break;
		case WSAENETUNREACH:	OutputDebugString("WSAENETUNREACH\n");	 break;
		case WSAENETRESET:	  OutputDebugString("WSAENETRESET\n");	   break;
		case WSAECONNABORTED:   OutputDebugString("WSAECONNABORTED\n");	break;
		case WSAECONNRESET:	 OutputDebugString("WSAECONNRESET\n");	  break;
		case WSAENOBUFS:		OutputDebugString("WSAENOBUFS\n");		 break;
		case WSAEISCONN:		OutputDebugString("WSAEISCONN\n");		 break;
		case WSAENOTCONN:	   OutputDebugString("WSAENOTCONN\n");		break;
		case WSAESHUTDOWN:	  OutputDebugString("WSAESHUTDOWN\n");	   break;
		case WSAETOOMANYREFS:   OutputDebugString("WSAETOOMANYREFS\n");	break;
		case WSAETIMEDOUT:	  OutputDebugString("WSAETIMEDOUT\n");	   break;
		case WSAECONNREFUSED:   OutputDebugString("WSAECONNREFUSED\n");	break;
		case WSAELOOP:		  OutputDebugString("WSAELOOP\n");		   break;
		case WSAENAMETOOLONG:   OutputDebugString("WSAENAMETOOLONG\n");	break;
		case WSAEHOSTDOWN:	  OutputDebugString("WSAEHOSTDOWN\n");	   break;
		case WSAEHOSTUNREACH:   OutputDebugString("WSAEHOSTUNREACH\n");	break;
		case WSAENOTEMPTY:	  OutputDebugString("WSAENOTEMPTY\n");	   break;
		case WSAEPROCLIM:	   OutputDebugString("WSAEPROCLIM\n");		break;
		case WSAEUSERS:		 OutputDebugString("WSAEUSERS\n");		  break;
		case WSAEDQUOT:		 OutputDebugString("WSAEDQUOT\n");		  break;
		case WSAESTALE:		 OutputDebugString("WSAESTALE\n");		  break;
		case WSAEREMOTE:		OutputDebugString("WSAEREMOTE\n");		 break;
		case WSASYSNOTREADY:	OutputDebugString("WSASYSNOTREADY\n");	 break;
		case WSAVERNOTSUPPORTED:OutputDebugString("WSAVERNOTSUPPORTED\n"); break;
		case WSANOTINITIALISED: OutputDebugString("WSANOTINITIALISED\n");  break;
		case WSAEDISCON:		OutputDebugString("WSAEDISCON\n");		 break;
		case WSAENOMORE:		OutputDebugString("WSAENOMORE\n");		 break;
		case WSAECANCELLED:	 OutputDebugString("WSAECANCELLED\n");	  break;
		case WSAEINVALIDPROCTABLE:	  OutputDebugString("WSAEINVALIDPROCTABLE\n");   break;
		case WSAEINVALIDPROVIDER:	   OutputDebugString("WSAEINVALIDPROVIDER\n");	break;
		case WSAEPROVIDERFAILEDINIT:	OutputDebugString("WSAEPROVIDERFAILEDINIT\n"); break;
		case WSASYSCALLFAILURE:		 OutputDebugString("WSASYSCALLFAILURE\n");	  break;
		case WSASERVICE_NOT_FOUND:	  OutputDebugString("WSASERVICE\n_NOT_FOUND");   break;
		case WSATYPE_NOT_FOUND:		 OutputDebugString("WSATYPE\n_NOT_FOUND");	  break;
		case WSA_E_NO_MORE:	 OutputDebugString("WSA\n_E_NO_MORE");	  break;
		case WSA_E_CANCELLED:   OutputDebugString("WSA\n_E_CANCELLED");	break;
		case WSAEREFUSED:	   OutputDebugString("WSAEREFUSED\n");		break;

		default:
			OutputDebugString("Unkown error"); //, %d);
		break;
	}

	return err;
}
