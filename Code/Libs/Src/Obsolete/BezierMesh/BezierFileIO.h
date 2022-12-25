//$Header: /Libs/Dev/Src/RendComp/BezierMesh/BezierFileIO.h 1     9/03/99 6:41p Mstembera $

#include <Windows.h>
#include "FileSys.h"
#include <malloc.h>
//****************************************************************************
//*                                                                          *
//*  Read all or part of a file into memory, returning memory location       *
//*  or NULL on error                                                        *
//*                                                                          *
//*  Memory will be allocated if dest==NULL                                  *
//*                                                                          *
//****************************************************************************

static void * read (IFileSystem  *parent, //)     
                    const C8     *filename, 
                    void         *dest,
                    S32           len,
                    S32           start_offset)

{
   U32    n_bytes;
   U32    nbytes_read;
   S32    result;
   void  *buf;

   //
   // Open file
   //

   DAFILEDESC   desc (filename);

	HANDLE file;

   file = parent->OpenChild (&desc);

   if (file == INVALID_HANDLE_VALUE)
   {
      return NULL;
   }

   //
   // Set pointer to beginning of range
   //

   if (parent->SetFilePointer(file,
                            start_offset,
                            NULL,
                            FILE_BEGIN) == 0xffffffff)
   {
      parent->CloseHandle (file);
      return NULL;
   }

   //
   // Allocate memory for file range
   //

   n_bytes = len;

   if (n_bytes == 0xffffffff)
   {
      n_bytes = parent->GetFileSize(file, NULL) - start_offset;
   }

   buf = (dest == NULL) ? malloc(n_bytes) : dest;

   if (buf == NULL)
   {
      parent->CloseHandle (file);
      return NULL;
   }

   //
   // Read range
   //

   result = parent->ReadFile(file,
                           buf,
                           n_bytes,
                          &nbytes_read,
                           NULL);

   parent->CloseHandle (file);

   if ((!result) || (nbytes_read != n_bytes))
   {
      if (dest != buf)
      {
         free(buf);
      }

      return NULL;
   }

   return buf;
}

//****************************************************************************
//*                                                                          *
//*  Read file and store contents at pointer                                 *
//*                                                                          *
//****************************************************************************

S32 read_val(IFileSystem *parent, const C8 *filename, void *dest)
{
   return (read(parent, filename, dest, -1, 0) != NULL);
}

//****************************************************************************
//*                                                                          *
//*  Allocate memory, read file, and store location at pointer               *
//*                                                                          *
//****************************************************************************

S32 read_ptr(IFileSystem *parent, C8 *filename, void *dest)
{
   void **destination = (void **) dest;

   *destination = (S32 *) read(parent, filename, NULL, -1, 0);

   return (*destination != NULL);
}

//****************************************************************************
//*                                                                          *
//*  Enter subdirectory, returning child file system on success              *
//*                                                                          *
//****************************************************************************

GENRESULT descend_read (IComponentFactory *parent, const C8 *directory_name, IFileSystem ** file)
{
   DAFILEDESC   desc = directory_name;

   desc.lpImplementation = "DOS";		// optimization -- assume no UTF's within UTF

   return parent->CreateInstance(&desc, (void **) file);
}
