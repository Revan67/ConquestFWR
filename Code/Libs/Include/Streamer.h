#ifndef STREAMER_H
#define STREAMER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 Streamer.h                               //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Rmarr $

   $Header: /Conquest/Libs/Include/Streamer.h 6     4/28/00 11:57p Rmarr $

   Low level audio player. Streams .WAV files in the background.
*/
//--------------------------------------------------------------------------//
//
/*
	//------------------------------
	//
	BOOL32 IStreamer::Init (STREAMERDESC * desc);
		INPUT:
			desc: Address of user supplied init structure.
			struct STREAMERDESC
				size			: size of the structure. 
				lpDSound		: Address of IDirectSound interface created by the application.
				hMainWindow		: Handle to application's main window.
				uMsg			: Message to send to application window on stream state change. (See Notes below.)
				readBufferTime  : size of read ahead buffer, in seconds
				soundBufferTime : size of DirectSound buffer, in seconds (actually, size of half-buffer)
		RETURNS:
			TRUE if it was initialized properly, FALSE if 'desc' was invalid.
		OUTPUT:
			Attaches to the low-level sound interface, clears out internal states, and creates streaming
			thread(s)
		NOTES:
			'lpDSound' is now a required parameter. The application must create an instance of DirectSound before
			initializing the streamer. 'hMainWindow' and 'uMsg' are optional parameters. If you specify a valid
			window handle, and a valid windows message (i.e. >= WM_USER), the streamer will notify you of state
			changes to your streams.
			A windows message will be posted when the stream reaches INVALID, EOFREACHED and COMPLETED states:
				PostMessage(hMainWindow, uMsg, (WPARAM) streamStatus, (LPARAM) hStream);
			A stream can reach the INVALID state if it fails to initialize.(e.g. file not found.)
			A windows message (INITSUCCESS) will be sent when the stream is ready for play.


  //------------------------------
	//
	HSTREAM IStreamer::Open (const char * filename, struct IFileSystem * parent, U32 flags);
		INPUT:
			filename: ASCIIZ name of the file to open within 'parent'
			parent: Parent file system that contains the WAV file to play. (Required)
			flags: 
				STRMFL_PLAY:		Start stream once opened.
				STRMFL_LOOPING		Loop the entire stream.
		RETURNS:
			Handle to an audio stream. NULL if the method fails.
		OUTPUT:
			Since the file is opened in the background, the method may return a valid handle, even though
			the file failed to load. (File not found).
		NOTES:
			Any number of streams can be loaded and active at one time. Use the returned handle
			to identify the stream (the 'hStream' parameter) in future calls to IStreamer.

	//------------------------------
	//
	BOOL32 IStreamer::CloseHandle (HSTREAM hStream);
		INPUT:
			hStream: Handle to a audio stream created by Open().
		RETURNS:
			TRUE if the handle was valid and the associated stream was successfully closed.
		OUTPUT:
			Immediately stops the stream (if it was playing) and frees all resources attached to the stream.

	//------------------------------
	//
	BOOL32 IStreamer::Stop (HSTREAM hStream);
		INPUT:
			hStream: Handle to an audio stream created by Open().
		RETURNS:
			TRUE if the handle was valid and the stream was successfully stopped.

	//------------------------------
	//
	BOOL32 IStreamer::Restart (HSTREAM hStream);
		INPUT:
			hStream: Handle to a audio stream created by Open().
		RETURNS:
			TRUE if the handle was valid and the stream was successfully restarted.
		NOTES;
			Call this method to restart a stream from the point that it was stopped. 
			It will not restart a stream that has completed. 
			To replay a stream from the beginning, you must open a new stream.

	//------------------------------
	//
	BOOL32 IStreamer::SetVolume (HSTREAM hStream, S32 volume);
		INPUT:
			hStream: Handle to a audio stream created by Open().
			volume: 0 to -10000, where 0 is full volume.
		RETURNS:
			TRUE if the handle was valid and the volume was successfully adjusted.
		OUTPUT:
			Sets the volume for the stream.

			
	//------------------------------
	//
	BOOL32 IStreamer::GetVolume (HSTREAM hStream, S32 * volume) const;
		INPUT:
			hStream: Handle to a audio stream created by Open().
			volume: Address of value to be set by this method
		RETURNS:
			TRUE if the handle was valid.
		OUTPUT:
			Sets '*volume' to the stream's current volume setting, where -10000 is silent and 
			0 is full volume.
			
	//------------------------------
	//
	STATUS IStreamer::GetStatus (HSTREAM hStream) const;
		INPUT:
			hStream: Handle to a audio stream created by Open().
		RETURNS:
			INVALID:		audio stream could not be loaded. (File error)
			PLAYING:		Currently playing
			STOPPED:		Sound data is ready, but is in paused state.
			EOFREACHED:		End of data file has been reached, but audio is still playing.
			COMPLETED:		All sound data has been played
			
*/
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif
//--------------------------------------------------------------------------//
//
typedef struct Streamer *HSTREAM;
//--------------------------------------------------------------------------//
// structure used in Init()
//
struct STREAMERDESC
{
	U32 size;
	struct IDirectSound *lpDSound;
	HWND hMainWindow;
	UINT uMsg;
	SINGLE readBufferTime;		// in seconds, 0 = default setting (4.0 is a reasonable value)
	SINGLE soundBufferTime;		// in seconds, 0 = default setting (0.25 is a reasonable value)
	
	STREAMERDESC (void)
	{
		memset(this, 0, sizeof(*this));
		size = sizeof(*this);
	}
};
//--------------------------------------------------------------------------//
// Flags for Open() method
//
#define STRMFL_PLAY			0x00000001			// start stream once opened
#define STRMFL_LOOPING		0x00000002			// loop the entire stream

#define IID_IStreamer MAKE_IID("IStreamer",2)
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IStreamer : IDAComponent
{
	enum STATUS
	{
		INVALID=0,
		PLAYING,
		STOPPED,
		EOFREACHED,
		COMPLETED,
		INITSUCCESS
	};
	
	DEFMETHOD_(BOOL32,Init) (STREAMERDESC * desc) = 0;

	DEFMETHOD_(HSTREAM,Open) (const char * filename, struct IFileSystem * parent, DWORD flags=STRMFL_PLAY) = 0;

	DEFMETHOD_(BOOL32,CloseHandle) (HSTREAM hStream) = 0;

	DEFMETHOD_(BOOL32,Stop) (HSTREAM hStream) = 0;

	DEFMETHOD_(BOOL32,Restart) (HSTREAM hStream) = 0;

	DEFMETHOD_(BOOL32,SetVolume) (HSTREAM hStream, S32 volume) = 0;

	DEFMETHOD_(BOOL32,GetVolume) (HSTREAM hStream, S32 * volume) const = 0;

	DEFMETHOD_(STATUS,GetStatus) (HSTREAM hStream) const = 0;
};




//--------------------------------------------------------------------------//
// -------------------------------END Streamer.h--------------------------------//
//--------------------------------------------------------------------------//
#endif