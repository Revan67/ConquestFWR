//////////////////////////////////////////////////////////////////////////////
//  File:    Break.cpp
//
//  Purpose: Implements the state the debugger is in while python is broken - 
//  waiting for a step or run command.
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
#include "DbgGlobals.h"

#include "Run.h"
#include "VariablesMsg.h"
#include "HeDbg.h"

CVariablesMsg VarsMsg;


extern string DebugProgPathname;



TCharVector DictVector;
TCharVector ModuleVector;
TCharVector StackVector;



int  ProcessBreakStateMessages(PyFrameObject *frame);
void StepCode(const char* cszStepMsg);
void EvaluateExpr(char* szExpr, PyFrameObject *frame, bool bReturnErr = true);
void ExecuteExpr(const char* cszExpr, PyFrameObject *frame);
void SetVariable(const char* cszExpr, PyFrameObject *frame);

void SendLocalsAndGlobals(PyObject *pLocals, PyObject *pGlobals);
void SendModules();



void AddStringToVec(char* Str, TCharVector& rVec)
{
	assert(Str);
	while (*Str)
		rVec.push_back(*Str++);
}

void FormatStackMsg(PyFrameObject *frame, PyObject* pTraceback, TCharVector& rvecStackMsg)
{
	assert(frame);

	rvecStackMsg.resize(0);

	PyFrameObject *pCurFrame = frame;
	
	//the traceback object is an optional parameter.
	//sometimes the linenumbers held by the traceback object differ slightly from those in the
	//stack frame.  In order to show the correct line numbers we want the one from the traceback
	//(perhaps these are the ones before the exception occurred???)
	//run through the various levels of the traceback object and store the line numbers in a vector.
	tracebackobject *pCurTb = (tracebackobject *)pTraceback;
	vector<int> tbLineNos;
	while (pCurTb)
	{
		tbLineNos.push_back(pCurTb->tb_lineno);
		pCurTb = pCurTb->tb_next;
	}

	char buff[64];
	while (pCurFrame)
	{
		//#filename, line#, FnName(p1 p1val, p2 p2val...)
		char *FileNmStr = PyString_AsString(pCurFrame->f_code->co_filename);
		AddStringToVec(FileNmStr, rvecStackMsg);

		AddStringToVec(",", rvecStackMsg);
		
		if (tbLineNos.size() > 0)
		{
			//the top of the stack frame corresponds to the last tracback (if any) 
			sprintf(buff, "%d", tbLineNos.back());
			tbLineNos.pop_back();
		}
		else
		{
			int line = PyCode_Addr2Line(pCurFrame->f_code, pCurFrame->f_lasti);
			sprintf(buff, "%d", line);
		}

		AddStringToVec(buff, rvecStackMsg);
		AddStringToVec(",", rvecStackMsg);
		

		char *FuncNmStr = PyString_AsString(pCurFrame->f_code->co_name);
		AddStringToVec(FuncNmStr, rvecStackMsg);

		AddStringToVec("(", rvecStackMsg);
		
		if (pCurFrame->f_code->co_argcount)
		{
			assert(PyTuple_Check(pCurFrame->f_code->co_varnames) );

			int ArgCount = __min(PyTuple_Size(pCurFrame->f_code->co_varnames), pCurFrame->f_code->co_argcount);

			for (int i = 0; i < ArgCount; i++)
			{
				PyObject* argname = PyTuple_GetItem(pCurFrame->f_code->co_varnames, i); 
				if (argname)
				{
					char* cszArgName = PyString_AsString(argname);
					if (cszArgName)
					{
						if (strcmp(cszArgName, "self") == 0)
							continue;

						AddStringToVec( cszArgName, rvecStackMsg);
						AddStringToVec(" ", rvecStackMsg);
						
						//PyObject* pVal = PyDict_GetItemString(pCurFrame->f_locals, cszArgName);
						//if (pVal)
						//	AddStringToVec(PyString_AsString( PyObject_Str(pVal)) , rvecStackMsg);
					}
				}
						
				if (i < pCurFrame->f_code->co_argcount-1)
					  AddStringToVec(", ", rvecStackMsg);
			}
		}
		AddStringToVec(")", rvecStackMsg);
		rvecStackMsg.push_back(0);

		pCurFrame = pCurFrame->f_back;
	}
	rvecStackMsg.push_back(0);
}

int EnterBreakState(PyFrameObject *frame, PyObject* pTraceback)
{
	//printf("Breaking - line%d: \n", frame->f_lineno);
	PyFrame_FastToLocals(frame);

	//send a break message along with a call stack representation
	FormatStackMsg(frame, pTraceback, StackVector);
	g_DebugSocket.SendMessageSize(eMSG_BREAK, &(StackVector[0]), StackVector.size());

	//send an up to date list of modules
	SendModules();

	//send the locals and globals vars
	SendLocalsAndGlobals(frame->f_locals, frame->f_globals);

	//handls break state messages
	ProcessBreakStateMessages(frame);
	
	//reset the break now flag
	g_bBreakNow = false;

	return 0;
	
} // EnterBreakState



int ProcessBreakStateMessages(PyFrameObject *frame)
{
	bool bRun = false;
	SMsgHeader MsgHdr;
	TCharVector MessageBuffer;

	static int nInvalidCount = 0;

	EMessageType eType = g_DebugSocket.ReadMessageHdr(&MsgHdr);
	while (bRun == false)
	{
		//check to see that the socket is still open
		if (g_DebugSocket.eGetState() != CHeDbgSocket::eSOCKSTATE_CONNECTED)
		{
			dprintf("Socket connection closed - quitting\n");
			throw "Stopping Debugger";
			return -1;
		}
		
		
		int nMsgBytes = MsgHdr.GetMsgSize();	// strtol(MsgHdr.MsgSize, NULL, 10);
		MessageBuffer.resize(nMsgBytes);

		int nMsgSize = 0;
		if (nMsgBytes > 0)
		{
			memset (&(MessageBuffer[0]), 0, nMsgBytes);
			nMsgSize = g_DebugSocket.ReadMessageBody(&(MessageBuffer[0]), &MsgHdr);	
		}
		if (eType == eMSG_INVALID)
			nInvalidCount++;
		else
			nInvalidCount = 0;

		switch (eType)
		{
		case eMSG_INVALID:
			if (nInvalidCount > 250)
			{
				//check to see if the socket is still there:
				g_DebugSocket.SendMessage(eMSG_PING, "");
				nInvalidCount = 0;
			}
			Sleep(50);
			break;

		case eMSG_SETBREAKPOINT:
			assert(nMsgSize > 0);
			SetBreakpoint(&(MessageBuffer[0]));
			break;

		case eMSG_BREAKNOW:
			//can ignore since we are allready in break;
			break;

		case eMSG_EVALUATE:
			assert(nMsgSize > 0);
			EvaluateExpr(&(MessageBuffer[0]), frame, true);
			break;

		case eMSG_EVALNOERR:
			assert(nMsgSize > 0);
			EvaluateExpr(&(MessageBuffer[0]), frame, false);
			break;

		case eMSG_EXECUTE:
			assert(nMsgSize > 0);
			ExecuteExpr(&(MessageBuffer[0]), frame);
			break;

		case eMSG_STEP:
			assert(nMsgSize > 0);
			StepCode(&(MessageBuffer[0]));
			bRun = true;
			break;

		case eMSG_RELOAD:
			assert(nMsgSize > 0);
			//ReloadFile(&(MessageBuffer[0]));
			break;

		case eMSG_STOP:
			printf("Stopping Debugger\n");
			throw "Stopping Debugger";
			bRun = true;
			break;

		case eMSG_EXIT:
			printf("Stopping Debugger\n");
			throw "Stopping Debugger";
			
			g_bContinueRun = false;
			bRun = true;
			break;

		case eMSG_PING:
			g_DebugSocket.SendMessage(eMSG_PONG, "");
			break;

		case eMSG_PONG:
			//ignore
			break;

		case eMSG_SETOPTION:
			SetOption(&(MessageBuffer[0]));
			break;

		default: 
			dprintf("ProcessBreakStateMessages Invalid message: %d\n", eType);
			assert(0);
			break;
		}
		if (bRun)
			break;
		else
			eType = g_DebugSocket.ReadMessageHdr(&MsgHdr);

	} //while

	return 0;
}


void StepCode(const char* cszStepMsg)
{
	assert(strlen(cszStepMsg) > 0);
	switch (cszStepMsg[0])
	{
		case STEP_GO:
			g_nStepFlag = STEPFLAG_GO;
			break;
		case STEP_OVER:
			g_nStepFlag = STEPFLAG_OVER;
			g_nStepOverBreakDepth = CurrentRecursionLevel();
			break;
		case STEP_INTO:
			g_nStepFlag = STEPFLAG_INTO;
			g_nStepOverBreakDepth = CurrentRecursionLevel() + 1;
			break;
		case STEP_OUT:
			g_nStepFlag = STEPFLAG_OUT;
			g_nStepOverBreakDepth = CurrentRecursionLevel() - 1;
			break;
		default:
			assert(0);
			break;
	}
}


void EvaluateExpr(char* szExprInitial, PyFrameObject *frame, bool bReturnErr)
{
	//szExprInitial should look like "1, evaluateme" where the 1 is the expression number
	char* space = strchr(szExprInitial, ' ');
	if (!space)
	{
		dprintf("EvaluateExpr: Invalid expression\n");
		return;
	}
	assert(space > szExprInitial);
	assert(space[-1] == ',');

	// Create two strings, one containing the prefix (expression number, comma, and space)
	// and the other containing the expression to be evaluated.
	string prefix(szExprInitial, space + 1);
	string expression(space + 1, szExprInitial + strlen(szExprInitial));

	// BUGBUG - hack!
	// This piece of code lets the current objects variables be more easily viewed when in
	// a member function. However this should really be done in a more generic way - anytime
	// the expression evaluates to a class object. Oh well - this hack is very useful, and
	// seems to be harmless.
	if (expression == "self")
	{
		expression = "self.__dict__";
	}
	
	//run the string in the current context
	PyObject* pGlobals = frame->f_globals;
	PyObject* pLocals = frame->f_locals;

	// It would be nice to try to evaluate private member variables. To do this we look for any ".__" sequences.
	// When we find one we want to find the expression to the left, find it's class name,
	// and replace .__ with ._classname__
	// Given an object handle self of a class, self.__class__ is the class object.
	// It typically looks like this: <class main.Foo at 0x00765DE0>
	// You can parse out 'Foo' or evaluate str(self.__class__) which returns something like:
	// '__main__.Foo' (quotes included). The formatting may be different for types implemented
	// in C++.
	// So, if str = "self.__private" we just need to go:
	// // Break up into multiple chunks.
	// chunks = str.split(".__")
	// // Assume we just have two.
	// className = eval("str(%s.__class__)" % chunks[0]).split(".")[1][:-1]
	// stringToEvaluate = "%s._%s__%s" % (chunks[0], className, chunks[1])

	// Another useful thing to do is to notice when the result of evaluating a variable is
	// just the instance information, i.e.; repr(self) gives something like this:
	// '<__main__.Foo instance at 0x0076A218>' (quotes included) whereas dir(self) will
	// give much more useful results. Perhaps the ideal situation would be doing a dir
	// and then evaluating each element to give name/value pairs.
	// Perhaps the ideal thing to do is display self.__dict__. Displaying metadata such
	// as __module__, __doc__, __class__, and the list of functions would also be useful.

	PyObject* pEvalRes = PyRun_String(const_cast<char*>(expression.c_str()), Py_eval_input, pGlobals, pLocals);

	if (pEvalRes)
	{
		//we have a result, get a repr of it
		PyObject* pReprObj = PyObject_Repr(pEvalRes);
		if (pReprObj)
		{
			//convert that to a string
			char* pObjStr = PyString_AsString(pReprObj);

			//copy the result to the result buffer (preserving the number, comma and space)

			string result = prefix + pObjStr;

			g_DebugSocket.SendMessage(eMSG_EVALUATE, result.c_str());
		}
		else if (bReturnErr)
			g_DebugSocket.SendMessage(eMSG_EVALUATE, "Repr Failed");
	}
	else if (bReturnErr)
	{
		//to see the error in the output window:
		//PyErr_Print();

		string ErrMsg;

		PyObject* pType = 0;
		PyObject* pVal = 0;
		PyObject* pTracebk = 0;

		PyErr_Fetch(&pType, &pVal,&pTracebk);
		PyErr_NormalizeException(&pType, &pVal,&pTracebk);
		if ( PyErr_GivenExceptionMatches(pType, PyExc_SyntaxError))
		{
			PyObject *message;
			char *filename;
			int *lineno;
			int *offset;
			char *text = 0;

			if (PyTuple_Check(pVal))
			{
				PyArg_Parse(pVal, "(O(ziiz))", &message, &filename,
				            &lineno, &offset, &text);
				ErrMsg += PyString_AsString(message);
				if (text)
					ErrMsg += text;
			}
			else
			{
				PyObject* v;
				if ( (v = PyObject_GetAttrString(pVal, "msg")))
				{
					ErrMsg += PyString_AsString(v);
				}
				if ((v = PyObject_GetAttrString(pVal, "text")))
				{
					if (v && (v != Py_None))
						ErrMsg += PyString_AsString(v);
				}
			}
		}
		else if (PyClass_Check(pType))
		{
			PyClassObject* exc = (PyClassObject*)pType;
			PyObject* className = exc->cl_name;
			if (className)
				ErrMsg += PyString_AsString(className);		
		}
		else
		{

		}
		
		if (ErrMsg.length())
		{
			string result = prefix + ErrMsg;
			g_DebugSocket.SendMessage(eMSG_EVALUATE, result.c_str());
		}
	}
	else // no error information is desired, clear the error indicator
		PyErr_Clear();

} // EvaluateExpr


void ExecuteExpr(const char* cszExpr, PyFrameObject *frame)
{
	PyObject* pGlobals = frame->f_globals;
	PyObject* pLocals = frame->f_locals;

	PyObject* pEvalRes = PyRun_String((char*)cszExpr, Py_single_input, pGlobals, pLocals);
	if (pEvalRes)
	{
		//store the (possibly) modified locals back into their "fast" counterpart
		PyFrame_LocalsToFast(frame, 0);
	}
	else
	{
		PyErr_Print();
	}
	SendLocalsAndGlobals(pLocals, pGlobals);
}


void SendLocalsAndGlobals(PyObject *pLocals, PyObject *pGlobals)
{
	if (pGlobals)
	{
		VarsMsg.FormatGlobalsDict(pGlobals);
		int MsgSize = VarsMsg.MsgSize();
		if (MsgSize > 0)
		{
			g_DebugSocket.SendMessageSize(eMSG_GLOBALS, VarsMsg.GetMsg(), MsgSize);	
		}
	}
	
	if (pLocals)
	{
		VarsMsg.FormatLocalsDict(pLocals);
		int MsgSize = VarsMsg.MsgSize();
		if (MsgSize > 0)
		{
			g_DebugSocket.SendMessageSize(eMSG_LOCALS, VarsMsg.GetMsg(), MsgSize);	
		}
	}

}


void FormatModuleEntry(PyObject* pKey, PyObject* pValue, TCharVector* pFormatedEntrys)
{
	char* pKeyStr = PyString_AsString(PyObject_Str(pKey));
	if (pKeyStr && strlen(pKeyStr) > 0)
	{
		char* pModuleFile = PyModule_GetFilename(pValue);
		if (pModuleFile && strlen(pModuleFile) > 0)
		{
			//module name, filename
			pFormatedEntrys->insert(pFormatedEntrys->end(), pKeyStr, pKeyStr+strlen(pKeyStr));
			
			ModuleVector.push_back(',');			
			pFormatedEntrys->insert(pFormatedEntrys->end(), pModuleFile, pModuleFile+strlen(pModuleFile)+1);
		}
		else
		{
			if (PyErr_Occurred() != NULL)
			{
				PyErr_Clear();
			}
		}
	}
}

void SendModules()
{
	PyObject* pImportDict = PyImport_GetModuleDict();
	
	PyObject *key, *value;
	int pos = 0;
	while (PyDict_Next(pImportDict, &pos, &key, &value)) 
	{		
		//dprintf("Module:  %s - %s\n", PyString_AsString(PyObject_Str(key)), PyString_AsString(PyObject_Str(PyObject_Type(value))) );
		FormatModuleEntry(key, value, &ModuleVector);	
	}

	if (ModuleVector.size() > 1)
	{
		ModuleVector.push_back(0);
		g_DebugSocket.SendMessageSize(eMSG_MODULES, &(ModuleVector[0]), ModuleVector.size());
	}

}

