//--------------------------------------------------------------------------//
//                                                                          //
//                               FERROR.CPP                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.              //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author:   JYENAWINE  $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include <windows.h>
#include <stdarg.h>
#include <stdlib.h>

#include "ferror.h"

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
//static char InFatal = 0;
char bFatalHappened = 0;
char szAppName[40] = "szAppName[] (ferror.h) is not set";
char szRecName[64];
HINSTANCE hInstance = 0;
HWND hMainWindow=0;
//--------------------------------------------------------------------------//
//
void Fatal (char *fmt, ...)
{
	char buffer[256];

/*
    if (InFatal)
    {
        if (MessageBox(hMainWindow, "Re-entered Fatal. Continue?", szAppName, MB_ICONEXCLAMATION | MB_YESNO | MB_SETFOREGROUND) != IDYES)
			_exit(1);
		else
			return;
	}
*/
	va_list ap;
	va_start(ap, fmt);
	wvsprintf (buffer, fmt, ap);
	va_end(ap);

	MessageBox(hMainWindow, buffer, szAppName, MB_ICONSTOP | MB_OK | MB_SETFOREGROUND);
	bFatalHappened = 1;
//	InFatal = 1;
//	PostQuitMessage (0);
//	::exit(1);
}

//------------------------------------------------------------------
//
void FatalwID(long dwStringResource)
{
	char msg[64];

	if (LoadString(hInstance, dwStringResource, msg, 64) == 0)
	{
		Fatal("Unknown error.");
		return;
	}
	
	Fatal(msg);
}
//------------------------------------------------------------------
//
void WarningBox(HWND hwnd, long dwStringResource, ...)
{
	char msg[64];
	char buffer[256];

	if (LoadString(hInstance, dwStringResource, msg, 64))
	{
		va_list ap;
		va_start(ap, dwStringResource);
		wvsprintf (buffer, msg, ap);
		va_end(ap);

		MessageBox(hwnd, buffer, szAppName, MB_ICONEXCLAMATION | MB_OK | MB_SETFOREGROUND);
	}
}
//------------------------------------------------------------------
//--------------------END FERROR.CPP--------------------------------
//------------------------------------------------------------------












