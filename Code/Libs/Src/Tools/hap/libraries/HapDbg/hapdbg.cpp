//////////////////////////////////////////////////////////////////////////////
//  File:    hapdbg.cpp
//
//  Purpose: Defines the entry point for the DLL application.
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
#include "hapdbg.h"

#include "hedbg.h"
#include "init.h"
#include "DbgGlobals.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// global variables

CHeDbgSocket g_DebugSocket;
bool g_bBreakNow = false;
bool g_bContinueRun = true;
PyObject* g_pMainModule = Py_None;
PyObject* g_pMainDict = Py_None;
PyObject* g_pHeDebugger = Py_None;

//true if the debugger should break on any exception.
EExceptionMode g_eExceptionMode = eEXCEPTMODE_IGNORE;

EStepCommands g_nStepFlag = STEPFLAG_GO;
// Depth at which to break - as compared to frame->f_tstate->recursion_depth
int g_nStepOverBreakDepth = 1;	// Initial value means 'stop at the beginning of the first script'.
bool g_bBptsSet = false;


string DebugProgPathname;

const char* cszOption_BreakOnException = "BreakOnException";
const char* cszOption_BreakFirstLine = "BreakFirstLine";



void SetOption(const char* cszMsgBuffer)
{
	if (strstr(cszMsgBuffer, cszOption_BreakOnException))
	{
		char* pVal = strchr(cszMsgBuffer, '=');
		if (pVal)
		{
			pVal++;
			g_eExceptionMode = (EExceptionMode)strtoul(pVal, 0, 10);
			
			dprintf("Exception Mode set to: %d\n", g_eExceptionMode);
		}
	}
	else if (strstr(cszMsgBuffer, cszOption_BreakFirstLine))
	{
		g_nStepFlag = STEPFLAG_INTO;
	}
}




/**
    @brief Pass options along connections and agree on something.
    Internal method
*/
string strScript = "";
string strWorkingDir = "";
string strParams = "";

bool InitDebugConnection()
{
    // in_addr sAddr;
    // sAddr.S_un.S_addr = g_DebugSocket.GetAddress();
    // printf("Connected to: %s, port: %d\n", inet_ntoa(sAddr), nPort);

    // send the init message
    SendInit();

    // wait for the init response and the command line
    if (0 != WaitForInit(strScript, strWorkingDir, strParams))
        return false;

    if (0 == strScript.length())
        return false;

    return true;
}




//////////////////////////////////////////////////////////////////////////////
//  Exported debugger functions

namespace hapdbg
{


//////////////////////////////////////////////////////////////////////////////
//  Initialize
//
//  Py_Initialize must have been called along with importing __main__

HAPDBG_API bool Initialize(int nPort)
{
    // Initialise debug extensions
    if (0 > InitPythonDebug())
        return false;

	CHeDbgSocket::InitSockets();

    // Open a listening port
    int nRetVal = g_DebugSocket.Listen(nPort);
    if (nRetVal != 0)
        return false;

    return true;
} // Initialize

//////////////////////////////////////////////////////////////////////////////
//  HasConnection
//
//  Check if there is a current connection.
//
HAPDBG_API bool HasConnection()
{
    return g_DebugSocket.eGetState() != CHeDbgSocket::eSOCKSTATE_LISTENING;
}

//////////////////////////////////////////////////////////////////////////////
//  CheckForNewConnection
//
//  Poll for a fresh connection
//
HAPDBG_API bool CheckForNewConnection()
{
    if (g_DebugSocket.eGetState() == CHeDbgSocket::eSOCKSTATE_LISTENING)
        return false;

    return InitDebugConnection();
}

//////////////////////////////////////////////////////////////////////////////
//  WaitForConnection
//
//  Wait for connection from remote debug host
//  return true for successful connection, false when something goes wrong
//
HAPDBG_API bool WaitForConnection()
{
    // Wait for debug connection
    while (g_DebugSocket.eGetState() == CHeDbgSocket::eSOCKSTATE_LISTENING)
    {
		dprintf(".");
		Sleep(1000);
    }

    if (g_DebugSocket.eGetState() != CHeDbgSocket::eSOCKSTATE_CONNECTED)
    {
		dprintf("Unable to connect\n");
		return false;
    }

    return InitDebugConnection();
}

//////////////////////////////////////////////////////////////////////////////
//  Uninitialize
//
//  Finalise debugging, shutdown sockets.
//
HAPDBG_API void Uninitialize()
{
    //notify the GUI that we are closing
    g_DebugSocket.SendMessage(eMSG_DONE, "");

    //cleanup the sockets
    g_DebugSocket.Close();
    WSACleanup();
}

//////////////////////////////////////////////////////////////////////////////
//  GetOption
//
//  Get an option set from the GUI
//
HAPDBG_API bool GetOption(const string& optName, string& optVal)
{
	if (optName == cszOption_BreakOnException)
	{
		optVal = stringprintf("%d", g_eExceptionMode);
		
		return true;
	}
	else if (optName == cszOption_BreakFirstLine)
	{
		if (g_nStepFlag == STEPFLAG_INTO)
		{
			optVal = "1";
		}
		else
		{
			optVal = "0";
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////
//  GetInitVals
//
//  Get the script, dir and params set by the GUI
//
HAPDBG_API bool GetInitVals(string& cmdLine, string& workDir, string& cmdParms)
{
	cmdLine = strScript;
	workDir = strWorkingDir;
	cmdParms = strParams;
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////
//  RunPythonScript
//
//  Run a python script file
//
HAPDBG_API int RunPythonScript(const string& PythonScriptName)
{
	char* pFileOnly = strrchr(PythonScriptName.c_str(), '\\');
	if (pFileOnly)
	{
		pFileOnly++;
		DebugProgPathname = pFileOnly;
	}
	else
		DebugProgPathname = PythonScriptName;

	DebugProgPathname += ",";
	DebugProgPathname += PythonScriptName;
	
	
	FILE* DbgFile = fopen(PythonScriptName.c_str(), "rt");
	if (DbgFile)
	{
		int iRun = PyRun_SimpleFile(DbgFile, (char*)PythonScriptName.c_str());
		fclose(DbgFile);
	}
	else
		printf("Unable to open file %s\n", PythonScriptName.c_str());

	return -1;
	
} // RunPythonScript


//////////////////////////////////////////////////////////////////////////////
//  SetPythonArguments
//
//  Set the arguments used by python
//
void SetPythonArguments(string& strParams, string& strScript)
{
	//arguments are passed in as a space delimited string and must 
	//be converted to an array of char*

	//pointer to the current character
	char* pCur = &(strParams[0]);

	//pointer to the start of the current parameter
	char* pParam = pCur;

	//heres our array of char*
	vector<char*> vecParams;

	//here is something to allow a single parameter that contains spaces by enclosing
	//it in quotes
	bool bInQuotes = false;

	
	//first parameter should be the script name.
	if (strScript.size() > 0)
		vecParams.push_back(&(strScript[0]));
	
	while (*pCur)
	{
		if ((*pCur == ' ') && (bInQuotes == false))
		{
			//add a pointer to the current string to the parameter array
			vecParams.push_back(pParam);
			
			//move the parameter pointer to the next character
			pParam = pCur+1;

			//convert the current character (space) to a null
			*pCur = 0;
		}
		else if (*pCur == '"')
		{
			bInQuotes = !bInQuotes;
		}
		
		pCur++;
	}
	//add the last parameter
	if (*pParam)
		vecParams.push_back(pParam);

	//store the arguments
	PySys_SetArgv(vecParams.size(), &(vecParams[0]));

}//SetArguments


}//namespace









