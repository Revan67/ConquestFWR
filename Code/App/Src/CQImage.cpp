//--------------------------------------------------------------------------//
//                                                                          //
//                               CQImage.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/CQImage.cpp 37    11/09/00 11:52a Jasony $

*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>

#include "CQImage.h"
#include "Startup.h"
#include "CQTrace.h"
#include "Resource.h"
#include "EventSys.h"
#include "DBHotkeys.h"

#include "ObjmapIterator.h"

#include <HeapObj.h>
#include <WindowManager.h>
#include <TSmartPointer.h>
#include <TComponent.h>

#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <imagehlp.h>
#include <commctrl.h>

#define ERROR_SND "AppGPFault"
#define INFO_SND  "SystemExclamation"

static const char errorMsg[] =	"The program encountered an expected problem.\r\nInformation specific to this problem can be found in the \"Error.txt\" file.";
static const char errorTitle[] = "Program Error";

//--------------------------------------------------------------------------//
//
struct STACK_FRAME
{
	struct STACK_FRAME *pNext;
	DWORD  dwRetAddr;
};
//--------------------------------------------------------------------------//
//
enum CQERROR_TYPE
{
	CQERR_REPORT=1,
	CQERR_FATAL_EXCEPTION,
	CQERR_ERROR,
	CQERR_ASSERT,
	CQERR_EXCEPTION,
	CQERR_BOMB
};
//--------------------------------------------------------------------------//
//
template <int Size> 
struct TEXT_BUFFER
{
	char * buffer;
	int index;
	CQERROR_TYPE type;

	TEXT_BUFFER (void)
	{
		buffer = (char *) VirtualAlloc(0, Size, MEM_COMMIT, PAGE_READWRITE);
		index = 0;
	}

	~TEXT_BUFFER (void)
	{
		VirtualFree(buffer, 0, MEM_RELEASE);
		buffer = 0;
	}

	TEXT_BUFFER & operator = (const TEXT_BUFFER<Size> & text)
	{
		memcpy(buffer, text.buffer, Size);
		index = text.index;
		return *this;
	}

	void addText (const char * pText, int len)
	{
		const int imax = Size - index - 1;
		len = __min(imax, len);

		memcpy(buffer+index, pText, len);
		index += len;
	}

	void addText (const char * pText)
	{
		addText(pText, strlen(pText));
	}
};

static bool CQImage_bEnabled = true;

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct CQImage : public ICQImage
{
	BEGIN_DACOM_MAP_INBOUND(CQImage)
	DACOM_INTERFACE_ENTRY(ICQImage)
	END_DACOM_MAP()

	typedef BOOL  (__stdcall * SYMINIT) (IN HANDLE hProcess, IN LPSTR UserSearchPath, IN BOOL fInvadeProcess);
	typedef BOOL  (__stdcall * SYMCLEANUP) (IN HANDLE hProcess);
	typedef BOOL  (__stdcall * SYMLOADMODULE)  (IN  HANDLE          hProcess,
												IN  HANDLE          hFile,
												IN  PSTR            ImageName,
												IN  PSTR            ModuleName,
												IN  DWORD           BaseOfDll,
												IN  DWORD           SizeOfDll
												);
	typedef BOOL  (__stdcall * SYMGETLINEFROMADDR) 
												(IN  HANDLE                  hProcess,
												 IN  DWORD                   dwAddr,
												 OUT PDWORD                  pdwDisplacement,
												 OUT PIMAGEHLP_LINE          Line
												);
	typedef DWORD (__stdcall * SYMSETOPTIONS) (IN DWORD   SymOptions);

	typedef BOOL  (__stdcall * SYMGETMODULEINFO) (
													IN  HANDLE              hProcess,
													IN  DWORD               dwAddr,
													OUT PIMAGEHLP_MODULE    ModuleInfo
													);
	typedef BOOL  (__stdcall * SYMGETSYMFROMADDR) (
													IN  HANDLE              hProcess,
													IN  DWORD               dwAddr,
													OUT PDWORD              pdwDisplacement,
													OUT PIMAGEHLP_SYMBOL    Symbol
													);
	typedef BOOL  (__stdcall * SYMUNLOADMODULE) (
												IN  HANDLE          hProcess,
												IN  DWORD           BaseOfDll
												);


	//------------------------
	//
	HANDLE hProcess;
	SYMINIT SymInitialize;
	SYMCLEANUP SymCleanup;
	SYMLOADMODULE SymLoadModule;
	SYMGETLINEFROMADDR SymGetLineFromAddr;
	SYMSETOPTIONS SymSetOptions;
	SYMGETMODULEINFO SymGetModuleInfo;
	SYMGETSYMFROMADDR SymGetSymFromAddr;
	SYMUNLOADMODULE SymUnloadModule;
	HINSTANCE hInstance;

	CQImage (void);

	~CQImage (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ICQImage methods */

	virtual void LoadSymTable (HINSTANCE hInstance);

	virtual void UnloadSymTable (HINSTANCE hInstance);

	virtual void Report (const char * szInfo);

	virtual void MemoryReport (struct IHeap * heap);

	virtual void SetMessagesEnabled(bool bSetting){ CQImage_bEnabled = bSetting; }

	/* CQImage methods */

static long __stdcall PrintBlock (struct IHeap * pHeap, void *allocatedBlock, U32 dwFlags, void *context);

	IDAComponent * getBase (void)
	{
		return static_cast<ICQImage *>(this);
	}

	void init (void);
};
static DAComponent<struct CQImage> image;
//--------------------------------------------------------------------------//
//
CQImage::CQImage (void)
{
	CQIMAGE = this;
	init();
}
//--------------------------------------------------------------------------//
//
void CQImage::init (void)
{
#ifndef FINAL_RELEASE

	hInstance = LoadLibrary("DbgHelp.dll");
	if (hInstance)
	{
		if ((SymInitialize = (SYMINIT) GetProcAddress(hInstance, "SymInitialize")) == 0)
			goto Done;
		if ((SymCleanup = (SYMCLEANUP) GetProcAddress(hInstance, "SymCleanup")) == 0)
			goto Done;
		if ((SymLoadModule = (SYMLOADMODULE) GetProcAddress(hInstance, "SymLoadModule")) == 0)
			goto Done;
		if ((SymUnloadModule = (SYMUNLOADMODULE) GetProcAddress(hInstance, "SymUnloadModule")) == 0)
			goto Done;
		if ((SymGetLineFromAddr = (SYMGETLINEFROMADDR) GetProcAddress(hInstance, "SymGetLineFromAddr")) == 0)
			goto Done;
		if ((SymSetOptions = (SYMSETOPTIONS) GetProcAddress(hInstance, "SymSetOptions")) == 0)
			goto Done;
		if ((SymGetModuleInfo = (SYMGETMODULEINFO) GetProcAddress(hInstance, "SymGetModuleInfo")) == 0)
			goto Done;
		if ((SymGetSymFromAddr = (SYMGETSYMFROMADDR) GetProcAddress(hInstance, "SymGetSymFromAddr")) == 0)
			goto Done;
	
//		hProcess = (HANDLE) this;
		hProcess = (HANDLE) GetCurrentProcessId();

		char path[MAX_PATH+4];
		GetModuleFileName(NULL, path, sizeof(path));
		char * ptr;

		if ((ptr = strrchr(path, '\\')) != 0)
			*ptr = 0;		// just use the path

		if (SymInitialize(hProcess, path, FALSE) == 0)
			hProcess = 0;
		else
		{
			SymSetOptions(SYMOPT_LOAD_LINES|SYMOPT_UNDNAME);	//SYMOPT_DEFERRED_LOADS);
		}
		GetLastError();

		if (hProcess)
		{
			LoadSymTable(GetModuleHandle("Conquest.exe"));
			LoadSymTable(GetModuleHandle("Mission.dll"));
			LoadSymTable(GetModuleHandle("Trim.dll"));
			LoadSymTable(GetModuleHandle("ZBatcher.dll"));
			LoadSymTable(GetModuleHandle("DACOM.dll"));
		}
	}

Done:
	return;

#endif  // end !FINAL_RELEASE

}
//--------------------------------------------------------------------------//
//
CQImage::~CQImage (void)
{
	if (hProcess)
	{
		SymCleanup(hProcess);
		hProcess = 0;
	}
	if (hInstance)
	{
		FreeLibrary(hInstance);
		hInstance = 0;
	}

	CQIMAGE = 0;
}
//--------------------------------------------------------------------------//
//
void CQImage::LoadSymTable (HINSTANCE hInstance)
{
#ifndef FINAL_RELEASE
	if (hProcess)
	{
		char imageName[MAX_PATH+4];

		if (GetModuleFileName(hInstance, imageName, sizeof(imageName)))
		{
			imageName[sizeof(imageName)-1] = 0;

			SymLoadModule(hProcess, NULL, imageName, NULL, DWORD(hInstance), 0);
		}
	}
#endif  // end !FINAL_RELEASE
}
//--------------------------------------------------------------------------//
//
void CQImage::UnloadSymTable (HINSTANCE hInstance)
{
#ifndef FINAL_RELEASE
	if (hProcess)
	{
		SymUnloadModule(hProcess, DWORD(hInstance));
	}
#endif  // end !FINAL_RELEASE
}
#ifndef FINAL_RELEASE
//--------------------------------------------------------------------------//
//
static void restorePriority (void)
{
	if (CQFLAGS.b3DEnabled && CQFLAGS.bNoGDI)
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	else
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
}
//--------------------------------------------------------------------------//
//
static void getVersion (char * buffer, U32 bufferSize)
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
//--------------------------------------------------------------------------//
//
static void initStatic (HWND hwnd)
{
	char format[128];
	char version[128];
	char buffer[128];

	GetWindowText(hwnd, format, sizeof(format));
	getVersion(version, sizeof(version));
	sprintf(buffer, format, version);

	SetWindowText(hwnd, buffer);
}
//--------------------------------------------------------------------------//
//
static BOOL CALLBACK dlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;

	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND hItem;
			const TEXT_BUFFER<4096> * pText = (const TEXT_BUFFER<4096> *) lParam;
			//if (hMainWindow==0)
//			SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);

			hItem = GetDlgItem(hwnd, IDC_STATIC_VERSION);
			initStatic(hItem);

			switch (pText->type)
			{
			case CQERR_REPORT:
				SetWindowText(hwnd, "Report");
				EnableWindow(GetDlgItem(hwnd, IDABORT), 0);
				EnableWindow(GetDlgItem(hwnd, IDDEBUG), 0);
				break;
			case CQERR_FATAL_EXCEPTION:
				SetWindowText(hwnd, "Exception");
				EnableWindow(GetDlgItem(hwnd, IDIGNORE), 0);
				break;
			case CQERR_ERROR:
				SetWindowText(hwnd, "Recoverable Error");
				break;
			case CQERR_EXCEPTION:
				SetWindowText(hwnd, "Exception");
				break;
			case CQERR_BOMB:
				SetWindowText(hwnd, "Bomb");
				break;
			} // end switch
			hItem = GetDlgItem(hwnd, IDC_EDIT1);
			SetWindowText(hItem, pText->buffer);
			SetFocus(hItem);
		}
		break;
	
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			if (IsWindowEnabled(GetDlgItem(hwnd, IDIGNORE)) == 0)
				break;
			else
				wParam = IDIGNORE;
			// fall through intentional
		case IDDEBUG:
		case IDABORT:
		case IDIGNORE:
			EndDialog(hwnd, LOWORD(wParam));
			break;
		}
		break;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
#if 0
static void writeError (const TEXT_BUFFER<4096> & text)
{
	HANDLE hFile = CreateFile("Error.txt", GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwWritten;
		WriteFile(hFile, text.buffer, text.index, &dwWritten, NULL);
		CloseHandle(hFile);
	}
}
#endif  // end if 0
#endif  // end !FINAL_RELEASE
//--------------------------------------------------------------------------//
//
void CQImage::Report (const char * szInfo)
{
#ifndef FINAL_RELEASE
	
	TEXT_BUFFER<4096> text;

	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	text.type = CQERR_REPORT;
	PlaySound(INFO_SND, NULL, SND_ALIAS | SND_ASYNC);
	text.addText("Report: ");
	text.addText(szInfo);

	FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), szInfo);

	FlipToGDI();
	DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG13), hMainWindow, dlgProc, (LPARAM) &text);
	restorePriority();

#endif  // end !FINAL_RELEASE
}
//--------------------------------------------------------------------------//
//

#define OPPRINT0(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp)
//--------------------------------------------------------------------------//
//
bool ICQImage::Assert (const char *exp, void *file, unsigned line)
{
#ifndef FINAL_RELEASE

	if( !CQImage_bEnabled )
	{
		// dont take care of alert message (danger!)
		return false;
	}

	if (image.hProcess)
	{
		STACK_FRAME * pFrame;
		char buffer[1024];
		int i;
		DWORD dwDisp;
		TEXT_BUFFER<4096> text;
		char version[64];
		getVersion(version, sizeof(version));

		__asm mov DWORD ptr [pFrame], ebp

		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

		text.type = CQERR_ASSERT;
		if (CQFLAGS.bTraceMission != 0 && EVENTSYS)
			EVENTSYS->Send(CQE_DEBUG_HOTKEY, (void *) IDH_PRINT_OPLIST);		// print the op list
		PlaySound(ERROR_SND, NULL, SND_ALIAS | SND_ASYNC);

		sprintf(buffer, "Expression: %s [%s]\r\n", exp, version);
		text.addText(buffer);
		text.addText("Call Stack:\r\n");

		for (i = 0; i < 8; i++)
		{
			if (IsBadReadPtr(pFrame, sizeof(STACK_FRAME)) == 0)
			{
				bool bLineValid=false;
				IMAGEHLP_LINE *iLine = (IMAGEHLP_LINE *) buffer;
				memset(iLine, 0, sizeof(*iLine));
				iLine->SizeOfStruct = sizeof(*iLine);
				dwDisp = 0;

				if (image.SymGetLineFromAddr(image.hProcess, pFrame->dwRetAddr, &dwDisp, iLine))
				{
					bLineValid = true;
					char * ptr = strrchr(iLine->FileName, '\\');
					if (ptr)
						ptr++;
					else
						ptr = iLine->FileName;
					text.addText(ptr);
					sprintf(buffer, ", Line %d", iLine->LineNumber);
					text.addText(buffer);
				}
				GetLastError();
				
				
				IMAGEHLP_SYMBOL *iSymbol = (IMAGEHLP_SYMBOL *) buffer;
				memset(iSymbol, 0, sizeof(*iSymbol));
				iSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
				iSymbol->MaxNameLength = sizeof(buffer) - sizeof(IMAGEHLP_SYMBOL);
				dwDisp = 0;

 				if (image.SymGetSymFromAddr(image.hProcess, pFrame->dwRetAddr, &dwDisp, iSymbol))
				{
					if (bLineValid)
						text.addText(", ");
					text.addText(iSymbol->Name);
					if (bLineValid)
						text.addText("()\r\n");
					else
					{
						sprintf(buffer, " + %d bytes\r\n", dwDisp);
						text.addText(buffer);
					}
				}

				pFrame = pFrame->pNext;
			}
			else
				break;	// stop if invalid address
		}

		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "-------------------------------------------------\r\nAssertion Failed: \r\n");
		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), text.buffer);
		int result = IDIGNORE;

		FlipToGDI();
		result = DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG13), hMainWindow, dlgProc, (LPARAM) &text);

		restorePriority();

		if (result == IDABORT)
		{
			CQFLAGS.bNoExitConfirm = 1;
			PostQuitMessage(-1);		// cannot use exit(-1) here
			if (WM)
				WM->ServeMessageQueue();
		}
		else
		if (result == IDDEBUG)		// DEBUG chosen
			return true;
		
		return false;
	}

	FDUMP(ErrorCode(ERR_ASSERT, SEV_FATAL), exp, file, line);

#endif  // end !FINAL_RELEASE

	return 0;
}
//--------------------------------------------------------------------------//
//
bool __cdecl ICQImage::Bomb (const char *exp, ...)
{
#ifndef FINAL_RELEASE

	if( !CQImage_bEnabled )
	{
		// dont take care of alert message (danger!)
		return false;
	}
	
	char buffer[1024];

	va_list args;
	va_start (args, exp);
	vsprintf (buffer, exp, args);
	va_end (args);

	if (image.hProcess)
	{
		STACK_FRAME * pFrame;
		int i;
		DWORD dwDisp;
		TEXT_BUFFER<4096> text;
		char version[64];
		getVersion(version, sizeof(version));

		__asm mov DWORD ptr [pFrame], ebp

		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

		text.type = CQERR_BOMB;
		if (CQFLAGS.bTraceMission != 0 && EVENTSYS)
			EVENTSYS->Send(CQE_DEBUG_HOTKEY, (void *) IDH_PRINT_OPLIST);		// print the op list
		PlaySound(ERROR_SND, NULL, SND_ALIAS | SND_ASYNC);
	
		{
			char * ptr = buffer;
			if (ptr[0] && ptr[1] == ':')
				ptr += 2;
			ptr = strchr(ptr, ':');
			if (ptr)
				ptr += 2;
			else
				ptr = buffer;
			text.addText("Error: ");
			text.addText(ptr);
			sprintf(buffer, " [%s]\r\n", version);
			text.addText(buffer);
			text.addText("Call Stack:\r\n");
		}

		for (i = 0; i < 8; i++)
		{
			if (IsBadReadPtr(pFrame, sizeof(STACK_FRAME)) == 0)
			{
				bool bLineValid=false;
				IMAGEHLP_LINE *iLine = (IMAGEHLP_LINE *) buffer;
				memset(iLine, 0, sizeof(*iLine));
				iLine->SizeOfStruct = sizeof(*iLine);
				dwDisp = 0;

				if (image.SymGetLineFromAddr(image.hProcess, pFrame->dwRetAddr, &dwDisp, iLine))
				{
					bLineValid = true;
					char * ptr = strrchr(iLine->FileName, '\\');
					if (ptr)
						ptr++;
					else
						ptr = iLine->FileName;
					text.addText(ptr);
					sprintf(buffer, ", Line %d", iLine->LineNumber);
					text.addText(buffer);
				}
				GetLastError();
				
				
				IMAGEHLP_SYMBOL *iSymbol = (IMAGEHLP_SYMBOL *) buffer;
				memset(iSymbol, 0, sizeof(*iSymbol));
				iSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
				iSymbol->MaxNameLength = sizeof(buffer) - sizeof(IMAGEHLP_SYMBOL);
				dwDisp = 0;

 				if (image.SymGetSymFromAddr(image.hProcess, pFrame->dwRetAddr, &dwDisp, iSymbol))
				{
					if (bLineValid)
						text.addText(", ");
					text.addText(iSymbol->Name);
					if (bLineValid)
						text.addText("()\r\n");
					else
					{
						sprintf(buffer, " + %d bytes\r\n", dwDisp);
						text.addText(buffer);
					}
				}

				pFrame = pFrame->pNext;
			}
			else
				break;	// stop if invalid address
		}

		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "-------------------------------------------------\r\nBomb: \r\n");
		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), text.buffer);
		int result = IDABORT;
		
		FlipToGDI();
		result = DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG13), hMainWindow, dlgProc, (LPARAM) &text);

		restorePriority();

		if (result == IDABORT)
		{
			CQFLAGS.bNoExitConfirm = 1;
			PostQuitMessage(-1);		// cannot use exit(-1) here
			if (WM)
				WM->ServeMessageQueue();
		}
		else
		if (result == IDDEBUG)		// DEBUG chosen
			return true;
		
		return false;
	}

	FDUMP(ErrorCode(ERR_GENERAL, SEV_FATAL), buffer);

#endif  // end !FINAL_RELEASE
	
	return 0;
}
//--------------------------------------------------------------------------//
//
bool __cdecl ICQImage::Error (const char *exp, ...)
{
#ifndef FINAL_RELEASE

	if( !CQImage_bEnabled )
	{
		// dont take care of alert message (danger!)
		return false;
	}

	char buffer[1024];

	va_list args;
	va_start (args, exp);
	vsprintf (buffer, exp, args);
	va_end (args);

	if (image.hProcess)
	{
		STACK_FRAME * pFrame;
		int i;
		DWORD dwDisp;
		TEXT_BUFFER<4096> text;
		char version[64];
		getVersion(version, sizeof(version));

		__asm mov DWORD ptr [pFrame], ebp

		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

		text.type = CQERR_ERROR;
		PlaySound(INFO_SND, NULL, SND_ALIAS | SND_ASYNC);

		{
			char * ptr = buffer;
			if (ptr[0] && ptr[1] == ':')
				ptr += 2;
			ptr = strchr(ptr, ':');
			if (ptr)
				ptr += 2;
			else
				ptr = buffer;
			text.addText("Error: ");
			text.addText(ptr);
			sprintf(buffer, " [%s]\r\n", version);
			text.addText(buffer);
			text.addText("Call Stack:\r\n");
		}

		for (i = 0; i < 8; i++)
		{
			if (IsBadReadPtr(pFrame, sizeof(STACK_FRAME)) == 0)
			{
				bool bLineValid=false;
				IMAGEHLP_LINE *iLine = (IMAGEHLP_LINE *) buffer;
				memset(iLine, 0, sizeof(*iLine));
				iLine->SizeOfStruct = sizeof(*iLine);
				dwDisp = 0;

				if (image.SymGetLineFromAddr(image.hProcess, pFrame->dwRetAddr, &dwDisp, iLine))
				{
					bLineValid = true;
					char * ptr = strrchr(iLine->FileName, '\\');
					if (ptr)
						ptr++;
					else
						ptr = iLine->FileName;
					text.addText(ptr);
					sprintf(buffer, ", Line %d", iLine->LineNumber);
					text.addText(buffer);
				}
				GetLastError();
				
				
				IMAGEHLP_SYMBOL *iSymbol = (IMAGEHLP_SYMBOL *) buffer;
				memset(iSymbol, 0, sizeof(*iSymbol));
				iSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
				iSymbol->MaxNameLength = sizeof(buffer) - sizeof(IMAGEHLP_SYMBOL);
				dwDisp = 0;

 				if (image.SymGetSymFromAddr(image.hProcess, pFrame->dwRetAddr, &dwDisp, iSymbol))
				{
					if (bLineValid)
						text.addText(", ");
					text.addText(iSymbol->Name);
					if (bLineValid)
						text.addText("()\r\n");
					else
					{
						sprintf(buffer, " + %d bytes\r\n", dwDisp);
						text.addText(buffer);
					}
				}

				pFrame = pFrame->pNext;
			}
			else
				break;	// stop if invalid address
		}

		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "-------------------------------------------------\r\nRecoverable Error: \r\n");
		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), text.buffer);
		int result = IDIGNORE;
		
		FlipToGDI();
		result = DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG13), hMainWindow, dlgProc, (LPARAM) &text);

		restorePriority();

		if (result == IDABORT)
		{
			CQFLAGS.bNoExitConfirm = 1;
			PostQuitMessage(-1);		// cannot use exit(-1) here
			if (WM)
				WM->ServeMessageQueue();
		}
		else
		if (result == IDDEBUG)		// DEBUG chosen
			return true;
		
		return false;
	}

	FDUMP(ErrorCode(ERR_GENERAL, SEV_ERROR), buffer);

#endif  // end !FINAL_RELEASE
	
	return 0;
}
//--------------------------------------------------------------------------//
//
int ICQImage::Exception (struct _EXCEPTION_POINTERS * exceptionInfo)
{
#ifndef FINAL_RELEASE

	if( !CQImage_bEnabled )
	{
		// dont take care of alert message (danger!)
		return false;
	}

	const CONTEXT * pContext = exceptionInfo->ContextRecord;

	if (image.hProcess && (pContext->ContextFlags & CONTEXT_CONTROL) != 0)
	{
		STACK_FRAME firstFrame;
		STACK_FRAME * pFrame;
		char buffer[1024];
		int i;
		DWORD dwDisp;
		TEXT_BUFFER<4096> text;
		bool bFatal = (exceptionInfo->ExceptionRecord->ExceptionFlags != 0);
		char version[64];
		getVersion(version, sizeof(version));

		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

		PlaySound(ERROR_SND, NULL, SND_ALIAS | SND_ASYNC);

		text.type = CQERR_EXCEPTION;
		firstFrame.dwRetAddr = pContext->Eip;
		firstFrame.pNext = (STACK_FRAME *) pContext->Ebp;
	
		pFrame = &firstFrame;

		if (bFatal)
		{
			text.addText("Fatal ");
			text.type = CQERR_FATAL_EXCEPTION;
		}
		text.addText("Exception: ");
		{
			const char * name = "??Unknown type??";

			switch (exceptionInfo->ExceptionRecord->ExceptionCode)
			{
			case EXCEPTION_ACCESS_VIOLATION:
				name = "Access Violation";
				break;
			case EXCEPTION_BREAKPOINT:
				name = "User Breakpoint";
				break;
			case EXCEPTION_FLT_DIVIDE_BY_ZERO:
				name = "Float Div by Zero";
				break;
			case EXCEPTION_FLT_INVALID_OPERATION:
				name = "Float Invalid Op";
				break;
			case EXCEPTION_FLT_DENORMAL_OPERAND:
			case EXCEPTION_FLT_INEXACT_RESULT:
			case EXCEPTION_FLT_OVERFLOW:
			case EXCEPTION_FLT_STACK_CHECK:
			case EXCEPTION_FLT_UNDERFLOW:
				name = "FPU";
				break;
			case EXCEPTION_PRIV_INSTRUCTION:
			case EXCEPTION_ILLEGAL_INSTRUCTION:
				name = "Illegal Instruction";
				break;
			case EXCEPTION_IN_PAGE_ERROR:
				name = "Paging error";
				break;
			case EXCEPTION_INT_DIVIDE_BY_ZERO:
				name = "Integer Div by Zero";
				break;
			case EXCEPTION_INT_OVERFLOW:
				name = "Integer overflow";
				break;
			case EXCEPTION_NONCONTINUABLE_EXCEPTION:
				name = "Noncontinuable";
				break;
			case EXCEPTION_SINGLE_STEP:
				name = "Single Step";
				break;
			case EXCEPTION_STACK_OVERFLOW:
				name = "Stack overflow";
				break;
			}
			
			text.addText(name);
			sprintf(buffer, " [%s]\r\n", version);
			text.addText(buffer);
		}

		//  oh no! exception during an iterator
		if (DEBUG_ITERATOR)
		{
			text.addText("Live iterator:\r\n");
			if (IsBadReadPtr(DEBUG_ITERATOR, sizeof(ObjMapIterator)) == 0)
			{
				sprintf(buffer, "   current [0x%08X] -> ", DEBUG_ITERATOR->current);
				text.addText(buffer);
				if (IsBadReadPtr(DEBUG_ITERATOR->current, sizeof(ObjMapNode)) == 0)
				{
					sprintf(buffer, "obj=0x%08X, dwMissionID=0x%X, flags=0x%X, next=0x%08X\r\n", 
						DEBUG_ITERATOR->current->obj, DEBUG_ITERATOR->current->dwMissionID,
						DEBUG_ITERATOR->current->flags, DEBUG_ITERATOR->current->next);
					text.addText(buffer);
				}
				else
					text.addText("  ???\r\n");
			}
			else
				text.addText("  ???\r\n");
		}

		text.addText("Call Stack:\r\n");

		for (i = 0; i < 8; i++)
		{
			if (IsBadReadPtr(pFrame, sizeof(STACK_FRAME)) == 0)
			{
				bool bLineValid=false;
				bool bSymValid=false;
				IMAGEHLP_LINE *iLine = (IMAGEHLP_LINE *) buffer;
				memset(iLine, 0, sizeof(*iLine));
				iLine->SizeOfStruct = sizeof(*iLine);
				dwDisp = 0;

				if (image.SymGetLineFromAddr(image.hProcess, pFrame->dwRetAddr, &dwDisp, iLine))
				{
					bLineValid = true;
					char * ptr = strrchr(iLine->FileName, '\\');
					if (ptr)
						ptr++;
					else
						ptr = iLine->FileName;
					text.addText(ptr);
					sprintf(buffer, ", Line %d", iLine->LineNumber);
					text.addText(buffer);
				}
				GetLastError();
				
				
				IMAGEHLP_SYMBOL *iSymbol = (IMAGEHLP_SYMBOL *) buffer;
				memset(iSymbol, 0, sizeof(*iSymbol));
				iSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
				iSymbol->MaxNameLength = sizeof(buffer) - sizeof(IMAGEHLP_SYMBOL);
				dwDisp = 0;

 				if (image.SymGetSymFromAddr(image.hProcess, pFrame->dwRetAddr, &dwDisp, iSymbol))
				{
					bSymValid=true;
					if (bLineValid)
						text.addText(", ");
					text.addText(iSymbol->Name);
					if (bLineValid)
						text.addText("()\r\n");
					else
					{
						sprintf(buffer, " + %d bytes\r\n", dwDisp);
						text.addText(buffer);
					}
				}

				if (bLineValid==false && bSymValid==false)
				{
					sprintf(buffer, "Address=0x%08X, ??Unknown??\r\n", pFrame->dwRetAddr);
					text.addText(buffer);

					// try to read the return address
					if (i == 0 && IsBadReadPtr((void *)pContext->Esp, sizeof(U32)) == 0)
					{
						U32 address = ((U32 *)pContext->Esp)[0];

						bool bLineValid=false;
						bool bSymValid=false;
						IMAGEHLP_LINE *iLine = (IMAGEHLP_LINE *) buffer;
						memset(iLine, 0, sizeof(*iLine));
						iLine->SizeOfStruct = sizeof(*iLine);
						dwDisp = 0;

						text.addText("   [ESP] -> ");

						if (image.SymGetLineFromAddr(image.hProcess, address, &dwDisp, iLine))
						{
							bLineValid = true;
							char * ptr = strrchr(iLine->FileName, '\\');
							if (ptr)
								ptr++;
							else
								ptr = iLine->FileName;
							text.addText(ptr);
							sprintf(buffer, ", Line %d", iLine->LineNumber);
							text.addText(buffer);
						}
						GetLastError();
						
						IMAGEHLP_SYMBOL *iSymbol = (IMAGEHLP_SYMBOL *) buffer;
						memset(iSymbol, 0, sizeof(*iSymbol));
						iSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
						iSymbol->MaxNameLength = sizeof(buffer) - sizeof(IMAGEHLP_SYMBOL);
						dwDisp = 0;

 						if (image.SymGetSymFromAddr(image.hProcess, address, &dwDisp, iSymbol))
						{
							bSymValid=true;
							if (bLineValid)
								text.addText(", ");
							text.addText(iSymbol->Name);
							if (bLineValid)
								text.addText("()\r\n");
							else
							{
								sprintf(buffer, " + %d bytes\r\n", dwDisp);
								text.addText(buffer);
							}
						}

						if (bLineValid==false && bSymValid==false)
						{
							sprintf(buffer, "Address=0x%08X, ??Unknown??\r\n", address);
							text.addText(buffer);
						}
					}
				}
	
				pFrame = pFrame->pNext;
			}
			else
				break;
		}

		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "-------------------------------------------------\r\nException: \r\n");
		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), text.buffer);
		int result = IDABORT;
		
		FlipToGDI();
		result = DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG13), hMainWindow, dlgProc, (LPARAM) &text);

		restorePriority();

		return result;
	}

#endif  // end !FINAL_RELEASE

	return IDABORT;
}
//--------------------------------------------------------------------------//
//
void CQImage::MemoryReport (struct IHeap * heap)
{
#ifndef FINAL_RELEASE
	if (heap->GetLargestBlock() != heap->GetHeapSize())
	{
		TEXT_BUFFER<4096> text;
		text.type = CQERR_REPORT;
		text.addText("Memory Leaks Detected:\r\n");
		int oldOffset = text.index;

		heap->EnumerateBlocks(PrintBlock, &text);

		if (oldOffset != text.index)
		{
			SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
			PlaySound(INFO_SND, NULL, SND_ALIAS | SND_ASYNC);
			FlipToGDI();
			DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG13), hMainWindow, dlgProc, (LPARAM) &text);
			restorePriority();
		}
	}
#endif  // end !FINAL_RELEASE
}
//--------------------------------------------------------------------------//
//
long CQImage::PrintBlock (struct IHeap * pHeap, void *allocatedBlock, U32 dwFlags, void *context)
{
#ifndef FINAL_RELEASE

	if (image.hProcess)
	{
		TEXT_BUFFER<4096> * pText = (TEXT_BUFFER<4096> *) context;
		char buffer[1024];
		U32 owner = pHeap->GetBlockOwner(allocatedBlock);
		IMAGEHLP_LINE *iLine = (IMAGEHLP_LINE *) buffer;
		int oldOffset = pText->index;
		U32 dwDisp;

		if (owner == 0xFFFFFFFF)		// marked block
			goto Done;
		if ((dwFlags & DAHEAPFLAG_ALLOCATED_BLOCK)==0)
			goto Done;

		memset(iLine, 0, sizeof(*iLine));
		iLine->SizeOfStruct = sizeof(*iLine);
		dwDisp = 0;

		if (image.SymGetLineFromAddr(image.hProcess, owner, &dwDisp, iLine))
		{
			pText->addText(iLine->FileName);
			sprintf(buffer, "(%d) : ", iLine->LineNumber);
			pText->addText(buffer);
		}
		else
		{
			const char *pMsg = pHeap->GetBlockMessage(allocatedBlock);
			pText->addText("?");

			if (IsBadReadPtr(pMsg, 32) == 0)
			{
				if (strcmp(pMsg, "Heap control block") == 0)	// ignore heap control blocks!
				{
					pText->index = oldOffset;
					goto Done;
				}
				pText->addText(pMsg);
			}
			pText->addText("(?) : ");
		}

		sprintf(buffer, "%d byte memory leak. [PTR=0x%08X]\r\n", pHeap->GetBlockSize(allocatedBlock), allocatedBlock);
		pText->addText(buffer);


		OutputDebugString(pText->buffer + oldOffset);
		//	FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), pText->buffer + oldOffset);
	}

Done:

#endif  // end !FINAL_RELEASE
	
	return 1;
}
//--------------------------------------------------------------------------//
//
int ICQImage::STANDARD_DUMP (ErrorCode code, const C8 *fmt, ...)
{
#ifndef FINAL_RELEASE

	char buffer[4096];

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
			// let it go...growl
			return 0;

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

		case DAHEAP_OUT_OF_MEMORY:
			Bomb(buffer);
			InitializeDAHeap(0x800000, 0x800000, DAHEAPFLAG_DEBUGFILL_SNAN|DAHEAPFLAG_NOMSGS);
			return 1;		// retry
			break;
		}
	}

	return Bomb(buffer);

#else   // FINAL_RELEASE

	return 0;

#endif   // end FINAL_RELEASE
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//----------------------------End CQImage.cpp-------------------------------//
//--------------------------------------------------------------------------//
