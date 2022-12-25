#ifndef VOICE_IO_H
#define VOICE_IO_H

///////////////////////////////////////////////////////////////////////

#include <afxwin.h>
#include <afxmt.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

///////////////////////////////////////////////////////////////////////

struct IDirectSound;
struct IDirectSoundBuffer;
struct IDirectSoundCapture;
struct IDirectSoundCaptureBuffer;
struct IDirectSoundNotify;

///////////////////////////////////////////////////////////////////////

class VoiceInput
{
public:
	//
	// VoiceInput()
	//		Used to sample the DirectSound input device to create
	//		compressed voice data.  Note that the VoiceInput instance
	//		requires dedicated use of the IDirectSoundCapture device
	//		since it cannot be shared between applications.  Only
	//		once VoiceInput instance can successfully exist at one
	//		time.
	//

	VoiceInput();

	//
	//	~VoiceInput()
	//		Releases all resources used, including the
	//		IDirectSoundCapture device.
	//

	virtual ~VoiceInput();

	//
	// Start()
	//		Initiates sampling and compression.  The results are
	//		made available by overloading OnInputReady().
	//

	void Start();

	//
	// Stop()
	//		Ceases sampling and compression, but does not deallocate
	//		any resources.
	//

	void Stop();

	//
	// OnInputReady()
	//		A pure virtual function that must be overloaded to
	//		receive the resulting compressed voice data.  Each
	//		buffer must eventually be delivered in its entirity
	//		in a single operation to the VoiceOutput's Output()
	//		function.  Fragmented buffer deliveries will cause
	//		major audio anomalies.
	//

	virtual void OnInputReady(void* buffer, DWORD bufferSize) = 0;

	//
	// GetLastError()
	//		Returns a string briefly describing the last error.  This
	//		is really intended only for application debugging as the
	//		messages are not intended for the end user.  If there was
	//		no error, the resulting string begins with a null character.
	//

	const char* GetLastError() { return lastErrorString; }

private:
	static DWORD WINAPI InputThread(LPVOID lpParameter);

	void LoadInput(DWORD byteCount);

	void Error(const char*, int = 0);

	static VoiceInput* voiceInput;
	static HANDLE event[2];
	static BOOL runThread;
	static BOOL threadRunning;

	IDirectSoundCapture* 		soundInterface; 
	IDirectSoundCaptureBuffer*	soundBuffer;
	IDirectSoundNotify*			soundNotifier;

   HACMSTREAM			streamHandle;
	LPACMSTREAMHEADER	streamHeader;

	char* inputRemaining;
	DWORD inputRemainingCount;

	DWORD inputBufferSize;
	DWORD outputBufferSize;

	BOOL capturingFlag;

	char lastErrorString[80];
};

///////////////////////////////////////////////////////////////////////

class VoiceOutput
{
public:
	//
	// VoiceOutput()
	//		Used to buffer, decompress, and stream voice output
	//		through the DirectSound interfaces.  This version
	//		uses an existing IDirectSound instance.
	//
	VoiceOutput(IDirectSound& directSoundInstance);

	//
	// VoiceOutput()
	//		Used to buffer, decompress, and stream voice output
	//		through the DirectSound interfaces.  This version
	//		creates a new IDirectSound instance.
	//
	//	window: Required to set the cooperative level of this
	//		application's use of the DirectSound output device.
	//

	VoiceOutput(CWnd& window);

	//
	// ~VoiceOutput
	//		Releases all resources used, including the
	//		IDirectSound output device.
	//

	virtual ~VoiceOutput();

	//
	// Start()
	//		Begins processing of the queued compressed data that
	//		will be received via calls to the Output() function.
	//		Any data that may have already been queued is flushed.
	//

	void Start();

	//
	// Stop()
	//		Stops processing of queued and incoming data immediately.
	//		Any existing queued data is flushed.  No more data will
	//		be queued until Start() is called.  Any incoming data
	//		will be discarded.
	//

	void Stop();

	//
	// Output()
	//		Loads a buffer previously presented by VoiceInput's
	//		OnInputReady() function into the output queue to be
	//		streamed to the DirectSound output device.  This
	//		data is only processed after the Start() function is
	//		called.
	//

	void Output(void* buffer, DWORD bufferSize);

	//
	// GetLastError()
	//		Returns a string briefly describing the last error.  This
	//		is really intended only for application debugging as the
	//		messages are not intended for the end user.  If there was
	//		no error, the resulting string begins with a null character.
	//

	const char* GetLastError() { return lastErrorString; }

private:
	static DWORD WINAPI OutputThread(LPVOID lpParameter);

	void Initialize(CWnd*, IDirectSound* directSoundInstance);

	void LoadOutput();

	void Error(const char*, int = 0);
	
	static VoiceOutput* voiceOutput;
	static BOOL runThread;
	static BOOL threadRunning;
	static CMutex mutex;

	IDirectSound*			soundInterface; 
	IDirectSoundBuffer*	soundBuffer;

   HACMSTREAM			streamHandle;
	LPACMSTREAMHEADER	streamHeader;

	DWORD nextPosition;

	DWORD inputBufferSize;
	DWORD outputBufferSize;

	char* queue;
	DWORD freeIndex;
	DWORD usedIndex;
	DWORD queueUsed;
	DWORD queueSize;

	bool stopped;

	char lastErrorString[80];
};

///////////////////////////////////////////////////////////////////////


#endif