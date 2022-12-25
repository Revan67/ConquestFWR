//****************************************************************************
//*                                                                          *
//*  AUDIOMGR.CPP: DA COM audio services                                     *
//*                                                                          *
//*  Source compatible with 32-bit 80386 C/C++                               *
//*                                                                          *
//*  V1.00 of 9-Jan-98: Initial                                              *
//*                                                                          *
//*  32-bit protected-mode source compatible with Watcom 10.5/MSC 9.0        *
//*                                                                          *
//*  Author: John Miles                                                      *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  Copyright (C) 1997 Digital Anvil, Inc.                                  *
//*                                                                          *
//****************************************************************************

#include <windows.h>
#include <dsound.h>

#include "dacom.h"                    // DA component manager
#include "heapobj.h"
#include "stddat.h"
#include "TSmartPointer.h"
#include "TComponent.h"
#include "System.h"
#include "davariant.h"
#include "audiomgr.h"

#include "mssw.h"

#include "audiosample.h"
#include "audiofile.h"
#include "IDumpText.h"

#include <stdio.h>
#include <stdlib.h>

IDumpText * DUMP;

C8 *interface_name = "IAudioManager"; // Interface name used for registration

#define TEMP_CODE 1		// remove this later

//****************************************************************************
//*                                                                          *
//* debug_printf()                                                           *
//*                                                                          *
//****************************************************************************

void __cdecl debug_printf(char *fmt, ...)
{
   char work_string[4096];

   if ((fmt == NULL) || (strlen(fmt) > sizeof(work_string)))
      {
      strcpy(work_string, "(String missing or too large)");
      }
   else
      {
      va_list ap;

      va_start(ap, 
               fmt);

      wvsprintf(work_string, 
               fmt, 
               ap);

      va_end  (ap);
      }

   if (DUMP)
	   DUMP->debug_printf(work_string);
   else
      OutputDebugString(work_string);
}

//****************************************************************************
//*                                                                          *
//*  Structure which represents a loaded sound file, indexed by filename     *
//*                                                                          *
//*  TBD: Allow the *data to be part of a large cache, rather than simply    *
//*  allocated at precache() time and never freed.                           *
//*                                                                          *
//****************************************************************************

//
// Filenames are stored as literal ASCII strings limited to MAX_PATH 
// characters in size, with an associated 8-bit checksum which is used as a 
// hash key for fast lookup
//

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
      strcpy(name, (C8 *) object);

      file = NULL;
      }

   //
   // HashPool shutdown function -- called when unlinking entry from pool
   //

   inline void shutdown(void)
      {
         if (file)
         {
            delete file;
            file = NULL;
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

   AudioFile* file;

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
		return HEAP->ClearAllocateMemory(size, "AUDMGR instance");
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

   BOOL32     initialized;

   HMDIDRIVER mdi;					// Handle to MIDI driver
   HDIGDRIVER dig;					// Handle to digital driver

   U32        current_ID;			// Current SOUND_ID available for allocation

   LPDIRECTSOUND lpDS;				// DirectSound provider in use, if any

	HSAMPLE acquire_handle();
	void    release_handle(HSAMPLE);

   HSAMPLE find_handle(SOUND_ID);
   //
   // Constructor/destructor
   //

   AUDMGR::AUDMGR()
      {
      initialized = FALSE;
      }

   AUDMGR::~AUDMGR()
      {
	     DUMP = 0;
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

private:
	static void AILCALLBACK play_done_callback(HSAMPLE S);
	static void AILCALLBACK timer_callback(U32 user);
	
	void start_ramp(HSAMPLE S, S32 ramp_milliseconds,
		             S32 initial_volume, S32 target_volume);

	void stop_ramp(HSAMPLE S);
};
struct AUDMGR * AUDMGR::global_instance;

//--------------------------------------------------------------------------
//-----------------------StartUp code---------------------------------------
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//  
void main (void)
{
}

//--------------------------------------------------------------------------
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
      //
      // DLL_PROCESS_ATTACH: Create object server component and register it 
      // with DACOM manager
      //

      case DLL_PROCESS_ATTACH:

			HEAP = HEAP_Acquire();
			SetDllHeapMsg(hinstDLL);

			server = new DAComponentFactory2<DAComponentAggregate<AUDMGR>, AGGDESC> (interface_name);
			DACOM_Acquire()->RegisterComponent(server, interface_name, DACOM_LOW_PRIORITY);
			server->Release();

#if TEMP_CODE
			server = new DAComponentFactory<DAComponent<AUDMGR>, DACOMDESC> (interface_name);
			DACOM_Acquire()->RegisterComponent(server, interface_name, DACOM_LOW_PRIORITY);
			server->Release();
#endif			
			break;

      case DLL_PROCESS_DETACH:
         break;
      }

   return TRUE;
}

//****************************************************************************
//*                                                                          *
//* init()                                                                   *
//*                                                                          *
//****************************************************************************

GENRESULT AUDMGR::init (AGGDESC * info)
{
	if (global_instance)
		return GR_GENERIC;		// only allow one instance to run at a time
	global_instance = this;
	return GR_OK;
}
#if TEMP_CODE
GENRESULT AUDMGR::init (DACOMDESC * info)
{
	if (global_instance)
		return GR_GENERIC;		// only allow one instance to run at a time
	global_instance = this;
	return GR_OK;
}
#endif
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
   static PCMWAVEFORMAT sPCMWF;

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
   // Start up AIL API
   //

   AIL_startup();

   //
   // Init globals
   // 

   mdi = NULL;
   dig = NULL;

   //
   // Attempt AIL_waveOutOpen(), trying waveOut if DirectSound fails
   //

   if (digital_device_ID != 0xFFFFFFFF)
      {
      sPCMWF.wf.nChannels       = (U16) output_channels;
      sPCMWF.wf.nSamplesPerSec  = output_rate;
      sPCMWF.wBitsPerSample     = (U16) output_bits;

      sPCMWF.wf.wFormatTag      = WAVE_FORMAT_PCM;
      sPCMWF.wf.nBlockAlign     = (sPCMWF.wBitsPerSample / 8) * sPCMWF.wf.nChannels;
      sPCMWF.wf.nAvgBytesPerSec = (U32)sPCMWF.wf.nSamplesPerSec * (U32) sPCMWF.wf.nBlockAlign;

		//
		// Each AudioSample can use up to two HSAMPLE handles
		//

      AIL_set_preference(DIG_MIXER_CHANNELS, maximum_handles * 2);

      if (AIL_waveOutOpen(&dig,
                          0,
                          digital_device_ID,
          (LPWAVEFORMAT) &sPCMWF))
         {

         AIL_set_preference(DIG_USE_WAVEOUT, AIL_get_preference(DIG_USE_WAVEOUT)==FALSE);

         if (AIL_waveOutOpen(&dig,
                              0,
                              digital_device_ID,
               (LPWAVEFORMAT) &sPCMWF))
            {
            AIL_shutdown();
            return FALSE;
            }
         }
      }

   AIL_set_DirectSound_HWND(dig, hWnd);

   //
   // Initialize XMIDI sound system, if requested
   //

   if (MIDI_device_ID != 0xFFFFFFFF)
      {
      AIL_set_preference(MDI_SEQUENCES, maximum_handles);

      if (AIL_midiOutOpen(&mdi,
                           NULL,
                           MIDI_device_ID))
         {
         if (dig) 
            {
            AIL_waveOutClose(dig);
            dig = NULL;
            }

         AIL_shutdown();
         return FALSE;
         }
      }

	AudioSample::startup(dig, maximum_handles);

   //
   // Get pointer to DirectSound provider from first HSAMPLE created
   //

   lpDS = NULL;

   //AIL_get_DirectSound_info(smp[0], (void **) &lpDS, NULL);

   //
   // Reset sample ID pool
   //

   current_ID = 0;

   //
   // TBD: Allocate sound data cache
   //

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

void   COMAPI AUDMGR::shutdown(void)
{                     
   if (!initialized)
      {
      return;
      }

	//
	// The timer thread must be stopped since it may access some of
	// the data being deleted below before the AIL_shutdown call is made.
	//
	AudioSample::shutdown();

   if (dig)
      {
      AIL_waveOutClose(dig);
      dig = NULL;
      }

   if (mdi)
      {
      AIL_midiOutClose(mdi);
      mdi = NULL;
      }

   AIL_shutdown();

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
   S32 index = filenames.search(name);

   if (index != -1)
   {
   // (tlp) now keeping the same entry for multiple calls to precache
      return TRUE;
      
      //If filename already has an entry, remove it
      //filenames.unlink(index);
      //index = -1;      
   }

   //
   // Allocate slot for file
   //

   index = filenames.allocate(name);

   SNDFILE_TYPE *slot = &filenames.list[index];

   //
   // Load file into memory (TBD: Load this file into cache instead, 
   // possibly punting some others out to make room!)
   //

    slot->file = new AudioFile(parent, name, (mode != SOUND_STREAMABLE));
    slot->mode = mode;

   return slot->file->loaded();
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
   // TBD: Make this support MIDI HSEQUENCEs as well!  The file type or
   // extention will have to be examined to identify the data.
   //

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
   if (!slot->file->loaded())
   {
       return INVALID_SOUND_ID;
   }

   //
   // Acquire a sample handle, preparing it for use.
   //

    AudioSample::new_sample(++current_ID, slot->file, slot->mode);

    return current_ID;
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
	AudioSample* sample = AudioSample::lookup(sound_id);

	if (sample) sample->play(ramp_milliseconds);
}

//****************************************************************************
//*                                                                          *
//* stop()                                                                   *
//*                                                                          *
//****************************************************************************

void COMAPI AUDMGR::stop(SOUND_ID sound_id,
                         S32      ramp_milliseconds)
{
	AudioSample* sample = AudioSample::lookup(sound_id);

	if (sample) sample->stop(ramp_milliseconds);
}

//****************************************************************************
//*                                                                          *
//* resume()                                                                 *
//*                                                                          *
//****************************************************************************

void  COMAPI AUDMGR::resume(SOUND_ID sound_id,
                            S32      ramp_milliseconds)
{
	AudioSample* sample = AudioSample::lookup(sound_id);

	if (sample) sample->resume(ramp_milliseconds);
}

//****************************************************************************
//*                                                                          *
//* get_status()                                                             *
//*                                                                          *
//****************************************************************************

U32 COMAPI AUDMGR::get_status(SOUND_ID sound_id)
{
	U32 result = SOUND_DONE;

	AudioSample* sample = AudioSample::lookup(sound_id);

	if (sample) result = sample->get_status();

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
	AudioSample* sample = AudioSample::lookup(sound_id);

	if (sample) sample->set_volume(volume, ramp_milliseconds);
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
	AudioSample* sample = AudioSample::lookup(sound_id);

	if (sample) sample->set_sound_bearing(bearing, falloff_3db);
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
	AudioSample* sample = AudioSample::lookup(sound_id);

	if (sample) sample->set_relative_rate(rate_percent);
}

//****************************************************************************
//*                                                                          *
//* set_loop_count()                                                         *
//*                                                                          *
//****************************************************************************

void COMAPI AUDMGR::set_loop_count(SOUND_ID sound_id,
                                   S32      loop_count)
{
	AudioSample* sample = AudioSample::lookup(sound_id);

	if (sample) sample->set_loop_count(loop_count);
}

//****************************************************************************
//*                                                                          *
//* get_DirectSound_provider()                                               *
//*                                                                          *
//****************************************************************************

IDirectSound* COMAPI AUDMGR::get_DirectSound_provider(void)
{
   if (lpDS != NULL)
      {
      lpDS->AddRef();   
      }

   return lpDS;
}

//****************************************************************************
//*                                                                          *
//* Initialize()                                                             *
//*                                                                          *
//****************************************************************************

GENRESULT AUDMGR::Initialize (void)
{
	if (getBase()->QueryInterface("IDumpText", (void **)&DUMP) == GR_OK)
		getBase()->Release();		// release the extra reference

	return GR_OK;
}
