//--------------------------------------------------------------------------//
//                                                                          //
//                               LFParser.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/LFParser.cpp 11    12/13/00 11:46a Jasony $

   Parser for the lip flapping files
*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>

#include "LFParser.h"
#include "CQTrace.h"
#include "Resource.h"

#include <FileSys.h>
#include <TSmartPointer.h>
#include <TComponent.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE LFParser : public ILFParser
{
	BEGIN_DACOM_MAP_INBOUND(LFParser)
	DACOM_INTERFACE_ENTRY(ILFParser)
	END_DACOM_MAP()

	//------------------------
	U32 * frameArray;
	const char *pMemory;
	U32 bufferSize;
	U32 numElements;
	U32 currentFrame;
	U32 bytesRead;
	OVERLAPPED overlapped;			// used for asynchronous I/O
	HANDLE hFile;
	bool bIOComplete;
	//------------------------

	LFParser (void);

	~LFParser (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ILFParser methods */

	virtual bool Initialize (const char * fileName);
	
	// returns the number of elements in the array, 
	// -1 on error
	// 0 = IOPENDING
	virtual S32 ParseFile (U32 ** frameArray);


	/* LFParser methods */

	void reset (void);

	U32 parseFile (const char *_pMemory);

	inline char getChar (const char * & _pMemory);

	IDAComponent * getBase (void)
	{
		return static_cast<ILFParser *>(this);
	}
};
//--------------------------------------------------------------------------//
//
LFParser::LFParser (void)
{
	hFile = INVALID_HANDLE_VALUE;
}
//--------------------------------------------------------------------------//
//
LFParser::~LFParser (void)
{
	reset();
}
//--------------------------------------------------------------------------//
//
void LFParser::reset (void)
{
	if (frameArray)
	{
		VirtualFree(frameArray, 0, MEM_RELEASE);
		frameArray = 0;
	}
	if (pMemory)
	{
		free((void *)pMemory);
		pMemory = 0;
	}
	bufferSize = 0;
	numElements = 0;
	currentFrame = 0;
	if (hFile != INVALID_HANDLE_VALUE)
	{
		if (bIOComplete==0)	// if waiting on an overlapped read
			MSPEECHDIR->GetOverlappedResult(hFile, &overlapped, &bytesRead, 1);
		MSPEECHDIR->CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
	bIOComplete = 0;
	bytesRead = 0;
}
//--------------------------------------------------------------------------//
//
bool LFParser::Initialize (const char * fileName)
{
	bool result = false;
	DAFILEDESC fdesc = fileName;

	reset();

	if ((hFile = MSPEECHDIR->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQFILENOTFOUND(fdesc.lpFileName);
	}
	else
	{
		bufferSize = MSPEECHDIR->GetFileSize(hFile);
		pMemory = (const char *) malloc(bufferSize);

		memset(&overlapped, 0, sizeof(overlapped));
		bytesRead = 0;
		bIOComplete = (MSPEECHDIR->ReadFile(hFile, const_cast<char *>(pMemory), bufferSize, &bytesRead, &overlapped) != 0);

		result = true;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
S32 LFParser::ParseFile (U32 ** ppFrameArray)
{
	S32 result = -1;		// assume error case

	CQASSERT(hFile != INVALID_HANDLE_VALUE);

	if (bIOComplete == 0)  // still more data to read
	{
		BOOL bOResult = MSPEECHDIR->GetOverlappedResult(hFile, &overlapped, &bytesRead, 0);

		if (bOResult == 0 && MSPEECHDIR->GetLastError() == ERROR_IO_PENDING)
		{
			result = 0;		// IO pending
			goto Done;
		}
		bIOComplete = true;
	}

	if (bytesRead == bufferSize)
	{
		numElements = 0;

		result = parseFile(pMemory);
		if (ppFrameArray)
			*ppFrameArray = frameArray;

		if (frameArray && result>0)
		{
			DWORD dwRead = PAGE_READWRITE;
			VirtualProtect(frameArray, result * sizeof(U32), PAGE_READONLY, &dwRead);

			free((void *)pMemory);
			pMemory = 0;
			MSPEECHDIR->CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
		}
		else
			result = -1;	// return error
	}
	else
	{
		//
		// test to see if speech is on the CDROM
		//
		char buffer[MAX_PATH+4];

		MSPEECHDIR->GetFileName(buffer, sizeof(buffer));

		if (buffer[1] != ':')
		{
			COMPTR<IFileSystem> parent;
			if (MSPEECHDIR->GetParentSystem(parent) == GR_OK)
			{
				parent->GetFileName(buffer, sizeof(buffer));
			}
		}

		// look for "x:\\path\\"
		buffer[3] = 0;
		if (GetDriveType(buffer) == DRIVE_CDROM)
		{
			if (CQMessageBox(IDS_REINSERT_CDROM, IDS_MEDIA_FAILED, MB_OKCANCEL, NULL))
			{
				// read again
				memset(&overlapped, 0, sizeof(overlapped));
				bytesRead = 0;
				bIOComplete = (MSPEECHDIR->ReadFile(hFile, const_cast<char *>(pMemory), bufferSize, &bytesRead, &overlapped) != 0);
				result = 0;		// IO pending
				goto Done;
			}
		}
	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
inline char LFParser::getChar (const char * & _pMemory)
{
	if (U32(_pMemory - pMemory) < bufferSize)
		return *_pMemory++;
	else
		return 0;
}
//--------------------------------------------------------------------------//
// recursive function
//
U32 LFParser::parseFile (const char * _pMemory)
{
	S32 frameNum=-1;
	char a;
	bool bAlpha;

Top:
	//
	// skip to the next line
	//
	while ((a = getChar(_pMemory)) != 0)
	{
		if (a == 10)
			break;
	}
	if (a==0)
		goto BaseCase;
	
	//
	// find the ','
	//

	while ((a = getChar(_pMemory)) != 0)
	{
		if (a == ',')
			break;
		else
		if (a == 13)
			goto Top;
	}
	if (a==0)
		goto BaseCase;
	//
	// find the number
	//

	bAlpha=false;
	while ((a = getChar(_pMemory)) != 0)
	{
		if (isdigit(a) || a == '<')
			break;
		else
		if (a == 13)
		{
			if (bAlpha)
				goto Top;
			break;
		}
		else
		if (isalpha(a))
			bAlpha = true;
	}
	if (a==0)
		goto BaseCase;
	else
	if (isdigit(a))
	{
		char b;
		b = getChar(_pMemory);
		if (isdigit(b))
			frameNum = ((a - '0')*10) + b - '0';
		else
			frameNum = a - '0';
		numElements++;
		currentFrame=frameNum;
		U32 result = parseFile(_pMemory);
		frameArray[--numElements] = frameNum;
		return result;
	}
	else
	{
		numElements++;
		frameNum = currentFrame;
		U32 result = parseFile(_pMemory);
		frameArray[--numElements] = frameNum;
		return result;
	}

BaseCase:
	if (numElements)
		frameArray = (U32 *)VirtualAlloc(0, numElements * sizeof(U32), MEM_COMMIT, PAGE_READWRITE);
	return numElements;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
GENRESULT __stdcall CreateLFParser (ILFParser ** result)
{
	*result = new DAComponent<LFParser>;
	return GR_OK;
}

//--------------------------------------------------------------------------//
//----------------------------End LFParser.cpp-------------------------------//
//--------------------------------------------------------------------------//
