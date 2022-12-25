//////////////////////////////////////////////////////////////////////////////
//  File:    HeDbgSocket.cpp
//
//  Purpose: Implementation of a socket to communicate between the python
//  debugger and the debug engine
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001 by Humongous Entertainment
//  
//  This library is free software; you can redistribute it and/or modify it 
//  under the terms of the GNU Lesser General Public License as published by 
//  the Free Software Foundation; either version 2.1 of the License, or  any 
//  later version.
//  
//  This library is distributed in the hope that it will be useful, but 
//  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
//  License for more details.
//  
//  You should have received a copy of the GNU Lesser General Public License 
//  along with this library (see the file license.txt); if not, write to the 
//  Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//////////////////////////////////////////////////////////////////////////////	

#include "stdafx.h"
#include "HeDbgSocket.h"
#include "HeDbgMsg.h"

#include <process.h>

#include <fstream>
using std::ofstream;
using std::ostream;

//note: the next include and the pragma should be #ifdef'd for cross platform
//support.  Ideally we can wrap the critical section stuff and add it to hecore.

//for critical section stuff
#include <windows.h>
//and the associated library
#pragma comment(lib, "Ws2_32.lib")



//delay between attempts to accept an incoming connection
const int ACCEPT_RETRY_DELAY_MS		=   50;
//timeout on the select used in accepting an incoming connection
const int ACCEPT_SELECTTIMEOUT_USEC	= 5000;	//wait for 5 ms before timing out

//delay between connect retries
const int CONNECT_RETRY_DELAY		=  250;

//delay between subsequent select retries
const int SELECT_RETRY_DELAY		=    1;		//0 indicates just give up the rest of the timeslice
const int SELECT_TIMEOUT_USEC		=  500;		//wait for 1/2 a ms before giving up on select

//maximum number of times to retry sending data
const int MAX_SEND_RETRYS			=   20;
// Time to wait before resending. The maximum total time should be at least
// one second - greater than the timeouts?
const int SEND_RETRY_DELAY			=  150;

//number of times to retry receiving data
const int MAX_RECV_RETRYS			=   50;
//time to wait before trying to read the data again (failed)
const int RECV_RETRY_DELAY			=   50;
//time to wait before trying to read the rest of the data (partial read)
const int RECV_READMORE_DELAY		=   10;

//amount of time to wait after signalling a thread to quit before killing it.
const int CANCEL_DELAY				= 1000;

const int INVALID_THREAD			=   -1;

// RecordDataSent and RecordDataReceived assume that the caller has used synchronization
// methods to make sure that they are only called by one thread at a time.

// Set this to true to log socket traffic to files.
static bool g_logSocketTraffic = false;

static ostream& GetRecordFileStream()
{
	static bool nameInitialized = false;
	char	fileName[1000];
	if (!nameInitialized)
	{
		nameInitialized = true;
		(void)GetModuleFileName(0, fileName, sizeof(fileName));
		char* pExtension = strrchr(fileName, '.');
		assert(pExtension);
		strcpy(pExtension, ".slg");	// Socket log
	}
	static ofstream object(fileName);
	return object;
}

static void RecordData(const char* type, const void* data, size_t size)
{
	if (!g_logSocketTraffic)
		return;

	const char* text = (const char*)data;
	ostream& output = GetRecordFileStream();
	output << "\t" << type << size << " bytes.\n";
	while (size != 0)
	{
		size_t length = strlen(text);
		assert(length < size);	// Make sure strings are null terminated.
		// Skip over empty strings/zero padding, but write other strings.
		if (length > 0)
		{
			output.write(text, length);
			output << "\n";
		}
		text += length + 1;
		size -= length + 1;
	}
	output.flush();
}

static void RecordDataf(const char* format, ...)
{
	va_list ap;

	va_start(ap,format);
	rdprintv(format, ap);
	va_end(ap);

	if (!g_logSocketTraffic)
		return;

	char tempstr[8000] = "";
	// I subtract one from the buffer length to ensure proper behaviour on
	// the boundary case.
	if (_vsnprintf(tempstr, sizeof(tempstr) - 1, format,ap) < 0) 
	{
		// would have exceeded max
		tempstr[ sizeof(tempstr) - 2 ] = '\n'; // force a new-line
	}
	// BUG! _vsnprintf() does not always null terminate the output. If the number
	// of characters printed, not including the terminating null, exactly fits in
	// the buffer, it returns the 'success' code (positive) even though a badly
	// formed string is in the buffer. Yucch.  Bruce, Feb 1998.
	tempstr[ sizeof(tempstr) - 1 ] = '\0'; // force a terminating null

	ostream& output = GetRecordFileStream();
	output.write(tempstr, strlen(tempstr));
}

void RecordDataSent(const void* data, size_t size)
{
	RecordData("Writing ", data, size);
}

void RecordDataReceived(const void* data, size_t size)
{
	RecordData("Reading ", data, size);
}

bool CHeDbgSocket::m_bSocketsInitialized = false;

CHeDbgSocket::CHeDbgSocket()
{
	m_hSocketThread = INVALID_THREAD;
	m_bCancelThread = true;

	m_bDataWaitingInSocket = false;

	m_ListenSocket = INVALID_SOCKET;
	m_Socket = INVALID_SOCKET;

	m_eState = eSOCKSTATE_NONE;
	m_RemoteAddress.sin_addr.S_un.S_addr = 0;
	m_RemoteAddress.sin_family = 0;
	m_RemoteAddress.sin_port = 0;

	m_fnEventCallback = NULL;

	//InitializeCriticalSection(&m_SockCS);
}

CHeDbgSocket::~CHeDbgSocket()
{
	Close();
	//DeleteCriticalSection(&m_SockCS);
}

bool CHeDbgSocket::InitSockets()
{
	// Initialise windows sockets
    WSAData wd;
    WORD wVersionRequested = MAKEWORD(1, 1);
    int WSAErr = WSAStartup(wVersionRequested, &wd);
    m_bSocketsInitialized = (0 == WSAErr);
    
	return m_bSocketsInitialized;
}

void CHeDbgSocket::Close()
{
	m_bCancelThread = true;
	if (m_hSocketThread != INVALID_THREAD)
	{		
		//the thread is appears to be running - give it a chance to stop
		if (WaitForSingleObject((HANDLE)m_hSocketThread, CANCEL_DELAY) == WAIT_TIMEOUT)
		{
			//something very very bad has happened, the thread appears to be
			//blocked somehow - anyway, just kill it and continue
			dprintf("Terminating thread\n");
			TerminateThread((HANDLE)m_hSocketThread, -1);
		}
		m_hSocketThread = INVALID_THREAD;
	}

	//close the sockets
	if (m_ListenSocket != INVALID_SOCKET)
		closesocket(m_ListenSocket);
	m_ListenSocket = INVALID_SOCKET;
	
	if (m_Socket != INVALID_SOCKET)
		closesocket(m_Socket);
	m_Socket = INVALID_SOCKET;

	//reset the state to none
	m_eState = eSOCKSTATE_NONE;

} // Close


void CHeDbgSocket::SetEventCallback(TSocketCallbackFn fnCallback)
{
	m_fnEventCallback = fnCallback;
}

bool CHeDbgSocket::CreateSocket(SOCKET& socketRef, bool fBlocking, const char* pszSocketType)
{
	assert(pszSocketType);

	if (INVALID_SOCKET != socketRef)
	{
		dprintf("Attempted to create an already-open %s\n", pszSocketType);
		return false;
	}

	// create the socket

	if (INVALID_SOCKET == (socketRef = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)))
	{
		dprintf("Failed to create %s - %d\n", pszSocketType, WSAGetLastError());
		return false;
	}

	// set the socket to non blocking
	
	u_long nonBlockingFlag = !fBlocking;

	if (SOCKET_ERROR == ioctlsocket(socketRef, FIONBIO, &nonBlockingFlag))
	{
		closesocket(socketRef);
		socketRef = INVALID_SOCKET;
		dprintf("Failed to set %s mode to non-blocking - %d\n", pszSocketType, WSAGetLastError());
		return false;
	}

	return true;
}

int CHeDbgSocket::Listen(uint16_t nPort)
{	
	int nRetVal = eSOCK_NOERROR;

	//if we don't have a listen socket, create one
	if (m_ListenSocket == INVALID_SOCKET)
	{
		if (!CreateSocket(m_ListenSocket, false, "listen socket"))
		{
			return eSOCK_CREATEFAIL;
		}
	}

	//check to see if we are already listening
	if (m_hSocketThread != INVALID_THREAD)
	{
		assert(m_eState == eSOCKSTATE_LISTENING);
		return eSOCK_NOERROR;
	}

	//the address we are binding to - specific port, any address
	m_RemoteAddress.sin_family = AF_INET;
    m_RemoteAddress.sin_port = htons(nPort);;
    m_RemoteAddress.sin_addr.S_un.S_addr = INADDR_ANY;
	
	//bind the listen socket to the port, start listening and accept (in a thread)
	if (bind (m_ListenSocket, (sockaddr*)&m_RemoteAddress, sizeof(m_RemoteAddress)) == 0)
	{	
		//set the socket to listen for incoming connection
		if (listen (m_ListenSocket, SOMAXCONN) == 0)
		{
			//start a thread to accept any incoming connections
			m_bCancelThread = false;
			m_hSocketThread = _beginthread(CHeDbgSocket::SockListenThreadFn, 0, (void*)this);
			if (m_hSocketThread != INVALID_THREAD)
			{	
				m_eState = eSOCKSTATE_LISTENING;
				return 0;
			}
			else
			{
				nRetVal = eSOCK_THREADERROR;
			}
		}
		else
			nRetVal = eSOCK_LISTENFAIL;
	}
	else
	{
		dprintf("Unable to bind socket %d\n", WSAGetLastError());
		nRetVal = eSOCK_BINDFAIL;
	}

	m_eState = eSOCKSTATE_NONE;
	return nRetVal;

} // Listen

int CHeDbgSocket::Connect(unsigned long ConnectAddress, uint16_t nPort)
{
	//if the socket is connected or connection - just return
	if ((m_eState == eSOCKSTATE_CONNECTED) || (m_eState == eSOCKSTATE_CONNECTING))
	{
		dprintf("CHeDbgSocket::Connect - already connected(ing)\n");
		return eSOCK_NOERROR;
	}

	//if we are listening then this is a server socket and so we shouldn't be connecting
	if (m_eState == eSOCKSTATE_LISTENING)
		return eSOCK_INVALIDSTATE;

	if (m_hSocketThread != INVALID_THREAD)
	{
		assert(m_eState == eSOCKSTATE_CONNECTING);
		return eSOCK_NOERROR;
	}

	//create the socket

	assert(m_Socket == INVALID_SOCKET);

	if (!CreateSocket(m_Socket, true, "connect socket"))
	{
		return eSOCK_CREATEFAIL;
	}

	//setup the address to connect to
	m_RemoteAddress.sin_family = AF_INET;
    m_RemoteAddress.sin_port = htons(nPort);
	m_RemoteAddress.sin_addr.S_un.S_addr = ConnectAddress;

	//start a thread to continue to retry connecting to the remote machine
	m_bCancelThread = false;
	m_hSocketThread = _beginthread(CHeDbgSocket::SockConnectThreadFn, 0, (void*)this);
	if (m_hSocketThread != INVALID_THREAD)
	{	
		m_eState = eSOCKSTATE_CONNECTING;
		return eSOCK_NOERROR;
	}
	
	m_eState = eSOCKSTATE_NONE;
	m_hSocketThread = INVALID_THREAD;
	
	return eSOCK_THREADERROR;

} // Connect




bool CHeDbgSocket::SendMessage(EMessageType MsgType, const char* szMsg)
{
	assert(szMsg);
	//if there is a string, send the string as well as the null terminator
	if (strlen(szMsg) > 0)
		return SendMessageSize(MsgType, szMsg, strlen(szMsg)+1);

	//no message data.
	return SendMessageSize(MsgType, szMsg, 0);
}


bool CHeDbgSocket::SendMessageSize(EMessageType MsgType, const char* szMsg, int Size)
{
	//dprintf("Sending Message %d bytes\n", Size);

	//if the size isn't 0, then there better be a message
	assert((Size == 0) || (szMsg));
	
	if (m_eState != eSOCKSTATE_CONNECTED)
		return FALSE;
	
	//set up the message header
	SMsgHeader SendHdr;
	
	//Message identifier
	const char* MsgString = GetMsgString(MsgType);
	strncpy(SendHdr.MsgType, MsgString, strlen(MsgString));
	
	//message body size
	SendHdr.SetMsgSize(Size);
	
	//block until the select thread releases the socket.
	//EnterCriticalSection(&m_SockCS);
	CCodeSingleUseEntry EnterCode(m_SockCS);

	bool bRetVal = false;
	int nRetryCount = 0;
	
	RecordDataSent(&SendHdr, sizeof(SendHdr));

	//send the message header
	if (!WaitForWrite())
		dprintf("WaitForWrite failed\n");
	int nSendSize = send (m_Socket, (const char*) &SendHdr, sizeof(SendHdr),0);
	
	while ((nSendSize == SOCKET_ERROR) && (WSAGetLastError() == WSAEWOULDBLOCK) && 
		   (++nRetryCount < MAX_SEND_RETRYS))
	{
		//we should never send a partial message header
		assert(nSendSize <= 0);

		//try try again
		if (!WaitForWrite())
			dprintf("WaitForWrite failed\n");

		nSendSize = send (m_Socket, (const char*) &SendHdr, sizeof(SendHdr),0);
		// It is important to wait before trying again, or else we will just fail again.
		if (nSendSize == SOCKET_ERROR)
			Sleep(SEND_RETRY_DELAY);
	}
	
	if (nSendSize == sizeof(SendHdr))
	{
		if (Size > 0)
		{
			RecordDataSent(szMsg, Size);

			nRetryCount = 0;

			//send the message body
			if (!WaitForWrite())
				dprintf("WaitForWrite failed\n");
			
			nSendSize = send(m_Socket, szMsg, Size, 0);
			while ((nSendSize == SOCKET_ERROR) && (WSAGetLastError() == WSAEWOULDBLOCK) && 
				   (++nRetryCount < MAX_SEND_RETRYS))
			{
				assert(nSendSize <= 0);
				
				//retry 
				nSendSize = send(m_Socket, szMsg, Size, 0);

				// It is important to wait long enough before trying again, or else we will just
				// fail again.
				Sleep(SEND_RETRY_DELAY);
			}

			
			if (nSendSize == Size)
				bRetVal = true;
			else
			{
				dprintf("Unable to write data to socket %d bytes written out of %d: %d\n", nSendSize, Size, WSAGetLastError());
				//assert(0);
			}
		}
		else
			bRetVal = true;
	}
	else
	{
		dprintf("Unable to write header to the socket: %d bytes written out of %d sent - %d\n", 
				nSendSize, sizeof(SendHdr), WSAGetLastError());
			
		int nErr = WSAGetLastError();
		if ((nErr == WSAECONNRESET) || (nErr == WSAECONNABORTED))
		{
			dprintf("Socket connection was reset - closing\n");
			//the socket was closed from the other side
			Close();
		}
		else
		{
			assert(0);
		}
	}

	//LeaveCriticalSection(&m_SockCS);
			
	return bRetVal;

} // SendMessageSize
	
EMessageType CHeDbgSocket::ReadMessageHdr(SMsgHeader* pHdr)
{
	EMessageType eRetType = eMSG_INVALID;	
	if (m_bDataWaitingInSocket)
	{
		memset(pHdr, 0, sizeof(SMsgHeader));

		CCodeSingleUseEntry EnterCode(m_SockCS);
		
		//dprintf("Reading header\n");

		int nRetryCount = 0;
		int nBytesReceived = recv(m_Socket, (char*)pHdr, sizeof(SMsgHeader), 0);
		while ( (nBytesReceived == SOCKET_ERROR) && (WSAGetLastError() == WSAEWOULDBLOCK) && 
			    (++nRetryCount < MAX_RECV_RETRYS) )
		{
			nBytesReceived = recv(m_Socket, (char*)pHdr, sizeof(SMsgHeader), 0);
		}

		if (nBytesReceived <= 0)
		{
			int nErr = WSAGetLastError();
			if ((nErr == WSAECONNRESET) || (nErr == WSAECONNABORTED))
			{
				//Socket was closed
				dprintf("Connection Reset - closing socket\n");
				Close();
			}
			m_bDataWaitingInSocket = false;
		}
			
		if (nBytesReceived == sizeof(SMsgHeader))
		{
			eRetType = GetMessageType(pHdr->MsgType);
		}
		if (nBytesReceived >= 0)
			RecordDataReceived(pHdr, nBytesReceived);

	}
	return eRetType;

} //ReadMessageHdr

int CHeDbgSocket::ReadMessageBody(char* szBuffer, const SMsgHeader* pHdr)
{
	const int nBytesToRead = pHdr->GetMsgSize();	//strtol(pHdr->MsgSize, NULL, 10);
	// Make a copy of the buffer pointer that won't change.
	int totalBytesRead = 0;
	if (m_bDataWaitingInSocket && (nBytesToRead > 0))
	{
		CCodeSingleUseEntry EnterCode(m_SockCS);
		
		int nRetryCount = 0;

		while (true)
		{
			//dprintf("Reading %d bytes\n", nBytesToRead);

			//recv may succeed but only read a portion of the message data - such is 
			//the price for non blocking sockets.
			if (!WaitForRead())
				dprintf("WaitForRead failed\n");

			int nBytesReceived = recv(m_Socket, szBuffer + totalBytesRead, nBytesToRead - totalBytesRead, 0);
			if (nBytesReceived != SOCKET_ERROR)
				totalBytesRead += nBytesReceived;

			if (nBytesReceived == 0)
				m_bDataWaitingInSocket = false;

			//if we read less than we wanted, or if there was an error that indicates the socket would block, retry
			if ( (totalBytesRead < nBytesToRead) || 
					((nBytesReceived == SOCKET_ERROR) && 
					 (WSAGetLastError() == WSAEWOULDBLOCK) && 
					 (++nRetryCount < MAX_RECV_RETRYS)) )
			{
				//if there is more to read, update the buffer pointers
				if (nBytesReceived != SOCKET_ERROR)
				{
					Sleep(RECV_READMORE_DELAY);
				}
				else
					Sleep(RECV_RETRY_DELAY);
			}
			else
				break;
		}
		
		RecordDataReceived(szBuffer, totalBytesRead);

		if (totalBytesRead != nBytesToRead)
		{
			dprintf("Socket Read Error Read %d, Expected %d - %d\n", totalBytesRead, 
			        nBytesToRead, WSAGetLastError());
			assert(0);
		}
	}
	return totalBytesRead;

} // ReadMessageBody


void CHeDbgSocket::ConnectComplete(bool bConnected)
{
	if (bConnected)
	{
		//the socket is connected, start a thread to watch for data to arrive
		m_bCancelThread = false;
		m_hSocketThread = _beginthread(CHeDbgSocket::SockSelectThreadFn, 0, (void*)this);
		if (m_hSocketThread != INVALID_THREAD)
		{
			dprintf("Socket connected\n");
			m_eState = eSOCKSTATE_CONNECTED;
			//call the socket event callback (if any) to signal that a change in state has occured
			if (m_fnEventCallback)
				m_fnEventCallback();

			return;
		}		
	}
	
	m_hSocketThread = INVALID_THREAD;

	dprintf("Socket connect failed\n");
	m_eState = eSOCKSTATE_NONE;
	closesocket(m_Socket);
	m_Socket = INVALID_SOCKET;
	if (m_fnEventCallback)
		m_fnEventCallback();

} // ConnectComplete

void CHeDbgSocket::ListenComplete()
{
	//The listen thread is complete
	
	if (m_Socket != INVALID_SOCKET)
	{
		m_eState = eSOCKSTATE_CONNECTED;

		//notify the main thread that state has changed
		if (m_fnEventCallback)
			m_fnEventCallback();

		//start the select thread
		m_bCancelThread = false;
		m_hSocketThread = _beginthread(CHeDbgSocket::SockSelectThreadFn, 0, (void*)this);
		if (m_hSocketThread != INVALID_THREAD)
			return;
		else
			dprintf("Unable to create select thread\n");
	
	}
	else
		dprintf("Socket accept failed\n");
	
	
	m_hSocketThread = INVALID_THREAD;
	m_eState = eSOCKSTATE_NONE;


} // ListenComplete

void CHeDbgSocket::ReceiveData()
{
	//the select thread indicates that data is waiting.
	m_bDataWaitingInSocket = true;		//set the data waiting flag
	
	if (m_fnEventCallback)		//signal the main thread
		m_fnEventCallback();
}


EMessageType CHeDbgSocket::GetMessageType(const char* szType)
{
	//get the message type from a message type string
	uint32_t u32Type = *((uint32_t*)szType);
	
	for (int type = (int)eMSG_INIT; type < (int)eMSG_NUMMSGS; type++)
	{
		if (u32Type == GetMsgVal((EMessageType)type) )
			return (EMessageType)type;
	}

	dprintf("Invalid Message Type String: %c%c%c%c\n", szType[0], szType[1], szType[2], szType[3]);
	return eMSG_INVALID;
}


void CHeDbgSocket::SockListenThreadFn(void* pParam)
{
	assert(pParam);
	CHeDbgSocket* pSocket = (CHeDbgSocket*) pParam;

	SOCKET NewSocket = INVALID_SOCKET;
	
	//provide an address to store the remote address
	sockaddr_in ConnectAddr;
	int AddrLen = sizeof(ConnectAddr);

	//timeout value for select
	timeval SelectTimeout;
	SelectTimeout.tv_sec = 0;
	SelectTimeout.tv_usec = ACCEPT_SELECTTIMEOUT_USEC; 

	//socket arrays for select
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;

	while(pSocket->m_bCancelThread == false)
	{
		FD_ZERO(&readfds);
		FD_SET(pSocket->m_ListenSocket, &readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exceptfds);
		
		if (select(0, &readfds, &writefds, &exceptfds, &SelectTimeout) > 0)
		{
			pSocket->m_Socket = accept(pSocket->m_ListenSocket, (sockaddr*)&ConnectAddr, &AddrLen);
		
			if (pSocket->m_Socket != INVALID_SOCKET)
			{
				//set the new socket to non blocking
				unsigned long SockArg = 1;
				ioctlsocket(pSocket->m_Socket, FIONBIO, &SockArg);
									
				assert(AddrLen <= sizeof(pSocket->m_RemoteAddress));

				//store the remote address
				memcpy(&(pSocket->m_RemoteAddress), &ConnectAddr, AddrLen);
				
				//break out of the loop - we're done listening.
				break;
			}
			else
			{
				//this is an unlikely case - select indicated that the listen socket had an incoming
				//connection request but accept failed.  Keep trying anyway.
				dprintf("Accept failed after select passed: %d\n", WSAGetLastError());
				Sleep(ACCEPT_RETRY_DELAY_MS);
			}
		}
		else
		{
			//this is the usual case, there is no connection to accept.
			Sleep(ACCEPT_RETRY_DELAY_MS);
		}
	}

	//notify the main thread that a connection has been made.
	pSocket->ListenComplete();

} // SockListenThreadFn


void CHeDbgSocket::SockConnectThreadFn(void* pParam)
{
	assert(pParam);
	CHeDbgSocket* pSocket = (CHeDbgSocket*) pParam;
	
	while (pSocket->m_bCancelThread == false)
	{
		if (connect (pSocket->m_Socket,(sockaddr*)&(pSocket->m_RemoteAddress), sizeof(pSocket->m_RemoteAddress)) != SOCKET_ERROR)
		{
			//set the socket to non blocking
			unsigned long SockArg = 1;
			ioctlsocket(pSocket->m_Socket, FIONBIO, &SockArg);

			//notify the main thread that connection is complete
			pSocket->ConnectComplete(true);
			return;
		}
		else
			dprintf("connect failed: %d\n", WSAGetLastError());
		
		Sleep(CONNECT_RETRY_DELAY);
	}
	
	//notify the main thread that connection failed
	pSocket->ConnectComplete(false);
			
} // SockConnectThreadFn

void CHeDbgSocket::SockSelectThreadFn(void* pParam)
{
	assert(pParam);
	CHeDbgSocket* pSocket = (CHeDbgSocket*) pParam;
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
	
	timeval SelectTimeout;
	SelectTimeout.tv_sec = 0;
	SelectTimeout.tv_usec = SELECT_TIMEOUT_USEC; 

	int iSelectRet;

	//loop until the main thread signals the thread to quit
	while (pSocket->m_bCancelThread == false)
	{
		//if we've already signalled, just skip it
		if (pSocket->m_bDataWaitingInSocket == false)
		{
			//signal read and exceptions - writing is done seperately
			FD_ZERO(&readfds);
			FD_SET(pSocket->m_Socket, &readfds);

			FD_ZERO(&writefds);

			FD_ZERO(&exceptfds);
			//FD_SET(pSocket->m_Socket, &exceptfds);

			
			//EnterCriticalSection(&(pSocket->m_SockCS));
			{
				CCodeSingleUseEntry EnterCode(pSocket->m_SockCS);
							
				iSelectRet = select(0, &readfds, &writefds, &exceptfds, &SelectTimeout);
				
				//LeaveCriticalSection(&(pSocket->m_SockCS));
			}
						
			if (iSelectRet > 0)
				pSocket->ReceiveData();

			if (iSelectRet < 0)
			{
				dprintf("select returned an error: %d\n", WSAGetLastError());
				pSocket->Close();
			}
		}

		Sleep(SELECT_RETRY_DELAY);
	}
} // SockSelectThreadFn









bool CHeDbgSocket::WaitForWrite()
{
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;

	//signal read and exceptions - writing is done seperately
	FD_ZERO(&readfds);
	
	FD_ZERO(&writefds);
	FD_SET(m_Socket, &writefds);

	FD_ZERO(&exceptfds);

	//block until the socket is ready to accept more data
	int iSelectRet = select(0, &readfds, &writefds, &exceptfds, 0);
	/*if (iSelectRet == 0)
	{
		Sleep(50);
		FD_ZERO(&readfds);
	
		FD_ZERO(&writefds);
		FD_SET(m_Socket, &writefds);

		FD_ZERO(&exceptfds);
		iSelectRet = select(0, &readfds, &writefds, &exceptfds, 0);
	}*/


	return (iSelectRet > 0);
}

bool CHeDbgSocket::WaitForRead()
{
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
	
	timeval SelectTimeout;
	SelectTimeout.tv_sec = 0;
	SelectTimeout.tv_usec = SELECT_TIMEOUT_USEC;

	//signal read and exceptions - writing is done seperately
	FD_ZERO(&readfds);
	FD_SET(m_Socket, &readfds);

	FD_ZERO(&writefds);
	
	FD_ZERO(&exceptfds);
	//FD_SET(m_Socket, &exceptfds);

	int iSelectRet = select(0, &readfds, &writefds, &exceptfds, &SelectTimeout);

	return (iSelectRet > 0);
}

