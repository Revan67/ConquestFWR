//////////////////////////////////////////////////////////////////////////////
//  File:    Run.cpp
//
//  Purpose: Implementation of run state
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
#include "DbgNetLib/HeDbgSocket.h"
#include "DbgNetLib/HeDbgMsg.h"
//#include "ConsoleEmbed.h"
#include "hapdbg.h"
#include "Run.h"
#include "DbgGlobals.h"



vector<CFileBreakpts> g_vecBreakpoints;
typedef vector<CFileBreakpts>::iterator TBpItr;
typedef vector<CFileBreakpts>::iterator TBpItr_const;





int ProcessRunStateMessages()
{
	if (g_DebugSocket.eGetState() != CHeDbgSocket::eSOCKSTATE_CONNECTED)
	{
		dprintf("Socket connection closed - quitting\n");
		PyObject_CallMethod(g_pHeDebugger, "set_quit", NULL);
		return -1;
	}
	
	SMsgHeader MsgHdr;
	EMessageType eType = g_DebugSocket.ReadMessageHdr(&MsgHdr);
	while (eType > eMSG_INVALID)
	{
		int MsgBytes = MsgHdr.GetMsgSize();	//strtol(MsgHdr.MsgSize, NULL, 10);
		
		static TCharVector RunMsgBuffer;
		RunMsgBuffer.resize(MsgBytes);

		int nMsgSize = 0;
		if (MsgBytes > 0)
		{
			memset (&(RunMsgBuffer[0]), 0, MsgBytes);
			nMsgSize = g_DebugSocket.ReadMessageBody(&(RunMsgBuffer[0]), &MsgHdr);	
		}
		
		switch (eType)
		{
		case eMSG_SETBREAKPOINT:
			assert(nMsgSize > 0);
			SetBreakpoint(&(RunMsgBuffer[0]));
			break;

		case eMSG_BREAKNOW:
			g_bBreakNow = true;
			break;
				
		case eMSG_RELOAD:
			assert(nMsgSize > 0);
			//ReloadFile(RunMsgBuffer);
			break;
		case eMSG_STOP:
			printf("Stopping Debugger\n");
			throw "Stopping Debugger";
			//PyObject_CallMethod(g_pHeDebugger, "set_quit", NULL);

			break;

		case eMSG_EXIT:
			g_bContinueRun = false;
			printf("Stopping Debugger\n");
			//PyObject_CallMethod(g_pHeDebugger, "set_quit", NULL);
			throw "Stopping Debugger";
			break;

		case eMSG_PING:
			g_DebugSocket.SendMessage(eMSG_PONG, "");
			break;

		case eMSG_SETOPTION:
			SetOption(&(RunMsgBuffer[0]));
			break;

		default:
			dprintf("ProcessRunStateMessages - Invalid Message: %d\n", eType);
	
		}
		
		eType = g_DebugSocket.ReadMessageHdr(&MsgHdr);
	}
	return 0;

} // ProcessRunStateMessages



void SetBreakpoint(const char* MsgBuffer)
{
	const char* pLine = NULL;
	const char* pFile = NULL;
	const char* pExpr = NULL;

	//MsgBuffer has the form: cmd,line#,filename[,expression]
	
	pLine = strchr(MsgBuffer, ',');
	if (pLine)
	{
		//move past the comma
		pLine++;

		pFile = strchr(pLine , ',');
		if (pFile)
		{
			pFile++;
			pExpr = strchr(pFile, ',');
			
			if (pExpr)
			{
				*((char*)pExpr) = 0;
				pExpr++;
			}
		}
	}

	
	PyObject* pEvalRes = NULL;
	int nLineNum = -1;
	if (pLine)
		nLineNum = strtol(pLine, NULL, 10)+1;			

	switch (MsgBuffer[0])
	{
	case BREAKPT_SET:
		if (pFile && pLine && (nLineNum > -1))
		{		
			/*if (pExpr == NULL)
				pEvalRes = PyObject_CallMethod(g_pHeDebugger, "SetBreakpoint", "si", pFile, nLineNum);
			else
				pEvalRes = PyObject_CallMethod(g_pHeDebugger, "SetBreakpoint", "siis", pFile, nLineNum, 0, pExpr);*/
			
			TBpItr pBp;
			for (pBp = g_vecBreakpoints.begin(); pBp != g_vecBreakpoints.end(); pBp++)
			{
				if (*pBp == pFile)
				{
					break;
				}
			}

			if (pBp == g_vecBreakpoints.end())
			{
				//a matching file was not found
				CFileBreakpts newbp;
				newbp.SetFilename(pFile);
				newbp.AddLine(nLineNum);
				g_vecBreakpoints.push_back(newbp);
			}
			else
			{
				//breakpoint exist in this file, add the line
				pBp->AddLine(nLineNum);
			}



		}
		break;

	case BREAKPT_TOGGLE:
		if (pFile && pLine && (nLineNum > -1))
		{		
			//pEvalRes = PyObject_CallMethod(g_pHeDebugger, "ToggleBreakpoint", "si", pFile, nLineNum);
			TBpItr pBp;
			for (pBp = g_vecBreakpoints.begin(); pBp != g_vecBreakpoints.end(); pBp++)
			{
				if (*pBp == pFile)
				{
					break;
				}
			}

			if (pBp == g_vecBreakpoints.end())
			{
				CFileBreakpts newbp;
				newbp.SetFilename(pFile);
				newbp.AddLine(nLineNum);
				g_vecBreakpoints.push_back(newbp);
			}
			else
			{
				if (pBp->CheckLine(nLineNum))
				{
					pBp->RemoveLine(nLineNum);
					if (pBp->GetLinesCount() == 0)
						g_vecBreakpoints.erase(pBp);
				}
				else
				{
					pBp->AddLine(nLineNum);
				}
			}
		}
		break;

	case BREAKPT_CLEAR:
		if (pFile && pLine && (nLineNum > -1))
		{		
			//pEvalRes = PyObject_CallMethod(g_pHeDebugger, "ClearBreakpoint", "si", pFile, nLineNum);
			for (TBpItr pBp = g_vecBreakpoints.begin(); pBp != g_vecBreakpoints.end(); pBp++)
			{
				if (*pBp == pFile)
				{
					pBp->RemoveLine(nLineNum);
					if (pBp->GetLinesCount() == 0)
						g_vecBreakpoints.erase(pBp);
				}
			}
		}
		break;

	case BREAKPT_CLEARALLFILE:
		{
			for (TBpItr pBp = g_vecBreakpoints.begin(); pBp != g_vecBreakpoints.end(); pBp++)
			{
				if (*pBp == pFile)
					g_vecBreakpoints.erase(pBp);
			}
		}
		break;
	case BREAKPT_CLEARALL:
		//pEvalRes = PyObject_CallMethod(g_pHeDebugger, "ClearBreakpoint", "si", "", -1);
		g_vecBreakpoints.clear();
		break;
	case BREAKPT_DISABLE:
	case BREAKPT_ENABLE:
		break;
	}

	if ((pEvalRes != NULL) && (pEvalRes != Py_None))
	{
		dprintf(PyString_AsString(pEvalRes));
		dprintf("\n");
	}
	
	//send the client the current list of breakpoints to get them up to date
	SendBreakpoints();

	g_bBptsSet = (g_vecBreakpoints.size() > 0);

}



void SendBreakpoints()
{
	//AString = '%d,%d,%s' % (bp.number, bp.line-1, bp.file)
	static TCharVector vecBpBuffer;
	vecBpBuffer.clear();

	int BpNum = 0;
	TBpItr pBp;
	char buff[32];

	for (pBp = g_vecBreakpoints.begin(); pBp != g_vecBreakpoints.end(); pBp++)
	{
		for (int i = 0; i < pBp->GetLinesCount(); i++)
		{
			//breakpoint number and line number
			sprintf(buff, "%d,%d,", BpNum++, pBp->GetLineInx(i)-1);
			char* pCur = buff;
			while (*pCur)
				vecBpBuffer.push_back(*pCur++);
			
			
			const char* BpFile = pBp->GetFilename();
			while(*BpFile)
			{
				vecBpBuffer.push_back(*BpFile++);
			}
			vecBpBuffer.push_back(0);
			//dprintf("%s: %d\n", pBp->GetFilename(), pBp->GetLineInx(i)-1);
		}
	}

	vecBpBuffer.push_back(0);

	g_DebugSocket.SendMessageSize(eMSG_BREAKPTLIST, &(vecBpBuffer[0]), vecBpBuffer.size());		
} // SendBreakpoints



bool BreakBreakpoint(const char* cszFilename, int LineNumber)
{
	TBpItr_const pBp = g_vecBreakpoints.begin();
	TBpItr_const pEnd = g_vecBreakpoints.end();

	while (pBp != pEnd)
	{
		if (pBp->CheckBreakpoint(cszFilename, LineNumber))
			return true;
		pBp++;
	}
	return false;
}

