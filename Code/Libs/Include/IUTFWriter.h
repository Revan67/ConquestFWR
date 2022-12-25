#ifndef IUTFWRITER_H
#define IUTFWRITER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IUTFWriter.h                                //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/Libs/Include/IUTFWriter.h 5     4/28/00 11:57p Rmarr $
*/

/*
	Implemented by FileSystem's that can write UTF files.
*/
//--------------------------------------------------------------------------//
//

#ifndef DACOM_H
#include "DACOM.h"
#endif


//--------------------------------------------------------------------------//
//
#define IID_IUTFWriter MAKE_IID("IUTFWriter",1)

struct DACOM_NO_VTABLE IUTFWriter : public IDAComponent
{
	// Flush extraneous data to the disk
	virtual void __stdcall Flush (void) = 0;

	// add space in the directory table for 'n' number of entries.
	// a "directory entry" can be used for a directory or a file.
	virtual void __stdcall AddDirEntries (U32 numEntries) = 0;

	// add 'n' bytes of space in the names table.
	// pre-allocating space in the table is faster than allocating "as needed."
	virtual void __stdcall AddNameSpace (U32 numBytes) = 0;

	// extra directory entries to add when the table needs to grow. Default = 0
	// Higher value results in faster table generation, but may result in unused directory entries,
	// and thus a larger header than actually needed.
	// FYI: A directory entry takes about 50 bytes of storage.
	virtual void __stdcall HintExpansionDirEntries (U32 numEntries) = 0;

	// extra bytes to add to the "names" table, when it needs to grow. Default = 0.
	// Higher value results in faster table generation, but may result in unused name space,
	// and thus a larger header than actually needed.
	// FYI: The names table is a series of ASCIIZ strings, terminated by two NULL characters.
	virtual void __stdcall HintExpansionNameSpace (U32 numBytes) = 0;
};


#endif