//////////////////////////////////////////////////////////////////////////////
//  File:    HeDbgMsg.h
//
//  Purpose: Definition of a messages to facilitate communication between the
//  Python debugger and the debug engine
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
#ifndef _HEDBGMSG_H_
#define _HEDBGMSG_H_

//-----------------------------------------------------------------------------
// Message header
//
// Each message sent between our two apps is preceeded by a message header 
// containing the message type and the size of the message data.
// The MsgType and MsgSize fields are sent as ascii text to eliminate the 
// problems associated with communcating between big and little endian processors
//
#pragma pack(push, 1)
struct SMsgHeader
{
	size_t GetMsgSize() const;
	void SetMsgSize(size_t size);

	//size these fields to hold a four byte string and a terminator
	char MsgType[sizeof(uint32_t)+1];
private:
	// Let's start by protecting this field from bad access.
	// Four data bytes let us have message to a maximum of 9999 bytes. That is
	// not sufficient. Let's increase it!
	char MsgSize[8+1];
public:
	char  Spare1[sizeof(uint32_t)+1];
	char  Spare2[sizeof(uint32_t)+1];

	SMsgHeader()
	{
		memset(this, 0, sizeof(SMsgHeader));
	}
};
#pragma pack(pop)


//-----------------------------------------------------------------------------
// Message Type
//
// This enumeration represents the different message types sent and recevied by
// the system.  These messages and their data packets are documented below.

enum EMessageType
{
	//the first batch of messages can be sent by server and client
	eMSG_INVALID = -1,
	eMSG_INIT,				
	eMSG_EVALUATE,
	eMSG_EVALNOERR,
	eMSG_SETOPTION,
	eMSG_PING,				
	eMSG_PONG,				
	
	//the next set are sent only by the server
	eMSG_BREAK,				
	eMSG_BREAKPTLIST,		
	eMSG_TRACE,
	eMSG_OUTPUT,			
	eMSG_DONE,				
	eMSG_EXCEPTION,			
	eMSG_GLOBALS,			
	eMSG_LOCALS,			
	eMSG_MODULES,			
	
	//the last are sent only by the client
	eMSG_EXECUTE,
	eMSG_SETBREAKPOINT,		
	eMSG_BREAKNOW,			
	eMSG_STEP,				
	eMSG_RELOAD,			
	eMSG_STOP,				
	eMSG_EXIT,				
	
	eMSG_NUMMSGS
};

//return the string corresponding to a given message type
const char* GetMsgString(EMessageType eType);

//return a dword representation of the four character string for quick
//comparisons
uint32_t GetMsgVal(EMessageType eType);







//-----------------------------------------------------------------------------
//
// eMSG_INIT
//		Sent by the debugging engine when it has received a connection and
//			when the program finishes.
//		Data: engine name and version string.
//
//		Also sent by the GUI as a response to the init message to begin a
//			debugging session.
//		Data: three null terminated strings - the script to execute under 
//          debugger control, the working directory and the command line 
//          parameters.
//
// eMSG_EVALUATE
//		Sent by the GUI when it needs to evaluate a statement (watch etc).
//		Data: Null terminated expression
//
//		Also sent by the engine with the results of the expression
//		Data: null terminated expression result (data val or error msg)
//	
// eMSG_PING
//		Sent by the GUI or the engine to verify the presence of the other
//
// eMSG_PONG
//		Sent by the GUI or the engine after recieveing a eMSG_PING
//
// eMSG_BREAK
//		Sent by the engine when a breakpoint has been reached.
//		Data: Call stack in the form "Filename, Line#, FnName(ParamName1 P1Val, ..)\n"
//
// eMSG_BREAKPTLIST: 
//		Sent by the engine whenever the breakpoint list is modified
//		Data: List of breakpoints in the form: "Filename, Line#, Expression\n"
//
// eMSG_TRACE
//		Sent by the engine when the code sends a trace command
//		Data: Null terminated string
//
// eMSG_DONE
//		Sent by the engine to signal that a script has completed.  Should be followed by
//		an init message
//
// eMSG_EXCEPTION
//		Sent by the engine to signal that a python exception has occurred.
//		Data:
//
// eMSG_GLOBALS:
//		Sent by the engine to refresh the global variables display.
//		Data: double null terminated list of variables in the form:
//			"key, value"
//
// eMSG_LOCALS:
//		Sent by the engine to refresh the local variables display.
//		Data: double null terminated list of variables in the form:
//			"key, value"
//
// eMSG_MODULES
//		Sent by the engine to refresh the list of current modules
//		Data: double null terminated list of modules in the form:
//			"ModuleName, FilePath"
//
// eMSG_EXECUTE
//		Sent by the GUI to execute a statement (modify watch)
//		Data: null terminated expression
//
// eMSG_BREAKNOW
//		Sent by the GUI to force the engine to break at the next line of 
//			Python code executed
//		Data: none
//
// eMSG_SETBREAKPOINT
//		Sent by the GUI to set or clear a breakpoint
//		Data: Action[, Filename, Line#[, expression]] Where "Action" can be:
//			"1" = Set a breakpoint
//			"2" = clear a breakpoint
//			"3" = clear all breakpoints (this file)
//			"4" = clear all breakpoints (all files)
//			"5" = disable a breakpoint
//			"6" = enable a breakpoint
//			"7" = toggle this breakpoint
//
// eMSG_STEP
//		Sent by the GUI to step over, into or out of the current line of code
//		Data: Action which can be any of the following:
//			"1" indicates step over
//			"2" indicates into
//			"3" indicates outof
//			"4" indicates continue
//
// eMSG_RELOAD
//
// eMSG_STOP
//		Sent by the GUI to signal the engine to stop debugging the current script.  The
//		engine should reset and issue the init message.
//
// eMSG_EXIT
//		Sent by the GUI to signal the engine to end the debugging session and exit.
	




//Parameters for eMSG_STEP
const char STEP_OVER = '1';
const char STEP_INTO = '2';
const char STEP_OUT  = '3';
const char STEP_GO	 = '4';


//Parameters for eMSG_SETBREAKPOINT
const char  BREAKPT_SET				= '1';
const char  BREAKPT_CLEAR			= '2';
const char  BREAKPT_CLEARALLFILE	= '3';
const char  BREAKPT_CLEARALL		= '4';
const char  BREAKPT_DISABLE			= '5';
const char  BREAKPT_ENABLE			= '6';
const char  BREAKPT_TOGGLE			= '7';

#endif //_HEDBGMSG_H_