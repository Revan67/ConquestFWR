//
// DAStuff.cpp - Module for importing DACOM functionality into UniTool
//

//
// Include files
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <dacom.h>
#include <tsmartpointer.h>
#include <3DMath.h>
#include <system.h>
#include <engine.h>
#include <LightMan.h>
#include <IAnim.h>
#include <rendpipeline.h>
#include <renderer.h>
#include <iddbackdoor.h>
#include <timer.h>
#include <IProperties.h>
#include <streamer.h>
#include <ISoundManager.h>
#include <fontimage.h>
#include <irenderprimitive.h>
#include <itexturelibrary.h>

#include "script.h"
#include "unitool.h"

//
// Local variables
//

static bool deform_openned = false;

//
// Global variables
//

ICOManager *        DACOM = NULL;
ISystemContainer *  SYSTEM = NULL;
IEngine *			ENGINE = NULL;
IRenderPipeline *   PIPE = NULL;
IAnimation *		ANIM = NULL;
ILightManager *		LIGHT = NULL;
ITextureLibrary *   TLIB = NULL;
IChannel*			CHANNEL = NULL;
IRenderer*			RENDERER = NULL;
IProperties*        PROPERTIES = NULL;
IStreamer *         STREAMER = NULL;
ISoundManager *     SOUND = NULL;
IRenderPrimitive *  PRIM = NULL;

SINGLE              frameTime = 0;
//
// Forward declarations
//

bool get_gamma (float &gamma);
bool set_gamma (float newGamma);

//
// Local routines
//

// Gamma control methods, also exposed to the scripting language
static void setGamma (void)
{
	// Syntax: 
	//     SetGamma
	//     (
	//         number <gamma>,
	//     )
	// <gamma> is the gamma value, valid from 1.0 to 500.0

	// Get and validate the parameters
	lua_Object gVal = lua_getparam(1);

	if (!lua_isnumber(gVal))
	{
		return;
	}

	float gamma = lua_getnumber(gVal);
	set_gamma (gamma);
}

static void getGamma (void)
{
	// Syntax: 
	//     GetGamma()

	float gamma;
	if (get_gamma (gamma))
	{
		lua_pushnumber (gamma);
	}
	else
	{
		// Return nil, indicating an error.
		lua_pushnil();
	}
}

// Streamer methods exposed to the scripting language.
static int streamTag = LUA_ANYTAG;
static void startStream()
{
	if (!STREAMER)
	{
		return;
	}

	lua_Object fName = lua_getparam(1);
	lua_Object oLoop = lua_getparam(2);

	if (!lua_isstring(fName))
	{
		return;
	}

	bool looping = false;
	if (oLoop != LUA_NOOBJECT)
	{
		if (!lua_isnil (oLoop))
		{
			looping = true;
		}
	}

	// Create the stream using a parent file system.
	// NOTE: Only works for the DOS file system.
	COMPTR<IFileSystem> fs;
	DAFILEDESC fdesc = lua_getstring (fName);

	if (DACOM->CreateInstance (&fdesc, fs) != GR_OK)
	{
		// Failed, so return failure.
		return;
	}

	HSTREAM hStream = STREAMER->Open (NULL, fs, STRMFL_PLAY | (looping ? STRMFL_LOOPING : 0));

	if (hStream)
	{
		// Return the stream handle.
		if (streamTag == LUA_ANYTAG)
		{
			streamTag = lua_newtag();
		}
		lua_pushusertag (hStream, streamTag);
	}
	else
	{
		// Return nil
		lua_pushnil();
		return;
	}
}

static void startRelativeStream()
{
	if (!STREAMER)
	{
		return;
	}

	lua_Object fName = lua_getparam(1);
	lua_Object oDirName = lua_getparam(2);
	lua_Object oLoop = lua_getparam(3);

	if (!lua_isstring(fName) || !lua_isstring(oDirName))
	{
		return;
	}

	bool looping = false;
	if (oLoop != LUA_NOOBJECT)
	{
		if (!lua_isnil (oLoop))
		{
			looping = true;
		}
	}

	// Create a file system for the directory.
	COMPTR<IFileSystem> fs;
	DAFILEDESC fdesc = lua_getstring (oDirName);

	if (DACOM->CreateInstance (&fdesc, fs) != GR_OK)
	{
		// Failed, so return failure.
		return;
	}

	HSTREAM hStream = STREAMER->Open (lua_getstring(fName), fs, STRMFL_PLAY | (looping ? STRMFL_LOOPING : 0));

	if (hStream)
	{
		// Return the stream handle.
		if (streamTag == LUA_ANYTAG)
		{
			streamTag = lua_newtag();
		}
		lua_pushusertag (hStream, streamTag);
	}
	else
	{
		// Return nil
		lua_pushnil();
		return;
	}
}

static void stopStream ()
{
	if (!STREAMER)
	{
		return;
	}

	lua_Object oStream = lua_getparam(1);

	if (!lua_isuserdata(oStream))
	{
		return;
	}

	// Return the stream handle.
	if (streamTag == LUA_ANYTAG)
	{
		streamTag = lua_newtag();
	}

	if (lua_tag (oStream) != streamTag)
	{
		return;
	}

	// This is a stream, so retrieve its value.
	HSTREAM hStream = (HSTREAM) lua_getuserdata (oStream);

	if (hStream)
	{
		STREAMER->Stop (hStream);
		STREAMER->CloseHandle (hStream);
	}
}

static void setStreamVolume ()
{
	if (!STREAMER)
	{
		return;
	}

	lua_Object oStream = lua_getparam(1);
	lua_Object oVol = lua_getparam(2);

	if (!lua_isuserdata(oStream))
	{
		return;
	}

	if (!lua_isnumber (oVol))
	{
		return;
	}

	// Return the stream handle.
	if (streamTag == LUA_ANYTAG)
	{
		streamTag = lua_newtag();
	}

	if (lua_tag (oStream) != streamTag)
	{
		return;
	}

	// This is a stream, so set its volume
	HSTREAM hStream = (HSTREAM) lua_getuserdata (oStream);
	S32 vol = lua_getnumber (oVol);

	if (hStream)
	{
		STREAMER->SetVolume (hStream, vol);
	}
}

static void getStreamVolume ()
{
	if (!STREAMER)
	{
		return;
	}

	lua_Object oStream = lua_getparam(1);

	if (!lua_isuserdata(oStream))
	{
		return;
	}

	// Return the stream handle.
	if (streamTag == LUA_ANYTAG)
	{
		streamTag = lua_newtag();
	}

	if (lua_tag (oStream) != streamTag)
	{
		return;
	}

	// This is a stream, so set its volume
	HSTREAM hStream = (HSTREAM) lua_getuserdata (oStream);

	if (hStream)
	{
		S32 vol;
		if (STREAMER->GetVolume (hStream, &vol))
		{
			lua_pushnumber (vol);
		}
	}
}

// Font factory routines exposed to scripting
int fontTag = LUA_ANYTAG;

static void createFont ()
{
	lua_Object faceNameObj = lua_getparam(1);
	lua_Object pointSizeObj = lua_getparam(2);

	if (!lua_isstring (faceNameObj) || !lua_isnumber (pointSizeObj))
	{
		return;
	}

	FONTFACTORYDESC ffdesc (lua_getstring (faceNameObj), lua_getnumber (pointSizeObj));

	IFontFactory *font;

	if (DACOM->CreateInstance (&ffdesc, (void **) &font) == GR_OK)
	{
		lua_pushusertag (font, fontTag);
	}
}

static void destroyFont ()
{
	lua_Object fontObj = lua_getparam(1);
	
	if (!lua_isuserdata (fontObj) || lua_tag (fontObj) != fontTag)
	{
		return;
	}

	IFontFactory *font = (IFontFactory *) lua_getuserdata (fontObj);
	font->Release ();
}

//
// Global routines
//

void dastuff_close()
{
	extern void destroy_renderwindow();  // in renderwin.cpp
	destroy_renderwindow ();

	extern void destroy_videowindow();  // in vidwin.cpp
	destroy_videowindow ();

#if 0
	if (deform_openned)
	{
		DeformClose();
	}
#endif

	if(PRIM)
	{
		PRIM->Release();
		PRIM = NULL;
	}

	if(SOUND)
	{
		SOUND->Release();
		SOUND = NULL;
	}

	if(LIGHT)
	{
		LIGHT->Release();
		LIGHT = NULL;
	}
	
	if(ANIM)
	{
		ANIM->Release();
		ANIM = NULL;
	}

	if(TLIB)
	{
		TLIB->Release();
		TLIB = NULL;
	}
	
	if(CHANNEL)
	{
		CHANNEL->Release();
		CHANNEL = NULL;
	}

	if(PROPERTIES)
	{
		PROPERTIES->Release();
		PROPERTIES = NULL;
	}

	if(STREAMER)
	{
		STREAMER->Release();
		STREAMER = NULL;
	}

	if(RENDERER)
	{
		RENDERER->Release();
		RENDERER = NULL;
	}

	if (PIPE)
	{
		PIPE->shutdown();
		PIPE->Release();
		PIPE = NULL;
	}
	if (ENGINE)
	{
		ENGINE->Release();
		ENGINE = NULL;
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

	Timer t;
	if (ini_filename)
	{
		t.begin();
		DACOM->SetINIConfig(ini_filename);
		t.end ();
		printf ("DACOM Library load time: %f\n", t.deltaSecs());
	}

	// *** TODO: Initialize and register with lua the various global functions and variables.
	// Create the system and engine components first
	AGGDESC adesc = "ISystemContainer";
	
	t.begin();
	if (DACOM->CreateInstance(&adesc, (void **) &SYSTEM) != GR_OK)
	{
		return false;
	}
	t.end ();
	printf ("DACOM System creation time: %f\n", t.deltaSecs());

	SYSTEM->LoadSystemComponents();

#if 1
	SYSTEM->QueryInterface("IStreamer",	(void **) &STREAMER);
#else
	{
		AGGDESC adesc = "IStreamer";
		if (DACOM->CreateInstance(&adesc, (void **) &STREAMER) != GR_OK)
		{
			return false;
		}
	}
#endif

	DACOMDESC desc = "IEngine";

	t.begin();
	if (DACOM->CreateInstance(&desc, (void **) &ENGINE) != GR_OK)
	{
		return false;
	}
	ENGINE->load_engine_components(SYSTEM);
	t.end ();
	printf ("DACOM Engine creation and component load time: %f\n", t.deltaSecs());

	// Query the interfaces we need and confirm that they are there
	SYSTEM->QueryInterface(IID_IRenderPipeline, (void **) &PIPE);
	SYSTEM->QueryInterface(IID_ISoundManager,	(void **) &SOUND);
	SYSTEM->QueryInterface(IID_IRenderPrimitive,(void **) &PRIM);
	SYSTEM->QueryInterface(IID_ITextureLibrary, (void **) &TLIB);
	SYSTEM->QueryInterface(IID_ILightManager,   (void **) &LIGHT);

	ENGINE->QueryInterface(IID_IAnimation,      (void **) &ANIM);
	ENGINE->QueryInterface(IID_IChannel,        (void **) &CHANNEL);
	ENGINE->QueryInterface(IID_IRenderer,       (void **) &RENDERER);
	ENGINE->QueryInterface(IID_IProperties,     (void **) &PROPERTIES);

	if (!ANIM || !LIGHT || !TLIB || !CHANNEL || !RENDERER || !PIPE || !PROPERTIES || !STREAMER || !PRIM)
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

	// Start up the sound system
	if (SOUND && SOUND->startup (hWndMain, SM_DEFAULT_SETTINGS) != GR_OK)
	{
		return false;
	}

	// Open the deformable library.
#if 0
	deform_openned = DeformOpen(SYSTEM, ENGINE);
	if (!deform_openned)
	{
		return false;
	}
#endif

	// For now, set the ambient light to a high value so we can see things.
	LIGHT->set_ambient_light (255, 255, 255);

	// We are all happy and initialized. Perform the rest of the installation.

	// Export the RenderWindow functionality.
	extern bool init_renderwindow(); // in renderwin.cpp
	if (!init_renderwindow ())
	{
		return false;
	}

	// Export the particle functionality
	extern bool init_particles(); // in psys.cpp
	if (!init_particles ())
	{
		return false;
	}

	// Initialize scripting stuff.
	lua_register ("GetGamma", getGamma);
	lua_register ("SetGamma", setGamma);
	lua_register ("StartStream", startStream);
	lua_register ("StartRelativeStream", startRelativeStream);
	lua_register ("StopStream", stopStream);
	lua_register ("SetStreamVolume", setStreamVolume);
	lua_register ("GetStreamVolume", getStreamVolume);

	fontTag = lua_newtag();
	lua_register ("CreateFont", createFont);
	lua_register ("DestroyFont", destroyFont);

	// Set exclusive cooperative level
	{
		COMPTR<IDDBackDoor>  bd;
		COMPTR<IUnknown>     pdd;
		COMPTR<IDirectDraw4> pDD4;

		if (PIPE->QueryInterface(IID_IDDBackDoor, bd) != GR_OK)
		{
			GENERAL_WARNING("Failed to query backdoor interface from the render pipeline.");
			goto exclusive_done;
		}

		if (bd->get_dd_provider (DDBD_P_DIRECTDRAW, pdd) != GR_OK)
		{
			GENERAL_WARNING("Failed to get direct draw object via back door.");
			goto exclusive_done;
		}

		if (pdd->QueryInterface(IID_IDirectDraw4, pDD4) != DD_OK)
		{
			GENERAL_WARNING("Couldn't get IDirectDraw4");
			goto exclusive_done;
		}

		extern HWND hWndMain; // in UniTool.cpp
		pDD4->SetCooperativeLevel (hWndMain,	DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_NOWINDOWCHANGES);

exclusive_done:
		int a = 1;
	}

	// Initialize the streamer.
	if (SOUND)
	{
		STREAMERDESC sDesc;
		sDesc.hMainWindow = hWndMain;
		if (SOUND->get_directsound_interface ((void **) &sDesc.lpDSound) != GR_OK)
		{
			GENERAL_WARNING ("Failed to get the DirectSound interface from the sound manager.\n");
			STREAMER->Release ();
			STREAMER = NULL;
		}
		else
		{
			if (!STREAMER->Init (&sDesc))
			{
				GENERAL_WARNING("Failed to initialize the streamer.\n");
				STREAMER->Release ();
				STREAMER = NULL;
			}
		}
	}

	// All is well, so return success.
	return true;
}

bool dastuff_update ()
{
	// Perform the normal, once-per-frame update.
	SINGLE now = (SINGLE) timeGetTime() / 1000.0;
	SINGLE dt = now - frameTime;
	frameTime = now;

	if (dt > 1.0f/30.0f)
	{
		// printf ("Delta time too large. (dt == %f)\n", dt);
		dt = 1.0f/30.0f;
	}

	if (dt < 0.0)
	{
		// Adjust for updates less than a msec.
		dt = 0.0f;
	}

	// First, update the engine with the current delta time.
#if 0
	if (ENGINE)
	{
		ENGINE->update (dt);
	}
#endif

	// Now, update the render window.
	extern void update_renderwindow(SINGLE dt); // in renderwin.cpp
	update_renderwindow (dt);

	// Update the video window.
	extern void update_videowindow(void); // in vidwin.cpp
	update_videowindow();

	// Return true, to keep the main loop running.
	return true;
}

bool get_gamma (float &gamma)
{
	if (PIPE)
	{
		COMPTR<IDDBackDoor>             bd;
		COMPTR<IUnknown>                pprim;
		COMPTR<IDirectDrawColorControl> pcc;
		COMPTR<IDirectDrawGammaControl> pgc;

		if (PIPE->QueryInterface(IID_IDDBackDoor, bd) != GR_OK)
		{
			GENERAL_ERROR("Failed to query backdoor interface from the render pipeline.");
			return false;
		}

		if (bd->get_dd_provider (DDBD_P_PRIMARYSURFACE, pprim) != GR_OK)
		{
			GENERAL_ERROR("Failed to get primary surface object via back door.");
			return false;
		}

		if (pprim->QueryInterface (IID_IDirectDrawColorControl, pcc) != S_OK)
		{
			// Get the current gamma value

			DDCOLORCONTROL cc;
			memset (&cc, 0, sizeof(cc));
			cc.dwSize = sizeof(cc);
			if (pcc->GetColorControls (&cc) != DD_OK)
			{
				GENERAL_ERROR ("Failed to get the color control values.");
				return false;
			}

			if (cc.dwFlags & DDCOLOR_GAMMA)
			{
				gamma = (float) (cc.lGamma-1) / 100.0;
				return true;
			}
			else
			{
				GENERAL_ERROR ("Gamma control not supported.");
				return false;
			}
		}
		else if (pprim->QueryInterface (IID_IDirectDrawGammaControl, pgc) == S_OK)
		{
			// Get the gamma ramp.

			DDGAMMARAMP gr;

			if (pgc->GetGammaRamp (0, &gr) != DD_OK)
			{
				GENERAL_ERROR ("Failed to get the gamma ramp values.");
				return false;
			}

			// Derive the gamma value by assuming that the gamma value is linear. Look at the value of 128.
			gamma = (float) gr.red[128] / 128.0;

			return true;
		}
		else
		{
			GENERAL_ERROR ("Neither IDirectDrawColorControl nor IDirectDrawGammaControl are supported.");
			return false;
		}
	}

	return false;
}

// Valid range: 0.0 to 5.0
bool set_gamma (float newGamma)
{
	if (PIPE)
	{
		COMPTR<IDDBackDoor>             bd;
		COMPTR<IUnknown>                pprim;
		COMPTR<IDirectDrawColorControl> pcc;
		COMPTR<IDirectDrawGammaControl> pgc;

		if (PIPE->QueryInterface(IID_IDDBackDoor, bd) != GR_OK)
		{
			GENERAL_ERROR("Failed to query backdoor interface from the render pipeline.");
			return false;
		}

		if (bd->get_dd_provider (DDBD_P_PRIMARYSURFACE, pprim) != GR_OK)
		{
			GENERAL_ERROR("Failed to get primary surface object via back door.");
			return false;
		}

		if (pprim->QueryInterface (IID_IDirectDrawColorControl, pcc) == S_OK)
		{
			// Set the new gamma value using the color control interface

			DDCOLORCONTROL cc;
			memset (&cc, 0, sizeof(cc));
			cc.dwSize = sizeof(cc);
			cc.dwFlags &= DDCOLOR_GAMMA;
			cc.lGamma = (long) min(newGamma * 100 + 1, 500);

			if (pcc->SetColorControls (&cc) != DD_OK)
			{
				GENERAL_ERROR ("Failed to set the color control values.");
				return false;
			}

			return true;
		}
		else if (pprim->QueryInterface (IID_IDirectDrawGammaControl, pgc) == S_OK)
		{
			// Set the gamma using the gamma ramp.
			// The gamma ramp will always be linear, with the gamma value determining slope and gamma
			// intercept of the graph.
			// The formula is: g(i) = m * i + b
			// g(255) = 65535
			// g(0) = gamma => b = gamma
			// => m = (65535 - gamma)/255

			// NOTE: newGamma range is assumed to be -1.0 to 1.0
			float b = newGamma * 65535;
			float m = (65535 - b) / 255;

			DDGAMMARAMP gr;
			for (int i = 0; i < 256; ++i)
			{
				int val = m * i + b;
				WORD wVal = max(0, min(val, 65535));
				gr.red[i] = wVal;
//				gr.green[i] = wVal;
//				gr.blue[i] = wVal;
				gr.green[i] = i * 256;
				gr.blue[i] = i * 256;
			}

//			if (pgc->SetGammaRamp (DDSGR_CALIBRATE, &gr) != DD_OK)
			if (pgc->SetGammaRamp (0, &gr) != DD_OK)
			{
				GENERAL_ERROR ("Failed to set the gamma ramp values.");
				return false;
			}

			return true;
		}
		else
		{
			GENERAL_ERROR ("Neither IDirectDrawColorControl nor IDirectDrawGammaControl are supported.");
			return false;
		}
	}

	return false;
}

