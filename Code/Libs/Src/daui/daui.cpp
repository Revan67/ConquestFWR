//
// daui.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include <dacom.h>
#include <rendpipeline.h>
#include <windowmanager.h>
#include <system.h>
#include <eventsys.h>
#include <fdump.h>
#include <streamer.h>
#include <iprofileparser.h>
#include <ilua.h>
#include <tempstr.h>
#include <tsmartpointer.h>

#include <stdio.h>

#include "dauievent.h"

#include <movieplay.h>

//
// Compile switches
//

#define TEST_MOVIE 0
#define MOVIE_FULLSCREEN 0
#define TEST_LUA 0

//
// Global variables
//

ICOManager *        DACOM = NULL;
ISystemContainer *  SYSTEM = NULL;
IRenderPipeline *   PIPE = NULL;
IWindowManager *    WINMGR = NULL;
IEventSystem *      EVT = NULL;
IStreamer *         STREAMER = NULL;

void dastuff_close()
{
	if (STREAMER)
	{
		STREAMER->Release();
		STREAMER = NULL;
	}

	if (PIPE)
	{
		PIPE->shutdown();
		PIPE->Release();
		PIPE = NULL;
	}

	if (SYSTEM)
	{
		SYSTEM->Release();
		SYSTEM = NULL;
	}
	
	if (DACOM)
	{
		DACOM->ShutDown();
		DACOM->Release();
		DACOM = NULL;
	}
}

bool dastuff_open(const char *ini_filename)
{
	if ((DACOM = DACOM_Acquire()) == 0)
	{
		fprintf(stdout, "DACOM startup failed!\n");
		return false;
	}

	atexit(dastuff_close);

	if (ini_filename)
	{
		DACOM->SetINIConfig(ini_filename);
	}

	// Create the system and engine components first
	AGGDESC adesc = "ISystemContainer";
	
	if (DACOM->CreateInstance(&adesc, (void **) &SYSTEM) != GR_OK)
	{
		return false;
	}

	SYSTEM->LoadSystemComponents();

	// Query the interfaces we need and confirm that they are there
	SYSTEM->QueryInterface(IID_IRenderPipeline,	(void **) &PIPE);
	SYSTEM->QueryInterface(IID_IWindowManager,	(void **) &WINMGR);
	SYSTEM->QueryInterface(IID_IEventSystem,	(void **) &EVT);
	SYSTEM->QueryInterface(IID_IStreamer,	    (void **) &STREAMER);


	if (!PIPE || !WINMGR || !EVT)
	{
		return false;
	}

	// Get the render pipeline interface and start it up on the default device, but defer the
	// creation of buffers until later.

	if (PIPE->startup() != GR_OK)
	{
		return false;
	}
	else
	{
		// *** TODO: Get the state information from the initialization file.
		PIPE->set_pipeline_state(RP_BUFFERS_COLOR_BPP, 16);
		PIPE->set_pipeline_state(RP_BUFFERS_DEPTH_BPP, 16);
		PIPE->set_pipeline_state(RP_BUFFERS_COUNT, 2);
		PIPE->set_pipeline_state(RP_BUFFERS_FULLSCREEN, false);
	}

	// All is well, so return success.
	return true;
}

static bool exitMovie = false;

static long FAR PASCAL MainWndproc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch( message )
    {
    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

	case WM_MOUSEMOVE:
		{
			// WARNING: Don't use post() here, because the given param pointer will be invalid
			// after this block exits.

			MousePos pos;
			pos.x = LOWORD(lParam);
			pos.y = HIWORD(lParam);

			if (EVT)
			{
				EVT->Send(SYS_MOUSE_MOVE, &pos);
			}

			// *** Should we return here, since we processed the message?
		}
		break;

	case WM_LBUTTONDOWN:
		{
			// WARNING: Don't use post() here, because the given param pointer will be invalid
			// after this block exits.

			MousePos pos;
			pos.x = LOWORD(lParam);
			pos.y = HIWORD(lParam);

			if (EVT)
			{
				EVT->Send(SYS_MOUSE_LEFT_DOWN, &pos);
			}

			// *** Should we return here, since we processed the message?
		}
		break;

	case WM_LBUTTONUP:
		{
			// WARNING: Don't use post() here, because the given param pointer will be invalid
			// after this block exits.

			MousePos pos;
			pos.x = LOWORD(lParam);
			pos.y = HIWORD(lParam);

			if (EVT)
			{
				EVT->Send(SYS_MOUSE_LEFT_UP, &pos);
			}

			// *** Should we return here, since we processed the message?
		}
		break;

	case WM_RBUTTONDOWN:
		break;

	case WM_KEYDOWN:
		{
			int nVirtKey = (int) wParam;
			long lKeyData = (long) lParam;
			if (nVirtKey == VK_ESCAPE)
			{
				exitMovie = true;
			}
		}
		break;

    default:
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

bool movieCallback (RECT *destRect)
{
#if 0
	WINMGR->ServeMessageQueue (WMF_BACKGROUND);

#if !MOVIE_FULLSCREEN
	if (destRect != NULL)
	{
		HWND hMainWindow = WINMGR->GetWindowHandle();
		RECT r;
		GetClientRect (hMainWindow, &r);
		POINT tl = {r.left, r.top};
		POINT br = {r.right, r.bottom};

		ClientToScreen (hMainWindow, &tl);
		ClientToScreen (hMainWindow, &br);

		r.top = tl.y;
		r.left = tl.x;
		r.bottom = br.y;
		r.right = br.x;

		*destRect = r;
	}
#endif
#endif
	return !exitMovie;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	// Initialize MSCOM before proceeding.
	// *** NOTE: We are supposed to call CoInitializeEx(), but it is not available unless we are on NT or
	// *** using DCOM.
//	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	CoInitialize(NULL);

	// Initialize the system and get the window manager and pipeline interfaces

	if (!dastuff_open ("daui.ini"))
	{
		return 0;
	}

	// Initialize the window manager.

	if (!WINMGR->Startup(hInstance, "DAUI Testbed", exit))
	{
		return 0;
	}

	// Show the window.

	WINMGR->SetWindowPos (640, 480, WMF_SHOW);

	WM_WINAREA area;
	WINMGR->GetClientArea (area);

	// Set the window's callback

	WINMGR->SetCallback ((WNDPROC) MainWndproc);

	// Initialize the render pipeline from the window manager.

#if TEST_MOVIE && MOVIE_FULLSCREEN
	PIPE->set_pipeline_state(RP_BUFFERS_FULLSCREEN, TRUE);
#endif

	if (PIPE->create_buffers (WINMGR->GetWindowHandle(), area.w, area.h) != GR_OK)
	{
		return 0;	
	}

	PIPE->set_render_state (D3DRS_ALPHABLENDENABLE, TRUE);
//	PIPE->set_render_state (D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	PIPE->set_render_state (D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	// Initialize the test code
	extern bool daui_startup ();
	if (!daui_startup())
	{
		PIPE->shutdown ();
		return 0;
	}

	const char *testMovie = "d:\\WC_Trailer_v3.avi";
//	const char *testMovie = "c:\\windows\\desktop\\fromdusktilldawn_01.avi";
//	const char *testMovie = "testmovie.avi";
#if TEST_MOVIE
	// HACK: Play a movie
	exitMovie = false;

#if MOVIE_FULLSCREEN
	PlayMovie (PIPE, testMovie, NULL, (PlayMovieCallback *) movieCallback);
#else
	{
		HWND hMainWindow = WINMGR->GetWindowHandle();
		RECT r;
		GetClientRect (hMainWindow, &r);
		POINT tl = {r.left, r.top};
		POINT br = {r.right, r.bottom};

		ClientToScreen (hMainWindow, &tl);
		ClientToScreen (hMainWindow, &br);

		r.top = tl.y;
		r.left = tl.x;
		r.bottom = br.y;
		r.right = br.x;

		PlayMovie (PIPE, testMovie, &r, (PlayMovieCallback *) movieCallback);
	}
#endif
#endif

	// HACK: Test dacomtest
	{
		extern bool test_dacom();
		extern void dacomtest_startup();

		dacomtest_startup();
		if (test_dacom())
		{
			GENERAL_TRACE_1("DACOM is working!\n");
		}
		else
		{
			GENERAL_TRACE_2("DACOM is not working!\n");
		}
	}

#if TEST_LUA
	{
		COMPTR<ILua> lua;
		if (SYSTEM->QueryInterface(IID_ILua,lua) == GR_OK)
		{
			COMPTR<IProfileParser> profile;
			if (lua->QueryInterface (IID_IProfileParser, profile) == GR_OK)
			{
				if (profile->Initialize ("luatest.lua") == GR_OK)
				{
					HANDLE global = profile->CreateSection ("$");
					if (global != NULL)
					{
						char buffer[1024];
						if (profile->ReadKeyValue (global, "TestKey", buffer, sizeof(buffer)) != 0)
						{
							GENERAL_TRACE_1 (TEMPSTR("TestKey == \"%s\"\n", buffer));
						}
						else
						{
							GENERAL_WARNING ("Failed to get test key.\n");
						}

						if (!profile->CloseSection (global))
						{
							GENERAL_WARNING ("Failed to close the global section.\n");
						}
					}
					else
					{
						GENERAL_WARNING ("Failed to create global section.\n");
					}

					global = profile->CreateSection ("TestSection");
					if (global != NULL)
					{
						char buffer[1024];
						if (profile->ReadKeyValue (global, "audio", buffer, sizeof(buffer)) != 0)
						{
							GENERAL_TRACE_1 (TEMPSTR("audio == \"%s\"\n", buffer));
						}
						else
						{
							GENERAL_WARNING ("Failed to get test section key.\n");
						}

						int line = 0;
						while (profile->ReadProfileLine(global, line, buffer, sizeof(buffer)) != 0)
						{
							GENERAL_TRACE_1 (TEMPSTR("Line %d - \"%s\"\n", line, buffer));
							++line;
						}

						if (!profile->CloseSection (global))
						{
							GENERAL_WARNING ("Failed to close the test section.\n");
						}
					}
					else
					{
						GENERAL_WARNING ("Failed to create test section.\n");
					}

					if (lua->CallFunction ("TestFunc", NULL, 0, NULL) != GR_OK)
					{
						GENERAL_WARNING ("Failed to call the test function.\n");
					}

					DACOM_VARIANT result;
					if (lua->CallFunction ("TestFunc2", &result, 0, NULL) == GR_OK)
					{
						GENERAL_TRACE_1 (TEMPSTR("TestFunc2 returned: \"%s\"\n", (const C8*) result));
					}
					else
					{
						GENERAL_WARNING ("Failed to call the second test function.\n");
					}

					DACOM_VARIANT name("Hillary");
					DACOM_VARIANT age(40);
					DACOM_VARIANT *params[2];
					params[0] = &name;
					params[1] = &age;

					if (lua->CallFunction ("TestFunc3", &result, 2, params) == GR_OK)
					{
						GENERAL_TRACE_1 (TEMPSTR("TestFunc3 returned: \"%s\"\n", (const C8*) result));
					}
					else
					{
						GENERAL_WARNING ("Failed to call the third test function.\n");
					}
				}
				else
				{
					GENERAL_WARNING ("Failed to load lua file.\n");
				}
			}
			else
			{
				GENERAL_WARNING ("Failed to query IProfileParser.\n");
			}
		}
		else
		{
			GENERAL_WARNING ("Failed to query ILua.\n");
		}
	}
#endif

	// Spin in a loop until the window is destroyed.

	while (true)
	{
		WINMGR->ServeMessageQueue (WMF_WAIT);
//		WINMGR->ServeMessageQueue ();
		extern void daui_doframe ();
		daui_doframe ();
		Sleep (1);
	}

	return 0;
}



