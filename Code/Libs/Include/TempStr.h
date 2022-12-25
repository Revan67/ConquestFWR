#ifndef TEMPSTR_H
#define TEMPSTR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              TempStr.h                                   //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Conquest/Libs/Include/TempStr.h 5     4/28/00 11:57p Rmarr $
*/			    
//---------------------------------------------------------------------------
//
// Design Notes:
//     This header provides a simple class for building temporary strings.
// It is used by the libraries for building trace output.
//
// WARNING: The string buffer is of limited size. Beware of buffer overflows.

//
// Include files
//

#ifndef _WINDOWS_
#error Windows.h required for this to compile!
#endif

//
// Constants
//
const int TempStrSize = 8 * 1024;

//
// Class and structure definitions
//

class TempStr
{
protected:
	char buffer[TempStrSize];

public:
	TempStr() {buffer[0] = '\0';}
	TempStr(char *fmt, ...)
	{
		va_list args;
		va_start (args, fmt);
		set (fmt, args);
		va_end (args);
	}

	void set (char *fmt, ...)
	{
		va_list args;
		va_start (args, fmt);
		set (fmt, args);
		va_end (args);
	}

	void set (char *fmt, va_list args)
	{
		wvsprintf (buffer, fmt, args);
	}

	operator char * (void) { return buffer; }
};

// This macro guarantees that the cast occurs when constructing a temp string in a variable
// argument list.
#define TEMPSTR (char *) TempStr

#endif
