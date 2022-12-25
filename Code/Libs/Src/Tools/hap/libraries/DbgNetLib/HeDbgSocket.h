//////////////////////////////////////////////////////////////////////////////
//  File:    HeDbgSocket.h
//
//  Purpose: Definition of a socket to communicate between the python
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
#ifndef _HEDBGSOCKET_H_
#define _HEDBGSOCKET_H_

#include <Winsock.h>
#include "HeDbgMsg.h"

//-----------------------------------------------------------------------------
// class CHeDbgSocket
//
// This class implements a TCP/IP socket.

class CHeDbgSocket
{
public:
//-----------------------------------------------------------------------------
// Socket error codes
enum ESocketError
{
	eSOCK_NOERROR		=  0,
	eSOCK_THREADERROR	= -1,
	eSOCK_BINDFAIL		= -2,
	eSOCK_LISTENFAIL	= -3,
	eSOCK_CREATEFAIL	= -4,
	eSOCK_INVALIDSTATE	= -5,
};
	

//-----------------------------------------------------------------------------
// Socket default port number
#define HE_DEBUGGER_PORTNUM	2020

//-----------------------------------------------------------------------------
// Socket state enumerated type
enum ESocketState
{
	eSOCKSTATE_NONE,		//socket is offline
	eSOCKSTATE_LISTENING,	//socket is listening for an incoming connection
	eSOCKSTATE_CONNECTING,	//socket is attempting to connect	
	eSOCKSTATE_CONNECTED,	//socket has an active connection

	eSOCKSTATE_NUMSTATES,
};

//-----------------------------------------------------------------------------
// typedef for the socket callback function
typedef void (__cdecl *TSocketCallbackFn)();


public:
	CHeDbgSocket();
	~CHeDbgSocket();

	static bool InitSockets();
	
public:
	//close the connection
	void Close();
	
	//get the current state of the socket 
	ESocketState eGetState()const {return m_eState;}

	//set the callback function called whenever data is received
	void SetEventCallback(TSocketCallbackFn fnCallback);
	
	//Start listening for an incoming connection - return ESocketError code
	int Listen(uint16_t nPort = HE_DEBUGGER_PORTNUM);

	//connect to the specified address - return ESocketError code
	int Connect(unsigned long ConnectAddress, uint16_t nPort = HE_DEBUGGER_PORTNUM);

	//send a null terminated string message	
	bool SendMessage(EMessageType MsgType, const char* szMsg);

	//send a message of a given size
	bool SendMessageSize(EMessageType MsgType, const char* szMsg, int Size);
 	
	//read the header of a message - return the message type read
	EMessageType ReadMessageHdr(SMsgHeader* pHdr);

	//read the message data - return the number of bytes received
	int ReadMessageBody(char* szBuffer, const SMsgHeader* pHdr);

	//get the address of the remote computer
	unsigned long GetAddress()const	{return *((unsigned long*)&m_RemoteAddress);}

private:
	//private copy constructor and assignment operator - unimplimented to prevent use
	CHeDbgSocket(const CHeDbgSocket& SockCopy);
	CHeDbgSocket& operator=(const CHeDbgSocket& SockEqu);

private:

	static bool CreateSocket(SOCKET& socketRef, bool fBlocking, const char* pszSocketType);

	void ConnectComplete(bool bConnected);
	void ListenComplete();
	void ReceiveData();
	
	EMessageType GetMessageType(const char* szType);

	bool WaitForWrite();
	bool WaitForRead();

private:
	//static functions for each of the worker threads
	static void SockListenThreadFn(void* pParam);
	static void SockConnectThreadFn(void* pParam);
	static void SockSelectThreadFn(void* pParam);

private:

	//the current state of the object
	ESocketState m_eState;

	//a handle for listening and one for the acutal connection
	SOCKET m_ListenSocket;
	SOCKET m_Socket;

	//address of the remote computer
	sockaddr_in m_RemoteAddress;

	//crit section used to protect the m_Socket between the select 
	//and main threads
	//CRITICAL_SECTION m_SockCS;
	CCodeSingleUseGate m_SockCS;

	//flag indicating that data is waiting
	bool m_bDataWaitingInSocket;
	
	//parameters related to the listen thread
	/*unsigned long m_hListenThread;
	bool m_bCancelListen;

	//parameters related to the connect thread
	unsigned long m_hConnectThread;
	bool m_bCancelConnect;

	//the select thread
	unsigned long m_hSocketSelectThread;
	bool m_bCancelSelect;*/

	//handle to listen, connect or select thread - only one can be active at a time
	unsigned long m_hSocketThread;
	bool m_bCancelThread;

	//callback function pointer
	TSocketCallbackFn m_fnEventCallback;

	static bool m_bSocketsInitialized;

}; // CHeDbgSocket




#endif //_HEDBGSOCKET_H_