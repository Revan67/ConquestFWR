//---------------------------------------------------------------------------
/*
	STRING.H

	FreeLancer (c) 1998 Digital Anvil

	01-28-98 created (pci)

	$Header: /Tools/TxmView/TString.h 1     5/06/99 12:59p Pisaac $
*/
//---------------------------------------------------------------------------

#ifndef STRING_H
#define STRING_H

#include <stdio.h>		// sprintf()
#include <stdarg.h>
#include <assert.h>
#include <malloc.h>		// malloc,free
#include <string.h>

//---------------------------------------------------------------------------
// TString template
//---------------------------------------------------------------------------

template<unsigned int MAX> struct TString
{
	unsigned int size;

	char string[MAX];

public:

	void set (const char *s)
	{
		int l = (s!=0) ? strlen(s) : 0;

		if (l >= MAX)
		{
			l = MAX-1;
		}
		memcpy(string,s,l);

		string[l] = 0;
		size = l;
	}

	void init (void)
	{
		size = 0;
		string[0] = 0;
	}

public:

	void free (void)
	{
		size = 0;
		string[0] = 0;
	}

	TString (void)
	{
		init();
	}

	TString (const char *s)
	{
		init();
		set(s);
	}

	TString &operator = (TString &s)
	{
		set(s);
		return *this;
	}

	const char *get_string (void) const
	{
		return string;
	}

	operator const char * (void) const
	{
		return string;
	}
	operator char * (void)
	{
		return string;
	}

	operator const void * (void) const
	{
		return string;
	}

	TString &operator = (const char *txt)
	{
		set(txt);
		return *this;
	}

	bool compare (const char *s) const
	{
		if (s == 0)
		{
			return size == 0;
		}
		return strcmp(string,s) == 0;
	}

	bool operator == (const char *s) const
	{
		return compare(s);
	}

	bool operator != (const char *s) const
	{
		return !compare(s);
	}

	friend bool operator == (const TString &s0, const TString &s1)
	{
		return s0.compare(s1);
	}
	friend bool operator != (const TString &s0, const TString &s1)
	{
		return !s0.compare(s1);
	}

	TString &operator += (char ch)
	{
		if (size+1 < MAX)
		{
			string[size++] = ch;
			string[size] = 0;
		}
		return *this;
	}

	void verify (int &len) const
	{
		if (len >= MAX)
		{
			len = MAX-1;
		}
	}

	TString &operator += (const char *s)
	{
		if (s)
		{
			int l = size+strlen(s); 
			verify(l);
			memcpy(string+size,s,l-size);
			string[l] = 0;
			size = l;
		}
		return *this;
	}

// CString - emulation

	void Empty (void)
	{
		size = 0;
		string[0] = 0;
	}

	unsigned int GetLength (void) const
	{
		return size;
	}

	bool IsEmpty (void) const
	{
		return GetLength() == 0;
	}

	unsigned int GetMaxLength (void) const
	{
		return sizeof(string);
	}

	void SetAt (unsigned int i, char ch)
	{
		if (i < size)
		{
			string[i] = ch;
			if (ch == 0)
				size = i;
		}
	}

	void Format (const char *fmt, ...)
	{
		char bfr[256];
		va_list args;
		va_start(args,fmt);
		vsprintf(bfr,fmt,args);
		assert(strlen(bfr) < sizeof(bfr));
		set(bfr);
	}

	void MakeUpper (void)
	{
		strupr(string);
	}

	void MakeLower (void)
	{
		strlwr(string);
	}
};

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// CString emulator
//---------------------------------------------------------------------------

#define CString CharString

class CharString
{
	int max;
	int size;
	char *string;
	bool owner;

public:

	void free (void)
	{
		if (string)
		{
			if (owner)
			{
				::free(string);
			}
			string = 0;
			size = 0;
		}
	}

	void alloc (int n)
	{
		if (owner && n != max) // can't re-allocate un-owned strings
		{
			char *ptr = (char *)malloc(n);
			if (ptr)
			{
				if (string)
				{
					if (size > n)
						size = n;
					memcpy(ptr,string,size);
					::free(string);
				}
				else
				{
					size = 0;
				}
				string = ptr;
				string[size] = 0;
				max = n;
			}
		}
	}

	bool verify (int s)
	{
		if (owner)
		{
			if (s > max)
			{
				alloc(s);
			}
		}
		assert(s<max);
		return s < max;
	}

	void share (char *_data, int _max, bool _owner=false)
	{
		free();
		max = _max;
		string = _data;
		size = strlen(string);
		owner = _owner;
	}

	void init (void)
	{
		max = 0;
		size = 0;
		string = 0;
		owner = true;
	}

	~CharString (void)
	{
		free();
	}

	void set (const char *src, int len=-1)
	{
		if (len<0)
			len = strlen(src);
		if (max == 0)
		{
			alloc(len+1);
		}
		if (max == 0)
			return;
		if (len >= max)
			len = max-1;
		strncpy(string,src,len);
		string[len] = 0;
		size = len;
	}
/*
	CharString (int _max)
	{
		init();
		alloc(_max);
	}
*/
	CharString (void)
	{
		init();
	}

	CharString (const char *s)
	{
		init();
		set(s);
	}
/*
	CharString (const CharString &s)
	{
		init();
		set(s.string);
	}
*/
	CharString (char *_data, int _max, bool _owner)
	{
		init();
		share(_data,_max,_owner);
	}

	int GetMaxLength (void) const
	{
		return max;
	}
	int FixLength (void)
	{
		int s;
		for (s=0; s<max; s++)
		{
			if (string[s] == 0)
				break;
		}
		size = s;
		string[size] = 0;
		return size;
	}

	void Empty (void)
	{
		string[0] = 0;
		size = 0;
	}

	int GetLength (void) const
	{
		return size;
	}

	bool IsEmpty (void) const
	{
		return GetLength() == 0;
	}

	void operator = (const char *s)
	{
		set(s);
	}
	char operator [] (int i) const
	{
		if (i >= size)
			return 0;
		return string[i];
	}

	CharString &operator += (const char *s)
	{
		if (s)
		{
			int l = size+strlen(s); 
			verify(l);
			strcat(string,s);
			size = l;
		}
		return *this;
	}

	CharString &operator += (char ch)
	{
		if (size+1 < max)
		{
			string[size++] = ch;
			string[size] = 0;
		}
		return *this;
	}
/*
	friend CharString operator + (const CharString &s1, const CharString &s2)
	{
		CharString tmp(s1);
		tmp += s2.string;
		return CharString(tmp);
	}
  
	friend CharString operator + (const CharString &s, char ch)
	{
		CharString tmp(s);
		tmp += ch;
		return CharString(tmp);
	}
 
	friend CharString operator + (char ch, const CharString &s)
	{
		CharString tmp;
		tmp += ch;
		tmp += s;
		return CharString(tmp);
	}
  
	friend CharString operator + (const CharString &s, const char *lpsz)
	{
		CharString tmp(s);
		tmp += lpsz;
		return CharString(tmp);
	}
  
	friend CharString operator + (const char *lpsz, const CharString &s)
	{
		CharString tmp(lpsz);
		tmp += s;
		return CharString(tmp);
	}
*/
	operator const char * (void) const
	{
		return string;
	}

	char *GetBuffer (void)
	{
		return string;	// note: if modified call FixLength()
	}

	int compare (const char *s) const
	{
		if (size)
		{
			return strcmp(string,s) == 0;
		}
		else
		{
			return s[0] == 0;
		}
	}

	int operator == (const char *s) const
	{
		return compare(s);
	}

	int operator != (const char *s) const
	{
		return !compare(s);
	}

	void Format (const char *fmt, ...)
	{
		char bfr[256];
		va_list args;
		va_start(args,fmt);
		vsprintf(bfr,fmt,args);
		assert(strlen(bfr) < sizeof(bfr));
		set(bfr);
	}

	void MakeUpper (void)
	{
		strupr(string);
	}

	int Find (char ch)
	{
		char *p = strchr(string,ch);
		if (p)
			return p - string;
		else
			return -1;
	}

	int ReverseFind (char ch)
	{
		char *p = strrchr(string,ch);
		if (p)
			return p - string;
		else
			return -1;
	}

	void SetAt (int i, char ch)
	{
		if (i < size)
		{
			string[i] = ch;
			if (ch == 0)
				size = i;
		}
	}

	CharString Left (int l)
	{
		CharString s;
		if (l > size)
			l = size;
		if (l > 0)
		{
			s.set(string,l);
		}
		return s;
	}
};

// Use this to convert a TString<?> to a CharString
#define CSTRING(t) CharString(t.string, t.GetMaxLength(), false)

//---------------------------------------------------------------------------

#endif // STRING_H
