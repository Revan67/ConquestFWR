//
// prepstub.h
//


#ifndef __PREPSTUB_H
#define __PREPSTUB_H

#include <windows.h>
#include <windowsx.h>
#include <winver.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "myassert.h"
#include "script.h"

typedef char *			HPSTR;
typedef void *			HPVOID;


#define INI_FILE			"PREPSTUB.INI"
#define INI_SECTION			"PrepStub"
#define INI_ENTRY_INPUT		"InputFile"
#define INI_ENTRY_OUTPUT	"OutputFile"
#define INI_ENTRY_STUBPATH	"StubPath"
#define INI_ENTRY_AUTONUM	"AutoNum"
#define INI_ENTRY_BINARY	"Binary"
#define INI_ENTRY_DISKPATHS	"DiskPaths"



extern HINSTANCE 	g_hInst;
extern HWND			g_hwnd;
extern char			g_szAppTitle[256];
extern BOOL			g_bDiskPaths;
extern int			g_nCurrentDiskID;

#endif //__PREPSTUB_H
