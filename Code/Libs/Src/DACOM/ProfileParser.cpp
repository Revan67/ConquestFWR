 //--------------------------------------------------------------------------//
//                                                                          //
//                            ProfileParser.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Pbleisch $

   $Header: /Libs/dev/Src/DACOM/ProfileParser.cpp 15    3/21/00 4:30p Pbleisch $
*/
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "IProfileParser.h"
#include "TComponent.h"
#include "HeapObj.h"
#include <da_heap_utility.h>

#include <stdlib.h>

#ifdef PROFILE_PARSER_IS_CASE_SENSITIVE
#define compare strcmp
#define compare_len strncmp
#else
#define compare stricmp
#define compare_len strnicmp
#endif
#pragma warning (disable : 4100)		// formal parameter unused

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE ProfileParser : public IProfileParser
{
	//
	// interface mapping
	//
	BEGIN_DACOM_MAP_INBOUND(ProfileParser)
	DACOM_INTERFACE_ENTRY(IProfileParser)
	DACOM_INTERFACE_ENTRY2(IID_IProfileParser,IProfileParser)
	END_DACOM_MAP()

	//
	// data items
	//

	C8 * fileBuffer;
	U32 bufferSize;


	//
	// startup / shutdown code
	//

	GENRESULT init (PROFPARSEDESC * info) { return GR_OK; }

	DA_HEAP_DEFINE_NEW_OPERATOR(ProfileParser);

	ProfileParser( void )
	{
		fileBuffer = NULL;
		bufferSize = 0;
	}

	~ProfileParser (void)
	{
		free();
	}


	/* IProfileParser methods */

	DEFMETHOD(Initialize) (const C8 *fileName, ACCESS access = READ_ACCESS );

	DEFMETHOD_(BOOL32,EnumerateSections) (ENUM_PROC proc = 0, void *context=0);
	
	DEFMETHOD_(HANDLE,CreateSection) (const C8 *sectionName, CREATE_MODE mode = PP_OPENEXISTING);

	DEFMETHOD_(BOOL32,CloseSection) (HANDLE hSection);

	DEFMETHOD_(U32,ReadProfileLine) (HANDLE hSection, U32 lineNumber, C8 * buffer, U32 bufferSize);

	DEFMETHOD_(U32,ReadKeyValue) (HANDLE hSection, const C8 * keyName, C8 * buffer, U32 bufferSize);

	/* ProfileParser methods */

	void free (void);

	static const char * __fastcall getLine (const char *buffer, int line);

	static const char * __fastcall getSection (const char * buffer, int count);

	const char * getLine (HANDLE hSection, int line)
	{
		return getLine(fileBuffer+((U32)hSection), line);
	}
};
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ProfileParser2 : public IProfileParser2, ProfileParser
{
	//
	// interface mapping
	//
	BEGIN_DACOM_MAP_INBOUND(ProfileParser2)
	DACOM_INTERFACE_ENTRY(IProfileParser2)
	DACOM_INTERFACE_ENTRY(IProfileParser)
	DACOM_INTERFACE_ENTRY2(IID_IProfileParser2,IProfileParser2)
	DACOM_INTERFACE_ENTRY2(IID_IProfileParser,IProfileParser)
	END_DACOM_MAP()


	GENRESULT init (PROFPARSEDESC2 * info) { return GR_OK; }

	/* IProfileParser methods */

	DEFMETHOD(Initialize) (const C8 *fileName, ACCESS access)
	{
		return ProfileParser::Initialize(fileName, access);
	}

	virtual HANDLE __stdcall CreateSection (const C8 *sectionName, IProfileParser::CREATE_MODE mode)
	{
		return ProfileParser::CreateSection(sectionName, mode);
	}

	virtual BOOL32 __stdcall CloseSection (HANDLE hSection)
	{
		return ProfileParser::CloseSection(hSection);
	}

	virtual U32 __stdcall ReadProfileLine (HANDLE hSection, U32 lineNumber, C8 * buffer, U32 bufferSize)
	{
		return ProfileParser::ReadProfileLine(hSection, lineNumber, buffer, bufferSize);
	}

	virtual U32 __stdcall ReadKeyValue (HANDLE hSection, const C8 * keyName, C8 * buffer, U32 bufferSize)
	{
		return ProfileParser::ReadKeyValue(hSection, keyName, buffer, bufferSize);
	}

	/* IProfileParser2 methods */

	virtual GENRESULT __stdcall Initialize2 (const C8 *buffer, U32 bufferSize );

	virtual BOOL32 __stdcall EnumerateSections (IProfileCallback * callback, void *context);

	virtual BOOL32 __stdcall EnumerateKeys (IProfileCallback * callback, HANDLE hSection, void *context);
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
void ProfileParser::free (void)
{
	::free( fileBuffer );
	fileBuffer = 0;
}
//--------------------------------------------------------------------------//
//
GENRESULT ProfileParser::Initialize (const C8 *fileName, ACCESS access)
{
	HANDLE hFile=INVALID_HANDLE_VALUE;
	GENRESULT result = GR_FILE_ERROR;
	U32 dwAccess = GENERIC_READ;
	U32 dwOpen = OPEN_EXISTING;
	bool bBufferType = (S32(access) < 0);

	if (access == WRITE_ACCESS)
	{
		dwAccess = GENERIC_READ | GENERIC_WRITE;
		dwOpen = OPEN_ALWAYS;
	}

	if (bBufferType || (hFile = CreateFile(fileName, dwAccess, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, dwOpen, FILE_ATTRIBUTE_NORMAL, 0)) != INVALID_HANDLE_VALUE)
	{
		U32 fileSize;
		C8 * oldBuffer = fileBuffer;
		U32 bytesRead;

		if (bBufferType==false)
			fileSize = ::GetFileSize(hFile, 0);
		else
		{
			if (S32(access) == -1)
				fileSize = strlen(fileName);
			else
				fileSize = U32(access) & ~0x80000000;
		}

		if ((fileBuffer = (C8 *) calloc(fileSize+8,1)) != 0)
		{
			if (bBufferType)
				memcpy(fileBuffer+1, fileName, fileSize);
			else
				::ReadFile(hFile, fileBuffer+1, fileSize, &bytesRead, 0);
			::free(oldBuffer);
			bufferSize = fileSize+1;

			oldBuffer = fileBuffer+1;
			while (*oldBuffer == ' ')
				oldBuffer++;			// skip white space
			do
			{
				if (*oldBuffer == '[')      // next section
				{
					char * tmp, * tmp2;

					if ((tmp = strchr(oldBuffer, ']')) != 0)
					{
						tmp2 = strchr(oldBuffer, '\n');			// if no return between [ ... ]

						if (tmp2 == 0 || tmp2 > tmp)
						{
							// assume that previous character is not important,
							// put double 00 for '[', single 0 for ']'

							oldBuffer[-1] = 0;
							*oldBuffer = 0;		// mark it
							*tmp = 0;
							oldBuffer = tmp+1;
						}
					}
				}
			} while ((oldBuffer = (C8 *) getLine(oldBuffer, 1)) != 0);
			
			result = GR_OK;
		}
		else
		{
			fileBuffer = oldBuffer;
			result = GR_OUT_OF_MEMORY;
		}

		if (bBufferType==false)
			::CloseHandle(hFile);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
GENRESULT ProfileParser2::Initialize2 (const C8 *buffer, U32 bufferSize )
{
	return Initialize(buffer, ACCESS(bufferSize | 0x80000000));
}
//--------------------------------------------------------------------------//
// find 0D0A, then skip white space
//
const char * ProfileParser::getLine (const char * buffer, int line)
{
	const char * tmp;

	while (line>0)
	{
		if ((tmp = strchr(buffer, '\n')) == 0)
		{
			int len = strlen(buffer);

			buffer += len+1;
			if (buffer[0] == 0)
			{
				buffer = 0;
				goto Done;
			}
		}
		else
		{
			buffer = tmp+1;
			switch (*buffer)
			{
			case 0:
				buffer = 0;
				goto Done;
			case '\r':
				buffer++;
				break;
			}

			while (*buffer == ' ')
				buffer++;
			if (*buffer == 0)
			{
				buffer = 0;
				goto Done;
			}

			line--;
		}

	}

Done:	
	return buffer;
}
//--------------------------------------------------------------------------//
//
const char * ProfileParser::getSection (const char * buffer, int count)
{
	int len;

	while (count>0)
	{
		len = strlen(buffer);
		buffer += len;
		if (buffer[1] != 0)		// found a ']'
		{
			buffer++;
		}
		else
		if (buffer[2] != 0)		// found a '['
		{
			count--;
			buffer += 2;
		}
		else
			return 0;			// end of the line
	}

	return buffer;
}
//--------------------------------------------------------------------------//
//
BOOL32 ProfileParser::EnumerateSections (ENUM_PROC proc, void *context)
{
	BOOL32 result = 0;
	const char * buffer = fileBuffer;

	if (proc == 0 || fileBuffer==0)
		goto Done;

	result++;
	buffer = getSection(buffer, 1);
	while (result && buffer)
	{
		result = proc(this, buffer, context);
		buffer = getSection(buffer, 1);
	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 ProfileParser2::EnumerateSections (IProfileCallback * callback, void *context)
{
	BOOL32 result = 0;
	const char * buffer = fileBuffer;
	const char * name;

	if (callback == 0 || fileBuffer==0)
		goto Done;

	result++;
	buffer = getSection(buffer, 1);
	while (result && buffer)
	{
		name = buffer + strlen(buffer) + 1;	// name -> string to the right of the [section]
		result = callback->ProfileCallback(this, buffer, name, context);
		buffer = getSection(buffer, 1);
	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 ProfileParser2::EnumerateKeys (IProfileCallback * callback, HANDLE hSection, void *context)
{
	U32 result = 0;
	const char * tmp, * tmp2, * tmp3;
	char name[256];
	char value[256];
	int len;

	tmp = fileBuffer+((U32)hSection);

	while ((tmp = getLine(tmp, 1)) != 0)
	{
		if ((tmp2 = strchr(tmp, '=')) != 0)
		{
			tmp3 = tmp2--;
			while (tmp2 >= tmp)
			{
				// make sure last character of tmp[] is a letter,number,or underscore
				if (__iscsym(*tmp2))
					break;
				tmp2--;
			}

			// tmp2 -> last char of keyName
			if (tmp2 >= tmp)
			{
				len = tmp2-tmp+1;
				len = __min(255, len);
				memcpy(name, tmp, len);
				name[len] = 0;

				// elliminate newline and commented lines
				if (strchr(name, ';') == 0 && strchr(name, '\n') == 0)
				{
					do
					{	tmp3++;
					} while (*tmp3 == ' ');
			
					if ((tmp2 = strchr(tmp3, '\n')) != 0)
					{
						if (tmp2[-1] == '\r')
							tmp2--;	// get rid of extra char
					}
					else
					{
						tmp2 = tmp3 + strlen(tmp2);
					}

					len = tmp2-tmp3;
					len = __min(255, len);
					memcpy(value, tmp3, len);
					value[len] = 0;

					if ((result = callback->ProfileCallback(this, name, value, context)) == 0)
						break;
				}
			}
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
HANDLE ProfileParser::CreateSection (const C8 *sectionName, CREATE_MODE mode)
{
	HANDLE result = 0;

	if (mode != PP_OPENEXISTING)
		goto Done;						// nothing else is supported yet
	if (fileBuffer == 0)
		goto Done;						// not initialized?

	//
	// is requested sectionName already found within buffer?
	//

	if (sectionName > fileBuffer && sectionName < fileBuffer+bufferSize)
	{
		result = (HANDLE) (sectionName - fileBuffer);
	}
	else
	{	  
		//
		// else we must search for the section name 
		//
		const char * buffer = fileBuffer;

		buffer = getSection(buffer, 1);
		while (buffer)
		{
			if (compare(buffer, sectionName) == 0)
			{
				result = (HANDLE) (buffer - fileBuffer);
				goto Done;
			}

			buffer = getSection(buffer, 1);
		}
	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 ProfileParser::CloseSection (HANDLE hSection)
{
	return 1;
}
//--------------------------------------------------------------------------//
//
U32 ProfileParser::ReadProfileLine (HANDLE hSection, U32 lineNumber, C8 * buffer, U32 bufferSize)
{
	U32 result = 0;
	const char * tmp, * tmp2;

	if ((tmp = getLine(hSection, lineNumber+1)) != 0)		// line 0 is really the section name
	{
		if ((tmp2 = strchr(tmp, '\n')) != 0)
		{
			result = tmp2 - tmp;
			if (result && tmp[result-1] == '\r')
				result--;	// get rid of extra char
		}
		else
			result = strlen(tmp);
		if (bufferSize < result+1)
			result = bufferSize-1;

		if (buffer)
		{
			if (result == 0 && bufferSize>1)
			{
				*buffer = ' ';
				result++;
			}
			else
				memcpy(buffer, tmp, result);
			buffer[result] = 0;
		}
		else
		if (result == 0)
			result++;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
U32 ProfileParser::ReadKeyValue (HANDLE hSection, const C8 * keyName, C8 * buffer, U32 outBufferSize)
{
	U32 result = 0;
	const char * tmp, * tmp2;
	int cmplen;

	cmplen = strlen(keyName);
	tmp = fileBuffer+((U32)hSection);

	while ((tmp = getLine(tmp, 1)) != 0)
	{
		// is there enough room in the buffer for a match to be possible?
		if (U32(tmp) - U32(fileBuffer) + cmplen <= bufferSize)
		{
			// make sure last character of tmp[] is not a letter,number,or underscore
			if (__iscsym(tmp[cmplen])==0)
			{
				if (compare_len(tmp, keyName, cmplen) == 0)
				{
					if ((tmp = strchr(tmp, '=')) != 0)
					{
						tmp++;
						while (*tmp == ' ')
							tmp++;

						if ((tmp2 = strchr(tmp, '\n')) != 0)
						{
							result = tmp2 - tmp;
							if (result && tmp[result-1] == '\r')
								result--;	// get rid of extra char
						}
						else
							result = strlen(tmp);
						if (outBufferSize < result+1)
							result = outBufferSize-1;

						if (buffer)
						{
							if (result == 0 && outBufferSize>1)
							{
								*buffer = ' ';
								result++;
							}
							else
								memcpy(buffer, tmp, result);
							buffer[result] = 0;
						}
						else
						if (result == 0)
							result++;
						break;
					}
				}
			}
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
IComponentFactory * CreateProfileParserFactory2 (void)
{
	return new DAComponentFactory2<DAComponentAggregate<ProfileParser2>, PROFPARSEDESC2> ("IProfileParser2");
}
IComponentFactory * CreateProfileParserFactory (void)
{
	return new DAComponentFactory2<DAComponentAggregate<ProfileParser>, PROFPARSEDESC> ("IProfileParser");
}

//--------------------------------------------------------------------------//
//----------------------------END ProfileParser.cpp-------------------------//
//--------------------------------------------------------------------------//
