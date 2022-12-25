#include "dacom.h"
#include "audiofile.h"
#include "fdump.h"

//****************************************************************************
//*                                                                          *
//*                                                                          *
//****************************************************************************

AudioBuffer::AudioBuffer(AudioFile* file_arg,
								 U32 sample_offset_arg,
								 U32 sample_size) :
	file(file_arg),
	sample_offset(sample_offset_arg)
{
	ASSERT(file);

	sample_end = sample_offset + sample_size;

	if (file->file_data)
	{
		allocated_size = 0;

		buffer_data[0] = (char*)file->file_data + sample_offset;
		buffer_size[0] = sample_size;
		buffer_data[1] = buffer_data[0];
		buffer_size[1] = buffer_size[0];
	}
	else
	{
		allocated_size = 64 * 1024;

		buffer_data[0] = new char[allocated_size];
		buffer_size[0] = 0;
		buffer_data[1] = new char[allocated_size];
		buffer_size[1] = 0;
	}
	toggle = 0;
	file_offset = sample_offset;
}

//****************************************************************************
//*                                                                          *
//*                                                                          *
//****************************************************************************

AudioBuffer::~AudioBuffer()
{
	if (allocated_size > 0)
	{
		delete[] buffer_data[0];
		delete[] buffer_data[1];
	}
}

//****************************************************************************
//*                                                                          *
//*                                                                          *
//****************************************************************************

bool AudioBuffer::fetch(U32 offset)
{
	file_offset = offset + sample_offset;
	return fetch();
}

//****************************************************************************
//*                                                                          *
//*                                                                          *
//****************************************************************************

bool AudioBuffer::fetch()
{
	//
	// if allocated size is zero, then the file was already prefetched
	//
	bool fetched= allocated_size == 0;
	
	if (!fetched) 
	{
		if (file_offset >= sample_end)
		{
			file_offset = sample_offset;
		}
	
		toggle = toggle ? 0 : 1;
	
		if (file_offset + allocated_size > sample_end)
		{
			buffer_size[toggle] = sample_end - file_offset;
		}
		else
		{
			buffer_size[toggle] = allocated_size;
		}

		fetched = file->fetch(buffer_data[toggle], file_offset, buffer_size[toggle]);

		if (fetched)
		{
			file_offset += buffer_size[toggle];
		}
	}

	return fetched;
}

//****************************************************************************
//*                                                                          *
//*                                                                          *
//****************************************************************************

AudioFile::AudioFile (IFileSystem  *system,   
							 const C8 *filename,
							 bool precache) :
	file_data(NULL),
	file_size(0)
{
   DAFILEDESC desc(filename);

   if (system == NULL)
   {
      system = (IFileSystem *) DACOM_Acquire();
   }

   system->CreateInstance(&desc, (void **) &pFile);

	if (pFile)
	{
		file_size = pFile->GetFileSize(0, NULL);

		if (precache || (file_size < 64 * 1024))
		{
			file_data = new char[file_size];

			if (!fetch(file_data, 0, file_size))
			{
				delete file_data;
				file_data=0;
			}

			pFile->Release();
			pFile = NULL;
		}
	}
}

//****************************************************************************
//*                                                                          *
//*                                                                          *
//****************************************************************************

AudioFile::~AudioFile()
{
   if (pFile) pFile->Release();
	if (file_data)
	{
		delete[] file_data;
	}
}

//****************************************************************************
//*                                                                          *
//*                                                                          *
//****************************************************************************

bool AudioFile::fetch(void* buffer, U32 offset, U32 size)
{
	if ((offset + size) > file_size)
	{
	// this caused underflow when file_size was 0
		if (file_size < offset)
		{
			return false;
		}

		size = file_size - offset;
	}

	if (pFile)
	{
		//
		// Set pointer to beginning of range
		//

		if (pFile->SetFilePointer(0,
										  offset,
										  NULL,
										  FILE_BEGIN) == 0xffffffff)
		{
			pFile->Release();
			pFile = NULL;
			return false;
		}
		//
		// Read range
		//

		S32 result = pFile->ReadFile(0,
											  buffer,
											  size,
											  NULL,
											  NULL);
		if (!result)
		{
			pFile->Release();
			pFile = NULL;
			return false;
		}
	}
	else
	{
		memcpy(buffer, file_data, size);
	}

	return true;
}
