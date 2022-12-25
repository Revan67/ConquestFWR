#ifndef VIEWCNST_H
#define VIEWCNST_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               ViewCnst.H                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $



					View Contructor, parses C/C++ header, 
					

*/			    
//--------------------------------------------------------------------------//


#ifndef DACOM_H
#include "DACOM.h"
#endif

typedef void *HANDLE;
struct Symbol;
typedef Symbol *SYMBOL;

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
// Enumerate callback function. Return TRUE to continue the enumeration

typedef BOOL32  (__stdcall * IVIEWCNST_ENUM_PROC) (struct IViewConstructor * cnst, 
												   const C8 *typeName,
												   void *context);

typedef BOOL32  (__stdcall * IVIEWCNST_ENUM_PROC2) (struct IViewConstructor2 * cnst, 
												    const C8 *typeName,
													SYMBOL symbol,
												    void *context);

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE __baseViewConstructor : public IComponentFactory
{
	DEFMETHOD(ParseFile) (const C8 *filename) = 0;

	DEFMETHOD(ParseMemory) (const C8 *memory) = 0;

	// inline implementation

#ifdef _INC_WINDOWS
	GENRESULT FullParseFile (const C8 *filename);

	static BOOL32 PreprocessFile (const C8 *fileIn, HANDLE hFileOut);

	static HANDLE CreateTempFile (void);
#endif
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//

#define IID_IViewConstructor MAKE_IID("IViewConstructor",1)
//
struct DACOM_NO_VTABLE IViewConstructor : public __baseViewConstructor
{
	DEFMETHOD(ParseFile) (const C8 *filename) = 0;

	DEFMETHOD(ParseMemory) (const C8 *memory) = 0;

	DEFMETHOD_(BOOL32,EnumerateTypes) (IVIEWCNST_ENUM_PROC proc, void *context=0) = 0;

	DEFMETHOD_(U32,GetTypeSize) (const C8 *typeName) = 0;
};
//--------------------------------------------------------------------------//
//
/* 
   1) Build new symbol table
   2) Get/save new symbols
   3) Build old symbol table
   4) correlate symbols
   5) Destroy old symbols
   6) Restore new symbol table
 */

#define IID_IViewConstructor2 MAKE_IID("IViewConstructor2",1)
//
struct DACOM_NO_VTABLE IViewConstructor2 : public __baseViewConstructor
{
	DEFMETHOD(ParseFile) (const C8 *filename) = 0;

	DEFMETHOD(ParseMemory) (const C8 *memory) = 0;

	DEFMETHOD_(BOOL32,EnumerateTypes) (IVIEWCNST_ENUM_PROC2 proc, void *context=0) = 0;

	DEFMETHOD_(U32,GetTypeSize) (SYMBOL symbol) = 0;

	DEFMETHOD_(HANDLE,ParseNewMemory) (const C8 *memory) = 0;	// create a secondary symbol table

	DEFMETHOD_(SYMBOL,GetSymbol) (HANDLE hSymbolList, const C8 *pszSymbolName) = 0;
	
	DEFMETHOD(DestroySymbols) (HANDLE hSymbolList) = 0;

	DEFMETHOD_(void,CorrelateSymbol) (SYMBOL oldSymbol, void *pOldData, SYMBOL newSymbol, void *pNewData) = 0;

	DEFMETHOD_(BOOL32,IsEqual) (SYMBOL oldSymbol, SYMBOL newSymbol) = 0;

	DEFMETHOD_(BOOL32,HasVariableSize) (SYMBOL symbol) = 0;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifdef _INC_WINDOWS

#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

inline BOOL32 __baseViewConstructor::PreprocessFile (const C8 *fileIn, HANDLE hFileOut)
{
	STARTUPINFO info;
	BOOL32 result;
	char commandLine[128];
	PROCESS_INFORMATION processInfo;

	memset(&info, 0, sizeof(info));
	info.cb = sizeof(info);

	strcpy_s(commandLine, "cl /EP /nologo ");
	strcat_s(commandLine, fileIn);

	info.dwFlags = STARTF_USESTDHANDLES;
	info.hStdOutput = hFileOut;
	info.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);


	result = CreateProcess(0, commandLine, 0, 0,
			1,	// bInheritHandles
			NORMAL_PRIORITY_CLASS | DETACHED_PROCESS,
			0, 0, &info, &processInfo);

	if (result)
	{
		WaitForSingleObject(processInfo.hProcess, INFINITE);

		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}

	return result;
}
//--------------------------------------------------------------------------
//
inline HANDLE __baseViewConstructor::CreateTempFile (void)
{
	char path[MAX_PATH+4];
	char tempname[MAX_PATH+4];
	DWORD dwPrefix = '\0pmt';

	GetTempPath(MAX_PATH, path);	

	if (GetTempFileName(path, (const char *)&dwPrefix, 0, tempname) == 0)
		return 0;

 	return ::CreateFile(tempname, 
							GENERIC_READ|GENERIC_WRITE,
				             0,
					         0,
	                         OPEN_EXISTING,
	                         FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,
						     0);
}
//--------------------------------------------------------------------------
//
inline GENRESULT __baseViewConstructor::FullParseFile (const C8 *filename)
{
	GENRESULT result=GR_GENERIC;
	HANDLE hTemp;
	void *pMemory;
	DWORD dwFileSize, dwBytesRead;

	if ((hTemp = CreateTempFile()) == INVALID_HANDLE_VALUE)
		goto Done;

	if (PreprocessFile(filename, hTemp) == 0)
		goto Done;

	SetFilePointer(hTemp, 0, 0, FILE_BEGIN);
	if ((dwFileSize = GetFileSize(hTemp, 0)) == 0)
		goto Done;

	if ((pMemory = malloc(dwFileSize+1)) == 0)
		goto Done;

	memset(pMemory, 0, dwFileSize+1);
	if (ReadFile(hTemp, pMemory, dwFileSize, &dwBytesRead, 0))
		result = ParseMemory((const C8 *)pMemory);

	free(pMemory);

Done:
	if (hTemp != INVALID_HANDLE_VALUE)
		CloseHandle(hTemp);
	return result;
}
#endif    // #ifdef _INC_WINDOWS


#endif

//--------------------------------------------------------------------------//
//----------------------------End ViewCnst.h--------------------------------//
//--------------------------------------------------------------------------//
