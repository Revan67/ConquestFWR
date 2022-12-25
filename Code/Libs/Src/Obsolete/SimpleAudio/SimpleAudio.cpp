//
// SimpleAudio.cpp - A simple version of the DA AudioManager component that is built directly on top of DirectSound
//

//
// Include files
//

#include <dsound.h>
#include <wavlib.h>
#include <tsmartpointer.h>
#include <fdump.h>
#include <dacom.h>                    // DA component manager
#include <heapobj.h>
#include <stddat.h>
#include <vector.h>
#include <audiomgr.h>
#include <tcomponent.h>
#include <tempstr.h>

//
// Constants
//

const C8 *interface_name = "IAudioManager"; // Interface name used for registration

//
// Class and structure definitions
//

class Sound
{
protected:
	friend struct AUDMGR;

	static LPDIRECTSOUND       lpDS;
	static LPDIRECTSOUNDBUFFER lpPrimary;
	static int                 invocation;

	Sound *                original;         // the original copy of this sound.
	Sound *                prev;             // the previous copy of this sound, circular
	Sound *                next;             // the next copy of this sound, circular

	Sound (Sound *root);

public:
	SoundFile              file;
	LPDIRECTSOUNDBUFFER    lpDSBuff;
	int                    invoke;           // used to catch operator error
	bool                   active;           // this sound is being used in the sound list.
	float                  base_vol;         // the volume used when at "full"
	float                  current_vol;      // current unscaled volume
	LONG                   lvol;

public:
	Sound ();
	Sound (const char *filename);
	~Sound ();

	bool load (IFileSystem *file);
	bool load (const char *filename);
	bool create (SoundFile *srcFile);
	bool free ();

	bool isLoaded () { return lpDSBuff != NULL; }
	bool isLooping() { return false; }

	float getLength();              // in seconds.
	unsigned int getSampleCount();  // raw sample count.

	// Playback functions

	// Starts the sound playing at the given location, which is seconds from the start of the sound.
//	bool play (float fromWhere, bool looping = false);

	// Starts the sound playing at the given location, which is samples from the start of the sound.
	bool play (unsigned int fromWhere = 0, bool looping = false);

	// Stops the sound.
	bool stop ();

	// Sets the volume of the sound, taking the base volume into account.
	void set_volume (float atten);

	// Sets the pan value of the sound.
	void set_pan (float atten);

	// Sets the sample volume. Volume is floating point decibels of attenuation.
	// NOTE: This volume is the volume used when the sound bearing vector is zero length.
	void set_base_volume (float atten);

	// Sets the pan and volume of the sound based on the direction and length of the bearing vector.
	void set_sound_bearing(const Vector & bearing, float falloff_3db);

	// Set the frequency of the sound to a value relative to its sample rate.
	// < 1.0 indicates reduction, 1.0 indicates the original rate, > 1.0 indicates increase
	void set_relative_rate(float rate_percent);
	
	// Get the current status of the sound, if it is playing, and where the play cursor is.
	bool getStatus (bool &playing, unsigned int &playCursor);

	// Get an inactive copy of this sound, creating a new one if none are available
	Sound * get_inactive_copy();

	// These initialize the system. You must startup before creating or loading a Sound.

	// Pass in a valid format in order to set the primary buffer format. NULL uses the default(current)
	// setting.
	static bool startup (HWND hWnd, SoundFormat *primaryFormat = NULL);

	// To exit cleanly call this function before exiting.  It releases any allocated DirectSound resources.
	static bool shutdown ();
};


class SNDFILE_TYPE
{
public:
	//
	// Data and methods required by HashPool template
	//

	U32           hash_key;  // Hash key for this entry

	SNDFILE_TYPE *hash_next; // Next/prev pointers in each hash bucket,
	SNDFILE_TYPE *hash_prev; // organized for access speed

	SNDFILE_TYPE *next;      // Next/prev pointers in allocation list or  
	SNDFILE_TYPE *prev;      // free list, depending on entry's status

	S32           index;     // Index of this entry in linear array

	//
	// HashPool hash function derives 8-bit key from string by
	// adding all ASCII character values modulo 256
	//

	static U32 hash(const void *object);

	//
	// HashPool search comparison function -- returns TRUE if match found
	//

	inline BOOL32 compare(const void *object)
	{
		return !strcmp(name, (C8 *) object);
	}

	//
	// HashPool initialization function -- used when allocating new entry
	//

	inline void initialize(const void *object)
	{
		strncpy(name, (C8 *) object, MAX_PATH-1);
		name[MAX_PATH-1] = '\0';
		sound = NULL;
	}

	//
	// HashPool shutdown function -- called when unlinking entry from pool
	//

	inline void shutdown(void)
	{
		if (sound)
		{
			delete sound;
			sound = NULL;
		}
	}

	//
	// HashPool diagnostic display function
	//

	void display(void)
	{
	}

	//
	// User data
	//

	C8  name[MAX_PATH];     // ASCII filename
	Sound * sound;
	S32 mode;
};

inline U32 SNDFILE_TYPE::hash(const void *object)
{
#if 1
	U32 sum;

	_asm
	 {
	 mov ebx,object
	 xor eax,eax
	 xor edx,edx

	 ALIGN 16
chksum:
	 mov dl,BYTE PTR [ebx]
	 add eax,edx
	 inc ebx
	 test edx,edx
	 jnz chksum

	 and eax,0ffh
	 mov sum,eax
	 }
#else
	U8 sum;

	//
	// Equivalent C version for portability
	//

	U8 *ptr = (U8 *) object;
	sum     = 0;

	while (*ptr)
	{
		sum += *(ptr++);
	}

	sum &= 0xff;

#endif
	return sum;
}

struct SoundEntry
{
	SOUND_ID  id;
	Sound *   sound;
	DWORD     time;
	int       loopCount;

	bool isLooping () { return loopCount != 1; }
	void reset ()
	{
		id = INVALID_SOUND_ID;
		sound = NULL;
		time = 0;
		loopCount = 1;
	}
};

//****************************************************************************
//*                                                                          *
//*  AUDMGR class definition                                                 *
//*                                                                          *
//****************************************************************************

struct DACOM_NO_VTABLE AUDMGR : public IAudioManager, IAggregateComponent
{
	BEGIN_DACOM_MAP_INBOUND(AUDMGR)
	DACOM_INTERFACE_ENTRY(IAudioManager)
	END_DACOM_MAP()

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "Simple AUDMGR instance");
	}

	//
	// allow only one copy of the audio manager to run at a time
	//
	static struct AUDMGR * global_instance;

	//
	// Sound file names are stored in a hash table which is 
	// expandable on demand in blocks of 128 entries
	//

	HashPool <SNDFILE_TYPE, 128> filenames;
	SoundEntry *                 sounds;     // sounds[max_sounds]
	int                          max_sounds;

	BOOL32     initialized;

	U32        current_ID;			// Current SOUND_ID available for allocation

	LPDIRECTSOUND lpDS;				// DirectSound provider in use, if any

	//
	// Constructor/destructor
	//

	AUDMGR::AUDMGR()
	{
		initialized = FALSE;
	}

	AUDMGR::~AUDMGR()
	{
		global_instance = 0;

		if (initialized)
		{
			shutdown();
		}
	}

   //
   // Support instance creation
   // 

   GENRESULT init (AGGDESC * info);
#if TEMP_CODE
   GENRESULT init (DACOMDESC * info);
#endif


   //
   // IAudioManager functions
   //

   virtual BOOL32 COMAPI startup    (U32     digital_device_ID   = 0,
                                     U32     MIDI_device_ID  = 0,
                                     U32     data_cache_KB  = 4096,
                                     U32     maximum_handles = 32,
                                     U32     output_rate  = 22050,
                                     U32     output_bits  = 16,
                                     U32     output_channels  = 2,
									 HWND    hwnd=0);

   virtual void   COMAPI shutdown   (void);

   virtual BOOL32         COMAPI precache(const  C8          *name, 
                                          S32                 mode,
                                          struct IFileSystem *parent = 0);

   virtual BOOL32         COMAPI precache(const C8       *name,
                                          S32             mode,
                                          const PROPERTY *properties);

   virtual SOUND_ID       COMAPI get_ID  (const C8 *name);

   virtual SOUND_ID       COMAPI get_ID  (const PROPERTY *properties);

   virtual void           COMAPI play    (SOUND_ID sound,
                                          S32      ramp_milliseconds = 0);

   virtual void           COMAPI stop    (SOUND_ID sound,
                                          S32      ramp_milliseconds = 0);

   virtual void           COMAPI resume  (SOUND_ID sound,
                                          S32      ramp_milliseconds = 0);

   virtual U32            COMAPI get_status(SOUND_ID sound);

   virtual void           COMAPI set_volume(SOUND_ID sound,
                                            SINGLE   volume,
                                            S32      ramp_milliseconds = 0);

   virtual void           COMAPI set_sound_bearing(SOUND_ID       sound,
                                                   const Vector  &bearing,
                                                   SINGLE         falloff_3dB);

   virtual void           COMAPI set_speaker_level (SOUND_ID      sound,
                                                    U32           spkr_bitmask,
                                                    SINGLE        level);

   virtual void           COMAPI set_relative_rate (SOUND_ID      sound,
                                                    SINGLE        rate_percent);

   virtual void           COMAPI set_loop_count    (SOUND_ID      sound,
                                                    S32           loop_cnt);

   virtual IDirectSound*  COMAPI get_DirectSound_provider  (void);

	/* IAggregateComponent methods */

	DEFMETHOD(Initialize) (void);

	//
	// Private (internal) methods
	//

	IDAComponent * getBase (void)
	{
		return (IAudioManager *) this;
	}

	SoundEntry *lookup (SOUND_ID id);
};

//
// Static data members
//

LPDIRECTSOUND       Sound::lpDS = NULL;
LPDIRECTSOUNDBUFFER Sound::lpPrimary = NULL;
int                 Sound::invocation = 1;
AUDMGR *            AUDMGR::global_instance;

//
// Routines
//

//==========================================================================
// Sound class methods
//==========================================================================

Sound::Sound ()
{
	lvol = 0;
	base_vol = 0;
	current_vol = 0;
	active = false;
	lpDSBuff = NULL;
	invoke = 0;
	next = prev = original = this;
}

Sound::Sound (const char *filename)
{
	lvol = 0;
	base_vol = 0;
	current_vol = 0;
	active = false;
	lpDSBuff = NULL;
	invoke = 0;

	// Load the given .WAV file.
	load (filename);

	next = prev = original = this;
}

Sound::Sound (Sound *root)
{
	lvol = 0;
	base_vol = 0;
	current_vol = 0;
	active = false;
	// Protected constructor used to create duplicates.
	ASSERT (lpDS != NULL);
	ASSERT (root->isLoaded());

	original = root;
	HRESULT hr = lpDS->DuplicateSoundBuffer (root->lpDSBuff, &lpDSBuff);
	if (hr != DD_OK)
	{
		switch (hr)
		{
		case DSERR_ALLOCATED:
			GENERAL_WARNING ("Failed to duplicate sound buffer: DSERR_ALLOCATED.\n");
			// *** We could create a new sound buffer here, then lock the root and copy the
			// *** bits from the root to this buffer. This could be a workaround for that problem.
			{
				GENERAL_TRACE_1 ("Attempting duplication by hand.\n");
				if (!create (&root->file))
				{
					GENERAL_TRACE_1 ("Failed by-hand duplication of the sound buffer.\n");
				}
			}
			break;

		case DSERR_OUTOFMEMORY:
			GENERAL_WARNING ("Failed to duplicate sound buffer: DSERR_OUTOFMEMORY.\n");
			break;

		default:
			GENERAL_WARNING (TEMPSTR("Failed to duplicate sound buffer: Code == 0x%x.\n", hr));
			break;
		}
	}
	invoke = original->invoke;
	file = original->file;

	prev = root->prev;
	next = root;

	root->prev->next = this;
	root->prev = this;
}

Sound::~Sound ()
{
	active = false;

	// Remove this sound from the duplicate list
	prev->next = next;
	next->prev = prev;
	next = prev = NULL;

	// Free the audio resources
	free ();
}

Sound * Sound::get_inactive_copy()
{
	Sound *here = original;

	while (here->active)
	{
		here = here->next;
		if (here == original)
		{
			// We have looped back, so create a new one and return it.
			return new Sound (original);
		}
	}

	return here;
}

bool Sound::create (SoundFile *srcFile)
{
	ASSERT (lpDSBuff == NULL);
	ASSERT (lpDS != NULL);

	bool returnValue = false;

	if (srcFile != NULL && srcFile->samples != NULL)
	{
		// Create a buffer in the same format as the file data and
		// big enough to hold the entire sample.

		DSBUFFERDESC desc;
		WAVEFORMATEX wavFormat;

		memset (&wavFormat, 0, sizeof(wavFormat));
		wavFormat.wFormatTag = WAVE_FORMAT_PCM;
		wavFormat.nChannels = srcFile->format.num_channels;
		wavFormat.nSamplesPerSec = srcFile->format.samples_per_sec; 
		wavFormat.nAvgBytesPerSec = srcFile->format.samples_per_sec * srcFile->format.bytes_per_sample;
		wavFormat.nBlockAlign = srcFile->format.bytes_per_sample;
		wavFormat.wBitsPerSample = srcFile->format.bytes_per_channel*8;

		memset (&desc, 0, sizeof (desc));
		desc.dwSize = sizeof(desc);
		desc.dwFlags = DSBCAPS_CTRLDEFAULT | DSBCAPS_STATIC | DSBCAPS_GETCURRENTPOSITION2;
		desc.dwBufferBytes = srcFile->length;
		desc.lpwfxFormat = &wavFormat;
		
		HRESULT result = lpDS->CreateSoundBuffer (&desc, &lpDSBuff, NULL);
		if (result == DS_OK)
		{
			// Lock the buffer and store the samples into it.

			LPVOID ptr;
			DWORD  len;
			result = lpDSBuff->Lock (0, srcFile->length, &ptr, &len, NULL, NULL, DSBLOCK_ENTIREBUFFER);
			if (result == DS_OK)
			{
				memcpy (ptr, srcFile->samples, srcFile->length);
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
	}

	return returnValue;
}

bool Sound::load (IFileSystem *fs)
{
	ASSERT (lpDSBuff == NULL);
	ASSERT (lpDS != NULL);

	bool returnValue = false;

	if (fs != NULL && LoadWAV(fs, file))
	{
		// Create a buffer in the same format as the file data and
		// big enough to hold the entire sample.
	
		returnValue = create (&file);
	}

	return returnValue;
}

bool Sound::load (const char *filename)
{
	// Use WAVLib to load the given filename.

	COMPTR<IFileSystem> fs;
	DAFILEDESC fdesc = filename;
	DACOM_Acquire()->CreateInstance (&fdesc, fs);

	return load (fs);
}

bool Sound::free ()
{
	if (lpDSBuff != NULL)
	{
		if (invocation == invoke)
		{
			lpDSBuff->Release();
		}
		lpDSBuff = NULL;
	}

	if (file.samples != NULL)
	{
		delete file.samples;
		file.samples = NULL;
	}
	return true;
}

float Sound::getLength()
{
	ASSERT (lpDSBuff != NULL);

	return (float) file.num_samples / (float) file.format.samples_per_sec;
}

unsigned int Sound::getSampleCount()
{
	ASSERT (lpDSBuff != NULL);

	return file.num_samples;
}

#if 0
bool Sound::play (float fromWhere, bool looping)
{
	ASSERT (invoke == invocation);
	ASSERT (lpDSBuff != NULL);

	// Convert the given floating point value into a sample position.

	return play ((unsigned int)(fromWhere * file.format.samples_per_sec));
}
#endif

bool Sound::play (unsigned int fromWhere, bool looping)
{
	ASSERT (invoke == invocation);
	ASSERT (lpDSBuff != NULL);

	// Stop the sound. Set the position. Play the sound.

	HRESULT result = lpDSBuff->Stop ();
	if (result == DS_OK)
	{
		result = lpDSBuff->SetCurrentPosition (fromWhere);
		if (result == DS_OK)
		{
			result = lpDSBuff->Play (0,0,looping ? DSBPLAY_LOOPING : 0);
			if (result == DS_OK)
			{
				return true;
			}
			else if (result == DSERR_BUFFERLOST)
			{
				GENERAL_WARNING ("A sound buffer has been lost.\n");
			}
		}
	}

	return false;
}

bool Sound::stop ()
{
	ASSERT (invoke == invocation);
	ASSERT (lpDSBuff != NULL);

	// Stop the sound.

	HRESULT result = lpDSBuff->Stop ();

	if (result == DS_OK)
	{
		return true;
	}

	return false;
}

void Sound::set_volume (float atten)
{
	ASSERT (invoke == invocation);
	ASSERT (lpDSBuff != NULL);

	// Take the base volume into account. Attenuation is the sum of the base and the given
	// attenuations

	current_vol = atten;
	float vol = current_vol + base_vol;

	const float min_vol = ((float) DSBVOLUME_MIN) / 100.0f;
	const float max_vol = ((float) DSBVOLUME_MAX) / 100.0f;

	// Clip the volume to the maximum range.
	if (vol > max_vol)
	{
		vol = max_vol;
	}
	else if (vol < min_vol)
	{
		vol = min_vol;
	}
	
	lvol = (LONG) (vol * 100);

	// Double check the integer volume to ensure that we don't underflow 
	if (lvol < DSBVOLUME_MIN)
	{
		lvol = DSBVOLUME_MIN;
	}

	lpDSBuff->SetVolume (lvol);
}

void Sound::set_pan (float pan)
{
	ASSERT (invoke == invocation);
	ASSERT (lpDSBuff != NULL);

	const float pan_left = ((float) DSBPAN_LEFT) / 100.0f;
	const float pan_right = ((float) DSBPAN_RIGHT) / 100.0f;

	// Clip the pan to the valid range.
	if (pan > pan_right)
	{
		pan = pan_right;
	}
	else if (pan < pan_left)
	{
		pan = pan_left;
	}

	LONG lpan = (LONG) (pan * 100);
	
	// Ensure that the integer version of the pan is also in the valid integer range.
	if (lpan > DSBPAN_RIGHT)
	{
		lpan = DSBPAN_RIGHT;
	}
	else if (lpan < DSBPAN_LEFT)
	{
		lpan = DSBPAN_LEFT;
	}

	lpDSBuff->SetPan (lpan);
}

void Sound::set_base_volume (float atten)
{
	if (atten > 0)
	{
		atten = 0;
	}
	if (atten < -100)
	{
		atten = -100;
	}

	base_vol = atten;
	set_volume (current_vol);
}

void Sound::set_sound_bearing (const Vector& bearing, float falloff_3db)
{
	//
	// Assumes a bearing of 0,0,-1 to be straight ahead.
	//

	SINGLE falloff_factor = bearing.magnitude() / falloff_3db;

	SINGLE falloff = -3.0f * falloff_factor * falloff_factor;

	const DOUBLE offset = 30.0 / 180.0 * 3.14159265358979323846;

	Vector left_speaker_dir(-(SINGLE)sin(offset), 0.0, -(SINGLE)cos(offset));
	Vector right_speaker_dir((SINGLE)sin(offset), 0.0, -(SINGLE)cos(offset));

	Vector bearing_2D(bearing.x, 0.0, -(SINGLE)fabs(bearing.z));
	bearing_2D.normalize();

	SINGLE lvol = SINGLE(dot_product(left_speaker_dir,  bearing_2D));
	SINGLE rvol = SINGLE(dot_product(right_speaker_dir, bearing_2D));

	// Now, falloff is the distance scaled volume, and the lvol and rvol are the percentage attenuations of
	// the left and right speakers, respectively. As the bearing approaches a side, that side will approach 1 and the
	// other side will approach zero. At the center, the two values will be equal.

	// Set the volume to the falloff, and the pan to a value based on lvol and rvol

	set_volume (falloff);
	set_pan (-100 * lvol + 100 * rvol);
}

void Sound::set_relative_rate(float rate_percent)
{
	ASSERT (invoke == invocation);
	ASSERT (lpDSBuff != NULL);

	DWORD freq = (DWORD) (file.format.samples_per_sec * rate_percent);
	lpDSBuff->SetFrequency (freq);
}

bool Sound::getStatus (bool &playing, unsigned int &playCursor)
{
	ASSERT (invoke == invocation);
	ASSERT (lpDSBuff != NULL);

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
bool Sound::startup (HWND hWnd, SoundFormat *primaryFormat)
{
//	ASSERT (hWnd != NULL);
	ASSERT (lpDS == NULL);
	ASSERT (lpPrimary == NULL);

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

	// Set the primary format, if specified.
	if (primaryFormat != NULL)
	{
		DSBUFFERDESC desc;
		WAVEFORMATEX wavFormat;

		memset (&wavFormat, 0, sizeof(wavFormat));
		wavFormat.wFormatTag = WAVE_FORMAT_PCM;
		wavFormat.nChannels = primaryFormat->num_channels;
		wavFormat.nSamplesPerSec = primaryFormat->samples_per_sec; 
		wavFormat.nAvgBytesPerSec = primaryFormat->samples_per_sec * primaryFormat->bytes_per_sample;
		wavFormat.nBlockAlign = primaryFormat->bytes_per_sample;
		wavFormat.wBitsPerSample = primaryFormat->bytes_per_channel*8;

		memset (&desc, 0, sizeof (desc));
		desc.dwSize = sizeof(desc);
		desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
		desc.dwBufferBytes = 0;
		desc.lpwfxFormat = NULL; // Must be NULL for primary buffers. 
		
		HRESULT result = lpDS->CreateSoundBuffer (&desc, &lpPrimary, NULL);
		if (result == DS_OK)
		{
			// Set the primary buffer format.
			result = lpPrimary->SetFormat (&wavFormat);
			if (result != DS_OK)
			{
				GENERAL_TRACE_1 ("Failed to set primary buffer format. Default output format will be used.\n");
				lpPrimary->Release();
				lpPrimary = NULL;
			}
		}
		else
		{
			GENERAL_TRACE_1 ("Failed to create primary buffer. Default output format will be used.\n");
			lpPrimary = NULL;
		}
	}

	// All is well.
	return true;
}

bool Sound::shutdown ()
{
	// Release the DirectSound object, which automatically releases the sound buffers.
	// NOTE: This automatically makes all Sound objects invalid. Any attempt to
	// use a Sound object in this state will ASSERT.

	if (lpPrimary != NULL)
	{
		lpPrimary->Release();
		lpPrimary = NULL;
	}

	if (lpDS != NULL)
	{
		lpDS->Release();
		lpDS = NULL;
	}

	// Increment the invocation token, which prevents the use of bad Sound objects.
	if (++invocation == 0)
	{
		++invocation;
	}

	return true;
}


//==========================================================================
// AUDMGR Methods
//==========================================================================

//****************************************************************************
//*                                                                          *
//* init()                                                                   *
//*                                                                          *
//****************************************************************************

GENRESULT AUDMGR::init (AGGDESC * info)
{
	if (global_instance)
	{
		return GR_GENERIC;		// only allow one instance to run at a time
	}
	global_instance = this;
	return GR_OK;
}

//****************************************************************************
//*                                                                          *
//* startup()                                                                *
//*                                                                          *
//****************************************************************************

BOOL32 COMAPI AUDMGR::startup    (U32     digital_device_ID, //)
                                  U32     MIDI_device_ID,
                                  U32     data_cache_KB,
                                  U32     maximum_handles,
                                  U32     output_rate,
                                  U32     output_bits,
                                  U32     output_channels,
								  HWND    hWnd)
{
	//
	// Fail if already started
	//

	if (initialized)
	{
		return FALSE;
	}

	//
	// Fail if neither digital nor MIDI service requested
	//

	if ((digital_device_ID == 0xFFFFFFFF) && (MIDI_device_ID == 0xFFFFFFFF)) 
	{
		return FALSE;
	}

	//
	// Start up Sound API
	//

	SoundFormat fmt;
	fmt.num_channels = (unsigned short) output_channels;
	fmt.bytes_per_channel = (unsigned short) output_bits / 8;
	fmt.samples_per_sec = (unsigned short) output_rate;
	fmt.bytes_per_sample = fmt.num_channels * fmt.bytes_per_channel;

	if (!Sound::startup (hWnd, &fmt))
	{
		return FALSE;
	}

	sounds = new SoundEntry[maximum_handles];
	max_sounds = maximum_handles;
	for (int i = 0; i < max_sounds; ++i)
	{
		sounds[i].reset ();
	}

	//
	// NOTE: We are ignoring cache size, and MIDI.
	//

	//
	// Reset sample ID pool
	//

	current_ID = 0;

	//
	// Return success
	//

	initialized = TRUE;

	return TRUE;
}

//****************************************************************************
//*                                                                          *
//* shutdown()                                                               *
//*                                                                          *
//****************************************************************************

void COMAPI AUDMGR::shutdown(void)
{
	if (!initialized)
	{
		return;
	}

	// *** TODO: Free all of the allocated sounds before shutting down Sound.
	Sound::shutdown();

	initialized = FALSE;
}

//****************************************************************************
//*                                                                          *
//* precache()                                                               *
//*                                                                          *
//****************************************************************************

BOOL32         COMAPI AUDMGR::precache(const  C8          *name,
                                       S32                 mode,
                                       struct IFileSystem *parent)
{
	// NOTE: We are not supporting streaming audio.

	S32 index = filenames.search(name);

	if (index != -1)
	{
		// If already cached, keep it.
		return TRUE;
	}

	//
	// Create a new Sound and load the given file
	//

	DAFILEDESC desc(name);

	COMPTR<IFileSystem> fs;

	IComponentFactory *factory = parent;
	if (factory == NULL)
	{
		factory = DACOM_Acquire();
	}
	if (factory->CreateInstance(&desc, fs) != GR_OK)
	{
		return FALSE;
	}

	Sound *sound = new Sound;
	if (!sound->load(fs))
	{
		delete sound;
		return FALSE;
	}

	//
	// Allocate slot for the sound and store the sound in it
	//

	index = filenames.allocate(name);

	SNDFILE_TYPE *slot = &filenames.list[index];
	slot->sound = sound;
	slot->mode = mode;

	return true;
}

//****************************************************************************
//*                                                                          *
//* precache()                                                               *
//*                                                                          *
//****************************************************************************

BOOL32         COMAPI AUDMGR::precache(const C8       *name, //)
                                       S32             mode,
                                       const PROPERTY *properties)
{
   return TRUE;
}

//****************************************************************************
//*                                                                          *
//* get_ID()                                                                 *
//*                                                                          *
//****************************************************************************


SOUND_ID       COMAPI AUDMGR::get_ID  (const C8 *name)
{
	//
	// Find data for this sound
	//

	S32 index = filenames.search(name);

	if (index == -1)
	{
		return INVALID_SOUND_ID;
	}

	SNDFILE_TYPE *slot = &filenames.list[index];

	// not going to simply unlink this from filename list
	// figure that the app will probably continue asking for it bad names
	if (!slot->sound->isLoaded())
	{
		return INVALID_SOUND_ID;
	}

	//
	// Find a slot in the active sound list.
	// First, attempt to find an empty slot.
	// Then, look for the oldest, non-playing sound.
	// Then, look for the oldest, non-looping sound.
	// Then, look for the oldest sound
	//

	DWORD minTime;
	LONG minVol;
	bool playing;
	unsigned int playCursor;
	int killMe = -1;
	int i;
	for (i = 0; i < max_sounds; ++i)
	{
		if (sounds[i].sound == NULL)
		{
			killMe = i;
			break;
		}
	}

	if (killMe == -1)
	{
		// No empty slots, so look for the oldest non-playing sound.
		minTime = 0xFFFFFFFF;
		for (i = 0; i < max_sounds; ++i)
		{
			sounds[i].sound->getStatus(playing, playCursor);
			if (!playing && sounds[i].time < minTime)
			{
				killMe = i;
				minTime = sounds[i].time;
			}
		}
	}

	if (killMe == -1)
	{
		// No non-playing sounds, so look for oldest or softest, non-looping sound
		minTime = 0xFFFFFFFF;
		minVol = 0;
		int oldest = -1;
		int softest = -1;
		for (i = 0; i < max_sounds; ++i)
		{
			if (!sounds[i].isLooping())
			{
				if (sounds[i].time < minTime)
				{
					oldest = i;
					minTime = sounds[i].time;
				}
				if (sounds[i].sound->lvol < minVol)
				{
					softest = i;
					minVol = sounds[i].sound->lvol;
				}
			}
		}

//		killMe = softest;
		killMe = oldest;
	}

	if (killMe == -1)
	{
		// No non-playing, non-looping sounds, so find the oldest

		GENERAL_TRACE_1("All sounds are looping! Killing overall oldest sound.\n");

		minTime = 0xFFFFFFFF;
		for (i = 0; i < max_sounds; ++i)
		{
			if (sounds[i].time < minTime)
			{
				killMe = i;
				minTime = sounds[i].time;
			}
		}
	}

	ASSERT (killMe != -1);

	// Stop and deactivate the sample

	if (sounds[killMe].sound)
	{
		sounds[killMe].sound->stop();
		sounds[killMe].sound->active = false;
	}

	// Get an inactive copy of the desired sound, storing its pointer into the
	// handle array.

	sounds[killMe].reset();
	sounds[killMe].sound = slot->sound->get_inactive_copy();
	if (sounds[killMe].sound->isLoaded())
	{
		sounds[killMe].sound->set_base_volume (0);
		sounds[killMe].sound->set_volume (0);
		sounds[killMe].sound->set_pan (0);
		sounds[killMe].sound->active = true;
		sounds[killMe].time = GetTickCount();
		sounds[killMe].id = ++current_ID;

		return current_ID;
	}
	else
	{
		// Delete the sound and clear the slot because the sound failed to load.
		delete sounds[killMe].sound;
		sounds[killMe].sound = NULL;
	}

	return INVALID_SOUND_ID;
}

//****************************************************************************
//*                                                                          *
//* get_ID()                                                                 *
//*                                                                          *
//****************************************************************************

SOUND_ID       COMAPI AUDMGR::get_ID  (const PROPERTY *properties)
{                             
   return 0;                  
}

//****************************************************************************
//*                                                                          *
//* play()                                                                   *
//*                                                                          *
//****************************************************************************

void COMAPI AUDMGR::play(SOUND_ID sound_id,
                         S32      ramp_milliseconds)
{
	SoundEntry *s = lookup(sound_id);

	if (s)
	{
		s->time = GetTickCount();
		s->sound->play(0, s->loopCount != 1);
	}
}

//****************************************************************************
//*                                                                          *
//* stop()                                                                   *
//*                                                                          *
//****************************************************************************

void COMAPI AUDMGR::stop(SOUND_ID sound_id,
                         S32      ramp_milliseconds)
{
	SoundEntry *s = lookup(sound_id);

	if (s)
	{
		s->sound->stop();
	}
}

//****************************************************************************
//*                                                                          *
//* resume()                                                                 *
//*                                                                          *
//****************************************************************************

void  COMAPI AUDMGR::resume(SOUND_ID sound_id,
                            S32      ramp_milliseconds)
{
	// NOTE: This will start from the beginning.
	SoundEntry *s = lookup(sound_id);

	if (s)
	{
		GENERAL_WARNING ("Resuming sounds restarts the sound at the beginning.\n");
		s->time = GetTickCount();
		s->sound->play(0, s->loopCount != 1);
	}
}

//****************************************************************************
//*                                                                          *
//* get_status()                                                             *
//*                                                                          *
//****************************************************************************

U32 COMAPI AUDMGR::get_status(SOUND_ID sound_id)
{
	U32 result = SOUND_DONE;

	SoundEntry *s = lookup(sound_id);

	if (s)
	{
		bool playing;
		unsigned int playCursor;
		s->sound->getStatus(playing, playCursor);

		if (playing)
		{
			result = SOUND_PLAYING;
		}
	}

	return result;
}

//****************************************************************************
//*                                                                          *
//* set_volume()                                                             *
//*                                                                          *
//****************************************************************************

void COMAPI AUDMGR::set_volume(SOUND_ID sound_id,
                               SINGLE   volume, // In decibel reduction
                               S32      ramp_milliseconds)
{
	SoundEntry *s = lookup(sound_id);

	if (s)
	{
		s->sound->set_base_volume(volume);
	}
}

//****************************************************************************
//*                                                                          *
//* set_sound_bearing()                                                      *
//*                                                                          *
//****************************************************************************

void COMAPI AUDMGR::set_sound_bearing(SOUND_ID      sound_id,
                                      const Vector& bearing,
                                      SINGLE        falloff_3db)
{
	SoundEntry *s = lookup(sound_id);

	if (s)
	{
		s->sound->set_sound_bearing(bearing, falloff_3db);
	}
}

//****************************************************************************
//*                                                                          *
//* set_speaker_level()                                                      *
//*                                                                          *
//****************************************************************************

void COMAPI AUDMGR::set_speaker_level (SOUND_ID sound_id,
                                       U32      speaker_bitmask,
                                       SINGLE   level)
{
}

//****************************************************************************
//*                                                                          *
//* set_relative_rate()                                                      *
//*                                                                          *
//****************************************************************************

void COMAPI AUDMGR::set_relative_rate (SOUND_ID sound_id,
                                       SINGLE   rate_percent)
{
	SoundEntry *s = lookup(sound_id);

	if (s)
	{
		s->sound->set_relative_rate(rate_percent);
	}
}

//****************************************************************************
//*                                                                          *
//* set_loop_count()                                                         *
//*                                                                          *
//****************************************************************************

void COMAPI AUDMGR::set_loop_count(SOUND_ID sound_id,
                                   S32      loop_count)
{
	// Make the given sound looping if the loop_count is not equal to 1.
	SoundEntry *s = lookup(sound_id);

	if (s)
	{
		s->loopCount = loop_count;

		// If the sound is currently playing, change its looping state.

		bool playing;
		unsigned int playCursor;
		s->sound->getStatus(playing, playCursor);

		if (playing)
		{
			// Don't need to stop the sound, just play again with the looping flag.
			// We go directly to directX here to bypass the set position inherint in play()
			s->sound->lpDSBuff->Play(0,0, (s->loopCount != 1) ? DSBPLAY_LOOPING : 0);
		}
	}
}

//****************************************************************************
//*                                                                          *
//* get_DirectSound_provider()                                               *
//*                                                                          *
//****************************************************************************

IDirectSound* COMAPI AUDMGR::get_DirectSound_provider(void)
{
	if (Sound::lpDS != NULL)
	{
		Sound::lpDS->AddRef();   
	}

	return Sound::lpDS;
}

//****************************************************************************
//*                                                                          *
//* Initialize()                                                             *
//*                                                                          *
//****************************************************************************

GENRESULT AUDMGR::Initialize (void)
{
	return GR_OK;
}

// Local methods

SoundEntry *AUDMGR::lookup (SOUND_ID id)
{
	for (int i = 0; i < max_sounds; ++i)
	{
		if (sounds[i].id == id && sounds[i].sound != NULL)
		{
			return &sounds[i];
		}
	}

	return NULL;
}

//==========================================================================
// DLL startup code
//==========================================================================

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

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		//
		// DLL_PROCESS_ATTACH: Create object server component and register it 
		// with DACOM manager
		//

		HEAP = HEAP_Acquire();
		SetDllHeapMsg(hinstDLL);

		server = new DAComponentFactory2<DAComponentAggregate<AUDMGR>, AGGDESC> (interface_name);
		DACOM_Acquire()->RegisterComponent(server, interface_name, DACOM_LOW_PRIORITY);
		server->Release();
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}
