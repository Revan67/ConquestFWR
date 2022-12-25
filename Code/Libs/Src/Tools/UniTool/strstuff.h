#ifndef STRSTUFF_H
#define STRSTUFF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                            StrStuff.cpp                                  //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/DrawAgent16.cpp 10    11/25/98 2:09p Rmarr $

	
   Routines for handling resource strings, Unicode, and other string related
   stuff.
*/
//--------------------------------------------------------------------------//

//
// Constants
//

const int MAX_STRING = 256;  // the max string length handled by these routines

//
// Global functions
//

extern int __stdcall _localAnsiToWide (const char * input, wchar_t * output, U32 bufferSize);
extern int __stdcall _localWideToAnsi (const wchar_t * input, char * output, U32 bufferSize);
extern const char * __cdecl _localLoadString (HINSTANCE hResInst, U32 dwID);
extern const wchar_t * __cdecl _localLoadStringW (HINSTANCE hResInst, U32 dwID);

#endif
