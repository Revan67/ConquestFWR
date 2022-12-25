//////////////////////////////////////////////////////////////////////////////
//  File:    StdAfx.h
//
//  Purpose: include file for standard system include files,
//  or project specific include files that are used frequently, but
//  are changed infrequently
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


#if !defined(AFX_STDAFX_H__F7EB9042_7381_4C28_8599_7E016F99A232__INCLUDED_)
#define AFX_STDAFX_H__F7EB9042_7381_4C28_8599_7E016F99A232__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#pragma warning(disable:4786)


// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>




//this is a little switch file that allows me to easily exclude some
//useful Humongous owned code when I'm compiling an external release
#include "ossinclude/ossinclude.h"
#ifdef OSS_RELEASE
	using namespace HumongousOpenSource;

	//include all the python stuff we'll need
	#include <python.h>
	#include <compile.h>
	#include <eval.h>
	#include <frameobject.h>
	#include <structmember.h>

#else
	#include <hecore/hecore.h>
	#include <hecore/threadcore.h>
	using namespace he;

	//include all the python stuff we'll need
	#include <python/include/python.h>
	#include <python/include/compile.h>
	#include <python/include/eval.h>
	#include <python/include/frameobject.h>
	#include <python/include/structmember.h>

#endif

//we use these a lot so lets make sure they're everywhere.
#include <string>
#include <vector>
#include <set>
using namespace std;


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__F7EB9042_7381_4C28_8599_7E016F99A232__INCLUDED_)
