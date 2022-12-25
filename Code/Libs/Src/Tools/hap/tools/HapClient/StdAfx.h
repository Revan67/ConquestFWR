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

#if !defined(AFX_STDAFX_H__A0EB7F73_2D28_4816_9B90_B2A9BED8C5B1__INCLUDED_)
#define AFX_STDAFX_H__A0EB7F73_2D28_4816_9B90_B2A9BED8C5B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning(disable:4786)


#define WIN32_LEAN_AND_MEAN
#include <windows.h>



#include "ossinclude/ossinclude.h"
#ifdef OSS_RELEASE
	using namespace HumongousOpenSource;

	#ifdef _DEBUG              // Added 10-16-02 Bruce Dickey
	#undef _DEBUG
	#include "python.h"
	#define _DEBUG
	#else
	#include "python.h"
	#endif

#else
	#include <hecore/hecore.h>
	#include <hecore/threadcore.h>
	using namespace he;
	
	#ifdef _DEBUG              // Added 10-16-02 Bruce Dickey
	#undef _DEBUG
	#include <python/include/python.h>
	#define _DEBUG
	#else
	#include <python/include/python.h>
	#endif
#endif


#include <string>
#include <vector>
#include <set>
using namespace std;


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A0EB7F73_2D28_4816_9B90_B2A9BED8C5B1__INCLUDED_)
