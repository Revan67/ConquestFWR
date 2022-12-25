// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <time.h>
#include <commdlg.h>

extern "C" {
#include "lua.h"
};

#ifndef NO_BUGSLAYER
#include "BugslayerUtil.h"
#endif


#include "DACOM.h"
#include "FDUMP.h"

#include "DarthTest.h"
#include "resource.h"

#ifdef _WINNT_
#define UNICODE
#endif

#ifdef UNICODE
#define tcscpy_check( src, maxoutlen )						
#define tcscpy_ansi2os( tchar_dst, ansi_src, maxoutlen )	strcpy( tchar_dst, ansi_src )
#define tcscpy_os2ansi( ansi_dst, tchar_src, maxoutlen )	strcpy( ansi_dst, tchar_src )
#else
#define  tcscpy_check( src, maxoutlen )						if( !(_tcslen(src) < maxoutlen) ) { fprintf( stderr, "length exceeded\n" ); return; }
#define tcscpy_ansi2os( tchar_dst, ansi_src, maxoutlen )	MultiByteToWideChar( CP_ACP, 0, ansi_src, strlen(ansi_src), tchar_dst, maxoutlen )
#define tcscpy_os2ansi( ansi_dst, tchar_src, maxoutlen )	WideCharToMultiByte( CP_ACP, 0, tchar_src, _tcslen(tchar_src), anis_dst, maxoutlen, NULL, NULL )
#endif

#include "DARTHTESTINFO.h"
#include "darth_lua_helpers.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
