//
// RoseSound.cpp - Source for the RoseSound class
//

//
// Include files
//

#include "PCH.h"
#include <assert.h>
#include <string.h>
#include <MMSystem.h>

#include "rosesound.h"

//
// Static data members
//

LPDIRECTSOUND RoseSound::lpDS = NULL;
int           RoseSound::invocation = 1;

//
// Routines
//

RoseSound::RoseSound ()
{
	lpDSBuff = NULL;
	invoke = 0;
	currFilename = NULL;
}

RoseSound::RoseSound (const char *filename)
{
	lpDSBuff = NULL;
	invoke = 0;
	currFilename = NULL;

	// Load the given .WAV file.
	load (filename);
}

RoseSound::~RoseSound ()
{
	free ();
}

bool RoseSound::load (const char *filename)
{
	assert (lpDSBuff == NULL);
	assert (lpDS != NULL);

	bool returnValue = false;

	// Use WAVLib to load the given filename.

	IFileSystem *fs;
	DAFILEDESC fdesc = filename;
	DACOM_Acquire()->CreateInstance (&fdesc, (void **) &fs);

	if (fs && LoadWAV(fs, file))
	{
		// Create a buffer in the same format as the file data and
		// big enough to hold the entire sample.
	
		DSBUFFERDESC desc;
		WAVEFORMATEX wavFormat;

		memset (&wavFormat, 0, sizeof(wavFormat));
		wavFormat.wFormatTag = WAVE_FORMAT_PCM;
		wavFormat.nChannels = file.format.num_channels;
		wavFormat.nSamplesPerSec = file.format.samples_per_sec; 
		wavFormat.nAvgBytesPerSec = file.format.samples_per_sec * file.format.bytes_per_sample;
		wavFormat.nBlockAlign = file.format.bytes_per_sample;
		wavFormat.wBitsPerSample = file.format.bytes_per_channel*8;

		memset (&desc, 0, sizeof (desc));
		desc.dwSize = sizeof(desc);
		desc.dwFlags = DSBCAPS_CTRLALL | DSBCAPS_STATIC | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
		desc.dwBufferBytes = file.length;
		desc.lpwfxFormat = &wavFormat;
		
		HRESULT result = lpDS->CreateSoundBuffer (&desc, &lpDSBuff, NULL);
		if (result == DS_OK)
		{
			// Lock the buffer and store the samples into it.

			LPVOID ptr;
			DWORD  len;
			result = lpDSBuff->Lock (0, file.length, &ptr, &len, NULL, NULL, DSBLOCK_ENTIREBUFFER);
			if (result == DS_OK)
			{
				memcpy (ptr, file.samples, file.length);
				lpDSBuff->Unlock(ptr, len, NULL, 0);

				// Store the invocation value to guard against misuse.
				invoke = invocation;

				// All is well.
				returnValue = true;
			}
			else
			{
				lpDSBuff->Release ();
				lpDSBuff = NULL;
			}
		}
		else
		{
			lpDSBuff = NULL;
		}

		// Free the buffer.
		delete file.samples;
	}

	// Remember the filename
	if(currFilename)
	{	delete[] currFilename;
		currFilename = NULL;
	}
	currFilename = new char[strlen(filename) + 1];
	strcpy(currFilename, filename);

	return returnValue;
}

bool RoseSound::free ()
{
	if (lpDSBuff != NULL)
	{
		assert (invocation == invoke);

		lpDSBuff->Release();
		lpDSBuff = NULL;

		if(currFilename)
		{	delete[] currFilename;
			currFilename = NULL;
		}
	}
	return true;
}

float RoseSound::getLength()
{
	assert (lpDSBuff != NULL);

	return (float) file.num_samples / (float) file.format.samples_per_sec;
}

unsigned int RoseSound::getSampleCount()
{
	assert (lpDSBuff != NULL);

	return file.num_samples;
}

bool RoseSound::play (float fromWhere)
{
	assert (invoke == invocation);
	assert (lpDSBuff != NULL);

	// Convert the given floating point value into a sample position.

	return play ((unsigned int)(fromWhere * file.format.samples_per_sec));
}

bool RoseSound::play (unsigned int fromWhere)
{
	assert (invoke == invocation);
	assert (lpDSBuff != NULL);

	// Stop the sound. Set the position. Play the sound.

	HRESULT result = lpDSBuff->Stop ();
	if (result == DS_OK)
	{
		result = lpDSBuff->SetCurrentPosition (fromWhere);
		if (result == DS_OK)
		{
			result = lpDSBuff->Play (0,0,0);
			if (result == DS_OK)
			{
				return true;
			}
		}
	}

	return false;
}

bool RoseSound::stop ()
{
	assert (invoke == invocation);
	assert (lpDSBuff != NULL);

	// Stop the sound.

	HRESULT result = lpDSBuff->Stop ();

	if (result == DS_OK)
	{
		return true;
	}

	return false;
}

bool RoseSound::getStatus (bool &playing, unsigned int &playCursor)
{
	assert (invoke == invocation);
	assert (lpDSBuff != NULL);

	DWORD status;
	DWORD playPos;
	
	HRESULT result = lpDSBuff->GetStatus (&status);
	if (result == DS_OK)
	{
		playing = ((status & DSBSTATUS_PLAYING) != 0);

		result = lpDSBuff->GetCurrentPosition (&playPos, NULL);
		if (result == DS_OK)
		{
			playCursor = playPos;
			return true;
		}
	}

	return false;
}

// Static function members
bool RoseSound::startup (HWND hWnd, SoundFormat *primaryFormat)
{
//	assert (hWnd != NULL);
	assert (lpDS == NULL);

	// Create the DirectSound object, set the cooperative level, then set the
	// primary buffer format, if requested.

	HRESULT result = DirectSoundCreate (NULL, &lpDS, NULL);

	if (result != DS_OK)
	{
		return false;
	}

	result = lpDS->SetCooperativeLevel (hWnd, primaryFormat ? DSSCL_PRIORITY : DSSCL_NORMAL);
	if (result != DS_OK)
	{
		lpDS->Release ();
		lpDS = NULL;
		return false;
	}

	if (primaryFormat != NULL)
	{
		// TODO: Set the primary format here.
		// Create a primary sound buffer.
		// Set its format to that passed in.
		// Release the primary buffer.  [Will that work?]
	}

	// All is well.
	return true;
}

bool RoseSound::shutdown ()
{
	// Release the DirectSound object, which automatically releases the sound buffers.
	// NOTE: This automatically makes all RoseSound objects invalid. Any attempt to
	// use a RoseSound object in this state will assert.

	if (lpDS != NULL)
	{
		lpDS->Release();
		lpDS = NULL;
	}

	// Increment the invocation token, which prevents the use of bad RoseSound objects.
	if (++invocation == 0)
	{
		++invocation;
	}

	return true;
}

//
// Test program.
//

#ifdef BUILD_MAIN
#include <stdio.h>
#include <conio.h>

ICOManager *       DACOM = NULL;

const int MAX_SOUNDS = 16;

BOOL CALLBACK EnumThreadWndProc( HWND hwnd, LPARAM lParam)
{
	*((HWND *) lParam) = hwnd;
	return false;
}

int main (int argc, char *argv[])
{
	HWND hWnd;

	EnumThreadWindows (GetCurrentThreadId(), (WNDENUMPROC) EnumThreadWndProc, (LPARAM) &hWnd);
	DACOM = DACOM_Acquire();
	DACOM->SetINIFile("RoseSound.ini");

	// Initialize the sound.
	RoseSound::startup (hWnd);

	// Load and play all of the wave files passed on the command line.

	RoseSound *sounds[MAX_SOUNDS];
	memset (sounds, 0, sizeof(sounds));

	int i;
	for (i = 1; i < min(argc, MAX_SOUNDS); ++i)
	{
		sounds[i] = new RoseSound(argv[i]);
		printf ("Loaded %s: %.2f sec (%d samples)\n", argv[i], sounds[i]->getLength(), sounds[i]->getSampleCount());
		sounds[i]->play (0U);
	}

	// Loop until they are finished.

	bool done = false;
	while (!done)
	{
		done = true;

		for (i = 1; i < min(argc, MAX_SOUNDS); ++i)
		{
			bool playing;
			unsigned int cursor;
			if (sounds[i]->getStatus(playing, cursor))
			{
				if (playing)
				{
					done = false;
					break;
				}				
			}
		}

		if (_kbhit())
		{
			done = true;
		}
	}

	// Cleanup and exit.
	RoseSound::shutdown ();
	return 0;
}
#endif
