//
// WinStuff.cpp
//
//
//

#include <windows.h>

//

#include "resource.h"

//

HWND hWnd = NULL;
HACCEL hKeyBindings = 0;
unsigned int AppActive = !0;
extern long WINAPI WndProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);

//

HWND CreateAppWindow( HINSTANCE hAppInstance )
{
	WNDCLASS wc;

	if (hWnd)
		return hWnd;
	
	//
	// Set up and register application window class
	//
	
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hAppInstance;
	wc.hIcon         = 0; // LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = 0; // GetStockObject(BLACK_BRUSH); 
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "ObjView";
	
	RegisterClass(&wc);
	
	//
	// Create application's main window
	//
	
	hWnd = CreateWindowEx(
		0,
		"ObjView",
		"ObjView",
		WS_CLIPCHILDREN, 
		0,
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hAppInstance,
		NULL);


	if( (hKeyBindings = LoadAccelerators( hAppInstance, MAKEINTRESOURCE(IDR_ACCELERATORS) )) == 0 ) {
		return NULL;
	}
	
	return hWnd;
}

void SetStyleInWindow (int display_size_X, int display_size_Y)
{
	//
	// Enable caption menu and user preferences
	//
	
	SetWindowLong(hWnd, 
		GWL_STYLE, 
		GetWindowLong(hWnd, GWL_STYLE) & ~WS_POPUP);
	
	SetWindowLong(hWnd, 
		GWL_STYLE, 
		GetWindowLong(hWnd, GWL_STYLE) | (WS_OVERLAPPED  | 
		WS_CAPTION     | 
		WS_SYSMENU     | 
		WS_MINIMIZEBOX));

	SetWindowLong(hWnd, 
		GWL_EXSTYLE, 
		GetWindowLong(hWnd, GWL_EXSTYLE) & ~WS_EX_TOPMOST);

	//
	// If area not already established, center window's client area on
	// desktop, and size it to correspond to the display size for optimum 
	// performance (no stretching needed)
	//

	// Calculate adjusted position of window
	//
	// Do not allow overall window size to exceed desktop size; keep 
	// dividing height and width by 2 until entire window fits
	//
	// If window is offscreen (or almost entirely offscreen), center it
	//
	
	RECT rect;

	rect.left = (GetSystemMetrics (SM_CXSCREEN) - display_size_X) / 2;
	rect.top = (GetSystemMetrics (SM_CYSCREEN) - display_size_Y) / 2;
	rect.right = rect.left + display_size_X - 1;
	rect.bottom = rect.top + display_size_Y - 1;

	AdjustWindowRectEx (&rect,
		GetWindowLong (hWnd, GWL_STYLE),
		(GetMenu (hWnd) != NULL),
		GetWindowLong (hWnd, GWL_EXSTYLE));

	//
	// Set window size and position
	//
	
	SetWindowPos (hWnd, 
		HWND_TOP, 
		rect.left,
		rect.top,
		rect.right  - rect.left + 1,
		rect.bottom - rect.top  + 1,
		SWP_NOCOPYBITS | SWP_NOZORDER);
}

void SetStyleFullScreen (void)
{
	//
	// Disable caption menu
	//

	SetWindowLong(hWnd, 
		GWL_STYLE, 
		GetWindowLong(hWnd, GWL_STYLE) | WS_POPUP);

	SetWindowLong(hWnd, 
		GWL_STYLE, 
		GetWindowLong(hWnd, GWL_STYLE) & ~(WS_OVERLAPPED  | 
		WS_CAPTION     | 
		WS_SYSMENU     | 
		WS_MINIMIZEBOX | 
		WS_MAXIMIZEBOX | 
		WS_THICKFRAME));

	//
	// Set window boundaries to cover entire desktop, and show it
	//

	SetWindowPos(hWnd, 
		HWND_TOP, 
		0,
		0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN),
		SWP_NOCOPYBITS | SWP_NOZORDER);
}

void MessagePump (void)
{
	MSG msg;

	while (!AppActive || PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage (&msg, NULL, 0, 0 ))
		{
			AppActive = false;
			exit (0);
		}
		
		if( !TranslateAccelerator( hWnd, hKeyBindings, &msg ) ) {
			TranslateMessage( &msg ); 
		}

		DispatchMessage( &msg );
	}
}

