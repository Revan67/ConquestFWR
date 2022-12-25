//--------------------------------------------------------------------------//
//                                                                          //
//                            StrStuff.cpp                                  //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/DrawAgent16.cpp 10    11/25/98 2:09p Rmarr $

	
   Routines for handling string resources and other string related stuff.
*/
//--------------------------------------------------------------------------//

//
// Include files
//

#include <windows.h>

#include <typedefs.h>

#include "strstuff.h"

//
// Constants
//

//
// Routines
//

//--------------------------------------------------------------------------//
//
int __stdcall _localAnsiToWide (const char * input, wchar_t * output, U32 bufferSize)
{
		// 932 is Japanese code page
	return MultiByteToWideChar(CP_ACP, 0, input, -1, output, (bufferSize/sizeof(output[0])) );
}
//--------------------------------------------------------------------------//
//
int __stdcall _localWideToAnsi (const wchar_t * input, char * output, U32 bufferSize)
{
		// 932 is Japanese code page
	return WideCharToMultiByte(CP_ACP, 0, input, -1, output, bufferSize, 0, 0);
}
//--------------------------------------------------------------------------//
//
const char * __cdecl _localLoadString (HINSTANCE hResource, U32 dwID)
{
	static char buffer[MAX_STRING];

	buffer[0] = 0;
	LoadString(hResource, dwID, buffer, sizeof(buffer)-1);

	return buffer;
}
//--------------------------------------------------------------------------//
//
const wchar_t * __cdecl _localLoadStringW (HINSTANCE hResource, U32 dwID)
{
	static wchar_t buffer[MAX_STRING];

	buffer[0] = 0;
	if (LoadStringW(hResource, dwID, buffer, 255) == 0)		// assume it's not implemented
	{
		const char * src = _localLoadString(hResource, dwID);
		
		_localAnsiToWide(src, buffer, sizeof(buffer));
	}

	return buffer;
}
