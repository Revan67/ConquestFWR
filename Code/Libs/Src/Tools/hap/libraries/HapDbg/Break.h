//////////////////////////////////////////////////////////////////////////////
//  File:    Break.h
//
//  Purpose: Defines the state the debugger is in while python is broken - 
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

#ifndef _BREAK_H_
#define _BREAK_H_


int EnterBreakState(PyFrameObject *frame, PyObject* pTraceback = 0);
void SendLocalsAndGlobals(PyObject *pLocals, PyObject *pGlobals);

#endif //_BREAK_H_