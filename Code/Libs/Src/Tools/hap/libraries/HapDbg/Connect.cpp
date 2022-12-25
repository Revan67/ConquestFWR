//////////////////////////////////////////////////////////////////////////////
//  File:    Connect.cpp
//
//  Purpose: Implements the connect state which the program enters after the
//  program is run, until the debugger connects.
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
#include "DbgGlobals.h"

extern CHeDbgSocket g_DebugSocket;



int WaitForConnection(int nPort)
{
	int nRetVal = g_DebugSocket.Listen(nPort);
	if (nRetVal != 0)
		return -1;
	
	//wait for connection	
	while (g_DebugSocket.eGetState() == CHeDbgSocket::eSOCKSTATE_LISTENING)
	{
		dprintf(".");
		Sleep(1000);
	}

	if (g_DebugSocket.eGetState() == CHeDbgSocket::eSOCKSTATE_CONNECTED)
	{
		in_addr sAddr;
		sAddr.S_un.S_addr = g_DebugSocket.GetAddress();
		printf("Connected to: %s, port: %d\n", inet_ntoa(sAddr), nPort);
		return 0;
	}
	else
	{
		dprintf("Unable to connect\n");
		return -1;
	}
}
