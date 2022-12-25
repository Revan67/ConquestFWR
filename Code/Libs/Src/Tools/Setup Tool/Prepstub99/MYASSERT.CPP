//
// myassert.cpp
//
//		Implementation of the ASSERT macro
//
// History:
//
//		11/16/94	KenSh		Moved code here from MFC source.
//

#include "prepstub.h"

#ifdef _DEBUG

void AssertFailedLine( LPCSTR lpszFileName, int nLine )
{
	char sz[255];
	static char szTitle[] = "Assertion Failed!";
	static char szMessage[] = "%s: File %s, Line %d";
	static char szUnknown[] = "EXPLORE.EXE";
	
	wsprintf(sz, (LPCSTR)szMessage,
		(LPCSTR)szUnknown,
		lpszFileName,
		nLine);

	// assume the debugger or auxiliary port
	OutputDebugString(sz);
	OutputDebugString(", ");
	OutputDebugString(szTitle);
	OutputDebugString("\n\r");
	
	HWND hwndParent = GetDesktopWindow();

	int nCode = MessageBox(hwndParent, sz, szTitle,
					MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE);

	if (nCode == IDIGNORE)
	{
		return;     // ignore
	}
	else if (nCode == IDRETRY)
	{
		// break into the debugger (or Dr Watson log)
#ifndef _PORTABLE
		_asm { int 3 };
#endif
		return; // ignore and continue in debugger to diagnose problem
	}

	abort();
}

#endif //_DEBUG
