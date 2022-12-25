//---------------------------------------------------------------------------
/*
	PROJECT.H

	(Win32) Test Program (c) 1997 Digital Anvil

	02-10-97 created (pci)

	$Header: /Tools/TxmView/project.h 1     5/06/99 12:59p Pisaac $
*/
//---------------------------------------------------------------------------

#ifndef PROJECT_H__
#define PROJECT_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

#include <stdlib.h>		// exit()
#include <stdio.h>		// sprintf()
#include <assert.h>		// assert()

#include "mmsystem.h"

#define ASSERT assert

#include "dacom.h"

typedef	unsigned char	byte;
typedef	unsigned int	uint;

extern void DebugPrint (char *fmt, ...);

extern HINSTANCE AppInstance;
//extern HWND AppWindow;
extern HMODULE AppResource;

#define INTVEC(v) int(v.x),int(v.y),int(v.z)
#define COLORVEC(c) c.r,c.g,c.b

//--

#define IDC_TREEVIEW	40100
#define IDC_STATUS_BAR	40101

// (MFC) AFXRES.H - File commands
#define ID_FILE_NEW				0xE100
#define ID_FILE_OPEN			0xE101
#define ID_FILE_CLOSE			0xE102
#define ID_FILE_SAVE			0xE103
#define ID_FILE_SAVE_AS			0xE104
#define ID_FILE_PAGE_SETUP		0xE105
#define ID_FILE_PRINT_SETUP		0xE106
#define ID_FILE_PRINT			0xE107

//--

#pragma warning(disable:4305)	// 'initializing' : truncation from 'const double' to 'float'
#pragma warning(disable:4244)

// COLOR

struct RGB24
{
	unsigned char r,g,b;
};

#include "vfx.h"	// VFX_RGB, PANE, VFX_RECT

// MATH

#include "3dmath.h"

#define MUL_RAD_TO_DEG (1.0/MUL_DEG_TO_RAD)

#define MIN_FLOAT	0.001
#define MIN_SPEED	0.010
#define MIN_FORCE	0.100

//---------------------------------------------------------------------------
// Warning
//---------------------------------------------------------------------------

inline void Warning (char *fmt, ...)
{
	char buffer[256];
	va_list marker;
	va_start(marker,fmt);
	vsprintf(buffer,fmt,marker);
	va_end(marker);

	char msg[256];
//	String name(fmt);
	sprintf(msg,"Warning: %s\n",buffer);
	DebugPrint(msg);
}

//---------------------------------------------------------------------------
#endif