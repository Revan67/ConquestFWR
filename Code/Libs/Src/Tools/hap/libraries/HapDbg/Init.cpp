//////////////////////////////////////////////////////////////////////////////
//  File:    Init.cpp
//
//  Purpose: Implements the init state of the debugger
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
#include "Init.h"
#include "hapdbg.h"
#include "DbgGlobals.h"

//initial size for the socket message buffer
#define MSGBUFF_DEFSIZE		512

//time to sleep between checking the socket for init messages
#define WAITINIT_SLEEPMS	100

//number of failures before resending the init message
//RETRY_COUNT times SLEEPMS is the approximate time between resends
#define INIT_RETRY_COUNT	  5


int SendInit()
{
	char buff[64] = {0};
	_snprintf(buff, 63, "HapDbg: %s", GetDllLinkDate());
	g_DebugSocket.SendMessage(eMSG_INIT, buff);
	return 0;
}

int WaitForInit(string& strCommandLine, string& strWorkingDir, string& strCommandParams)
{
	SMsgHeader MsgHdr;
	TCharVector MessageBuffer;
	MessageBuffer.resize(MSGBUFF_DEFSIZE);

	strCommandLine = "";

	printf("Waiting for INIT - ");
	
	bool bContinueWait = true;
	int nInvalid = 0;


	while (bContinueWait == true)
	{
		//read the header
		EMessageType eType = g_DebugSocket.ReadMessageHdr(&MsgHdr);
	
		//check to see that the socket is still connected
		if (g_DebugSocket.eGetState() != CHeDbgSocket::eSOCKSTATE_CONNECTED)
		{
			dprintf("Socket connection closed - quitting\n");
			return -1;
		}
		
		if (eType > eMSG_INVALID)
		{
			//read the message body (if any)
			int nMsgBytes = MsgHdr.GetMsgSize();	//strtol(MsgHdr.MsgSize, NULL, 10);
			int nMsgSize = 0;
			MessageBuffer.resize(nMsgBytes);
			if (nMsgBytes > 0)
			{
				memset(&(MessageBuffer[0]), 0, MessageBuffer.size());
				nMsgSize = g_DebugSocket.ReadMessageBody(&(MessageBuffer[0]), &MsgHdr);	
			}

			//ignore messages other than INIT and EXIT
			switch (eType)
			{
			case eMSG_INIT:
				//init received, store the command line, parameters and path
				if (MessageBuffer.size() > 0)
				{
					const char* pCurStr = &(MessageBuffer[0]);
					strCommandLine = pCurStr;
					
					pCurStr += strlen(pCurStr) + 1;
					strWorkingDir = pCurStr;
					
					pCurStr += strlen(pCurStr) + 1;
					strCommandParams = pCurStr;

					printf("INIT received\nExecuting: %s\n", &(MessageBuffer[0]));					
				}
				else
					printf("INIT received without command line - exiting\n");
				
				bContinueWait = false;
				break;
			case eMSG_SETOPTION:
				SetOption(&(MessageBuffer[0]));
				break;
				
			case eMSG_EXIT:
				printf("EXIT received while waiting for INIT - exiting\n");
				bContinueWait = false;			
				break;

			case eMSG_PING:
				g_DebugSocket.SendMessage(eMSG_PONG, "");
				break;

			default:
				dprintf("Invalid message for Init state: %d\n", eType);
			}

		}
		else
		{
			//no message received, sleep for a bit.
			Sleep(WAITINIT_SLEEPMS);

			//if we wait for a while and still haven't gotten a response, resend the
			//init - if the socket is closed, this will reset the socket's state, otherwise
			//it will give the debug client another chance to respond
			if (++nInvalid > INIT_RETRY_COUNT)
			{
				//resend init
				SendInit();
			}
		}
	}

	if (strCommandLine.length() > 0)
		return 0;
	
	return -1;

} // WaitForInit