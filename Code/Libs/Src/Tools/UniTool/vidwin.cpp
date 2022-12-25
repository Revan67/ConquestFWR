//
// VidWin.cpp - Widget for playing movies
//

//
// Include files
//

#include <windows.h>

#include "stdwidget.h"
#include "script.h"

#include <TSmartPointer.h>
#include <System.h>
#include <stdio.h>
#include <ddrawex.h>
#include <mmstream.h>
#include <amstream.h>
#include <ddstream.h>

#include <fdump.h>
#include <tempstr.h>
#include "strstuff.h"

//
// GUID stuff
//
#if 0
#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID(CLSID_DirectDrawFactory, 
0x4fd2a832, 0x86c8, 0x11d0, 0x8f, 0xca, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);

DEFINE_GUID(IID_IDirectDrawFactory, 
0x4fd2a833, 0x86c8, 0x11d0, 0x8f, 0xca, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);
#else
EXTERN_C const GUID CLSID_DirectDrawFactory, IID_IDirectDrawFactory; 
#endif

//
// Imported Variables
//

//
// Class and structure definitions
//

// The render widget is a normal top level with special drawing code.
#define VideoWidgetAncestor TopLevelWidget
struct VideoWidget : public VideoWidgetAncestor
{
protected:
	COMPTR<IDirectDrawFactory>      cpDDFactory;
	COMPTR<IDirectDraw>             cpDD1;
	COMPTR<IDirectDraw4>            cpDD4;
	COMPTR<IDirectDrawSurface4>     cpDDSurface;
	COMPTR<IMultiMediaStream>       cpMMStream;
	COMPTR<IDirectDrawClipper>      cpClipper;
	COMPTR<IMediaStream>            cpPrimaryVidStream;
	COMPTR<IDirectDrawMediaStream>  cpDDStream;
	COMPTR<IDirectDrawSurface4>     cpSurface;
	COMPTR<IDirectDrawStreamSample> cpSample;
	RECT                            srcRect;
	bool                            playing;

protected:
	HRESULT openMMStream (const char * pszFileName, IDirectDraw4 *pDD, IMultiMediaStream **ppMMStream);
	bool beginPlayback(const char *filename);
	bool stopPlayback ();
	bool updatePlayback ();

public:
	VideoWidget ();
	~VideoWidget ();

	// Playback functions
	void startmovie (const char *filename);  // starts the playback of the given filename as a movie
	void stopmovie ();                       // stops playback and frees resources
	void updatemovie ();                     // updates the movie if playing
	void playmovie(const char *filename);    // blocking playback

	// BaseWidget overloads
	virtual bool on_erasebkgnd (HDC hdc); // WM_ERASEBKGND
	virtual bool on_destroy (); // WM_DESTROY

	// IScriptable Interface, inherited from IWidget
	virtual int method_count(void);
	virtual const char *method_name(int index);
	virtual int method_speclen(int index);
	virtual const ParamSpec *method_spec (int index);
	virtual bool invoke (const char *methodName, Variant *result, int paramCount, Variant *params);
	virtual bool invokeByIndex (int index, Variant *result, int paramCount, Variant *params);
};

VideoWidget::VideoWidget ()
{
	playing = false;
}

VideoWidget::~VideoWidget ()
{
	stopPlayback();
}

void VideoWidget::playmovie (const char *filename)
{
	// NOTE: This is a blocking call.
	extern HRESULT PlayMovie (HWND hMainWindow, const char * filename);
	PlayMovie (hBaseWnd, filename);
}

void VideoWidget::startmovie (const char *filename)
{
	beginPlayback (filename);
}

void VideoWidget::stopmovie ()
{
	stopPlayback ();
}

void VideoWidget::updatemovie ()
{
	updatePlayback();
}

// BaseWidget overloads
bool VideoWidget::on_erasebkgnd (HDC hdc)
{
	// Pretend that we erased the background, since we will be drawing
	// the entire screen anyway.
	return true;
}

bool VideoWidget::on_destroy ()
{
	// Stop the playback
	stopPlayback();

	// Let the default processing continue.
	return false;
}

// IScriptable Interface, inherited from IWidget
int VideoWidget::method_count(void)
{
	return VideoWidgetAncestor::method_count() + 3;
}

static const char *vw_names[] = {"play_movie", "start_movie", "stop_movie"};
const char *VideoWidget::method_name(int index)
{
	if (index >= VideoWidgetAncestor::method_count())
	{
		return vw_names[index-VideoWidgetAncestor::method_count()];
	}
	else
	{
		return VideoWidgetAncestor::method_name (index);
	}
}

int VideoWidget::method_speclen(int index)
{
	if (index >= VideoWidgetAncestor::method_count())
	{
		static int counts[] = {2, 2, 1};
		return counts[index-VideoWidgetAncestor::method_count()];
	}
	else
	{
		return VideoWidgetAncestor::method_speclen (index);
	}
}

const ParamSpec *VideoWidget::method_spec (int index)
{
	if (index >= VideoWidgetAncestor::method_count())
	{
		switch (index - VideoWidgetAncestor::method_count())
		{
		case 0: // play_movie
			{
				static ParamSpec spec[] = 
				{
					{PS_VOID, 0},
					{PS_STRING, 0}
				};
				return spec;
			}
			break;
		case 1: // start_movie
			{
				static ParamSpec spec[] = 
				{
					{PS_VOID, 0},
					{PS_STRING, 0}
				};
				return spec;
			}
			break;
		case 2: // stop_movie
			{
				static ParamSpec spec[] = 
				{
					{PS_VOID, 0}
				};
				return spec;
			}
			break;
		}
		return NULL;
	}
	else
	{
		return VideoWidgetAncestor::method_spec (index);
	}
}

bool VideoWidget::invoke (const char *methodName, Variant *result, int paramCount, Variant *params)
{
	for (int i = 0; i < sizeof(vw_names)/sizeof(char *); ++i)
	{
		if (!strcmp (methodName, vw_names[i]))
		{
			return invokeByIndex (VideoWidgetAncestor::method_count() + i, result, paramCount, params);
		}
	}
	return VideoWidgetAncestor::invoke (methodName, result, paramCount, params);
}

bool VideoWidget::invokeByIndex (int index, Variant *result, int paramCount, Variant *params)
{
	if (index >= VideoWidgetAncestor::method_count())
	{
		switch (index - VideoWidgetAncestor::method_count())
		{
		case 0: // play_movie
			if (params[0].spec.type == PS_STRING)
			{
				playmovie (params[0].sVal);
				result->spec.type = PS_VOID;
				result->spec.tag = 0;
				return true;
			}
			break;
		case 1: // start_movie
			if (params[0].spec.type == PS_STRING)
			{
				startmovie (params[0].sVal);
				result->spec.type = PS_VOID;
				result->spec.tag = 0;
				return true;
			}
			break;
		case 2: // stop_movie
			{
				stopmovie ();
				result->spec.type = PS_VOID;
				result->spec.tag = 0;
				return true;
			}
			break;
		}

		return false;
	}
	return VideoWidgetAncestor::invokeByIndex (index, result, paramCount, params);
}

// Playback functions

HRESULT VideoWidget::openMMStream (const char * pszFileName, IDirectDraw4 *pDD, IMultiMediaStream **ppMMStream)
{
    COMPTR<IAMMultiMediaStream> pAMStream;
    HRESULT hr;

    *ppMMStream = NULL;

	hr = CoCreateInstance(CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER, IID_IAMMultiMediaStream, pAMStream);
   	if (hr != DD_OK)
	{
		GENERAL_ERROR("Could not create IMultimediaStream instance");
		goto Done;
	}

    hr = pAMStream->Initialize(STREAMTYPE_READ, 0, NULL);
   	if (hr != DD_OK)
	{
		GENERAL_ERROR("IMultimediaStream::Initialize() failed.");
		goto Done;
	}

    hr = pAMStream->AddMediaStream(pDD, &MSPID_PrimaryVideo, 0, NULL);
   	if (hr != DD_OK)
	{
		GENERAL_ERROR("Adding Primary video failed");
		goto Done;
	}

    hr = pAMStream->AddMediaStream(NULL, &MSPID_PrimaryAudio, AMMSF_ADDDEFAULTRENDERER, NULL);
   	if (hr != DD_OK)
	{
		GENERAL_ERROR("Adding Primary audio failed");
		goto Done;
	}

    wchar_t wPath[MAX_PATH];
	_localAnsiToWide(pszFileName, wPath, sizeof(wPath));

	hr = pAMStream->OpenFile(wPath, 0);

    *ppMMStream = pAMStream;
    pAMStream->AddRef();

Done:
    return hr;
}

bool VideoWidget::beginPlayback(const char *filename)
{
	ASSERT (filename);
	if (!hBaseWnd)
	{
		// There is no window, so punt.
		return false;
	}

	HRESULT hr;
	COMPTR<IDirectDraw>             pDD1;
	COMPTR<IMediaStream>            pPrimaryVidStream;
    COMPTR<IDirectDrawMediaStream>  pDDStream;
    COMPTR<IDirectDrawSurface4>     pSurface;
    COMPTR<IDirectDrawStreamSample> pSample;
	COMPTR<IDirectDrawFactory>      pDDFactory;
	COMPTR<IDirectDraw4>            pDD4;
	COMPTR<IDirectDrawSurface4>     pDDSurface;
	COMPTR<IMultiMediaStream>       pMMStream;
	COMPTR<IDirectDrawClipper>      pClipper;
    DDSURFACEDESC2    ddsd2;
    
	//
	// Create a DirectDrawFactory object
	//
    hr = CoCreateInstance(CLSID_DirectDrawFactory, NULL, CLSCTX_INPROC_SERVER, 
                            IID_IDirectDrawFactory, pDDFactory);
	
	if (hr != DD_OK)
	{
		GENERAL_ERROR("Could not create DirectDrawFactory");
		goto Done;
	}

    hr = pDDFactory->CreateDirectDraw(NULL, hBaseWnd, DDSCL_NORMAL, NULL, NULL, pDD1);
	
	if (hr != DD_OK)
	{
		GENERAL_ERROR("Couldn't create DirectDraw object");
		goto Done;
	}
	
	//
    //Now query for the new I interface
	//
    hr = pDD1->QueryInterface(IID_IDirectDraw4, pDD4);    

	if (hr != DD_OK)
	{
		GENERAL_ERROR("Couldn't get IDirectDraw4");
		pDD1.free();
		goto Done;
	}

	pDD1.free();

	//
    //Initialize the DDSURFACEDESC structure for the primary surface
	//
	memset(&ddsd2, 0, sizeof(ddsd2));
	ddsd2.dwSize = sizeof(ddsd2);    
	ddsd2.dwFlags = DDSD_CAPS;
    ddsd2.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE; 
    hr = pDD4->CreateSurface(&ddsd2, pDDSurface, NULL);

   	if (hr != DD_OK)
	{
		GENERAL_ERROR("Couldn't create Primary Surface");
		pDD4.free();
		goto Done;
	}

	//
	// create the clipper object
	//

	hr = pDD4->CreateClipper(0, pClipper, NULL);

   	if (hr != DD_OK)
	{
		GENERAL_ERROR("Couldn't create clipper");
		goto Done;
	}

	pDDSurface->SetClipper(pClipper);
	pClipper->SetHWnd(0, hBaseWnd);

    hr = openMMStream(filename, pDD4, pMMStream);

   	if (hr != DD_OK)
	{
		GENERAL_ERROR(TEMPSTR("Couldn't get play '%s', error=%08x", filename, hr));
		goto Done;
	}

    //===============================================
	
    DDSURFACEDESC ddsd;

    hr = pMMStream->GetMediaStream(MSPID_PrimaryVideo, pPrimaryVidStream);
   	if (hr != DD_OK)
	{
		GENERAL_ERROR("GetMediaStream failed");
		goto Done;
	}

    hr = pPrimaryVidStream->QueryInterface(IID_IDirectDrawMediaStream, pDDStream);
   	if (hr != DD_OK)
	{
		GENERAL_ERROR("Could not get IDirectDrawMediaStream");
		goto Done;
	}

	//
	// try to set to a compatible format
	//
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	pDDStream->GetFormat(&ddsd, 0, 0, 0);
	pDDSurface->GetPixelFormat(&ddsd.ddpfPixelFormat);
	ddsd.dwFlags |= DDSD_PIXELFORMAT;
	pDDStream->SetFormat(&ddsd, NULL);

    hr = pDDStream->CreateSample(NULL, NULL, DDSFF_PROGRESSIVERENDER, pSample);
   	if (hr != DD_OK)
	{
		GENERAL_ERROR("IDirectDrawMediaStream::CreateSample failed");
		goto Done;
	}

	{
		COMPTR<IDirectDrawSurface> pSurface1;

		hr = pSample->GetSurface(pSurface1, &srcRect);
   		if (hr != DD_OK)
		{
			GENERAL_ERROR("IDirectDrawStreamSample::GetSurface failed");
			goto Done;
		}

		hr = pSurface1->QueryInterface(IID_IDirectDrawSurface4, pSurface);
   		if (hr != DD_OK)
		{
			GENERAL_ERROR("Could not get IDirectDrawSurface4");
			goto Done;
		}
	}

    hr = pMMStream->SetState(STREAMSTATE_RUN);
   	if (hr != DD_OK)
	{
		GENERAL_ERROR("IMultiMediaStream::SetState failed");
		goto Done;
	}

	// All is well, so copy the interface pointers to their final resting place.

	cpPrimaryVidStream = pPrimaryVidStream;
	pPrimaryVidStream->AddRef();
    
	cpDDStream = pDDStream;
	pDDStream->AddRef();

    cpSurface= pSurface;
    pSurface->AddRef();

	cpSample = pSample;
	pSample->AddRef();
	
	cpDDFactory = pDDFactory;
	pDDFactory->AddRef();
	
	cpDD4 = pDD4;
	pDD4->AddRef();

	cpDDSurface = pDDSurface;
	pDDSurface->AddRef();

	cpMMStream = pMMStream;
	pMMStream->AddRef();

	cpClipper = pClipper;
	pClipper->AddRef();

	playing = true;

Done:
    return hr == DD_OK;
}

bool VideoWidget::stopPlayback ()
{	
	if (playing)
	{
		cpMMStream->SetState(STREAMSTATE_STOP);

		cpPrimaryVidStream.free();
		cpDDStream.free();
		cpSurface.free();
		cpSample.free();
		cpDDFactory.free();
		cpDD4.free();
		cpDDSurface.free();
		cpMMStream.free();
		cpClipper.free();
		playing = false;
	}
	return true;
}

bool VideoWidget::updatePlayback ()
{
	if (playing)
	{
		if (cpSample->Update(0, NULL, NULL, 0) != S_OK)
		{
			return stopPlayback();
		}

		RECT r;
		GetClientRect (hBaseWnd, &r);
		POINT tl = {r.left, r.top};
		POINT br = {r.right, r.bottom};

		ClientToScreen (hBaseWnd, &tl);
		ClientToScreen (hBaseWnd, &br);

		r.top = tl.y;
		r.left = tl.x;
		r.bottom = br.y;
		r.right = br.x;

		HRESULT hr = cpDDSurface->Blt(&r, cpSurface, &srcRect, DDBLT_WAIT, NULL);
		return hr == DD_OK;
	}
	return false;
}

// Video widget creation function, exported to lua

static VideoWidget video_widget;
static bool video_created = false;

void destroy_videowindow (void)
{
	// NOTE: Just because the widget is destroyed, it doesn't mean that the instances go away.
	HWND hWnd = video_widget.get_hwnd();
	if (hWnd)
	{
		DestroyWindow (hWnd);
	}
}

static void destroyVideo (void)
{
	// Destroy the window, then clear the global instance object.
	destroy_videowindow ();
}

static void createVideo (void)
{
	// Syntax: 
	//     NewVideo
	//     (
	//         string <title>,
	//         number <xpos>, number <ypos>,
	//         number <width>, number <height>
	//     )
	// Create a button and returns its HWND as an object.
	// <image filename> is the name of the .BMP file to display
	// <xpos>,<ypos> are the location of the button in parent coordinates
	// <width>,<height> are the width and height of the image, -1 to use the actual image's value

	// Get and validate the parameters
	lua_Object title = lua_getparam(1);
	lua_Object xpos = lua_getparam(2);
	lua_Object ypos = lua_getparam(3);
	lua_Object width = lua_getparam(4);
	lua_Object height = lua_getparam(5);

	// Check the types of the input data before proceeding.
	if (!lua_isstring(title))
	{
		return;
	}
	if (!lua_isnumber(xpos) || !lua_isnumber(ypos))
	{
		return;
	}
	if (!lua_isnumber(width) || !lua_isnumber(height))
	{
		return;
	}

	// The parameters are valid, so go about creating a new button.

	if (video_widget.get_hwnd())
	{
		// Already created, so adjust its parameters
		video_widget.set_size (width, height);
		video_widget.set_position (xpos, ypos);
		video_widget.set_text (lua_getstring(title));

		// DO NOT PUSH THE WIDGET HERE!
		// Doing so will create a new object, which is not what we want.
		// The right method is below: to get the global instance value 
	}
	else
	{
		// Not created yet, so create it.
		if
		(
			!video_widget.create
			(
				(int) lua_getnumber(xpos), (int) lua_getnumber(ypos), 
				(int) lua_getnumber(width), (int) lua_getnumber(height),
				lua_getstring(title)
			)
		)
		{
			return;
		}

		// Export the widget, then set the global variable to point to it.
		export_widget (&video_widget);

		lua_Object vwo = lua_pop ();
		lua_pushobject (vwo);
		lua_setglobal ("VideoWindow");
	}

	// Return the VideoWindow object.
	lua_pushobject (lua_getglobal("VideoWindow"));
	return;
}

void init_video_widget (void)
{
	// Register the creation and destruction functions for the video widget
	lua_register ("CreateVideo", createVideo);
	lua_register ("DestroyVideo", destroyVideo);
}

void update_videowindow(void)
{
	if (video_widget.get_hwnd())
	{
		video_widget.updatemovie();
	}
}
