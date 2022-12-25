/****************************************************************************

	PROGRAM: Setup.cpp
		Copyright (C) 1996 Microsoft Corp.

	PURPOSE: Setup Launcher application

	FUNCTIONS:

	WinMain() - calls initialization function, processes message loop
	SetupInit() - initializes window data and registers window
	SetupWndProc() - processes messages

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
#include "resource.h"		    /* specific to this program		    */
#include "mmsystem.h"
#include "hotsetuprc.h"
#include "hotsetup.h"
#include "widclass.h"
#include "fdi.h"
#include "setup.h"
#include "io.h"
#include "fcntl.h"
#include "gauge.h"
#include "setupdlg.h"
#include "restart.h"
#include "resc1.h"
#include "progman.h"
//#include "imectrl.h"
#include <commctrl.h>
#include <richedit.h>

#include "appspecific.h"

#include "windowsx.h"
//#include "resource.h"

//----------------------------------------------------------------------------
//function declarations

BOOL UpdateGamePath( );

extern _declspec(dllexport) EBURETCODE WINAPI MasterCallback(void *cbd);
extern VOID ReplaceStringTokens(char *sz, size_t wBuf);

extern HWND	hWndRichEdit = NULL;

void DeleteMyFile(char *szSource);
void SetupButtons(HWND hWnd);

//TODO Move this to the EBUEngine
BOOL LaunchUrl(UINT uiURLStringID);

//This only needed will LaunchURL is implemented in this project.
extern LONG MyGetPrivateProfileString( LPSTR lpszKey, LPCSTR lpszDefault, LPSTR lpszBuf, int cbBuf );

//----------------------------------------------------------------------------
//*** Globals
// 

extern HINSTANCE hInst;			    /* current instance			    */
extern const char * const clsname;

HINSTANCE g_hInst;

HWND ghWnd;					/* global window handle for handlers		*/
HWND errhWnd;			    /* global window handle for current error	*/
HWND g_hCDWnd=NULL;
HWND hFocusWnd;

WNDPROC wpOrigEditProc;
MyDialog *glpMyDialog=NULL;

BOOL Told = FALSE;

Container ButContainer;
InstallButton *	pInstall = NULL;
UnInstallButton * pUnInstall = NULL;
ReInstallButton * pReInstall = NULL;
QuitButton * pExit = NULL;
PlayButton * pPlay = NULL;
WebLinkButton * pWebLink = NULL;
ConfigureButton * pConfigure = NULL;
ReadMeButton * pReadMe = NULL;

BOOL fbRetailInstalled=FALSE;

HWND		hWndPrintDialog;
HINSTANCE	hRichEditLib = NULL;
BOOL		fAbortPrint;

char lpSetupEnu[_MAX_PATH];
char szEula[50];
DWORD dwTempSpace = 1024*1024*1;
char szTempPath[_MAX_PATH];

char g_szAppTitle[150];

int gCurrentBitmapID = SETUP_INITIAL_BITMAP_ID;

int Y0 = 0;
int X0 = 0;
#define strRTFHEADER "{\\rtf1"
#define nTWIPSPERINCH 1440

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

	//
	//See if the RTF control is available it will be needed later.
	//
	hRichEditLib = LoadLibrary("RICHED32.DLL");

	if (!hRichEditLib)
		return FALSE;

	//Need this because it sets m_fWin95NotOSR2 need for that may be called
	// as a consequence of FindPath() on Win95 boxes.
	//Normally this is performed in InitEbuSetup.
	//But because we are self-extracting enough space for the setupenu.dll needs to be found.
	// __PETER a-petere

	SetOS(GetCurrentOperatingSystem());

	//Still Check OS BUILD and Service Pack Information

	if(os.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS && (os.dwPlatformId == VER_PLATFORM_WIN32_NT && os.dwMajorVersion < 4))
	{
		LoadString(hInst,STR_WINNTOR95,msg,sizeof(msg));
		MessageBox(NULL,msg,GetAppTitle(),MB_ICONEXCLAMATION | MB_OK);
		return(0);
	}
	DWORD dwNTBuildNumberRequired = 1381;
	BYTE bNTCSDVersionRequired = 3;
	if (os.dwPlatformId == VER_PLATFORM_WIN32_NT && os.dwMajorVersion == 4)
	{
		//
		//Check for minimum build number if running under NT 4.0
		//If required build number is exactly equal to OS version number,
		//then check the CSD version number also.
		//
		if	(dwNTBuildNumberRequired > os.dwBuildNumber ||
			(dwNTBuildNumberRequired == os.dwBuildNumber &&
			bNTCSDVersionRequired > GetServicePack()))
		{
			LoadString(hInst,STR_SERVICEPACK3,msg,sizeof(msg));
			MessageBox(NULL,msg,GetAppTitle(),MB_ICONEXCLAMATION | MB_OK);
			return(0);
		}
	}

	hInst = hInstance;
	SetAppInst(hInst);			/* Saves the current instance	     */
	
	lstrcpy(lpCmd, lpCmdLine);
	if (EBU_ERROR == InitEBUSetup(lpCmd, MasterCallback, NULL, NULL,TRUE,FALSE))
	{
		return FALSE;
	}
	else
	{
		char appClsname[128];
		EBULoadString(GetResourceInst(),STR_APP_WINDOW_CLASS,appClsname,sizeof(appClsname));
		
		// Determine if another window with our class name or the apps class name exists...
		HWND
			hWndPrev = FindWindow(clsname,NULL);
		if (!hWndPrev)
			hWndPrev = FindWindow(appClsname,NULL);

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
	
	EBULoadString(GetResourceInst(),STR_EULA,szEula,sizeof(szEula));
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

	ProInit(NULL,hInst);

	if (!hPrevInstance)			/* Has application been initialized? */
		if (!SetupInit(hInst))
			return (0);			/* Exits if unable to initialize     */

	if(APP_REQUIRES_BROWSER && !IsBrowserInstalled())
	{
		EBULoadString(GetResourceInst(),STR_NEEDBROWSER,msg,256);
		MessageBox(NULL,msg,GetAppTitle(),MB_ICONEXCLAMATION|MB_OK);
		return 0;				/* Exits if app needs browser and none installed     */
	}

	if (HasAppEverBeenLaunched(APP_MUST_LAUNCH))
		SETUP_BLACKOUT_SCREEN = FALSE; // only black it out the first time

	if (!SETUP_BLACKOUT_SCREEN)
		SETUP_WINDOW_STYLE |= WS_BORDER;
	//center the window
	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);
	x = width/2-SETUP_WINDOW_WIDTH/2;
	y = height/2-SETUP_WINDOW_HEIGHT/2;

	if (x<0)
		x=0;
	if(y<0)
		y=0;

	if (SETUP_BLACKOUT_SCREEN)
	{
		X0 = x;
		Y0 = y;
		x = y = 0;
	}
	else
	{
		X0 = Y0 = 0;
		width = SETUP_WINDOW_WIDTH;
		height = SETUP_WINDOW_HEIGHT;
	}
	hWnd = CreateWindow(clsname,					/* window class		*/
		GetAppTitle(),								/* window name		*/
		SETUP_WINDOW_STYLE | WS_SYSMENU,			/* window style		*/
		x,											/* x position		*/
		y,											/* y position		*/
		width,										/* width			*/
		height,										/* height			*/
		NULL,										/* parent handle	*/
		NULL,										/* menu or child ID	*/
		hInstance,									/* instance			*/
		NULL);										/* additional info	*/

	if (!hWnd)	/* Was the window created? */
		return (0);
	
	SetWndParent(hWnd);
	
	// if this isn't an uninstall, show the buttons
	if(!GetRemovingApp())
	{
		SetupButtons(hWnd);
	}

	SetPromptDelete(true);
	ShowWindow(hWnd, ((GetRemovingApp())? SW_HIDE : SW_SHOWNORMAL));	/* Shows the window			*/
	UpdateWindow(hWnd);															/* Sends WM_PAINT message	*/

	if(GetRemovingApp())
	{
		PostMessage(hWnd,WM_UNINSTALL,0,0);
	}
	
	SetWindowLong(hWnd,GWL_STYLE,(GetWindowLong(hWnd,GWL_STYLE) | WS_CLIPCHILDREN));
	
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


	UnregisterClass(clsname,hInst);

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
		//
		// don't nuke the setup resource.dll unless uninstall was called or the resource dll was bootstrapped.
		//
		if (GetBootstrapFlag() || GetRanUninstall() )
			DeleteFile(szDllPath);

		if ( GetRanUninstall() )  // setup is using this, so nuke it if uninstalling
		{
			FreeLibrary(GetModuleHandle("DACOM.DLL"));
			DeleteFile("DACOM.DLL");
		}
	}

	if (hRichEditLib)
	{
		FreeLibrary(hRichEditLib);
		hRichEditLib = NULL;
	}
	DeleteMyself(FALSE);			/* clean ourselves up from the temp directory on exit */
	return (mmsg.wParam);			/* Returns the value from PostQuitMessage */
}

/****************************************************************************
	FUNCTION: SetupButtons(HWND)

	PURPOSE: adds buttons defined in appspecific.h to the button container

	COMMENTS:

	This was added so that buttons could be created in any order as specified
	in appspecific.h
****************************************************************************/
void SetupButtons(HWND hWnd)
{
	int i;
	for (i = 0; i < SETUP_NUM_BUTTONS; i++)
	{
		switch (ButtonList[i].BitID)
		{
			case INSTALL:
				if (pInstall != NULL)
					ButContainer.Remove(pInstall);
				pInstall = new InstallButton(hWnd,&ButtonList[i],INSTALL_SOUND_FILE);
				ButContainer.Add(pInstall);
				break;

			case UNINSTALL:
				if (pUnInstall != NULL)
					ButContainer.Remove(pUnInstall);
				pUnInstall = new UnInstallButton(hWnd,&ButtonList[i],UNINSTALL_SOUND_FILE);
				ButContainer.Add(pUnInstall);
				break;

			case REINSTALL:
				if (pReInstall != NULL)
					ButContainer.Remove(pReInstall);
				pReInstall = new ReInstallButton(hWnd,&ButtonList[i],REINSTALL_SOUND_FILE);
				ButContainer.Add(pReInstall);
				break;

			case EXIT:
				if (pExit != NULL)
					ButContainer.Remove(pExit);
				pExit = new QuitButton(hWnd,&ButtonList[i],EXIT_SOUND_FILE);
				ButContainer.Add(pExit);
				break;

			case PLAY:
				if (pPlay != NULL)
					ButContainer.Remove(pPlay);
				pPlay = new PlayButton(hWnd,&ButtonList[i],PLAY_SOUND_FILE);
				ButContainer.Add(pPlay);
				break;

			case WEBLINK:
				if (pWebLink != NULL)
					ButContainer.Remove(pWebLink);
				pWebLink = new WebLinkButton(hWnd,&ButtonList[i],WEBLINK_SOUND_FILE);
				ButContainer.Add(pWebLink);
				break;

			case CONFIGURE:
				if (pConfigure != NULL)
					ButContainer.Remove(pConfigure);
				pConfigure = new ConfigureButton(hWnd,&ButtonList[i],CONFIGURE_SOUND_FILE);
				ButContainer.Add(pConfigure);
				break;

			case README:
				if (pReadMe != NULL)
					ButContainer.Remove(pReadMe);
				pReadMe = new ReadMeButton(hWnd,&ButtonList[i],README_SOUND_FILE);
				ButContainer.Add(pReadMe);
				break;

			default:
				break;
		}
	}
}


/****************************************************************************

	FUNCTION: SetupInit(HANDLE)

	PURPOSE: Initializes window data and registers window class

	COMMENTS:

	Sets up a structure to register the window class.  Structure includes
	such information as what function will process messages, what cursor
	and icon to use, etc.

****************************************************************************/
BOOL SetupInit(HINSTANCE hInstance)
{
	HANDLE hMemory;				/* handle to allocated memory */
	PWNDCLASS pWndClass;		/* structure pointer	     */
	BOOL bSuccess;				/* RegisterClass() result     */

	hMemory = LocalAlloc(LPTR, sizeof(WNDCLASS));
	
	pWndClass = (PWNDCLASS) LocalLock(hMemory);

	pWndClass->style			= CS_GLOBALCLASS; /*CS_HREDRAW | CS_VREDRAW; */
	pWndClass->lpfnWndProc		= (WNDPROC)SetupWndProc;
	pWndClass->hInstance		= hInstance;
	pWndClass->hIcon			= LoadIcon(GetAppInst(),"0-INSTALL");
	pWndClass->hCursor			= LoadCursor(NULL, IDC_ARROW);
	pWndClass->hbrBackground	= (HBRUSH) GetStockObject(BLACK_BRUSH);
//	pWndClass->hbrBackground	= GetSysColorBrush(COLOR_3DFACE);
	pWndClass->lpszMenuName		= NULL;
	pWndClass->lpszClassName	= (LPSTR)clsname;

	bSuccess = RegisterClass(pWndClass);
	LocalUnlock(hMemory);		/* Unlocks the memory    */
	LocalFree(hMemory);			/* Returns it to Windows */
	return (bSuccess);			/* Returns result of registering the window */
}

/****************************************************************************

	FUNCTION: SetupWndProc(HWND, unsigned, WORD, LONG)

	PURPOSE:  Processes messages

	MESSAGES:

	WM_SYSCOMMAND - system menu (About dialog box)
	WM_CREATE     - create window
	WM_DESTROY    - destroy window
	WM_COMMAND    - application menus (Connect and Select dialog boxes

	COMMENTS:

	To process the ID_ABOUTSQL message, call MakeProcInstance() to get the
	current instance address of the About() function.  Then call Dialog
	box which will create the box according to the information in your
	Setup.rc file and turn control over to the About() function.	When
	it returns, free the intance address.
	This same action will take place for the two menu items Connect and
	Select.

****************************************************************************/
long FAR PASCAL SetupWndProc(HWND hWnd, WORD message, WPARAM wParam, LPARAM lParam)
{
	static bMoving = FALSE;
	static RECT rWindowPos;
	static WORD wXpos,wYpos;
	LRESULT retcode = 0;
	/* functions 			     */
	switch (message)
	{
	case WM_ACTIVATE:
		switch LOWORD(wParam)
		{
		case WA_ACTIVE:
		case WA_CLICKACTIVE:
			retcode = (DefWindowProc(hWnd, message, wParam, lParam));
			if (hFocusWnd)
			{
				SetFocus(hFocusWnd);
			}
			else
			{
				if( ( HasAppEverBeenLaunched(APP_MUST_LAUNCH) ) == TRUE )
				{
					if (pPlay)
						pPlay->Focus(); 
				}
				else
				{	if (pInstall)
						pInstall->Focus();
				}
			}
			return retcode;
		break;
		case WA_INACTIVE:
			hFocusWnd = GetFocus();
		}
		return (DefWindowProc(hWnd, message, wParam, lParam));
	break;
	case WM_SYSCOMMAND:		/* message: command from system menu */
		switch (wParam)			/* menu in WORD parameter   */
		{
		   case SC_SCREENSAVE:
			 return(0);
		}
		return (DefWindowProc(hWnd, message, wParam, lParam));

     break;
	case WM_LBUTTONDOWN:
		{
			if (!SETUP_BLACKOUT_SCREEN) // if fullscreen, don't pass the message and allow dragging the window
			{
				PostMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
			}
		}

    break;
	case WM_PAINT:
		{
			HDC hMemDC, hdc;
 			PAINTSTRUCT ps;
			RECT rect;
			HBITMAP hBitmap,hOldBitmap;
			HPALETTE hPalette;
			BITMAP bm;
			TCHAR szBuffer[512];
			HFONT hFont, hOldFont;
			COLORREF crColor;

			// Display bitmap background of window
			HDC hPDC = BeginPaint(hWnd,&ps);
			hBitmap = ::LoadResourceBitmap(GetResourceInst(),NULL, MAKEINTRESOURCE(gCurrentBitmapID), &hPalette);
			GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
			hdc = GetDC(hWnd);
		 	hMemDC = CreateCompatibleDC(hdc);
			SelectPalette(hdc,hPalette,FALSE);
			RealizePalette(hdc);
			SelectPalette(hMemDC,hPalette,FALSE);
			RealizePalette(hMemDC);
			hOldBitmap = (HBITMAP)SelectObject(hMemDC,hBitmap);
			BitBlt(hdc,X0,Y0,bm.bmWidth,bm.bmHeight,hMemDC,0,0,SRCCOPY);
			DeleteObject(SelectObject(hMemDC,hOldBitmap));
			DeleteDC(hMemDC);
			DeleteObject(hPalette);

			// Show all member controls
			ButContainer.ProcessMessage(message,wParam,lParam);

			// Display Copyright Stuff
			rect.left = X0;
			rect.top = Y0 + SETUP_WINDOW_HEIGHT-30;
			rect.right = X0 + SETUP_WINDOW_WIDTH;
			rect.bottom = Y0 + SETUP_WINDOW_HEIGHT;

			EBULoadString(GetResourceInst(), STR_COPYRIGHT_NOTICE, szBuffer,   sizeof(szBuffer));
			hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT); 
			hOldFont= (HFONT) SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			crColor = (COLORREF) GetSysColor (SETUP_COPYRIGHT_TEXT_COLOR);
			SetTextColor(hdc, crColor);
			DrawText( hdc, szBuffer, lstrlen(szBuffer), &rect, DT_BOTTOM | DT_CENTER | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER );

			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);

			ReleaseDC(hWnd,hdc);
			EndPaint(hWnd,&ps);

		}
		break;

	case WM_UNINSTALL:
		// Only sent if uninstal.exe was executed - not when uninstall button was selected from setup
		char msg[120];
		if(GetPromptDelete())
		{
		  EBULoadString(GetResourceInst(),STR_PROMPTUNINSTALL,msg,sizeof(msg));
		  if(MessageBox(hWnd,msg,GetAppTitle(),MB_OKCANCEL |MB_DEFBUTTON2) == IDCANCEL)
		  {
			  DestroyWindow(hWnd);
			  SetRanUninstall(false);
			  break;
		  }
        }
		UninstallApp(hWnd,APP_IS_DEMO_VERSION);
		EBULoadString(GetResourceInst(),STR_UNINSTALLED,msg,sizeof(msg));
		MessageBox(hWnd,msg,GetAppTitle(),MB_OK);
		DestroyWindow(hWnd);
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			if(HasAppEverBeenLaunched(APP_MUST_LAUNCH) == TRUE)
				if (pPlay != NULL)		pPlay->ButtonClicked();
			else
				if (pInstall != NULL)	pInstall->ButtonClicked();
			break;
		case IDCANCEL:
			pExit->ButtonClicked();
			break;
		default:
			ButContainer.ProcessMessage(message,wParam,lParam);
		}
		break;

	case WM_START_BILLBOARDS:
		DisplayNextBillboard(0,0,0,0);
		SetTimer(GetWndParent(),1,BILLBOARD_DURATION,(TIMERPROC) DisplayNextBillboard);
		break;

	case WM_STOP_BILLBOARDS:
		KillTimer(hWnd,1);
		break;

	case WM_DESTROY:		  /* message: window being destroyed */
		KillTimer(hWnd,1);
 	    PostQuitMessage(0);
		break;

	default:			  /* Passes it on if unproccessed    */
		return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	return (0);
}  

//
//	Button Commands
//
void InstallButton::ButtonClicked()
{
	int retc;
	int nMsgBoxResult=0;

	do // show EULA dialog
	{
		EULADlg *d = new EULADlg();
		int bretc;
		if(d)
		{
			bretc = (int)d->start();
			delete d;
		}
		if(bretc == IDCANCEL || bretc == IDABORT)
		{
//			DestroyWindow(parenthWnd);
			return;
		}
		else
		{
			break;
		}
	}	while (TRUE);
	
	// check requirements
	REQUIREMENTS req;
	req.cColors = REQ_COLORS;
	req.cBitsPixel = REQ_BIT_DEPTH;
	req.dwProcessorType = INTELPENTIUM;
	req.dwTotalPhys = 7630;
	req.dwRequiredPhys = 8;
	req.wCDRom = 4;
	req.dwResolution = MAKELONG(REQ_VERT_RES,REQ_HOR_RES);
	
	if(CheckHardware(fLaunched,&req)==FALSE)
	{
		DestroyWindow(GetWndParent());
		return;
	}
	
	enable(FALSE);
	if (pExit != NULL)		pExit->enable(FALSE);
	if (pWebLink != NULL)	pWebLink->enable(FALSE);
	if (pReInstall != NULL)	pReInstall->enable(FALSE);
	if (pUnInstall != NULL)	pUnInstall->enable(FALSE);
	if (pConfigure != NULL)	pConfigure->enable(FALSE);
	if (pReadMe != NULL)	pReadMe->enable(FALSE);

	if (SETUP_HIDE_BUTTONS_DURING_INSTALL)
	{
		ShowWindow(this->hButWnd, SW_HIDE);
		if (pExit != NULL)		pExit->show(SW_HIDE);
		if (pWebLink != NULL)	pWebLink->show(SW_HIDE);
		if (pReInstall != NULL)	pReInstall->show(SW_HIDE);
		if (pUnInstall != NULL)	pUnInstall->show(SW_HIDE);
		if (pConfigure != NULL)	pConfigure->show(SW_HIDE);
		if (pReadMe != NULL)	pReadMe->show(SW_HIDE);
	}

	if((retc = InstallApp(GetWndParent(),HasAppEverBeenLaunched(APP_MUST_LAUNCH),FALSE))==EBU_OK)
	{
		// install script was completed succesfully

		// turn the sound off abrubtly
		EBUPlaySound("STOP PLAYING THE SOUND", NULL, SND_FILENAME | SND_NODEFAULT);

		if(GetRebootFlag())
		{
			char buf[256];
			EBULoadString(GetResourceInst(),STR_RESTART,buf,256);
			if(MessageBox(GetWndParent(),buf,GetAppTitle(),MB_YESNO) == IDOK)
			{
				FreeLibrary(GetResourceInst());
				DeleteFile(lpSetupEnu);
				ExitWindowsEx(EWX_REBOOT, 0);
			}
			else
			{
				 EBULoadString(GetResourceInst(),STR_MUSTRESTART,buf,100);
				 MessageBox(GetWndParent(),buf,GetAppTitle(),MB_OK);
				 DestroyWindow(GetWndParent());
				 FreeLibrary(GetResourceInst());
				 DeleteFile(lpSetupEnu);
			}
			return;
		}
		ShowWindow(this->hButWnd, SW_HIDE);
		if (pPlay != NULL)		pPlay->enable(TRUE);
		if (pPlay != NULL)		ShowWindow(pPlay->hButWnd, SW_SHOW);
		if (pPlay != NULL)		SetFocus(pPlay->hButWnd);
		if (pPlay != NULL)		pPlay->fLaunched = TRUE;

		if (SETUP_HIDE_BUTTONS_DURING_INSTALL) // show the buttons if they were hidden
		{
			if (pExit != NULL)		pExit->show(SW_SHOW);
			if (pWebLink != NULL)	pWebLink->show(SW_SHOW);
			if (pReInstall != NULL)	pReInstall->show(SW_SHOW);
			if (pUnInstall != NULL)	pUnInstall->show(SW_SHOW);
			if (pConfigure != NULL)	pConfigure->show(SW_SHOW);
			if (pReadMe != NULL)	pReadMe->show(SW_SHOW);
		}

		if (pReInstall != NULL)	pReInstall->enable(TRUE);
		if (pUnInstall != NULL)	pUnInstall->enable(TRUE);
		if (pWebLink != NULL)	pWebLink->enable(TRUE);
		if (pExit != NULL)		pExit->enable(TRUE);
		if (pConfigure != NULL)	pConfigure->enable(TRUE);
		if (pReadMe != NULL)	pReadMe->enable(TRUE);
		fLaunched = TRUE;
	}
	else 
	{
		gCurrentBitmapID = SETUP_INITIAL_BITMAP_ID; // show default bg
		// turn the sound off abrubtly
		EBUPlaySound("STOP PLAYING THE SOUND", NULL, SND_FILENAME | SND_NODEFAULT);
		if (SETUP_HIDE_BUTTONS_DURING_INSTALL) // show the buttons if they were hidden
		{
			ShowWindow(this->hButWnd, SW_SHOW);
			if (pExit != NULL)		pExit->show(SW_SHOW);
			if (pWebLink != NULL)	pWebLink->show(SW_SHOW);
			if (pReInstall != NULL)	pReInstall->show(SW_SHOW);
			if (pUnInstall != NULL)	pUnInstall->show(SW_SHOW);
			if (pConfigure != NULL)	pConfigure->show(SW_SHOW);
			if (pReadMe != NULL)	pReadMe->show(SW_SHOW);
		}
		if (pReadMe != NULL)	pReadMe->enable(TRUE);
		if (pWebLink != NULL)	pWebLink->enable(TRUE);
		if (pExit != NULL)		pExit->enable(TRUE);

		if(retc == EBU_ABORT) // install cancelled by user
		{
			UninstallApp(GetWndParent(),APP_IS_DEMO_VERSION);
		}

		SetCurrentDirectory(GetSourcePath());
		enable(TRUE);
	}
	Told = FALSE;
	InvalidateRect(GetWndParent(), NULL, TRUE);
	UpdateWindow(GetWndParent());		/* Sends WM_PAINT message	*/
}

void PlayButton::ButtonClicked()
{
	REQUIREMENTS req;
	req.cColors = REQ_COLORS;
	req.cBitsPixel = REQ_BIT_DEPTH;
	req.dwProcessorType = INTELPENTIUM;
	req.dwTotalPhys = 7630;
	req.dwRequiredPhys = 8;
	req.wCDRom = 4;
	req.dwResolution = MAKELONG(REQ_VERT_RES,REQ_HOR_RES);

	if(CheckHardware(fLaunched,&req)==FALSE)
	{
		DestroyWindow(GetWndParent());
		return;
	}
	
	if(fLaunched) // if setup has been completed
	{
		HCURSOR hCurCursor = SetCursor(LoadCursor(NULL,IDC_WAIT));
		DestroyWindow(GetWndParent());
		LaunchApplication (STR_LAUNCHEXE,STR_COMMANDLINE);
	}
}

void QuitButton::ButtonClicked() 
{ 
	if(HasAppEverBeenLaunched(APP_MUST_LAUNCH) == FALSE)
	{
		  if(ReallyQuit(parenthWnd)==IDOK)
		  {
			 Sleep(500); DestroyWindow(parenthWnd);
		  }
	}
	else
	{
		Sleep(1000); DestroyWindow(parenthWnd);
	}
}

void ReInstallButton::ButtonClicked() 
{
	enable(FALSE);
	if (pPlay != NULL)		pPlay->enable(FALSE);
	if (pExit != NULL)		pExit->enable(FALSE);
	if (pUnInstall != NULL)	pUnInstall->enable(FALSE);
	if (pConfigure != NULL)	pConfigure->enable(FALSE);
	if (pReadMe != NULL)	pReadMe->enable(FALSE);

	InstallApp(parenthWnd,TRUE,FALSE);
//	if(InstallApp(parenthWnd,TRUE,FALSE) == EBU_ABORT)
//	{
//		DestroyWindow(parenthWnd);
//	}
//	else
//	{
	enable(TRUE);
	if (pPlay != NULL)		pPlay->enable(TRUE);
	if (pPlay != NULL)		pPlay->fLaunched = TRUE;
	if (pInstall != NULL)	pInstall->fLaunched = TRUE;
	if (pUnInstall != NULL)	pUnInstall->enable(TRUE);
	if (pExit != NULL)		pExit->enable(TRUE);
	if (pConfigure != NULL)	pConfigure->enable(TRUE);
	if (pReadMe != NULL)	pReadMe->enable(TRUE);
//	}
}

void UnInstallButton::ButtonClicked()
{
	HCURSOR hCurCursor = SetCursor(LoadCursor(NULL,IDC_WAIT));

	enable(FALSE);
	if (pPlay != NULL)		pPlay->enable(FALSE);
	if (pReInstall != NULL)	pReInstall->enable(FALSE);
	if (pConfigure != NULL)	pConfigure->enable(FALSE);
	if (pReadMe != NULL)	pReadMe->enable(FALSE);

	char msg[120];
	if(GetPromptDelete())
	{
		EBULoadString(GetResourceInst(),STR_PROMPTUNINSTALL,msg,sizeof(msg));
		if(MessageBox(GetWndParent(),msg,GetAppTitle(),MB_OKCANCEL |MB_DEFBUTTON2) == IDCANCEL)
		{	
			enable(TRUE);
			if (pPlay != NULL)		pPlay->enable(TRUE);
			if (pReInstall != NULL)	pReInstall->enable(TRUE);
			if (pConfigure != NULL)	pConfigure->enable(TRUE);
			if (pReadMe != NULL)	pReadMe->enable(TRUE);
			SetCursor(hCurCursor);
			return;
		}
	}

	if(UninstallApp(GetWndParent(),FALSE) == IDABORT)
	{
		enable(TRUE);
		if (pPlay != NULL)		pPlay->enable(TRUE);
		if (pReInstall != NULL)	pReInstall->enable(TRUE);
		if (pConfigure != NULL)	pConfigure->enable(TRUE);
		if (pReadMe != NULL)	pReadMe->enable(TRUE);
	}
	else
	{
		EBULoadString(GetResourceInst(),STR_UNINSTALLED,msg,sizeof(msg));
		MessageBox(GetWndParent(),msg,GetAppTitle(),MB_OK);
		if (pPlay != NULL)		ShowWindow(pPlay->hButWnd, SW_HIDE);
		if (pInstall != NULL)	pInstall->enable(TRUE);
		if (pInstall != NULL)	ShowWindow(pInstall->hButWnd, SW_SHOW);
		if (pReadMe != NULL)	pReadMe->enable(TRUE);
		UpdateWindow(GetWndParent());		/* Sends WM_PAINT message	*/
		if (pInstall != NULL)	pInstall->fLaunched=FALSE;
		if (pInstall != NULL)	SetFocus(pInstall->hButWnd);
		if (pPlay != NULL)		pPlay->fLaunched=FALSE;
		fbRetailInstalled = FALSE;
	}
	SetCursor(hCurCursor);
}

void WebLinkButton::ButtonClicked()
{
	HCURSOR hCurCursor = SetCursor(LoadCursor(NULL,IDC_WAIT));
	LaunchUrl (STR_LAUNCHURL);
	Sleep(500);
	SetCursor(hCurCursor);
}

void ConfigureButton::ButtonClicked()
{	// show the device selection dialog box
	char szDXVersion[32];
	EBULoadString(GetAppInst(), STR_DX_MIN_VERSION, szDXVersion, sizeof(szDXVersion) );
	int retVal = CheckDXVersion(szDXVersion, "dsetup");
	if ((EV_EXISTING_SAME == retVal) || (EV_EXISTING_NEWER == retVal))
	{
		LaunchApplication (STR_CONFIG_EXE_NAME,0);
	}
	else
	{
		char msg[256];
		EBULoadString(GetResourceInst(),STR_NODIRECTXCD,msg,256);
		MessageBox(GetWndParent(),msg,GetAppTitle(),MB_OK);
	}
}

void ReadMeButton::ButtonClicked()
{	// show the readme file
	SHELLEXECUTEINFO readMeInfo;
	readMeInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	readMeInfo.fMask = SEE_MASK_NO_CONSOLE;
	readMeInfo.hwnd = GetWndParent();
	readMeInfo.lpVerb = "open";
	readMeInfo.lpFile = SETUP_README_FILENAME;
	readMeInfo.lpParameters = "";
	readMeInfo.lpDirectory = GetSourcePath();
	readMeInfo.nShow = SW_SHOWNORMAL;

	ShellExecuteEx(&readMeInfo);
}

//
// Utility function for cleaning-up during self-extracting executable
//
void DeleteMyFile(char *szSource)
{
   char msg[_MAX_PATH];
   char *ptr;
   ptr = szSource;
   int x=0;
   while(*ptr != '\0')
   {
	  x=0;
      while(*ptr != ' ' && *ptr != '\0')
      {

         msg[x++] = *ptr++;
      }
      msg[x] = '\0';
	  ReplaceStringTokens(msg,_MAX_PATH);
      DeleteFile(msg);
	  if(*ptr == '\0')
		  break;
	  else
          ptr++;
   }
}

//
// Helper function to replace the older LaunchUrl engine call.
//
BOOL LaunchUrl(UINT uiURLStringID)
{
    char szLaunched[256];
    char AppDirectory[256];
	
    if(GetRebootFlag())
    {
        Alert(GetWndParent(), MB_OK | MB_ICONSTOP, STR_MUSTRESTART);
        return FALSE;
    }

	// Load the required strings
	// EBUShellExecute will execute ReplaceStringTokens for US
	// THIS GET US THE APPPATH KEY
    EBULoadString(GetResourceInst(), STR_REGKEY_VAL_APPPATH, szLaunched, sizeof(szLaunched) );
    MyGetPrivateProfileString( szLaunched, "", AppDirectory, sizeof(AppDirectory) );
    EBULoadString(GetResourceInst(), uiURLStringID, szLaunched, sizeof(szLaunched) );
	
		if (EBU_ERROR == EBUShellExecute(GetWndParent(),
										 szLaunched,
										 NULL,
										 AppDirectory,
										 SW_SHOWNORMAL,
										 EBUENGINE_SHELLEXECUTE,
										 STR_ERROR_INITFAILURE,
										 FALSE,  //don't wait
										 NULL))
		{
			return FALSE;
		}
	return TRUE;
}

//
//	GetGrpDlg -- Different dialog template treating a group as an optional component rather than size.
//	Custom get group dialog to allow the tester to maintain a stress client component install
//  This is never show in the application if a get group call isn't made.
//	Regardless group 0x0000000001 is always installed.  Since AC only offers a typical setup.
//
class GetGrpDlg : public Dialog {
public:
	GetGrpDlg() : Dialog(MAKEINTRESOURCE(GETGRPDLG)) 
	{
		TCHAR szSysDrive[_MAX_PATH]="";
		TCHAR szAppDrive[_MAX_PATH]="";
		GetSystemDirectory(szSysDrive, sizeof(szSysDrive));
		szSysDrive[3] = '\0';
		lstrcpy(szAppDrive, GetAppDir());
		szAppDrive[3] = '\0';
		if (!lstrcmpi(szSysDrive, szAppDrive))
		{
			SetSameDrive(TRUE);
		}
		else
		{
			SetSameDrive(FALSE);
		}
	}
	~GetGrpDlg() {}
	BOOL Init(LPARAM lParam);
	BOOL Command(WORD nId,WORD nNotify, LPARAM lParam);
	void SetSameDrive(BOOL bValue) {bSameDrive = bValue;};
	BOOL GetSameDrive() {return bSameDrive;};
	void SetGameSpaceNeeded(int iValue) {iGameSpaceNeeded = iValue;};
	int  GetGameSpaceNeeded() {return iGameSpaceNeeded;};
	void SetSystemSpaceNeeded(int iValue) {iSystemSpaceNeeded = iValue;};
	int  GetSystemSpaceNeeded() {return iSystemSpaceNeeded;};
	void GetGrpDlg::UpdateSpaceRequirements(__int64 iGroupValue);
private:
	LPGETGROUPDATA group;
	BOOL bSameDrive;
	int iGameSpaceNeeded;
	int iSystemSpaceNeeded;
};


BOOL GetGrpDlg :: Init(LPARAM lParam)
{
	TCHAR szBuffer[2048]="";
	char Buffer[MAX_PATH];
	int	iGroupIndex=1;
	int iTemporary = 0;
	__int64 i;

	// replace string tokens in window title
	GetWindowText(hDlg,Buffer,MAX_PATH);
	ReplaceStringTokens(Buffer,MAX_PATH);
	SetWindowText(hDlg,Buffer);

	// replace string tokens in text
	GetDlgItemText(hDlg,IDC_STATIC1,Buffer,MAX_PATH);
	ReplaceStringTokens(Buffer,MAX_PATH);
	SetDlgItemText(hDlg,IDC_STATIC1,Buffer);

	SetDlgItemText(hDlg,ID_DLGEDITCONTROL,GetAppDir());
	group = (LPGETGROUPDATA)lParam;
//	group->group = GetGroup();
	group->dwGameFreeSpace /= 1024;
	group->dwGameNeeded /= 1024;
	group->dwSystemFreeSpace /= 1024;
	group->dwSystemNeeded /= 1024;


	if (this->GetSameDrive())
	{
		itoa((int)group->dwGameFreeSpace, szBuffer, 10);
		lstrcat(szBuffer, " MB");
		SetDlgItemText(hDlg,ID_GAMEAVAIL,szBuffer);
		//Blank unused fields for same disk system game install
		SetDlgItemText(hDlg,ID_SYSTEMAVAIL,"");
		SetDlgItemText(hDlg,ID_TOTALAVAIL,"");
		SetGameSpaceNeeded(group->dwGameNeeded);
		for (i = group->group; i > 0;i=i>>1)
		{
			if (i & 1)
			{
//				SetGameSpaceNeeded(GetGameSpaceNeeded() + GetGroupSizes(iGroupIndex));
			}
			iGroupIndex++;
		}
		//dwGameFreespace = total game space + total system space on same drive installs
		SetSystemSpaceNeeded(group->dwSystemNeeded);
		itoa(GetGameSpaceNeeded()+GetSystemSpaceNeeded(), szBuffer, 10);
		lstrcat(szBuffer, " MB");
		SetDlgItemText(hDlg,ID_GAMEREQ,szBuffer);
		//Blank unused fields for same disk system game install
		SetDlgItemText(hDlg,ID_SYSTEMREQ,"");
		SetDlgItemText(hDlg,ID_TOTALREQUIRED, "");
		SetDlgItemText(hDlg,ID_SYSTEMTEXT,"");
		SetDlgItemText(hDlg,ID_TOTALTEXT,"");
	}
	else
	{
		//Display Available Space
		itoa((int)group->dwGameFreeSpace, szBuffer, 10);
		lstrcat(szBuffer, " MB");
		SetDlgItemText(hDlg,ID_GAMEAVAIL,szBuffer);
		itoa((int)group->dwSystemFreeSpace, szBuffer, 10);
		lstrcat(szBuffer, " MB");
		SetDlgItemText(hDlg,ID_SYSTEMAVAIL,szBuffer);
		itoa((((int)group->dwGameFreeSpace) + ((int)group->dwSystemFreeSpace)), szBuffer, 10);
		lstrcat(szBuffer, " MB");
		SetDlgItemText(hDlg,ID_TOTALAVAIL,szBuffer);
		//Display Required Space
		SetGameSpaceNeeded(group->dwGameNeeded);
		for (i = group->group; i > 0;i=i>>1)
		{
			if (i & 1)
			{
//				SetGameSpaceNeeded(GetGameSpaceNeeded() + GetGroupSizes(iGroupIndex));
			}
			iGroupIndex++;
		}
		itoa(GetGameSpaceNeeded(), szBuffer, 10);
		lstrcat(szBuffer, " MB");
		SetDlgItemText(hDlg,ID_GAMEREQ,szBuffer);
		//dwGameFreespace = total game space + total system space on same drive installs
		SetSystemSpaceNeeded((int) group->dwSystemNeeded);
		itoa(GetSystemSpaceNeeded(), szBuffer, 10);
		lstrcat(szBuffer, " MB");
		SetDlgItemText(hDlg,ID_SYSTEMREQ,szBuffer);
		itoa(GetSystemSpaceNeeded() + GetGameSpaceNeeded(), szBuffer, 10);
		lstrcat(szBuffer, " MB");
		SetDlgItemText(hDlg,ID_TOTALREQUIRED,szBuffer);
	}
	return TRUE;
}
void GetGrpDlg::UpdateSpaceRequirements(__int64 iGroupValue)
{
	TCHAR szBuffer[2048]="";
	int iGroupIndex=1;
	__int64 i = 0;
		SetGameSpaceNeeded(group->dwGameNeeded);
		for (i = iGroupValue; i > 0;i=i>>1)
		{
			if (i & 1)
			{
//				SetGameSpaceNeeded(GetGameSpaceNeeded() + GetGroupSizes(iGroupIndex));
			}
			iGroupIndex++;
		}
	if (this->GetSameDrive())
	{
		itoa(GetGameSpaceNeeded()+GetSystemSpaceNeeded(), szBuffer, 10);
		lstrcat(szBuffer, " MB");
		SetDlgItemText(hDlg,ID_GAMEREQ,szBuffer);
	}
	else
	{
		itoa(GetGameSpaceNeeded(), szBuffer, 10);
		lstrcat(szBuffer, " MB");
		SetDlgItemText(hDlg,ID_GAMEREQ,szBuffer);
		itoa(GetSystemSpaceNeeded() + GetGameSpaceNeeded(), szBuffer, 10);
		lstrcat(szBuffer, " MB");
		SetDlgItemText(hDlg,ID_TOTALREQUIRED,szBuffer);
	}
}

BOOL GetGrpDlg::Command(WORD nId,WORD nNotify, LPARAM lParam)
{
	int iCheckedState=0;
	switch(nId)
	{
		case IDC_COMPONENT1:
			iCheckedState=SendDlgItemMessage(hDlg, IDC_COMPONENT1, BM_GETCHECK, 0,0);
			switch (iCheckedState)
			{
			case BST_CHECKED:
				group->group = 1;
				GetGrpDlg::UpdateSpaceRequirements(group->group);
				SendDlgItemMessage(hDlg, IDC_COMPONENT1, BM_SETCHECK, BST_UNCHECKED,0);
				break;
			case BST_UNCHECKED:
				group->group = 3;
				GetGrpDlg::UpdateSpaceRequirements(group->group);
				SendDlgItemMessage(hDlg, IDC_COMPONENT1, BM_SETCHECK, BST_CHECKED,0);
				break;
			case BST_INDETERMINATE:
			default:
				break;
			}
			return FALSE;
			break;
		case ID_BACK:
		  EndDialog(hDlg,EBU_BACK);
		  return(TRUE);
		default:
		return FALSE;
	}
	EndDialog(hDlg,EBU_OK);

	return TRUE;
}

//
//	Custom Engine call backs for GetGrp and MkRoot for the preview II setup
//	These replace Craig's standard callback for the generic flat stuff.
//
EBURETCODE WINAPI GetGrpFn(LPGETGROUPDATA group)
{
	EBURETCODE retc= EBU_CANCEL;
	GetGrpDlg *gg = new GetGrpDlg();
	if(gg)
	{
       retc = gg->start((LPARAM)group);
	   delete gg;
	}
	return retc;
}

EBURETCODE WINAPI MyMkRootFn(LPMKROOTDATA mk)
{
	EBURETCODE retc=EBU_CANCEL;
	MyMkRootDlg *mkd = new MyMkRootDlg();
	if(mkd)
	{
       retc = mkd->start((LPARAM)mk);
	   delete mkd;
	}
	return retc;
}

// 
//	Implementation of MyDialog for allowing cutomizing of the edit control without killing Craig's code
//	This only differs in that it handles WM_DESTROY to clean up the sub-classing.
//
EBURETCODE MyDialog::start(LPARAM lParam)
{
   return (EBURETCODE)DialogBoxParam(GetResourceInst(),szTemplate, GetWndParent(),(DLGPROC)MyCPPDlgProc,(LPARAM)lParam);
}

EBURETCODE MyDialog::start()
{
   return (EBURETCODE)DialogBox(GetResourceInst(),szTemplate, GetWndParent(),(DLGPROC)MyCPPDlgProc);
}

_declspec(dllexport) BOOL CALLBACK MyCPPDlgProc (HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
	if(glpMyDialog)
		return glpMyDialog->ProcessCommand(hDlg,msg,wParam,lParam);
	else
		return FALSE;
}

BOOL MyDialog::ProcessCommand(HWND hwnd, WORD msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_NOTIFY:
			return Notify(wParam, lParam);
		case WM_INITDIALOG:
		    hDlg = hwnd;
		    return Init(lParam);
	    case WM_COMMAND:
		
		switch(LOWORD(wParam))
		{
			case IDOK:
					return Ok();
			case IDCANCEL:
				return Cancel();
			case IDHELP:
				return(Help(HIWORD(wParam)));
			default:
				return Command(LOWORD(wParam),HIWORD (wParam),lParam);
        }
		case WM_ACTIVATE:
		    return ((LOWORD(wParam) == WA_INACTIVE) ? Activate(FALSE) : Activate(TRUE));
		case WM_DESTROY:
			if (wpOrigEditProc)
			{
				SetWindowLong(hwndEdit, GWL_WNDPROC, (LONG) wpOrigEditProc);
				wpOrigEditProc = NULL;
			}
			return Destroy();
		default:
		    return FALSE;
    }
}

//
//	MyMkRootDlg -- fixes a user nuisance problem with empty text boxes.
//	Requested by the AC group.
//	Also adds token replacement in title and text.
BOOL MyMkRootDlg::Init(LPARAM lParam)
{
	char Buffer[MAX_PATH];
	mk = (LPMKROOTDATA)lParam;

	// replace string tokens in window title
	GetWindowText(hDlg,Buffer,MAX_PATH);
	ReplaceStringTokens(Buffer,MAX_PATH);
	SetWindowText(hDlg,Buffer);

	// replace string tokens in text
	GetDlgItemText(hDlg,IDC_STATIC1,Buffer,MAX_PATH);
	ReplaceStringTokens(Buffer,MAX_PATH);
	SetDlgItemText(hDlg,IDC_STATIC1,Buffer);

	// Retrieve the handle to the edit control.
	hwndEdit = GetDlgItem(hDlg, ID_DLGEDITCONTROL);
	
	// Initialize the value, selection, Focus.
	SetDlgItemText(hDlg,ID_DLGEDITCONTROL,mk->szAppDir);
	SendDlgItemMessage(hDlg,ID_DLGEDITCONTROL,EM_SETSEL,0,-1);
	SendDlgItemMessage(hDlg,ID_DLGEDITCONTROL,EM_LIMITTEXT,_MAX_PATH/2,0);
	SetFocus(GetDlgItem(hDlg,ID_DLGEDITCONTROL));
	return TRUE;
}

BOOL MyMkRootDlg::Browse()
{
	char Buffer[MAX_PATH];
	char fullPath[MAX_PATH];
	LPITEMIDLIST pidlBrowse;

	BROWSEINFO bi;
	bi.hwndOwner=hDlg;
	bi.pidlRoot=NULL;
	bi.pszDisplayName = Buffer;
	bi.lpszTitle = GetAppTitle();
	bi.ulFlags=BIF_RETURNONLYFSDIRS;
	bi.lpfn=NULL;
	bi.lParam=0;
	bi.iImage = NULL;
	pidlBrowse = SHBrowseForFolder(&bi);
	if(pidlBrowse != NULL)
	{
		if(SHGetPathFromIDList(pidlBrowse,Buffer))
		{
			sprintf(fullPath,"%s\\%s",Buffer,GetAppTitle());
			SetDlgItemText(hDlg,ID_DLGEDITCONTROL,fullPath);
		}
		LPMALLOC lpMalloc;

		if (SUCCEEDED(SHGetMalloc(&lpMalloc)))
		{
			lpMalloc->Free(pidlBrowse);
			lpMalloc->Release();
		}
	}

    return TRUE;
}

BOOL MyMkRootDlg::Command(WORD nId,WORD nNotify, LPARAM lParam)
{
	switch(nId)
	{
		case ID_BROWSE:
			return Browse();
		default:
			return FALSE;
	}
}

BOOL MyMkRootDlg::Ok()
{
	TCHAR tcDrive='\0';
	DWORD dwGameFreeSpace = 0;
	DWORD dwGameNeeded = 0;
	DWORD dwSystemFreeSpace = 0;
	DWORD dwSystemNeeded = 0;
	__int64 i64Group = 0x0000000000000001;
	BOOL fSameDrive = FALSE;

	GetDlgItemText(hDlg,ID_DLGEDITCONTROL,mk->UserRootEntry,sizeof(mk->UserRootEntry));
	//
	// Don't bother checking a empty string.  Restore the Edit Control to the Default Directory
	//
	if (*mk->UserRootEntry)
	{
		// Is the function pointer okay?
		if(mk->lpfnValidateEntry)
		{
			int retc;
			// Is the directory text okay
			if((retc = (*mk->lpfnValidateEntry)(mk->UserRootEntry)) == EBU_OK)
			{
				
				// if the directory doesn't exist, prompt the user before creating it
				char oldDir[256];
				GetCurrentDirectory(256, oldDir);
				if (! SetCurrentDirectory(mk->UserRootEntry))
				{
					char resourcestr[200];
					char message[256];
					EBULoadString(GetResourceInst(),STR_CREATEDIR,resourcestr,200);
					sprintf(message,resourcestr,mk->UserRootEntry);

					if (IDOK != MessageBox(hDlg, message, GetAppTitle(), MB_OKCANCEL))
						return EBU_CANCEL;
				}
				SetCurrentDirectory(oldDir);

				//
				// If directory is valid then Check disk space requirements for group 0x1,
				// because this flat doesn't have a group screen.  Except for internals build for stress testing.
				//
				TCHAR szSysDrive[_MAX_PATH]="";
				TCHAR szAppDrive[_MAX_PATH]="";
				GetSystemDirectory(szSysDrive, sizeof(szSysDrive));
				szSysDrive[3] = '\0';
				lstrcpy(szAppDrive, mk->UserRootEntry);
				szAppDrive[3] = '\0';

				//
				//	Are the system and application drives the same.
				//
				if (!lstrcmpi(szSysDrive, szAppDrive))
				{
					fSameDrive = TRUE;
				}
				else
				{
					fSameDrive = FALSE;
				}
				tcDrive = szAppDrive[0];
				//
				// Check Space requirements and act accordingly
				//
				AppGetFileSizeRequirements(tcDrive, 
									i64Group, 
									&dwGameFreeSpace,
									&dwGameNeeded,
									&dwSystemFreeSpace,
									&dwSystemNeeded);
				TCHAR	szBuf[MAX_PATH]="";
				TCHAR	szMessage[MAX_PATH]="";
// convert to MB per MS
				dwGameFreeSpace /= 1024;
				dwGameNeeded /= 1024;
				dwSystemFreeSpace /= 1024;
				dwSystemNeeded /= 1024;
//
				if (fSameDrive)
				{
					if (dwGameFreeSpace < dwSystemNeeded + dwGameNeeded)
					{
						// not enough disk space...so display special alert
						//lstrcpy (szBuf,"Not enough disk space on drive %s.\n  %d KB more needed");
						EBULoadString(GetResourceInst(), IDS_NOT_ENUF_DISK_SPACE, szBuf, sizeof(szBuf));
						wsprintf(szMessage,szBuf,szAppDrive, (dwSystemNeeded + dwGameNeeded - dwGameFreeSpace) );
						MessageBox(hDlg, szMessage, GetSetupTitle(), MB_OK | MB_ICONEXCLAMATION);
						return (FALSE);
					}
				}
				else
				{
					if (dwGameFreeSpace < dwGameNeeded)
					{
						// not enough disk space...so display special alert
						//lstrcpy (szBuf,"Not enough game disk space on drive %s.\n  %d KB more needed");
						EBULoadString(GetResourceInst(), IDS_NOT_ENUF_APP_SPACE, szBuf, sizeof(szBuf));
						wsprintf(szMessage,szBuf,szAppDrive, (dwGameNeeded - dwGameFreeSpace) );
						MessageBox(hDlg, szMessage, GetSetupTitle(), MB_OK | MB_ICONEXCLAMATION);
						return (FALSE);
					}
					if (dwSystemFreeSpace < dwSystemNeeded)
					{
						// not enough disk space...so display special alert
						//lstrcpy (szBuf,"Not enough system disk space on drive %s.\n  %d KB more needed");
						EBULoadString(GetResourceInst(), IDS_NOT_ENUF_SYS_SPACE, szBuf, sizeof(szBuf));
						wsprintf(szMessage,szBuf,szSysDrive, (dwSystemNeeded - dwSystemFreeSpace) );
						MessageBox(hDlg, szMessage, GetSetupTitle(), MB_OK | MB_ICONEXCLAMATION);
						return (FALSE);
					}
				}

				EndDialog(hDlg,EBU_OK);
				return(TRUE);
			}
			// Aborting.
			else if(retc == EBU_ABORT)
			{
				EndDialog(hDlg,EBU_ABORT);
				return(TRUE);
			}
			// Bad Directory
			else
			{
				// Restore focus to the mkroot dialog, because the engine steals it.
				SetFocus(GetDlgItem(hDlg,ID_DLGEDITCONTROL));
				return (TRUE);
			}
		}
		// Bad Function Pointer
		else
		{
			EndDialog(hDlg,EBU_OK);
			return(TRUE);
		}
	}
	// No User entry at all.
	else
	{
		SetDlgItemText(hDlg,ID_DLGEDITCONTROL,mk->szAppDir);
		SendDlgItemMessage(hDlg,ID_DLGEDITCONTROL,EM_SETSEL,0, -1);
		SetFocus(GetDlgItem(hDlg,ID_DLGEDITCONTROL));
	}
    return (FALSE);
}

//
// EulaDLG
//

// 
// Preserve selection and indicate focus with a background color change.
//
LRESULT CALLBACK EulaEditSubclassProc(HWND hwnd,	UINT uMsg,	WPARAM wParam,	LPARAM lParam)
{
	COLORREF crCurrent = 0;
	RECT rect;
	LRESULT retcode=0;

	if (uMsg == WM_GETDLGCODE) 
		return DLGC_WANTARROWS;  
	switch (uMsg) 
	{
	case WM_SETFOCUS:
		// Test for a selection and Hide the Caret if necessary
		crCurrent = (COLORREF) GetSysColor (COLOR_3DHILIGHT); 
		SendMessage(hwnd, EM_SETBKGNDCOLOR, FALSE, (LPARAM) crCurrent);
		SendMessage(hwnd, EM_GETRECT, NULL, (LPARAM) &rect);
		InvalidateRect(hwnd, &rect, TRUE);
		retcode = CallWindowProc(wpOrigEditProc, hwnd, uMsg, wParam, lParam);
		return retcode;
		break;
	case WM_KILLFOCUS: 
		// Test for a selection and Hide the Caret if necessary
		crCurrent = (COLORREF) GetSysColor (COLOR_3DFACE);//3DFACE); 
		SendMessage(hwnd, EM_SETBKGNDCOLOR, FALSE, (LPARAM) crCurrent);
		SendMessage(hwnd, WM_SETREDRAW, TRUE, NULL);
		InvalidateRect(hwnd, &rect, TRUE);
		retcode = CallWindowProc(wpOrigEditProc, hwnd, uMsg, wParam, lParam);
		return retcode;
		break;
	default:
		return CallWindowProc(wpOrigEditProc, hwnd, uMsg, wParam, lParam);
	}
	return retcode;
}

BOOL EULADlg::Init(LPARAM lParam)
{
	RECT  rectEdit;
	POINT ptEdit;
	//
	//Get coords of existing template edit box and use them for our RTF control...
	//
	GetWindowRect(GetDlgItem(hDlg, EULA_EDIT), &rectEdit);
	ptEdit.x = rectEdit.left;
	ptEdit.y = rectEdit.top;
	ScreenToClient(hDlg, &ptEdit);
	rectEdit.left = ptEdit.x;
	rectEdit.top = ptEdit.y;
	ptEdit.x = rectEdit.right;
	ptEdit.y = rectEdit.bottom;
	ScreenToClient(hDlg, &ptEdit);
	rectEdit.right = ptEdit.x;
	rectEdit.bottom = ptEdit.y;


	//
	//Create the EULA Edit Box
	//
	hWndRichEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
		"RichEdit",
		"",
		WS_CHILD | ES_MULTILINE | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | ES_NOIME | WS_VSCROLL |  ES_SUNKEN | ES_SAVESEL | WS_TABSTOP,
		rectEdit.left, rectEdit.top,
		rectEdit.right - rectEdit.left + 1, rectEdit.bottom - rectEdit.top + 1,
		hDlg,
		(HMENU) 11111,
		g_hInst,
		NULL);
	BOOL fRc = InitEULARTF(hWndRichEdit, STR_EULA);

	HWND rejectButton = GetDlgItem(hDlg, IDCANCEL);
	SetFocus(rejectButton);

	if (fRc)
	{
		// Subclass the edit control
		wpOrigEditProc = (WNDPROC) SetWindowLong(hWndRichEdit, GWL_WNDPROC, (LONG) EulaEditSubclassProc);
		hwndEdit = hWndRichEdit;
		ShowWindow(hWndRichEdit, SW_SHOWNORMAL);
		UpdateWindow(hWndRichEdit);
		return FALSE;
	}
	return fRc;
}

BOOL EULADlg::Command (WORD nId, WORD nNotifyCode, LPARAM lParam)
{
	if(nId == ID_PRINT)
	{
		PrintTheContents(hWndRichEdit);
	}

	return TRUE;
}

BOOL EULADlg::InitEULARTF(HWND hWndRichEdit, UINT nEULAPathResID)
{
	HANDLE	   hFile;
	EDITSTREAM eStream;
	TCHAR	   szEULAPath[MAX_PATH + 1];
	int		   nCharsRead = 0;
	int		   nEULAType = SF_TEXT;
	
	//
	//Load EULA Pathname string...
	//
	LoadString(g_hInst, 
		nEULAPathResID, 
		szEULAPath,
		sizeof(szEULAPath));
	
	//
	//Try to open the EULA file...
	//
	hFile = CreateFile(szEULAPath, 
						  GENERIC_READ, 
						  FILE_SHARE_READ, 
						  NULL, 
						  OPEN_EXISTING, 
						  FILE_ATTRIBUTE_NORMAL, 
						  NULL);
	
	//
	//If we had a problem reading the EULA file off of the setup CD-ROM,
	//use a default EULA string which basically says to read the EULA.RTF
	//file on the CD-ROM and click Yes if you agree to it...
	//
	if (INVALID_HANDLE_VALUE == hFile)
	{
		LoadString(g_hInst, STR_EULANOTFOUND, szEULAPath, sizeof(szEULAPath));

		SetWindowText(hWndRichEdit, szEULAPath);
	}
	else
	{
		TCHAR szHeader[sizeof(strRTFHEADER) + sizeof(TCHAR)];
		DWORD dwRead;

		if (ReadFile(hFile, szHeader, lstrlen(strRTFHEADER), &dwRead, NULL))
		{
			nEULAType = strncmp(szHeader, strRTFHEADER, lstrlen(strRTFHEADER)) ? SF_TEXT : SF_RTF;
			SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		}

		eStream.dwError = 0;
		eStream.dwCookie = (DWORD) hFile;
		eStream.pfnCallback = EditStreamCallback;
		
		//
		//Call the RTF EditStream callback to populate the RTF control with the EULA...
		//
		nCharsRead = SendMessage(hWndRichEdit, EM_STREAMIN, (WPARAM) nEULAType, (LPARAM) &eStream);
		
		CloseHandle ((HANDLE) hFile);
	}
	
	return TRUE;
}

DWORD CALLBACK EditStreamCallback(DWORD dwFile, LPBYTE pbBuffer, LONG cbToRead, LONG *pcbRead)
{
	ReadFile((HANDLE) dwFile, pbBuffer, cbToRead, (LPDWORD) pcbRead, NULL);
	
	return S_OK;
} 

void EULADlg::PrintTheContents (HWND hWndRichEdit)
{
	FORMATRANGE fr;
	DOCINFO     docInfo;
	LONG        lTextOut;
	LONG		lTextAmt;
	PRINTDLG	pd;
	int			nXOffset;
	int			nYOffset;
	int			nXPixelsPerInch;
	int			nYPixelsPerInch;
	HCURSOR		hOldCursor;
	
	//
	//Initialize the PRINTDLG structure.
	//
	ZeroMemory(&pd, sizeof(PRINTDLG));
	pd.lStructSize = sizeof (PRINTDLG);
	pd.hwndOwner = hDlg; //GetWndParent();
	pd.hInstance = (HINSTANCE) g_hInst;
	pd.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION | PD_PRINTSETUP;
	
	//
	//Get the printer DC.
	//

	if (PrintDlg (&pd) == TRUE)
	{
		fAbortPrint = FALSE;
		
		hWndPrintDialog = CreateDialog(g_hInst, "ABORTPRINT", ghWnd, (DLGPROC) AbortPrintDialogProc);

		hOldCursor = SetCursor(LoadCursor(NULL, IDC_APPSTARTING));

		ForwardMyMessages();

		//
		//Fill out the FORMATRANGE structure for the RTF output with the printer DC and an indication
		//that we want to print the whole thing...
		//
		ZeroMemory(&fr, sizeof(FORMATRANGE));
		fr.hdc = fr.hdcTarget = pd.hDC;
		fr.chrg.cpMin = 0;
		fr.chrg.cpMax = -1;
		
		//
		//Be sure that the printer DC is in text mode.
		//
		SetMapMode (pd.hDC, MM_TEXT);
		
		//
		//Used to convert pixels to twips...
		//
		nXPixelsPerInch = GetDeviceCaps(pd.hDC, LOGPIXELSX);
		nYPixelsPerInch = GetDeviceCaps(pd.hDC, LOGPIXELSY);
		
		//
		//Start print at printer's left and top capability
		//
		nXOffset = (int) (GetDeviceCaps(pd.hDC, PHYSICALOFFSETX) / (float) nXPixelsPerInch * nTWIPSPERINCH);
		fr.rc.left = fr.rcPage.left = (nXOffset + nTWIPSPERINCH / 2); //Plus 1/2 inch
		nYOffset = (int) (GetDeviceCaps(pd.hDC, PHYSICALOFFSETY) / (float) nYPixelsPerInch * nTWIPSPERINCH);
		fr.rcPage.top = fr.rc.top = nYOffset + nTWIPSPERINCH / 2; //Plus 1/2 inch
		
		//
		//Set right and bottom
		//
		fr.rc.right = fr.rcPage.right = (int) (GetDeviceCaps (pd.hDC, HORZRES) / (float) nXPixelsPerInch * nTWIPSPERINCH) - nXOffset;
		fr.rc.bottom = fr.rcPage.bottom = (int) (GetDeviceCaps (pd.hDC, VERTRES) / (float) nYPixelsPerInch * nTWIPSPERINCH) - nYOffset;
		
		//
		//Fill out the DOCINFO structure.
		//
		ZeroMemory(&docInfo, sizeof(DOCINFO));
		docInfo.cbSize = sizeof (DOCINFO);
		docInfo.lpszDocName = g_szAppTitle;
		docInfo.lpszOutput = NULL;
		
		//
		//Start the print job as a whole and prepare the printer to accept page data
		//
		StartDoc (pd.hDC, &docInfo);
		StartPage (pd.hDC);
		
		lTextOut = 0;
		lTextAmt = SendMessage(hWndRichEdit, WM_GETTEXTLENGTH, 0, 0);
		
		while (lTextOut < lTextAmt && FALSE == fAbortPrint)
		{
			ForwardMyMessages();

			//
			//Cause a page to be rendered to the printer...
			//
			lTextOut = SendMessage(hWndRichEdit, EM_FORMATRANGE, FALSE, (LPARAM) &fr);
			SendMessage(hWndRichEdit, EM_DISPLAYBAND, 0, (LPARAM) &fr.rc);
			
			if (lTextOut < lTextAmt)
			{
				//
				//End the current page
				//
				EndPage (pd.hDC);
				
				//
				//Start the next page
				//
				StartPage (pd.hDC);
				
				//
				//This pass we'll start at the character at offset lTextOut...
				//
				fr.chrg.cpMin = lTextOut;
			}

			ForwardMyMessages();
		}
		
		//
		//Done printing, reset the formatting of the rich edit control...
		//
		SendMessage(hWndRichEdit, EM_FORMATRANGE, TRUE, (LPARAM) NULL);
		
		//
		//Finish the document.
		//
		EndPage (pd.hDC);
		EndDoc (pd.hDC);

		//
		//Delete the printer DC.
		//
		DeleteDC (pd.hDC);

		if (FALSE == fAbortPrint)
		{
			DestroyWindow(hWndPrintDialog);
		}

		SetCursor(hOldCursor);
	}
} 

DLGPROC AbortPrintDialogProc(HWND hPrintDialog, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TCHAR sz[512];
	wsprintf(sz, "Message: %lu\n", uMsg);
	OutputDebugString(sz);

	switch(uMsg)
	{
	case WM_INITDIALOG:
		SetWindowText(hPrintDialog, g_szAppTitle);
		
//		if (IsDBCS())
//		{ 
//			changeIMEStatus(hPrintDialog, SCF_IME_DISABLE);
//		}
		
		return (DLGPROC) TRUE;
		
	case WM_COMMAND:
		if (IDCANCEL == LOWORD(wParam))
		{
			DestroyWindow(hPrintDialog);
			fAbortPrint = TRUE;
			
			return (DLGPROC) TRUE;
		}
	}
	
	return (DLGPROC) FALSE;
}

/****************************************************************************
	FUNCTION: DisplayNextBillboard(HWND, UINT, UINT, DWORD)

	PURPOSE: changes the current billboard

	COMMENTS:

	This function sets the id of the current billboard so that it will be drawn
	on the next WM_PAINT message.  The setup wave file is played the first time
	this function is called.
****************************************************************************/
void CALLBACK DisplayNextBillboard(HWND hwnd, UINT iMsg, UINT iTimerID, DWORD dwTime)
{
	static int nextBillboard = 0;
	if (0 == nextBillboard)
	{	// only play the sound the first time this function is called
		EBUPlaySound(SETUP_SOUND_FILE, NULL, SND_FILENAME | SND_ASYNC);
	}
	if (USE_BILLBOARDS)
	{	
		if ((nextBillboard < NUM_BILLBOARDS) || (CYCLE_BILLBOARDS))
		{
			nextBillboard %= NUM_BILLBOARDS;
			gCurrentBitmapID = BILLBOARD_LIST[nextBillboard];
			nextBillboard++;
		}
		SendMessage(GetWndParent(),WM_PAINT,0,0);
	}
}

