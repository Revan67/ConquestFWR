//--------------------------------------------------------------------------//
//                                                                          //
//                             Parser.cpp                                   //
//                                                                          //
//                  COPYRIGHT (C) 2003 BY Fever Pitch Studios, INC.          //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Ajackson $
*/			    
//--------------------------------------------------------------------------//
//
#include "stdafx.h"
#include "globals.h"

#include "BaseParser.h"
#include "resource.h"
#include "CQTrace.h"
#include "GameTypes.h"

#include <startup.h>
#include <System.h>

#include <TSmartPointer.h>
#include <TComponent.h>
#include <ViewCnst.h>
#include <FileSys.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

struct BaseParser : public IBaseParser
{
	// 
	// interface mapping
	//
	BEGIN_DACOM_MAP_INBOUND(BaseParser)
	DACOM_INTERFACE_ENTRY(IBaseParser)
	END_DACOM_MAP()

	COMPTR<IViewConstructor2> parser;
	HANDLE hPrevSymbols;

	BaseParser();
	~BaseParser();

	//IBaseParser

	virtual GENRESULT CorrelateSymbol (const C8 *pSymbolName, void *pOldData, void *pNewData);

	virtual BOOL32 LoadParseData (IFileSystem * inFile);

	virtual BOOL32 CreateParser (void);

	virtual BOOL32 ParseDefFile (void);

	//BaseParser
	void init();

	void * getPreprocessData (int & resourceSize);

	void buildHeaderData( const char* headerFile );
};
//--------------------------------------------------------------------------//
//
BaseParser::BaseParser()
{
	parser = NULL;
	hPrevSymbols = NULL;
}
//--------------------------------------------------------------------------//
//
BaseParser::~BaseParser()
{
	if (parser)
	{
		if (hPrevSymbols)
			parser->DestroySymbols(hPrevSymbols);
		hPrevSymbols = 0;
//		parser->Release();
	}
	parser = 0;
}
//-------------------------------------------------------------------
//
GENRESULT BaseParser::CorrelateSymbol (const C8 *pSymbolName, void *pOldData, void *pNewData)
{
	SYMBOL oldSymbol, newSymbol;

	oldSymbol = parser->GetSymbol(hPrevSymbols, pSymbolName);
	newSymbol = parser->GetSymbol(0, pSymbolName);

	if (oldSymbol && newSymbol)
	{
		parser->CorrelateSymbol(oldSymbol, pOldData, newSymbol, pNewData);
		return GR_OK;
	}
	else
		return GR_GENERIC;
}
//--------------------------------------------------------------------------
//
BOOL32 BaseParser::LoadParseData (IFileSystem * inFile)
{
	BOOL32 result = 0;
	DAFILEDESC fdesc = "\\ParseData\\Data.h";
	DWORD len, dwRead;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	C8 * pTemp = 0;

	if (hPrevSymbols)
		parser->DestroySymbols(hPrevSymbols);
	hPrevSymbols = 0;
	if ((hFile = inFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
		goto Done;
	len = inFile->GetFileSize(hFile);
	pTemp = new C8[len+1];
	if (pTemp == 0)
		goto Done;
	if (inFile->ReadFile(hFile, (void *)pTemp, len, &dwRead, 0) == 0)
		goto Done;
	pTemp[len] = 0;

	hPrevSymbols = parser->ParseNewMemory(pTemp);

	result=1;
Done:
	if (hFile != INVALID_HANDLE_VALUE)
		inFile->CloseHandle(hFile);
	delete pTemp;

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 BaseParser::CreateParser (void)
{
	if(SYSTEM->QueryInterface("IViewConstructor", (void **)&PARSER) == GR_OK)
	{
		AddToGlobalCleanupList((IDAComponent **) &PARSER);
		return 1;
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL32 BaseParser::ParseDefFile (void)
{
	BOOL32 result=0;

	if (PARSER->QueryInterface("IViewConstructor2", parser) == GR_OK)
	{
		void * pPreprocessBlock;
		char * ptr2;
		int len=0;

		if ((pPreprocessBlock = getPreprocessData(len)) == 0)
			goto Done;

		ptr2 = new char[len + 1];
		memcpy(ptr2, pPreprocessBlock, len);
		ptr2[len] = 0;

		result = (parser->ParseMemory(ptr2) == GR_OK);
		delete ptr2;
		delete pPreprocessBlock;
	}
	else
		result = 0;

Done:
	return result;
}
//--------------------------------------------------------------------------
//
void * BaseParser::getPreprocessData (int & resourceSize)
{
	CString szModuleHandleName = "Globals.dll";
	HANDLE hGlobals = ::LoadLibrary("Globals.dll");
	if( !hGlobals )
	{
		szModuleHandleName = "Z:\\CQ2\\Code\\App\\Src\\Debug\\Globals.dll";
		hGlobals = ::LoadLibrary(szModuleHandleName);
	}

	// use the 'PARSER' data form Globals.dll?
	if( hGlobals )
	{
		HMODULE hGlobalsModule = ::GetModuleHandle(szModuleHandleName);

		HRSRC hRes;

		if ((hRes = FindResource(hGlobalsModule, MAKEINTRESOURCE(IDR_PARSER1), "PARSER")) != 0)
		{
			HGLOBAL hGlobalResource;

			if ((hGlobalResource = LoadResource(hGlobalsModule, hRes)) != 0)
			{
				void * ptr = LockResource(hGlobalResource);
				DWORD resourceSize = SizeofResource(hGlobalsModule, hRes);

				BYTE * data = new BYTE[ resourceSize ];
				memcpy( data, ptr, resourceSize );
				return data;
			}
		}
		return 0;
	}
	else
	{
		HANDLE hFile = CreateFile( "data.i", GENERIC_READ, FILE_SHARE_READ, NULL,OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL );
		
		if( hFile == INVALID_HANDLE_VALUE )
		{
			buildHeaderData("..\\App\\DInclude\\Data.h");
			hFile = CreateFile( "data.i", GENERIC_READ, FILE_SHARE_READ, NULL,OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL );
			if( hFile == INVALID_HANDLE_VALUE )
			{
				return 0;
			}
		}
		
		// Allocate memory to read the vertex shader file
		DWORD dwSize = GetFileSize(hFile, NULL);
		BYTE* pData = new BYTE[dwSize + 4];
		if( NULL == pData )
		{
			CloseHandle( hFile );
			return 0;
		}
		resourceSize = dwSize;
		ZeroMemory( pData, dwSize+4 );
		
		// Read the pre-compiled vertex shader microcode
		DWORD dwNumBytesRead = 0;
		ReadFile(hFile, pData, dwSize, &dwNumBytesRead, 0);
		CloseHandle( hFile );

		return pData;
	}
	
	return NULL;
}
//--------------------------------------------------------------------------
//
void BaseParser::buildHeaderData( const char* headerFile )
{
	STARTUPINFO info;
	BOOL32 result;
	char commandLine[128];
	PROCESS_INFORMATION processInfo;
	
	memset(&info, 0, sizeof(info));
	info.cb = sizeof(info);
	
	strcpy(commandLine, "cl /P /EP /nologo ");
	strcat(commandLine, headerFile);
	
	info.dwFlags = STARTF_USESTDHANDLES;
	info.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	info.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

	result = CreateProcess(
		0, 
		commandLine, 
		0, 
		0,
		1,	// bInheritHandles
		NORMAL_PRIORITY_CLASS | DETACHED_PROCESS,
		0, 
		0, 
		&info, 
		&processInfo);
	
	if (result)
	{
		WaitForSingleObject(processInfo.hProcess, INFINITE);
		
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
}
//--------------------------------------------------------------------------//
//
void BaseParser::init()
{
	if (CreateParser() == 0 || ParseDefFile() == 0)
	{
		CQBOMB0("Failed parse of the Data definition file: ..shared\\dinclude\\Data.h");
	}
}
//--------------------------------------------------------------------------//
//
struct _parser : GlobalComponent
{
	BaseParser * baseParser;

	virtual void Startup (void)
	{
		BASEPARSER = baseParser = new DAComponent<BaseParser>;
		baseParser->init();
		AddToGlobalCleanupList((IDAComponent **)&BASEPARSER);
	}

	virtual void Initialize (void)
	{
	}
};

static _parser __parser;