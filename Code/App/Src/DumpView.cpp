//--------------------------------------------------------------------------//
//                                                                          //
//                              DumpView.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/DumpView.cpp 48    11/17/00 2:07p Jasony $
*/
//--------------------------------------------------------------------------//


#include "pch.h"
#include <globals.h>

#include "Startup.h"
#include "CQTrace.h"
#include "UserDefaults.h"
#include "DBHotkeys.h"

#include <EventSys.h>
#include "Resource.h"
#include <HeapObj.h>
#include <TComponent.h>
#include <WindowManager.h>
#include <FileSys.h>
#include <TSmartPointer.h>

#include <assert.h>
#include <stdlib.h>
#include <comdef.h>
#include <richedit.h>
#include <stdio.h>

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct DumpView : GlobalComponent, IDAComponent
{
	typedef DWORD (__stdcall DumpView::*DUMP_PROC) (LPBYTE pbBuff, LONG cb, LONG FAR *pcb);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	// stuff for cleanup
	HINSTANCE hRichLibrary;
	IDAComponent * _this;

	HWND hDlg, hEdit;
	LONG totalBytesSent;		// offset of space character used for text replacement
	//
	// info used in transfer to edit control
	//
	const char * buffer;
	LONG bufferSize;

	//
	// logfile output
	//
	COMPTR<IFileSystem> dumpFile;

	/* GlobalComponent methods */

	virtual void Startup (void)
	{
		if (CQFLAGS.bDumpWindow)
		{
			hDlg = CreateDialogParam(hResource, MAKEINTRESOURCE(IDD_DIALOG8), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) _dlgProc, (LPARAM) this);
		}

		_this = this;
//		AddToGlobalCleanupList(&_this);
		// delete later (on DLL shutdown)
	}

	virtual void Initialize (void)
	{
		if (CQFLAGS.bDumpFile)
		{
			DAFILEDESC fdesc = "CQDump.txt";

			fdesc.dwDesiredAccess |= GENERIC_WRITE;
			fdesc.dwCreationDistribution = CREATE_ALWAYS;
			fdesc.lpImplementation = "DOS";

			if (DACOM->CreateInstance(&fdesc, dumpFile) == GR_OK)
			{
				HEAP->SetBlockOwner(dumpFile.ptr, 0xFFFFFFFF);
			}
		}

		char buffer[256];
		getVersion(buffer, sizeof(buffer));
		CQTRACE11("Build Version %s", buffer);
	}

	/* IDAComponent methods */

	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance)
	{
		*instance = 0;
		return GR_GENERIC;
	}
	
	DEFMETHOD_(U32,AddRef)           (void)
	{
		return 1;
	}
	
	DEFMETHOD_(U32,Release)          (void)
	{
		WINDOWPLACEMENT loadStruct;

		loadStruct.length = sizeof(loadStruct);

		if (hDlg && GetWindowPlacement(hDlg, &loadStruct))
			DEFAULTS->SetDataInRegistry("dumpDialog", &loadStruct, sizeof(loadStruct));

		dumpFile.free();

		return 1;
	}

	/* DumpView methods */

	DumpView (void)
	{
		hRichLibrary = ::LoadLibrary("RICHED32.DLL");		// also used for assert dialog
		FDUMP = STANDARD_DUMP;
		HEAP_Acquire()->SetErrorHandler(STANDARD_DUMP);
	}

	~DumpView (void);
	
	BOOL dlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	bool sendData (const char * buffer, DWORD length, DWORD dwFlags=SF_TEXT|SFF_SELECTION);
	
	DWORD CALLBACK editStreamCallback (LPBYTE pbBuff, LONG cb, LONG FAR *pcb); 

	static int __cdecl STANDARD_DUMP (ErrorCode code, const C8 *fmt, ...);

	static BOOL CALLBACK _dlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	static void getVersion (char * buffer, U32 bufferSize);
};
static struct DumpView view;
//---------------------------------------------------------------------
//
DumpView::~DumpView (void)
{
	if (hRichLibrary)
		::FreeLibrary(hRichLibrary);
	hRichLibrary = 0;
}
//--------------------------------------------------------------------------//
//
void DumpView::getVersion (char * buffer, U32 bufferSize)
{
	char mname[MAX_PATH+4];
	char * tmp;
	DWORD dwHandle, dwSize;	// unused?
	void * versionBuffer = 0;
	UINT numChars;
	void * pString=0;
	
	*buffer = 0;

	::GetModuleFileName(0, mname, sizeof(mname));
	if ((tmp = strrchr(mname, '\\')) == 0)
		goto Done;
	tmp++;

	if ((dwSize = GetFileVersionInfoSize(tmp, &dwHandle)) == 0)
		goto Done;

	versionBuffer = malloc(dwSize);

	if (GetFileVersionInfo(tmp, dwHandle, dwSize, versionBuffer) == 0)
		goto Done;

	if (VerQueryValue(versionBuffer, const_cast<char *>(_localLoadString(IDS_BUILD_VERSION)), &pString, &numChars) == 0)
		goto Done;

	if (numChars == 0 || pString==0 || bufferSize<=2)
		goto Done;

	if ((numChars+1) > bufferSize)
		numChars = bufferSize-1;

	memcpy(buffer, pString, numChars);
#ifdef _DEMO_
	if (numChars+1 < bufferSize)
	{
		strcat(buffer, "D");
		numChars = strlen(buffer);
	}
#endif
	buffer[numChars] = 0;

Done:
	::free(versionBuffer);
	return;
}
//---------------------------------------------------------------------
//
bool DumpView::sendData (const char * _buffer, DWORD length, DWORD dwFlags)
{
	if (hEdit)
	{
		EDITSTREAM editStream;
		DUMP_PROC _proc = &DumpView::editStreamCallback;
		CHARRANGE charRange;

		// use assembly to avoid type checking here
		memset(&editStream, 0, sizeof(editStream));
		editStream.dwCookie = (DWORD) this;
		__asm
		{
			mov edx, DWORD ptr [_proc]
			mov DWORD PTR [editStream.pfnCallback], edx
		}
		charRange.cpMax = charRange.cpMin = totalBytesSent;		// set caret to the end of the file

		buffer = _buffer;
		bufferSize = length;
		
		SendMessage(hEdit, EM_EXSETSEL, 0, (LPARAM) &charRange);
		SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
		SendMessage(hEdit, EM_STREAMIN, (WPARAM) dwFlags, (LPARAM) & editStream);
	}
	if (dumpFile)
	{
		DWORD dwWritten;
		dumpFile->WriteFile(0, _buffer, length, &dwWritten, 0);
	}

	return (hEdit!=0 || dumpFile!=0);
}
//---------------------------------------------------------------------
//
int DumpView::STANDARD_DUMP (ErrorCode code, const C8 *fmt, ...)
{
	if (code.kind == ERR_MEMORY || code.kind == ERR_NETWORK || code.kind == ERR_MISSION ||
		((code.kind != ERR_RULES       || CQFLAGS.bTraceRules) &&
		(code.kind != ERR_PERFORMANCE || CQFLAGS.bTracePerformance) &&
		(code.kind != ERR_FILE        || CQFLAGS.bInsideCreateInstance || code.severity < SEV_TRACE_1) && 
		TRACELEVEL >= (DWORD)code.severity))
	{

		if (code.kind == ERR_FILE)
		{
			if (CQFLAGS.bInsideCreateInstance)
			{
				code.severity = SEV_WARNING;
				CQFLAGS.bInsideCreateInstance = 0;		// skip further error msgs
			}
			else
			if (code.severity > SEV_WARNING)
				return 0;		// ignore it
		}
		
		// Report the error
		// WARNING: This uses a fixed size buffer.
		char buffer[5100];
		if (code.kind == ERR_ASSERT)
		{
//			_assert((void *)fmt, (void *) *((&fmt)+1), *(((S32 *)(&fmt))+2));
			C8 *expr = const_cast<C8 *>(fmt);
			if (strncmp(fmt, "%s(%d)", 6)==0)	// if not conquest style assertion
				expr = const_cast<C8 *>(*((&fmt)+3));
			wsprintf(buffer, "%s(%d) : CQASSERT(%s)", (void *) *((&fmt)+1), *(((S32 *)(&fmt))+2), expr);
			if (view.sendData(buffer, strlen(buffer)) == 0)
				OutputDebugString (buffer);

			sprintf(buffer, "Assertion failed. (Press Retry to debug)\nExpression=\"%s\"\nSource file=%s\nLine %d",
				expr, (void *) *((&fmt)+1), *(((S32 *)(&fmt))+2) );
		}
		else
		{
			va_list args;
			va_start (args, fmt);
			vsprintf (buffer, fmt, args);
			va_end (args);
		}

		if (code.kind == ERR_MEMORY)
		{
			switch (*(((S32 *)(&fmt))+2))
			{
			case DAHEAP_ALLOC_ZERO:
				code.severity = SEV_TRACE_1;
				// let it go...growl
				break;
			case DAHEAP_VALLOC_FAILED:
				{
					code.severity = SEV_FATAL;
					int len = strlen(buffer);
					MEMORYSTATUS memoryStatus;
					GlobalMemoryStatus(&memoryStatus);
					sprintf(buffer+len, "\r\nPhysical Memory: %d MB, VMemory: %d MB, VAddress: %d MB, HeapSize: %d MB", memoryStatus.dwTotalPhys>>20, memoryStatus.dwAvailPageFile>>20, memoryStatus.dwAvailVirtual>>20,
						HEAP->GetHeapSize() >> 20);
				}
				break;
			default:
				if (code.severity > SEV_WARNING)
					code.severity = SEV_WARNING;
				break;
			}
		}

		// Kill the program in all SEV_FATAL or SEV_ERROR dumps.

		if (code.severity == SEV_FATAL || code.severity == SEV_ERROR || code.severity == SEV_WARNING)
		{
#pragma message ("commented out for demo, generating empty error messages")
//			if (view.sendData("\r\n", 2)==0)
//				OutputDebugString ("\n");
		}
		else
		{
			if (view.sendData(buffer, strlen(buffer)) == 0)
				OutputDebugString (buffer);
		}
	}
	
	return 0;
}
//-------------------------------------------------------------------
//
DWORD DumpView::editStreamCallback (LPBYTE pbBuff, LONG cb, LONG FAR *pcb)
{
	cb = __min(cb, bufferSize);
	memcpy(pbBuff, buffer, cb);
	buffer += cb;
	bufferSize -= cb;
	totalBytesSent += cb;
	*pcb = cb;

	return 0;  //	continue the operation
}
//-------------------------------------------------------------------
//
BOOL DumpView::_dlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	DumpView * _this;

	if (message == WM_INITDIALOG)
		SetWindowLong(hwnd, DWL_USER, lParam);
	
	if ((_this = (DumpView *) GetWindowLong(hwnd, DWL_USER)) != 0)
		return _this->dlgProc(hwnd, message, wParam, lParam);

	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL DumpView::dlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;

	switch (message)
	{
	case WM_INITDIALOG:
		{
			WINDOWPLACEMENT loadStruct;

//			memset(&loadStruct, 0, sizeof(loadStruct));
//			loadStruct.length = sizeof(*this);

			HMENU hMenu = GetSystemMenu(hwnd, 0);
			EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND|MF_DISABLED); 
			hEdit = GetDlgItem(hwnd, IDC_RICHEDIT1);

			if (DEFAULTS->GetDataFromRegistry("dumpDialog", &loadStruct, sizeof(loadStruct)) == sizeof(loadStruct))
			{
				SetWindowPos(hwnd, HWND_TOPMOST,
							 loadStruct.rcNormalPosition.left,
							 loadStruct.rcNormalPosition.top,
							 loadStruct.rcNormalPosition.right - loadStruct.rcNormalPosition.left,
							 loadStruct.rcNormalPosition.bottom - loadStruct.rcNormalPosition.top,
							 SWP_NOZORDER);
				ShowWindow(hwnd, loadStruct.showCmd);
			}
			else
				ShowWindow(hwnd, SW_SHOW);

			result = 1;
		}
		break;

	case WM_SIZE:
		if (hEdit)
		{
			SetWindowPos(hEdit, HWND_TOP, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER);
			SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
		}
		break;

	}

	return result;
}
//--------------------------------------------------------------------------//
//
void __stdcall TrimResetHeap (IHeap * heap)
{
	HEAP = heap;
}
//--------------------------------------------------------------------------//
//-----------------------------End DumpView.cpp-----------------------------//
//--------------------------------------------------------------------------//
