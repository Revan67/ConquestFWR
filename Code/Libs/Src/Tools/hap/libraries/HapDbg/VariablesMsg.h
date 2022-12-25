// VariablesMsg.h: interface for the CVariablesMsg class.
//
//////////////////////////////////////////////////////////////////////
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

#if !defined(AFX_VARIABLESMSG_H__3898158A_4647_42ED_B0C8_9D911CDB2FF2__INCLUDED_)
#define AFX_VARIABLESMSG_H__3898158A_4647_42ED_B0C8_9D911CDB2FF2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CVariablesMsg  
{
public:
	CVariablesMsg();
	virtual ~CVariablesMsg();

	void FormatLocalsDict(PyObject* pLocals);
	void FormatGlobalsDict(PyObject* pGlobals);
	

	int MsgSize()
	{
		return m_msg.size();
	}
	
	const char* GetMsg()
	{
		return &(m_msg[0]);
	}

private:
	void FormatItem(PyObject* pKey, PyObject* pValue, int IndentLevel);
	void FormatList(const char* ListName, PyObject* pList, int IndentLevel);
	void FormatTuple(const char* TupleName, PyObject* pTuple, int IndentLevel);
	static bool IsDebugObject(PyObject *pKey);
private:
	vector<char> m_msg;	

};

#endif // !defined(AFX_VARIABLESMSG_H__3898158A_4647_42ED_B0C8_9D911CDB2FF2__INCLUDED_)
