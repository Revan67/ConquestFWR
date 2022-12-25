#ifndef DRIVERMGR_H
#define DRIVERMGR_H

//

#include <assert.h>
#include <windows.h>
#include "dacom.h"
#include "iprofileparser.h"

///

extern ICOManager * DACOM;

//---------------------------------------------------------------------------
// DriverMgr
//---------------------------------------------------------------------------

struct DriverMgr
{
	char string[256];

	char name[128];
	char value[128];

	int current;

	DriverMgr (void)
	{
		current = -1;
	}

	int get_driver_string (int index, char *dst, int max)
	{
		IProfileParser *parser;

		int size = 0;

		if (DACOM->QueryInterface("IProfileParser", (void **)&parser) == GR_OK)
		{
			HANDLE hSection = parser->CreateSection("OpenGL_Drivers");
			if (hSection)
			{
				char buffer[256];
				int line=0;
				while (parser->ReadProfileLine(hSection, line, buffer, sizeof(buffer)) != 0)
				{
					if (index == line++)
					{
						size = strlen(buffer);
						if (size > max) size = max;
						strncpy(dst,buffer,size);
						break;
					}
				}
			}
			parser->Release();
		}
		dst[size] = 0;

		return size;
	}

	int get_count (void)
	{
		int count = 0;
		for (int i=0; get_driver_string(i,string,sizeof(string)) > 0; i++)
		{
			count = i+1;
		}
		return count;
	}

	int parameterize (const char *src)
	{
		const char *stop = 0;
		const char *p = strchr(src,' ');
		if (p)
		{
			stop = p;

			while (*p == ' ')
				p++;
			if (*p == '=')
			{
				p++;
				while (*p == ' ')
					p++;
			}
		}
		else
		{
			p = strchr(src,'=');
			if (p)
			{
				stop = p;

				p++;
				while (*p == ' ')
					p++;
			}
		}

		int size;
		if (stop)
			size = stop-src;
		else
		{
			size = strlen(src);
			p = src + size;
		}
		strncpy(name,src,size);
		name[size] = 0;

		int parms = 0;
		if (*p)
		{
			parms = 1;	// FUTURE: count separators
			strcpy(value,p);
		}
		return parms;
	}

	int get_parm (char *dst, int i=0)
	{
		assert(i==0);
		strcpy(dst,value);
		return strlen(dst);
	}

	int load_library (int i=0)
	{
		current = -1;
		name[0] = 0;
		value[0] = 0;
		return 0;
	}

	int next_library (void)
	{
		return load_library(current+1);
	}
};

#endif
