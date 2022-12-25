// traceout.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#include "tchar.h"
#include <assert.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];								// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];								// The title bar text
HWND      hMainWnd = NULL;
HANDLE hLogFile = NULL;

// Local variables

static TCHAR szSourceFileName[] = TEXT(__FILE__);
static TCHAR szApiFailed[]      = TEXT("A Windows API Failed");

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
void                AskAndLaunchProgram (HWND hParentWnd);
void                ProcessDebugEvents ();
BOOL                DebugNewProcess (LPTSTR lpszFileName, LPTSTR lpszTitle);
void                AskAndOpenLog (HWND hWnd);
void                OpenNewLog (LPTSTR lpszFileName);

static void ShowLastError (TCHAR *context);
static bool parse_token (TCHAR *token);

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_TRACEOUT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_TRACEOUT);

	// Run through the stuff on the command line, executing each whitespace delimited token as a
	// program.

	{
		LPTSTR lpCmdLine2 = GetCommandLine();

		static TCHAR seps[] = TEXT(" \t\n\r");
		
		TCHAR *token = _tcstok (lpCmdLine2, seps); // Note: the first token is the program name. Skip it.

		token = _tcstok (NULL, seps);

		while (token)
		{
			if (!parse_token (token))
			{
				break;
			}
			token = _tcstok (NULL, seps);
		}
	}

	// Main message loop:
	bool done = false;
	while (!done)
	{
		ProcessDebugEvents ();

		// Handle the message pump
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (GetMessage(&msg, NULL, 0, 0)) 
			{
				if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			else
			{
				// We got a WM_QUIT message
				done = true;
				break;
			}
		}
	}

	return msg.wParam;
}

static bool parse_token (TCHAR *token)
{
	// If the token contains <keyword>=<value>, then treat it as a keyword, otherwise launch the program.

	if (_tcsnicmp (TEXT("log="), token, 4) == 0)
	{
		TCHAR *filename = _tcschr (token, '=');
		filename = CharNext (filename);
		OpenNewLog (filename);
		return true;
	}
	else
	{
		// All other tokens are treated as processes to launch.
		DebugNewProcess (token, token);
		return true;
	}

	return false;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_TRACEOUT);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR)IDC_TRACEOUT;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   hMainWnd = hWnd;

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR szHello[MAX_LOADSTRING];
	LoadString(hInst, IDS_HELLO, szHello, MAX_LOADSTRING);

	switch (message) 
	{
		case WM_CREATE:
			// Create a multi-line text control we will use for displaying the incoming trace information.
			{
				RECT r;
				GetClientRect (hWnd, &r);
				HWND hOutputWnd = CreateWindowEx
				(
					0,
					"EDIT",
					NULL,
					WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
						ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | 
						ES_AUTOHSCROLL | ES_READONLY | ES_WANTRETURN,
					r.left, r.top, r.right, r.bottom,
					hWnd,
					(HMENU) IDC_OUTPUT,
					hInst,
					NULL
				);
				if (hOutputWnd == NULL)
				{
					// Stop window creation
					return -1;
				}
				else
				{
					// Continue window creation
				   return DefWindowProc(hWnd, message, wParam, lParam);
				}
			}
			break;

		case WM_SIZE:
			{
				// Make the text control the size of this window's client rect.
				RECT r;
				GetClientRect (hWnd, &r);
				HWND hOut = GetDlgItem (hWnd, IDC_OUTPUT);
				if (hOut)
				{
					SetWindowPos (hOut, NULL, r.left, r.top, r.right, r.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
				}
			}
			break;

		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDM_ABOUT:
					DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
					break;
				case IDM_EXIT:
					DestroyWindow(hWnd);
					break;
				case IDM_LAUNCH:
					AskAndLaunchProgram(hWnd);
					break;
				case IDM_ATTACH:
					// *** TODO: Write code to attach to a program with debugging
					break;
				case IDM_OPENLOG:
					AskAndOpenLog(hWnd);
					break;

				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code here...
			EndPaint(hWnd, &ps);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

// ************************************************************************
// FUNCTION : ErrorMessageBox( LPCTSTR, LPCTSTR, LPCTSTR, INT )
// PURPOSE  : Displays an error message box with various error information
//            and allows the user to terminate or continue the process.
//            For a Win32 Application, GetLastError and FormatMessage are
//            used to retrieve the last API error code and error message.
// COMMENTS :
// ************************************************************************
BOOL ErrorMessageBox( LPCTSTR lpszText, LPCTSTR lpszTitle, LPCTSTR lpszFile, INT Line )
{
	#define ERROR_BUFFER_SIZE 512

	static TCHAR Format[] =
	TEXT( "%s\n\n"                                  )
	TEXT( "-- Error Information --\n"               )
	TEXT( "File : %s\n"                             )
	TEXT( "Line : %d\n"                             )
	TEXT( "Error Number : %d\n"                     )
	TEXT( "Error Message : %s\n"                    )
	TEXT( "\n"                                      )
	TEXT( "Press OK to terminate this application." );

	LPTSTR lpFormatMessageBuffer;
	DWORD  dwFormatMessage;
	DWORD  dwGetLastError;
	HLOCAL hMessageBoxBuffer;
	LPVOID lpMessageBoxBuffer;

	//-- perform a simple check on the needed buffer size
	if( lstrlen(lpszText) > (ERROR_BUFFER_SIZE - lstrlen(Format)) )
		return( FALSE );

	//-- allocate the message box buffer
	hMessageBoxBuffer  = LocalAlloc( LMEM_MOVEABLE, ERROR_BUFFER_SIZE );
	lpMessageBoxBuffer = LocalLock( hMessageBoxBuffer );

	//-- get the system error and system error message
	dwGetLastError = GetLastError();
	dwFormatMessage = FormatMessage(
					  FORMAT_MESSAGE_ALLOCATE_BUFFER
					  | FORMAT_MESSAGE_FROM_SYSTEM,
					  NULL, dwGetLastError, LANG_NEUTRAL,
					  (LPTSTR) &lpFormatMessageBuffer, 0, NULL );
	if( !dwFormatMessage )
		lpFormatMessageBuffer = TEXT("FormatMessage() failed!");

	//-- format the error messge box string
	wsprintf( (TCHAR *) lpMessageBoxBuffer, Format, lpszText, lpszFile, Line,
	dwGetLastError, lpFormatMessageBuffer );

	// -- display the error and allow the user to terminate or continue
	if
	(
		MessageBox( NULL, (TCHAR *) lpMessageBoxBuffer, lpszTitle,
			MB_APPLMODAL | MB_ICONSTOP | MB_OKCANCEL )
		== IDOK
	)
		ExitProcess( 0 );

	//-- free all buffers
	if( dwFormatMessage )
		LocalFree( (HLOCAL) lpFormatMessageBuffer );

	LocalFree( (HLOCAL) hMessageBoxBuffer );

	return( TRUE );
}

// ************************************************************************
// FUNCTION : DebugNewProcess( LPTSTR, LPTSTR )
// PURPOSE  : starts a new process as a debuggee
// COMMENTS :
// ************************************************************************
BOOL DebugNewProcess( LPTSTR lpszFileName, LPTSTR lpszTitle )
{
	static STARTUPINFO           StartupInfo;
	static LPSTARTUPINFO         lpStartupInfo = &StartupInfo;
	static PROCESS_INFORMATION   ProcessInfo;
	static LPPROCESS_INFORMATION lpProcessInfo = &ProcessInfo;

	lpStartupInfo->cb          = sizeof( STARTUPINFO );
	lpStartupInfo->lpDesktop   = NULL;
	lpStartupInfo->lpTitle     = lpszTitle;
	lpStartupInfo->dwX         = 0;
	lpStartupInfo->dwY         = 0;
	lpStartupInfo->dwXSize     = 0;
	lpStartupInfo->dwYSize     = 0;
	lpStartupInfo->dwFlags     = (DWORD) NULL;
	lpStartupInfo->wShowWindow = SW_SHOWDEFAULT;

	lpProcessInfo->hProcess = NULL;

	//-- create the Debuggee process instead
	if
	(
		!CreateProcess
		(
			NULL,
			lpszFileName,
			(LPSECURITY_ATTRIBUTES) NULL,
			(LPSECURITY_ATTRIBUTES) NULL,
			TRUE,
			DEBUG_PROCESS | NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,
			(LPVOID) NULL,
			(LPTSTR) NULL,
			lpStartupInfo, lpProcessInfo
		)
	)
	{

		switch( GetLastError() )
		{

		  case ERROR_FILE_NOT_FOUND:
			MessageBox( GetDesktopWindow(), TEXT( "This file does not exist." ),
			  TEXT( "Open File Error" ), MB_OK | MB_APPLMODAL | MB_SETFOREGROUND );
			break;
		  case ERROR_ACCESS_DENIED:
			MessageBox( GetDesktopWindow(), TEXT( "Access denied." ),
			  TEXT( "Open File Error" ), MB_OK | MB_APPLMODAL | MB_SETFOREGROUND );
			break;
		  case ERROR_FILE_INVALID:
			MessageBox( GetDesktopWindow(), TEXT( "Invalid file." ),
			  TEXT( "Open File Error" ), MB_OK | MB_APPLMODAL | MB_SETFOREGROUND );
			break;
		  case ERROR_FILE_CORRUPT:
			MessageBox( GetDesktopWindow(), TEXT( "The file is corrupt." ),
			  TEXT( "Open File Error" ), MB_OK | MB_APPLMODAL | MB_SETFOREGROUND );
			break;
		  case ERROR_BAD_EXE_FORMAT:
			MessageBox( GetDesktopWindow(), TEXT( "The file has a bad format." ),
			  TEXT( "Open File Error" ), MB_OK | MB_APPLMODAL | MB_SETFOREGROUND );
			break;
		  default:
			ErrorMessageBox( TEXT( "CreateProcess()" ),
			  szApiFailed, szSourceFileName, __LINE__ );
			break;

		}
		return( FALSE );
	}
	else
	{
		CloseHandle( ProcessInfo.hProcess );
		CloseHandle( ProcessInfo.hThread );
	}

	return (TRUE);
}

void AskAndLaunchProgram(HWND hWnd)
{
	// Get the name of the file from the user

	TCHAR filename[MAX_PATH];
	TCHAR title[1024];
	OPENFILENAME ofn;
	filename[0] = '\0';
	memset (&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH-1;
	ofn.lpstrFileTitle = title;
	ofn.nMaxFileTitle = 1023;

	if (GetOpenFileName(&ofn))
	{
		// Attempt to create a process from this file. The function will pop up an error if it fails, in which
		// case all subsequent WaitForDebugEvent() calls will fail, so we don't need to check the return value.
		DebugNewProcess (filename, title);
	}
}

void AskAndOpenLog (HWND hWnd)
{
	// Get the name of the file from the user

	TCHAR filename[MAX_PATH];
	TCHAR title[1024];
	OPENFILENAME ofn;
	filename[0] = '\0';
	memset (&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH-1;
	ofn.lpstrFileTitle = title;
	ofn.nMaxFileTitle = 1023;

	if (GetOpenFileName(&ofn))
	{
		OpenNewLog (filename);
	}
}

static void appendToHwnd (HWND hOut, const char *buffer)
{
	// WARNING: Only works on ANSI and MBCS strings.

	// Move the insertion point to the end of the text box.
	int length = GetWindowTextLength (hOut);
	SendMessage (hOut, EM_SETSEL, (WPARAM) length, (LPARAM) length);
	
	const int SCRUBLEN = 128;
	char scrub[SCRUBLEN];
	assert (SCRUBLEN > 3);

	// Convert all '\n' to "\r\n" before adding to the window.

	const char *here = buffer;
	int i = 0;
	while (*here)
	{
		if (i > SCRUBLEN-3)
		{
			scrub[i] = '\0';
			SendMessage (hOut, EM_REPLACESEL, (WPARAM) FALSE, (LPARAM) scrub);
			i = 0;
		}

		register char c = *here++;
		if (c == '\n')
		{
			scrub[i++] = '\r';
		}
		scrub[i++] = c;
	}

	// Send the final scrub text.
	scrub[i] = '\0';
	SendMessage (hOut, EM_REPLACESEL, (WPARAM) FALSE, (LPARAM) scrub);
}

static void WriteToWindow (const char *buffer)
{
	HWND hOut = GetDlgItem (hMainWnd, IDC_OUTPUT);
	if (hOut)
	{
		appendToHwnd (hOut, buffer);
	}
}

static void WriteToLog (TCHAR *buffer)
{
	// NOTE: The log files are openned with write through, so no buffer flushing is needed here.
	if (hLogFile)
	{
		DWORD len = strlen(buffer);
		DWORD totalWritten = 0;
		DWORD written = 0;

		while (totalWritten < len)
		{
			if (!WriteFile (hLogFile, buffer + totalWritten, len - totalWritten, &written, NULL))
			{
				ShowLastError (TEXT("Failed to write to log file"));
				return;
			}

			totalWritten += written;
		}
	}
}

static void ShowLastError (TCHAR *context)
{
	LPVOID lpMsgBuf;
	FormatMessage
	(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	    FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
	);
	MessageBox (NULL, (LPCTSTR)lpMsgBuf, context, MB_OK | MB_ICONINFORMATION );
	LocalFree (lpMsgBuf);
}

void OpenNewLog (LPTSTR lpszFileName)
{
	// Close any open log file.
	if (hLogFile)
	{
		CloseHandle (hLogFile);
		hLogFile = NULL;
	}

	// Attempt to create the given log file.

	hLogFile = 
		CreateFile
		(
			lpszFileName,
			GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
			NULL
		);

	if (!hLogFile)
	{
		ShowLastError (TEXT("Failed to open the log file"));
	}
	else
	{
		// Seek to the end of the file and write a line indicating that it was openned.
		SetFilePointer (hLogFile, 0, NULL, FILE_END);
		TCHAR buffer[1024];
		TCHAR userName[UNLEN+1];
		TCHAR machineName[MAX_COMPUTERNAME_LENGTH + 1];
		SYSTEMTIME now;
		GetLocalTime (&now);
		DWORD len = UNLEN+1;
		if (!GetUserName (userName, &len))
		{
			lstrcpy (userName, TEXT("<Unknown User>"));
		}
		len = MAX_COMPUTERNAME_LENGTH+1;
		if (!GetComputerName (machineName, &len))
		{
			lstrcpy (machineName, TEXT("<Unnamed Computer>"));
		}
		
		wsprintf
		(
			buffer,
			TEXT("****\n**** Open Log - %d/%d/%d at %02d:%02d:%02d (local time)\n**** By \"%s\" on \"%s\"\n****\n"),
			now.wMonth, now.wDay, now.wYear,
			now.wHour, now.wMinute, now.wSecond,
			userName, machineName
		);

		WriteToLog (buffer);
	}
}

void ProcessDebugEvents ()
{
	DEBUG_EVENT db;
	if (WaitForDebugEvent (&db, 0))
	{
		// Handle debug string output
		if (db.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT)
		{
			// Retrieve the text of the output and append it to the text buffer.
			// NOTE: Only process ANSI reports.
			OUTPUT_DEBUG_STRING_INFO *info = &db.u.DebugString;
			if (!info->fUnicode)
			{
				char *buffer = new char[info->nDebugStringLength+1];
				assert (buffer != NULL);

				DWORD readCount;
				HANDLE hProc = OpenProcess(PROCESS_VM_READ, FALSE, db.dwProcessId);
				if
				(
					hProc && 
					ReadProcessMemory
					(
						hProc,
						info->lpDebugStringData,
						buffer,
						info->nDebugStringLength,
						&readCount
					)
				)
				{
					// Ensure that the string is NULL terminated.
					buffer[info->nDebugStringLength] = '\0';

					// If a log file is open, write the text to it and flush.
					WriteToLog (buffer);

					// Append the text to the text box.
					WriteToWindow (buffer);
				}

				if (hProc)
				{
					CloseHandle (hProc);
				}

				if (buffer)
				{
					delete [] buffer;
				}
			}

			// Continue from the event.
			ContinueDebugEvent (db.dwProcessId, db.dwThreadId, DBG_CONTINUE);
		}

		// Handle exceptions
		else if (db.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
		{
			// Handle breakpoint exceptions by continuing, otherwise fall through to default behavior
			if
			(
				db.u.Exception.dwFirstChance &&
				(
					db.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT ||
					db.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP
				)
			)
			{
				ContinueDebugEvent (db.dwProcessId, db.dwThreadId, DBG_CONTINUE);
			}
			else
			{
				// Inidicate we didn't handle the exception
				ContinueDebugEvent (db.dwProcessId, db.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
			}
		}

		// For all others, continue from the event, indicating that we didn't handle it.
		else
		{
			ContinueDebugEvent (db.dwProcessId, db.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
		}
	}
}
