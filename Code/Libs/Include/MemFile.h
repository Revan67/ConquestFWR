#ifndef MEMFILE_H
#define MEMFILE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              MemFile.h                                   //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/Libs/Include/MemFile.h 5     4/28/00 11:57p Rmarr $
*/

/*
	MemoryFile, an IFileSystem implementation that reads from memory.
*/
//--------------------------------------------------------------------------//

#ifndef FILESYS_H
#include "FileSys.h"
#endif

#ifndef TSMARTPOINTER_H
#include "TSmartPointer.h"
#endif

//--------------------------------------------------------------------------//
// flags for MEMFILEDESC
//--------------------------------------------------------------------------//
#define CMF_DONT_COPY_MEMORY   0x00000001		// use the memory directly, dont allocate another copy of buffer
#define CMF_OWN_MEMORY		   0x00000002		// take ownership of memory buffer, (free memory on destruction)
//EMAURER  in order to use CMF_OWN_MEMORY correctly, the buffer being handed off 
//must be allocated from the heap that is used by the MemFile implementation.

//
//--------------------------------------------------------------------------//
//
struct MEMFILEDESC : public DAFILEDESC
{
	PVOID		lpBuffer;
	DWORD		dwBufferSize;
	DWORD		dwFlags;

	MEMFILEDESC (const C8 *_file_name = NULL, const C8 *_interface_name = "IFileSystem") : DAFILEDESC(_file_name, _interface_name)
	{
		memset(((C8 *)this)+sizeof(DAFILEDESC), 0, sizeof(*this)-sizeof(DAFILEDESC));
		size = sizeof(*this);
		lpImplementation = "MEM";
	}
};

//--------------------------------------------------------------------------//
//-------------Helper functions---------------------------------------------//
//--------------------------------------------------------------------------//


inline GENRESULT CreateUTFMemoryFile (MEMFILEDESC & desc, struct IFileSystem ** file)
{
	GENRESULT result;
	COMPTR<IFileSystem> memfile;

	if ((result = DACOM_Acquire()->CreateInstance(&desc, memfile)) == GR_OK)
	{
		DAFILEDESC fdesc;

		fdesc = desc;
        fdesc.size = sizeof(fdesc);
		fdesc.lpImplementation = "UTF";
		fdesc.lpParent = memfile;
		memfile->AddRef();

		if ((result = DACOM_Acquire()->CreateInstance(&fdesc, (void **) file)) != GR_OK)
			memfile->Release();	// release the extra reference
	}

	return result;
}



#endif
