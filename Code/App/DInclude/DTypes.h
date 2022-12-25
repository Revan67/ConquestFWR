#ifndef DTYPES_H
#define DTYPES_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DTypes.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $

    $Header: /Conquest/App/DInclude/DTypes.h 13    9/07/00 6:35p Rmarr $
*/			    
//--------------------------------------------------------------------------//

#ifndef US_TYPEDEFS
#include "typedefs.h"
#endif

#define M_MAX_STRING	32

#ifdef _ADB

class Vector
{
	SINGLE x,y,z;
};

class Matrix
{
	SINGLE d[3][3];
};

class Transform	: Matrix	// data version of transform class
{
	Vector translation;
};

struct GRIDVECTOR
{
	U8 x, y;
};

struct NETGRIDVECTOR
{
	U8 x, y;
	U8 systemID;
};


typedef U32 NETVECTOR;

typedef Transform TRANSFORM;
typedef long LONG;

typedef struct tagRECT
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} RECT;

typedef char M_STRING[M_MAX_STRING];
typedef unsigned short wchar_t;
typedef wchar_t M_STRINGW[M_MAX_STRING];

#else

#ifndef _3DMATH_H
#include <3dmath.h>
#endif

#ifndef GRIDVECTOR_H
#include "GridVector.h"
#endif

#include <string.h>

struct M_STRING
{
	char string[M_MAX_STRING];

	void operator=(const char* str) { strncpy(string, str ? str : "", M_MAX_STRING); }
	operator char*() { return string; }
	operator const char*() const { return string; }

	BOOL32 IsNull() const { return *string == '\0'; }

	bool operator==(const M_STRING& other) const { return strncmp(string, other.string, M_MAX_STRING) == 0; }
	bool operator!=(const M_STRING& other) const { return strncmp(string, other.string, M_MAX_STRING) != 0; }
};

struct M_STRINGW
{
	wchar_t string[M_MAX_STRING];

	void operator=(const wchar_t* str) { wcsncpy(string, str ? str : L"", M_MAX_STRING); }
	operator wchar_t*() { return string; }
	operator const wchar_t* () const { return string; }

	BOOL32 IsNull() const { return *string == 0; }

	bool operator==(const M_STRINGW& other) const { return wcsncmp(string, other.string, M_MAX_STRING) == 0; }
	bool operator!=(const M_STRINGW& other) const { return wcsncmp(string, other.string, M_MAX_STRING) != 0; }
};

#endif	// _ADB

#endif
