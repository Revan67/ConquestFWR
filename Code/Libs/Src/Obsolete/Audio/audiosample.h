#ifndef AUDIO_SAMPLE_H
#define AUDIO_SAMPLE_H

#include "audiomgr.h"
#include "audiofile.h"
#include "mssw.h"

class AudioSample
{
public:
	//
	// Static functions to manage all audio samples
	//

	static void startup(HDIGDRIVER, U32 max_hsamples);
	static void shutdown();

	static void new_sample(SOUND_ID, AudioFile* file, U32 mode);

	static AudioSample* lookup(SOUND_ID);
	static AudioSample* lookup(HSAMPLE);

	//
	// Per sample member functions
	//

	void play(S32 ramp_milliseconds);
	void stop(S32 ramp_milliseconds);
	void resume(S32 ramp_milliseconds);

	U32 get_status();

	void set_volume(SINGLE decibel_reduction, U32 ramp_milliseconds);

	void set_sound_bearing(const Vector& bearing, SINGLE falloff_3dB);

	void set_relative_rate (SINGLE rate_percent);

	void set_loop_count(U32 loop_count);

	U32 get_loop_count();

private: // Member functions

	AudioSample(HDIGDRIVER);

	void acquire(SOUND_ID, AudioFile* file, U32 mode);
   void release();

	void play_done();

	void start_ramp(U32 ramp_milliseconds,
						 U32 initial_volume_arg,
						 U32 target_volume_arg);
	void stop_ramp();

	void update_ramp(U32 current_time);

	void start_stream_updates();

	void stop_stream_updates();

	void update_stream();

	void set_timestamp(U32 time);

	U32 apply_base_volume (U32 vol);


	static void AILCALLBACK timer_callback(U32);

	static void AILCALLBACK play_done_callback(HSAMPLE);

private: // Data

	// Initialization data received from AUDMGR hack of AIL

	S32      sample_size;
	S32		sample_offset;
	S32		initial_sample_rate;
	S16      channels;
	S16      bits_per_sample;

	// Data maintainted internally

	HSAMPLE hsample1; // Main sample used for all types of sound
	HSAMPLE hsample2; // Inverted sample used for Prologic sound

	AudioFile* file;

	AudioBuffer* sample_buffer;

	SOUND_ID sound_id;
	S32		target_time;
	S32		target_volume;  // destination volume during volume ramp
	S32		normal_volume;  // current volume 
	S32     base_volume;    // the volume of the sound with normal_volume is 127.
	S32		stream_offset;
	S32		stream_size;
	S32		loop_count;
	S32		loops_made;

	U32		prologic_mode;
	U32		streaming_mode;
	U32		play_timestamp;
	U32		access_timestamp;

	bool		surround_active;

	static AudioSample** samples;
	static U32           max_samples;

	static HTIMER timer;		// Timer for ramping volumes

	static U32 ramping_count;
	static AudioSample** ramping_samples;

	static U32 streaming_count;
	static AudioSample** streaming_samples;
};

#endif