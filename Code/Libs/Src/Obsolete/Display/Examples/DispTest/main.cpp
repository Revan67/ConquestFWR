//---------------------------------------------------------------------------
//
// MAIN.CPP = DISPLAY Testbed Program
//
//---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>		// sprintf()
#include <stdlib.h>		// exit()
#include <assert.h>		// assert()

#pragma warning(disable:4244)	// conversion double to float
#pragma warning(disable:4305)	// truncation double to float

#include "dacom.h"
#include "display.h"

#include "system.h"		// struct SYSCOMPDESC

#include "timer.h"		// struct GameTimer
#include "text.h"		// glutTextPrint()

//---------------------------------------------------------------------------
// GLOBALS
//---------------------------------------------------------------------------

ICOManager *DACOM = 0;
IDAComponent *xGL = 0;
IDisplay *GL = 0;

int DisplayWidth = 640;
int DisplayHeight = 480;

int Active = 0;
int Quit = 0;

int FrameIndex = 0;

//---------------------------------------------------------------------------
// App Handlers
//---------------------------------------------------------------------------

void OnKeyDown (int key)
{
	switch (key)
	{
	case VK_ESCAPE:
		Quit = true;
		break;
	}
}

//---------------------------------------------------------------------------
// MainWindowProc = callback for all windows messages
//---------------------------------------------------------------------------

static long FAR PASCAL MainWindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC         hdc;

    switch( message )
    {
    case WM_ACTIVATEAPP:
        Active = (int) wParam;
        break;

    case WM_CREATE:
        break;

	case WM_KEYDOWN:
		OnKeyDown(int(wParam));
		break;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );

		// If the display is in window mode, blit the buffer

        EndPaint( hWnd, &ps );
        return 1;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

    default:
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

//---------------------------------------------------------------------------
// App
//---------------------------------------------------------------------------

struct App
{
	HINSTANCE	hInstance;
	HWND		hWnd;

	GameTimer	time;

	App (void)
	{
		hInstance = 0;
		hWnd = 0;
	}

	HWND create_window (int nCmdShow=0)
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
			rect.right = DisplayWidth - 1;
			rect.bottom = DisplayHeight - 1;

			DWORD style;

			style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
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

	bool open (HINSTANCE hInst, LPSTR cmd)
	{
		DACOM = DACOM_Acquire();
		if (!DACOM)
			return false;

		DACOM->SetINIFile("DACOM.ini");

		hInstance = hInst;
		hWnd = create_window();
		if (hWnd == 0)
			return false;

		SYSCOMPDESC desc = "IDisplay";

		if (DACOM->CreateInstance(&desc, (void**)&xGL) != GR_OK)
			return false;

		if (xGL->QueryInterface("IDisplay",(void**)&GL) != GR_OK)
			return false;

		GL->SetPixelFormat(hWnd,16);
		GL->InitWindowSize(640,480);
		int mode = GLUT_RGBA|GLUT_TRIPLE;
		//mode |= GLUT_FULLSCREEN;
		GL->InitDisplayMode(mode);

		HGLRC hglrc = GL->CreateContext(hWnd);

		if (hglrc == 0)
			return false;

		GL->MakeCurrent(hWnd,hglrc);

		return true;
	}

	void close (void)
	{
	}

	int render (void);

	int main_loop (void)
	{
	    MSG msg;
		int result = 0;

		GL->Color(0,255,0);
		glutTextInit(12,16);

		FrameIndex = 0;

		time.init();

		while (!Quit)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
			{
				if (!GetMessage(&msg, NULL, 0, 0))
				{
					result = msg.wParam;
					break;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else if (Active)
			{
				time.update();

				if (!render())
				{
					DestroyWindow(hWnd);
				}

				FrameIndex++;
			}
			else
			{
				WaitMessage();
			}
		}
		return result;
	}
};

//---------------------------------------------------------------------------

	int App::render (void)
	{
		GL->ClearColor(0,0,0.25);
		GL->Clear(GL_COLOR_BUFFER_BIT);

		GL->Ortho(+1,-1);
		GL->MatrixMode(GL_MODELVIEW);
		GL->LoadIdentity();

		GL->Disable(GL_CULL_FACE);

		#define NUM 8
		static float x[NUM];
		static float y[NUM];

		if ((FrameIndex & 0xFF) == 0)
		{
			for (int i=0; i<NUM; i++)
			{
				x[i] = float(rand()) * DisplayWidth / RAND_MAX;
				y[i] = float(rand()) * DisplayHeight / RAND_MAX;
			}
		}
		else
		{
			for (int i=0; i<NUM; i++)
			{
				x[i] *= 0.98;
				y[i] *= 0.98;
			}
		}

		GL->Color(255,0,0);
		GL->Begin(GL_TRIANGLES);

			int g = rand() * 255 / RAND_MAX;
			GL->Color(g,0,0);
			GL->Vertex(x[0],y[0]); GL->Vertex(x[1],y[1]); GL->Vertex(x[2],y[2]);
			GL->Color(0,g,0);
			GL->Vertex(x[1],y[1]); GL->Vertex(x[2],y[2]); GL->Vertex(x[3],y[3]);
			GL->Color(0,g,0);
			GL->Vertex(x[2],y[2]); GL->Vertex(x[3],y[3]); GL->Vertex(x[0],y[0]);

		GL->End();

		static float fps = 0;
		#define DELAY 0.5
		static float delay = 0;
		delay -= time.time_per_frame;
		if (delay <= 0)
		{
			delay = DELAY;
			fps = time.fps;
		}

		char msg[128];
		sprintf(msg,"FPS = %d",int(fps));
		glutTextPrint(8,64,msg);

		GL->Flush();
		GL->SwapBuffers();

		return true;
	}


App TheApp;

//---------------------------------------------------------------------------
//
// WinMain = Windows main() function
//
//---------------------------------------------------------------------------

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int ok = 1;

	if (TheApp.open(hInstance,lpCmdLine))
	{
		int err = TheApp.main_loop();
		TheApp.close();
		if (err) exit(1);
	}
	else // failed to startup?
	{
		exit(1);
		ok = 0;
	}

	return ok;
}