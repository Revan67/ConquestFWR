#include "VoiceIO.h"

#include <dsound.h>

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifndef WAVE_FORMAT_MSRT24
#define WAVE_FORMAT_MSRT24 0x0082
#endif

#ifndef VOXWARE_KEY
#define VOXWARE_KEY "35243410-F7340C0668-CD78867B74DAD857-AC71429AD8CAFCB5-E4E1A99E7FFD-371"
#endif

#pragma pack(1)	// Byte pack this structure

#ifndef VOXACM_WAVEFORMATEX
typedef struct tagVOXACM_WAVEFORMATEX 
{
	WAVEFORMATEX	wfx;
	DWORD				dwCodecId;
	DWORD				dwMode;
	char				szKey[72];
} VOXACM_WAVEFORMATEX, *PVOXACM_WAVEFORMATEX, FAR *LPVOXACM_WAVEFORMATEX;
#endif

#pragma pack()		// Restore normal packing


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

VoiceInput* VoiceInput::voiceInput = NULL;
HANDLE VoiceInput::event[2] = { NULL, NULL };
BOOL VoiceInput::runThread = FALSE;
BOOL VoiceInput::threadRunning = FALSE;

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

VoiceInput::VoiceInput() :
   soundInterface(NULL),
	soundBuffer(NULL),
	soundNotifier(NULL),
	streamHandle(NULL),
	streamHeader (NULL),
	inputRemaining(NULL),
	inputRemainingCount(0),
	inputBufferSize(0),
	capturingFlag(FALSE)
{
	voiceInput = this;

	HRESULT  hr;
	MMRESULT mmr;

	memset(lastErrorString, '\0', sizeof lastErrorString);

	WAVEFORMATEX* mikeFormat = new WAVEFORMATEX;
	WAVEFORMATEX* compressedFormat = NULL;

	//
	// Wave format definition for 8000hz 16 bit mono.
	// RT24 requires this input format.
	//

	mikeFormat->wFormatTag = WAVE_FORMAT_PCM;
   mikeFormat->nChannels = 1;
	mikeFormat->nSamplesPerSec = 8000; 
   mikeFormat->nBlockAlign = 2;
	mikeFormat->nAvgBytesPerSec = 16000;
	mikeFormat->wBitsPerSample = 16;
	mikeFormat->cbSize = 0;

	//
	// Allocate a buffer large enough to hold max size format data
	//

	DWORD compressedFormatSize;
	acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &compressedFormatSize);

	//
	// Lookup up the wave format settings for the RT24 format
	//

	compressedFormat = (WAVEFORMATEX*)new char[compressedFormatSize];

	memset(compressedFormat, 0, compressedFormatSize);
	compressedFormat->wFormatTag = WAVE_FORMAT_MSRT24;

	mmr = acmFormatSuggest(NULL, mikeFormat, compressedFormat, compressedFormatSize,
		                    ACM_FORMATSUGGESTF_WFORMATTAG);
	if (mmr)
	{
		Error("acmFormatSuggest() failed: %d\n", mmr);
		goto errorCleanup;
	}

	//
	// Set the license key part of the RT24 wave format structure
	//
	
	if (compressedFormatSize >= sizeof(VOXACM_WAVEFORMATEX))
	{
		VOXACM_WAVEFORMATEX* voxFormat = (VOXACM_WAVEFORMATEX*)compressedFormat;
		strcpy(voxFormat->szKey,VOXWARE_KEY);
	}
	
	//
	// Open the ACM stream needed to convert from the mike format
	// to the compressed format
	//

	if (mmr = acmStreamOpen(&streamHandle, NULL, mikeFormat, compressedFormat,
		                     NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME))
	{
		Error("acmStreamOpen() failed: %d\n", mmr);
		goto errorCleanup;
	}

	//
	// 2-second input/output buffers (2x1 second half-buffers)
	//
	
	inputBufferSize = mikeFormat->nAvgBytesPerSec * 2;
	outputBufferSize = compressedFormat->nAvgBytesPerSec * 2;

	//
	// Initialize buffer to hold source data left unprocessed during a conversion
	//

	inputRemaining = new char[inputBufferSize];

	//
	// Prepare header describing conversion buffers
	//

	streamHeader = new ACMSTREAMHEADER;

	memset(streamHeader, 0, sizeof *streamHeader);
	streamHeader->cbStruct = sizeof *streamHeader;
	streamHeader->pbSrc = new UCHAR[inputBufferSize];
	streamHeader->cbSrcLength = inputBufferSize;
	streamHeader->cbSrcLengthUsed = 0;
	streamHeader->pbDst = new UCHAR[outputBufferSize];
	streamHeader->cbDstLength = outputBufferSize;
	streamHeader->cbDstLengthUsed = 0;

	if (mmr = acmStreamPrepareHeader(streamHandle, streamHeader, 0))
	{
		Error("acmStreamPrepareHeader() failed: %d\n", mmr);
		goto errorCleanup;
	}

	//
	// Create our DirectSound capture interface.
	//

	hr = DirectSoundCaptureCreate(NULL, &soundInterface, NULL); 

	if (hr != DS_OK)
	{
		Error("DirectSoundCaptureCreate() failed: hr = %d\n", hr);
		goto errorCleanup;
	}

	//
	// Prepare the direct sound capture buffer
	//

	DSCBUFFERDESC bufferDesc;

	memset(&bufferDesc, 0, sizeof(DSCBUFFERDESC));
	bufferDesc.dwSize = sizeof(DSCBUFFERDESC);

	bufferDesc.dwBufferBytes = inputBufferSize;
	bufferDesc.lpwfxFormat = mikeFormat;

	hr = soundInterface->CreateCaptureBuffer(&bufferDesc, &soundBuffer, NULL);
	
	if(hr != DS_OK)
	{ 
		Error("CreateCaptureBuffer() failed: hr = %d\n", hr);
		goto errorCleanup;
	}

	//
	// The format descriptions are no longer needed... clean 'em up
	//

	delete mikeFormat; mikeFormat = NULL;
	delete compressedFormat; compressedFormat = NULL;

	//
	// Access the interface to the direct sound soundNotifier, used to signal
	// the input thread each time the input buffer becomes half empty
	//

	hr = soundBuffer->QueryInterface(IID_IDirectSoundNotify,
		                              (void**)&soundNotifier);
	
	if(hr != DS_OK)
	{ 
		Error("QueryInterface() failed: hr = %d\n", hr);
		goto errorCleanup;
	}

	event[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	event[1] = CreateEvent(NULL, FALSE, FALSE, NULL);

	DWORD threadId;

	if (CreateThread(NULL, 0, InputThread, NULL, 0, &threadId) == NULL)
	{
		Error("CreateThread() failed\n");
		goto errorCleanup;
	}

	//
	// Instruct soundNotifier to signal when the capture position passes
	// the middle of the buffer and the end of the buffer
	//

	DSBPOSITIONNOTIFY notifications[2];

	notifications[0].dwOffset = 0;
	notifications[0].hEventNotify = event[0];

	notifications[1].dwOffset = bufferDesc.dwBufferBytes/2;
	notifications[1].hEventNotify = event[1];

	hr = soundNotifier->SetNotificationPositions(
		               sizeof notifications/sizeof notifications[0],
		               notifications);

	if(hr != DS_OK)
	{ 
		Error("SetNotificationPositions() failed: hr = %d\n", hr);
		goto errorCleanup;
	}

	return;

//
// Error Handler
//

errorCleanup:

	if (threadRunning)
	{
		runThread = FALSE;
		while (threadRunning) Sleep(0);
	}

	if (soundInterface)
	{
		soundInterface->Release(); soundInterface = NULL;
	}
	//if (soundNotifier)
	//{
		//soundNotifier->Release(); soundNotifier = NULL;
	//}
	if (streamHandle)
	{
		acmStreamClose(streamHandle, 0); streamHandle = NULL;
	}
	if (streamHeader)
	{
		acmStreamUnprepareHeader(streamHandle, streamHeader, 0);
		delete streamHeader; streamHeader = NULL;
	}
	if (event[0])
	{
		CloseHandle(event[0]); event[0] = NULL;
		CloseHandle(event[1]); event[1] = NULL;
	}
	delete mikeFormat; mikeFormat = NULL;
	delete compressedFormat; compressedFormat = NULL;
	delete inputRemaining; inputRemaining = NULL;

	voiceInput = NULL;
}

///////////////////////////////////////////////////////////////////////

VoiceInput::~VoiceInput()
{
	if (threadRunning)
	{
		runThread = FALSE;
		while (threadRunning) Sleep(100);
	}

	//
	// Cleanup all remaining resources
	//

	if (event[0])
	{
		CloseHandle(event[0]); event[0] = NULL;
		CloseHandle(event[1]); event[1] = NULL;
	}
	if (soundInterface)
	{
		soundInterface->Release(); soundInterface = NULL;
	}
	//if (soundNotifier)
	//{
	//	soundNotifier->Release(); soundNotifier = NULL;
	//}
	if (streamHandle)
	{
		acmStreamClose(streamHandle, 0); streamHandle = NULL;
	}
	if (streamHeader)
	{
	   acmStreamUnprepareHeader(streamHandle, streamHeader, 0);
		delete streamHeader->pbSrc;
		delete streamHeader->pbDst;
		delete streamHeader; streamHeader = NULL;
	}
	delete inputRemaining; inputRemaining = NULL;

	voiceInput = NULL;
}

///////////////////////////////////////////////////////////////////////

void VoiceInput::Start()
{
	if (!voiceInput) return; // Return if not properly initialized

	//
	// Start capture using a circular buffer
	//
	soundBuffer->Start(DSCBSTART_LOOPING);

	capturingFlag = TRUE;
}

///////////////////////////////////////////////////////////////////////

void VoiceInput::Stop()
{
	if (!voiceInput) return; // Return if not properly initialized

	capturingFlag = FALSE;

	soundBuffer->Stop();
}

///////////////////////////////////////////////////////////////////////

DWORD WINAPI VoiceInput::InputThread(LPVOID lpParameter)
{
	runThread = TRUE;
	threadRunning = TRUE;

	DWORD result;

	while(runThread)
	{
		if ((result = WaitForMultipleObjects(2, event, FALSE, 500))
			              == WAIT_FAILED) break;

		if (result != WAIT_TIMEOUT)
		{
			voiceInput->LoadInput(-1);
		}
	}
	threadRunning = FALSE;
	runThread = FALSE;
	return 0;
}

///////////////////////////////////////////////////////////////////////

void VoiceInput::LoadInput(DWORD byteCount)
{
	LPVOID pointer1;
	DWORD  size1;
	LPVOID pointer2;
	DWORD  size2;
	DWORD  readPosition;

	if (byteCount == -1)
	{
		byteCount = inputBufferSize / 2;
	}

	soundBuffer->GetCurrentPosition(NULL, &readPosition);

	HRESULT hr = soundBuffer->Lock(readPosition, byteCount,
							             &pointer1, &size1, &pointer2, &size2, 0); 

	if (hr != DS_OK) 
	{
		Error("soundBuffer->Lock() Other error %d\n", hr);
		return;
	}

	//
	// First load any input data left over from a 
	// previous call to acmStreamConvert
	//

	if (inputRemainingCount > 0)
	{
		memcpy(streamHeader->pbSrc, inputRemaining, inputRemainingCount);
	}

	//
	// Then load the data from the first part of the capture buffer
	// into the conversion stream's input buffer
	//

	memcpy(streamHeader->pbSrc + inputRemainingCount, pointer1, size1);

	//
	// If the input data wraps around the end of the buffer, get that data
	//

	if (pointer2)
	{
		memcpy(streamHeader->pbSrc + inputRemainingCount + size1, pointer2, size2);
	}

	//
	// Release the part of the capture buffer that we just loaded
	//

	hr = soundBuffer->Unlock(pointer1, size1, pointer2, size2);

	//
	// Tell the converter how much data we stuffed into its input buffer,
	// then indicate that the is no more input left over from an earlier call.
	//

	streamHeader->cbSrcLength = size1 + size2 + inputRemainingCount;

	inputRemainingCount = 0;

	//
	// Let the ACM compress the data
	//

	MMRESULT mmr;

	if (mmr = acmStreamConvert(streamHandle, streamHeader,
		                        ACM_STREAMCONVERTF_BLOCKALIGN ))
	{
		Error("acmStreamConvert() failed %d\n", mmr);
		return;
	}

	//
	// Notify the user that compressed data is available
	//

	if (capturingFlag)
	{
		OnInputReady(streamHeader->pbDst, streamHeader->cbDstLengthUsed);
	}

	//
	// Remember any data that the converter did not handle this time
	//

	inputRemainingCount = streamHeader->cbSrcLength - 
			          streamHeader->cbSrcLengthUsed;

	if (inputRemainingCount)
	{
		memcpy(inputRemaining, streamHeader->pbSrc + streamHeader->cbSrcLengthUsed,
				 inputRemainingCount);
	}	             
}

///////////////////////////////////////////////////////////////////////

void VoiceInput::Error(const char* str, int code)
{
	sprintf(lastErrorString, str, code);
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

VoiceOutput* VoiceOutput::voiceOutput = NULL;
BOOL VoiceOutput::runThread = FALSE;
BOOL VoiceOutput::threadRunning = FALSE;
CMutex VoiceOutput::mutex;

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

VoiceOutput::VoiceOutput(CWnd& window)
{
	Initialize(&window, NULL);
}

///////////////////////////////////////////////////////////////////////

VoiceOutput::VoiceOutput(IDirectSound& soundInterface)
{
	Initialize(NULL, &soundInterface);
}

///////////////////////////////////////////////////////////////////////

void VoiceOutput::Initialize(CWnd* window,
								     IDirectSound* soundInterfaceIn)
{
   soundInterface = soundInterfaceIn;
	soundBuffer = NULL;
	streamHandle = NULL;
   streamHeader = NULL;
	nextPosition = 0;
	inputBufferSize = 0;
	outputBufferSize = 0;
	queue = NULL;
	freeIndex = 0;
	usedIndex = 0;
	queueUsed = 0;
	queueSize = 0;
	stopped = FALSE;

	voiceOutput = this;

	HRESULT  hr;
	MMRESULT mmr;

	memset(lastErrorString, '\0', sizeof lastErrorString);

	if (window == NULL && soundInterface == NULL)
	{
		strcpy(lastErrorString,
			    "CWnd and IDirectSound cannot both be NULL");
		goto errorCleanup;
	}

	WAVEFORMATEX* pcmFormat;
	pcmFormat = new WAVEFORMATEX;

	WAVEFORMATEX* compressedFormat;
	compressedFormat = NULL;

	//
	// Wave format definition for 8000hz 16 bit mono.
	// RT24 requires this output format.
	//

	pcmFormat->wFormatTag = WAVE_FORMAT_PCM;
   pcmFormat->nChannels = 1;
	pcmFormat->nSamplesPerSec = 8000; 
   pcmFormat->nBlockAlign = 2;
	pcmFormat->nAvgBytesPerSec = 16000;
	pcmFormat->wBitsPerSample = 16;
	pcmFormat->cbSize = 0;

	//
	// Create a ten second input queue
	//

	queueSize = pcmFormat->nAvgBytesPerSec * 10;
	queue = new char[queueSize];

	//
	// Allocate a buffer large enough to hold max size format data
	//

	DWORD dstSize;
	acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &dstSize);

	//
	// Lookup up the wave format settings for the RT24 format
	//

	compressedFormat = (WAVEFORMATEX*)new char[dstSize];

	memset(compressedFormat, 0, dstSize);
	compressedFormat->wFormatTag = WAVE_FORMAT_MSRT24;

	mmr = acmFormatSuggest(NULL, pcmFormat, compressedFormat, dstSize,
		                    ACM_FORMATSUGGESTF_WFORMATTAG);
	if (mmr)
	{
		Error("acmFormatSuggest() failed: %d\n", mmr);
		goto errorCleanup;
	}

	//
	// Set the license key part of the RT24 wave format structure
	//
	
	if (dstSize >= sizeof(VOXACM_WAVEFORMATEX))
	{
		VOXACM_WAVEFORMATEX* voxFormat = (VOXACM_WAVEFORMATEX*)compressedFormat;
		strcpy(voxFormat->szKey,VOXWARE_KEY);
	}
	
	//
	// Open the ACM stream needed to convert from the compressed format
	// to the PCM format
	//

	if (mmr = acmStreamOpen(&streamHandle, NULL, compressedFormat, pcmFormat, NULL,
		                     0, 0, ACM_STREAMOPENF_NONREALTIME))
	{
		Error("acmStreamOpen() failed: %d\n", mmr);
		goto errorCleanup;
	}

	//
	// 2 second input and output buffers
	//
	
	outputBufferSize = pcmFormat->nAvgBytesPerSec * 2;
	inputBufferSize = compressedFormat->nAvgBytesPerSec * 2;

	//
	// Prepare header describing conversion buffers.  Be able to convert
	// as much as 2 seconds worth of data, since in the worst case the
	// input buffer may have overshot its normal 1 second of data.
	//

	streamHeader = new ACMSTREAMHEADER;

	memset(streamHeader, 0, sizeof *streamHeader);
	streamHeader->cbStruct = sizeof *streamHeader;
	streamHeader->pbSrc = new UCHAR[inputBufferSize];
	streamHeader->cbSrcLength = inputBufferSize;
	streamHeader->cbSrcLengthUsed = 0;
	streamHeader->pbDst = new UCHAR[outputBufferSize];
	streamHeader->cbDstLength = outputBufferSize;
	streamHeader->cbDstLengthUsed = 0;

	if (mmr = acmStreamPrepareHeader(streamHandle, streamHeader, 0))
	{
		Error("acmStreamPrepareHeader() failed: %d\n", mmr);
		goto errorCleanup;
	}

	//
	// If the caller didn't provide a direct sound interface, create our own.
	// Otherwise, add a reference to the existing interface
	//

	if (!soundInterface)
	{
		hr = DirectSoundCreate(NULL, &soundInterface, NULL); 

		if (hr != DS_OK)
		{
			Error("DirectSoundCreate() failed: hr = %d\n", hr);
			goto errorCleanup;
		}
	}
	else
	{
		soundInterface->AddRef();
	}

	//
	// This sound requires no special priorities
	//

	hr = soundInterface->SetCooperativeLevel(window->m_hWnd, DSSCL_NORMAL);
	
	if (hr != DS_OK)
	{
		Error("SetCooperativeLevel() failed: hr = %d\n", hr);
		goto errorCleanup;
	}

	//
	// Describe the output buffer for direct sound
	//

	DSBUFFERDESC bufferDesc;

	memset(&bufferDesc, 0, sizeof(DSBUFFERDESC));
	bufferDesc.dwSize = sizeof(DSBUFFERDESC);

	bufferDesc.dwBufferBytes = outputBufferSize;
	bufferDesc.lpwfxFormat = pcmFormat;
	bufferDesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY;

	hr = soundInterface->CreateSoundBuffer(&bufferDesc, &soundBuffer, NULL);
	
	if(hr != DS_OK)
	{ 
		Error("CreateSoundBuffer() failed: hr = %d\n", hr);
		goto errorCleanup;
	}

	//
	// Don't need the format descriptions anymore
	//

	delete pcmFormat; pcmFormat = NULL;
	delete compressedFormat; compressedFormat = NULL;
	
	//
	// Access the interface to the direct sound soundNotifier, used to signal
	// the input thread each time the input buffer becomes half empty
	//

	soundBuffer->Restore();

	DWORD threadId;

	if (CreateThread(NULL, 0, OutputThread, NULL, 0, &threadId) == NULL)
	{
		Error("CreateThread() failed\n");
		goto errorCleanup;
	}

	return;

//
// Error Clean-up
//

errorCleanup:

	if (threadRunning)
	{
		runThread = FALSE;
		while (threadRunning) Sleep(0);
	}

	if (soundInterface)
	{
		soundInterface->Release(); soundInterface = NULL;
	}
	if (streamHandle)
	{
		acmStreamClose(streamHandle, 0); streamHandle = NULL;
	}
	if (streamHeader)
	{
	   acmStreamUnprepareHeader(streamHandle, streamHeader, 0);
		delete streamHeader->pbSrc;
		delete streamHeader->pbDst;
		delete streamHeader; streamHeader = NULL;
	}
	delete pcmFormat; pcmFormat = NULL;
	delete compressedFormat; compressedFormat = NULL;

	voiceOutput = NULL;
}

///////////////////////////////////////////////////////////////////////

VoiceOutput::~VoiceOutput()
{
	if (threadRunning)
	{
		runThread = FALSE;
		while (threadRunning) Sleep(100);
	}

	if (soundInterface)
	{
		soundInterface->Release(); soundInterface = NULL;
	}
	if (streamHandle)
	{
		acmStreamClose(streamHandle, 0); streamHandle = NULL;
	}
	if (streamHeader)
	{
	   acmStreamUnprepareHeader(streamHandle, streamHeader, 0);
		delete streamHeader->pbSrc;
		delete streamHeader->pbDst;
		delete streamHeader; streamHeader = NULL;
	}

	delete queue; queue = NULL;

	voiceOutput = NULL;
}

///////////////////////////////////////////////////////////////////////

void VoiceOutput::Start()
{
	mutex.Lock();
	freeIndex = 0;
	usedIndex = 0;
	queueUsed = 0;

	stopped = FALSE;
	mutex.Unlock();
}

///////////////////////////////////////////////////////////////////////

void VoiceOutput::Stop()
{
	soundBuffer->Stop();

	mutex.Lock();
	freeIndex = 0;
	usedIndex = 0;
	queueUsed = 0;

	stopped = TRUE;
	mutex.Unlock();
}

///////////////////////////////////////////////////////////////////////

void VoiceOutput::Output(void* buffer, DWORD bufferSize)
{
	if (!voiceOutput) return;  // Return if not properly initialized

	if (stopped) return;
	
	streamHeader->cbSrcLength = bufferSize;
	memcpy(streamHeader->pbSrc, buffer, bufferSize);

	MMRESULT mmr;

	if (mmr = acmStreamConvert(streamHandle, streamHeader, 0))
	{
		Error("acmStreamConvert() failed %d\n", mmr);
		return;
	}

	char* data = (char*)streamHeader->pbDst;
	DWORD lengthToStore = streamHeader->cbDstLengthUsed;
	
	//
	// Don't conflict with the Direct Sound output thread
	//

	mutex.Lock();

	//
	// Throw away data if the queue cannot hold all of the data
	//

	if (queueUsed + lengthToStore <= queueSize)
	{
		DWORD lengthToWrap = queueSize - freeIndex;

		DWORD length1 = (lengthToWrap > lengthToStore)
									? lengthToStore : lengthToWrap;
		DWORD length2 = (lengthToStore > length1)
									? (lengthToStore - length1) : 0;

		memcpy(queue + freeIndex, data, length1);
		freeIndex += length1;
		if (freeIndex >= queueSize) freeIndex -= queueSize;

		if (length2)
		{
			memcpy(queue + freeIndex, data + length1, length2);
			freeIndex += length2;
			if (freeIndex >= queueSize) freeIndex -= queueSize;
		}

		queueUsed += lengthToStore;

		//
		// If the Direct Sound buffer hasn't started yet,
		// get it loaded and playing.
		//

		DWORD status;
		
		soundBuffer->GetStatus(&status);

		if (!stopped &&
			 !(status & DSBSTATUS_PLAYING) &&
			  (queueUsed >= queueSize / 2))
		{
			LoadOutput();
		}
	}
	mutex.Unlock();
}

///////////////////////////////////////////////////////////////////////

DWORD WINAPI VoiceOutput::OutputThread(LPVOID lpParameter)
{
	runThread = TRUE;
	threadRunning = TRUE;

	while(runThread)
	{
		voiceOutput->mutex.Lock();
		voiceOutput->LoadOutput();
		voiceOutput->mutex.Unlock();

		Sleep(100);
	}
	threadRunning = FALSE;
	runThread = FALSE;
	return 0;
}

///////////////////////////////////////////////////////////////////////

void VoiceOutput::LoadOutput()
{
	if (!voiceOutput) return;

	if (stopped) return;

	LPVOID pointer1;
	DWORD  size1;

	DWORD playPosition;

	soundBuffer->GetCurrentPosition(&playPosition, NULL);

	DWORD status;
	
	soundBuffer->GetStatus(&status);

	if (status & DSBSTATUS_PLAYING)
	{
		//
		// Wait until the play position is in the other half-buffer,
		// then fill the half-buffer that just finished playing.
		// If the next half-buffer isn't ready, return and try
		// again later.
		//

		if (nextPosition == 0)
		{
			if (playPosition < outputBufferSize / 2) return;
		}
		else
		{
			if (playPosition >= outputBufferSize / 2) return;
		}
	}

	//
	// Obtain memory address of write block. The write block will never
	// consist of two parts since it will never wrap around the end of
	// the buffer (we always write exactly 1/2 the buffer each time).
	//

	HRESULT hr;

	hr = soundBuffer->Lock(nextPosition, outputBufferSize / 2,
							     &pointer1, &size1, NULL, NULL, 0); 

	//
	// If DSERR_BUFFERLOST is returned, restore and retry lock. 
	//

	if (hr == DSERR_BUFFERLOST)
	{         
		soundBuffer->Restore();

		hr = soundBuffer->Lock(nextPosition, outputBufferSize / 2, 
									  &pointer1, &size1, NULL, NULL, 0);
	}
	
	if (hr != DS_OK) 
	{
		Error("Lock() %d\n", hr);
		return;
	}

	if (size1 != outputBufferSize / 2)
	{
		Error("");
	}

	memset(pointer1, 0, size1);

	if (queueUsed > 0)
	{
		DWORD lengthToStore = outputBufferSize / 2;

		if (lengthToStore > queueUsed) lengthToStore = queueUsed;

		//
		// If we hit the end of the queue (it wraps), then we need
		// to move the queue data in two pieces
		//

		DWORD lengthToWrap = queueSize - usedIndex;

		DWORD length1 = (lengthToWrap > lengthToStore)
									? lengthToStore : lengthToWrap;
		DWORD length2 = (lengthToStore > length1)
									? (lengthToStore - length1) : 0;

		memcpy(pointer1, queue + usedIndex, length1);
		usedIndex += length1;

		//
		// If we wrapped on the input queue...
		//

		if (length2)
		{
			memcpy((char*)pointer1 + length1, queue, length2);
			usedIndex = length2;
		}
		queueUsed -= lengthToStore;
	}

	//
	// Release the data back to DirectSound.
	//

	hr = soundBuffer->Unlock(pointer1, size1, NULL, 0);

	if (!(status & DSBSTATUS_PLAYING))
	{
		hr = soundBuffer->SetCurrentPosition(nextPosition);
		hr = soundBuffer->Play(0, 0, DSBPLAY_LOOPING);
	}

	nextPosition += outputBufferSize / 2;

	if (nextPosition >= outputBufferSize)
	{
		nextPosition -= outputBufferSize;
	}
} 

///////////////////////////////////////////////////////////////////////

void VoiceOutput::Error(const char* str, int code)
{
	sprintf(lastErrorString, str, code);
}
