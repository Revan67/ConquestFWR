//---------------------------------------------------------------------------
/*
	INI.CPP

	(Win32) Lancer (c) 1997 Digital Anvil

	01-27-97 created (pci)
	02-17-98 severed from project (Ed Maurer)
*/
//---------------------------------------------------------------------------

#include "pch.h"	//precompiled header
#include "ini.h"
#include <math.h>
#include <stdlib.h>
#include "typedefs.h"

#define ASSERT assert

//---------------------------------------------------------------------------
// CHARACTER TYPES
//---------------------------------------------------------------------------

#define IS_ALPHA	1
#define IS_UPPER	2
#define IS_LOWER	4
#define IS_NUMBER	8
#define IS_HEX		16
#define IS_WORD		32
#define IS_WHITE	64
#define IS_OP		128

typedef unsigned char byte;
extern byte IsFlags[128];

#define CharFlags(ch)	IsFlags[((ch)&127)]

#define IsAlpha(ch)		(CharFlags(ch) & IS_ALPHA)
#define IsNum(ch)		(CharFlags(ch) & IS_NUMBER)
#define IsHex(ch)		(CharFlags(ch) & IS_HEX)
#define IsWhite(ch)		(CharFlags(ch) & IS_WHITE)
#define IsWord(ch)		(CharFlags(ch) & IS_WORD)
#define IsOperator(ch)	(CharFlags(ch) & IS_OP)

#define IsLabelStart(ch) (IsAlpha(ch)||(ch)=='_')

inline int IsEOL (int ch)
{
	return ch == '\r' || ch == '\n';
}

inline int PairEOL (int ch)
{
	return ch ^ '\r' ^ '\n';
}

//---------------------------------------------------------------------------

byte IsFlags[128] =
{
#define A IS_ALPHA
#define U IS_UPPER
#define L IS_LOWER
#define N IS_NUMBER
#define H IS_HEX
#define W IS_WORD
#define w IS_WHITE
#define o IS_OP

//0x00
	0, 0, 0, 0, 0, 0, 0, 0, 0, w, w, 0, 0, w, 0, 0,
//0x10
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//0x20
	w, o, 0, W, 0, 0, o, 0, 0, 0, o, o, 0, o, 0, o,	// SPACE, 0x23='#'
//0x30
	N|H|W,N|H|W,N|H|W,N|H|W,N|H|W,N|H|W,N|H|W,N|H|W,N|H|W,N|H|W, 0, 0, o, o, o, 0,
//0x40
	0,A|U|H|W,A|U|H|W,A|U|H|W,A|U|H|W,A|U|H|W,A|U|H|W, A|U|W, A|U|W, A|U|W, A|U|W, A|U|W, A|U|W, A|U|W, A|U|W, A|U|W,
//0x50
	A|U|W, A|U|W, A|U|W, A|U|W, A|U|W, A|U|W, A|U|W, A|U|W,
	A|U|W, A|U|W, A|U|W, 0, 0, 0, o, W, // 0x5F='_'
//0x60
    0,A|L|H|W,A|L|H|W,A|L|H|W,A|L|H|W,A|L|H|W,A|L|H|W, A|L|W, A|L|W, A|L|W, A|L|W, A|L|W, A|L|W, A|L|W, A|L|W, A|L|W,
//0x70
	A|L|W, A|L|W, A|L|W, A|L|W, A|L|W, A|L|W, A|L|W, A|L|W, A|L|W, A|L|W, A|L|W, 0, o, 0, o, 0,

#undef A
#undef U
#undef L
#undef N
#undef H
#undef W
#undef w
};

//---------------------------------------------------------------------------

byte ToDigit[128] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x00
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x10
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x20
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,	// 0x30
	0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x40
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x50
	0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x60
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x70
};

//---------------------------------------------------------------------------
// INI_Reader
//---------------------------------------------------------------------------

int INI_Reader::skip_white (void)
{
	while (line_pos < line_size)
	{
		if (!IsWhite(line[line_pos]))
			break;
		line_pos++;
	}
	return (line_pos);
}

//---------------------------------------------------------------------------

double INI_Reader::value_num (int i)
{
	double num;
	double a,b;

	char *val = parm_value;

	while (i > 0)
	{
		val = strchr(val,',');
		if (val == 0)
			return 0;
		val++;

		i--;
	}

	// skip any white space after comma
	while (IsWhite(*val))
	{
		++val;
	}

	char work[128];
	for (int n=0; val[n]; n++)
	{
		if (IsWhite(val[n]) || val[n] == ';' || val[n] == ',')
		{
			break;
		}
		work[n] = val[n];
	}
	work[n] = 0;

	char *p = strchr(work,'/');	// SIMPLE DIVISION!

	if (p == 0)
	{
		num = atof(work);
	}
	else
	{
		a = atof(work);
		b = atof(p+1);
		num = a/b;
	}
	return num;
}

//---------------------------------------------------------------------------

int INI_Reader::get_indexed_value (char *dst, int i)
{
	char *start = parm_value;

	while (1)
	{
		while (IsWhite(*start))
		{
			++start;
		}

		char *stop = strchr(start,',');

		if (i < 1)
		{
			if (stop == 0) // last parameter
			{
				strcpy(dst,start);
			}
			else // strip trailing spaces
			{
				while (stop > start && IsWhite(stop[-1]))
				{
					--stop;
				}
				int len = stop-start;
				strncpy(dst,start,len);
				dst[len] = 0;
			}
			break;
		}
		else // step past comma
		{
			if (stop)
			{
				start = stop+1;
			}
			else // no more parameters?
			{
				dst[0] = 0;
				break;
			}
		}

		i--;
	}

	return strlen(dst);
}

//---------------------------------------------------------------------------

int INI_Reader::read_line (void)
{
	int pos = File::tell();

	line_size = File::read(line,sizeof(line)-1);
	line[sizeof(line)-1] = 0;

	if (line_size == 0)
		return FALSE;

	for (int i=0; i<line_size; i++)
	{
		char ch = line[i];
		if (IsEOL(ch))
		{
			line_size = i;

			line[i++] = 0;

			if (line[i] == PairEOL(ch))
				i++;

			File::seek(pos+i);

			break;
		}
	}

	if (line_size > 0)
		line_num++;

	line_pos = 0;

	return TRUE;
}

//---------------------------------------------------------------------------

int INI_Reader::fetch_string (char *dst, int stop)
{
	int size = stop-line_pos;

	ASSERT(size >= 0);
	assert(size < INI_STRING_SIZE);

	memcpy(dst,line+line_pos,size);

	while (size && IsWhite(dst[size-1]))
		size--;

	dst[size] = 0;

	return size;
}

//---------------------------------------------------------------------------

int INI_Reader::read_header (void)
{
	int size = 0;

	while (read_line())
	{
		skip_white();

		char ch = get_char();

		if (ch == ';' || ch == 0)
		{
			// comment - skip line
		}
		else if (ch == '[')
		{
			line_pos++;
			int j = find_char(']');
			if (j == -1)
				j = line_size;
			size = fetch_string(header, j);
			break;
		}
		else
		{
			// parameter
		}
	}
	header[size] = 0;
	return size > 0;
}

//---------------------------------------------------------------------------

int INI_Reader::read_value (void)
{
	int start = File::tell();
	while (read_line())
	{
		skip_white();

		char ch = get_char();

		if (ch == ';' || ch == 0)
		{
			// comment
		}
		else if (ch == '[')
		{
			File::seek(start);
			return FALSE;
		}
		else
		{
			int i = find_char(' ');

			if (i == -1)
			{
				i = find_char('=');
				if (i == -1)
					i = line_size;
			}

			fetch_string(parm_name,i);

			line_pos = i;

			skip_white();
			if (skip_char('='))
			{
				skip_white();
			}

			fetch_string(parm_value,line_size);

			// remove ';' comments
			char *ptr = strchr(parm_value,';');
			if (ptr)
			{
				for (; ptr > parm_value; ptr--)
				{
					if (!IsWhite(ptr[-1]))
						break;
				}
				*ptr = 0; // terminate string
			}

			return TRUE;
		}
	}
	return FALSE;
}

//---------------------------------------------------------------------------

int INI_Reader::find_header (const char *name)
{
	reset();

	while (read_header())
	{
		if (is_header(name))
			return true;
	}

	return false;
}

//---------------------------------------------------------------------------
