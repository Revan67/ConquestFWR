/****************************************************************************

	FILE: Config.cpp
		Copyright (C) 1999 Digital Anvil

	PURPOSE: Config/Launcher application

	COMMENTS:

	Windows can have several copies of your application running at the
	same time.  The variable GetResourceInst() keeps track of which instance this
	application is so that processing will be to the correct window.

	You only need to initialize the application once.  After it is
	initialized, all other copies of the application will use the same
	window class, and do not need to be separately initialized.

****************************************************************************/
#include "windows.h"		    /* required for all Windows applications*/
#include <shlobj.h>
#include "stdio.h"
#include "string.h"
#include "resource.h"
#include "mmsystem.h"
#include "hotsetup.h"
#include "hotsetuprc.h"
#include "resc1.h"
#include "windowsx.h"

const char * const configClsname = "DAConfig";
using namespace NGLOBALS;

extern "C" {
int PASCAL WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
long FAR PASCAL ConfigWndProc(HWND, WORD, WPARAM, LPARAM);
}

BOOL ConfigInit(HINSTANCE);

_declspec(dllexport) EBURETCODE WINAPI MasterCallback(void *cbd);
extern bool GetDeviceSelection(HWND hwnd, HINSTANCE hinst, LPSTR iniFileName, LPSTR szFlags, bool forceConfigureWindow);

//----------------------------------------------------------------------------
//*** Globals
//

HINSTANCE hInst;			    /* current instance			    */

//----------------------------------------------------------------------------
 
/****************************************************************************

	FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

	PURPOSE: calls initialization function, processes message loop

	COMMENTS:

	This will initialize the window class if it is the first time this
	application is run.  It then creates the window, and processes the
	message loop until a PostQuitMessage is received.  It exits the
	application by returning the value passed by the PostQuitMessage.

****************************************************************************/
         
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;				     /* window handle		     */
	MSG mmsg;				     /* message			     */
	HACCEL hAccel;
	char dtwinname[50];
	int x,y,width,height;
  	char msg[256];
	char lpCmd[_MAX_PATH * 10] = "";
	OSVERSIONINFO os;
	os.dwOSVersionInfoSize=sizeof(os);
	GetVersionEx(&os);

	hInst = hInstance;
	SetAppInst(hInst);			/* Saves the current instance	     */
	
	lstrcpy(lpCmd, lpCmdLine);
	if (EBU_ERROR == InitEBUSetup(lpCmd, MasterCallback, NULL, NULL,TRUE,FALSE))
	{
		return FALSE;
	}
	else
	{
		// Determine if another window with our class name exists...
		HWND
			hWndPrev = FindWindow(configClsname,GetAppTitle());
		if (hWndPrev)
		{
			// If iconic, restore the main window
			if (IsIconic(hWndPrev))
				ShowWindow(hWndPrev, SW_RESTORE);
			
			// Bring the window to the foreground
			SetForegroundWindow(hWndPrev);
			
			// and we are done activating the previous one.
			DeleteMyself(FALSE);			/* clean ourselves up from the temp directory on exit */
			return FALSE;
		}
	}
	
	EBULoadString( GetResourceInst(), STR_SETUP_APPTITLE, GetAppTitle(), 128 );
	hAccel = LoadAccelerators(GetResourceInst(),"ACCEL");

	EBULoadString(GetResourceInst(),STR_LAUNCHEXEWINNAME,dtwinname,50);
	if(FindWindow(dtwinname,dtwinname)!= NULL)
	{
		if(!GetInAutoRun())
		{
			EBULoadString(GetResourceInst(),STR_CLOSEGAME,msg,sizeof(msg));
			MessageBox(NULL,msg,GetAppTitle(),MB_ICONEXCLAMATION | MB_OK);
		}
		return(0);
	}

	HDC hdc = GetDC(HWND_DESKTOP);
	int cColors = GetDeviceCaps(hdc, NUMCOLORS);
	int cBitsPixel = GetDeviceCaps(hdc, BITSPIXEL);
	ReleaseDC(HWND_DESKTOP, hdc);

	if (!hPrevInstance)			/* Has application been initialized? */
		if (!ConfigInit(hInst))
			return (0);			/* Exits if unable to initialize     */


	//center the window
	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);
	x = 0;
	y = 0;

	if (x<0)
		x=0;
	if(y<0)
		y=0;

	hWnd = CreateWindow(configClsname,					/* window class		*/
		GetAppTitle(),								/* window name		*/
		WS_POPUPWINDOW,			/* window style		*/
		x,											/* x position		*/
		y,											/* y position		*/
		width,										/* width			*/
		height,										/* height			*/
		NULL,										/* parent handle	*/
		NULL,										/* menu or child ID	*/
		hInstance,									/* instance			*/
		NULL);										/* additional info	*/

	if (!hWnd)	/* Was the window created? */
	{
		int error = GetLastError();
		return (0);
	}
	
	SetWndParent(hWnd);
	
	bool forceConfigure = false;
	char szAppName[128];
	EBULoadString(GetResourceInst(), STR_LAUNCHEXE, szAppName, sizeof(szAppName) );
	char szDXVersion[32];
	EBULoadString(GetAppInst(), STR_DX_MIN_VERSION, szDXVersion, sizeof(szDXVersion) );
	if ( (strstr(lpCmdLine, "configure")) || (strstr(lpCmdLine, "CONFIGURE")) || (!stricmp(GetSetupExeName(),"config.exe")) )
	{
		forceConfigure = true; // show configure then launch real app
	}
	if ( forceConfigure || (!stricmp(GetSetupExeName(),szAppName)) )
	{	// show the device selection dialog box if config was run or "configure" was passed in
		int retVal = CheckDXVersion(szDXVersion, "dsetup");
		if ((EV_EXISTING_SAME == retVal) || (EV_EXISTING_NEWER == retVal))
		{
			char str[128];
			EBULoadString(GetResourceInst(), STR_INI_FILE_NAME, str, sizeof(str) );
			char inipath[255+1];
			sprintf( inipath, "%s\\%s\\%s", GetAppDir(), "game", str);
			ShowWindow(hWnd, SW_HIDE);	/* Shows the window			*/
			if ( GetDeviceSelection(hWnd, GetResourceInst(), inipath, "", forceConfigure) )
			{	// inifile is ok and configure window not forced, so just launch the app
				HCURSOR hCurCursor = SetCursor(LoadCursor(NULL,IDC_WAIT));
				LaunchApplication (STR_REAL_EXENAME,STR_COMMANDLINE);
				Sleep(500);
				DestroyWindow(hWnd);
			}
		}
		else
		{
			char msg[256];
			EBULoadString(GetResourceInst(),STR_NODIRECTXCD,msg,256);
			MessageBox(GetWndParent(),msg,GetAppTitle(),MB_OK);
		}
		return (0);
	}

	ShowWindow(hWnd, SW_HIDE);	/* Shows the window			*/
	UpdateWindow(hWnd);															/* Sends WM_PAINT message	*/

	while (GetMessage(&mmsg,	/* message structure						*/
		NULL,					/* handle of window receiving the message	*/
		0,						/* lowest message to examine				*/
		0))						/* highest message to examine				*/
	{
		// see if it is a dialog type message for our main window controls (tab, enter, space, etc.)
		if (IsDialogMessage(hWnd, &mmsg))
		{
			continue;
		}
		if(TranslateAccelerator(hWnd,hAccel,&mmsg))
			continue;
		TranslateMessage(&mmsg);	/* Translates virtual key codes			*/
		DispatchMessage(&mmsg);		/* Dispatches message to window			*/
	}


	UnregisterClass(configClsname,hInst);

	//	If not uninstalling clean-up after ourselves because we didn't bootstrap normally
	if (GetResourceInst())
	{
		TCHAR szDllPath[_MAX_PATH]="";
		//
		//Get the name of the resource DLL
		//
		GetModuleFileName(GetResourceInst(), szDllPath, sizeof(szDllPath));
		
		//
		//Free the resource DLL and add batch lines to delete it...
		//
		FreeLibrary(GetResourceInst());
		SetResourceInst(NULL);
	}
	return (mmsg.wParam);			/* Returns the value from PostQuitMessage */
}

/****************************************************************************

	FUNCTION: ConfigInit(HANDLE)

	PURPOSE: Initializes window data and registers window class

	COMMENTS:

	Sets up a structure to register the window class.  Structure includes
	such information as what function will process messages, what cursor
	and icon to use, etc.

****************************************************************************/
BOOL ConfigInit(HINSTANCE hInstance)
{
	HANDLE hMemory;				/* handle to allocated memory */
	PWNDCLASS pWndClass;		/* structure pointer	     */
	BOOL bSuccess;				/* RegisterClass() result     */

	hMemory = LocalAlloc(LPTR, sizeof(WNDCLASS));
	
	pWndClass = (PWNDCLASS) LocalLock(hMemory);

	pWndClass->style			= CS_GLOBALCLASS; /*CS_HREDRAW | CS_VREDRAW; */
	pWndClass->lpfnWndProc		= (WNDPROC)ConfigWndProc;
	pWndClass->hInstance		= hInstance;
	pWndClass->hIcon			= LoadIcon(GetAppInst(),"0-INSTALL");
	pWndClass->hCursor			= LoadCursor(NULL, IDC_ARROW);
	pWndClass->hbrBackground	= (HBRUSH) GetStockObject(BLACK_BRUSH);
	pWndClass->lpszMenuName		= NULL;
	pWndClass->lpszClassName	= (LPSTR)configClsname;

	bSuccess = RegisterClass(pWndClass);
	LocalUnlock(hMemory);		/* Unlocks the memory    */
	LocalFree(hMemory);			/* Returns it to Windows */
	return (bSuccess);			/* Returns result of registering the window */
}

/****************************************************************************

	FUNCTION: ConfigWndProc(HWND, unsigned, WORD, LONG)

	PURPOSE:  Processes messages

	MESSAGES:

	WM_DESTROY    - destroy window

****************************************************************************/
long FAR PASCAL ConfigWndProc(HWND hWnd, WORD message, WPARAM wParam, LPARAM lParam)
{
	LRESULT retcode = 0;
	/* functions 			     */
	switch (message)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
	case WM_PAINT:
		break;
	case WM_COMMAND:
		break;
	case WM_DESTROY:		  /* message: window being destroyed */
 	    PostQuitMessage(0);
		break;
	default:			  /* Passes it on if unproccessed    */
		return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	return (0);
}  


_declspec(dllexport) EBURETCODE WINAPI MasterCallback(void *cbd)
{
	return EBU_OK;
}
