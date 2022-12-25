//---------------------------------------------------------------------------
/*
	SYSTEM.CPP

	(Win32) Lancer (c) 1997 Digital Anvil

	02-05-97 created (pci)
	$Header: /Tools/TxmView/sys.cpp 2     7/14/99 12:46p Pisaac $
*/
//---------------------------------------------------------------------------

#include "project.h"

#include "sys.h"

#include "window.h"

#include "gamesys.h"	// ISystemContatiner

#include "physics.h"		// IPhysics
#include "renderer.h"		// IRenderer
#include "itxmlib.h"		// ITXMLib
#include "rendpipeline.h"	// RenderPipeline

#define RELEASE(x)			if(x) {(x)->Release();(x)=0;}

//---------------------------------------------------------------------------
// GLOBAL
//---------------------------------------------------------------------------

ICOManager		*DACOM = 0;
IRenderPipeline *PIPE = 0;
IEngine			*ENGINE = 0;

IPhysics		*PHYSICS = 0;
IModel			*MODEL = 0;
ICollision		*COLLISION = 0;
IRenderer		*RENDER = 0;
ITXMLib			*TXMLIB = 0;
ILightManager	*LIGHT = 0;

int ScreenWidth = 0;
int ScreenHeight = 0;

//---------------------------------------------------------------------------
// DispMode
//---------------------------------------------------------------------------

struct DispMode
{
	bool fullscreen;
	bool flip;

	int width;
	int height;

	int color_bpp;
	int depth_bpp;

	DispMode (void)
	{
		fullscreen = false;
		flip = false;

		width = height = 0;		// note: default is an invalid state

		color_bpp = 16;
		depth_bpp = 16;
	}

	bool is_valid (void) const
	{
		return width > 0 && height > 0;
	}
};

DispMode CurrentMode;

//---------------------------------------------------------------------------
// Set Window Styles
//---------------------------------------------------------------------------

static void SetStyleInWindow (HWND hWnd, int display_size_X, int display_size_Y)
{
	// Enable caption menu and user preferences
	
	SetWindowLong(hWnd, GWL_STYLE, 
		(GetWindowLong(hWnd, GWL_STYLE) | (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX))
		& ~WS_POPUP);

	SetWindowLong(hWnd, GWL_EXSTYLE, 
		GetWindowLong(hWnd, GWL_EXSTYLE) & ~WS_EX_TOPMOST);

	// If area not already established, center window's client area on
	// desktop, and size it to correspond to the display size for optimum 
	// performance (no stretching needed)
	
	RECT rect;

	GetWindowRect(GetDesktopWindow(),&rect);
	int DESK_W = rect.right;
	int DESK_H = rect.bottom;

	rect.left = (DESK_W - display_size_X) / 2;
	rect.top = (DESK_H - display_size_Y) / 2;
	rect.right = rect.left + display_size_X - 1;
	rect.bottom = rect.top + display_size_Y - 1;

	AdjustWindowRectEx (&rect,
		GetWindowLong(hWnd, GWL_STYLE),
		(GetMenu(hWnd) != NULL),
		GetWindowLong(hWnd, GWL_EXSTYLE));

	SetWindowPos (hWnd, 
		HWND_TOP, 
		rect.left,rect.top,
		rect.right  - rect.left + 1,
		rect.bottom - rect.top  + 1,
		SWP_NOCOPYBITS | SWP_NOZORDER);
}

//---------------------------------------------------------------------------

static void SetStyleFullScreen (HWND hWnd)
{
	// Disable caption menu

	SetMenu(hWnd, NULL);	// unattach any menu

	SetWindowLong(hWnd, GWL_STYLE, 
		GetWindowLong(hWnd, GWL_STYLE)
		| WS_POPUP
		& ~(WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_THICKFRAME));

	// Set window boundaries to cover entire desktop

	SetWindowPos(hWnd, 
		HWND_TOP, 
		0,0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN),
		SWP_NOCOPYBITS | SWP_NOZORDER);
}

static void SetupWindow (HWND hWnd, int width, int height, bool fullscreen=false)
{
	if (fullscreen)
		SetStyleFullScreen(hWnd);
	else
		SetStyleInWindow(hWnd,width,height);
}


//****************************************************************************
//
// Exit handlers to shut down AIL and DirectDraw
//
//****************************************************************************

static void AppExit(void)
{
	DebugPrint("AppExit() called via atexit()\n");

	TheSystem.shutdown();				// App::close()

	DebugPrint("Final exit OK\n");
}

//---------------------------------------------------------------------------
// System
//---------------------------------------------------------------------------

System TheSystem;		// GLOBAL INSTANCE

//---------------------------------------------------------------------------
// SYSTEM
//---------------------------------------------------------------------------

static IDAComponent *CreateAggComponent (const char *name)
{
	IDAComponent *result = 0;

	AGGDESC ddesc(name);

	IDAComponent* dacomp;

	GENRESULT ok = DACOM->CreateInstance (&ddesc, (void**)&dacomp);

	if (ok == GR_OK)
	{
		IAggregateComponent *agg;

		if ((ok = dacomp->QueryInterface("IAggregateComponent",(void**)&agg)) == GR_OK)
		{
			ok = agg->Initialize();
			agg->Release();
		}

		if (ok == GR_OK)
		{
			if (dacomp->QueryInterface (name,(void**)&result) == GR_OK)
				dacomp->Release ();
		}
	}

	return result;
}


bool System::startup (HINSTANCE instance)
{
	DebugPrint("System::startup()\n");

	hInstance = instance;

	ASSERT(DACOM == 0);

	atexit(AppExit);	// make sure System::close gets called to free resources

	DACOM = DACOM_Acquire();

	if (DACOM == NULL)
	{
		DebugPrint("ERROR: unable to acquire DACOM component(s)\n");
		return false; // missing DA's component loader?
	}


	extern char ExePath[];
	char file[_MAX_PATH];
	_makepath(file, 0,ExePath,"TxmView.ini",0);

	DACOM->SetINIConfig(file,0);

//
//	STARTUP - ISystemContainer, IEngine
//

	ISystemContainer *SYS = (ISystemContainer*)CreateAggComponent("ISystemContainer");

	if (SYS)
	{
		SYS->LoadSystemComponents();

		// Verify essential SYSTEM components

		SYS->QueryInterface ("IRenderPipeline", (void**)&PIPE);

	// RENDER PIPELINE

		if (!PIPE)
		{
			DebugPrint("ERROR: missing essential SYSTEM component(s) (IRenderPipeline)\n");
			return false;
		}

		if (PIPE->startup() != GR_OK)
		{
			DebugPrint("Pipeline::startup() failed\n");
			return false;
		}

		PIPE->set_pipeline_state(RP_BATCH_TRANSLUCENT_POOL,128*1024);

	// ENGINE

		DACOMDESC desc = "IEngine";
		if (DACOM->CreateInstance(&desc, (void**)&ENGINE) == GR_OK)
		{
			ENGINE->load_engine_components(SYS);

			// Verify essential ENGINE components

			ENGINE->QueryInterface("IPhysics",(void**)&PHYSICS);
			if (!PHYSICS)
			{
				DebugPrint("ERROR: missing essential ENGINE component (IPhysics)\n");
				return false;
			}

			ENGINE->QueryInterface("IModel",(void**)&MODEL);
			if (!MODEL)
			{
				DebugPrint("ERROR: missing essential ENGINE component (IModel)\n");
				return false;
			}

			ENGINE->QueryInterface("ICollision",(void**)&COLLISION);
			if (!COLLISION)
			{
				DebugPrint("ERROR: missing essential ENGINE component (ICollision)\n");
				return false;
			}

			ENGINE->QueryInterface("IRenderer",(void**)&RENDER);
			if (!RENDER)
			{
				DebugPrint("ERROR: missing essential ENGINE component (IRenderer)\n");
				return false;
			}

			if (ENGINE->QueryInterface ("ITXMLib", (void**)&TXMLIB) != GR_OK)
			{
				DebugPrint("ERROR: missing essential ENGINE component (ITXMLib)\n");
				return false;
			}

			ENGINE->QueryInterface ("ILightManager", (void**)&LIGHT);
			if (!LIGHT)
			{
				DebugPrint ("No light manager\n");
			}
		}
	}

	return true;
}

void System::shutdown (void)
{
	DebugPrint("System::shutdown()\n");

	if (DACOM) // is System open?
	{
		if (PIPE)
		{
			PIPE->shutdown();
			RELEASE(PIPE);
		}

		//RELEASE(WIN);

		RELEASE(PHYSICS);
		RELEASE(COLLISION);
		RELEASE(RENDER);
		RELEASE(TXMLIB);

		RELEASE(LIGHT);

		if (DACOM)
		{
			DACOM->ShutDown();
			DACOM->Release();
			DACOM = 0;
		}
	}
}

//---------------------------------------------------------------------------
// RenderMgr
//---------------------------------------------------------------------------

	bool activate (HWND hWnd, const DispMode &mode)
	{
		bool active = false;

		if (mode.width && mode.height && mode.color_bpp)
		{
			PIPE->set_pipeline_state(RP_BUFFERS_COUNT, 2);

			PIPE->set_pipeline_state(RP_BUFFERS_COLOR_BPP, mode.color_bpp);
			PIPE->set_pipeline_state(RP_BUFFERS_DEPTH_BPP, mode.depth_bpp);

			PIPE->set_pipeline_state(RP_BUFFERS_FULLSCREEN, mode.fullscreen);
			PIPE->set_pipeline_state(RP_BUFFERS_HWFLIP, mode.flip);
			
			if (PIPE->create_buffers(hWnd, mode.width, mode.height) == GR_OK)
			{
			// note: change after create so Desktop window is correct
				//SetupWindow(hWnd, mode.width, mode.height, mode.fullscreen);

				active = true;
			}
			else
			{
				DebugPrint("RenderMgr - failed to create_buffers(%d,%d)\n",mode.width,mode.height);
			}
		}
		return active;
	}

	bool set_display_mode (HWND hWnd, const DispMode &mode)
	{
		if (!PIPE)
			return false;

	DebugPrint("set_display_mode(%d,%d)\n",mode.width,mode.height);

		if (activate(hWnd,mode))
		{
			CurrentMode = mode;
//pci?
			ScreenWidth = mode.width;
			ScreenHeight = mode.height;

			//glViewport(0,0,ScreenWidth,ScreenHeight);

			return true;
		}

	// FAILED - restore previous mode

		if (CurrentMode.is_valid()) // was it ever activated?
		{
			activate(hWnd,CurrentMode);
		}

		return false;
	}

bool System::is_ready (void)
{
	return CurrentMode.is_valid();
}

int NumOpen = 0;

bool System::open_view (Window *win)
{
	DebugPrint("Opening View...\n");

	if (DACOM == 0 || PIPE == 0)
		return false;

	if (!win)
		return false;

	HWND hWnd = win->hWnd;
	{
	HDC hDC = GetDC(hWnd);

	if (hWnd == 0 || hDC == 0)
		return false;

	ReleaseDC(hWnd,hDC);
	}

	if (++NumOpen > 1)	// have the buffers already created?
		return true;

// Analyze Desktop

	HWND desk = GetDesktopWindow();
	HDC dc = GetWindowDC(desk);

	RECT rect;
	#if 1 // note: necessary to allow window to be RE-SIZED to full screen!
	GetWindowRect(desk,&rect);
	int WIN_W = rect.right;
	int WIN_H = rect.bottom;
	#else
	GetClientRect(hWnd,&rect);
	int WIN_W = rect.right;
	int WIN_H = rect.bottom;
	#endif

	int DESK_BPP = GetDeviceCaps(dc, BITSPIXEL);

	ReleaseDC(desk, dc);

// Set Video mode

	DispMode mode;
	mode.width = WIN_W;
	mode.height = WIN_H;
	mode.color_bpp = DESK_BPP;
	mode.fullscreen = fullscreen;

	if (!set_display_mode(hWnd,mode))
	{
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------

void System::close_view (Window *win)
{
	DebugPrint("Closing View...\n");

	HWND hWnd = win->hWnd;

	if (--NumOpen > 0)	// are buffers still needed?
		return;

// SHUTDOWN

	if (CurrentMode.is_valid() && CurrentMode.fullscreen)
	{
		SetupWindow(hWnd,ScreenWidth,ScreenHeight,false);
	// how do we have to destroy PIPE buffers?
		CurrentMode.fullscreen = false;
	}

/*
	if (hRC)
	{
		wglMakeCurrent(0,0);
		wglDeleteContext(hRC);
		hRC = 0;
	}
*/

	ShowWindow(hWnd,SW_HIDE);

	PIPE->destroy_buffers();//hWnd);

	hWnd = 0;
}

//---------------------------------------------------------------------------

#include "resource.h"

bool System::update (int &result)
{
	//WIN->ServeMessageQueue();
    MSG msg;
	result = 0;

	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage(&msg, NULL, 0, 0))
		{
			result = msg.wParam;
			return false;
		}

		BOOL used = false;

		CWnd *w = CWnd::FromHandle(msg.hwnd);
		if (w)
		{
			used = w->PreTranslateMessage(&msg);
		}
		if (!used)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return true;
}
