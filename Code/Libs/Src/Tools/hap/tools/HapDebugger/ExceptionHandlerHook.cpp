
//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001 by Humongous Entertainment
//  
//  This library is free software; you can redistribute it and/or modify it 
//  under the terms of the GNU Lesser General Public License as published by 
//  the Free Software Foundation; either version 2.1 of the License, or  any 
//  later version.
//  
//  This library is distributed in the hope that it will be useful, but 
//  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
//  License for more details.
//  
//  You should have received a copy of the GNU Lesser General Public License 
//  along with this library (see the file license.txt); if not, write to the 
//  Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//////////////////////////////////////////////////////////////////////////////	
#include "stdafx.h"

// This code copied from the MFC source code in winmain.cpp
int HandledAfxWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine, int nCmdShow)
{
    ASSERT(hPrevInstance == NULL);
    
    int nReturnCode = -1;
	CWinThread* pThread = AfxGetThread();
    CWinApp* pApp = AfxGetApp();

    // AFX internal initialization
    if (!AfxWinInit(hInstance, hPrevInstance, lpCmdLine, nCmdShow))
        goto InitFailure;

    // App global initializations (rare)
    if (!pApp->InitApplication())
        goto InitFailure;

	// Perform specific initializations
	if (!pThread->InitInstance())
	{
		if (pThread->m_pMainWnd != NULL)
		{
			TRACE0("Warning: Destroying non-NULL m_pMainWnd\n");
			pThread->m_pMainWnd->DestroyWindow();
		}
		nReturnCode = pThread->ExitInstance();
		goto InitFailure;
	}
    nReturnCode = pApp->Run();

InitFailure:
#ifdef _DEBUG
    // Check for missing AfxLockTempMap calls
    if (AfxGetModuleThreadState()->m_nTempMapLock != 0)
    {
        TRACE1("Warning: Temp map lock count non-zero (%ld).\n",
            AfxGetModuleThreadState()->m_nTempMapLock);
    }
    AfxLockTempMaps();
    AfxUnlockTempMaps(-1);
#endif

    AfxWinTerm();
    return nReturnCode;
}

// This is the initial entry point into your MFC app. Normally this is in the
// MFC library.
int AFXAPI AfxWinMain(HINSTANCE hInstance, HINSTANCE hPrevInst,
    LPTSTR lpszArgs, int nWinMode)
{
	HEInitializeLibrary();

    // Wrap WinMain in a structured exception handler (different from C++
    // exception handling) in order to make sure that all access violations
    // and other exceptions are displayed - regardless of when they happen.
    // This should be done for each thread, if at all possible, so that exceptions
    // will be reliably caught, even inside the debugger.
    __try
    {
        return HandledAfxWinMain(hInstance, hPrevInst, lpszArgs, nWinMode);
    }
    __except(HandleException(GetExceptionInformation(), "Main Thread"))
    {
        // We don't actually do anything inside the handler. All of the
        // work is done by he::HandleException()
    }
    return 0;
}
