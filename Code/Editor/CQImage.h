#ifndef CQIMAGE_H
#define CQIMAGE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               CQImage.h                                  //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/CQImage.h 8     9/29/00 9:48a Jasony $

*/
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

//#undef CQIMEXTERN
//#ifdef BUILD_TRIM
//#define CQIMEXTERN __declspec(dllexport)
//#else
//#define CQIMEXTERN __declspec(dllimport)
//#endif

#define CQIMEXTERN

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ICQImage : IDAComponent
{
	virtual void LoadSymTable (HINSTANCE hInstance) = 0;

	virtual void UnloadSymTable (HINSTANCE hInstance) = 0;

	virtual void Report (const char * szInfo) = 0;

	virtual void MemoryReport (struct IHeap * heap) = 0;

CQIMEXTERN static bool __stdcall Assert (const char *exp, void *file, unsigned line);

CQIMEXTERN static bool __cdecl Bomb (const char *exp, ...);

CQIMEXTERN static bool __cdecl Error (const char *exp, ...);

CQIMEXTERN static int __stdcall Exception (struct _EXCEPTION_POINTERS * exceptionInfo);

CQIMEXTERN static int __cdecl STANDARD_DUMP (struct ErrorCode code, const C8 *fmt, ...);

};
//--------------------------------------------------------------------------//
//----------------------------End CQImage.h---------------------------------//
//--------------------------------------------------------------------------//
#endif