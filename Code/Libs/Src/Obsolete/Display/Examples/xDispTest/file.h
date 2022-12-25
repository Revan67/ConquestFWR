#ifndef FILE_H
#define FILE_H

#include <stdio.h>	// fopen, fseek, fread, etc.
#include <string.h>	// strcpy
#include <memory.h>	// memcpy

//#include "std.h"

//---------------------------------------------------------------------------
// FileInfo
//---------------------------------------------------------------------------

typedef unsigned FilePos;

// FileInfo.flags
#define FILE_OPEN		1
#define FILE_WRITE		2

struct FileInfo
{
	int flags;
	FILE *handle;
	char *memory;	// alternate storage

	FilePos	size;

	char name[128];

	void reset (void)
	{
		flags = 0;
		handle = NULL;
		memory = NULL;
		size = 0;
		name[0] = 0;
	}

	void set_name (const char *fname)
	{
		strcpy(name,fname);
	}

	int is_open (void)
	{
		return (handle != NULL);
	}

	void open (FILE *h, FilePos _size, int _flags=0)
	{
		handle = h;
		size = _size;
		if (h)
			flags |= FILE_OPEN|_flags;
	}

	void close (void)
	{
		flags = 0;
		handle = NULL;
		memory = NULL;
		size = 0;
	}
};

//---------------------------------------------------------------------------
// FileServer
//---------------------------------------------------------------------------

struct FileServer
{
	static FilePos filesize (FILE *handle)
	{
		FilePos size = 0;
		if (handle)
		{
			fseek(handle,0,SEEK_END);	// move to end
			size = ftell(handle);
			fseek(handle,0,SEEK_SET);	// back to start
		}
		return (size);
	}

	static int open (FileInfo &info)
	{
		FILE *handle;
		handle = fopen(info.name,"rb");
		if (handle)
		{
			info.open(handle,filesize(handle));
		}
		return (info.is_open());
	}

	static void close (FileInfo &info)
	{
		if (info.is_open())
		{
			fclose(info.handle);
			info.close();
		}
	}

	static int create (FileInfo &info)
	{
		FILE *handle;
		handle = fopen(info.name,"wb");
		if (handle)
		{
			info.open(handle,0,FILE_WRITE);
		}
		return (info.is_open());
	}
};

//---------------------------------------------------------------------------
// File
//---------------------------------------------------------------------------

class File : public FileInfo
{
	FilePos offset;

	void init (void)
	{
		FileInfo::reset();
		offset = 0;
	}

public:

	File (void)
	{
		init();
	}

	int open (const char *fname)
	{
		FileInfo::set_name(fname);
		return FileServer::open(*this);
	}

	void close (void)
	{
		FileServer::close(*this);
	}

	int eof (void)
	{
		offset = ftell(handle);
		return (offset >= size);
	}

	long limit (FilePos pos)
	{
		if (pos < 0)
			pos = 0;
		if (pos > size)
			pos = size;
		return (pos);
	}

	void seek (FilePos pos)
	{
		pos = limit(pos);
		if (handle)
		{
			fseek(handle,pos,SEEK_SET);
		}
		offset = pos;
	}

	void seek_relative (long pos)
	{
		seek(offset+pos);
	}

	FilePos tell (void)
	{
		return ftell(handle);
	}
	FilePos get_offset (void)
	{
		return (offset);
	}

	FilePos read (void *dst, FilePos len)
	{
		FilePos count = 0;
		if (handle)
		{
			count = fread(dst,1,len,handle);
			offset += count;
		}
		else if (memory)
		{
			count = size-offset;
			if (count > len)
				count = len;
			memcpy(dst,memory+offset,count);
			offset += count;
		}
		return (count);
	}

	FilePos read (FilePos start, void *dst, FilePos len)
	{
		FilePos count;
		seek(start);
		count = read(dst,len);
		seek(offset);
		return (count);
	}

	BYTE read_byte (void)
	{
		char data[1];
		read(data,sizeof(data));
		return *(BYTE *)data;
	}

	WORD read_word (void)
	{
		char data[2];
		read(data,sizeof(data));
		return *(WORD *)data;
	}

	DWORD read_long (void)
	{
		char data[4];
		read(data,sizeof(data));
		return *(DWORD *)data;
	}

#define MAX_LINE_SIZE 256

	int read_line (char *line, int max=MAX_LINE_SIZE)
	{
		return 0;
	}
};

//---------------------------------------------------------------------------

#endif // FILE_H