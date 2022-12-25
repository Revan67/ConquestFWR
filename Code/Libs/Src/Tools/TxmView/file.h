#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <share.h>

#include <sys/stat.h>
#include <io.h>

//---------------------------------------------------------------------------
// File
//---------------------------------------------------------------------------

class File
{
protected:

	char name[128];
	FILE *handle;
	int offset;
	int length;

public:

// test file status

	int get_length (void)
	{
		return length;
	}

	int eof (void)
	{
		return (offset >= length);
	}

	int is_open (void)
	{
		return (handle != NULL);
	}

//public:

	File (void)
	{
		name[0] = 0;
		handle = NULL;
		length = 0;
	}

	void close (void)
	{
		if (handle)
		{
			fclose(handle);
			handle = NULL;
		}
	}
	
	~File (void)
	{
		close();
	}

	void get_info (void)
	{
		assert(handle);
		fseek(handle,0,SEEK_END);
		length = ftell(handle);
		offset = 0;
		fseek(handle,offset,SEEK_SET);
	}

	void set_name (const char *_name)
	{
		strcpy(name,_name);
	}

	bool open (const char *_name)
	{
		set_name(_name);

		if (_name[0] == 0)
			return false;

//		if (_access(_name,0) == -1)
//			MessageBox(0,_name,"_access = DENIED!",MB_OK);

		handle = fopen(_name,"rb");
		if (handle)
		{
			get_info();
		}
		else
		{
//			MessageBox(0,name,"File::open = FAILED!",MB_OK);
		}
		return (handle != NULL);
	}

	bool append (const char *_name)
	{
		set_name(_name);

		if (_name[0] == 0)
			return false;

//		if (_access(_name,0) == -1)
//			MessageBox(0,_name,"_access = DENIED!",MB_OK);

		handle = fopen(_name,"a+b");
		if (handle)
		{
			get_info();
			offset = length;
		}
		else
		{
//			MessageBox(0,name,"File::open = FAILED!",MB_OK);
		}
		return (handle != NULL);
	}

	bool create (const char *_name)
	{
		set_name(_name);

		handle = fopen(name,"wb");
		if (handle)
		{
			get_info();
		}
		return (handle != NULL);
	}

// MISC

	int set_offset (int pos)
	{
		if (pos >= 0 && pos <= length)
			offset = pos;
		return offset;
	}

	int seek (int pos)
	{
		if (pos == -1)
			return offset;
		if (pos != offset)
			set_offset(pos);
		if (is_open())
		{
			if (fseek(handle,offset,SEEK_SET) == 0)
				return (offset);
		}
		return (0); // ERROR?
	}

	int tell (void) const
	{
		return (offset);
	}

	int read (void *dst, int size)
	{
		if (handle == 0)
			return 0;

		return fread(dst,1,size,handle);
	}

	int read (void *dst, int pos, int size)
	{
		seek(pos);
		return read(dst,size);
	}

	int write (const void *src, int size)
	{
		if (handle == 0)
			return 0;
		return fwrite(src,1,size,handle);
	}
	int write (const void *src, int pos, int size)
	{
		seek(pos);
		return write(src,size);
	}

	int write_string (const char *src)
	{
		int s = strlen(src);
		return write(src,s);
	}
};

//---------------------------------------------------------------------------

#endif // FILE_H
