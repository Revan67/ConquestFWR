//////////////////////////////////////////////////////////////////////////////
//  File:    HeDbg.h
//
//  Purpose: defines a function which intializes the python debugging.
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

#ifndef _HEDBG_H_
#define _HEDBG_H_


//////////////////////////////////////////////////////////////////////////////	
// traceback structure definition.  This structure is used by hapdbg to 
// correctly handle exceptions.  The structure is actually defined in traceback.cpp
typedef struct _tracebackobject {
	PyObject_HEAD
	struct _tracebackobject *tb_next;
	PyFrameObject *tb_frame;
	int tb_lasti;
	int tb_lineno;
} tracebackobject;


int InitPythonDebug();


#endif //_HEDBG_H_