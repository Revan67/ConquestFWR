//////////////////////////////////////////////////////////////////////////////
//  File:    hapdbg.h
//
//  Purpose: the external interface to the hapdbg library
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




// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the HAPDBG_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// HAPDBG_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef HAPDBG_EXPORTS
#define HAPDBG_API __declspec(dllexport)
#else
#define HAPDBG_API __declspec(dllimport)
#endif

//-----------------------------------------------------------------------------
// Socket default port number
#define HE_DEBUGGER_PORTNUM	2020


//wrap all this in a simple namespace to avoid collisions with the fairly
//common sounding names used.
namespace hapdbg
{
	//initialize hap's debugging capability
	HAPDBG_API bool Initialize(int nPort);

	//check to see if a connection has been established
	HAPDBG_API bool HasConnection();

	//wait (block) until a connection is found
	HAPDBG_API bool WaitForConnection();

	//check to see if a new connection is available
	HAPDBG_API bool CheckForNewConnection();

	//clean up
	HAPDBG_API void Uninitialize();

	//get the value of a Hap specified debugging option
	HAPDBG_API bool GetOption(const string& optName, string& optVal);

	//get the values sent with the init function
	HAPDBG_API bool GetInitVals(string& cmdLine, string& workDir, string& cmdParms);

	//run a python script
	HAPDBG_API int RunPythonScript(const string& pyScriptFileName);
	
	//set the python command line args.
	HAPDBG_API void SetPythonArguments(string& strParams, string& strScript);
}










