//////////////////////////////////////////////////////////////////////////////
//  File:    Run.h
//
//  Purpose: This file contains functions related to the run state of the
//  python interpreter.  The interpreter operates in one of a number of
//  states that determine its behavior.  The run state handles messages during
//  the execution of a line (or lines) of a python program.  It is particularly
//  important that the ProcessRunStateMessages be called periodically to ensure
//  that messages like "Break Now" get handled.  It is also important that the 
//  ProcessRunStateMessages be lightweight in order to acheive decent performance
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

#ifndef _RUN_H_
#define _RUN_H_

#include "Breakpoint.h"

//called from the main fn to activate a python script
int RunPythonScript(const char* cszPythonScriptName);

//called from the hedbg.RunLine fn to process commands during execution
int ProcessRunStateMessages();

//called to store the python command line arguments
void SetArguments(string& strParams, string& strScript);

//set a single breakpoint
void SetBreakpoint(const char* MsgBuffer);

//send a list of breakpoints to the debug client
void SendBreakpoints();

bool BreakBreakpoint(const char* cszFilename, int LineNumber);


#endif //_RUN_H_