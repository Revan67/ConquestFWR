//---------------------------------------------------------------------------
//
// MAIN.CPP = DISPLAY Testbed Program
//
//---------------------------------------------------------------------------

#define USE_SAL 1 // aka WindowManager

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>			// sprintf()
#include <stdlib.h>			// exit()
#include <assert.h>			// assert()

#pragma warning(disable:4244)	// conversion double to float
#pragma warning(disable:4305)	// truncation double to float

#include <math.h>
#define PI			3.141592654
#define DEG2RAD		(PI/180.0)

#include "dacom.h"
#include "IProfileParser.h"

#undef BUILD_DISPLAY
#include "display.h"

#include "system.h"			// struct SYSCOMPDESC

#include "timer.h"			// struct GameTimer
#include "text.h"			// glutTextPrint()

#include "bitmap.h"

#define DebugPrint OutputDebugString

#if USE_SAL
#include "gamesys.h"		// CreateGameSystem()
#include "WindowManager.h"	// IWindowManager
#endif

//---------------------------------------------------------------------------
// GLOBALS
//---------------------------------------------------------------------------

typedef void (__stdcall *LOCKBUFFEREXT) (void **, int *);
typedef void (__stdcall *UNLOCKBUFFEREXT) (void);

LOCKBUFFEREXT glLockBufferEXT = 0;
UNLOCKBUFFEREXT glUnlockBufferEXT = 0;

//---------------------------------------------------------------------------
// User
//---------------------------------------------------------------------------

struct User
{
	float x,y,z;

	int key;

	User (void)
	{
		key = 0;

		x = 0;
		y = 0;
		z = 0;
	}

	void update (void)
	{
		key = 0;
	}
};

User Player;

#define Camera Player

//---------------------------------------------------------------------------
// MISC
//---------------------------------------------------------------------------

		float frand (void)
		{
			return float(rand())/RAND_MAX;
		}

		float fsrand (void)
		{
			return frand() * 2 - 1.0;
		}

//---------------------------------------------------------------------------

struct Bounce
{
	float x,y,z;

	float dx,dy;

	float x_size;
	float y_size;

	Bounce (int _x, int _y, int _z, int h, int v)
	{
		x = _x;
		y = _y;
		z = _z;
		dx = +0.5;
		dy = +0.5;
		x_size = h;
		y_size = v;
	}

	void update (void)
	{
		x += dx;
		y += dy;

		if (x > +x_size) dx = -dx;
		if (x < -x_size) dx = -dx;
		if (y > +y_size) dy = -dy;
		if (y < -y_size) dy = -dy;
	}
};


struct VECTOR
{
	float x,y,z,w;

	void set (float _x, float _y, float _z, float _w)
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}

	int get_clip (void)
	{
		float vx = x;
		float vy = y;
		float vz = z;
		float vw = w;

		float wpos = w;
		float wneg = -w;

		int clip;

		clip = 0;
		if (wpos < x) clip |= 0x20;
		if (x < wneg) clip |= 0x10;
		if (y > wpos) clip |= 0x08;
		if (y < wneg) clip |= 0x04;
		if (z > wpos) clip |= 0x02;
		if (z < wneg) clip |= 0x01;

		int fclip = 0;

		_asm
		{
			fld wneg
			fld wpos

			xor ecx,ecx		// clip = 0
			xor ebx,ebx


			fld vx
									fcomp st(2)
									fnstsw ax
									and eax,0100h
									lea ecx, [ecx*2 + eax]
			 fxch
			fcom st(1)
			 fxch

			fnstsw ax
			and ah,1	;and eax,0100h
			mov ch,ah
			lea ecx, [ecx*2 + eax]
			fld vy
			fxch
			fcom st(1)
			fxch
			fnstsw ax
			and eax,0100h
			lea ecx, [ecx*2 + eax]
									fcomp st(2)
									fnstsw ax
									and eax,0100h
									lea ecx, [ecx*2 + eax]
			fld vz
			fxch
			fcom st(1)
			fxch
			fnstsw ax

			and eax,0100h
			lea ecx, [ecx*2 + eax]
									fcomp st(2)
									fnstsw ax
									and eax,0100h
									lea ecx, [ecx*2 + eax]
			fstp st(0)
			fstp st(0)

			shr ecx,8
			mov fclip,ecx
		}

		if (clip != fclip)
		{
			return -1;
		}

		return clip;
	}
};

	void test_math (void)
	{
		VECTOR v;

		v.set(2,5,7, 10);
		v.get_clip();

		v.set(4,5,6, 5);
		v.get_clip();

		v.set(0,-1,2, 0.5);
		v.get_clip();

		v.set(7,8,9, 1);
		v.get_clip();
	}


//---------------------------------------------------------------------------
// CONFIGURE
//---------------------------------------------------------------------------

#define DISPLAY_W		640
#define DISPLAY_H		480

#define DISPLAY_BPP		16

#define DISPLAY_MODE	GLUT_RGBA|GLUT_DOUBLE

//---------------------------------------------------------------------------
// GLOBALS
//---------------------------------------------------------------------------

ICOManager *DACOM = 0;
IDisplay *DISPLAY = 0;

#if USE_SAL
ISystemContainer *SYS = 0;
IWindowManager *WIN = 0;
#define SAL WIN
#endif

int Active = 0;
int Quit = 0;

int FrameIndex = 0;

bool ClearScreen = true;
float BackColor[] = { 0,0,0, 1 };

//---------------------------------------------------------------------------
// App Handlers
//---------------------------------------------------------------------------

#define MOVE 1

void OnKeyUp (int key)
{
	Player.key = 0;
}

void OnKeyDown (int key)
{
	switch (key)
	{
	case VK_ESCAPE:
		Quit = true;
		break;

	case VK_LEFT:
		Camera.x -= MOVE;
		break;
	case VK_RIGHT:
		Camera.x += MOVE;
		break;
	case VK_UP:
		Camera.z -= MOVE;
		break;
	case VK_DOWN:
		Camera.z += MOVE;
		break;

	default:
		Player.key = key;
		{
			char msg[128];
			sprintf(msg,"KEY = %X\n",key);
			DebugPrint(msg);
		}
		break;
	}
}


static void AppExit(void);

static long FAR PASCAL MainWindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

//---------------------------------------------------------------------------

#if 1

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

#else

/*
HWND hWnd = 0;

#define WndProc MainWindowProc

HWND CreateAppWindow (HINSTANCE hAppInstance, const char* title)
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
	wc.lpszClassName = title;
	
	RegisterClass(&wc);
	
	//
	// Create application's main window
	//
	
	hWnd = CreateWindowEx(
		0,
		title,
		title,
		WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 
		0,
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hAppInstance,
		NULL);
	
	return hWnd;
}

struct Area
{
	int x;
	int y;
	int w;
	int h;
};

void SetStyleInWindow (S32 display_size_X, S32 display_size_Y)
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

HWND CreateMainWindow (HINSTANCE hInstance, int display_width, int display_height, int nCmdShow=0)
{
	hWnd = CreateAppWindow(hInstance,"test");
	SetStyleFullScreen();
	return hWnd;
}
*/

#endif

//---------------------------------------------------------------------------

IDAComponent *CreateSysComponent (const char *interface_name)
{
	AGGDESC desc = interface_name;

	IDAComponent *ref = 0;
	IDAComponent *result = 0;

	if (DACOM->CreateInstance(&desc, (void**)&ref) == GR_OK)
	{
		IAggregateComponent *agg;
		if (ref->QueryInterface("IAggregateComponent",(void**)&agg) == GR_OK)
		{
			int ok = agg->Initialize();

			agg->Release();
			agg = 0;

			if (ok != GR_OK)
			{
				goto abort;
			}
		}
		if (ref->QueryInterface(interface_name,(void**)&result) != GR_OK)
			result = 0;
	}

abort:

	if (ref)
		ref->Release();

	return result;
}

//---------------------------------------------------------------------------
// DriverMgr
//---------------------------------------------------------------------------

struct DriverMgr
{
	char string[256];

	char name[128];
	char value[128];

	int current;

	DriverMgr (void)
	{
		current = -1;
	}

	int get_driver_string (int index, char *dst, int max)
	{
		IProfileParser *parser;

		int size = 0;

		if (DACOM->QueryInterface("IProfileParser", (void **)&parser) == GR_OK)
		{
			HANDLE hSection = parser->CreateSection("OpenGL_Drivers");
			if (hSection)
			{
				char buffer[256];
				int line=0;
				while (parser->ReadProfileLine(hSection, line, buffer, sizeof(buffer)) != 0)
				{
					if (index == line++)
					{
						size = strlen(buffer);
						if (size > max) size = max;
						strncpy(dst,buffer,size);
						break;
					}
				}
			}
			parser->Release();
		}
		dst[size] = 0;

		return size;
	}

	int get_count (void)
	{
		int count = 0;
		for (int i=0; get_driver_string(i,string,sizeof(string)) > 0; i++)
		{
			count = i+1;
		}
		return count;
	}

	int parameterize (const char *src)
	{
		const char *stop = 0;
		const char *p = strchr(src,' ');
		if (p)
		{
			stop = p;

			while (*p == ' ')
				p++;
			if (*p == '=')
			{
				p++;
				while (*p == ' ')
					p++;
			}
		}
		else
		{
			p = strchr(src,'=');
			if (p)
			{
				stop = p;

				p++;
				while (*p == ' ')
					p++;
			}
		}

		int size;
		if (stop)
			size = stop-src;
		else
		{
			size = strlen(src);
			p = src + size;
		}
		strncpy(name,src,size);
		name[size] = 0;

		int parms = 0;
		if (*p)
		{
			parms = 1;	// FUTURE: count separators
			strcpy(value,p);
		}
		return parms;
	}

	int get_parm (char *dst, int i=0)
	{
		assert(i==0);
		strcpy(dst,value);
		return strlen(dst);
	}

	int load_library (int i=0)
	{
		int loaded = 0;
		for (; get_driver_string(i,string,sizeof(string)); i++)
		{
			parameterize(string);

			loaded = DISPLAY->load_library(name);
			if (loaded > 0)
			{
				current = i;
				return loaded;
			}
		}
		current = -1;
		name[0] = 0;
		value[0] = 0;
		return 0;
	}

	int next_library (void)
	{
		return load_library(current+1);
	}
};

DriverMgr drivers;

//---------------------------------------------------------------------------
// App
//---------------------------------------------------------------------------

struct App
{
	HINSTANCE	hInstance;		// app instance
	HWND		hWnd;			// window handle
	HDC			hDC;			// device context
	HGLRC		hRC;			// render context

	GameTimer	time;

	const char *dacom_ini;

	bool fullscreen;

	App (void)
	{
		fullscreen = 0;

		hInstance = 0;
		hWnd = 0;
		hDC = 0;

		dacom_ini = "DACOM.ini";
	}

	void configure (LPSTR cmd)
	{
		char *argv[64];
		int argc = 0;

		char *c = cmd;
		while (*c)
		{
			argv[argc++] = c;
			while (*c)
			{
				if (*c == ' ')
				{
					*c = 0;
					c++;
					break;
				}
				c++;
			}
			while (*c == ' ')
				c++;
		}

		for (int a=0; a<argc; a++)
		{
			char *arg = argv[a];
			if (arg[0] == '-')
			{
				if (arg[1] == 'w')
					fullscreen = (arg[2] == '-');
				if (arg[1] == 'f')
					fullscreen = (arg[2] != '-');
			}
			else
			{
			}
		}
	}

	bool setup_format (HDC hDC)
	{
		bool gdi = false;
		bool flip = true;
		int pages = 2;

		int flags = PFD_SUPPORT_OPENGL|PFD_GENERIC_FORMAT;

		if (!fullscreen)	flags |= PFD_DRAW_TO_WINDOW;
		//else				flags |= PFD_DRAW_TO_BITMAP;

		if (pages > 1)		flags |= PFD_DOUBLEBUFFER;
		if (gdi)
		{
			flags |= PFD_SUPPORT_GDI|PFD_SWAP_COPY;
		}
		else if (flip)		flags |= PFD_SWAP_EXCHANGE;

		PIXELFORMATDESCRIPTOR pfd =
		{
		sizeof(PIXELFORMATDESCRIPTOR),	// size
		1,				// version
		flags,			// dwFlags
		PFD_TYPE_RGBA,	// color type
		16,				// prefered color depth
		0, 0, 0, 0, 0, 0, // color bits (ignored)
		0,				//  no alpha buffer 
		0,				// alpha bits (ignored)
		0,				// no accumulation buffer
		0, 0, 0, 0,		// accum bits (ignored)
		16,				// depth buffer
		0,				// no stencil buffer
		0,				// no auxiliary buffers
		PFD_MAIN_PLANE,	// main layer
		0,				// reserved
		0, 0, 0,		// no layer, visible, damage masks
		};

		int pixelFormat;

		pixelFormat = ChoosePixelFormat(hDC, &pfd);
		if (pixelFormat == 0)
		{
			return false;
		}

		if (!SetPixelFormat(hDC, pixelFormat,&pfd))
		{
			return false;
		}

		pixelFormat = GetPixelFormat(hDC);

		DescribePixelFormat(hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

		if (pfd.dwFlags & PFD_NEED_PALETTE)
		{
			return false;
		}

		return true;
	}
	
	bool open (HINSTANCE hInst, LPSTR cmd)
	{
		atexit(AppExit);

		hInstance = hInst;

		DACOM = DACOM_Acquire();
		if (!DACOM)
			return false;

		DACOM->SetINIFile(dacom_ini);

		configure(cmd);

#if USE_SAL
		SYS = CreateGameSystem(hInstance,"test",exit,0,0);

		SYS->QueryInterface("IDisplay",(void **)&DISPLAY);
		SYS->QueryInterface("IWindowManager",(void **)&WIN);

		if (DISPLAY == 0 || WIN == 0)
			return false;

		hWnd = WIN->GetWindowHandle();

		hDC = GetDC(hWnd);

		WIN->SetCallback(MainWindowProc);

		int flags = (fullscreen) ? WMF_FULL_SCREEN:0;
		WIN->ShowWindow(640,480,flags);
#else
		DISPLAY = (IDisplay *)CreateSysComponent("IDisplay");
		if (DISPLAY == 0)
			return false;

		hWnd = CreateMainWindow(hInstance,DISPLAY_W,DISPLAY_H);
		if (hWnd == 0)
			return false;

		hDC = GetDC(hWnd);
#endif

		if (fullscreen)
		{
			if (!DISPLAY->set_display_mode(hDC,640,480,16))
				return false;
		}

		if (!drivers.load_library(0))
			return false;

		return startup();
	}

	void shutdown (void)
	{
		glLockBufferEXT = 0;
		glUnlockBufferEXT = 0;

		if (hRC)
		{
			wglMakeCurrent(0,0);
			wglDeleteContext(hRC);
			hRC = 0;
		}
	}

	bool startup (void)
	{
		if (drivers.current == -1)
			return false;

		glLockBufferEXT = (LOCKBUFFEREXT)wglGetProcAddress("glLockBufferEXT");
		glUnlockBufferEXT = (UNLOCKBUFFEREXT)wglGetProcAddress("glUnlockBufferEXT");

		if (!setup_format(hDC))
			return false;

		hRC = wglCreateContext(hDC);

		if (hRC == 0)
			return false;

		wglMakeCurrent(hDC,hRC);

		glColor3ub(255,255,255);
		glutTextInit(12,16);

		return true;
	}

	void close (void)
	{
		if (DISPLAY)
		{
			shutdown();

			DISPLAY->restore_display_mode(hDC);

			DISPLAY->Release();
			DISPLAY = 0;
		}
		if (hWnd)
		{
			if (hDC)
			{
				ReleaseDC(hWnd,hDC);
				hDC = 0;
			}
			// DestroyWindow();
			hWnd = 0;
		}
#if USE_SAL
		if (WIN)
		{
			WIN->Release();
			WIN = 0;
		}
		if (SYS)
		{
			SYS->Shutdown();
			SYS->Release();
			SYS = 0;
		}
#endif
		if (DACOM)
		{
			DACOM->ShutDown();
			DACOM->Release();
			DACOM = 0;
		}
	}

	void test_pixel_format (void)
	{
		PIXELFORMATDESCRIPTOR f;
		int pix = GetPixelFormat(hDC);
		if (DescribePixelFormat(hDC,pix,sizeof(f),&f) != 0)
		{
			char msg[256];
			DebugPrint("PIXELFORMATDESCRIPTOR\n");
			sprintf(msg," mode  = %d\n",f.iPixelType);
			DebugPrint(msg);
			sprintf(msg," bpp   = %d\n",f.cColorBits);
			DebugPrint(msg);
			sprintf(msg," red   = %d,%d\n",f.cRedBits,f.cRedShift);
			DebugPrint(msg);
			sprintf(msg," green = %d,%d\n",f.cGreenBits,f.cGreenShift);
			DebugPrint(msg);
			sprintf(msg," blue  = %d,%d\n",f.cBlueBits,f.cBlueShift);
			DebugPrint(msg);
			sprintf(msg," alpha = %d,%d\n",f.cAlphaBits,f.cAlphaShift);
			DebugPrint(msg);
		}
	}

	int render (void);

	int main_loop (void)
	{
		int result = 0;

		FrameIndex = 0;

		time.init();

		test_pixel_format();

		while (!Quit)
		{
#if USE_SAL
			WIN->ServeMessageQueue();
			if (Quit)
				break;
#else
			MSG msg;

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
			else if (!Active)
			{
				WaitMessage();
			}
			else
#endif
			{
				time.update();

				if (Player.key == '\t')
				{
					int i = drivers.current;
					shutdown();

					int count = drivers.get_count();
					do
					{
						i++;
						if (i>=count)
							i = 0;
					}
					while (drivers.load_library(i) < 1);

					if (!startup())
						return 0;
				}

				if (!render())
				{
					DestroyWindow(hWnd);
				}

				Player.update();

				FrameIndex++;
			}
		}
		return result;
	}
};

//---------------------------------------------------------------------------
// VARIOUS RENDERING TESTS
//---------------------------------------------------------------------------

	#define glRED		glColor3ub(255,0,0);
	#define glGREEN		glColor3ub(0,255,0);
	#define glBLUE		glColor3ub(0,0,255);
	#define glWHITE		glColor3ub(255,255,255);
	#define glGRAY(c)	glColor3ub(c,c,c);

	#define glLockArrays(x,y)
	#define glUnlockArrays()
/*
	void gluPerspective (float fovy, float aspect, float z0, float z1)
	{
#if 1
		float fy = z0 * tan(fovy * PI/180);
		float fx = fy * aspect;

		glFrustum(-fx,fx,-fy,fy, z0,z1);
#else
		gluPerspective(fovy,aspect,z0,z1);
#endif
	}

	void __glColorTableEXT (GLenum target, GLenum ifmt, GLsizei count, GLenum fmt, GLenum type, const GLvoid *data)
	{
		if (target != GL_TEXTURE_2D ||
			ifmt != GL_RGB8 ||
			count > 256 ||
			fmt != GL_RGB ||
			type != GL_UNSIGNED_BYTE)
		{
			return;
		}

		struct RGB
		{
			GLubyte r,g,b;
		};
		const RGB *palette = (const RGB *)data;

		float table[256];
		double i2f = 1.0/255;
		int i;
		for (i=0; i<256; i++) { table[i] = palette[i].r * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_R,256,table);
		for (i=0; i<256; i++) { table[i] = palette[i].g * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_G,256,table);
		for (i=0; i<256; i++) { table[i] = palette[i].b * i2f; }
		glPixelMapfv(GL_PIXEL_MAP_I_TO_B,256,table);
	}
*/
	void OrthoView (void)
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,640,480,0, 0,10000);
		glViewport(0,0,640,480);

		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);

		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CW);

		glColor3ub(255,255,255);
	}

	void DefaultView (float zfar=1E6)
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		float fovy = 45;
		float xaspect = 4.0/3;
		gluPerspective(fovy,xaspect, 1,zfar);
		glViewport(0,0,640,480);

		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(true);
		glDepthFunc(GL_GEQUAL);

		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CW);

		glColor3ub(255,255,255);
	}

	GLuint LoadTexture (Bitmap &bmp, const char *name)
	{
		GLuint txm = 0;
		if (bmp.load(name))
		{
			glGenTextures(1,&txm);
			glBindTexture(GL_TEXTURE_2D,txm);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glColorTableEXT(GL_TEXTURE_2D,GL_RGB8,256,GL_RGB,GL_UNSIGNED_BYTE,bmp.color_table);
//			glPixelMap(GL_RGB,256,bmp.color_table);
			int w = bmp.width;
			int h = bmp.height;
			glTexImage2D(GL_TEXTURE_2D, 0, 3, w,h, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, bmp.pixels);
		}
		return txm;
	}

	GLuint ReUseTexture (Bitmap &bmp)
	{
		GLuint txm = 0;
		if (bmp.pixels)
		{
			glGenTextures(1,&txm);
			glBindTexture(GL_TEXTURE_2D,txm);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glColorTableEXT(GL_TEXTURE_2D,GL_RGB8,256,GL_RGB,GL_UNSIGNED_BYTE,bmp.color_table);
//			glColor3ubMap(GL_RGB,256,bmp.color_table);
			int w = bmp.width;
			int h = bmp.height;
			glTexImage2D(GL_TEXTURE_2D, 0, 3, w,h, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, bmp.pixels);
		}
		return txm;
	}

//---------------------------------------------------------------------------

	void cube0 (void)
	// normal Vertex commands
	{
		#define Tnw glVertex3f(-2,+2,-2)
		#define Tne glVertex3f(+2,+2,-2)
		#define Tse glVertex3f(+2,+2,+2)
		#define Tsw glVertex3f(-2,+2,+2)

		#define Bnw glVertex3f(-2,-2,-2)
		#define Bne glVertex3f(+2,-2,-2)
		#define Bse glVertex3f(+2,-2,+2)
		#define Bsw glVertex3f(-2,-2,+2)

		glBegin(GL_QUADS);
			Tnw; Tne; Tse; Tsw;
			Tsw; Tse; Bse; Bsw;
			Tse; Tne; Bne; Bse;
			Tnw; Tsw; Bsw; Bnw;
			Tne; Tnw; Bnw; Bne;
			Bsw; Bse; Bne; Bnw;
		glEnd();

		#undef Tnw
		#undef Tne
		#undef Tse
		#undef Tsw
		#undef Bnw
		#undef Bne
		#undef Bse
		#undef Bsw
	}

	void cube1 (void)
	// selective ArrayElement() commands
	{
		float vertices[8*3] = 
		{
			-2,+2,-2,	// Tnw
			+2,+2,-2,	// Tne
			+2,+2,+2,	// Tse
			-2,+2,+2,	// Tsw

			-2,-2,-2,	// Bnw
			+2,-2,-2,	// Bne
			+2,-2,+2,	// Bse
			-2,-2,+2,	// Bsw
		};

		#define Tnw glArrayElement(0)
		#define Tne glArrayElement(1)
		#define Tse glArrayElement(2)
		#define Tsw glArrayElement(3)

		#define Bnw glArrayElement(4)
		#define Bne glArrayElement(5)
		#define Bse glArrayElement(6)
		#define Bsw glArrayElement(7)

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3,GL_FLOAT,0,vertices);

		glBegin(GL_QUADS);
			Tnw; Tne; Tse; Tsw;
			Tsw; Tse; Bse; Bsw;
			Tse; Tne; Bne; Bse;
			Tnw; Tsw; Bsw; Bnw;
			Tne; Tnw; Bnw; Bne;
			Bsw; Bse; Bne; Bnw;
		glEnd();

		glDisableClientState(GL_VERTEX_ARRAY);

		#undef Tnw
		#undef Tne
		#undef Tse
		#undef Tsw
		#undef Bnw
		#undef Bne
		#undef Bse
		#undef Bsw
	}

	void cube2 (void)
	// block DrawArrays() commands
	{
		float vertices[6*4*3] = 
		{
			-2,+2,-2,	// Tnw
			+2,+2,-2,	// Tne
			+2,+2,+2,	// Tse
			-2,+2,+2,	// Tsw

			-2,+2,+2,	// Tsw
			+2,+2,+2,	// Tse
			+2,-2,+2,	// Bse
			-2,-2,+2,	// Bsw

			+2,+2,+2,	// Tse
			+2,+2,-2,	// Tne
			+2,-2,-2,	// Bne
			+2,-2,+2,	// Bse

			-2,+2,-2,	// Tnw
			-2,+2,+2,	// Tsw
			-2,-2,+2,	// Bsw
			-2,-2,-2,	// Bnw

			+2,+2,-2,	// Tne
			-2,+2,-2,	// Tnw
			-2,-2,-2,	// Bnw
			+2,-2,-2,	// Bne

			-2,-2,+2,	// Bsw
			+2,-2,+2,	// Bse
			+2,-2,-2,	// Bne
			-2,-2,-2,	// Bnw
		};

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3,GL_FLOAT,0,vertices);
		glDrawArrays(GL_QUADS, 0, 4); // solo
		glDrawArrays(GL_QUADS, 4,20); // group
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	void cube3 (void)
	// full package DrawElements() commands
	{
		unsigned char colors[8*4] = 
		{
			255,0,0, 0xFF,
			0,255,0, 0xFF,
			0,0,255, 0xFF,
			255,0,0, 0xFF,

			0,255,0, 0xFF,
			0,0,255, 0xFF,
			0,255,0, 0xFF,
			255,0,0, 0xFF,
		};

		float texcoords[8*2] = 
		{
			0,0,
			0,0,
			0,0,
			0,0,

			0,0,
			0,0,
			0,0,
			0,0,
		};

		float vertices[8*3] = 
		{
			-2,+2,-2,	// Tnw
			+2,+2,-2,	// Tne
			+2,+2,+2,	// Tse
			-2,+2,+2,	// Tsw

			-2,-2,-2,	// Bnw
			+2,-2,-2,	// Bne
			+2,-2,+2,	// Bse
			-2,-2,+2,	// Bsw
		};

		int quads[4*6] =
		{
			//0,1,2,3,	// U = Tnw; Tne; Tse; Tsw;
			//2,6,7,3,	// F = Tsw; Tse; Bse; Bsw;
			//1,5,6,2,	// R = Tse; Tne; Bne; Bse;
			4,0,3,7,	// L = Tnw; Tsw; Bsw; Bnw;
			5,1,0,4,	// B = Tne; Tnw; Bnw; Bne;
			7,6,5,4,	// D = Bsw; Bse; Bne; Bnw;
		};

		int tris[3*6] =
		{
			0,1,2,	// U = Tnw; Tne; Tse; Tsw;
			0,2,3,

			2,6,7,	// F = Tsw; Tse; Bse; Bsw;
			2,7,3,

			1,5,6,	// R = Tse; Tne; Bne; Bse;
			1,6,2,
		};

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(3,GL_FLOAT,0,vertices);
		glColorPointer(4,GL_UNSIGNED_BYTE,0,colors);
		glTexCoordPointer(2,GL_FLOAT,0,texcoords);

		glLockArrays(0,8);

//		glDrawElements(GL_QUADS,4*6,GL_UNSIGNED_INT,quads);
		glDrawElements(GL_TRIANGLES,3*6,GL_UNSIGNED_INT,tris);
		glDrawElements(GL_QUADS,4*3,GL_UNSIGNED_INT,quads);

		glUnlockArrays();

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	void cube4 (void)
	// normal Vertex commands
	{
		int list = 0xD0D0;
		if (!glIsList(list))
		{
			glNewList(list,GL_COMPILE);
			cube0();
			glEndList();
		}
		glCallList(list);
	}

//---------------------------------------------------------------------------

	void test_fov (void)
	{
		DefaultView();

		glColor3ub(255,0,0);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45,4.0/3, 1,1E6);
		glViewport(0,0,640,480);
		glBegin(GL_QUADS);
			glVertex3f(-15,+15,-20);
			glVertex3f(+15,+15,-20);
			glVertex3f(+15,-15,-20);
			glVertex3f(-15,-15,-20);
		glEnd();

		glColor3ub(0,255,0);

		glLoadIdentity();
		gluPerspective(45,6.0/5, 1,1E6);
		glViewport(0,0,640,480);
		glBegin(GL_QUADS);
			glVertex3f(-11,+11,-20);
			glVertex3f(+11,+11,-20);
			glVertex3f(+11,-11,-20);
			glVertex3f(-11,-11,-20);
		glEnd();
	}

//---------------------------------------------------------------------------

	void test_cube (void)
	{
		DefaultView();

		glMatrixMode(GL_MODELVIEW);
		static float y = 0; y+=0.5;
		static float p = 0; //p+=1;
		glLoadIdentity();
		glTranslatef(0,0,-8);
		glRotatef(y, 0,1,0);
		glRotatef(p, 1,0,0);

		glViewport(0,0,640,480);
		glColor3ub(255,255,255); cube3();

		glViewport(384,0,256,192);
		glColor3ub(0,0,255); cube0();

		glViewport(0,288,256,192);
		glColor3ub(0,255,0); cube1();

		glViewport(384,288,256,192);
		glColor3ub(255,0,0); cube2();

		glViewport(0,0,256,192);
		glColor3ub(180,180,180); cube4();

		glViewport(0,0,640,480);
	}

//---------------------------------------------------------------------------

	void test_array (void)
	{
		DefaultView();

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		#define VCOUNT 8
		float vlist[VCOUNT*3] =
		{
			100,100,0,
			150,100,0,
			150,150,0,
			100,150,0,

			300,200,0,
			350,200,0,
			350,250,0,
			300,250,0,
		};

		unsigned char clist[VCOUNT*3] =
		{
			0xFF,0xFF,0x00,
			0xFF,0x00,0x00,
			0x00,0xFF,0x00,
			0x00,0x00,0xFF,

			0x00,0xFF,0xFF,
			0xFF,0x00,0xFF,
			0x00,0xFF,0x00,
			0xFF,0xFF,0xFF,
		};

		glVertexPointer(3, GL_FLOAT, 0, vlist);
		glColorPointer(3, GL_UNSIGNED_BYTE, 0, clist);

		#define FCOUNT 4
		int flist[FCOUNT*3] =
		{
			0,1,2,
			2,3,0,

			4,5,6,
			7,4,6,
		};

		glDrawElements(GL_TRIANGLES,3*FCOUNT, GL_INT, flist);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}

//---------------------------------------------------------------------------

	void test_nearclip (void)
	{
		DefaultView();

		static float zz = -0.5;
		static float dz = 0.08;
		zz += dz;
		if (zz > +5 || zz < -5)
			dz = -dz;

		glBegin(GL_TRIANGLES);

			glColor3ub(255,255,255);
			glVertex3f(- 5, 5,   zz-30);
			glVertex3f(+0.1,0.1, zz);
			glVertex3f(+ 5,-5,   zz-30);

		glEnd();
	}

//---------------------------------------------------------------------------

	void test_clip (void)
	{
		DefaultView();

		glDisable(GL_CULL_FACE);

		static Bounce b1(0,0,-30, 50,40);
		static Bounce b2(25,37,-30, 50,40);
		b1.update();
		b2.update();

		glBegin(GL_LINES);
			glColor3ub(0,255,0);
			glVertex3f(-100,+20, -40);
			glVertex3f(+20,  0, -40);

			glColor3ub(64,64,255);
			glVertex3f(-10,+75, -40);
			glVertex3f(+60,-45, -40);

			glVertex3f(b2.x-2,b2.y,b2.z);
			glVertex3f(b2.x,b2.y,b2.z);

		glEnd();

#if 0
	glBegin(GL_LINES);
#else
	glBegin(GL_POINTS);
#endif
			glColor3ub(255,255,255);
			glVertex3f(b1.x,b1.y,b1.z);
			glVertex3f(b2.x,b2.y,b2.z);
	glEnd();

		glEnable(GL_BLEND);

		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		glBegin(GL_TRIANGLES);
			glColor4ub(255,0,0, 255);	glVertex3f(-25,+10, -50);
			glColor4ub(255,0,0, 255);	glVertex3f(-175,  0, -50);
			glColor4ub(255,0,0,   0);	glVertex3f(-25,-10, -50);

			glColor4ub(255,0,0,  255);
			glVertex3f(-10,+25, -50);
			glVertex3f(  0,+65, -50);
			glVertex3f(+10,+25, -50);
		glEnd();

		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glBegin(GL_TRIANGLES);
			glColor4ub(255,0,0,  128);
			glVertex3f(+25,+10, -50);
			glVertex3f(+75,  0, -50);
			glVertex3f(+25,-10, -50);
		glEnd();

		glBlendFunc(GL_SRC_ALPHA,GL_ZERO);
		glBegin(GL_TRIANGLES);

			glColor4ub(255,0,0,  255);

			glVertex3f(-10,-25, -50);
			glVertex3f(  0,-65, -50);
			glVertex3f(+10,-25, -50);
		glEnd();
		glDisable(GL_BLEND);

		glBegin(GL_TRIANGLES);
#if 0
			glColor3ub(255,255,255);

			static float zz = -5;
			static float dz = 0.2;
			zz += dz;
			if (zz > +5 || zz < -10)
				dz = -dz;

			glVertex3f(- 5, 5, zz-30);
			glVertex3f(+0.1,0.1, zz);
			glVertex3f(+ 5,-5, zz-30);
#endif
/*
			glColor3ub(0,0,255);
			glVertex3f(-50,+20,-30);
			glVertex3f(-70, 50,-30);
			glVertex3f(-90,-20,-30);

			glColor3ub(0,0,255);
			glVertex3f(-40,+20,-30);
			glVertex3f(+70, 50,-30);
			glVertex3f(-10,-20,-30);

			glColor3ub(255,0,0);
			glVertex3f(-40,+90,-30);
			glVertex3f(+70,  0,-30);
			glVertex3f(-10,-20,-30);

			glColor3ub(0,255,0);
			glVertex3f(-60,+20,-30);
			glVertex3f(+70,  0,-30);
			glVertex3f(-10,-50,-30);
*/
#if 0
		static float angle = 0;
		angle += PI/128;
		float t = sin(angle);

		float depth = 70 + t*50;

			glColor3ub(255,0,0);
			glVertex3f(-100,+50,-depth);
			glVertex3f(+100,+50,-depth);
			glVertex3f(- 10,-270,-depth);
#endif
		glEnd();
	}

//---------------------------------------------------------------------------

	void test_blend (void)
	{
		DefaultView();

		glEnable(GL_BLEND);

		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		glBegin(GL_TRIANGLES);
			glColor4ub(255,0,0, 255);	glVertex3f(-25,+10, -50);
			glColor4ub(255,0,0, 255);	glVertex3f(-175,  0, -50);
			glColor4ub(255,0,0,   0);	glVertex3f(-25,-10, -50);

			glColor4ub(255,0,0,  255);
			glVertex3f(-10,+25, -50);
			glVertex3f(  0,+65, -50);
			glVertex3f(+10,+25, -50);
		glEnd();

		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glBegin(GL_TRIANGLES);
			glColor4ub(255,0,0,  128);
			glVertex3f(+25,+10, -50);
			glVertex3f(+75,  0, -50);
			glVertex3f(+25,-10, -50);
		glEnd();

		glBlendFunc(GL_SRC_ALPHA,GL_ZERO);
		glBegin(GL_TRIANGLES);

			glColor4ub(255,0,0,  255);

			glVertex3f(-10,-25, -50);
			glVertex3f(  0,-65, -50);
			glVertex3f(+10,-25, -50);
		glEnd();

		glDisable(GL_BLEND);
	}

//---------------------------------------------------------------------------

	void test_speed (void)
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		float fovy = 45;
		float xaspect = 6.0/5;
		gluPerspective(fovy,xaspect, 1,1E6);

		glViewport(0,0,640,480);

		glDisable(GL_CULL_FACE);
		glFrontFace(GL_CW);

		glBegin(GL_TRIANGLES);

		#define W 4.0
		#define H 4.0

		#define S 7

		float z = -100;
		for (int v=-S; v<S; v++)
		{
			float y = (H+1)*float(v);

			for (int h=-S; h<S; h++)
			{
				float x = (W+1)*float(h);

				glVertex3f(x  ,y  ,z);
				glVertex3f(x+W,y  ,z);
				glVertex3f(x+W,y+H,z);

				glVertex3f(x  ,y  ,z);
				glVertex3f(x+W,y+H,z);
				glVertex3f(x  ,y+H,z);
			}
		}

		glEnd();
	}

//---------------------------------------------------------------------------

	#define TURN		12
	#define speed		150
	#define RADIUS		20
	#define LENGTH		50
	#define DURATION	6
	#define WIDTH		(PI/48)
	#define LWIDTH		(PI/4)

	struct Effect
	{
		Effect *next;

		float x,y,z;
		unsigned char r,g,b;

		float age,life;

		float angle;
		float alpha,da;

		float size;
		float radius;
		float length;

		int count;

		int type;
	};

//---------------------------------------------------------------------------

	struct EffectMgr
	{
		#define MAX_EFFECTS 32

		Effect	effects[MAX_EFFECTS];

		Effect	*free;

		#define MAX_GROUPS	16
		int num_groups;
		Effect *groups[MAX_GROUPS];

		Bitmap	bitmaps[8];
		GLuint	textures[8];
		int		num_bitmaps;

		EffectMgr (void)
		{
			num_bitmaps = 0;

			for (int t=0; t<8; t++)
				textures[t] = 0;

			free = effects;
			for (int e=1; e<MAX_EFFECTS; e++)
			{
				effects[e].next = free;
				free = effects+e;
			}

			for (int i=0; i<MAX_GROUPS; i++)
				groups[i] = 0;
		}

		Effect *new_effect (void)
		{
			Effect *e = free;
			if (e)
			{
				free = e->next;
				e->next = 0;
			}
			return e;
		}

		void remove (unsigned int g, Effect *e)
		{
			assert(e);
			assert(g < MAX_GROUPS);
			Effect *prev = 0;
			Effect *curr = groups[g];
			while (curr != e)
			{
				if (curr == 0)
					return; // FAILED!
				prev = curr;
				curr = curr->next;
			}

			// REMOVE FROM LIST
			if (prev == 0)
				groups[g] = e->next;
			else
				prev->next = e->next;

			// ADD TO FREE LIST
			e->next = free;
			free = e;
		}

		void add (unsigned int g, Effect *e)
		{
			if (e)
			{
				assert(g < MAX_GROUPS);
				e->next = groups[g];
				groups[g] = e;
			}
		}

	// BOLTS

		void init_bolt (Effect *e, float a)
		{
			e->z = 0;
			e->life = DURATION;
			e->angle = a;
			e->alpha = 0.5;
			e->da = 0;
		}

		void update_bolts (unsigned int g, float dt)
		{
			assert(g < MAX_GROUPS);
			Effect *head = groups[g];

			Effect *next = 0;
			for (Effect *e=head; e; e=next)
			{
				next = e->next;

				if (e->life != 0)
				{
					e->z += dt * speed;

					e->life -= dt;
					if (e->life <= 0)
					{
						remove(g,e); // FUTURE: update this one again?
					}
				}
			}
		}

		void draw_bolts (unsigned int g)
		{
			int txm = 1;

			if (textures[txm] == 0)
			{
				textures[txm] = LoadTexture(bitmaps[txm],"art\\bolt8.tga");
			}

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,textures[txm]);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE);

			for (Effect *e = groups[g]; e; e=e->next)
			{
				float a = e->angle;

				float z = e->z;

				float x0,y0;
				x0 = RADIUS * sin(a);
				y0 = RADIUS * cos(a);
				float x1,y1;
				x1 = RADIUS * sin(a+WIDTH);
				y1 = RADIUS * cos(a+WIDTH);

				glColor4ub(255,255,255, int(e->alpha*255));

				glBegin(GL_QUADS);
					glTexCoord2f(0,0);	glVertex3f(x1,y1,-z-LENGTH);
					glTexCoord2f(1,0);	glVertex3f(x0,y0,-z-LENGTH);
					glTexCoord2f(1,1);	glVertex3f(x0,y0,-z);
					glTexCoord2f(0,1);	glVertex3f(x1,y1,-z);
				glEnd();
			}

			glDisable(GL_BLEND);
		}

	// LASERS

		void init_laser (Effect *e)
		{
			e->life = DURATION;
			e->angle = PI;
			e->alpha = 0.75;
			e->da = 0;
			e->z = 20;
		}

		void update_lasers (unsigned int g, float dt)
		{
			assert(g < MAX_GROUPS);
			Effect *head = groups[g];

			Effect *next = 0;
			for (Effect *e=head; e; e=next)
			{
				next = e->next;

				if (e->life != 0)
				{
					e->life -= dt;
					if (e->life <= 0)
					{
						remove(g,e); // FUTURE: update this one again?
					}
				}
			}
		}

		void draw_lasers (unsigned int g)
		{
			int txm = 2;

			if (textures[txm] == 0)
			{
				textures[txm] = LoadTexture(bitmaps[txm],"art\\laser8.tga");
			}

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,textures[txm]);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE);

			for (Effect *e = groups[g]; e; e=e->next)
			{
				float a = e->angle;

				float z = e->z;

				float x0,y0;
				x0 = RADIUS * sin(a);
				y0 = RADIUS * cos(a);
				float x1,y1;
				x1 = RADIUS * sin(a+LWIDTH);
				y1 = RADIUS * cos(a+LWIDTH);

				glColor4ub(255,255,255, int(e->alpha*255));

				float length = 300;

				glBegin(GL_QUADS);
					glTexCoord2f(0,0);	glVertex3f(x1,y1,-z-length);
					glTexCoord2f(1,0);	glVertex3f(x0,y0,-z-length);
					glTexCoord2f(1,1);	glVertex3f(x0,y0,-z);
					glTexCoord2f(0,1);	glVertex3f(x1,y1,-z);
				glEnd();
			}

			glDisable(GL_BLEND);
		}

	// WAVES

		void init_wave (Effect *e)
		{
			e->age = 0;
			e->life = 5;
			e->size = 10;
			e->alpha = 1.0;
			e->da = 1.0 / e->life;
			e->z = 200;
		}

		void update_waves (unsigned int g, float dt)
		{
			assert(g < MAX_GROUPS);
			Effect *head = groups[g];

			Effect *next = 0;
			for (Effect *e=head; e; e=next)
			{
				next = e->next;

				e->alpha -= e->da*dt;

				e->age += dt;
				e->size = 500*sin((PI/2)*e->age/e->life);
				//life[i] -= dt;
				if (e->age >= e->life)
				{
					remove(g,e);
				}
			}
		}

		void draw_waves (unsigned int g)
		{
			int txm = 0;

			if (textures[txm] == 0)
			{
				textures[txm] = LoadTexture(bitmaps[txm],"art\\wave8.tga");
			}

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,textures[txm]);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE);

			for (Effect *e = groups[g]; e; e=e->next)
			{
				float x,y,z;
				x = y = e->size / 2;
				z = e->z;

				unsigned int a = e->alpha*255;
				glColor4ub(255,255,255, a);

				glBegin(GL_QUADS);
					glTexCoord2f(0,0);	glVertex3f(-x,+y,-z);
					glTexCoord2f(1,0);	glVertex3f(+x,+y,-z);
					glTexCoord2f(1,1);	glVertex3f(+x,-y,-z);
					glTexCoord2f(0,1);	glVertex3f(-x,-y,-z);
				glEnd();
			}

			glDisable(GL_BLEND);
		}

	// THRUST

		void init_thrust (Effect *e)
		{
			e->life = 20;
			e->alpha = 0.5;
			e->angle = 0;
			e->x = 0; e->y = 0; e->z = -25;
			e->r = e->g = e->b = 255;
			e->radius = 4;
			e->size = 0;
			e->length = 30;
			e->count = 7;
		}

		void update_thrust (unsigned int g, float dt)
		{
			assert(g < MAX_GROUPS);
			Effect *head = groups[g];

			Effect *next = 0;
			for (Effect *e=head; e; e=next)
			{
				next = e->next;

				e->angle += (5*PI)*dt;
				e->size = e->length*(1-0.1*sin(e->angle));

				e->life -= dt;
				if (e->life <= 0)
				{
					remove(g,e);
				}
			}
		}

		void draw_thrust (unsigned int g)
		{
			Effect *head = groups[g];
			if (!head)
				return;

			int txm = 3;

			if (textures[txm] == 0)
			{
				textures[txm] = LoadTexture(bitmaps[txm],"art\\thrust8.tga");
			}

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,textures[txm]);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE);

			glDisable(GL_CULL_FACE);

#if 1
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glBegin(GL_QUADS);
			glTexCoord2f(0,0); glVertex3f(-20,+20,-20);
			glTexCoord2f(1,0); glVertex3f(-15,+20,-20);
			glTexCoord2f(1,1); glVertex3f(-15,+10,-20);
			glTexCoord2f(0,1); glVertex3f(-20,+10,-20);
		glEnd();
#endif

		static float yaw = 0;
		yaw += PI/6;

			for (Effect *e = head; e; e=e->next)
			{
				#define PIECES 5

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(e->x-Camera.x,e->y-Camera.y,e->z-Camera.z);
		glRotatef(yaw, 0,1,0);

				unsigned int a = e->alpha*255;
				glColor4ub(e->r,e->g,e->b, a);

	static float aa = 0;
	//aa += 39*PI/64;
	float sa = 2*PI/e->count;

				glBegin(GL_TRIANGLES);
				for (int i=0; i<e->count; i++)
				{
					float r = e->radius/7;
					float x0 = r*sin( 2*PI*frand() );
					float y0 = r*sin( 2*PI*frand() );
					glTexCoord2f(0.5,0); glVertex3f(x0,y0,-e->size);
					float a1 = aa+i*sa;
					float x1 = e->radius*sin(a1);
					float y1 = e->radius*cos(a1);
					float a2 = a1+sa;
					float x2 = e->radius*sin(a2);
					float y2 = e->radius*cos(a2);
					glTexCoord2f(0.8,1); glVertex3f(x1,y1,0);
					glTexCoord2f(0.2,1); glVertex3f(x2,y2,0);
				}
				glEnd();
			}

			glDisable(GL_BLEND);
		}
	};

//---------------------------------------------------------------------------

/*
	struct EffectHandler
	{
		virtual void update (Effect *head, float dt) = 0;
		virtual void draw (Effect *head) = 0;
	}

	struct WaveHandler : EffectHandler
	{
		void update (Effect &*head, Effect &*free, float dt)
		{
			Effect *prev = 0;
			Effect *next = 0;
			for (Effect *e=head; e; e=next)
			{
				next = e->next;

				e->alpha -= e->da*dt;

				e->age += dt;
				e->size = 500*sin((PI/2)*e->age/e->life);
				//life[i] -= dt;
				if (e->age >= e->life)
				{
					remove(g,e);
				}

				prev = e;
			}
		}

		void draw_waves (unsigned int g)
		{
			int txm = 0;

			if (textures[txm] == 0)
			{
				textures[txm] = LoadTexture(bitmaps[txm],"art\\wave8.tga");
			}

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,textures[txm]);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE);

			for (Effect *e = groups[g]; e; e=e->next)
			{
				float x,y,z;
				x = y = e->size / 2;
				z = e->z;

				unsigned int a = e->alpha*255;
				glColor3ub(255,255,255, a);

				glBegin(GL_QUADS);
					glTexCoord(0,0);	glVertex3f(-x,+y,-z);
					glTexCoord(1,0);	glVertex3f(+x,+y,-z);
					glTexCoord(1,1);	glVertex3f(+x,-y,-z);
					glTexCoord(0,1);	glVertex3f(-x,-y,-z);
				glEnd();
			}

			glDisable(GL_BLEND);
		}
	};
*/

	void test_fx (float dt)
	{
		DefaultView();

		glDisable(GL_CULL_FACE);
		glFrontFace(GL_CW);

		static EffectMgr FX;

		#define WAVES	0
		#define BOLTS	1
		#define LASERS	2
		#define THRUST	3

		//FX.define(WAVES,update_waves);

	// UPDATE EXPLODE

		bool boom = (Player.key == 'W'); //(FrameIndex & 0xFF) == 0;

		if (boom)
		{
			Effect *e = FX.new_effect();
			if (e)
			{
				FX.init_wave(e);
				FX.add(WAVES,e);
			}
		}

		FX.update_waves(WAVES,dt);

		FX.draw_waves(WAVES);

	// UPDATE BOLTS

//		bool fire = (Player.key == 'B');//(FrameIndex & 0x1F) == 0;
		bool fire = (FrameIndex & 0x1F) == 0;

		if (fire)
		{
			Effect *e = FX.new_effect();
			if (e)
			{
				static float a = 0;
				FX.init_bolt(e,a);
				FX.add(BOLTS,e);
				a += 2*PI/TURN;
			}
		}

		FX.update_bolts(BOLTS,dt);

		FX.draw_bolts(BOLTS);

	// UPDATE LASERS

		bool zap = (Player.key == 'L');//(FrameIndex & 0xFF) == 0;

		if (zap)
		{
			Effect *e = FX.new_effect();
			if (e)
			{
				FX.init_laser(e);
				FX.add(LASERS,e);
			}
		}

		FX.update_lasers(LASERS,dt);

		FX.draw_lasers(LASERS);

	// UPDATE THRUST

		bool thrust = (Player.key == 'T'); //(FrameIndex & 0x3FF) == 0;

		if (thrust)
		{
			Effect *e = FX.new_effect();
			if (e)
			{
				FX.init_thrust(e);
				e->x = fsrand() * 20;
				e->y = fsrand() * 20;
				e->r = (rand() & 3) * 85;
				e->g = (rand() & 3) * 85;
				e->b = (rand() & 3) * 85;
				e->radius = 1 + 10*frand();
				e->length = 30 + 50*frand();
				FX.add(3,e);
			}
		}

		FX.update_thrust(3,dt);

		FX.draw_thrust(3);

	}

//---------------------------------------------------------------------------

	struct Puff
	{
		float x,y,z;
		float angle;
		float radius;
		float spin;
		float age, max_life;

		void init (void)
		{
			angle = 0;
			spin = 0;
			age = 0;
		}
	};

	void draw_cube (float x, float y, float z, float size)
	{
		float Tnw[] = { x-size,y+size,z-size };
		float Tne[] = { x+size,y+size,z-size };
		float Tse[] = { x+size,y+size,z+size };
		float Tsw[] = { x-size,y+size,z+size };

		float Bnw[] = { x-size,y-size,z-size };
		float Bne[] = { x+size,y-size,z-size };
		float Bse[] = { x+size,y-size,z+size };
		float Bsw[] = { x-size,y-size,z+size };

		#define V(x) glVertex3fv(x)

		glBegin(GL_QUADS);
			V(Tnw); V(Tne); V(Tse); V(Tsw);
			V(Tsw); V(Tse); V(Bse); V(Bsw);
			V(Tse); V(Tne); V(Bne); V(Bse);
			V(Tnw); V(Tsw); V(Bsw); V(Bnw);
			V(Tne); V(Tnw); V(Bnw); V(Bne);
			V(Bsw); V(Bse); V(Bne); V(Bnw);
		glEnd();

		#undef Tnw
		#undef Tne
		#undef Tse
		#undef Tsw
		#undef Bnw
		#undef Bne
		#undef Bse
		#undef Bsw
	}

	GLenum next_src (GLenum m)
	{
		switch (m)
		{
		case GL_ZERO:				return GL_ONE;
		case GL_ONE:				return GL_DST_COLOR;
		case GL_DST_COLOR:			return GL_ONE_MINUS_DST_COLOR;
		case GL_ONE_MINUS_DST_COLOR:return GL_SRC_ALPHA;
		case GL_SRC_ALPHA:			return GL_ONE_MINUS_SRC_ALPHA;
		case GL_ONE_MINUS_SRC_ALPHA:return GL_DST_ALPHA;
		case GL_DST_ALPHA:			return GL_ONE_MINUS_DST_ALPHA;
		case GL_ONE_MINUS_DST_ALPHA:return GL_SRC_ALPHA_SATURATE;
		case GL_SRC_ALPHA_SATURATE:
		default:					return GL_ZERO;
		}
	}

	GLenum next_dst (GLenum m)
	{
		switch (m)
		{
		case GL_ZERO:				return GL_ONE;
		case GL_ONE:				return GL_SRC_COLOR;
		case GL_SRC_COLOR:			return GL_ONE_MINUS_SRC_COLOR;
		case GL_ONE_MINUS_SRC_COLOR:return GL_SRC_ALPHA;
		case GL_SRC_ALPHA:			return GL_ONE_MINUS_SRC_ALPHA;
		case GL_ONE_MINUS_SRC_ALPHA:return GL_DST_ALPHA;
		case GL_DST_ALPHA:			return GL_ONE_MINUS_DST_ALPHA;
		case GL_ONE_MINUS_DST_ALPHA:
		default:					return GL_ZERO;
		}
	}

	const char *get_enum (GLenum e)
	{
		switch (e)
		{
		case GL_ZERO:				return "GL_ZERO";
		case GL_ONE:				return "GL_ONE";
		case GL_SRC_COLOR:			return "GL_SRC_COLOR";
		case GL_ONE_MINUS_SRC_COLOR:return "GL_ONE_MINUS_SRC_COLOR";
		case GL_DST_COLOR:			return "GL_DST_COLOR";
		case GL_ONE_MINUS_DST_COLOR:return "GL_ONE_MINUS_DST_COLOR";
		case GL_SRC_ALPHA:			return "GL_SRC_ALPHA";
		case GL_ONE_MINUS_SRC_ALPHA:return "GL_ONE_MINUS_SRC_ALPHA";
		case GL_DST_ALPHA:			return "GL_DST_ALPHA";
		case GL_ONE_MINUS_DST_ALPHA:return "GL_ONE_MINUS_DST_ALPHA";
		case GL_SRC_ALPHA_SATURATE:	return "GL_SRC_ALPHA_SATURATE";
		}
		return "UNKNOWN?";
	}

	void test_cloud (float dt)
	{
		DefaultView();

		glDisable(GL_CULL_FACE);
		glFrontFace(GL_CW);

		float min[] = { -40, -20, -60 };
		float max[] = { +40, +20, -80 };

		static int count = 0;

		#define MAX_PUFFS 512
		static Puff *list[MAX_PUFFS];

	// DRAW CUBE INSIDE CLOUD

		static float a = 0; a += PI*dt;
		glColor3ub(128, 32, 32);
		draw_cube(45*sin(a), 45*cos(a), -75, 3);

		glColor3ub(128,128,128);
		draw_cube(  0,   0, -75, 5);

		glColor3ub( 32,128, 32);
		draw_cube(15*cos(a), 15*sin(a), -75, 3);

	// CREATE PUFFS

		if (count < MAX_PUFFS)
		{
			Puff *p = new Puff;
			if (p)
			{
				p->init();

				p->max_life = 4 + fsrand()*3;

				p->spin = fsrand()*PI/4;

				p->x = fsrand() * 40;
				p->y = fsrand() * 20;
				p->z = fsrand() * 10 -50;

				p->angle = frand() * 2*PI;
				p->radius = 6 + fsrand()*4;

				list[count++] = p;
			}
		}

	// UPDATE PUFFS

		int out = 0;
		for (int i=0; i<count; i++)
		{
			Puff *p = list[i];
			if (p)
			{
				p->age += dt;

				p->angle += dt*p->spin;

				if (p->age > p->max_life)
				{
					delete p;
					list[i] = 0;
				}
				else
				{
					list[out++] = p;
				}
			}
		}
		count = out;

	// SORT PUFFS

		for (int s1=0; s1<count; s1++)
		{
			Puff *p1 = list[s1];
			for (int s2=s1+1; s2<count; s2++)
			{
				Puff *p2 = list[s2];
				if (p1->z > p2->z)
				{
					list[s1] = p2;
					list[s2] = p1;
					p1 = p2;
				}
			}
		}

	// DRAW PUFFS

		static int txm[4] = { -1, -1, -1, -1 };
		static Bitmap shape[4];
		static index = 0;
		if (txm[0] == -1)
		{
			txm[0] = LoadTexture(shape[0],"art\\cloud8.tga");
			txm[1] = LoadTexture(shape[1],"art\\flare8.tga");
			txm[2] = LoadTexture(shape[2],"art\\bubble8.tga");
			txm[3] = LoadTexture(shape[3],"art\\spike8.tga");
		}

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,txm[index]);

		static GLenum src = GL_SRC_ALPHA;
		static GLenum dst = GL_ONE;

		if (Player.key == 'Z')
			index = (index+1) & 3;

		if (Player.key == 0xBC)	// VK_COMMA?
			src = next_src(src);
		if (Player.key == 0xBE) // VK_PERIOD?
			dst = next_dst(dst);

		glEnable(GL_BLEND);
		glBlendFunc(src,dst);

		glBegin(GL_QUADS);
		for (int z=0; z<count; z++)
		{
			Puff *p = list[z];
			if (p)
			{
				unsigned char alpha = 70*sin(PI*p->age/p->max_life);
				glColor3ub(alpha,alpha,alpha);
//				glColor3ub(255,255,255, alpha);

				float angle = p->angle + 0.25*PI;
				float x,y,z;
				z = p->z;
				x = p->x + p->radius*sin(angle);
				y = p->y + p->radius*cos(angle);
				glTexCoord2f(0,0); glVertex3f(x,y,z);
				angle += 0.50*PI;
				x = p->x + p->radius*sin(angle);
				y = p->y + p->radius*cos(angle);
				glTexCoord2f(1,0); glVertex3f(x,y,z);
				angle += 0.50*PI;
				x = p->x + p->radius*sin(angle);
				y = p->y + p->radius*cos(angle);
				glTexCoord2f(1,1); glVertex3f(x,y,z);
				angle += 0.50*PI;
				x = p->x + p->radius*sin(angle);
				y = p->y + p->radius*cos(angle);
				glTexCoord2f(0,1); glVertex3f(x,y,z);
			}
		}
		glEnd();

		OrthoView();

		char msg[128];
		sprintf(msg,"SRC = %s",get_enum(src));
		glutTextPrint(32,440,msg);
		sprintf(msg,"DST = %s",get_enum(dst));
		glutTextPrint(32,460,msg);
	}

//---------------------------------------------------------------------------

	void draw_block (int i)
	{
		float a  = float(i & 7) * PI/4;

		float x =  64+ ((i >> 3) & 15) * 32;
		float y =  128+ ((i >> 7) & 7)  * 32;

		float r = 12.0;

		glVertex3f(x,y,0);
		glVertex3f(x+r*sin(a),y+r*cos(a),0);
		float a1 = a + PI/8;
		glVertex3f(x+r*sin(a1),y+r*cos(a1),0);
	}

	void test_blocks (void)
	{
		static bool one = true;

		if (Player.key == VK_RETURN)
			one = !one;

		OrthoView();

		glDisable(GL_CULL_FACE);

		int count = 8*16*8;
		if (one)
		{
			glColor3ub(255,64,64);

			glBegin(GL_TRIANGLES);
			for (int i=0; i<count; i++)
			{
				draw_block(i);
			}
			glEnd();
		}
		else
		{
			glColor3ub(64,255,64);

			int blocks = 64;
			for (int i0=0; i0<count;)
			{
				glBegin(GL_TRIANGLES);

				int i1 = i0 + blocks;
				if (i1 > count) i1 = count;
				for (int i=i0; i<i1; i++)
				{
					draw_block(i);
				}
				i0 = i1;

				glEnd();
			}
		}
	}

//---------------------------------------------------------------------------

	void test_depth (void)
	{
		OrthoView();

		#define zRect(x1,y1,x2,y2,z) glVertex3f(x1,y1,z);glVertex3f(x2,y1,z);glVertex3f(x2,y2,z);glVertex3f(x1,y2,z);
/*
		glBegin(GL_QUADS);
			glGRAY(255); zRect(540,20, 580,80, -  1);
			glGRAY(192); zRect(530,25, 590,35, -100);
			glGRAY(128); zRect(520, 0, 560,60, -500);
			glGRAY( 64); zRect(500,40, 560,99, -999);
		glEnd();

		glBegin(GL_QUADS);
			glGRAY(255); zRect(440,20, 480,80, -999);
			glGRAY(192); zRect(430,25, 490,35, -999);
			glGRAY(128); zRect(420, 0, 460,60, -1000);
			glGRAY( 64); zRect(400,40, 460,99, -1000);
		glEnd();
*/
		//DefaultView(512*1024);
		DefaultView(1E5);

		static float z = 1075;
		static float dz = 0;
		if (Player.key == VK_RETURN)
			dz = 51 - dz;
		z += dz;
		if (z > 1E6)
			z = 100;

#define zTRI(R,A,Z) \
{ \
	if (r > 1000) r = 1000; \
	glVertex3f(R*sin(A       ),R*cos(A       ),Z); \
	glVertex3f(R*sin(A+2*PI/3),R*cos(A+2*PI/3),Z); \
	glVertex3f(R*sin(A-2*PI/3),R*cos(A-2*PI/3),Z); \
}

		static float a0 = 0;		a0 += PI/32;
		static float a1 = PI/3;		a1 -= PI/32;
		static float a2 = 0;		a2 += PI/64;
		float r;
#if 0
		glBegin(GL_TRIANGLES);
			r = 0.3*z;
			glGRAY(255); zTRI(r,a0,-z-0);
					r = 0.5*z;
					glGRAY( 96); zTRI(r,a2,-z-20);
				r = 0.4*z;
				glGRAY(128); zTRI(r,a1,-z-10);
		glEnd();
#endif

// Highway Knight terrain?


		glDisable(GL_CULL_FACE);
float vertices[] =
{
// Quad 1 

	-165.196,
	-63.9500,
	-85.4854,

	81.4751,
	-26.9502,
	-143.109,

	149.955,
	-160.227,
	64.4573,

	-96.7160,
	-197.227,
	122.081,

// Quad 2

	81.4751,
	-26.9502,
	-143.109,

	328.146,
	10.0496,
	-200.733,

	396.626,
	-123.227,
	6.83353,

	149.955,
	-160.227,
	64.4573,

// Triangles

	-141.219,
	-76.6141,
	-62.0605,

	105.452,
	-39.6144,
	-119.684,

	101.172,
	-31.2846,
	-132.657,

	-145.499,
	-68.2843,
	-75.0334,
};

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(36.8,4.0/3, 1,500);
		glViewport(0,0,640,480);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3,GL_FLOAT,0,vertices);

		glBegin(GL_QUADS);
			glColor3ub(255,0,0);
			glArrayElement(0);
			glArrayElement(1);
			glArrayElement(2);
			glArrayElement(3);

			glColor3ub(180,0,0);
			glArrayElement(4);
			glArrayElement(5);
			glArrayElement(6);
			glArrayElement(7);
		glEnd();

		glBegin(GL_TRIANGLES);
			glColor3ub(255,255,255);
			glArrayElement(8);
			glArrayElement(9);
			glArrayElement(10);

			glArrayElement(8);
			glArrayElement(10);
			glArrayElement(11);
		glEnd();

		DefaultView();

		glBegin(GL_TRIANGLES);
			glColor3ub(0,255,0);
			glArrayElement(8);
			glArrayElement(9);
			glArrayElement(10);

			glArrayElement(8);
			glArrayElement(10);
			glArrayElement(11);
		glEnd();

		glDisableClientState(GL_VERTEX_ARRAY);

		OrthoView();

		char msg[128];
		sprintf(msg,"Z = %d",int(z));
		glutTextPrint(300,450,msg);
	}

//---------------------------------------------------------------------------

	void get_shape_rect (float *rect, int x, int y, int nx, int ny)
	{
		float dx = 1.0 / nx;
		float dy = 1.0 / ny;

		float tx = x * dx;
		float ty = y * dy;

		rect[0] = tx;
		rect[1] = ty;

		rect[2] = tx + dx;
		rect[3] = ty + dy;
	}

	void draw_rect (const float *rect, float x1, float y1, float x2, float y2, float z=0)
	{
		glBegin(GL_QUADS);
			glTexCoord2f(rect[0],rect[1]); glVertex3f(x1,y1,z);
			glTexCoord2f(rect[2],rect[1]); glVertex3f(x2,y1,z);
			glTexCoord2f(rect[2],rect[3]); glVertex3f(x2,y2,z);
			glTexCoord2f(rect[0],rect[3]); glVertex3f(x1,y2,z);
		glEnd();
	}

	void test_shapes (void)
	{
		OrthoView();

		float rect[4];

		static int stars = -1;
		static Bitmap shapes;
		if (stars == -1)
		{
			stars = LoadTexture(shapes,"art\\stars8.tga");
		}

		float dx = 1.0 / 4;
		float dy = 1.0 / 4;

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,stars);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);

		glDisable(GL_BLEND);
		glColor3ub(255,255,25);
		get_shape_rect(rect, 0,0, 4,4);
		draw_rect(rect, 100,100, 129,129);

		glEnable(GL_BLEND);
		glColor3ub(255,0,0);
		get_shape_rect(rect, 1,1, 4,4);
		draw_rect(rect, 130,100, 159,129);

		glDisable(GL_BLEND);
		glColor3ub(0,255,0);
		get_shape_rect(rect, 2,2, 4,4);
		draw_rect(rect, 160,100, 189,129);

		glEnable(GL_BLEND);
		glColor3ub(0,0,255);
		get_shape_rect(rect, 3,3, 4,4);
		draw_rect(rect, 190,100, 219,129);

		glDisable(GL_BLEND);
		glColor3ub(255,255,255);
		get_shape_rect(rect, 0,0, 1,1);
		draw_rect(rect, 100,240, 220,360);
	}

//---------------------------------------------------------------------------

	void test_mipmaps (void)
	{
		DefaultView();

		glEnable(GL_TEXTURE_2D);

		float rect[4] = { 0,0, 1,1};

		static int txm = -1;
		static Bitmap shape;
		if (txm == -1)
		{
			txm = LoadTexture(shape,"art\\stars8.tga");
		}

		glBindTexture(GL_TEXTURE_2D,txm);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

		static float angle = 0;
		static float da = 0;
		angle += da;
		if (Player.key == VK_RETURN)
			da = PI/400 - da;
		float z = 100 + 500 + 500*sin(angle);
		
		draw_rect(rect, -40,+85, +40,+ 5, -z);

		static int txm2 = -1;
		static Bitmap shape2;
		if (txm2 == -1)
		{
			txm2 = LoadTexture(shape2,"art\\stars8.tga");
		}

		glBindTexture(GL_TEXTURE_2D,txm2);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

		draw_rect(rect, -40,- 5, +40,-85, -z);
	}

//---------------------------------------------------------------------------

	void test_texmem (void)
	{
		OrthoView();

		glEnable(GL_TEXTURE_2D);

		static int tested = 0;
		if (!tested)
		{
			tested = 1;

			Bitmap bmp;
			bmp.load("art\\hex256.tga");

			#define MAX 100
			int txms[MAX];
			for (int i=0; i<MAX; i++)
			{
				char msg[128];
				sprintf(msg,"shape = %d\n",i);
				DebugPrint(msg);
				txms[i] = ReUseTexture(bmp);
				glBegin(GL_TRIANGLES);
				glEnd();
			}
		}
	}

//---------------------------------------------------------------------------

	void test_pixels (void)
	{
		OrthoView();

		typedef unsigned char uchar;

		static uchar pixels[640*480];

		#define GREY(x)	x,x,x

		static uchar palette[8*3] = 
		{
			GREY(0x1F),
			GREY(0x3F),
			GREY(0x5F),
			GREY(0x7F),
			GREY(0x9F),
			GREY(0xBF),
			GREY(0xDF),
			GREY(0xFF),
		};

		glColorTableEXT(GL_TEXTURE_2D,GL_RGB8,8,GL_RGB,GL_UNSIGNED_BYTE,palette);

		if (Player.key == VK_RETURN)
		{
			static xx = 132;
			static yy = 173;

			for (int y=0; y<480; y++)
			for (int x=0; x<640; x++)
			{
//				float f = x*sin(xx/float(y+1)) + y*cos(yy/float(x+1));
//				uchar c = uchar(f) & 7;
				uchar c = int(x / 32) & 0x6;
				c += int(y / 64) & 0x1;
				pixels[x+y*640] = c;
			}
		}

		static float a = 0; a += PI/64;

		float x = 48*sin(a);
		float y = 16*cos(a);

		glRasterPos3f(x,y,0);
		glDrawPixels(640,480,GL_COLOR_INDEX,GL_UNSIGNED_BYTE,pixels);

		//ClearScreen = false;
	}

//---------------------------------------------------------------------------

	void test_lock (void)
	{
		if (glLockBufferEXT==0 || glUnlockBufferEXT==0)
			return;

		OrthoView();

		unsigned short *lfb = 0;
		int stride;

		glColor3ub(192,0,192);
		glBegin(GL_LINES);
			glVertex3f(200, 40,0); glVertex3f(200,440,0);
			glVertex3f(202, 40,0); glVertex3f(202,440,0);
		glEnd();

		glLockBufferEXT((void**)&lfb,&stride);
		if (lfb)
		{
			#define PIXEL(x,y) lfb[x+y*(stride/2)]
			PIXEL(16, 8) = 0xFFFF;
			PIXEL(20,16) = 0xF800;
			PIXEL(16,24) = 0x001F;
			PIXEL(12,16) = 0x07E0;

			for (int y=200; y<280; y++)
			{
				void *line = &PIXEL(40,y);
				memset(line,0xFF,600*2);
			}

			//glUnlockFrameBuffer();
		}

		glColor3ub(255,0,255);
		glBegin(GL_LINES);
			glVertex3f(100, 40,0); glVertex3f(100,440,0);
			glVertex3f(102, 40,0); glVertex3f(102,440,0);
		glEnd();

		glColor3ub(255,0,255);
		glBegin(GL_LINES);
			glVertex3f(100, 40,0); glVertex3f(100,440,0);
			glVertex3f(102, 40,0); glVertex3f(102,440,0);
		glEnd();

	}

//---------------------------------------------------------------------------
// RENDER
//---------------------------------------------------------------------------

	int App::render (void)
	{
		if (ClearScreen)
		{
			glClearColor(BackColor[0],BackColor[1],BackColor[2],BackColor[3]);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		}

		static int test[256] = { -1 };

		if (test[0] == -1)
		{
			memset(test,0,sizeof(test));
		}

		unsigned k = Player.key;
		if (k < 256)
			test[k] = !test[k];

		//test_fx(time.time_per_frame);

		if (k == 'Q')
		{
			BackColor[2] += 0.1; if (BackColor[2] > 1.0) BackColor[2] = 0;
		}

		if (test['0']) test_cube();
		if (test['1']) test_clip();
		if (test['2']) test_speed();
		if (test['3']) test_fov();
		if (test['5']) test_blend();
		if (test['6']) test_array();

		if (test['N']) test_nearclip();
		if (test['S']) test_shapes();
		if (test['L']) test_lock();
		if (test['D']) test_depth();
		if (test['B']) test_blocks();

		if (test['M']) test_mipmaps();
		if (test['T']) test_texmem();

		if (test['P']) test_pixels();

		if (test['C']) test_cloud(time.time_per_frame);

		#define NUM 8
		static float x[NUM];
		static float y[NUM];

		if ((FrameIndex & 0xFF) == 0)
		{
			for (int i=0; i<NUM; i++)
			{
				x[i] = float(rand()) * DISPLAY_W / RAND_MAX;
				y[i] = float(rand()) * DISPLAY_H / RAND_MAX;
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
#if 0
		glColor3ub(255,0,0);
		glBegin(GL_TRIANGLES);

			int g = rand() * 255 / RAND_MAX;
			glColor3ub(255,0,0);
			glVertex3f(x[0],y[0]); glVertex3f(x[1],y[1]); glVertex3f(x[2],y[2]);
			glColor3ub(0,255,0);
			glVertex3f(x[1],y[1]); glVertex3f(x[2],y[2]); glVertex3f(x[3],y[3]);
			glColor3ub(0,0,255);
			glVertex3f(x[2],y[2]); glVertex3f(x[3],y[3]); glVertex3f(x[0],y[0]);

		glEnd();
#endif


#if 0
		static float fps = 0;
		#define DELAY 0.5
		static float delay = 0;
		delay -= time.time_per_frame;
		if (delay <= 0)
		{
			delay = DELAY;
			fps = time.fps;
		}
#else
		#define AVG 16
		static float fps_list[AVG];
		fps_list[FrameIndex&15] = time.fps;
		float fps = 0;
		for (int i=0; i<AVG; i++)
		{
			fps += fps_list[i];
		}
		fps /= AVG;
#endif

		OrthoView();

#if 1
		char msg[128];
		sprintf(msg,"FPS = %d",int(fps));
		glColor3ub(255,0,0);
		glutTextPrint(8,64,msg);

		drivers.get_parm(msg,0);
		glColor3ub(255,255,255);
		glutTextPrint(8,24,msg);
#endif
		glFlush();
		SwapBuffers(hDC);

		return true;
	}


App TheApp;

//---------------------------------------------------------------------------
// MainWindowProc = callback for all windows messages
//---------------------------------------------------------------------------

static long FAR PASCAL MainWindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch( message )
    {
    case WM_ACTIVATEAPP:
		Active = (int) wParam;
        break;

#if USE_SAL
	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN)
		{
			TheApp.fullscreen ^= 1;
			if (TheApp.fullscreen)
			{
				WIN->ShowWindow(640, 480, WMF_FULL_SCREEN);
				DISPLAY->set_display_mode(TheApp.hDC, 640, 480, 16);
			}
			else
			{
				DISPLAY->restore_display_mode(TheApp.hDC);
				WIN->ShowWindow(640, 480, 0);
			}
			return 0;
		}
		break;
#endif

    case WM_CREATE:
        break;

	case WM_KEYDOWN:
		OnKeyDown(int(wParam));
		break;
	case WM_KEYUP:
		OnKeyUp(int(wParam));
		break;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

    case WM_CLOSE:
		PostQuitMessage(0);	   // always quit with window valid
		Quit = true;
		return 0;

    default:
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

//---------------------------------------------------------------------------

#include <crtdbg.h>	// _CrtDumpMemoryLeaks()

static void AppExit(void)
// atexit() callback
{
	DebugPrint("AppExit() called via atexit()\n");

	TheApp.close();

//	_CrtDumpMemoryLeaks();
}

//---------------------------------------------------------------------------
//
// WinMain = Windows main() function
//
//---------------------------------------------------------------------------

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int ok = 1;
	int err = 0;

//test_math();

	if (TheApp.open(hInstance,lpCmdLine))
	{
		ok = TheApp.main_loop();
	}
	else // failed to startup?
	{
		err = -1;
		ok = 0;
	}

	TheApp.close();
	if (err) exit(1);

	return ok;
}