#ifndef ROSESOUND_H
#define ROSESOUND_H
//
// RoseSound.h - Simple sound player for ROSE.
//

//
// Include files
//

#include <dsound.h>
#include "wavlib.h"

//
// Class and structure definitions
//

class RoseSound
{
protected:
	static LPDIRECTSOUND   lpDS;
	static int             invocation;

public:
	SoundFile              file;
	LPDIRECTSOUNDBUFFER    lpDSBuff;
	int                    invoke;  // used to catch operator error
	char*                  currFilename;

public:
	RoseSound ();
	RoseSound (const char *filename);
	~RoseSound ();

	bool load (const char *filename);
	bool free ();

	float getLength();              // in seconds.
	unsigned int getSampleCount();  // raw sample count.

	// Playback functions

	// Starts the sound playing at the given location, which is seconds from the start of the sound.
	bool play (float fromWhere = 0.0);

	// Starts the sound playing at the given location, which is samples from the start of the sound.
	bool play (unsigned int fromWhere = 0);

	// Stops the sound.
	bool stop ();

	// Get the current status of the sound, if it is playing, and where the play cursor is.
	bool getStatus (bool &playing, unsigned int &playCursor);

	// These initialize the system. You must startup before creating or loading a RoseSound.

	// Pass in a valid format in order to set the primary buffer format. NULL uses the default(current)
	// setting.
	static bool startup (HWND hWnd, SoundFormat *primaryFormat = NULL);

	// To exit cleanly call this function before exiting.  It releases any allocated DirectSound resources.
	static bool shutdown ();
};

#endif

