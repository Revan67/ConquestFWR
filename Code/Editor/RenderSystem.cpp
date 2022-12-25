
#include "stdafx.h"
#include "globals.h"

#include <DACOM.h>
#include <HeapObj.h>
#include <GameSys.h>
#include <FileSys.h>
#include <RendPipeline.h>
#include <renderer.h>
#include <IHardPoint.h>
#include <IRenderPrimitive.h>
#include <Streamer.h>
#include <LightMan.h>
#include <IVertexBufferManager.h>
#include <ITextureLibrary.h>
#include <DPlay.h>
#include "CQTrace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CQEXTERN LPTOP_LEVEL_EXCEPTION_FILTER prevExceptionHandler;
// CQEXTERN LPTOP_LEVEL_EXCEPTION_FILTER cqExceptionHandler;

static bool bStartupCalledOnce = true;
static bool bHasBuffers = false;

static U32 iniResX, iniResY;
static U32 bitDepth;
static bool bForceMonochromeOff, bForceMonochromeOn;

static DWORD showWindowFlags = 0;
static RECT windowRect, clientRect;

//---------------------------------------------------------------------
//
static void enableIMC (bool bEnable)
{
//	if (bEnable)
//	{
//		if (hIMC)
//			ImmAssociateContext(hMainWindow, hIMC);
//	}
//	else
//	{
//		HIMC _hIMC = ImmAssociateContext(hMainWindow, 0);
//
//		if (hIMC==0)
//			hIMC = _hIMC;
//	}
}
//---------------------------------------------------------------------
//
void WindowManager_GetClientArea (WM_WINAREA & area)
{
	if (showWindowFlags & WMF_FULL_SCREEN)
	{
		area.x = area.y = 0;
		area.w = GetSystemMetrics(SM_CXSCREEN);
		area.h = GetSystemMetrics(SM_CYSCREEN);
	}
	else
	{
		::GetClientRect( hMainWindow, &clientRect );

		area.x = clientRect.left;
		area.y = clientRect.top;
		area.w = clientRect.right - clientRect.left + 1;
		area.h = clientRect.bottom - clientRect.top + 1;
	}
}
//------------------------------------------------------------------------------
//
BOOL32 WindowManager_resizeTheWindow (S32 display_size_X, S32 display_size_Y, U32 flags)
{
	WM_WINAREA area;                     // Location/size of window client area
	S32      retry;

	//
	// Enable caption menu and user preferences
	//
	
//	SetWindowLong(hMainWindow, 
//		GWL_STYLE, 
//		GetWindowLong(hMainWindow, GWL_STYLE) & ~WS_POPUP);
//	
//	SetWindowLong(hMainWindow, 
//		GWL_STYLE, 
//		GetWindowLong(hMainWindow, GWL_STYLE) | (WS_OVERLAPPED  | 
//		WS_CAPTION     | 
//		WS_SYSMENU     | 
//		WS_MINIMIZEBOX));
//	
//	if (flags & WMF_ALLOW_WINDOW_RESIZE)
//	{
//		SetWindowLong(hMainWindow, 
//			GWL_STYLE, 
//			GetWindowLong(hMainWindow, GWL_STYLE) | WS_THICKFRAME |
//			WS_MAXIMIZEBOX);
//	}
//	
//	if (flags & WMF_ALWAYS_ON_TOP)
//	{
//		SetWindowLong(hMainWindow, 
//			GWL_EXSTYLE, 
//			GetWindowLong(hMainWindow, GWL_EXSTYLE) | WS_EX_TOPMOST);
//	}
//	else
//	{
//		SetWindowLong(hMainWindow, 
//			GWL_EXSTYLE, 
//			GetWindowLong(hMainWindow, GWL_EXSTYLE) & ~WS_EX_TOPMOST);
//	}

	//
	// If area not already established, center window's client area on
	// desktop, and size it to correspond to the display size for optimum 
	// performance (no stretching needed)
	//

	//
	// Get desktop size
	//
	
	S32 desktop_w = GetSystemMetrics(SM_CXSCREEN);
	S32 desktop_h = GetSystemMetrics(SM_CYSCREEN);

	WindowManager_GetClientArea(area);
	
	area.w = display_size_X;
	area.h = display_size_Y;
		
	area.x = ((desktop_w - area.w ) / 2);
	area.y = ((desktop_h - area.h) / 2);
	
	//
	// Calculate adjusted position of window
	//
	// Do not allow overall window size to exceed desktop size; keep 
	// dividing height and width by 2 until entire window fits
	//
	// If window is offscreen (or almost entirely offscreen), center it
	//
	
//	do
	{
		retry = 0;

		windowRect.left   = area.x;
		windowRect.right  = area.x + area.w - 1;
		windowRect.top    = area.y;
		windowRect.bottom = area.y + area.h - 1;
		
		AdjustWindowRectEx(&windowRect,
			GetWindowLong(hMainWindow, GWL_STYLE),
			(GetMenu(hMainWindow) != NULL),
			GetWindowLong(hMainWindow, GWL_EXSTYLE));
		
		if ((windowRect.right - windowRect.left + 1) > desktop_w)
		{
			area.w >>= 1;
			area.x = ((desktop_w - area.w ) / 2);
			retry = 1;
		}
		
		if ((windowRect.bottom - windowRect.top + 1) > desktop_h)
		{
			area.h >>= 1;
			area.y = ((desktop_h - area.h) / 2);
			retry = 1;
		}
		
		if ((windowRect.left   >= (desktop_w-16)) ||
			(windowRect.top    >= (desktop_h-16)) ||
			(windowRect.right  <= 16)          ||
			(windowRect.bottom <= 16))
		{
			area.x = ((desktop_w - area.w ) / 2);
			area.y = ((desktop_h - area.h) / 2);
			retry = 1;
		}
	}
//	while (retry);
  
	if (retry)
		return 0;
	//
	// Set window size and position
	//
	
	UINT posflags = SWP_NOCOPYBITS | SWP_NOZORDER;
	if ((flags & WMF_CENTER) == 0)
		posflags |= SWP_NOMOVE;

	::SetWindowPos(hMainWindow, 
		HWND_TOP, 
		windowRect.left,
		windowRect.top,
		windowRect.right  - windowRect.left + 1,
		windowRect.bottom - windowRect.top  + 1,
		posflags);
	
	CQTRACE14("Window at (%d,%d), client size = (%d,%d)\n", windowRect.left, windowRect.top, clientRect.right+1-clientRect.left, clientRect.bottom+1-clientRect.top);

	return 1;
}
//---------------------------------------------------------------------
//
BOOL32 WindowManager_SetWindowPos (U32 width, U32 height, U32 flags)
{
	BOOL32 result=0;

	if (flags & WMF_FULL_SCREEN)
	{
		//
		// Disable caption menu
		//
		
//		SetWindowLong(hMainWindow, GWL_STYLE, GetWindowLong(hMainWindow, GWL_STYLE) | WS_POPUP);
		
//		SetWindowLong(hMainWindow, 
//			GWL_STYLE, 
//			GetWindowLong(hMainWindow, GWL_STYLE) & ~(WS_OVERLAPPED  | 
//			WS_CAPTION     | 
//			WS_SYSMENU     | 
//			WS_MINIMIZEBOX | 
//			WS_MAXIMIZEBOX | 
//			WS_THICKFRAME));
	
		//
		// Set window boundaries to cover entire desktop, and show it
		//
			
		U32 screenWidth  = GetSystemMetrics(SM_CXSCREEN);
		U32 screenHeight = GetSystemMetrics(SM_CYSCREEN);
			
		if (screenWidth <= width || screenHeight <= height)
		{
			::SetWindowPos(hMainWindow, 
				HWND_TOP, 
				0,
				0,
				width,
				height,
				SWP_NOCOPYBITS | SWP_NOZORDER);
		}
		else
		{
			::SetWindowPos(hMainWindow, 
				HWND_TOP, 
				0,
				0,
				screenWidth,
				screenHeight,
				SWP_NOCOPYBITS | SWP_NOZORDER);
		}

		result = 1;
	}
	else
		result = WindowManager_resizeTheWindow(width, height, flags);


	if (result)
	{
		showWindowFlags = flags;
		if (flags & WMF_SHOW)
			::ShowWindow(hMainWindow, SW_SHOWNORMAL);
	}

	//
	// NOTE: If the program is switching to windowed mode from DDraw full screen,
	// you should invalidate the desktop
	// 
	//if (current_window_mode==0)
	//	InvalidateRect(0, 0, 1);	
	
	return result;
}
//---------------------------------------------------------------------
//
bool verifyMultiTex (void)
{
//	//voodoo cards do not do any of the multitex effects we want to do
//	RPDEVICEINFO rpdi;
//	PIPE->get_device_info(&rpdi);
//	if (rpdi.device_chipset_id >= RP_D_VOODOO_1 && rpdi.device_chipset_id <= RP_D_VOODOO_BANSHEE)
//		return false;
//
//	U32 testTex = TMANAGER->CreateTextureFromFile("cloak.tga",TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
//	CQASSERT(testTex && (testTex != -1));
//	
//	PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
//	PIPE->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
//
//	PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
//	PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
//
//	PIPE->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
//	PIPE->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
//	PIPE->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
//	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
//	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
//	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
//
//	// filtering - bilinear with mips
//	PIPE->set_texture_stage_state( 0, D3DTSS_MINFILTER,		D3DTFN_LINEAR );
//	PIPE->set_texture_stage_state( 0, D3DTSS_MAGFILTER,		D3DTFG_LINEAR );
//
//	PIPE->set_texture_stage_texture( 0, testTex);
//	
//	// addressing - clamped
//	PIPE->set_texture_stage_state( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_WRAP);
//	PIPE->set_texture_stage_state( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_WRAP);
//	
//	PIPE->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
//	
//	PIPE->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_ADD );
//	PIPE->set_texture_stage_state( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
//	PIPE->set_texture_stage_state( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
//	PIPE->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
//	PIPE->set_texture_stage_state( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
//	PIPE->set_texture_stage_state( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );
//
//	// filtering - bilinear with mips
//	PIPE->set_texture_stage_state( 1, D3DTSS_MINFILTER,		D3DTFN_LINEAR );
//	PIPE->set_texture_stage_state( 1, D3DTSS_MAGFILTER,		D3DTFG_LINEAR );
//	
//	PIPE->set_texture_stage_texture(1,testTex);
//	
//	// addressing - clamped
//	PIPE->set_texture_stage_state( 1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP );
//	PIPE->set_texture_stage_state( 1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP );
//	
//	PIPE->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);
//
//	TMANAGER->ReleaseTextureRef(testTex);
//	
//	return (PIPE->verify_state() == GR_OK);

	return true;
}
//---------------------------------------------------------------------
// see if hardware can support colored cursors
//
static void testHardwareCursorSupport (void)
{
//	COMPTR<IDDBackDoor> pBackDoor;
//	COMPTR<IUnknown> pDD1;
//	COMPTR<IDirectDraw4> pDD4;
//	HRESULT hr;
//
//	hr = PIPE->QueryInterface(IID_IDDBackDoor, pBackDoor);
//	if (hr != GR_OK)
//		goto Done;
//	hr = pBackDoor->get_dd_provider(DDBD_P_DIRECTDRAW, pDD1);
//	if (hr != GR_OK)
//		goto Done;
//	hr = pDD1->QueryInterface(IID_IDirectDraw4, pDD4);    
//	if (hr != DD_OK)
//		goto Done;
//
//	DDCAPS caps;
//
//	memset(&caps, 0, sizeof(caps));
//	caps.dwSize = sizeof(caps);
//
//	if (pDD4->GetCaps(&caps, NULL) != DD_OK)
//		goto Done;
//	if ((caps.dwCaps & DDCAPS_OVERLAY) == 0)
//		goto Done;
//	if (caps.dwCurrVisibleOverlays == caps.dwMaxVisibleOverlays)
//		goto Done;		// out of overlays
//	if (caps.dwVidMemTotal < 0xA00000)
//		goto Done;
//
//	CQFLAGS.bUseBWCursors = 0;		// support detected!
//
//Done:
//	return;
}
//---------------------------------------------------------------------
//
void __stdcall Enable3DMode (bool bEnable)
{
	if (bStartupCalledOnce==0 || (CQFLAGS.b3DEnabled!=0) != bEnable)
	{
		iniResX = SCREEN_WIDTH;
		iniResY = SCREEN_HEIGHT;
		bitDepth = 16;

//		if (prevExceptionHandler)
//		{
//			SetUnhandledExceptionFilter(prevExceptionHandler);
//			prevExceptionHandler = 0;
//		}
		
		enableIMC(false);

//		if (DEBUGFONT)
//		{
//			((IDAComponent *)DEBUGFONT)->Release();
//			DEBUGFONT = 0;
//		}
		
		TEXLIB->free_library(true);			// release everything!

//		CQRENDERFLAGS.bSoftwareRenderer = true;
		CQRENDERFLAGS.bSoftwareRenderer = false;

RetryStart:
		if ((CQFLAGS.b3DEnabled = bEnable) != 0)
		{
			bHasBuffers = false;  //at this point we can't make assumptions

//			char regValue[64];
//			DEFAULTS->GetStringFromRegistry(RENDERDEV_REG_KEY, regValue, sizeof(regValue));
//
//			const char * device = (DEFAULTS->GetDefaults()->bHardwareRender)?getRenderSection(regValue):NULL;
//
//			DEFAULTS->GetDefaults()->bHardwareRender = (device!=0);
//			CQRENDERFLAGS.bSoftwareRenderer = (device==0);		// software if NULL device name
			CQRENDERFLAGS.bSoftwareRenderer = false;

//			CQFLAGS.bPrimaryDevice = true;
//			if (device)
//			{
//				GUID guid;
//				if (ConvertStringToGUID(regValue, &guid) != 0)		// rely on NULL return for NULL GUID
//					CQFLAGS.bPrimaryDevice = 0;
//			}

//			GENRESULT result = PIPE->startup(device);			// switch 3D device

			GENRESULT result = PIPE->startup(NULL);			// switch 3D device
			if (result!=GR_OK)
			{
				const char * device = "NULL";

				CQTRACE11("PIPE->startup(\"%s\") failed!", (device)?device:"NULL");
				if (device==0)
				{
					CQBOMB0("Can't Continue");
				}
				else
				{
					result = PIPE->startup(NULL);			// switch back to primary device
					if (result!=GR_OK)
						CQERROR0("Hardware startup failed. Switching to software.");
					CQRENDERFLAGS.bSoftwareRenderer = true;
					CQFLAGS.bPrimaryDevice = 1;
					//DEFAULTS->GetDefaults()->bHardwareRender = 0;
					//iniResX=SCREEN_WIDTH;
					//iniResY=SCREEN_HEIGHT;
				}
			}
		}
		else
		{
			GENRESULT result = PIPE->startup(NULL);			// switch back to primary device
			if (result!=GR_OK)
				CQBOMB0("PIPE->startup(NULL) failed!");

			CQRENDERFLAGS.bSoftwareRenderer = true;
			CQFLAGS.bPrimaryDevice = 1;
		}

		if (bStartupCalledOnce==0)
		{	
			//here???
			VB_MANAGER->initialize(NULL);
			//

		}

		if (CQFLAGS.bFPUExceptions)
		{
			unsigned int cw = _controlfp(0, 0);
			// Set the exception masks OFF, which turns exceptions on.
			cw &= ~(EM_OVERFLOW|EM_ZERODIVIDE|EM_DENORMAL|EM_INVALID);
			_controlfp( cw, MCW_EM );
		}

		bStartupCalledOnce=true;
		PIPE->set_pipeline_state(RP_BUFFERS_FULLSCREEN, CQFLAGS.bFullScreen);
		S32 nextMode = bEnable ? bitDepth:16;
//		if ((CQFLAGS.bWindowModeAllowed = (CQFLAGS.bNoGDI==0 && get_desktop_bpp() == nextMode)) == 0)
//			DEFAULTS->GetDefaults()->bWindowMode=0;
		PIPE->set_pipeline_state(RP_BUFFERS_COLOR_BPP,nextMode);
		PIPE->set_pipeline_state(RP_BUFFERS_DEPTH_BPP,16);
		if (CQFLAGS.bNoGDI)
			PIPE->set_pipeline_state(RP_BUFFERS_HWFLIP,1);
		else
			PIPE->set_pipeline_state(RP_BUFFERS_HWFLIP,0);

		if (CQRENDERFLAGS.bStallPipeline)
			PIPE->set_pipeline_state(RP_BUFFERS_SWAP_STALL,1);


		BATCH->set_state(RPR_BATCH_POOLS, RPR_TRANSLUCENT_DEPTH_SORTED|RPR_TRANSLUCENT_UNSORTED |RPR_OPAQUE);
		BATCH->set_state(RPR_BATCH_TRANSLUCENT_MODE, RPR_TRANSLUCENT_UNSORTED );
		
		
		if (bEnable)
		{
			// get these from the INI file
			SCREENRESX = iniResX;
			SCREENRESY = iniResY;
			CQFLAGS.bFrameLockEnabled = (SCREENRESX == SCREEN_WIDTH) && (SCREENRESY == SCREEN_HEIGHT) && (bitDepth == 16);
		}
		else
		{
			SCREENRESX = 800;// SCREEN_WIDTH;
			SCREENRESY = 600;//SCREEN_HEIGHT;
			CQFLAGS.bFrameLockEnabled = 1;
		}

Retry:
		if (CQFLAGS.bFullScreen==0)
		{
//			if (DEFAULTS->GetDefaults()->iMainWidth)
//			{
//				WM->SetWindowPos(SCREENRESX, SCREENRESY, 0);
//				SetWindowPos(hMainWindow, HWND_TOPMOST,
//								 DEFAULTS->GetDefaults()->iMainX,
//								 DEFAULTS->GetDefaults()->iMainY,
//								 DEFAULTS->GetDefaults()->iMainWidth,
//								 DEFAULTS->GetDefaults()->iMainHeight,
//								 SWP_NOZORDER|SWP_NOSIZE);
//			}
//			else
				WindowManager_SetWindowPos(SCREENRESX, SCREENRESY, WMF_CENTER);
		}
//		else
//		{
//			U32 flag = (CQFLAGS.bNoGDI) ? (WMF_ALWAYS_ON_TOP|WMF_FULL_SCREEN) : WMF_FULL_SCREEN;
//			WM->SetWindowPos(SCREENRESX, SCREENRESY, flag);
//		}

		ShowWindow(hMainWindow, SW_SHOWNORMAL);
		
		U32 hasFSAA=0;

		if (CQRENDERFLAGS.bFSAA)
		{
			PIPE->query_device_ability( RP_A_DEVICE_FULLSCENE_ANTIALIAS, &hasFSAA);
			if (hasFSAA)
			{
				PIPE->set_pipeline_state(RP_BUFFERS_ANTIALIAS,TRUE);
				CQTRACE10("Enabling FSAA");
			}
		}

		GENRESULT gr = PIPE->create_buffers(hMainWindow,SCREENRESX, SCREENRESY);
	//	U32 depth;
	//	PIPE->get_pipeline_state(RP_BUFFERS_DEPTH_BPP,&depth);
		if (gr != GR_OK)
		{
			if (CQFLAGS.bFullScreen==0)
			{
				CQTRACE12("PIPE->create_buffers(%d,%d) failed.",SCREENRESX, SCREENRESY);
				CQFLAGS.bWindowModeAllowed = 0;
				CQFLAGS.bFullScreen = 1;
//				DEFAULTS->GetDefaults()->bWindowMode=0;
				PIPE->set_pipeline_state(RP_BUFFERS_FULLSCREEN, CQFLAGS.bFullScreen);
				goto Retry;
			}

			if (bEnable && iniResX != SCREEN_WIDTH && iniResY != SCREEN_HEIGHT)// && DEFAULTS->GetDefaults()->bHardwareRender)
			{
				CQTRACE10("Failed to create buffers.  Trying 640x480x16.");
				//do a message box here
//				wchar_t name[64];
//				wcsncpy(name, _localLoadStringW(IDS_APP_NAME), sizeof(name)/sizeof(wchar_t));
//				MessageBoxW(hMainWindow, _localLoadStringW(IDS_HELP_TOO_HIRES), name, MB_OK|MB_ICONSTOP);
				
				iniResX=SCREEN_WIDTH;
				iniResY=SCREEN_HEIGHT;
				bitDepth=16;
				goto RetryStart;
			}

			//if (bEnable && DEFAULTS->GetDefaults()->bHardwareRender)
			if( bEnable && CQRENDERFLAGS.bSoftwareRenderer == false )
			{
				CQTRACE10("Hardware startup failed. Switching to software.");
				CQRENDERFLAGS.bSoftwareRenderer = true;
//				DEFAULTS->GetDefaults()->bHardwareRender = 0;

//				//do a message box here
//				wchar_t name[64];
//				wcsncpy(name, _localLoadStringW(IDS_APP_NAME), sizeof(name)/sizeof(wchar_t));
//				MessageBoxW(hMainWindow, _localLoadStringW(IDS_HELP_DEFAULT_TO_SOFTWARE), name, MB_OK|MB_ICONSTOP);

				iniResX=SCREEN_WIDTH;
				iniResY=SCREEN_HEIGHT;
				goto RetryStart;
			}

//			wchar_t name[64];
//			wcsncpy(name, _localLoadStringW(IDS_APP_NAME), sizeof(name)/sizeof(wchar_t));
//			MessageBoxW(hMainWindow, _localLoadStringW(IDS_HELP_BAD_HARDWARE), name, MB_OK|MB_ICONSTOP);

			PostQuitMessage(-1);
//			if (WM)
//				WM->ServeMessageQueue();

			return;
		}

		bHasBuffers = true;

		// force window to be on top
		if (CQFLAGS.bNoGDI)
			SetWindowPos(hMainWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSIZE);

//		CQASSERT(prevExceptionHandler==0);
//		prevExceptionHandler = SetUnhandledExceptionFilter(cqExceptionHandler);

		if (bEnable)
		{
//			CQBATCH->Startup();
			BATCH->set_sampler_state( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
			BATCH->set_sampler_state( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
			BATCH->set_sampler_state( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
			BATCH->set_sampler_state( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
			
			bool bNoMips=0;
			U32 yes;
			PIPE->query_device_ability( RP_A_TEXTURE_LOD, &yes);
			if (yes)
			{
				TEXLIB->set_library_state( ITL_STATE_TEXTURE_LOD_LOAD, TRUE );
				SINGLE dummy = 1.0f;
				TEXLIB->set_library_state( ITL_STATE_TEXTURE_LOD_LOAD_SCALE, *((U32 *)&dummy));

				// enable bilinear mip.
				BATCH->set_sampler_state( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
				BATCH->set_sampler_state( 1, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
			}
			else
			{
				TEXLIB->set_library_state( ITL_STATE_TEXTURE_LOD_LOAD, TRUE );
				SINGLE scale=0.0f;
				if (CQRENDERFLAGS.bSoftwareRenderer)
					scale = 1.0f;//0.7f;
				
				TEXLIB->set_library_state( ITL_STATE_TEXTURE_LOD_LOAD_SCALE, *(U32 *)&scale);
				PIPE->set_pipeline_state(RP_TEXTURE_LOD,0);
				BATCH->set_sampler_state( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
				BATCH->set_sampler_state( 1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
				bNoMips = true;
			}
			
			CQRENDERFLAGS.bNoPerVertexAlpha = true;
			PIPE->query_device_ability( RP_A_ALPHA_ITERATED, &yes);
			if (yes)
				CQRENDERFLAGS.bNoPerVertexAlpha = false;
			
			CQRENDERFLAGS.bMultiTexture = false;
			U32 tex;
			PIPE->query_device_ability( RP_A_TEXTURE_SIMULTANEOUS, &tex);
			if (tex > 1)
			{
				PIPE->query_device_ability( RP_A_TEXTURE_COORDINATES, &tex);
				if (tex > 1 && verifyMultiTex())
					CQRENDERFLAGS.bMultiTexture = true;
			}
			
			CQRENDERFLAGS.bHardwareGeometry = false;
			PIPE->query_device_ability( RP_A_DEVICE_GEOMETRY, &tex);
			if (tex && CQFLAGS.bHardwareGeometry)
				CQRENDERFLAGS.bHardwareGeometry = true;
			
			SINGLE bias=0.0f;
			TEXLOD = TL_LOW;
			PIPE->query_device_ability( RP_A_DEVICE_MEMORY, &tex);
			if (tex > (8*1024*1024))
			{
				bias = -1.0f;
				TEXLOD = TL_MEDIUM;
				if (tex > (16*1024*1024))
				{
					bias = 0.0f;
					TEXLOD = TL_HIGH;
				}
			}
			
			MEMORYSTATUS memoryStatus;
			GlobalMemoryStatus(&memoryStatus);
			
//			//force software options
//			if (CQRENDERFLAGS.bSoftwareRenderer)
//			{
//				CQEFFECTS.bExpensiveTerrain = GlobalEffectsOptions::OPTVAL(0);
//				TEXLOD = TL_LOW;
//			}
			
			switch (TEXLOD)
			{
			case TL_ULTRA_LOW:
				bias = -3.0f;
				break;
			case TL_LOW:
				bias = -2.0f;
				break;
			case TL_MEDIUM:
				bias = -1.0f;
				break;
			case TL_HIGH:
				bias = 0.0f;
				break;
			}
			
			if (bNoMips) //humoring the algorithm in TEXTURELIBRARY
			{
				if (CQRENDERFLAGS.bSoftwareRenderer)
					bias = 0.0f;
				else
					bias = 0.6f;
			}
			
			TEXLIB->set_library_state(ITL_STATE_TEXTURE_LOD_LOAD_BIAS,*((U32 *)&bias));
			
//			if (CQRENDERFLAGS.bMultiTexture==false)
//			{
//				CQEFFECTS.bEmissiveTextures = GlobalEffectsOptions::OPTVAL(0);
//			}
//			if (CQEFFECTS.bExpensiveTerrain==0)
//			{
//				CQEFFECTS.bBackground = GlobalEffectsOptions::OPTVAL(0);
//				CQEFFECTS.bHighBackground = GlobalEffectsOptions::OPTVAL(0);
//			}
			
//			if (hasFSAA)
//				PIPE->set_render_state(D3DRS_ANTIALIAS,TRUE);
			
//			U32 modes[4];
//			
//			PIPE->query_device_ability( RP_A_BLEND_MATRIX, &modes[0]);
//			if( !rp_a_is_blend_supported( D3DBLEND_ONE, D3DBLEND_ONE, modes ) )
//			{
//				CQEFFECTS.bExpensiveTerrain = GlobalEffectsOptions::OPTVAL(0);
//			}
		}
//		else
//			CQBATCH->Shutdown();
		
//		if (CQFLAGS.bFrameLockEnabled)
//		{
//			if (SURFACE->Lock())	// test to see if locking is supported
//				SURFACE->Unlock();
//			else
//				CQBOMB0("Frame Lock attempt failed.");
//		}
		
		BATCH->set_state(RPR_BATCH,0);
		BATCH->set_state(RPR_BATCH_TRANSLUCENT_POOL,262144);
		BATCH->set_state(RPR_BATCH_TRANSLUCENT_NONZ_POOL,1024*1024);
		PIPE->set_pipeline_state(RP_CLEAR_COLOR,0xff000000);  // a,r,g,b
		PIPE->set_render_state(D3DRS_ZENABLE,TRUE);
		
//		static bool bFirstTime = true;
//		if (bFirstTime && DEFAULTS->GetDefaults()->bWindowMode == false)
//		{
//			set_gamma(true);
//			bFirstTime = false;
//		}
//		else
//		{
//			set_gamma(!DEFAULTS->GetDefaults()->bWindowMode);
//		}
		
		BATCH->set_state(RPR_BATCH,1);
		
//		init_debug_font();
		
//		EVENTSYS->Send(CQE_ENABLE3DMODE, (void*)bEnable);
		
//		CQFLAGS.bNoToolbar = CQFLAGS.bMovieMode = 0;		// turn this back on by default
		
//		if (bEnable==0 && TMANAGER)
//		{
//			TMANAGER->Flush();
//		}
		
//		CQFLAGS.bUseBWCursors = 1;
//		if (bEnable)
//		{
//			if (bForceMonochromeOff)
//				CQFLAGS.bUseBWCursors = 0;
//			else
//				if (bForceMonochromeOn)
//					CQFLAGS.bUseBWCursors = 1;
//				else
//					testHardwareCursorSupport();
//		}
		
		if (bEnable && CQFLAGS.bNoGDI)
			SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
		else
			SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	}
}
//---------------------------------------------------------------------
//
void __stdcall SetupDiffuseBlend( U32 irp_texture_id, bool bClamp )
{
 	BATCH->set_texture_stage_texture( 0, irp_texture_id );
	
	// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
	BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

//	// filtering - bilinear with mips
//	BATCH->set_texture_stage_state( 0, D3DSAMP_MINFILTER, D3DTFN_LINEAR );
//	BATCH->set_texture_stage_state( 0, D3DSAMP_MAGFILTER, D3DTFG_LINEAR );
//	BATCH->set_texture_stage_state( 0, D3DSAMP_MIPFILTER, D3DTFG_POINT );

	if (bClamp)
	{
		// addressing - clamped
		BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
		BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
	}
	else
	{
		BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
		BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
	}


	BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
	BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	BATCH->set_texture_stage_texture(1,0);
}

//--------------------------------------------------------------------------//
//
static U32 bitmaskR,  bitmaskG,  bitmaskB;
static U32 bitshiftR, bitshiftG, bitshiftB;
static U32 bitwidthR, bitwidthG, bitwidthB;

//---------------------------------------------------------------------
//												 
U32 __fastcall ColorRefToPixel (COLORREF colorref)
{
	U32 result;

	result = (((colorref & 0xFF)     >> (8-bitwidthR))  << bitshiftR) |
			 (((colorref & 0xFF00)   >> (16-bitwidthG)) << bitshiftG) |
			 (((colorref & 0xFF0000) >> (24-bitwidthB)) << bitshiftB);

	return result;
}
