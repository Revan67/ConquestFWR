//--------------------------------------------------------------------------//
//                                                                          //
//                              Streamer.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Pbleisch $

   $Header: /Libs/dev/Src/Streamer/Streamer.cpp 14    3/21/00 4:30p Pbleisch $

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
#include "IProfileParser.h"

#include <mmsystem.h>
//#define INITGUID
#include <dsound.h>
#include <mmreg.h>
#include <msacm.h>

#define CHUNK_NAME(d0,d1,d2,d3) ((long(d3)<<24)+(long(d2)<<16)+(d1<<8)+d0)
#define CDALIGNMASK 0x7FF

static IDirectSound * DSOUND;
static HWND hMainWindow;
static UINT uMsg;
static SINGLE g_ReadBufferTime;		// in seconds
static SINGLE g_SoundBufferTime;	// in seconds
static U32 g_MaxIdleTime;			// in msecs

#define DEFAULT_READ_BUFFER_TIME  4.0F
#define DEFAULT_SOUND_BUFFER_TIME (1.0F / 4.0F)

#define CQERROR0(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_ERROR), "%s(%d) : "##exp "\n", __FILE__, __LINE__)
#define CQERROR1(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_ERROR), "%s(%d) : "##exp "\n", __FILE__, __LINE__, p1)
#define CQTRACE10(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "%s(%d) : "##exp "\n", __FILE__, __LINE__)
#define CQTRACE11(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "%s(%d) : "##exp "\n", __FILE__, __LINE__, p1)
#define CQTRACE12(exp,p1, p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "%s(%d) : "##exp "\n", __FILE__, __LINE__, p1, p2)
#define CQWARNING1(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_WARNING), "%s(%d) : "##exp "\n", __FILE__, __LINE__, p1)

#ifdef _DEBUG
#define CQTRACE50(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "%s(%d) : "##exp "\n", __FILE__, __LINE__)
#define CQTRACE51(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "%s(%d) : "##exp "\n", __FILE__, __LINE__, p1)
#else
#define CQTRACE50(exp)      ((void)0)
#define CQTRACE51(exp,p1)   ((void)0)
#endif
//--------------------------------------------------------------------------//
//
struct StreamBuffer
{
	U8	 	*pBuffer; 
	U8		*pReader;
	U8		*pWriter;
	U32		bufferSize, origBufferSize;
	S32		alignMask;

	StreamBuffer (void)
	{
		pBuffer = 0;
	}
	
	~StreamBuffer (void)
	{
		VirtualFree(pBuffer, 0, MEM_RELEASE);		// multithreaded release
	}

	void init (U32 size)
	{
		origBufferSize = bufferSize = size;
		pBuffer = (U8 *) VirtualAlloc(0, bufferSize, MEM_COMMIT, PAGE_READWRITE);	// multithreaded alloc
		dumpBuffer();	// set it to empty state
		alignMask = -1;
	}

	void setAlignment (U32 align)	// should only be called during beginning and ending of streaming
	{
		alignMask = ~(align - 1);
	}

	void dumpBuffer (void)		// sets buffer to empty state
	{
		pReader = pWriter = pBuffer;
	}

	U32 dataInBuffer (void)
	{
		if (pWriter < pReader)
			return bufferSize - DWORD(pReader) + DWORD(pWriter);
		return pWriter - pReader;
	}

	U32 addToBuffer (U8 ** pData, S32 origLength)
	{
		long avail = bufferSize - dataInBuffer() - 1;
		long length = __min(origLength, avail);

		length &= alignMask;
		if (length > 0)
		{
	 		avail = bufferSize - DWORD(pWriter) + DWORD(pBuffer);
			// avail = amount of data from write cursor to end of buffer

			avail = __min(avail, length);

			if (avail == 0)	// we are at the end of the buffer
			{	
				pWriter = pBuffer;
				avail = length;
			}
			else
			{
				avail &= alignMask;

				if (avail == 0)		// near the end of the buffer
				{
					bufferSize = DWORD(pWriter) - DWORD(pBuffer);
					pWriter = pBuffer;
					// avail = length;
					avail = bufferSize - dataInBuffer() - 1;
					// avail is the amount of data we can now write to the buffer
					length = __min(origLength, avail);
					length &= alignMask;
					avail = length;
				}
			}

			*pData = pWriter;
			return avail;
		}
		return 0;
	}

	U32 removeFromBuffer (U8 ** pData, S32 length)
	{
		long avail = dataInBuffer();
		
		length = __min(length, avail);
		if (length > 0)
		{
	 		avail = bufferSize - DWORD(pReader) + DWORD(pBuffer);
			// avail = amount of data from read cursor to end of buffer
			avail = __min(avail, length);
			if (avail == 0)
			{
				pReader = pBuffer;
				avail = length;
				bufferSize = origBufferSize;
			}

			*pData = pReader;
			pReader += avail;

			return avail;
		}
		return 0;
	}
};
//--------------------------------------------------------------------------//
//
enum STATE
{
	OFF=0,
	INIT,
	START,
	PLAY,
	RESTART,			// main thread request to restart the playback
	LOOPEND,
	ENDOFFILE,
	ENDOFFILE1,
	ENDOFFILE2,
	ENDOFFILE3,
	STOP,
	RESET				// main thread request to free resources
};
//--------------------------------------------------------------------------//
//
struct Streamer
{
	struct Streamer * pNext;
	volatile bool bStopRequested;
	volatile bool bKillRequested;
	volatile bool bInPlayList;
	volatile S32 volumeRequested;
	STATE state;
	char filename[MAX_PATH];
	COMPTR<IFileSystem> pFile;		// parent file system
	HANDLE hFile;					// file opened without system buffering
	COMPTR<IDirectSoundBuffer> lpDSBuffer;
	OVERLAPPED overlapped;			// used for asynchronous I/O
	BOOL32     bIOPending;			// true if waiting on I/O
	U32		   overlappedBytesRead;	// number of bytes read in the background
	
	WAVEFORMATEX	* pInputWaveFormat;
	WAVEFORMATEX	outputWaveFormat;

	U32		soundDataSize;		// bytes of raw data in the file
	U32		soundDataStart;		// offset where data starts
	U32		soundDataPos;		// offset where next read should start

	U32		halfBufferSize;		// half of the size of the dsound buffer
	BOOL32	whichHalf;			// 0 or 1. 0 means reader is in first half

	//---------------------------
	// looping data
	//---------------------------
	BOOL32 bLooping;
	LPVOID lpLoopData;
	DWORD  dwLoopDataSize;
	
	//---------------------------
	// ACM data (used for decompression)
	//---------------------------
	HACMSTREAM hACMStream;
	ACMSTREAMHEADER  acmHeader;
	U32 acmOrigSrcSize;

	//---------------------------

	StreamBuffer rusty, raw;

	//---------------------------

	Streamer (void);

	~Streamer (void) { free(); }
	
	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "Streamer");
	}
	void operator delete( void *ptr )
	{
		HEAP->FreeMemory(ptr);
	}

	S32 readLong (HANDLE handle)
	{
		DWORD dwBytesRead;
		S32 result;

		pFile->ReadFile(handle, &result, sizeof(result), &dwBytesRead, 0);

		return result;
	}

	S32 bytesLeftToRead (void)
	{
		return soundDataSize + soundDataStart - soundDataPos;
	}

	void free (void);		// free allocated resources
	void init (void);
	void start (void);		// start playing the file
	void play (void);		// continue playing the file
	void fillHalfBuffer (BOOL32 which);		// fill the side that the reader is NOT on
	void fillDecompressionChamber (void);		// decompress data
	void fillCompressionChamber (void);
	void moveDecompressedData (void);

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
	hFile = INVALID_HANDLE_VALUE;
	volumeRequested = 0;
}
//--------------------------------------------------------------------------//
// free things allocated in this thread
//
void Streamer::free (void)
{
	DWORD dwBytesRead;

	lpDSBuffer.free();

	if (bIOPending)
		pFile->GetOverlappedResult(hFile, &overlapped, &dwBytesRead, 1);
	if (hFile != INVALID_HANDLE_VALUE)
		pFile->CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;
	bIOPending = 0;
	if (lpLoopData)
	{
		GlobalFree(lpLoopData);
		lpLoopData = 0;
	}
	if (pInputWaveFormat)
	{
		GlobalFree(pInputWaveFormat);
		pInputWaveFormat = 0;
	}
	if (hACMStream)		// compression data
	{
		acmHeader.cbSrcLength = acmOrigSrcSize;
		if (acmHeader.fdwStatus != 0)
			acmStreamUnprepareHeader(hACMStream, &acmHeader, 0);
		if (acmHeader.pbSrc)
		{
			VirtualFree(acmHeader.pbSrc, 0, MEM_RELEASE);		// multithreaded release
			acmHeader.pbSrc = 0;
		}
		if (acmHeader.pbDst)
		{
			VirtualFree(acmHeader.pbDst, 0, MEM_RELEASE);
			acmHeader.pbDst = 0;
		}
		acmStreamClose(hACMStream, 0);
		hACMStream = 0;
	}
}
//--------------------------------------------------------------------------//
// read the wav header, create a sound buffer
// allocate secondary buffers,
// start the buffer in filling mode
//
//
void Streamer::init (void)
{
	S32 chunk_length, chunk_name;
	U32 riff_size;
	U32 dwBytesRead;
	DSBUFFERDESC dsdesc;
	HRESULT hr;
	U32 initialRead;
	U8 * pBuffer;
	DAFILEDESC fdesc = filename;
	HANDLE hBuffered;		// handle to buffered version of file
	STATE newState = OFF;	// assume failure
	MMRESULT mmr;

	if (filename[0] == 0)
		hBuffered = 0;
	else
	if ((hBuffered = pFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		goto Done;
	}

	if ((chunk_name = readLong(hBuffered)) != CHUNK_NAME('R','I','F','F'))
		goto Done;
	riff_size = readLong(hBuffered);
	if ((chunk_name = readLong(hBuffered)) != CHUNK_NAME('W','A','V','E'))
		goto Done;
	while ((chunk_name = readLong(hBuffered)) != CHUNK_NAME('f','m','t',' '))
	{
		chunk_length = (readLong(hBuffered) + 1) & ~1;
		if (pFile->GetFilePosition(hBuffered) + chunk_length >= riff_size)
			goto Done;
		pFile->SetFilePointer(hBuffered, chunk_length, 0, FILE_CURRENT);	// skip over this chunk
	}
	chunk_length = (readLong(hBuffered) + 1) & ~1;

	if ((pInputWaveFormat = (WAVEFORMATEX *) GlobalAlloc(GMEM_FIXED, __max(sizeof(WAVEFORMATEX),chunk_length))) == 0)
	{
		CQTRACE10("GlobalAlloc failed");
		goto Done;
	}

	pFile->ReadFile(hBuffered, pInputWaveFormat, chunk_length, &dwBytesRead, 0);
	pFile->SetFilePointer(hBuffered, chunk_length - dwBytesRead, 0, FILE_CURRENT);	// skip to next chuck
	
	while ((chunk_name = readLong(hBuffered)) != CHUNK_NAME('d','a','t','a'))
	{
		chunk_length = (readLong(hBuffered) + 1) & ~1;
		if (pFile->GetFilePosition(hBuffered) + chunk_length >= riff_size)
			goto Done;
		pFile->SetFilePointer(hBuffered, chunk_length, 0, FILE_CURRENT);	// skip to next chunk
	}
	soundDataSize = readLong(hBuffered);
	soundDataStart = soundDataPos = pFile->GetFilePosition(hBuffered);

	if (soundDataStart + soundDataSize > pFile->GetFileSize(hBuffered))
		soundDataSize = pFile->GetFileSize(hBuffered) - soundDataPos;
	
	// are we going to need decompression ?
	if (pInputWaveFormat->wFormatTag != WAVE_FORMAT_PCM)
	{
		outputWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	
		mmr = acmFormatSuggest(NULL, pInputWaveFormat, &outputWaveFormat, sizeof(WAVEFORMATEX), ACM_FORMATSUGGESTF_WFORMATTAG);
		if (mmr) 
		{
			CQTRACE11("ACM error #%08X", mmr);
			goto Done;
		}

		mmr = acmStreamOpen(&hACMStream, NULL, pInputWaveFormat, &outputWaveFormat, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME);

		if (mmr) 
		{
			CQTRACE11("ACM error #%08X", mmr);
			goto Done;
		}

		// now prepare the acm header

		acmHeader.cbDstLength = (U32) (outputWaveFormat.nAvgBytesPerSec * (g_SoundBufferTime * 1.5));
		acmHeader.cbStruct = sizeof(acmHeader);

		mmr = acmStreamSize(hACMStream, acmHeader.cbDstLength, &acmHeader.cbSrcLength, ACM_STREAMSIZEF_DESTINATION);

		if (mmr) 
		{
			CQTRACE11("ACM error #%08X", mmr);
			goto Done;
		}

		if (acmHeader.cbSrcLength < pInputWaveFormat->nBlockAlign)
		{
			acmHeader.cbSrcLength = pInputWaveFormat->nBlockAlign;

			mmr = acmStreamSize(hACMStream, acmHeader.cbSrcLength, &acmHeader.cbDstLength, ACM_STREAMSIZEF_SOURCE);

			if (mmr) 
			{
				CQTRACE11("ACM error #%08X", mmr);
				goto Done;
			}
		}
		acmOrigSrcSize = acmHeader.cbSrcLength;

		if ((acmHeader.pbSrc = (U8 *) VirtualAlloc(0, acmHeader.cbSrcLength, MEM_COMMIT, PAGE_READWRITE)) == 0)
		{
			CQTRACE10("GlobalAlloc failed");
			goto Done;
		}
		if ((acmHeader.pbDst = (U8 *) VirtualAlloc(0, acmHeader.cbDstLength, MEM_COMMIT, PAGE_READWRITE)) == 0)
		{
			CQTRACE10("GlobalAlloc failed");
			goto Done;
		}

		mmr = acmStreamPrepareHeader(hACMStream, &acmHeader, 0);
		if (mmr) 
		{
			CQTRACE11("ACM error #%08X", mmr);
			goto Done;
		}

		acmHeader.cbSrcLength = acmHeader.cbSrcLengthUsed = 0;	// nothing in the buffer
		acmHeader.cbDstLengthUsed = 0;
	}
	else	// no compression needed
	{
		outputWaveFormat = *pInputWaveFormat;
	}

	//
	// allocate a read buffer 
	// read ahead = 4 seconds
	//

	raw.init((U32(pInputWaveFormat->nAvgBytesPerSec * g_ReadBufferTime) + CDALIGNMASK) & ~CDALIGNMASK);

	// if using decompression, allocate a buffer for it

	if (hACMStream)
		rusty.init(acmHeader.cbDstLength * 2);

	//
	// create a DirectSound buffer
	// Shoot for updates 4 times per second
	//
	
	halfBufferSize = (U32(outputWaveFormat.nAvgBytesPerSec * g_SoundBufferTime) + 0xFF) & ~0xFF;

	memset(&dsdesc, 0, sizeof(dsdesc));
	dsdesc.dwSize = sizeof(dsdesc);
	dsdesc.lpwfxFormat = & outputWaveFormat;		
	dsdesc.dwFlags = DSBCAPS_CTRLVOLUME|DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_STICKYFOCUS;
	dsdesc.dwBufferBytes = halfBufferSize * 2;
	hr = DSOUND->CreateSoundBuffer(&dsdesc, lpDSBuffer, 0);
	if (hr != DS_OK)
	{
		CQWARNING1("CreateSoundBuffer failed, error=#%08X", hr);
		goto Done;
	}

	lpDSBuffer->SetVolume(volumeRequested);

	newState = START;
	
	//
	// align origFile pointer on 2K boundary
	//
	initialRead = ((soundDataStart + CDALIGNMASK) & ~CDALIGNMASK) - soundDataStart;
	if ((initialRead = raw.addToBuffer (&pBuffer, initialRead)) != 0)
	{
		pFile->ReadFile(hBuffered, pBuffer, initialRead, &dwBytesRead, 0);
		soundDataPos += dwBytesRead;
		raw.pWriter += dwBytesRead;
		//
		// remember this bit of data for looping
		// 
		if (bLooping)
		{
			if ((lpLoopData = GlobalAlloc(GMEM_FIXED, dwBytesRead)) != 0)
			{
				memcpy(lpLoopData, pBuffer, dwBytesRead);
				dwLoopDataSize = dwBytesRead;
			}
			else
			{
				CQTRACE10("GlobalAlloc failed");
				bLooping = 0;
			}
		}
	}

	//
	// try to reopen without buffering
	//
#if 1
	fdesc.dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING;
	if ((hFile = pFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
#endif
	{
		hFile = hBuffered;	// buffering not supported
		hBuffered = INVALID_HANDLE_VALUE;
	}

	//
	// start file read in the background
	//
	raw.setAlignment(CDALIGNMASK+1);
	initialRead = raw.addToBuffer(&pBuffer, raw.bufferSize);

	overlapped.Offset = soundDataPos;		// should be aligned on 2K boundary
	overlapped.OffsetHigh = 0;
	
	bIOPending = (pFile->ReadFile(hFile, pBuffer, initialRead, &overlappedBytesRead, &overlapped) == 0);

Done:
	if (newState != START)
	{
		CQTRACE11("Streamer::init() failed on '%s'", fdesc.lpFileName);
		if (hMainWindow && uMsg >= WM_USER)
			PostMessage(hMainWindow, uMsg, (WPARAM) IStreamer::INVALID, (LPARAM) this);
	}
	pFile->CloseHandle(hBuffered);
	setState(newState);
}
//--------------------------------------------------------------------------//
// start playing the file. The current state should be START
//
void Streamer::start (void)
{
	S32 overage;

	if (bIOPending)
	{
		// still waiting on data?
		if ((bIOPending = (pFile->GetOverlappedResult(hFile, &overlapped, &overlappedBytesRead, 0) == 0)) != 0)
			return;
	}

	soundDataPos += overlappedBytesRead;

	if ((overage = bytesLeftToRead()) <= 0)
		overlappedBytesRead += overage;
	raw.pWriter += overlappedBytesRead;
	
	fillHalfBuffer(0);
	fillHalfBuffer(1);

	if (overage <= 0)
	{
		if (bLooping)
			setState(LOOPEND);
		else
			setState(ENDOFFILE);
	}
	else
		setState(PLAY);

	if (hMainWindow && uMsg >= WM_USER)
		PostMessage(hMainWindow, uMsg, (WPARAM) IStreamer::INITSUCCESS, (LPARAM) this);

	if (bStopRequested)
		setState(STOP);
	else
		lpDSBuffer->Play(0,0, DSBPLAY_LOOPING);

	whichHalf = 0;
}
//--------------------------------------------------------------------------//
// state can be PLAY or ENDOFFILE
//
void Streamer::play (void)
{
	BOOL32 newHalf;
	DWORD readPos, writePos;

	lpDSBuffer->GetCurrentPosition(&readPos,&writePos);
	newHalf = (readPos >= halfBufferSize);

	if (newHalf != whichHalf)
	{
		fillHalfBuffer(whichHalf);		// fill the side that the reader is NOT on
		whichHalf = newHalf;
	}

		// still waiting on data?
	if (bIOPending)
	{
		S32 overage;
		if ((bIOPending = (pFile->GetOverlappedResult(hFile, &overlapped, &overlappedBytesRead, 0) == 0)) != 0)
			return;

		soundDataPos += overlappedBytesRead;
		if ((overage = bytesLeftToRead()) <= 0)
		{
			overlappedBytesRead += overage;
			if (bLooping)
				setState(LOOPEND);
			else
				setState(ENDOFFILE);
		}
		raw.pWriter += overlappedBytesRead;
	}

	if (state==LOOPEND)
	{
		U8 *pBuffer;
		S32 readSize;

		if ((readSize = raw.addToBuffer(&pBuffer, raw.bufferSize)) != 0)
		{
			ASSERT(readSize >= S32(dwLoopDataSize));		// dwLoopDataSize is always smaller than alignment
			memcpy(pBuffer, lpLoopData, dwLoopDataSize);
			raw.pWriter += dwLoopDataSize;
			soundDataPos = soundDataStart + dwLoopDataSize;
			setState(PLAY);
		}
	}
	
	//
	// fill more into raw buffer
	//
	if (state == PLAY)
	{
		U32 avail = raw.bufferSize - raw.dataInBuffer() - 1;		// free space available

//		if (avail * 2 >= raw.bufferSize)
		ASSERT(raw.bufferSize > 0x1000);
		if (avail >= 0x1000)
		{
			U8 *pBuffer;
			S32	readSize = bytesLeftToRead();
		
			if (readSize <= 0)
			{
				if (bLooping)
					setState(LOOPEND);
				else
					setState(ENDOFFILE);
			}
			else
			{
				if ((readSize = raw.addToBuffer(&pBuffer, raw.bufferSize)) != 0)
				{
					overlapped.Offset = soundDataPos;		// should be aligned on 2K boundary
					overlapped.OffsetHigh = 0;
		
					bIOPending = (pFile->ReadFile(hFile, pBuffer, readSize, &overlappedBytesRead, &overlapped) == 0);
					if (bIOPending == 0)	// read already completed
					{
						S32 overage;

						soundDataPos += overlappedBytesRead;
						if ((overage = bytesLeftToRead()) <= 0)
						{
						 	overlappedBytesRead += overage;
							if (bLooping)
								setState(LOOPEND);
							else
								setState(ENDOFFILE);
						}
						raw.pWriter += overlappedBytesRead;
					}
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
// fill the side that the reader is NOT on
//
void Streamer::fillHalfBuffer (BOOL32 which)
{
	U8 *pSndMemory;
	U32 bufferLen;
	U32 copy=0;

	if (hACMStream)
		fillDecompressionChamber();		// decompress data

	//
	// lock the sound buffer, fill it with data
	//

	if (lpDSBuffer->Lock((which)?halfBufferSize:0, halfBufferSize, (void **) &pSndMemory, &bufferLen, 0, 0, 0) == DS_OK)
	{
		U8 *pSrc, *pDst;

		pDst = pSndMemory;

		if (state == START || state == PLAY || state == ENDOFFILE || state==LOOPEND)
		do
		{
			if (hACMStream)  // if using compression
			{
				if ((copy = rusty.removeFromBuffer(&pSrc, bufferLen)) == 0)
					break;
			}
			else		// else take data directly from read source
			{
				if ((copy = raw.removeFromBuffer(&pSrc, bufferLen)) == 0)
					break;
			}
			memcpy(pDst, pSrc, copy);
			bufferLen -= copy;
			pDst += copy;
		} while (bufferLen > 0);

		if (bufferLen > 0)
		{
			if (outputWaveFormat.wBitsPerSample < 16)
				memset(pDst, 0x80, bufferLen);
			else
				memset(pDst, 0, bufferLen);
		}

		lpDSBuffer->Unlock(pSndMemory, halfBufferSize, 0, 0);
	}
	else
		CQTRACE10("lpDSBuffer->Lock() failed");

	switch (state)
	{
	case ENDOFFILE:
		if (copy == 0)
			setState(ENDOFFILE1);
		break;
	case ENDOFFILE1:
		setState(ENDOFFILE2);
		break;
	case ENDOFFILE2:
		setState(ENDOFFILE3);
		break;
	}
}
//--------------------------------------------------------------------------//
// called only when using compression
// move data from raw buffer to source chamber for decompression
//
void Streamer::fillCompressionChamber (void)
{
	U32 bufferLen = acmOrigSrcSize;
	U8 *pSrc;
	U8 *pDst = acmHeader.pbSrc;
	S32 extra;		// left-over data from last time

	extra = acmHeader.cbSrcLength - acmHeader.cbSrcLengthUsed;

	if (extra > 0)
	{
		if (acmHeader.cbSrcLengthUsed > 0)
			memcpy(pDst, pDst+acmHeader.cbSrcLengthUsed, extra);
		bufferLen -= extra;
		pDst += extra;
	}

	do
	{
		U32 copy;
		if ((copy = raw.removeFromBuffer(&pSrc, bufferLen)) == 0)
			break;
		memcpy(pDst, pSrc, copy);
		bufferLen -= copy;
		pDst += copy;
	} while (bufferLen > 0);

	acmHeader.cbSrcLength = (pDst - acmHeader.pbSrc);
	acmHeader.cbSrcLengthUsed = 0;
}
//--------------------------------------------------------------------------//
// move decompressed data into rusty buffer
//
void Streamer::moveDecompressedData (void)
{
	if (acmHeader.cbDstLengthUsed > 0)
	{
		long avail = rusty.bufferSize - rusty.dataInBuffer() - 1;

		if (avail >= long(acmHeader.cbDstLengthUsed))
		{
			U8 *pBuffer;
			U32 readSize;
			U32 bytesSent=0;

			do
			{
				if ((readSize = rusty.addToBuffer(&pBuffer, acmHeader.cbDstLengthUsed)) == 0)
					break;

				memcpy(pBuffer, acmHeader.pbDst+bytesSent, readSize);
				bytesSent += readSize;
				acmHeader.cbDstLengthUsed -= readSize;
				rusty.pWriter += readSize;

			} while (acmHeader.cbDstLengthUsed > 0);
		}
	}

}
//--------------------------------------------------------------------------//
// called only when using compression
// decompress source chamber, move decompressed data to rusty buffer
//
void Streamer::fillDecompressionChamber (void)
{
	do
	{

		if (acmHeader.cbDstLengthUsed > 0)
			moveDecompressedData();
		if (acmHeader.cbDstLengthUsed > 0)
			break;		// not enough room for decompressed data

		fillCompressionChamber();

		bool bEndOfStream = (acmHeader.cbSrcLength < acmOrigSrcSize && bLooping==false && bytesLeftToRead() <= 0);

		//
		// do not feed the decompressor irregularrly sized buffers unless
		// we are at the end of the stream. Some decompressors interpret an odd sized
		// buffer as the end of stream, and stop working after that.
		//
		
		if (acmHeader.cbSrcLength == acmOrigSrcSize || (bEndOfStream && acmHeader.cbSrcLength!=0))
		{
			if (bEndOfStream)
			{
				CQTRACE50("End of stream detected on compressed stream");
			}

			MMRESULT mmr;
			mmr = acmStreamConvert(hACMStream, &acmHeader, (bEndOfStream) ? 0 : ACM_STREAMCONVERTF_BLOCKALIGN);
			if (mmr) 
			{
				CQTRACE51("ACM error #%08X", mmr);
				break;
			}
			if (acmHeader.cbDstLengthUsed == 0)		// something went wrong!
			{
				CQTRACE50("Unknown error!");
				break;
			}
		}
		else
		{
			if (bEndOfStream==false)	// only a problem if stream is supposed to be continuing
				CQTRACE50("Source stream underflow detected");
			break;
		}

	} while (1);
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
	volatile Streamer * playList;		// list of streamers that are playing
	volatile Streamer * inactiveList;	// list of streamers that are in some other state
								// the 'inactiveList' is guarded by the criticalSection

	mutable	CRITICAL_SECTION criticalSection;
	HANDLE  hEvent;
	//--------------------------------

	Music (void);

	~Music (void);

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "Music");
	}
	void operator delete( void *ptr )
	{
		HEAP->FreeMemory(ptr);
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
		SetEvent(hEvent);
		WaitForSingleObject(hThread, INFINITE);
		::CloseHandle(hThread);
		hThread = 0;
	
		DeleteCriticalSection(&criticalSection);
	}

	//
	// delete streamer lists
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

	::CloseHandle(hEvent);
	hEvent = 0;

	if (DSOUND)
	{
		DSOUND->Release();
		DSOUND = 0;
	}
}
//--------------------------------------------------------------------------//
// return true if a streamer needs to be moved off to another list
//
BOOL32 Music::updateList (Streamer * streamer)
{
	BOOL32 result = 0;

	while (streamer)
	{
		switch (streamer->state)
		{
		case INIT:
			streamer->init();
			break;
		case START:
			// need critical section because start() messes with state and bStopRequested
			EnterCriticalSection(&criticalSection);
			streamer->start();
			if (streamer->state == STOP)
				result = 1;
			LeaveCriticalSection(&criticalSection);
			break;

		case RESTART:
			if (streamer->bStopRequested==0 && streamer->lpDSBuffer!=0)
				streamer->lpDSBuffer->Play(0,0, DSBPLAY_LOOPING);
			streamer->state = PLAY;
			// fall through intentional !
		case PLAY:
		case LOOPEND:
		case ENDOFFILE:
		case ENDOFFILE1:
		case ENDOFFILE2:
			streamer->play();
			if (streamer->bStopRequested)
				result = 1;
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
			prev = node;
			node = node->pNext;
			break;

		case PLAY:
		case RESTART:
		case LOOPEND:
		case ENDOFFILE:
		case ENDOFFILE1:
		case ENDOFFILE2:
			if (node->bStopRequested)
			{
				node->setState(STOP);
				goto StopIt;
			}
			prev = node;
			node = node->pNext;
			break;
		case ENDOFFILE3:
			node->free();
			// fall through intentional
		default:	// else not active, move to other list
StopIt:
			if (node->bKillRequested)
				node->free();
			if (node->lpDSBuffer)
				node->lpDSBuffer->Stop();
			node->bInPlayList = false;
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
		case RESTART:
		case LOOPEND:
		case ENDOFFILE:
		case ENDOFFILE1:
		case ENDOFFILE2:
			if (node->bStopRequested==0 && node->lpDSBuffer!=0)
				node->lpDSBuffer->Play(0,0, DSBPLAY_LOOPING);
			if (node->state==RESTART)
				node->state=PLAY;
			// fall through intentional
		case INIT:
		case START:			// node is active, move to the active list
			node->bInPlayList = true;
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
		case RESET:
			node->free();
			// fall through intentional
		default:	// else not active, keep in this list
			prev = node;
			node = node->pNext;
			break;
		}
	}
}
//--------------------------------------------------------------------------//
//
void Music::main (void)
{
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

		WaitForSingleObject(hEvent, g_MaxIdleTime);
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

	if (desc==0 || desc->size != sizeof(*desc) || desc->lpDSound==0)
		return 0;

	if (hThread != 0)	// if already initialized!
	{
		if ((g_ReadBufferTime = desc->readBufferTime) == 0)
			g_ReadBufferTime = DEFAULT_READ_BUFFER_TIME;

		if ((g_SoundBufferTime = desc->soundBufferTime) == 0)
			g_SoundBufferTime = DEFAULT_SOUND_BUFFER_TIME;

		U32 newTime = U32(g_SoundBufferTime * 333.0);		// convert to milleseconds / 3
		g_MaxIdleTime = __min(newTime, g_MaxIdleTime);

		return 1;
	}

	DSOUND = desc->lpDSound;
	desc->lpDSound->AddRef();

	hMainWindow = desc->hMainWindow;
	uMsg = desc->uMsg;

	if ((g_ReadBufferTime = desc->readBufferTime) == 0)
		g_ReadBufferTime = DEFAULT_READ_BUFFER_TIME;

	if ((g_SoundBufferTime = desc->soundBufferTime) == 0)
		g_SoundBufferTime = DEFAULT_SOUND_BUFFER_TIME;

	g_MaxIdleTime = U32(g_SoundBufferTime * 333.0);		// convert to milleseconds / 3

	//
	// setup stuff for multi-threading
	//
	
	InitializeCriticalSection(&criticalSection);
	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);		// create an auto-reset event

	hThread = CreateThread(0,4096, (LPTHREAD_START_ROUTINE) threadMain, (LPVOID)this, 0, &threadID);
	ASSERT_ERROR(hThread);

	if (hThread==0)
	{
		desc->lpDSound->Release();
		DSOUND = 0;
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
	if (parent && hThread)
	{
		Streamer * result = new Streamer;

		if (filename)
			strncpy(result->filename, filename, sizeof(result->filename)-1);
		result->pFile = parent;
		result->state = INIT;
		result->bStopRequested = ((flags & STRMFL_PLAY) == 0);
		result->bLooping = ((flags & STRMFL_LOOPING) != 0);
		result->overlapped.hEvent = hEvent;
		//
		// add new stream to the list
		//
		EnterCriticalSection(&criticalSection);
		
		result->pNext = (Streamer *) inactiveList;
		inactiveList = result;

		LeaveCriticalSection(&criticalSection);
		SetEvent(hEvent);
		return result;
	}
	else
		return 0;
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

	tmp->bStopRequested = true;
	tmp->bKillRequested = true;

	while (1)
	{
		EnterCriticalSection(&criticalSection);
		if (tmp->bInPlayList==0)
			break;
		LeaveCriticalSection(&criticalSection);
		SetEvent(hEvent);
		Sleep(0);
	}
	tmp->state = RESET;
	LeaveCriticalSection(&criticalSection);

	while (tmp->lpDSBuffer != 0)
	{
		SetEvent(hEvent);
		Sleep(0);
	}

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
			result = (tmp->lpDSBuffer != 0);
			tmp->bStopRequested = true;
			SetEvent(hEvent);
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
			tmp->state = RESTART;
		LeaveCriticalSection(&criticalSection);
		SetEvent(hEvent);
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

		if (tmp->lpDSBuffer)
		{
			result = (tmp->lpDSBuffer->SetVolume(tmp->volumeRequested) == DS_OK);
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

		if (tmp->lpDSBuffer)
		{
			LONG dsvolume;

			result = (tmp->lpDSBuffer->GetVolume(&dsvolume) == DS_OK);

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
//---------------------------End Streamer.cpp-------------------------------//
//--------------------------------------------------------------------------//
