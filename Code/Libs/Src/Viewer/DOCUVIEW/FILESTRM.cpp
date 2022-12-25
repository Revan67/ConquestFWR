//--------------------------------------------------------------------------//
//                                                                          //
//                               FileStrm.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 2002 Fever Pitch Studios, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $
*/			    
//--------------------------------------------------------------------------//

/*
	FILESTREAM CLASS:
		Meant to provide faster routines for getchar() and peekchar(), 
		by reading the whole file in at one time.
*/

// 4514: unused inline function
// 4201: nonstandard no-name structure use
// 4100: formal parameter was not used
// 4512: assignment operator could not be generated
// 4245: conversion from signed to unsigned
// 4127: constant condition expression
// 4355: 'this' used in member initializer
// 4244: conversion from int to unsigned char, possible loss of data
// 4200: zero sized struct member
// 4710: inline function not expanded
// 4702: unreachable code
// 4786: truncating function name (255 chars) in browser info
#pragma warning (disable : 4514 4201 4100 4512 4245 4127 4355 4244 4710 4702 4786)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning (disable : 4355 4201)

#include "FileSys.h"
#include "filestrm.h"
#include "ferror.h"

//#include <string.h>
#include <malloc.h>


extern ICOManager * DACOM;

//--------------------------------------------------------------------------//
//--------------------------STATICS-----------------------------------------//

//------------------------------METHODS--------------------------------------//

FileStream::FileStream(void)
{
	buffer = 0;
	buffer_index = 0;
	bOwnMemory=false;
}

//--------------------------------------------------------------------------//

int FileStream::open(const char *filename, char fatal)
{
	LPFILESYSTEM pFile;
	DAFILEDESC desc = filename;
	int result;
	DWORD dwRead;

	if ((result = DACOM->CreateInstance(&desc, (void**) &pFile)) == GR_OK)
	{
		unsigned char *tmp;

		if (bOwnMemory)
			free((void *)buffer);
		buffer_index = 0;
		tmp = (BYTE *) malloc(pFile->GetFileSize()+1);
		buffer = tmp;
		bOwnMemory = true;
		pFile->ReadFile(0, tmp, pFile->GetFileSize(), &dwRead, 0);
		tmp[pFile->GetFileSize()] = 0;	// mark last byte as EOF
		
		pFile->Release();
	}
	return (result == GR_OK);
}
//--------------------------------------------------------------------------//

void FileStream::close(void)
{
	if (bOwnMemory && buffer)
		free((void *)buffer);

	buffer = 0;
	buffer_index = 0;
	bOwnMemory = false;
}

//--------------------------------------------------------------------------//
#ifdef USING_CC
	
	UBYTE FileStream::get_char(void)
	{
		if (buffer[buffer_index])	
			return buffer[buffer_index++];
		return 0;
	}

	UBYTE FileStream::peek_char(void)
	{
		return buffer[buffer_index];
	}

	UBYTE FileStream::peek2_char(void)
	{
		if (buffer[buffer_index])
			return buffer[buffer_index + 1];
		return 0;
	}

	char *FileStream::get_current_filename(void)
	{
		return latest_filename;
	}
#endif

//--------------------------------------------------------------------------//
//----------------------------End FileStrm.cpp------------------------------//
//--------------------------------------------------------------------------//


