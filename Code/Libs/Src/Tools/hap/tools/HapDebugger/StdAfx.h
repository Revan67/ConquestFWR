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

#if !defined(AFX_STDAFX_H__1D689CB8_8275_4F28_A3B6_D3D4EF02739C__INCLUDED_)
#define AFX_STDAFX_H__1D689CB8_8275_4F28_A3B6_D3D4EF02739C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning(disable:4786)

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxsock.h>

//#include <python/include/python.h>

//include the header file containing useful macros and diagnostic stubs
#include <ossinclude/ossinclude.h>

//defined (or not) in OssInclude.h
#ifdef OSS_RELEASE
//import the open source namespace
using namespace HumongousOpenSource;
//include and link to the expat parser
#include "expat/expat.h"
#ifdef _DEBUG
#pragma comment(lib,"expat_d.lib")
#else
#pragma comment(lib,"expat.lib")
#endif

//link to python using the generic path
//#include <python.h>

#else
//include humongous diagnostics core library
#include <hecore/hecore.h>
#include <hecore/threadcore.h>
#include <hecore/hestring.h>
using namespace he;

//link to python using the he standard relative path
//#include <python/include/python.h>

#endif

//basic STL includes
#include <vector>
#include <map>
using namespace std;

#ifdef _PLATFORM_SDK
#include <Htmlhelp.h>
//link to the lib that makes it work
#pragma comment(lib, "htmlhelp.lib")


//this is needed to support GetProcessMemoryInfo and GetModuleFileNameEx
#include <Psapi.h>
#pragma comment(lib, "psapi.lib")


#else

	#if !defined(LVS_EX_LABELTIP)
		const int LVS_EX_LABELTIP = 0x00004000; // Added 10-15-02 Bruce Dickey
	#endif
	
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__1D689CB8_8275_4F28_A3B6_D3D4EF02739C__INCLUDED_)
