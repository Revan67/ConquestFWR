//////////////////////////////////////////////////////////////////////////////
//  File:    DbgGlobals.h
//
//  Purpose: dll global variables used for the debugger mechanism
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

#ifndef _DBGGLOBALS_H_
#define _DBGGLOBALS_H_

#include "DbgNetLib/HeDbgSocket.h"
#include "DbgNetLib/HeDbgMsg.h"


//a typdef for a char buffer - we'll use this a lot.
typedef vector<char> TCharVector;


//global pointer to the python's main module and dictionary objects
extern PyObject* g_pMainModule;
extern PyObject* g_pMainDict;

//global pointer to the python debugger object
extern PyObject *g_pHeDebugger;

//global flag indicating whether execution should continue
extern bool g_bContinueRun;

//global flag signalling the debugger to break on the next line of python code
extern bool g_bBreakNow;

//global socket object
extern CHeDbgSocket g_DebugSocket;


// I changed these from const ints to enums so that the debugger
// will translate the values into symbolic names for me.
enum EStepCommands
{
	STEPFLAG_GO   = 0,
	STEPFLAG_OVER = 1,
	STEPFLAG_INTO = 2,
	STEPFLAG_OUT  = 3
};
extern EStepCommands g_nStepFlag;


// Depth at which to break - only relevant when in OVER, INTO or OUT modes.
extern int g_nStepOverBreakDepth;

//true if there are any breakpoints set
extern bool g_bBptsSet;

//gets the current level of recursion
int CurrentRecursionLevel();


//debugger options:
enum EExceptionMode
{
	eEXCEPTMODE_BREAKALL = 0,
	eEXCEPTMODE_BREAKUNHAND,
	eEXCEPTMODE_IGNORE,
};

extern EExceptionMode g_eExceptionMode;

//global function to set an option when an eMSG_SETOPTION message is received
void SetOption(const char* cszMsgBuffer);



extern string DebugProgPathname;



#endif //_DBGGLOBALS_H_