//--------------------------------------------------------------------------//
//                                                                          //
//                              DSStream.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/DSStream.cpp 21    3/11/02 10:18a Tmauer $
*/

//--------------------------------------------------------------------------//
#include "pch.h"
#include "globals.h"

#include "CQTrace.h"
#include "DSStream.h"
#include "Startup.h"
#include "Resource.h"

#include <WindowManager.h>
#include <FileSys.h>
#include <IDDBackDoor.h>
//#include <RPUL.h>
//#include <System.h>

#include <mmsystem.h>
#include <dsound.h>

#include <stdio.h>
#include <mmsystem.h>
//#include <ddrawex.h>
#include <mmstream.h>
#include <amstream.h>
#include <ddstream.h>
#include <control.h>
#include <evcode.h>

#define CQE_DSSTREAM  (WM_USER+1)
#define DSERROR(hr)				\
			if (hr != DS_OK)	\
				CQBOMB1("DSound error #%08X", hr)
#define MAX_STREAMS 4


static HWND hDSStreamWnd;
static HANDLE hThread;
static CRITICAL_SECTION criticalSection;
static HANDLE hLogFile=INVALID_HANDLE_VALUE;

struct STRMELM
{
	S32		   handle;
	DSStream * pStream;
	char	   pathname[MAX_PATH];
};
static STRMELM  strmList[MAX_STREAMS];
static S32 lastHandle;
static DSStream * g_streamList;
//--------------------------------------------------------------------------//
//
static S32 addToStreamList (DSStream * pStream, const char *pathName)
{
	EnterCriticalSection(&criticalSection);
	
	S32 i;
	for (i = 0; i < MAX_STREAMS; i++)
	{
		if (strmList[i].pStream == 0)
		{
			strmList[i].pStream = pStream;
			strmList[i].handle = ++lastHandle;
			strncpy(strmList[i].pathname, pathName, sizeof(strmList[i].pathname));
			LeaveCriticalSection(&criticalSection);
			return i;
		}
		else
		if (pStream == strmList[i].pStream)
		{
			LeaveCriticalSection(&criticalSection);
			CQBOMB0("can't be in the list twice");
			return -1;
		}
	}

	LeaveCriticalSection(&criticalSection);
	CQBOMB0("Max streams exceeded");
	return -1;
}
//--------------------------------------------------------------------------//
//
static void removeFromList (DSStream * pStream)
{
	EnterCriticalSection(&criticalSection);
	
	int i;
	for (i = 0; i < MAX_STREAMS; i++)
	{
		if (pStream == strmList[i].pStream)
		{
			strmList[i].pStream = 0;
			strmList[i].handle = 0;
			strmList[i].pathname[0] = 0;
		}
	}

	LeaveCriticalSection(&criticalSection);
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
DSStream::DSStream (void)
{
	bInitialized = false;
	refCount = 1;
	state = STOPPED;
	pendingState = ABORTED;
	volume = 0;
	pNext = g_streamList;
	g_streamList = this;
	posX = posY = 0;

//	COMPTR<IDDBackDoor> pBackDoor;
//	COMPTR<IUnknown> pDD1;
	//HRESULT hr;

	//hr = PIPE->QueryInterface(IID_IDDBackDoor, pBackDoor);
	//if (hr != GR_OK)
	//	goto Done;
	//hr = pBackDoor->get_dd_provider(DDBD_P_DIRECTDRAW, pDD1);
	//if (hr != GR_OK)
	//	goto Done;
//	hr = pDD1->QueryInterface(IID_IDirectDraw4, pDD4);    
//	if (hr != DD_OK)
//		goto Done;
pDD4 = 0;
//Done:
	return;
}
//--------------------------------------------------------------------------//
//
DSStream::~DSStream (void)
{
	if (hThread)
		removeFromList(this);
	DSStream * prev = 0, *node = g_streamList;
	while (node != this)
	{
		prev = node;
		node = node->pNext;
		CQASSERT(node);
	}

	// remove ourselves from the global linked list
	if (prev == 0)
		g_streamList = pNext;
	else
		prev->pNext = pNext;
}
//--------------------------------------------------------------------------//
//
DSStream::DSStream (const DSStream & new_ptr)
{
	CQBOMB0("Operation not allowed");
}
//--------------------------------------------------------------------------//
//
DSStream & DSStream::operator = (const DSStream & new_ptr)
{
	CQBOMB0("Operation not allowed");
	return *this;
}
//--------------------------------------------------------------------------//
//
void DSStream::ThreadedOpen (const char * pathName)
{
	bool result = false;
    HRESULT hr;
	wchar_t buffer[MAX_PATH];

	_localAnsiToWide(pathName, buffer, sizeof(buffer));

	if (bInitialized==0)
	{
		hr = CoCreateInstance(CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER, IID_IAMMultiMediaStream, pAMStream);
   		if (hr != DD_OK)
		{
			CQTRACE10("Could not create IMultimediaStream instance");
			goto Done;
		}

//		hr = pAMStream->Initialize(STREAMTYPE_READ, 0, NULL);
		// remember: can't wait for completion in single-threaded mode (jy)
		hr = pAMStream->Initialize(STREAMTYPE_READ, AMMSF_NOGRAPHTHREAD, NULL);
   		if (hr != DD_OK)
		{
			CQTRACE10("IMultimediaStream::Initialize() failed.");
			goto Done;
		}

		if (pDD4!=0)
		{
			hr = pAMStream->AddMediaStream(pDD4, &MSPID_PrimaryVideo, 0, NULL);
   			if (hr != DD_OK)
			{
				CQTRACE10("Adding Primary video failed");
				goto Done;
			}
		}

#if 0
		hr = pAMStream->AddMediaStream(NULL, &MSPID_PrimaryAudio, AMMSF_ADDDEFAULTRENDERER, NULL);
   		if (hr != DD_OK)
		{
			CQTRACE10("Adding Primary audio failed");
			//goto Done;
		}
#endif

		// THIS IS FAILING FOR REASONS YET UNKNOWN 
		// -RYAN
		hr = pAMStream->GetFilterGraph(pGB);
		
   		if (hr != DD_OK)
		{
			CQTRACE10("IMultimediaStream::GetFilterGraph() failed.");
			goto Done;
		}

		if (CQFLAGS.bDShowLog)
		{
			if (pGB != 0) hr = pGB->SetLogFile((U32)hLogFile);
   			if (hr != DD_OK)
			{
				CQTRACE10("IGraphBuilder::SetLogFile() failed.");
			}
		}

		if (pGB != 0) hr = pGB->QueryInterface(IID_IMediaEventEx, pME);
   		if (hr != DD_OK)
		{
			CQTRACE10("IGraphBuilder::QueryInterface(IID_IMediaEventEx) failed.");
			goto Done;
		}

		if (pME) hr = pME->SetNotifyWindow((OAHWND)hMainWindow, CQE_STREAMER2, (LONG) this);
   		if (hr != DD_OK)
		{
			CQTRACE10("IMediaControl::SetNotifyWindow() failed.");
			goto Done;
		}
		if (pME) hr = pME->SetNotifyFlags(0);		// enable notification
   		if (hr != DD_OK)
		{
			CQTRACE10("IMediaControl::SetNotifyFlags() failed.");
			goto Done;
		}

		bInitialized = true;
	}

	if (pGB != 0) hr = pGB->RenderFile(buffer, NULL);
 	if (hr != DD_OK)
	{
		CQTRACE12("IGraphBuilder::RenderFile() returned error 0x%X on file %s.", hr, pathName);
		goto Done;
	}

	result = true;

Done:
	state = (result) ? RUNNING : ABORTED;

	// notify other thread that we have completed the opening stage
	PostMessage(hMainWindow, CQE_STREAMER2, 0, (LPARAM) this);
}
//--------------------------------------------------------------------------//
//
HRESULT DSStream::createDDSample (void)
{
	COMPTR<IMediaStream> pPrimaryVidStream;
    COMPTR<IDirectDrawMediaStream> pDDStream;
	COMPTR<IDirectDrawSurface4> pPrimary;
	COMPTR<IDDBackDoor> pBackDoor;
	COMPTR<IUnknown> pDD1;
    DDSURFACEDESC ddsd;
	HRESULT hr;

	hr = PIPE->QueryInterface(IID_IDDBackDoor, pBackDoor);
	if (hr != GR_OK)
		goto Done;
	hr = pBackDoor->get_dd_provider(DDBD_P_PRIMARYSURFACE, pDD1);
	if (hr != GR_OK)
		goto Done;
	hr = pDD1->QueryInterface(IID_IDirectDrawSurface4, pPrimary);
	if (hr != GR_OK)
		goto Done;
	//
	// create a directDrawStreamSample
	//
    hr = pAMStream->GetMediaStream(MSPID_PrimaryVideo, pPrimaryVidStream);
   	if (hr != DD_OK)
	{
		CQTRACE10("GetMediaStream failed");
		goto Done;
	}
    hr = pPrimaryVidStream->QueryInterface(IID_IDirectDrawMediaStream, pDDStream);
   	if (hr != DD_OK)
	{
		CQTRACE10("Could not get IDirectDrawMediaStream");
		goto Done;
	}
	//
	// try to set to a compatible format
	//
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	pDDStream->GetFormat(&ddsd, 0, 0, 0);
	pPrimary->GetPixelFormat(&ddsd.ddpfPixelFormat);
	ddsd.dwFlags |= DDSD_PIXELFORMAT;
	pDDStream->SetFormat(&ddsd, NULL);

	// can fail if there is no video data!
    hr = pDDStream->CreateSample(NULL, NULL, DDSFF_PROGRESSIVERENDER, pSample);
   	if (hr != DD_OK)
	{
		CQTRACE11("IDirectDrawMediaStream::CreateSample returned error 0x%X", hr);
		goto Done;
	}

Done:
	return hr;
}
//--------------------------------------------------------------------------//
//
bool DSStream::Open (struct IFileSystem * parentDir, const char * fileName)
{
	int len;
	char abuffer[MAX_PATH];

	if (parentDir==0)
		return false;
	parentDir->GetFileName(abuffer, sizeof(abuffer));
	len = strlen(abuffer);
	if (len && abuffer[len-1] != '\\')
	{
		abuffer[len++] = '\\';
		abuffer[len] = 0;
	}
	strncpy(abuffer+len, fileName, MAX_PATH-len);

	while (state == OPENPENDING)
		Sleep(1);

	S32 index = addToStreamList(this, abuffer);
	if (index >= 0)
	{
		state = OPENPENDING;
		pendingState = ABORTED;
		volume = 0;
		PostMessage(hDSStreamWnd, CQE_DSSTREAM, 0, strmList[index].handle);
		return true;
	}

	return false;
}
//------------------------------------------------------------------------------------------//
//
void DSStream::Stop (void)
{
	if (bInitialized && state != OPENPENDING)
	{
		pAMStream->SetState(STREAMSTATE_STOP);
		state = STOPPED;
		pendingState = ABORTED;
	}
	else
		pendingState = STOPPED;
}
//------------------------------------------------------------------------------------------//
//
void DSStream::Pause (void)
{
	if (bInitialized && state != OPENPENDING)
	{
		pAMStream->SetState(STREAMSTATE_STOP);
		state = PAUSED;
		pendingState = ABORTED;
	}
	else
		pendingState = PAUSED;
}
//------------------------------------------------------------------------------------------//
//
void DSStream::Blit (void)
{
	HRESULT hr;
    COMPTR<IDirectDrawSurface4> pSurface;
	COMPTR<IDirectDrawSurface4> pBackBuffer;
	COMPTR<IDDBackDoor> pBackDoor;
	COMPTR<IUnknown> pDD1;
	RECT srcRect, destRect;
	WM_WINAREA area;

	if (pSample==0)
		goto Done;

	hr = PIPE->QueryInterface(IID_IDDBackDoor, pBackDoor);
	if (hr != GR_OK)
		goto Done;
	// if we are in the front-end, render directly to the front buffer, no swapping
	hr = pBackDoor->get_dd_provider(((CQFLAGS.bFullScreenMovie) ? DDBD_P_PRIMARYSURFACE : DDBD_P_BACKSURFACE), pDD1);
	if (hr != GR_OK)
		goto Done;
	hr = pDD1->QueryInterface(IID_IDirectDrawSurface4, pBackBuffer);
	if (hr != GR_OK)
		goto Done;

	{
		COMPTR<IDirectDrawSurface> pSurface1;

		hr = pSample->GetSurface(pSurface1, &srcRect);
   		if (hr != DD_OK)
		{
			CQERROR0("IDirectDrawStreamSample::GetSurface failed");
			goto Done;
		}

		hr = pSurface1->QueryInterface(IID_IDirectDrawSurface4, pSurface);
   		if (hr != DD_OK)
		{
			CQERROR0("Could not get IDirectDrawSurface4");
			goto Done;
		}
	}

	hr = pSample->Update(0, NULL, NULL, 0);

	if (bUseRect)
		destRect = rect;
	else
	{
		destRect.left = posX;
		destRect.top = posY;
		destRect.right = destRect.left + IDEAL2REALX(srcRect.right-srcRect.left);
		destRect.bottom = destRect.top + IDEAL2REALY(srcRect.bottom-srcRect.top);
	}

	if (CQFLAGS.bFullScreenMovie)		// are we using the front buffer!?
	{
		WM->GetClientArea(area);
		destRect.left += area.x;
		destRect.right += area.x;
		destRect.top += area.y;
		destRect.bottom += area.y;
	}
	
	hr = pBackBuffer->Blt(&destRect, pSurface, &srcRect, DDBLT_WAIT, NULL);

Done:
	return;
}
//------------------------------------------------------------------------------------------//
//
void DSStream::Run (void)
{
	if (bInitialized && state != OPENPENDING)
	{
		if (pSample==0)
			createDDSample();
		if (pSample==0)
		{
			state = ABORTED;
			pendingState = ABORTED;
			// notify caller that playback has failed
			PostMessage(hMainWindow, CQE_STREAMER2, 0, (LPARAM) this);
		}
		else
		{
			SetVolume(volume);
			if (bUseRect)
				SetPosition(rect);
			else
				SetPosition(posX, posY);
			pAMStream->SetState(STREAMSTATE_RUN);
			state = RUNNING;
			pendingState = ABORTED;
		}
	}
	else
		pendingState = RUNNING;
}
//------------------------------------------------------------------------------------------//
//
void DSStream::SetVolume (S32 dsVolume)
{
	volume = dsVolume;
	if (bInitialized && state != OPENPENDING)
	{
		COMPTR<IBasicAudio> pBA;

		if (pGB != 0) 
			if (pGB->QueryInterface(IID_IBasicAudio, pBA) == DD_OK)
		{
			pBA->put_Volume(dsVolume);
		}
	}
}
//------------------------------------------------------------------------------------------//
//
void DSStream::SetPosition (S32 x, S32 y)
{
	posX = x;
	posY = y;
	bUseRect = false;
}
//------------------------------------------------------------------------------------------//
//
void DSStream::SetPosition (const RECT& rc)
{
	posX = rc.left;
	posY = rc.top;
	bUseRect = true;
	rect = rc;
}
//------------------------------------------------------------------------------------------//
//
void DSStream::GetEvent (void)
{
	S32 evcode, lParam1, lParam2;
	HRESULT hr;

	while (state==OPENPENDING)
		Sleep(1);

	while (pME!=0 && (hr = pME->GetEvent(&evcode, &lParam1, &lParam2, 0)) == S_OK)
	{
		switch (evcode)
		{
		case EC_COMPLETE:
			state = STOPPED;
			break;
		case EC_STREAM_ERROR_STILLPLAYING:
		case EC_ERROR_STILLPLAYING:
			break;
		case EC_ERRORABORT:
		case EC_NEED_RESTART:
//		case EC_SNDDEV_IN_ERROR:
//		case EC_SNDDEV_OUT_ERROR:
		case EC_STREAM_ERROR_STOPPED:
		case EC_USERABORT:
		case EC_WINDOW_DESTROYED:
			state = ABORTED;
			break;
		case EC_SHUTTING_DOWN:
			CQTRACE10("Shutting down");
			break;
		case 0xD:
		case 0xE:
			break;
		default:
			CQTRACE11("Unhandled event 0x%X.", evcode);
		}

		pME->FreeEventParams(evcode, lParam1, lParam2);
	}

	// do queued command
	if (pendingState != ABORTED && bInitialized && state != OPENPENDING)
	{
		if (state == RUNNING)
		{
			switch (pendingState)
			{
			case RUNNING:
				Run();
				break;
			case STOPPED:
				Stop();
				break;
			case PAUSED:
				Pause();
				break;
			default: 
				CQBOMB0("Unknown state");
				break;
			}
		}
		else
			pendingState = ABORTED;
	}
}
//------------------------------------------------------------------------------------------//
//
void DSStream::ThreadedReset (void)
{
if (pGB != 0) 	pGB.free();
	pME.free();
    pAMStream.free();
    pSample.free();
	bInitialized = false;

	if (pNext)
		pNext->ThreadedReset();
}
//--------------------------------------------------------------------------
//
static void threadDoOpen (LONG index)
{
	return; // streams are having strange problems -RYAN

	EnterCriticalSection(&criticalSection);
	
	int i;
	for (i = 0; i < MAX_STREAMS; i++)
	{
		if (index == strmList[i].handle)
		{
			strmList[i].pStream->ThreadedOpen(strmList[i].pathname);
			strmList[i].pStream = 0;
			strmList[i].handle = 0;
			strmList[i].pathname[0] = 0;
		}
	}

	LeaveCriticalSection(&criticalSection);
}
//--------------------------------------------------------------------------
//  
static long CALLBACK WndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	switch (message)
	{
	case CQE_DSSTREAM:
		{
			threadDoOpen(lParam);
		}
		break;
		
	}	// end of switch
	
	return DefWindowProc (hwnd, message, wParam, lParam);
}
//--------------------------------------------------------------------------
//  
static DWORD CALLBACK DSStreamThreadMain (void * pNull)
{
	MSG         msg;
	char *szName = "DSStreamThreadMain";
	HINSTANCE hInstance = GetModuleHandle(0);
	ATOM atom;

	// create a window
	
	{
		WNDCLASS    wndclass ;
		wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
		wndclass.lpfnWndProc   = WndProc ;
		wndclass.cbClsExtra    = 0 ;
		wndclass.cbWndExtra    = 0;
		wndclass.hInstance     = hInstance;
		wndclass.hIcon         = 0;
		wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
		wndclass.hbrBackground = (HBRUSH)(COLOR_MENU+1);
		wndclass.lpszMenuName  = NULL ;
		wndclass.lpszClassName = szName;
		
		if ((atom = RegisterClass (&wndclass)) == 0)
			goto Done;
	}
	
    hDSStreamWnd = CreateWindow(
        szName,
        szName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        400,
        200,
        NULL,
        NULL,
        hInstance,
        NULL
		);                                                    
	
    if (!hDSStreamWnd)   
        goto Done;
	
	CoInitialize(0);
	
	// main loop
	
	while (GetMessage (&msg, NULL, 0, 0))
	{
		TranslateMessage (&msg) ;
		DispatchMessage (&msg) ;
	}

	// shutdown all instances of streamer before closing the thread
	if (g_streamList)
		g_streamList->ThreadedReset();

	CoUninitialize();
	
	DestroyWindow(hDSStreamWnd);
	hDSStreamWnd = 0;
	UnregisterClass((char *)atom, hInstance);
	// close down the show
	
	OutputDebugString("Shutting down streaming thread.\n");
	
Done:
	return 0;
}
//--------------------------------------------------------------------------
//  
static BOOL startStreamerThread (void)
{
	DWORD dwThreadID;
	if (hThread)
		return 1;

	InitializeCriticalSection(&criticalSection);

	
	hThread = CreateThread(0,4096, (LPTHREAD_START_ROUTINE) DSStreamThreadMain,
		(LPVOID)0, 0, &dwThreadID);
	
	if (hThread==0)
		return 0;

	DWORD dwCode;
			
	while (hDSStreamWnd==0 && GetExitCodeThread(hThread, &dwCode) && dwCode == STILL_ACTIVE) 
		Sleep(1);

//	SetThreadPriority(hThread, THREAD_PRIORITY_LOWEST);

	return 1;
}
//--------------------------------------------------------------------------//
//
static void shutdownStreamerThread (void)
{
	PostMessage(hDSStreamWnd, WM_QUIT, 0, 0);
	WaitForSingleObject(hThread, INFINITE);
	::CloseHandle(hThread);
	hThread=0;
	DeleteCriticalSection(&criticalSection);
}

#define SET_FORMAT 1
//--------------------------------------------------------------------------//
//
struct _dsstreamer : GlobalComponent , IDAComponent
{
	IDAComponent * _this;

	BOOL32 init (void)
	{
		return 1;
	}
	
	virtual void Startup (void)
	{
		_this = this;
		AddToGlobalCleanupList(&_this);
		startStreamerThread();
	}

	virtual void Initialize (void)
	{
		if (CQFLAGS.bDShowLog)
		{
			hLogFile = CreateFile("DShowlog.txt", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
		}
	}

	/* IDAComponent methods */

	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance)
	{
		*instance = 0;
		return GR_GENERIC;
	}
	
	DEFMETHOD_(U32,AddRef) (void)
	{
		return 2;
	}
	
	DEFMETHOD_(U32,Release) (void)
	{
		shutdownStreamerThread();
		if (hLogFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hLogFile);
			hLogFile = INVALID_HANDLE_VALUE;
		}
		return 0;
	}


};

static _dsstreamer startup;
//------------------------------------------------------------------------------------------//
//-----------------------------------End DSStream.cpp---------------------------------------//
//------------------------------------------------------------------------------------------//
