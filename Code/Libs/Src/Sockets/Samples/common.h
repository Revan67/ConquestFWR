#include "netlink.h" // include me before windows.h to get winsock2.h instead of winsock.h

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <windowsx.h>

#include "typedefs.h"

#include "stddat.h"
#include "tempstr.h"

#include "resource.h"

#pragma comment(lib, "sockets.lib")
#pragma comment(lib, "msgbuffer.lib")

#define CONNECT_PORT 15121

#define NAMELEN 64
#define CHATLEN 64

struct modeless_box
{
	modeless_box( HINSTANCE hins, int resid, DLGPROC p ) {
		hwnd= CreateDialog(hins, MAKEINTRESOURCE( resid ), NULL, p);
	}

	~modeless_box( void )
	{
		if (IsWindow(hwnd)) {
			DestroyWindow( hwnd );
			//while (handle_msgs()) ;
		}
	}

	// false: exit condition
	// true: continue with idle processing
	BOOL32 handle_msgs( void )
	{
		MSG msg;
		
		while (PeekMessage(&msg, hwnd, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, hwnd, 0, 0))
				return FALSE;

			if (!IsDialogMessage(hwnd, &msg))
			{
//				TranslateMessage(&msg); 
//				DispatchMessage(&msg);
			}
		}

		if (!IsWindow( hwnd ))
			return FALSE;

		return TRUE;
	}

	HWND hwnd;
};

struct SampleGameDesc : public GameDesc
{
	SampleGameDesc( void )
	{
		addr= NetAddr_TCP( "local", CONNECT_PORT );
		strncpy( name, "SAMPLE", 16 );
		strncpy( desc, "SAMPLE", 16 );
		max_connections= 16;
	}
};

class LocalStr
{
protected:
	enum { LocalStrSize = 1024 };
	char * buffer;
public:
	LocalStr(char *fmt, ...)
	{
		static char buf[LocalStrSize];
		buffer=buf;
		va_list args;
		va_start (args, fmt);
		_vsnprintf (buffer, LocalStrSize, fmt, args);
		va_end (args);
	}

	operator const char * (void) const { return buffer; }
};


