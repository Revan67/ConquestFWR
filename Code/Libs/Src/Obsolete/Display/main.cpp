
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//#define BUILD_DISPLAY
#include "static\display.h"

#define DISPLAY_W 640
#define DISPLAY_H 480

IDisplay *DISPLAY = 0;

//---------------------------------------------------------------------------
// WndProc
//---------------------------------------------------------------------------

static long FAR PASCAL MainWindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch( message )
    {
/*
#if USE_SAL
	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN)
		{
			TheApp.fullscreen ^= 1;
			if (TheApp.fullscreen)
			{
				WIN->ShowWindow(640, 480, WMF_FULL_SCREEN);
				GL->SetDisplayMode(TheApp.hDC, 640, 480, 16);
			}
			else
			{
				GL->RestoreDisplayMode(TheApp.hDC);
				WIN->ShowWindow(640, 480, 0);
			}
			return 0;
		}
		break;
#endif

	case WM_KEYDOWN:
		OnKeyDown(int(wParam));
		break;
	case WM_KEYUP:
		OnKeyUp(int(wParam));
		break;
*/
    case WM_ACTIVATEAPP:
//		Active = (int) wParam;
        break;

    case WM_CREATE:
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

    case WM_CLOSE:
		PostQuitMessage(0);	   // always quit with window valid
//		Quit = true;
		return 0;

    default:
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

//---------------------------------------------------------------------------

HWND CreateMainWindow (HINSTANCE hInstance, int display_width, int display_height, int nCmdShow=0)
{
	HWND wnd = 0;

	bool result;

	WNDCLASS    wc;
	BOOL        rc;

	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = MainWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = 0; //LoadIcon(hInstance, MAKEINTRESOURCE (IDI_GAMEAPP));
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName =  NULL;
	wc.lpszClassName = "GameClass";
	rc = RegisterClass( &wc );

	if (rc)
	{
		RECT rect;

		rect.top = rect.left = 0;
		rect.right = display_width - 1;
		rect.bottom = display_height - 1;

		DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
		AdjustWindowRectEx(&rect, style, FALSE, 0);

		wnd = CreateWindowEx(0,
			"GameClass",
			"DISPLAY testbed",
			style, 
			0,
			0,
			rect.right - rect.left + 1,
			rect.bottom - rect.top + 1,
			NULL,
			NULL,
			hInstance,
			NULL );

		if (wnd)
		{
			InvalidateRect(wnd, NULL, FALSE);
			UpdateWindow(wnd);
			result = true;
		}
		else
		{
			result = false;
		}
	}
	else
	{
		result = false;
	}

	if (!result)
		wnd = 0;

	return wnd;
}

//---------------------------------------------------------------------------
// App
//---------------------------------------------------------------------------

struct App
{
	HINSTANCE hApp;	// hInstance

	HWND hWnd;
	HDC hDC;

	int hglrc;

	App (void)
	{
		hApp = 0;
		hWnd = 0;
		hDC = 0;
		hglrc = 0;
	}

	void close (void)
	{
	}

	bool open (HINSTANCE i)
	{
		hApp = i;

	// SYSTEM

		DISPLAY = CreateDisplay();

		if (!DISPLAY)
			return false;

	// WINDOW

		hWnd = CreateMainWindow(hApp,DISPLAY_W,DISPLAY_H);

		if (hWnd == 0)
			return false;

		hDC = GetDC(hWnd);

		return true;
	}

	void test (void)
	{
		glVertex3f(1,2,3);

		glVertex3f(1,2,3);
		glVertex4f(1,2,3,0);

		glNormal3f(0,1,0);

		DISPLAY->SetDisplayMode(hDC,640,480,16);
		DISPLAY->RestoreDisplayMode(hDC);
	}

	bool startup (void)
	{
		hglrc = wCreateContext((int)hDC);

		if (!hglrc)
		{
			OutputDebugString("ERROR: wCreateContext\n");
			return false;
		}

		if (!wMakeCurrent((int)hDC,hglrc))
		{
			OutputDebugString("ERROR: wMakeCurrent\n");
			return false;
		}

		return true;
	}

	void shutdown (void)
	{
		wMakeCurrent(0,0);
		wDeleteContext(hglrc);
	}
};

//---------------------------------------------------------------------------
// WinMain
//---------------------------------------------------------------------------

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	App app;

	if (app.open(hInstance))
	{
		if (DISPLAY->LoadLibrary("c:\\windows\\system\\opengl.dll"))
		{
			if (app.startup())
			{
				app.test();
				app.shutdown();
			}
		}

		if (DISPLAY->LoadLibrary("exe\\DisplaySW.dll"))
		{
			if (app.startup())
			{
				app.test();
				app.shutdown();
			}
		}
	}

	return 0;
}

//---------------------------------------------------------------------------
