//--------------------------------------------------------------------------//
//                                                                          //
//                              DShowStreamer.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Pbleisch $

   $Header: /Libs/dev/Src/Streamer/dsstreamer/dsstreamer.cpp 6     3/21/00 4:30p Pbleisch $

   Audio player
*/
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Streamer.h"

#include "TComponent.h"
#include "HeapObj.h"
#include "FDump.h"
#include "TSmartPointer.h"
#include "FileSys.h"
#include "tempstr.h"

#include <mmsystem.h>
//#define INITGUID
// Begin headers for the DXMedia SDK. This sucks, by the way.

// todo(aaj-3/31/2004): since VC6.0 SP5 does not include high enough RPCNDR, probably a .Net thing
#ifndef __REQUIRED_RPCNDR_H_VERSION__
	#define __REQUIRED_RPCNDR_H_VERSION__ 450
#endif

#include <strmif.h>
#include <control.h>
#include <uuids.h>
#include <evcode.h>
// End headers for the DXMedia SDK.

static HWND hMainWindow;
static UINT uMsg;
static SINGLE g_ReadBufferTime;		// in seconds
static SINGLE g_SoundBufferTime;	// in seconds

#define DEFAULT_READ_BUFFER_TIME  4.0F
#define DEFAULT_SOUND_BUFFER_TIME (1.0F / 4.0F)

#define CQERROR0(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_ERROR), "%s(%d) : "##exp "\n", __FILE__, __LINE__)
#define CQERROR1(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_ERROR), "%s(%d) : "##exp "\n", __FILE__, __LINE__, p1)
#define CQTRACE10(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "%s(%d) : "##exp "\n", __FILE__, __LINE__)
#define CQTRACE11(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "%s(%d) : "##exp "\n", __FILE__, __LINE__, p1)
#define CQTRACE12(exp,p1, p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "%s(%d) : "##exp "\n", __FILE__, __LINE__, p1, p2)
#define CQTRACE51(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_5), "%s(%d) : "##exp "\n", __FILE__, __LINE__, p1)
#define CQWARNING1(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_WARNING), "%s(%d) : "##exp "\n", __FILE__, __LINE__, p1)

// OLE Automation has different ideas of TRUE and FALSE

#define OATRUE (-1)
#define OAFALSE (0)

//--------------------------------------------------------------------------//
//
enum STATE
{
	OFF=0,
	INIT,
	START,
	PLAY,
	LOOPEND,
	ENDOFFILE,
	ENDOFFILE1,
	ENDOFFILE2,
	ENDOFFILE3,
	STOP
};
//--------------------------------------------------------------------------//
//
struct Streamer
{
	// Streamer specific stuff.
	struct Streamer * pNext;
	volatile bool bStopRequested;
	volatile bool bKillRequested;
	volatile S32 volumeRequested;
	STATE state;
	char filename[MAX_PATH];
	WCHAR wFile[MAX_PATH];

	//---------------------------
	// looping data
	//---------------------------
	BOOL32 bLooping;
	LPVOID lpLoopData;
	DWORD  dwLoopDataSize;

	// Direct show filter graph members
	COMPTR<IGraphBuilder>  pigb;
	COMPTR<IMediaControl>  pimc;
	COMPTR<IMediaEventEx>  pimex;
	COMPTR<IVideoWindow>   pivw;
	COMPTR<IBasicAudio>    piba;
	COMPTR<IMediaPosition> pimp;
	HANDLE                 hEventReady;

	// Constructors
	Streamer (void);

	~Streamer (void){ free(); }
	
	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "DShowStreamer");
	}

	void free (void);		// free allocated resources
	void init (void);
	void start (void);		// start playing the file
	void play (void);		// continue playing the file

	STATE setState (STATE newState)
	{
		if (newState != state)
		{
			switch (newState)
			{
			case ENDOFFILE:
				if (hMainWindow && uMsg >= WM_USER)
					PostMessage(hMainWindow, uMsg, (WPARAM) IStreamer::EOFREACHED, (LPARAM) this);
				break;
			case ENDOFFILE3:
				if (hMainWindow && uMsg >= WM_USER)
					PostMessage(hMainWindow, uMsg, (WPARAM) IStreamer::COMPLETED, (LPARAM) this);
				break;
			}
		}
		return state = newState;
	}
};

//--------------------------------------------------------------------------//
//
Streamer::Streamer (void)
{
	volumeRequested = 0;
}
//--------------------------------------------------------------------------//
// free things allocated in this thread
//
void Streamer::free (void)
{
	// Clean up the DirectShow handles here even though they will be cleaned
	// up by the destructor automatically. This allows them to be freed
	// without destroying this instance.

    pivw.free ();
    pigb.free ();
    pimc.free ();
    pimex.free ();
	piba.free ();
	pimp.free ();
}

//--------------------------------------------------------------------------//
// Create the DShow interfaces.
// Create the filter graph.
//
void Streamer::init (void)
{
	// NOTE: The filename has been copied into the "filename" member by this time.

	STATE newState = OFF;	// assume failure

	// NOTE: This only works with the Win32 filesystem, so if we don't have a valid filename, we don't work.
	if (filename[0] == 0)
	{
		CQTRACE10("NULL filename in DShowStreamer::init()\n");
		goto Done;
	}

	// Convert the filename to a unicode string.
	MultiByteToWideChar( CP_ACP, 0, filename, -1, wFile, MAX_PATH );

	// Attempt to create the filter graph

	HRESULT   hr;
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, pigb);

	if (SUCCEEDED(hr))
	{
		// QueryInterface for some basic interfaces
		pigb->QueryInterface(IID_IMediaControl, pimc);
		pigb->QueryInterface(IID_IMediaEventEx, pimex);
		pigb->QueryInterface(IID_IVideoWindow, pivw);
		pigb->QueryInterface(IID_IBasicAudio, piba);
		pigb->QueryInterface(IID_IMediaPosition, pimp);

		// Have the graph construct the appropriate graph automatically
		hr = pigb->RenderFile(wFile, NULL);

		// If there is a window associated with this stream (for some reason), hide it and ensure that it
		// doesn't automatically pop up.
		if (pivw != NULL)
		{
			pivw->put_Visible(OAFALSE);
			pivw->put_AutoShow(OAFALSE);
		}

		if (SUCCEEDED(hr))
		{
			if (piba)
			{
				piba->put_Volume(volumeRequested);
			}

			hr = pimex->GetEventHandle ((OAEVENT*) &hEventReady);
			if (SUCCEEDED(hr))
			{
				newState = START;
			}
			else
			{
				GENERAL_TRACE_1(TEMPSTR("Failed to GetEventHandle() (result=%d) in DShowStreamer::init()\n", hr));
			}
		}
		else
		{
			GENERAL_TRACE_1(TEMPSTR("Failed to RenderFile() (result=%d) in DShowStreamer::init()\n", hr));
		}
	}
	else
	{
		GENERAL_TRACE_1(TEMPSTR("Failed to create IID_IGraphBuilder (result=%d) in DShowStreamer::init()\n", hr));
	}

Done:
	if (newState != START)
	{
		CQTRACE10("DShowStreamer::init() failed");
		if (hMainWindow && uMsg >= WM_USER)
			PostMessage(hMainWindow, uMsg, (WPARAM) IStreamer::INVALID, (LPARAM) this);
	}
	setState(newState);
}
//--------------------------------------------------------------------------//
// start playing the file. The current state should be START
//
void Streamer::start (void)
{
	HRESULT hr;

	// The next state we will enter is PLAY, unless we have been requested to stop.
	setState(PLAY);

	if (bStopRequested)
	{
		setState(STOP);
	}
	else
	{
		// WARNING: We assume that pimc is valid here because we reached the START state.

		// Run the filter graph. If it fails, go to the OFF state.
		// NOTE: S_OK and S_FALSE are both valid return values here. S_OK indicates that the graph is already running,
		// while S_FALSE indicates that the graph is preparing to run and will automatically run when it is
		// ready.
		hr = pimc->Run();
		if (hr != S_OK && hr != S_FALSE)
		{
			CQTRACE10("pimc->Run() failed");
			setState(OFF);
		}
	}
}
//--------------------------------------------------------------------------//
// state can be PLAY or ENDOFFILEx
//
void Streamer::play (void)
{
	// NOTE: This is called repeatedly from the playing state.
	HRESULT hr;

	if (bStopRequested)
	{
		setState(STOP);
		hr = pimc->Stop();
		if (!SUCCEEDED(hr))
		{
			CQTRACE10("pimc->Stop() failed");
		}
		return;
	}

	// Check to see if the graph has completed. If it has, and we are looping, set LOOPEND, otherwise set
	// ENDOFFILE.
	LONG evCode;
	LONG evParam1;
	LONG evParam2;

	if (WaitForSingleObject (hEventReady, 0) == WAIT_OBJECT_0)
	{
		// Spin through the events for this stream, check for completion.
		// NOTE: GetEvent will reset the handle, so there is no need to reset it by hand.
		while (SUCCEEDED(pimex->GetEvent(&evCode, &evParam1, &evParam2, 0)))
		{
			// Free any resources allocated for the event. We don't care about them.
			hr = pimex->FreeEventParams(evCode, evParam1, evParam2);

			if (EC_COMPLETE == evCode)
			{
				// The stream is finished. 

				if (bLooping)
					setState(LOOPEND);
				else
					setState(ENDOFFILE);
				break;
			}
		}
	}

	if (state == LOOPEND)
	{
		// Rewind if possible
		STATE newState = ENDOFFILE; 
		if (pimp)
		{
			LONG seekBack;
			if (SUCCEEDED(pimp->CanSeekBackward (&seekBack)))
			{
				if (seekBack == OATRUE)
				{
					// Go back to the beginning on the next run.
					if (SUCCEEDED(pimp->put_CurrentPosition (0)))
					{
						newState = PLAY;
					}
				}
			}
		}

		setState(newState);
	}

	if (state == PLAY)
	{
		// NOTE: S_OK and S_FALSE are both valid return values here. S_OK indicates that the graph is already running,
		// while S_FALSE indicates that the graph is preparing to run and will automatically run when it is
		// ready.
		hr = pimc->Run();
		if (hr != S_OK && hr != S_FALSE)
		{
			CQTRACE10("pimc->Run() failed");
			setState(OFF);
		}
	}

	if (state == ENDOFFILE)
	{
		// The messages EOFREACHED and COMPLETED are sent at different times, to allow for the delay between the
		// end of reading the file and the end of playing the read data.
		// DirectShow simply indicates when the end of playback has been reached, so we don't really know when
		// EOFREACHED should be sent. Therefore, we will just transition from EOFREACHED to COMPLETED in the same
		// step.
		setState (ENDOFFILE3);
	}
}

//--------------------------------------------------------------------------//
//
struct Music : IStreamer, IAggregateComponent
{
	BEGIN_DACOM_MAP_INBOUND(Music)
	DACOM_INTERFACE_ENTRY(IStreamer)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IStreamer,IStreamer)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

	//-------------------------------
	// data items
	//-------------------------------
	
	volatile U8 threadStatus;	// bit 0 set if running
								// bit 1 set if shutdown
								// bit 7 set if shutdown requested
	HANDLE hThread;
	HANDLE hWakeUp;                     // resettable event that will wake up the music handling thread.
	volatile Streamer * playList;		// list of streamers that are playing
	volatile Streamer * inactiveList;	// list of streamers that are in some other state
								// the 'inactiveList' is guarded by the criticalSection

	mutable	CRITICAL_SECTION criticalSection;

	//--------------------------------

	Music (void);

	~Music (void);

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "Music");
	}

	/* IStreamer methods */

	DEFMETHOD_(BOOL32,Init) (STREAMERDESC * desc);

	DEFMETHOD_(HSTREAM,Open) (const char * filename, struct IFileSystem * parent, DWORD flags);

	DEFMETHOD_(BOOL32,CloseHandle) (HSTREAM hMusic);

	DEFMETHOD_(BOOL32,Stop) (HSTREAM hMusic);

	DEFMETHOD_(BOOL32,Restart) (HSTREAM hMusic);

	DEFMETHOD_(BOOL32,SetVolume) (HSTREAM hMusic, S32 volume);

	DEFMETHOD_(BOOL32,GetVolume) (HSTREAM hMusic, S32 * volume) const;

	DEFMETHOD_(STATUS,GetStatus) (HSTREAM hMusic) const;

	/* IAggregateComponent methods */

	DEFMETHOD(Initialize) (void);
		
	/* Music methods */

	void wait (int msectimeout);
	void main (void);

	BOOL32 updateList (Streamer * streamer);

	void exchange (void);

	BOOL32 verify (HSTREAM hMusic) const;

	static DWORD WINAPI threadMain (LPVOID lpThreadParameter)
	{
		((Music *)lpThreadParameter)->main();
		
		return 0;
	}

	GENRESULT init (AGGDESC * desc)
	{
		return GR_OK;
	}

	IDAComponent * getBase (void)
	{
		return static_cast<IStreamer *>(this);
	}
};
//--------------------------------------------------------------------------//
//
Music::Music (void)
{
}
//--------------------------------------------------------------------------//
//
Music::~Music (void)
{
	//
	// shutdown thread
	//
	if (hThread)
	{
		threadStatus |= 0x80;		// signal for thread to terminate
		WaitForSingleObject(hThread, INFINITE);
		::CloseHandle(hThread);
		hThread = 0;
	
		DeleteCriticalSection(&criticalSection);

		::CloseHandle(hWakeUp);
	}

	//
	// delete Streamer lists
	// 
	volatile Streamer * node;

	while ((node = playList) != 0)
	{
		playList = node->pNext;
		delete node;
	}
	
	while ((node = inactiveList) != 0)
	{
		inactiveList = node->pNext;
		delete node;
	}
}
//--------------------------------------------------------------------------//
// return true if a Streamer needs to be moved off to another list
//
BOOL32 Music::updateList (Streamer * streamer)
{
	// This is the state table manager for the list of streamers.
	BOOL32 result = 0;

	while (streamer)
	{
		switch (streamer->state)
		{
		case INIT:
			streamer->init();
			break;
		case START:
			streamer->start();
			break;
		case PLAY:
		case LOOPEND:
		case ENDOFFILE:
		case ENDOFFILE1:
		case ENDOFFILE2:
			streamer->play();
			break;
		default:
			result = 1;
		}

		streamer = streamer->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
// move members of lists around as appropriate
//
void Music::exchange (void)
{
	Streamer * node = (Streamer *) playList, *prev = 0;

	//
	// move elements of playList to the inactiveList
	//
	while (node)
	{
		switch (node->state)
		{
		case INIT:
		case START:
		case PLAY:
		case LOOPEND:
		case ENDOFFILE:
		case ENDOFFILE1:
		case ENDOFFILE2:
			prev = node;
			node = node->pNext;
			break;
		case ENDOFFILE3:
			node->free();
			// fall through intentional
		default:	// else not active, move to other list
			if (node->bKillRequested)
				node->free();		// free resources
			if (prev)
			{
				prev->pNext = node->pNext;
				node->pNext = (Streamer *) inactiveList;
				inactiveList = node;
				node = prev->pNext;
			}
			else
			{
				playList = (Streamer *) node->pNext;
				node->pNext = (Streamer *) inactiveList;
				inactiveList = node;
				node = (Streamer *) playList;
			}
			break;
		}
	}

	//
	// move elements from the inactiveList to the playList
	//
	node = (Streamer *) inactiveList;
	prev = 0;

	while (node)
	{
		switch (node->state)
		{
		case PLAY:
			if (node->bStopRequested==0 && node->pimc!=0)
				node->pimc->Run();
			// fall through intentional
		case INIT:
		case START:
		case LOOPEND:
		case ENDOFFILE:
		case ENDOFFILE1:
		case ENDOFFILE2:	// node is active, move to the active list
			if (prev)
			{
				prev->pNext = node->pNext;
				node->pNext = (Streamer *) playList;
				playList = node;
				node = prev->pNext;
			}
			else
			{
				inactiveList = (Streamer *) node->pNext;
				node->pNext = (Streamer *) playList;
				playList = node;
				node = (Streamer *) inactiveList;
			}
			break;
		default:	// else not active, keep in this list
			prev = node;
			node = node->pNext;
			break;
		}
	}
}
//--------------------------------------------------------------------------//
//
void Music::wait (int msectimeout)
{	
	// This will cause the current thread to wait for something to happen
	// in the music class.
	// WARNING: Only call this from the second thread!!!

	Streamer *stream = (Streamer *) playList;
	
	static int waitHandleCount = 0;
	static HANDLE *waitHandles = NULL;

	int count = 1;  // there is always the hWakeUp handle to wait for.
	while (stream)
	{
		++count;
		stream = stream->pNext;
	}

	if (count > waitHandleCount)
	{
		waitHandleCount = count;
		if (waitHandles)
		{
			delete [] waitHandles;
		}
		waitHandles = new HANDLE[waitHandleCount];
	}

	ASSERT_FATAL (waitHandles != NULL);
	waitHandles[0] = hWakeUp;
	stream = (Streamer *) playList;
	count = 1;
	while (stream)
	{
		waitHandles[count] = stream->hEventReady;
		++count;
		stream = stream->pNext;
	}

	// Now that we have a list of handles to wait for, wait for them, with a timeout.
	// We don't care why we stop waiting here.
	// *** TODO: Ensure that these handles have the appropriate access on WinNT/2000.

	WaitForMultipleObjects (waitHandleCount, waitHandles, FALSE, msectimeout);
}

//--------------------------------------------------------------------------//
//
void Music::main (void)
{
	// Initialize the COM library for this thread.
	bool releaseCom = false;

	if (CoInitialize(NULL) == S_OK)
	{
		releaseCom = true;
	}

	threadStatus |= 0x01;		// set active flag

	while ((threadStatus & 0x80) == 0)
	{
		// update everyone in the play list
		//
		if (updateList((Streamer *)playList) || inactiveList != 0)
		{
			EnterCriticalSection(&criticalSection);
			exchange();		// move Streamers to their appropriate list
			LeaveCriticalSection(&criticalSection);
		}

		// We can set this timeout larger if we use the hWakeUp event to tell the thread that it has to
		// service a primary thread request. Until that event is used, this timeout is the delay between
		// a primary thread action and the second thread's reaction.

		wait (1000);
	}

	//
	// clean up resources we allocated
	// 
	Streamer * node;
	node = (Streamer *)playList;
	while (node)
	{
		node->free();
		node = node->pNext;
	}
	
	node = (Streamer *)inactiveList;
	while (node)
	{
		node->free();
		node = node->pNext;
	}
	
	if (releaseCom)
	{
		CoUninitialize ();
		releaseCom = false;
	}

	threadStatus |= 0x02;		// thread has shutdown
}
//--------------------------------------------------------------------------//
//
BOOL32 Music::verify (HSTREAM hMusic) const
{
	Streamer * tmp = hMusic;
	Streamer * result;

	EnterCriticalSection(&criticalSection);

	result = (Streamer *) playList;
	while (result)
	{
		if (result == tmp)
			goto Done;
		result = result->pNext;
	}
	result = (Streamer *) inactiveList;
	while (result)
	{
		if (result == tmp)
			goto Done;
		result = result->pNext;
	}

Done:
	LeaveCriticalSection(&criticalSection);
	return (result != 0);
}
//---------------------------------------------------------------------
//
static bool isTrue (const char * buffer)
{
	if (strncmp(buffer, "1", 1) == 0 || strnicmp(buffer, "on", 2)==0 || strnicmp(buffer, "true", 4)==0)
		return true;
	else
		return false;
}
//--------------------------------------------------------------------------//
//
BOOL32 Music::Init (STREAMERDESC * desc)
{
	BOOL32 result = 0;
	U32 threadID;

	if (desc==0 || desc->size != sizeof(*desc))
		return 0;

	hMainWindow = desc->hMainWindow;
	uMsg = desc->uMsg;

	//
	// setup stuff for multi-threading
	//
	
	InitializeCriticalSection(&criticalSection);

	// This event is automatically reset, initially unsignalled, with no name and no security attributes.
	hWakeUp = CreateEvent (NULL, FALSE, FALSE, NULL);
	ASSERT_ERROR(hWakeUp);

	hThread = CreateThread(0,4096, (LPTHREAD_START_ROUTINE) threadMain, (LPVOID)this, 0, &threadID);
	ASSERT_ERROR(hThread);

	if (hThread==0 || hWakeUp==0)
	{
		return 0;
	}

	while ((threadStatus & 3) == 0)		// wait for thread to start
		;

	result=1;	

	return result;
}
//--------------------------------------------------------------------------//
//
HSTREAM Music::Open (const char * filename, struct IFileSystem * parent, DWORD flags)
{
	if (hThread)
	{
		Streamer * result = new Streamer;

		if (parent)
		{
			// WARNING: This will NOT work for anything but a DOS filesystem.
			if (filename)
			{
				// Build the filename from the combination of the parent filesystem and the filename.
				// *** The GetCurrentDirectory doesn't work if the parent file system is a directory. Sigh.
//				DWORD len = parent->GetCurrentDirectory(sizeof(result->filename)-1, result->filename);
				LONG len = parent->GetFileName(result->filename, sizeof(result->filename)-1);
				if (len != 0)
				{
					// The GetFileName() function writes beyond the NULL terminator, so the return value is actually
					// larger than the end of the file. Therfore, we work around by retrieving the length of the
					// filename before proceeding. Sigh.
					len = strlen(result->filename);
					if (result->filename[len-1] != '\\')
					{
						result->filename[len++] = '\\';
					}
					strncpy(result->filename+len, filename, sizeof(result->filename)-len-1);
					result->filename[sizeof(result->filename)-1] = '\0';
				}
				else
				{
					GENERAL_TRACE_1("Failed to get current directory from parent filesystem in DSStreamer::Open.\n");
					return (HSTREAM) 0;
				}
			}
			else
			{
				// Just use the filename of the parent file system.
				if (parent->GetFileName (result->filename, MAX_PATH) == 0)
				{
					GENERAL_TRACE_1("Failed to get filename from parent filesystem in DSStreamer::Open.\n");
					return (HSTREAM) 0;
				}
			}
		}
		else if (filename)
		{
			// Just use the raw filename.
			strncpy(result->filename, filename, sizeof(result->filename)-1);
		}
		else
		{
			// One or the other has to be set.
			GENERAL_WARNING("NULL filename and parent filesystem in DSStreamer::Open.\n");
			return 0;
		}

		result->state = INIT;
		result->bStopRequested = ((flags & STRMFL_PLAY) == 0);
		result->bLooping = ((flags & STRMFL_LOOPING) != 0);
		//
		// add new stream to the list
		//
		EnterCriticalSection(&criticalSection);
		
		result->pNext = (Streamer *) inactiveList;
		inactiveList = result;

		LeaveCriticalSection(&criticalSection);

		// Tell the second thread to wake up.
		SetEvent (hWakeUp);

		return result;
	}
	else
		return (HSTREAM) 0;
}
//--------------------------------------------------------------------------//
//
BOOL32 Music::CloseHandle (HSTREAM hMusic)
{
	if (hThread == 0 || hMusic == 0)
		return 0;

	Streamer * tmp = hMusic;
	Streamer * node, *prev = 0;

#ifdef _DEBUG
	if (verify(hMusic) == 0)
	{
		CQERROR0("Invalid handle.");
		return 0;
	}
#endif

	EnterCriticalSection(&criticalSection);
	tmp->bKillRequested = true;
	tmp->bStopRequested = true;
	if (tmp->state == OFF || tmp->state == STOP)
		tmp->state = PLAY;	// force music thread to consider this stream
	LeaveCriticalSection(&criticalSection);

	// Tell the second thread to wake up.
	SetEvent (hWakeUp);
	
	while (tmp->pimex != 0)
		Sleep(0);

	//
	// remove node from the list
	//
	EnterCriticalSection(&criticalSection);

	node = (Streamer *) inactiveList;
	while (node)
	{
		if (node == tmp)
		{
			if (prev)
				prev->pNext = node->pNext;
			else
				inactiveList = node->pNext;
			node->pNext = 0;
			break;
		}
		prev = node;
		node = node->pNext;
	}

	LeaveCriticalSection(&criticalSection);
	
	delete node;

	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 Music::Stop (HSTREAM hMusic)
{
	BOOL32 result = 0;
	Streamer * tmp = hMusic;

	if (hThread)
	{
	#ifdef _DEBUG
		if (verify(hMusic) == 0)
		{
			CQERROR0("Invalid handle.");
			return 0;
		}
	#endif

		if (tmp)
		{
			result = (tmp->pimex != 0);
			tmp->bStopRequested = true;

			// Tell the second thread to wake up.
			SetEvent (hWakeUp);
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 Music::Restart (HSTREAM hMusic)
{
	Streamer * tmp = hMusic;

	if (hThread)
	{
#ifdef _DEBUG
		if (verify(hMusic) == 0)
		{
			CQERROR0("Invalid handle.");
			return 0;
		}
#endif
		EnterCriticalSection(&criticalSection);
		tmp->bStopRequested = false;
		if (tmp->state == STOP)
		{
			tmp->state = PLAY;

			// Tell the second thread to wake up.
			SetEvent (hWakeUp);
		}
		LeaveCriticalSection(&criticalSection);
	}
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 Music::SetVolume (HSTREAM hMusic, S32 volume)
{
	Streamer * tmp = hMusic;
	BOOL32 result=0;

	if (hThread)
	{
#ifdef _DEBUG
		if (verify(hMusic) == 0)
		{
			CQERROR0("Invalid handle.");
			return 0;
		}
#endif

		tmp->volumeRequested = volume;

		EnterCriticalSection(&criticalSection);

		if (tmp->piba)
		{
			result = (tmp->piba->put_Volume(tmp->volumeRequested) == S_OK);

			// Tell the second thread to wake up.
			SetEvent (hWakeUp);
		}
		
		LeaveCriticalSection(&criticalSection);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 Music::GetVolume (HSTREAM hMusic, S32 * volume) const
{
	Streamer * tmp = hMusic;
	BOOL32 result=0;

	if (hThread)
	{
#ifdef _DEBUG
		if (verify(hMusic) == 0)
		{
			CQERROR0("Invalid handle.");
			return 0;
		}
#endif

		EnterCriticalSection(&criticalSection);

		if (tmp->piba)
		{
			LONG dsvolume;

			result = (tmp->piba->get_Volume(&dsvolume) == S_OK);

			*volume = dsvolume;
		}
		else
		{
			*volume = tmp->volumeRequested;
			result = 1;
		}

		LeaveCriticalSection(&criticalSection);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
Music::STATUS Music::GetStatus (HSTREAM hMusic) const
{
	STATUS result = INVALID;
	Streamer * tmp = hMusic;

	if (hThread)
	{
#ifdef _DEBUG
		if (verify(hMusic) == 0)
		{
			CQERROR0("Invalid handle.");
			return INVALID;
		}
#endif

		switch (tmp->state)
		{
		case STOP:
			result = STOPPED;
			break;
  
		case INIT:
		case START:
		case PLAY:
		case LOOPEND:
			result = PLAYING;
			break;

		case ENDOFFILE:
		case ENDOFFILE1:
		case ENDOFFILE2:
			result = EOFREACHED;
			break;

		case ENDOFFILE3:
			result = COMPLETED;
			break;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
GENRESULT Music::Initialize (void)
{
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void SetDllHeapMsg (HINSTANCE hInstance)
{
   DWORD dwLen;
   char buffer[260];
   
   dwLen = GetModuleFileName(hInstance, buffer, sizeof(buffer));
 
   while (dwLen > 0)
   {
      if (buffer[dwLen] == '\\')
      {
         dwLen++;
         break;
      }
      dwLen--;
   }

   SetDefaultHeapMsg(buffer+dwLen);
}
void main (void)
{
}
//****************************************************************************
//*                                                                          *
//*  DLLMain() called on startup/shutdown                                    *
//*                                                                          *
//****************************************************************************
//
BOOL COMAPI DllMain(HINSTANCE hinstDLL,  //)
                    DWORD     fdwReason,
                    LPVOID    lpvReserved)
{
   IComponentFactory *server;
   static char interface_name[] = "IStreamer";

   switch (fdwReason)
      {
      //
      // DLL_PROCESS_ATTACH: Create object server component and register it 
      // with DACOM manager
      //

      case DLL_PROCESS_ATTACH:

			HEAP = HEAP_Acquire();
			SetDllHeapMsg(hinstDLL);

			server = new DAComponentFactory2<DAComponentAggregate<Music>, AGGDESC> (interface_name);
			DACOM_Acquire()->RegisterComponent(server, interface_name, DACOM_LOW_PRIORITY);
			server->Release();

			break;

      case DLL_PROCESS_DETACH:
         break;
      }

   return TRUE;
}

//--------------------------------------------------------------------------//
//-------------------------End DShowStreamer.cpp----------------------------//
//--------------------------------------------------------------------------//
