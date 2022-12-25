//////////////////////////////////////////////////////////////////////////////
//  File:    HeDbg.cpp
//
//  Purpose: Initializes the python debugging and implements the python 
//  debugging interface functions
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
#include "HeDbg.h"
#include "DbgNetLib/HeDbgSocket.h"
#include "DbgNetLib/HeDbgMsg.h"
#include "hapdbg.h"
#include "Run.h"
#include "Break.h"
#include "DbgGlobals.h"


//////////////////////////////////////////////////////////////////////////////
// static vars (used for simple profiling support

// Keep track of calls, exceptions, lines of code executed and returns.
static __int64 s_executionCounts[4];
// Store the last value returned, for calculating deltas.
static __int64 s_lastValues[NUMELEMENTS(s_executionCounts)];





//////////////////////////////////////////////////////////////////////////////
// Python extension functions


//////////////////////////////////////////////////////////////////////////////
// OutputDebugString
//
// This function is used to replace the stderr output from python to direct
// it to Hap's stderr window.
//
static PyObject*
OutputDebugString(PyObject*, PyObject* args)
{
	char *msg;
	PyObject* pObj;
	//args contains a single parameter which is the string to write.
	if (!PyArg_ParseTuple(args, "Os:OutputDebugString", &pObj, &msg))
		return NULL;

	if (strlen(msg) > 0)
	{
		//send the message to our own stderr
		//dprintf(msg);

		//send it to the debugger as well
		g_DebugSocket.SendMessage(eMSG_TRACE, msg);
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

//////////////////////////////////////////////////////////////////////////////
// OutputDebugString
//
// This function is used to replace the print output from python to direct
// it to Hap's output window.
//
static PyObject*
OutputString(PyObject*, PyObject* args)
{
	char *msg;
	PyObject* pObj;
	//args contains a single parameter which is the string to write.
	if (!PyArg_ParseTuple(args, "Os:OutputString", &pObj, &msg))
		return NULL;

	if (strlen(msg) > 0)
	{
		//send it to our stdout
		printf(msg);

		//send it to the debugger as well
		g_DebugSocket.SendMessage(eMSG_OUTPUT, msg);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

//////////////////////////////////////////////////////////////////////////////
// GetCounts
//
// Get the current count for the indicated counter
//
static PyObject* GetCounts(int what)
{
	assert(what >= 0 && what < NUMELEMENTS(s_executionCounts));

	// Return the values as a float (actually double) because an
	// int doesn't have enough precision. The 53 bits of a double
	// should ensure that this won't lose precision for a really
	// long time, and should never overflow.

	// Build a tuple of the two numbers.
	PyObject* p = Py_BuildValue("(ff)",
				double(s_executionCounts[what]),
				double(s_executionCounts[what] - s_lastValues[what]));

	// This is not thread safe. i.e.; the results will be unreliable or meaningless if
	// Python is running in multiple OS threads.
	s_lastValues[what] = s_executionCounts[what];
	return p;
}

//////////////////////////////////////////////////////////////////////////////
// GetFunctionCallCount
//
// Return performance information as a tuple - total is element zero, delta
// is element one.
//
static PyObject*
GetFunctionCallCount(PyObject*, PyObject* args)
{
	return GetCounts(PyTrace_CALL);
}

//////////////////////////////////////////////////////////////////////////////
// GetExceptionCount
//
// Return performance information as a tuple - total is element zero, delta
// is element one.
//
static PyObject*
GetExceptionCount(PyObject*, PyObject* args)
{
	return GetCounts(PyTrace_EXCEPTION);
}

//////////////////////////////////////////////////////////////////////////////
// GetLineCount
//
// Return performance information as a tuple - total is element zero, delta
// is element one.
//
static PyObject*
GetLineCount(PyObject*, PyObject* args)
{
	return GetCounts(PyTrace_LINE);
}

//////////////////////////////////////////////////////////////////////////////
// GetFunctionReturnCount
//
// Return performance information as a tuple - total is element zero, delta
// is element one.
//
static PyObject*
GetFunctionReturnCount(PyObject*, PyObject* args)
{
	return GetCounts(PyTrace_RETURN);
}



//////////////////////////////////////////////////////////////////////////////
// python extension definitions

static PyMethodDef ErrMethods[] = 
{
    {"write", OutputDebugString, METH_VARARGS, "write to stderr"},
    {NULL},
};

static PyMethodDef OutMethods[] = 
{
    {"write", OutputString, METH_VARARGS, "write to stdout"},
    {NULL},
};

static PyMethodDef ModuleMethods[] =
{
	{ "GetFunctionCallCount", GetFunctionCallCount, METH_VARARGS, "Get the total number of function calls executed and the number executed since the last call to this function." },
	{ "GetExceptionCount", GetExceptionCount, METH_VARARGS, "Get the total number of exceptions and the number executed since the last call to this function." },
	{ "GetLineCount", GetLineCount, METH_VARARGS, "Get the total number of lines executed and the number executed since the last call to this function." },
	{ "GetFunctionReturnCount", GetFunctionReturnCount, METH_VARARGS, "Get the total number of function returns executed and the number executed since the last call to this function." },
	{ NULL },
};

//forward declaration (implementation below)
static PyObject* he_excepthook(PyObject* self, PyObject* args);

static PyMethodDef he_except_method = 
	{"he_excepthook", he_excepthook, METH_VARARGS, "Custom exception handler"};






//////////////////////////////////////////////////////////////////////////////
// 
void initOutput()
{
    PyMethodDef *def;

    /* create a new module and class */
    PyObject *module = Py_InitModule("HeDbg", ModuleMethods);
    PyObject *moduleDict = PyModule_GetDict(module);
    PyObject *classDict = PyDict_New();
    PyObject *className = PyString_FromString("HeDbgOut");
    PyObject *dbgClass = PyClass_New(NULL, classDict, className);
    PyDict_SetItemString(moduleDict, "HeDbgOut", dbgClass);
    Py_DECREF(classDict);
    Py_DECREF(className);
    Py_DECREF(dbgClass);
 
    
    /* add methods to class */
    for (def = ErrMethods; def->ml_name != NULL; def++) 
	{
		PyObject *func = PyCFunction_New(def, NULL);
		PyObject *method = PyMethod_New(func, NULL, dbgClass);
		PyDict_SetItemString(classDict, def->ml_name, method);
		Py_DECREF(func);
		Py_DECREF(method);
    }
	static PyObject* pDbgInst = PyInstance_New(dbgClass, 0, 0);
	PySys_SetObject("stderr", pDbgInst);

	classDict = PyDict_New();
    className = PyString_FromString("HeStdOut");
    dbgClass = PyClass_New(NULL, classDict, className);
    PyDict_SetItemString(moduleDict, "HeStdOut", dbgClass);

	for (def = OutMethods; def->ml_name != NULL; def++) 
	{
		PyObject *func = PyCFunction_New(def, NULL);
		PyObject *method = PyMethod_New(func, NULL, dbgClass);
		PyDict_SetItemString(classDict, def->ml_name, method);
		Py_DECREF(func);
		Py_DECREF(method);
    }

	static PyObject* pOutInst = PyInstance_New(dbgClass, 0, 0);
	PySys_SetObject("stdout", pOutInst);
}



/*#define BUFFERSIZE 10240

typedef struct {
    PyObject_HEAD
    PyObject *filemap;
    PyObject *logfilename;
    int index;
    unsigned char buffer[BUFFERSIZE];
    FILE *logfp;
    int lineevents;
    int linetimings;
    int frametimings;
    // size_t filled;
    int active;
    int next_fileno;
  //  hs_time prev_timeofday;
} ProfilerObject;*/

// Just forward declare this until we need it.
struct ProfilerObject;

static s_currentRecursionLevel = 0;
int CurrentRecursionLevel()
{
	return s_currentRecursionLevel;
}

//static int lineExecutedSinceExcept = 0;
//static int lastlevel = -1;



static void HandleExcept(ProfilerObject *self, PyFrameObject *frame, int what,
                PyObject *arg)
{
	if (eEXCEPTMODE_IGNORE == g_eExceptionMode)
	{
		return;
	}
	else if (eEXCEPTMODE_BREAKALL == g_eExceptionMode)
	{
		//the exception
		PyObject* ptype = PyTuple_GetItem(arg, 0);
		//the argument to raise
		PyObject* pvalue = PyTuple_GetItem(arg, 1);
		//the traceback
		PyObject* ptraceback = PyTuple_GetItem(arg, 2);
		
		PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
		
		PyErr_Display(ptype, pvalue, ptraceback);
		if (ptraceback)
		{
			//get the pointer to the frame held by the bottom traceback object - this
			//should be where the exception occurred.
			tracebackobject* pTb = (tracebackobject*)ptraceback;			
			while (pTb->tb_next != NULL) 
			{
				pTb = pTb->tb_next;
			}
			frame = (PyFrameObject*)PyObject_GetAttr((PyObject*)pTb, PyString_FromString("tb_frame"));
		}
		EnterBreakState(frame, ptraceback);
	}
	else
	{
		//set the exception hook to handle any uncaught exceptions
		PyObject *func = PyCFunction_New(&he_except_method, NULL);
		PySys_SetObject("excepthook", func);
	}
} // HandleExcept



//////////////////////////////////////////////////////////////////////////////
// he_excepthook
//
//
static PyObject *
he_excepthook(PyObject* self, PyObject* args)
{
	PyObject *exc, *value, *tb;
	if (!PyArg_UnpackTuple(args, "excepthook", 3, 3, &exc, &value, &tb))
		return NULL;
	
	PyErr_NormalizeException(&exc, &value, &tb);
	
	PyErr_Display(exc, value, tb);
	
	if (eEXCEPTMODE_IGNORE != g_eExceptionMode)
	{
		assert(tb);
		
		if (tb && (tb != Py_None))
		{
			//get the pointer to the frame held by the bottom traceback object - this
			//should be where the exception occurred.
			tracebackobject* pTb = (tracebackobject*)tb;
			while (pTb->tb_next != NULL) 
			{
				pTb = pTb->tb_next;
			}
			PyFrameObject* frame = (PyFrameObject*)PyObject_GetAttr((PyObject*)pTb, PyString_FromString("tb_frame"));
			EnterBreakState(frame, (PyObject*)pTb);
		}
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}


static int
tracer_callback(ProfilerObject *self, PyFrameObject *frame, int what,
                PyObject *arg)
{
	//if (what != PyTrace_EXCEPTION)
	//	++lineExecutedSinceExcept;

	s_currentRecursionLevel = frame->f_tstate->recursion_depth;

	// Legal values for 'what' are:
	//#define PyTrace_CALL 0
	//#define PyTrace_EXCEPTION 1
	//#define PyTrace_LINE 2
	//#define PyTrace_RETURN 3
	if (what >= 0 && what < NUMELEMENTS(s_executionCounts))
	{
		++s_executionCounts[what];
	}
	else
	{
		assert(!"Invalid what!");
	}

	

	if (g_bBreakNow)
	{
		EnterBreakState(frame);
	}
	else if (what == PyTrace_EXCEPTION)
	{
		HandleExcept(self, frame, what, arg);
	}
	else if (g_nStepFlag == STEPFLAG_GO)
	{
		if (what == PyTrace_LINE)
		{
			ProcessRunStateMessages();

			if (g_bBptsSet)
			{
				const char* cszFilename = PyString_AsString(frame->f_code->co_filename);
				const char* pFileOnly = strrchr(cszFilename, '\\');
				if (pFileOnly)
					cszFilename = pFileOnly+1;
				
				int line = PyCode_Addr2Line(frame->f_code, frame->f_lasti);
				if (BreakBreakpoint(cszFilename, line))
				{
					EnterBreakState(frame);
				}
			}
		}
	}
	else
	{
		assert (g_nStepFlag != STEPFLAG_GO);
			
		switch (what) 
		{
		case PyTrace_LINE:
		{
			assert (g_nStepFlag != STEPFLAG_GO);

			int line = PyCode_Addr2Line(frame->f_code, frame->f_lasti);
			if (g_nStepOverBreakDepth >= frame->f_tstate->recursion_depth)
			{
				if (line != 0)
					EnterBreakState(frame);
			}
			// ALWAYS check breakpoints - even when doing step over.
			//check if there is a breakpoint...
			const char* cszFilename = PyString_AsString(frame->f_code->co_filename);

			if (BreakBreakpoint(cszFilename, line))
			{
				EnterBreakState(frame);
			}
			break;
		}
    
		/*
		case PyTrace_CALL:
			break;

		case PyTrace_RETURN:
			break;
		*/
		default:
			break;
		}
	}

    return 0;

} // tracer_callback




int InitPythonDebug()
{
	//add a main module to python
	g_pMainModule = PyImport_AddModule("__main__");
	if (g_pMainModule == NULL)
		return -1;

	g_pMainDict = PyModule_GetDict(g_pMainModule);
	
	//initialize our output handler functions
	initOutput();

	//set our trace function
	PyEval_SetTrace((Py_tracefunc) tracer_callback, 0);


	PyObject *func = PyCFunction_New(&he_except_method, NULL);
	PySys_SetObject("excepthook", func);
	
	g_nStepFlag = STEPFLAG_GO;

	return 0;

} // InitPythonDebug
