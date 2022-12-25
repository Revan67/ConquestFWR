//
// UniTool.cpp - The UNIversal TOOL framework main source file
//

//
// Design Notes:
//    This is my attempt at creating a tool which can be extended in a variety of ways.
// The two major extension mechanisms are DACOM plug-in objects and scripting in LUA.
// Complex operations (i.e. ones that take a lot of effort to write) are expected to be written as
// DACOM objects and plugged into this framework.  Examples include a bitmap editor, an object viewer, 
// etc. These objects expose interfaces which are, in turn, exposed to LUA by the framework. LUA scripts
// provide the "glue logic" between the framework objects, and the framework (this program) oversees the
// entire operation.
//    When completed, the idea is that a new application can be quickly created by plugging the
// appropriate objects into the framework, then scripting the new behavior. This could all be done without
// compiling any code, and it may be possible to put together tools in a matter of hours instead of
// days or weeks.
//


//
// Include files
//

#include <windows.h>
#include <assert.h>
#include <stdio.h>

#include <timer.h>

#include "resource.h"

#include "stdwidget.h"
#include "script.h"
#include "dastuff.h"

//
// Constants
//

//
// Class and structure definitions
//

//
// Global variables
// NOTE: Those exposed to LUA are marked as such
//

HINSTANCE hToolInstance = NULL;
HWND      hWndMain = NULL;
HMENU     hMainMenu = NULL;
HMENU     hFileMenu = NULL;
BOOL      bIsActive = FALSE;
DWORD     dwLastTickCount = 0;
DWORD     dwFrameTime = 0;
DWORD     dwLastFrameTime = 0;
SINGLE    frameRate = 0.0f;

//
// Routines
//

static long FAR PASCAL MainWndproc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch( message )
    {
    case WM_ACTIVATEAPP:
        bIsActive = (BOOL) wParam;
        if( bIsActive )
        {
            dwLastTickCount = GetTickCount();
			// We are being activated
        }
		else
		{
			// We are being deactivated
		}
        break;

	case WM_COMMAND:
		{
			WORD wNotifyCode = HIWORD(wParam); // notification code 
			WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier 
			HWND hwndCtl = (HWND) lParam;      // handle of control

			if (wNotifyCode == 0)
			{
				// This is a menu command. If it is the exit command,
				// destroy the main window, killing the app.
				if (wID == ID_FILE_EXIT)
				{
					DestroyWindow(hWndMain);
					return 0;
				}
				
				// If this is a script defined menu, execute its attached code, if any
				if (wID >= MENU_BASE_ID)
				{
					MENUITEMINFO mi;
					memset (&mi, 0, sizeof(mi));
					mi.cbSize = sizeof(mi);
					mi.fMask = MIIM_DATA;
					if (GetMenuItemInfo (GetMenu(hWnd), wID, FALSE, &mi))
					{
						lua_beginblock();
						// Get the action object associated with this menu.
						// We check to see if it is a function because it might be the nil
						// object, which indicates that the menu has no function.s
						lua_Object action = lua_getref(mi.dwItemData);
						bool done = false;
						if (lua_isfunction(action))
						{
							lua_callfunction(action);
							done = true;
						}
						lua_endblock();

						if (done)
						{
							return 0;
						}
					}
				}
			}
			else
			{
				// Not a menu command.
			}
		}
		break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

    default:
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static BOOL initApplication( HINSTANCE hInstance )
{
    WNDCLASS    wc;
    BOOL        rc;

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = MainWndproc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE (IDI_UNITOOL));
    wc.hCursor = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = (HBRUSH) GetStockObject( BLACK_BRUSH );
    wc.lpszMenuName =  MAKEINTRESOURCE(IDR_MAINMENU);
    wc.lpszClassName = "UniToolAppClass";
    rc = RegisterClass( &wc );
    if( !rc )
    {
        return FALSE;
    }

	RECT rect;

	rect.top = rect.left = 0;
	rect.right = 640;
	rect.bottom = 480;

	AdjustWindowRectEx 
		(
			&rect,
	        WS_VISIBLE | // so we don't have to call ShowWindow
		    WS_OVERLAPPEDWINDOW |   // non-app window
			WS_SYSMENU |  // so we get an icon in the tray
			WS_CAPTION |
			WS_MINIMIZEBOX,
			FALSE,
			0
		);

    hWndMain = CreateWindowEx
	(
		WS_EX_ACCEPTFILES | WS_EX_APPWINDOW | WS_EX_CONTROLPARENT, 
        "UniToolAppClass",
        "Universal Tool Framework",
        WS_VISIBLE | // so we don't have to call ShowWindow
        WS_OVERLAPPEDWINDOW |   // non-app window
        WS_SYSMENU |  // so we get an icon in the tray
		WS_CAPTION |
		WS_MINIMIZEBOX,
        0,
        0,
        rect.right - rect.left + 1,
		rect.bottom - rect.top + 1,
        NULL,
        NULL,
        hInstance,
        NULL );

    if( !hWndMain )
    {
        return FALSE;
    }

	hMainMenu = GetMenu(hWndMain);
	hFileMenu = GetSubMenu(hMainMenu, 0);

    InvalidateRect (hWndMain, NULL, FALSE);
	UpdateWindow (hWndMain);

    return TRUE;
}

//
// Utility functions
//

typedef char **ARGV_TYPE;
const int CMDBUF_LEN = 4096;
const int ARGC_MAX = 256;

static void parse_command_line (const char *cmdLine, int *argcPtr, ARGV_TYPE *argvPtr)
{
	assert (cmdLine != NULL);
	assert (argcPtr != NULL);
	assert (argvPtr != NULL);
	assert (strlen(cmdLine) < CMDBUF_LEN);

	// Make a copy of the command line into a static buffer.
	static char buffer[CMDBUF_LEN];
	static char *argv[ARGC_MAX];

	strcpy (buffer, cmdLine);

	// Parse the individual components out of the command line.

	char *here = buffer;
	int argc = 0;
	int state = 1;
	bool done = false;
	while (!done)
	{
		char c = *here;
		switch (state)
		{
		case 0:  // accumulating characters into current entry.
			if (c == ' ' || c == '\0')
			{
				// Terminate the current entry.
				*here = '\0';
				++argc;

				// Either exit the loop or enter whitespace skipping mode.
				if (c == '\0')
				{
					done = true;
				}
				else
				{
					// Go to whitespace skipping mode.
					state = 1;
				}
			}
			break;

		case 1:  // skipping whitespace
			if (c == '\0')
			{
				// Exit the loop without incrementing the argument count.
				done = true;	
			}
			if (c != ' ')
			{
				argv[argc] = here; // initial entry
				state = 0;
			}
			break;

		default:
			assert (false && "Messed up state in command line parser.");
			break;
		}
		++here;
	}

	*argcPtr = argc;
	*argvPtr = (ARGV_TYPE) argv;
}

//
// Main entry routine
//

int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCommandLine, int cmdShow )
{
    MSG     msg;
	WPARAM  result = 0;

	hToolInstance = hInstance;

	// *** This will probably go away in favor of having the initial LUA script force the
	// *** creation of a top-level window.
    if (!initApplication(hInstance))
    {
        return false;
    }

	// Initialize the common controls
//	InitCommonControls ();

	// Parse the command line
	int argc;
	char **argv;
	parse_command_line (lpszCommandLine, &argc, &argv);

	char *startupFilename = "startup.lua";

	for (int i = 0; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
			case 'd':
			case 'D':
				// Create a console for this program and make sure that the C runtime is properly
				// bound to the standard handles.
				AllocConsole ();
				freopen ("CONIN$", "r", stdin);
				freopen ("CONOUT$", "w", stdout);
				freopen ("CONOUT$", "w", stderr);
				SetConsoleTitle ("UniTool Console");
				break;
			}
		}
		else
		{
			// Anything else is a startup filename.
			startupFilename = argv[i];
		}
	}

	// Initialize the scripting
	if (!init_scripting ())
	{
		return 0;
	}

	// Initialize the da stuff
	if (!dastuff_open ("unitool.ini"))
	{
		printf ("Failed to open DA library.\n");
	}

	// Initialize the standard widgets.
	init_standard_widgets ();

	// Initialize the video widget.
	extern void init_video_widget (void);
	init_video_widget();

	// Create a main window, exporting both it and its menu as globals.
	if (hMainMenu != NULL)
	{
		lua_pushusertag ((void *) hMainMenu, MENU_TAG);
		lua_setglobal ("MainMenu");
	}
	if (hFileMenu != NULL)
	{
		lua_pushusertag ((void *) hFileMenu, MENU_TAG);
		lua_setglobal ("FileMenu");
	}
	if (hWndMain != NULL)
	{
		lua_pushusertag ((void *) hWndMain, WINDOW_TAG);
		lua_setglobal ("MainWindow");
	}

	// Run the initialization script.
	lua_dofile (startupFilename);

	// Initialize the frame timer.
    dwFrameTime = timeGetTime();
	dwLastFrameTime = dwFrameTime;
	SINGLE frameTime = (SINGLE) dwFrameTime / 1000.0f;

	// Perform the main loop.
	Timer tmFrame;
	bool done = false;
	const int frameReportRate = 10;
	int frameCounter = frameReportRate;
    while (!done)
    {
		tmFrame.begin ();
        while ( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
        {
            if( !GetMessage( &msg, NULL, 0, 0 ) )
            {
				result = msg.wParam;
				done = true;
				break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!done)
		{
			// Perform per-frame operations.
			dwLastFrameTime = dwFrameTime;
			dwFrameTime = timeGetTime ();
	
            if (!dastuff_update())
			{
				// The destroy window will cause the GetMessage above to return FALSE, which will
				// then break out of the loop.
				DestroyWindow (hWndMain);
			}

        }

		tmFrame.end();
		if (--frameCounter <= 0)
		{
			SINGLE ftime = tmFrame.accumSecs ();
			if (ftime != 0.0)
			{
				frameRate = frameReportRate / ftime;
			}
			tmFrame.reset ();
			frameCounter = frameReportRate;
		}
    }

	dastuff_close ();
    return true;
}

