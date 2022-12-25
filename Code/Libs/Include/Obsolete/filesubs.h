//****************************************************************************
//*                                                                          *
//*  FILESUBS.H: Quick-integration interface to DACOM IFileSystem provider   *
//*                                                                          *
//*  32-bit protected-mode source compatible with MSVC 10.2                  *
//*                                                                          *
//*  Version 1.00 of 12-Dec-96: Initial                                      *
//*                                                                          *
//*  Author: John Miles                                                      *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  Copyright (C) 1996 Digital Anvil, Inc.                                  *
//*                                                                          *
//****************************************************************************

#ifndef FILESUBS_H
#define FILESUBS_H

#ifndef TYPEDEFS_H
#include "typedefs.h"
#endif

#ifndef DACOM_H
#include "dacom.h"
#endif

#ifndef FILESYS_H
#include "filesys.h"
#endif

#include <malloc.h>

//
// General file system error codes
//

#define FILE_ERR_NO_ERROR        0
#define FILE_ERR_IO_ERROR        1
#define FILE_ERR_OUT_OF_MEMORY   2
#define FILE_ERR_FILE_NOT_FOUND  3
#define FILE_ERR_CANT_WRITE_FILE 4
#define FILE_ERR_CANT_READ_FILE  5
#define FILE_ERR_DISK_FULL       6

//
// Attribute bitmasks for find_first() / find_next() calls
//

#define FF_DIRECTORY 0x0001      // Directory if set, normal data if clear

class FILESUBS
{
   ICOManager   *DACOM;          // DA COM manager
   IComponentFactory *parent;         // Parent IFileSystem instance
   S32           last_error;     // Last-occurring file error

public:

   //
   // Constructor needs DA COM IFileSystem pointer
   //

   FILESUBS(IComponentFactory *_parent = NULL)
      {
      last_error = FILE_ERR_NO_ERROR;
      parent     = _parent;

      DACOM = DACOM_Acquire();

      if (parent == NULL)
         {
         parent = DACOM;
         }
      }

  ~FILESUBS(void)
      {
      if (DACOM != NULL)
         {
         DACOM->Release();
         DACOM = NULL;
         }
      }

   //
   // Get last file error
   //

   S32 COMAPI error      (void)
      {
      S32 result;

      result = last_error;

      last_error = FILE_ERR_NO_ERROR;

      return result;
      }

   //
   // Return file size in bytes, or -1 on error
   //

   S32 COMAPI size (C8 *filename)
      {
      S32    len;

      DAFILEDESC   desc = filename;
      IFileSystem *file;

      parent->CreateInstance(&desc, (void **) &file);

      if (file == NULL)
         {
         last_error = FILE_ERR_FILE_NOT_FOUND;
         return -1;
         }

      len = file->GetFileSize(NULL, NULL);

      file->Release();

      return len;
      }

   //
   // Read all or part of a file into memory, returning memory location
   // or NULL on error
   //
   // Memory will be allocated if dest==NULL
   //

   void * COMAPI read       (C8     *name   =  NULL,       
                             void   *dest   =  NULL,
                             S32     len    = -1,
                             S32     offset =  0)
      {
      U32    n_bytes;
      U32    nbytes_read;
      S32    result;
      void  *buf;

      //
      // Open file
      //

      DAFILEDESC   desc = name;
      IFileSystem *file;

      parent->CreateInstance(&desc, (void **) &file);

      if (file == NULL)
         {
         last_error = FILE_ERR_FILE_NOT_FOUND;
         return NULL;
         }

      //
      // Set pointer to beginning of range
      //

      if (file->SetFilePointer(NULL,
                               offset,
                               NULL,
                               FILE_BEGIN) == 0xffffffff)
         {
         last_error = FILE_ERR_CANT_READ_FILE;

         file->Release();
         return NULL;
         }

      //
      // Allocate memory for file range
      //

      n_bytes = len;

      if (n_bytes == 0xffffffff)
         {
         n_bytes = file->GetFileSize(NULL, NULL) - offset;
         }

      buf = (dest == NULL) ? malloc(n_bytes) : dest;

      if (buf == NULL)
         {
         last_error = FILE_ERR_OUT_OF_MEMORY;

         file->Release();
         return NULL;
         }

      //
      // Read range
      //

      result = file->ReadFile(NULL,
                              buf,
                              n_bytes,
                             &nbytes_read,
                              NULL);

      file->Release();

      if ((!result) || (nbytes_read != n_bytes))
         {
         if (dest != buf)
            {
            free(buf);
            }

         last_error = FILE_ERR_CANT_READ_FILE;
         return NULL;
         }   

      return buf;
      }

   //
   // Write memory block to file, returning # of bytes written or 0
   // on error
   // 

   S32    COMAPI write      (C8   *filename, 
                             void *buf, 
                             S32   len)
      {
      S32    nbytes;
      S32    result;

      DAFILEDESC   desc = filename;
      IFileSystem *file;

      desc.dwDesiredAccess        = GENERIC_WRITE;
      desc.dwCreationDistribution = CREATE_ALWAYS;

      parent->CreateInstance(&desc, (void **) &file);

      if (file == NULL)
         {
         last_error = FILE_ERR_CANT_WRITE_FILE;
         return 0;
         }

      result = file->WriteFile(NULL,
                               buf,
                               len,
                      (U32 *) &nbytes,
                               NULL);

      file->Release();

      if ((!result) || (nbytes != len))
         {
         last_error = FILE_ERR_DISK_FULL;
         return 0;
         }   

      return (S32) nbytes;
      }

   //
   // Write memory block as portion of file, returning # of bytes written     
   // (either len or 0 if error occurs)                                       
   //                                                                         
   // start_offset = -1 to append to end of file                              
   //

   S32    COMAPI write_range(C8   *filename, 
                             void *buf, 
                             S32   len,
                             S32   start_offset = -1)
      {
      S32    nbytes;
      S32    result;

      DAFILEDESC   desc = filename;
      IFileSystem *file;

      desc.dwDesiredAccess        = GENERIC_WRITE;
      desc.dwCreationDistribution = OPEN_ALWAYS;

      parent->CreateInstance(&desc, (void **) &file);

      if (file == NULL)
         {
         last_error = FILE_ERR_CANT_WRITE_FILE;
         return 0;
         }

      if (start_offset == -1)
         {
         start_offset = file->GetFileSize(NULL, NULL);
         }

      file->SetFilePointer(NULL,
                           start_offset,
                           NULL,
                           FILE_BEGIN);

      result = file->WriteFile(NULL,
                               buf,
                               len,
                      (U32 *) &nbytes,
                               NULL);

      file->Release();

      if ((!result) || (nbytes != len))
         {
         last_error = FILE_ERR_DISK_FULL;
         return 0;
         }   

      return (S32) nbytes;
      }
                                    
   //
   // Copy file 
   //

   S32    COMAPI copy       (C8   *src_pathname,
                             C8   *dest_pathname)
      {
      S32 result;

      if (parent == DACOM)
         {
         result = CopyFile(src_pathname, dest_pathname, FALSE);
         }
      else
         {
         result = ((IFileSystem *) parent)->CopyFile(src_pathname, dest_pathname, FALSE);
         }

      return result;
      }

   //
   // Delete file
   //

   S32    COMAPI unlink     (C8   *filename)
      {
      S32 result;

      if (parent == DACOM)
         {
         result = DeleteFile(filename);
         }
      else
         {
         result = ((IFileSystem *) parent)->DeleteFile(filename);
         }

      return result;
      }

   //
   // Back up file to temporary file
   //
   // Returns TRUE if backup succeeded, FALSE otherwise 
   //
   // *backup_pathname should point to an array of MAX_PATH characters which
   // will receive the name of the backup file for later rollback and/or 
   // deletion.  If the backup process fails, *backup_pathname will receive
   // an empty string, which will be ignored by copy() and 
   // unlink().
   //
   // This function should NOT be used on an open UTF file, as subsequent
   // reversion to the backup may cause loss of synchronization between 
   // the memory-resident UTF directory and the contents of the 
   // backed-up file
   //
   // This function works only with the OS file system, not with a DACOM-based
   // one
   //

   S32    COMAPI create_backup 
                            (C8  *filename,
                             C8  *backup_pathname)
      {
      C8 path[MAX_PATH];
      C8 name[MAX_PATH];

      //
      // Clear returned pathname (assume failure)
      //

      backup_pathname[0] = 0;

      //
      // Get temporary filename for backup
      // 

      if (!GetTempPath(MAX_PATH,
                     path))
         {
         return FALSE;
         }

      if (!GetTempFileName(path,
                           "BAK",
                           0,
                           name))
         {
         return FALSE;
         }

      //
      // Back up file
      //

      if (!CopyFile(filename, name, FALSE))
         {
         return FALSE;
         }

      //
      // Pass backup pathname back to application, and return success
      //

      strcpy(backup_pathname, name);

      return TRUE;
      }

   //
   // Get fully-qualified version of filename, with all volume and path
   // components
   //
   // This function does NOT check to see if the resulting filename is valid,
   // or if the file actually exists.  It will, however, correctly avoid
   // altering filenames which already contain absolute volume and/or
   // path specifiers.
   //
   // This function works only with the OS file system, not with a DACOM-based
   // one

   C8 *   COMAPI qualify_filename
                            (C8  *filename)
      {
      static C8  qualified_filename[256];
             C8 *dummy;

      if (!GetFullPathName(filename,
                           sizeof(qualified_filename),
                           qualified_filename,
                          &dummy))
         {
         //
         // API function failed, return original filename
         //

         strcpy(qualified_filename, filename);
         }

      return qualified_filename;
      }

   //
   // Identify first file matching search pattern in current working 
   // directory, returning a handle which may be used to identify 
   // subsequent files
   //
   // Returns NULL if no qualifying files found
   //
   // Search handle must be closed with find_close() when application is
   // finished searching with it
   //

   HANDLE COMAPI find_first
                            (C8  *search_pattern,
                             C8 **name       = NULL,
                             S32 *size       = NULL,
                             U32 *attributes = NULL)
      {
      HANDLE    handle;
      WIN32_FIND_DATA found;
      static C8       name_buffer[MAX_PATH];

      if (parent == DACOM)
         {
         handle = FindFirstFile(search_pattern,
                               &found);
         }
      else
         {
         handle = ((IFileSystem *) parent)->FindFirstFile(search_pattern,
                                                         &found);
         }

      if (handle == INVALID_HANDLE_VALUE)
         {
         free(handle);
         return NULL;
         }

      if (size != NULL)
         {
         *size = found.nFileSizeLow;
         }

      if (attributes != NULL)
         {
         *attributes = 0;

         if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
            *attributes |= FF_DIRECTORY;
            }
         }

      if (name != NULL)
         {
         strncpy(name_buffer,
                 found.cFileName,
                 sizeof(name_buffer)-1);

         *name = name_buffer;
         }

      return handle;
      }

   //
   // Identify next file matching search pattern originally specified to
   // find_first()
   //
   // Returns FALSE if no qualifying files found
   //

   BOOL32     COMAPI find_next
                            (HANDLE    handle,
                             C8      **name       = NULL,
                             S32      *size       = NULL,
                             U32      *attributes = NULL)
      {
      WIN32_FIND_DATA found;
      static C8       name_buffer[MAX_PATH];

      if ((handle == NULL) || (handle == INVALID_HANDLE_VALUE))
         {
         return FALSE;
         }

      if (parent == DACOM)
         {
         if (!FindNextFile(handle, 
                          &found))
            {
            return FALSE;
            }
         }
      else
         {
         if (!(((IFileSystem *) parent)->FindNextFile(handle, 
                                                     &found)))
            {
            return FALSE;
            }
         }

      if (size != NULL)
         {
         *size = found.nFileSizeLow;
         }

      if (attributes != NULL)
         {
         *attributes = 0;

         if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
            *attributes |= FF_DIRECTORY;
            }
         }

      if (name != NULL)
         {
         strncpy(name_buffer,
                 found.cFileName,
                 sizeof(name_buffer)-1);

         *name = name_buffer;
         }

      return TRUE;
      }

   //
   // Close file search handle
   //

   void      COMAPI find_close (HANDLE handle)
      {
      if ((handle == NULL) || (handle == INVALID_HANDLE_VALUE))
         {
         return;
         }

      if (parent == DACOM)
         {
         FindClose(handle);
         }
      else
         {
         ((IFileSystem *) parent)->FindClose(handle);
         }
      }

   //
   // Create a new directory
   //

   BOOL32     COMAPI create_directory (C8 *name)
                            
      {
      if (parent == DACOM)
         {
         return CreateDirectory(name, NULL);
         }
      else
         {
         return ((IFileSystem *) parent)->CreateDirectory(name, NULL);
         }
      }

   //
   // Remove a directory
   //

   BOOL32    COMAPI remove_directory (C8 *name)
                            
      {
      if (parent == DACOM)
         {
         return RemoveDirectory(name);
         }
      else
         {
         return ((IFileSystem *) parent)->RemoveDirectory(name);
         }
      }

   //
   // Get pathname of current working directory
   //

   BOOL32    COMAPI get_current_directory
                            (U32       buffer_length,
                             C8       *name_buffer)
      {
      if (parent == DACOM)
         {
         return GetCurrentDirectory(buffer_length, 
                                    name_buffer);
         }
      else
         {
         return ((IFileSystem *) parent)->GetCurrentDirectory(buffer_length, 
                                                              name_buffer);
         }
      }

   //
   // Set new current working directory
   //

   BOOL32    COMAPI set_current_directory
                            (C8       *name)
      {
      if (parent == DACOM)
         {
         return SetCurrentDirectory(name);
         }
      else
         {
         return ((IFileSystem *) parent)->SetCurrentDirectory(name);
         }
      }
};

#endif
