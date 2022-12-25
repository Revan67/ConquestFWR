#ifndef AUDIO_FILE_H
#define AUDIO_FILE_H

#include <windows.h>

#include "FileSys.h"

//****************************************************************************
//*                                                                          *
//*                                                                          *
//****************************************************************************

class AudioFile
{
public:
	AudioFile(IFileSystem  *system,   
				 const C8  *filename,
				 bool precache);

	~AudioFile();

	bool loaded ( void ) { return pFile || file_data; }

	bool fetch(void* buffer, U32 offset, U32 size);

private:
	friend class AudioBuffer;

	IFileSystem* pFile;

	void* file_data;
	U32   file_size;
};

//****************************************************************************
//*                                                                          *
//*                                                                          *
//****************************************************************************

class AudioBuffer
{
public:
	AudioBuffer(AudioFile*, U32 sample_offset, U32 sample_size);
	~AudioBuffer();

	bool fetch(U32 offset); // Fetch at given offset
	bool fetch();			   // Fetch next

	void* data() const { return buffer_data[toggle]; }
	U32   size() const { return buffer_size[toggle]; }

	bool at_end() const { return file_offset >= sample_end; }

private:
	AudioFile* file;

	void* buffer_data[2];
	U32   buffer_size[2];

	U32   allocated_size;
	U32	sample_offset;
	U32	sample_end;
	U32	file_offset;
	int	toggle;
};


#endif
