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
// VariablesMsg.cpp: implementation of the CVariablesMsg class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VariablesMsg.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CVariablesMsg::CVariablesMsg()
{

}

CVariablesMsg::~CVariablesMsg()
{

}

bool CVariablesMsg::IsDebugObject(PyObject *pKey)
{
	const char* DebugObjNames[] = 
	{
		"Breakpoint",
		"HeDebugger",
		"currentDebugger",
		"DebugOutput",
		"HeBdb",
		"BdbQuit",
	};
	const char* pKeyName = PyString_AsString(PyObject_Str(pKey));
	for (int i = 0; i < NUMELEMENTS(DebugObjNames); i++)
	{
		if (strcmp(DebugObjNames[i], pKeyName) == 0)
			return true;
	}
	return false;
}










void CVariablesMsg::FormatLocalsDict(PyObject* pLocals)
{
	int pos = 0;
	PyObject *key, *value;
	
	m_msg.resize(0);
	
	while (PyDict_Next(pLocals, &pos, &key, &value)) 
	{
		//dprintf("Local: %s - %s\n", PyString_AsString(PyObject_Str(key)), PyString_AsString(PyObject_Str(PyObject_Type(value))) );

		//leave modules and instances out of the locals display
		if ( key && value && (PyModule_Check(value) == 0) && (PyInstance_Check(value) == 0) )
		{
			//leave out functions and classes
			if (0 == PyObject_Compare((PyObject*)&PyFunction_Type, PyObject_Type(value)) )
				continue;
			if (0 == PyObject_Compare((PyObject*)&PyClass_Type, PyObject_Type(value)) )
				continue;
			if (0 == PyObject_Compare((PyObject*)&PyType_Type, PyObject_Type(value)) )
				continue;
			
			//leave out debugger objects
//			if (IsDebugObject(key))
//				continue;
			
			FormatItem(key, value, 0);
		}
	}
	//add a second terminating null
	if (m_msg.size() > 0)
		m_msg.push_back(0);
}


void CVariablesMsg::FormatGlobalsDict(PyObject* pGlobals)
{
	m_msg.resize(0);

	int pos = 0;
	PyObject *key, *value;
	
	while (PyDict_Next(pGlobals, &pos, &key, &value)) 
	{
		//leave modules and debug objects out of the globals display
		if ( key && value && (PyModule_Check(value) == 0) && (IsDebugObject(key) == false) )
		{	
			//dprintf("Global:  %s - %s\n", PyString_AsString(PyObject_Str(key)), PyString_AsString(PyObject_Str(PyObject_Type(value))) );
			FormatItem(key, value, 0);
		}	
	}
	if (m_msg.size() > 0)
		m_msg.push_back(0);
}





void CVariablesMsg::FormatItem(PyObject* pKey, PyObject* pValue, int IndentLevel)
{
	char* pKeyStr = PyString_AsString(PyObject_Str(pKey));
	if (strcmp("__builtins__", pKeyStr) == 0)
		return;

	assert(pKeyStr);
	PyObject* objType = PyObject_Type(pValue);
	PyObject* valType = PyObject_Type(pKey);
	bool addQuotes = (objType == valType);

	char* pValStr = PyString_AsString(PyObject_Str(pValue));
	assert(pValStr);

	//prefix the key name with "+" to indicate the indentation
/*	for (int i = 0; i < IndentLevel; i++)
	{
		m_msg.push_back('+');
	}
*/	
	//add the key name
	m_msg.insert(m_msg.end(), pKeyStr, pKeyStr+strlen(pKeyStr));
	//sep with a comma
	m_msg.push_back(',');

	//add the value name
	if (addQuotes)
		m_msg.push_back('\"');
	
	if (strlen(pValStr))
		m_msg.insert(m_msg.end(), pValStr, pValStr+strlen(pValStr));
	
	if (addQuotes)
		m_msg.push_back('\"');
	
	//terminate the entry with a 0
	m_msg.push_back(0);


	//handle container types (lists, tuples, dicts)
/*	if (PyList_Check(pValue))
		FormatList(pKeyStr, pValue, IndentLevel+1);
	else if (PyTuple_Check(pValue))
		FormatTuple(pKeyStr, pValue, IndentLevel+1);
	
	else if (PyDict_Check(pValue))
		//FormatDict(pValue, IndentLevel+1);
		;
*/
	
	

	Py_XDECREF(objType);
	Py_XDECREF(valType);
}




void CVariablesMsg::FormatList(const char* ListName, PyObject* pList, int IndentLevel)
{
	char Buff[256];
	strcpy(Buff, ListName);
	char* pNmEnd = Buff+strlen(Buff);

	int ListSize = PyList_Size(pList);
	for (int i = 0; i < ListSize; i++)
	{
		sprintf(pNmEnd, "[%d]", i);

		PyObject* pKey = PyString_FromString(Buff);
		PyObject* pItem = PyList_GetItem(pList, i);
		
		FormatItem(pKey, pItem, IndentLevel);
	}
}

void CVariablesMsg::FormatTuple(const char* TupleName, PyObject* pTuple, int IndentLevel)
{
	char Buff[256];
	strcpy(Buff, TupleName);
	char* pNmEnd = Buff+strlen(Buff);

	int TupSize = PyTuple_Size(pTuple);
	for (int i = 0; i < TupSize; i++)
	{
		sprintf(pNmEnd, "[%d]", i);

		PyObject* pKey = PyString_FromString(pNmEnd);
		PyObject* pItem = PyTuple_GetItem(pTuple, i);
		
		FormatItem(pKey, pItem, IndentLevel);
	}
}