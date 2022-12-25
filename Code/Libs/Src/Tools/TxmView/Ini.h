//---------------------------------------------------------------------------
/*
	INI.H

	(Win32) Test Program (c) 1997 Digital Anvil

	01-27-97 created (pci)
	02-17-98 severed from project (Ed Maurer)
*/
//---------------------------------------------------------------------------

#ifndef INI_H
#define INI_H

#pragma warning(disable:4244)

//---------------------------------------------------------------------------

#include "file.h"
#include "dllinc.h"

//---------------------------------------------------------------------------
// INI_Reader
//---------------------------------------------------------------------------

#define INI_STRING_SIZE 256

#pragma warning(disable: 4275)

class COMMON_EXP INI_Reader : public File
{
protected:

	int line_num;
	int line_size;
	char line[INI_STRING_SIZE];

	int line_pos;

	char header[INI_STRING_SIZE];
	char parm_name[INI_STRING_SIZE];
	char parm_value[INI_STRING_SIZE];

protected:

	int read_line (void);

	int skip_white (void);

	int get_char (int i = -2)
	{
		if (i == -2)
		{
			i = line_pos;
		}

		assert(i >= 0);

		if (i > line_size)
			return 0;

		return line[i];
	}

	int skip_char (int ch)
	{
		if (line_pos < line_size && line[line_pos] == ch)
		{
			line_pos++;
			return 1;
		}
		return 0;
	}

	int find_char (int ch)
	{
		for (int i=line_pos; i<line_size; i++)
		{
			if (line[i] == ch)
			{
				return i;
			}
		}
		return -1;
	}

	int fetch_string (char *dst, int stop);

public:

	void reset (void)
	{
		line_num = 0;
		line[0] = 0;
		line_size = 0;
		line_pos = 0;
		header[0] = 0;

		File::seek(0);
	}

	INI_Reader (void)
	{
		reset();
	}

// READ FUNCTIONS

	int read_header (void);
	int read_value (void);

	int find_header (const char *name);
	// note: next call to get_header() will succeed!

// COMPARE FUNCTIONS

	int is_header (const char *name)
	{
		return stricmp(header,name) == 0;
	}
	int is_value (const char *name)
	{
		return stricmp(parm_name,name) == 0;
	}

	const char *get_line_ptr(void)		{ return line; }
	const char *get_header_ptr(void)	{ return header; }
	const char *get_name_ptr(void)		{ return parm_name; }
	const char *get_value_ptr(void)		{ return parm_value; }

	const char *get_value_string (void)	{ return parm_value; }

// GET FUNCTIONS

	int get_indexed_value (char *dst, int i);
	// similar value_string() but allows comma separated arguments

	void value_string (char *dst)
	{
		strcpy(dst,parm_value);
	}

	double value_num (int i=0);

/*
	int value_id (void)
	{
		return *(int *)parm_value; // TEMPORARY!
	}
*/

// MISC.

	struct State
	{
		protected:

		friend INI_Reader;

		int offset;

		int line_num;
		int line_size;
		char line[256];

		int line_pos;

		char header[256];
		char parm_name[256];
		char parm_value[256];
	};

	void get_state (State& out)
	{
		out.offset = tell ();

		out.line_num = line_num;
		out.line_size = line_size;
		memcpy (out.line, line, sizeof (out.line));
		out.line_pos = line_pos;
		memcpy (out.header, header, sizeof (out.header));
		memcpy (out.parm_name, parm_name, sizeof (out.parm_name));
		memcpy (out.parm_value, parm_value, sizeof (out.parm_value));
	}

	void set_state (const State& in)
	{
		seek (in.offset);

		line_num = in.line_num;
		line_size = in.line_size;
		memcpy (line, in.line, sizeof (line));
		line_pos = in.line_pos;
		memcpy (header, in.header, sizeof (header));
		memcpy (parm_name, in.parm_name, sizeof (parm_name));
		memcpy (parm_value, in.parm_value, sizeof (parm_value));
	}
};

#pragma warning(default: 4275)

//---------------------------------------------------------------------------

#endif // INI_H
