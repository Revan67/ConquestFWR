#include "stdafx.h"
#include "globals.h"

#include "CQTrace.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
			const char* pText = (const char*) lParam;

			hItem = GetDlgItem(hwnd, IDC_EDIT_ERR);
			if( hItem )
			{
				::SetWindowText( hItem, pText );
			}
		}
		break;
	
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
		case IDOK:
			EndDialog(hwnd, LOWORD(wParam));
			break;
		}
		break;
	}

	return result;
}

//--------------------------------------------------------------------------//
//
bool __cdecl ICQImage::Bomb (const char *exp, ...)
{
#ifndef FINAL_RELEASE
	
	char buffer[1024];

	va_list args;
	va_start (args, exp);
	vsprintf (buffer, exp, args);
	va_end (args);

	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	int result = DialogBoxParam(::AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDD_ERROR), hMainWindow, dlgProc, (LPARAM) &buffer);

//	if (image.hProcess)
//	{
//		STACK_FRAME * pFrame;
//		int i;
//		DWORD dwDisp;
//		TEXT_BUFFER<4096> text;
//		char version[64];
//		getVersion(version, sizeof(version));
//
//		__asm mov DWORD ptr [pFrame], ebp
//
//		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
//
//		text.type = CQERR_BOMB;
//		if (CQFLAGS.bTraceMission != 0 && EVENTSYS)
//			EVENTSYS->Send(CQE_DEBUG_HOTKEY, (void *) IDH_PRINT_OPLIST);		// print the op list
//		PlaySound(ERROR_SND, NULL, SND_ALIAS | SND_ASYNC);
//	
//		{
//			char * ptr = buffer;
//			if (ptr[0] && ptr[1] == ':')
//				ptr += 2;
//			ptr = strchr(ptr, ':');
//			if (ptr)
//				ptr += 2;
//			else
//				ptr = buffer;
//			text.addText("Error: ");
//			text.addText(ptr);
//			sprintf(buffer, " [%s]\r\n", version);
//			text.addText(buffer);
//			text.addText("Call Stack:\r\n");
//		}
//
//		for (i = 0; i < 8; i++)
//		{
//			if (IsBadReadPtr(pFrame, sizeof(STACK_FRAME)) == 0)
//			{
//				bool bLineValid=false;
//				IMAGEHLP_LINE *iLine = (IMAGEHLP_LINE *) buffer;
//				memset(iLine, 0, sizeof(*iLine));
//				iLine->SizeOfStruct = sizeof(*iLine);
//				dwDisp = 0;
//
//				if (image.SymGetLineFromAddr(image.hProcess, pFrame->dwRetAddr, &dwDisp, iLine))
//				{
//					bLineValid = true;
//					char * ptr = strrchr(iLine->FileName, '\\');
//					if (ptr)
//						ptr++;
//					else
//						ptr = iLine->FileName;
//					text.addText(ptr);
//					sprintf(buffer, ", Line %d", iLine->LineNumber);
//					text.addText(buffer);
//				}
//				GetLastError();
//				
//				
//				IMAGEHLP_SYMBOL *iSymbol = (IMAGEHLP_SYMBOL *) buffer;
//				memset(iSymbol, 0, sizeof(*iSymbol));
//				iSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
//				iSymbol->MaxNameLength = sizeof(buffer) - sizeof(IMAGEHLP_SYMBOL);
//				dwDisp = 0;
//
// 				if (image.SymGetSymFromAddr(image.hProcess, pFrame->dwRetAddr, &dwDisp, iSymbol))
//				{
//					if (bLineValid)
//						text.addText(", ");
//					text.addText(iSymbol->Name);
//					if (bLineValid)
//						text.addText("()\r\n");
//					else
//					{
//						sprintf(buffer, " + %d bytes\r\n", dwDisp);
//						text.addText(buffer);
//					}
//				}
//
//				pFrame = pFrame->pNext;
//			}
//			else
//				break;	// stop if invalid address
//		}
//
//		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "-------------------------------------------------\r\nBomb: \r\n");
//		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), text.buffer);
//		int result = IDABORT;
//		
//		FlipToGDI();
//		result = DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG13), hMainWindow, dlgProc, (LPARAM) &text);
//
//		restorePriority();
//
//		if (result == IDABORT)
//		{
//			CQFLAGS.bNoExitConfirm = 1;
//			PostQuitMessage(-1);		// cannot use exit(-1) here
//			if (WM)
//				WM->ServeMessageQueue();
//		}
//		else
//		if (result == IDDEBUG)		// DEBUG chosen
//			return true;
//		
//		return false;
//	}
//
//	FDUMP(ErrorCode(ERR_GENERAL, SEV_FATAL), buffer);

#endif  // end !FINAL_RELEASE
	
	return 0;
}
//--------------------------------------------------------------------------//
//
bool __cdecl ICQImage::Error (const char *exp, ...)
{
#ifndef FINAL_RELEASE

	char buffer[1024];

	va_list args;
	va_start (args, exp);
	vsprintf (buffer, exp, args);
	va_end (args);

	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	int result = DialogBoxParam(::AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDD_ERROR), hMainWindow, dlgProc, (LPARAM) &buffer);

//	if (image.hProcess)
//	{
//		STACK_FRAME * pFrame;
//		int i;
//		DWORD dwDisp;
//		TEXT_BUFFER<4096> text;
//		char version[64];
//		getVersion(version, sizeof(version));
//
//		__asm mov DWORD ptr [pFrame], ebp
//
//		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
//
//		text.type = CQERR_ERROR;
//		PlaySound(INFO_SND, NULL, SND_ALIAS | SND_ASYNC);
//
//		{
//			char * ptr = buffer;
//			if (ptr[0] && ptr[1] == ':')
//				ptr += 2;
//			ptr = strchr(ptr, ':');
//			if (ptr)
//				ptr += 2;
//			else
//				ptr = buffer;
//			text.addText("Error: ");
//			text.addText(ptr);
//			sprintf(buffer, " [%s]\r\n", version);
//			text.addText(buffer);
//			text.addText("Call Stack:\r\n");
//		}
//
//		for (i = 0; i < 8; i++)
//		{
//			if (IsBadReadPtr(pFrame, sizeof(STACK_FRAME)) == 0)
//			{
//				bool bLineValid=false;
//				IMAGEHLP_LINE *iLine = (IMAGEHLP_LINE *) buffer;
//				memset(iLine, 0, sizeof(*iLine));
//				iLine->SizeOfStruct = sizeof(*iLine);
//				dwDisp = 0;
//
//				if (image.SymGetLineFromAddr(image.hProcess, pFrame->dwRetAddr, &dwDisp, iLine))
//				{
//					bLineValid = true;
//					char * ptr = strrchr(iLine->FileName, '\\');
//					if (ptr)
//						ptr++;
//					else
//						ptr = iLine->FileName;
//					text.addText(ptr);
//					sprintf(buffer, ", Line %d", iLine->LineNumber);
//					text.addText(buffer);
//				}
//				GetLastError();
//				
//				
//				IMAGEHLP_SYMBOL *iSymbol = (IMAGEHLP_SYMBOL *) buffer;
//				memset(iSymbol, 0, sizeof(*iSymbol));
//				iSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
//				iSymbol->MaxNameLength = sizeof(buffer) - sizeof(IMAGEHLP_SYMBOL);
//				dwDisp = 0;
//
// 				if (image.SymGetSymFromAddr(image.hProcess, pFrame->dwRetAddr, &dwDisp, iSymbol))
//				{
//					if (bLineValid)
//						text.addText(", ");
//					text.addText(iSymbol->Name);
//					if (bLineValid)
//						text.addText("()\r\n");
//					else
//					{
//						sprintf(buffer, " + %d bytes\r\n", dwDisp);
//						text.addText(buffer);
//					}
//				}
//
//				pFrame = pFrame->pNext;
//			}
//			else
//				break;	// stop if invalid address
//		}
//
//		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "-------------------------------------------------\r\nRecoverable Error: \r\n");
//		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), text.buffer);
//		int result = IDIGNORE;
//		
//		FlipToGDI();
//		result = DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG13), hMainWindow, dlgProc, (LPARAM) &text);
//
//		restorePriority();
//
//		if (result == IDABORT)
//		{
//			CQFLAGS.bNoExitConfirm = 1;
//			PostQuitMessage(-1);		// cannot use exit(-1) here
//			if (WM)
//				WM->ServeMessageQueue();
//		}
//		else
//		if (result == IDDEBUG)		// DEBUG chosen
//			return true;
//		
//		return false;
//	}
//
//	FDUMP(ErrorCode(ERR_GENERAL, SEV_ERROR), buffer);

#endif  // end !FINAL_RELEASE
	
	return 0;
}

//bool __cdecl ICQImage::Error (const char *exp, ...)
//{
//#ifndef FINAL_RELEASE
//	
//	char buffer[1024];
//
//	va_list args;
//	va_start (args, exp);
//	vsprintf (buffer, exp, args);
//	va_end (args);
//
//	::MessageBox( NULL, buffer, "error", MB_OK );
//#endif  // end !FINAL_RELEASE
//	
//	return 0;
//}

bool ICQImage::Assert (const char *exp, void *file, unsigned line)
{
//#ifndef FINAL_RELEASE
//
//	if (image.hProcess)
//	{
//		STACK_FRAME * pFrame;
//		char buffer[1024];
//		int i;
//		DWORD dwDisp;
//		TEXT_BUFFER<4096> text;
//		char version[64];
//		getVersion(version, sizeof(version));
//
//		__asm mov DWORD ptr [pFrame], ebp
//
//		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
//
//		text.type = CQERR_ASSERT;
//		if (CQFLAGS.bTraceMission != 0 && EVENTSYS)
//			EVENTSYS->Send(CQE_DEBUG_HOTKEY, (void *) IDH_PRINT_OPLIST);		// print the op list
//		PlaySound(ERROR_SND, NULL, SND_ALIAS | SND_ASYNC);
//
//		sprintf(buffer, "Expression: %s [%s]\r\n", exp, version);
//		text.addText(buffer);
//		text.addText("Call Stack:\r\n");
//
//		for (i = 0; i < 8; i++)
//		{
//			if (IsBadReadPtr(pFrame, sizeof(STACK_FRAME)) == 0)
//			{
//				bool bLineValid=false;
//				IMAGEHLP_LINE *iLine = (IMAGEHLP_LINE *) buffer;
//				memset(iLine, 0, sizeof(*iLine));
//				iLine->SizeOfStruct = sizeof(*iLine);
//				dwDisp = 0;
//
//				if (image.SymGetLineFromAddr(image.hProcess, pFrame->dwRetAddr, &dwDisp, iLine))
//				{
//					bLineValid = true;
//					char * ptr = strrchr(iLine->FileName, '\\');
//					if (ptr)
//						ptr++;
//					else
//						ptr = iLine->FileName;
//					text.addText(ptr);
//					sprintf(buffer, ", Line %d", iLine->LineNumber);
//					text.addText(buffer);
//				}
//				GetLastError();
//				
//				
//				IMAGEHLP_SYMBOL *iSymbol = (IMAGEHLP_SYMBOL *) buffer;
//				memset(iSymbol, 0, sizeof(*iSymbol));
//				iSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
//				iSymbol->MaxNameLength = sizeof(buffer) - sizeof(IMAGEHLP_SYMBOL);
//				dwDisp = 0;
//
// 				if (image.SymGetSymFromAddr(image.hProcess, pFrame->dwRetAddr, &dwDisp, iSymbol))
//				{
//					if (bLineValid)
//						text.addText(", ");
//					text.addText(iSymbol->Name);
//					if (bLineValid)
//						text.addText("()\r\n");
//					else
//					{
//						sprintf(buffer, " + %d bytes\r\n", dwDisp);
//						text.addText(buffer);
//					}
//				}
//
//				pFrame = pFrame->pNext;
//			}
//			else
//				break;	// stop if invalid address
//		}
//
//		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "-------------------------------------------------\r\nAssertion Failed: \r\n");
//		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), text.buffer);
//		int result = IDIGNORE;
//
//		FlipToGDI();
//		result = DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG13), hMainWindow, dlgProc, (LPARAM) &text);
//
//		restorePriority();
//
//		if (result == IDABORT)
//		{
//			CQFLAGS.bNoExitConfirm = 1;
//			PostQuitMessage(-1);		// cannot use exit(-1) here
//			if (WM)
//				WM->ServeMessageQueue();
//		}
//		else
//		if (result == IDDEBUG)		// DEBUG chosen
//			return true;
//		
//		return false;
//	}
//
//	FDUMP(ErrorCode(ERR_ASSERT, SEV_FATAL), exp, file, line);
//
//#endif  // end !FINAL_RELEASE

	return 0;
}
