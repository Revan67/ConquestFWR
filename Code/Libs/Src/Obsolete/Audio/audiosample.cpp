#include <windows.h>

#include <math.h>
#include <stdlib.h>

#include "vector.h"

#include "audiosample.h"
#include "waudmgrsfil.h"
#include "audiofile.h"

//
// Rate at which the volume of a ramped sound gets modified
//

#define RAMP_FREQUENCY	15	// hz

//
// Index equate for AIL_set_sample/sequence_user_data()
//

#define AUDIO_SAMPLE 0

//****************************************************************************
//*                                                                          *
//* static member variables                                                  *
//*                                                                          *
//****************************************************************************

AudioSample** AudioSample::samples = NULL;
U32           AudioSample::max_samples = 0;

HTIMER        AudioSample::timer;

U32           AudioSample::ramping_count = 0;
AudioSample** AudioSample::ramping_samples = NULL;

U32           AudioSample::streaming_count = 0;
AudioSample** AudioSample::streaming_samples = NULL;

//****************************************************************************
//*                                                                          *
//* startup()                                                                   *
//*                                                                          *
//****************************************************************************

void AudioSample::startup(HDIGDRIVER driver, U32 max_samples_arg)
{
	max_samples = max_samples_arg;
	samples = new AudioSample*[max_samples];

	for (U32 i = 0; i < max_samples; i++)
	{			
		samples[i] = new AudioSample(driver);
	}

	//
	// Initialize timer for volume ramping
	//

	timer = AIL_register_timer(timer_callback);
	AIL_set_timer_frequency(timer, RAMP_FREQUENCY);

	AIL_start_timer(timer);

	ramping_samples = new AudioSample*[max_samples];
	ramping_count = 0;

	streaming_samples = new AudioSample*[max_samples];
	streaming_count = 0;
}

//****************************************************************************
//*                                                                          *
//* shutdown()                                                                   *
//*                                                                          *
//****************************************************************************

void AudioSample::shutdown()
{
	AIL_stop_timer(timer);
	for (U32 i = 0; i < max_samples; i++)
	{
		delete samples[i];
	}
	delete[] samples;
	delete[] ramping_samples;
}

//****************************************************************************
//*                                                                          *
//* acquire()                                                                *
//*                                                                          *
//****************************************************************************

void AudioSample::new_sample(U32 sound_id, AudioFile* file, U32 mode)
{
	// The timestamps are set up so that the most recently accessed
	// samples have a greater chance of being retained than others.
	// When a sample completes playing, its access timestamp is set
	// to when play() or resume was last called.  This makes sounds
	// that have been more recently played more likely to be retained.
	//
	U32 timestamp;
	U32 min_timestamp = (U32)-1;
	U32 min_index = 0;

	for (U32 i=0; i < max_samples; i++)
	{
		timestamp = samples[i]->access_timestamp;
		
		if (samples[i]->get_status() != SOUND_PLAYING &&
			timestamp < min_timestamp)
		{
			min_timestamp = timestamp;
			min_index = i;
		}
	}
	//
	// Uh oh... they must all be playing, pick one to kill
	//
	if (min_timestamp == (U32)-1)
	{
		for (U32 i=0; i < max_samples; i++)
		{
			timestamp = samples[i]->access_timestamp;
			
			if (samples[i]->get_loop_count() != 0 &&
				 timestamp < min_timestamp)
			{
				min_timestamp = timestamp;
				min_index = i;
			}
		}
	}
	//
	// Oh crap... they must all be playing, just kill the oldest one
	//
	if (min_timestamp == (U32)-1)
	{
		for (U32 i=0; i < max_samples; i++)
		{
			timestamp = samples[i]->access_timestamp;
			
			if (timestamp < min_timestamp)
			{
				min_timestamp = timestamp;
				min_index = i;
			}
		}
	}
	AudioSample* sample = samples[min_index];

	sample->release(); // Even if it wasn't in use

	sample->acquire(sound_id, file, mode);
}

//****************************************************************************
//*                                                                          *
//* Constructor                                                              *
//*                                                                          *
//****************************************************************************

AudioSample::AudioSample(HDIGDRIVER driver) :
	sample_size(0),
	sample_offset(0),
	initial_sample_rate(0),
	channels(0),
	bits_per_sample(0),
	hsample1(AIL_allocate_sample_handle(driver)),
	hsample2(AIL_allocate_sample_handle(driver)),
	sound_id(0),
	target_time(0),
	target_volume(0),
	normal_volume(0),
	base_volume(0),
	play_timestamp(0),
	access_timestamp(0),
	loop_count(1),
	loops_made(0),
	prologic_mode(FALSE),
	streaming_mode(FALSE),
	surround_active(FALSE),
	sample_buffer(NULL)
{
}

//****************************************************************************
//*                                                                          *
//* acquire()                                                                *
//*                                                                          *
//****************************************************************************

void AudioSample::acquire(SOUND_ID sound_id_arg, AudioFile* file_arg, U32 mode)
{
	file = file_arg;

   AIL_init_sample(hsample1);

   AUDMGR_set_sample_file(hsample1, file, 0,
								  &channels,
								  &initial_sample_rate,
								  &bits_per_sample,
								  &sample_size,
								  &sample_offset);

	sound_id = sound_id_arg;
	target_time = 0;
	target_volume = normal_volume = base_volume = 127;
	play_timestamp = access_timestamp = AIL_ms_count();
	loop_count = 1;
	loops_made = 0;
	prologic_mode = FALSE;
	streaming_mode = FALSE;
	surround_active = FALSE;

	sample_buffer = new AudioBuffer(file, sample_offset, sample_size);

	if ((mode & SOUND_STREAMABLE) && (sample_size < 64 * 1024))
	{
		mode ^= SOUND_STREAMABLE;
	}

	if ((mode & SOUND_PRO_LOGIC) && (channels != 1))
	{
		mode ^= SOUND_PRO_LOGIC;
	}

	if ((mode & SOUND_STREAMABLE) && (mode & SOUND_PRO_LOGIC))
	{
		mode ^= SOUND_PRO_LOGIC;
	}
	
	if (mode & SOUND_STREAMABLE)
	{
		streaming_mode = TRUE;
	}
	else
	{
		sample_buffer->fetch(0);

		AIL_set_sample_address(hsample1, sample_buffer->data(),
			                              sample_buffer->size());
	}

	//
	// Create a second set of channels if ProLogic encoding desired,
	// but only if the source data is monophonic
	//

	if (mode & SOUND_PRO_LOGIC)
	{
		prologic_mode = TRUE;

		AIL_init_sample(hsample2);
		//
		// Go ahead and set up the sample normally...
		// then invert the data later.
		//

		AUDMGR_set_sample_file(hsample2, file, 0);

		void* inverted = (void*)malloc(sample_size);

		if (bits_per_sample == 16)
		{
			U32 size_in_words = (sample_size + 1) / 2;

			S16* src = (S16*)sample_buffer->data();
			S16* dst = (S16*)inverted;

			while (size_in_words--)
			{
				*dst++ = -*src++;
			}
		}
		else
		{
			U32 size_in_bytes = sample_size;

			S8* src = (S8*)sample_buffer->data();
			S8* dst = (S8*)inverted;

			while (size_in_bytes--)
			{
				*dst++ = 255 - *src++;
			}
		}

		AIL_set_sample_address(hsample2, inverted, sample_size);

		//
		// The inverted data is only driven through the right track,
		// this will never change so go ahead and set it up here
		//

		AIL_set_sample_pan(hsample2, 127);
	}

	//
	// Register a callback to mark the handle as being free when playing is
	// complete. No need to worry about S2, it is dependent on S1's actions.
	//

   AIL_register_EOS_callback(hsample1, play_done_callback);
}

//****************************************************************************
//*                                                                          *
//* release()                                                                *
//*                                                                          *
//****************************************************************************

void AudioSample::release()
{
	set_timestamp(play_timestamp);
	stop_ramp();
	stop_stream_updates();

	if (sample_buffer)
    	delete sample_buffer;
}

//****************************************************************************
//*                                                                          *
//* play()                                                                   *
//*                                                                          *
//****************************************************************************

void AudioSample::play(S32 ramp_milliseconds)
{
	start_ramp(ramp_milliseconds, 0, normal_volume);

	if (streaming_mode)
	{
		stop_stream_updates();
		
		AIL_end_sample(hsample1);
		AIL_set_sample_position(hsample1, 0);

		sample_buffer->fetch(0);

		AIL_load_sample_buffer(hsample1, 0,
			                    sample_buffer->data(),
			                    sample_buffer->size());

		sample_buffer->fetch();

		AIL_load_sample_buffer(hsample1, 1,
									  sample_buffer->data(),
									  sample_buffer->size());
		start_stream_updates();
	}
	else
	{
		if (prologic_mode)
		{
			// When using waveout, make sure that samples start together
			// or we'll end up with phase problems

			AIL_lock();
			AIL_start_sample(hsample1);
			AIL_start_sample(hsample2);
			AIL_unlock();
		}
		else
		{
			AIL_start_sample(hsample1);
		}
	}
	play_timestamp = AIL_ms_count();

	set_timestamp(play_timestamp);

	loops_made = 0;
}

//****************************************************************************
//*                                                                          *
//* stop()                                                                   *
//*                                                                          *
//****************************************************************************

void AudioSample::stop(S32 ramp_milliseconds)
{
	//
	// The actual AIL_stop_sample will occur in the ramping code
	// when the sound level has dropped to zero
	//

	start_ramp(ramp_milliseconds, AIL_sample_volume(hsample1), 0);

	//
	// Samples that are stopping are good candidates for being trashed...
	// make this timestamp older than any more recently played.
	//

	set_timestamp(play_timestamp);
}

//****************************************************************************
//*                                                                          *
//* resume()                                                                 *
//*                                                                          *
//****************************************************************************

void AudioSample::resume(S32 ramp_milliseconds)
{
	start_ramp(ramp_milliseconds, 0, normal_volume);

	if (streaming_mode)
	{
		stop_stream_updates();

		update_stream();

		start_stream_updates();
	}

	if (prologic_mode)
	{
		AIL_lock();
		AIL_resume_sample(hsample1);
		AIL_resume_sample(hsample2);
		AIL_unlock();
	}
	else
	{
		AIL_resume_sample(hsample1);
	}
	play_timestamp = AIL_ms_count();
	set_timestamp(play_timestamp);
}

//****************************************************************************
//*                                                                          *
//* get_status()                                                             *
//*                                                                          *
//****************************************************************************

U32 AudioSample::get_status()
{
	U32 result;

	switch (AIL_sample_status(hsample1))
	{
	case SMP_PLAYING: result = SOUND_PLAYING; break;
	case SMP_STOPPED: result = SOUND_STOPPED; break;
	default:          result = SOUND_DONE;    break;
	}
	return result;
}

//****************************************************************************
//*                                                                          *
//* set_volume()                                                             *
//*                                                                          *
//****************************************************************************

void AudioSample::set_volume(SINGLE volume, // In decibel reduction
                             U32    ramp_milliseconds)
{
	// Set the base volume to the given value
	if (volume >= 0.0f)
	{
		base_volume = 127;
	}
	else if (volume < -96.0f) 
	{
		base_volume = 0;
	}
	else
	{
		//base_volume = 1 + U32(126.0f * (SINGLE)pow(10.0, volume/20.0f));

		// Convert the linear 0 to -10000 range to a 127 to 0 range.
		// f(0.0) == 127, f(-96.0) == 0
		// Using y = mx+b formula, m = 127/(0 - -96.0) = 127/96 and b is 127.
		base_volume = (U32) ((127.0 / 96.0) * volume + 127.0);
	}

	// Start a new ramp to the normal volume, which will take the base volume into account.
	start_ramp(ramp_milliseconds, AIL_sample_volume(hsample1), normal_volume);
}

//****************************************************************************
//*                                                                          *
//* set_sound_bearing()                                                      *
//*                                                                          *
//****************************************************************************

void AudioSample::set_sound_bearing(const Vector& bearing,
                                    SINGLE        falloff_3db)
{
	//
	// Assumes a bearing of 0,0,-1 to be straight ahead.
	//

	SINGLE falloff_factor = bearing.magnitude() / falloff_3db;

	SINGLE falloff = -3.0f * falloff_factor * falloff_factor;

	SINGLE distance_factor = (SINGLE)pow(10.0, falloff/20.0f);

	const DOUBLE offset = 30.0 / 180.0 * 3.14159265358979323846;

	Vector left_speaker_dir(-(SINGLE)sin(offset), 0.0, -(SINGLE)cos(offset));
	Vector right_speaker_dir((SINGLE)sin(offset), 0.0, -(SINGLE)cos(offset));

	Vector bearing_2D(bearing.x, 0.0, -(SINGLE)fabs(bearing.z));
	bearing_2D.normalize();

	SINGLE lvol = SINGLE(dot_product(left_speaker_dir,  bearing_2D));
	SINGLE rvol = SINGLE(dot_product(right_speaker_dir, bearing_2D));

	SINGLE max_vol = lvol > rvol ? lvol : rvol;

//	normal_volume = 1 + U32(126.0f * distance_factor);
	normal_volume = (U32) ((127.0 / 96.0) * falloff + 127.0);

	if (prologic_mode) // Prologic
	{
		if (bearing.z > 0.0) // Behind us?
		{
			surround_active = TRUE;

			AIL_lock();

			AIL_set_sample_pan(hsample1, 0);
			start_ramp(0, normal_volume, normal_volume);

			AIL_unlock();
		}
		else // In front
		{
			surround_active = FALSE;

			AIL_lock();

			AIL_set_sample_pan(hsample1, U32(64 - (lvol * 64) + (rvol * 63)));
			start_ramp(0, normal_volume, normal_volume);

			AIL_unlock();
		}
	}
	else
	{
		AIL_set_sample_pan(hsample1, U32(64 - (lvol * 64) + (rvol * 63)));
		start_ramp(0, normal_volume, normal_volume);
	}
}

//****************************************************************************
//*                                                                          *
//* set_relative_rate()                                                      *
//*                                                                          *
//****************************************************************************

void AudioSample::set_relative_rate (SINGLE rate_percent)
{
	U32 sampleRate = U32(initial_sample_rate * rate_percent / 100.0f);

	if (prologic_mode)
	{
		// Make sure they change rate together so they don't go out
		// of phase

		AIL_lock();
		AIL_set_sample_playback_rate(hsample1, sampleRate);
		AIL_set_sample_playback_rate(hsample2, sampleRate);
		AIL_unlock();
	}
	else
	{
		AIL_set_sample_playback_rate(hsample1, sampleRate);
	}
}

//****************************************************************************
//*                                                                          *
//* set_loop_count()                                                         *
//*                                                                          *
//****************************************************************************


void AudioSample::set_loop_count(U32 loop_count_arg)
{
	loop_count = loop_count_arg;

	if (!streaming_mode)
	{
		if (prologic_mode)
		{
			AIL_lock();
			AIL_set_sample_loop_count(hsample1, loop_count);
			AIL_set_sample_loop_count(hsample2, loop_count);
			AIL_unlock();
		}
		else
		{
			AIL_set_sample_loop_count(hsample1, loop_count);
		}
	}
	set_timestamp(AIL_ms_count());
}

//****************************************************************************
//*                                                                          *
//* get_loop_count()                                                         *
//*                                                                          *
//****************************************************************************


U32 AudioSample::get_loop_count()
{
	return loop_count;
}

//****************************************************************************
//*                                                                          *
//* start_ramp()                                                               *
//*                                                                          *
//****************************************************************************

void AudioSample::start_ramp(U32 ramp_milliseconds,
							        U32 initial_volume,
									  U32 target_volume_arg)
{
	// A target_volume of 0 means STOP when ramp is complete

	target_volume = target_volume_arg;

	if (ramp_milliseconds == 0) // No ramp... do it now
	{
		target_time = 0;

		initial_volume = target_volume;

		if (target_volume == 0) // If stopping immediately
		{
			stop_stream_updates();

			if (prologic_mode)
			{
				AIL_lock();
				AIL_stop_sample(hsample1);
				AIL_stop_sample(hsample2);
				AIL_unlock();
			}
			else
			{
				AIL_stop_sample(hsample1);
			}
			set_timestamp(play_timestamp);
		}
	}
	else
	{
		target_time = AIL_ms_count() + ramp_milliseconds;

		// Update the static list of samples that are currently ramping

		for (U32 i = 0; i < ramping_count; i++)
		{
			if (ramping_samples[i] == this)
			{
				break;
			}
		}

		// If not already in the list...

		if (i == ramping_count)
		{
			ramping_samples[ramping_count++] = this;
		}
	}

	// Use the base_volume value to scale the initial_volume value before sending it to AIL.
	U32 scaled_volume = apply_base_volume (initial_volume);

	// Apply the initial volume
	if (prologic_mode)
	{
		AIL_lock();
		AIL_set_sample_volume(hsample1, scaled_volume);
		AIL_set_sample_volume(hsample2, surround_active ? scaled_volume : 0);
		AIL_unlock();
	}
	else
	{
		AIL_set_sample_volume(hsample1, scaled_volume);
	}
}

//****************************************************************************
//*                                                                          *
//* stop_ramp()                                                              *
//*                                                                          *
//****************************************************************************

void AudioSample::stop_ramp()
{
	// Remove this from the static list of samples that are ramping

	for (U32 i = 0; i < ramping_count; i++)
	{
		if (ramping_samples[i] == this)
		{
			ramping_count--;

			for (; i < ramping_count; i++)
			{
				ramping_samples[i] = ramping_samples[i+1];
			}
			break;
		}
	}
}

//****************************************************************************
//*                                                                          *
//* update_ramp()																			     *
//*                                                                          *
//****************************************************************************

void AudioSample::update_ramp(U32 current_time)
{
	U32 current_volume = AIL_sample_volume(hsample1);

	U32 time_remaining = target_time - current_time;
	U32 scaled_target_volume = apply_base_volume (target_volume);
	U32 volume_remaining = scaled_target_volume - current_volume;
					           
	if (time_remaining <= 0 || volume_remaining == 0)
	{
		target_time = 0;

		if (prologic_mode)
		{
			AIL_lock();
			AIL_set_sample_volume(hsample1, scaled_target_volume);
			AIL_set_sample_volume(hsample2, surround_active ? scaled_target_volume : 0);
			AIL_unlock();
		}
		else
		{
			AIL_set_sample_volume(hsample1, scaled_target_volume);
		}

		if (scaled_target_volume == 0)
		{
			stop_stream_updates();

			if (prologic_mode)
			{
				AIL_lock();
				AIL_stop_sample(hsample1);
				AIL_stop_sample(hsample2);
				AIL_unlock();
			}
			else
			{
				AIL_stop_sample(hsample1);
			}
		}
		stop_ramp();
	}
	else
	{
		// Add 1 to make sure there is no divide-by-zero...

		U32 slices_remaining = time_remaining / (1000 / RAMP_FREQUENCY) + 1;
		U32 volume_step = volume_remaining / slices_remaining;

		if (volume_step == 0)
		{
			volume_step = (volume_remaining > 0) ? 1 : -1;
		}

		// NOTE: We in post-scaled volumes here
		current_volume += volume_step;

		if (prologic_mode)
		{
			AIL_lock();
			AIL_set_sample_volume(hsample1, current_volume);
			AIL_set_sample_volume(hsample2, surround_active ? current_volume : 0);
			AIL_unlock();
		}
		else
		{
			AIL_set_sample_volume(hsample1, current_volume);
		}
	}
}

//****************************************************************************
//*                                                                          *
//* start_stream_updates()																			     *
//*                                                                          *
//****************************************************************************

void AudioSample::start_stream_updates()
{
	for (U32 i = 0; i < streaming_count; i++)
	{
		if (streaming_samples[i] == this)
		{
			break;
		}
	}
	if (i == streaming_count)
	{
		streaming_samples[streaming_count++] = this;
	}
}

//****************************************************************************
//*                                                                          *
//* stop_stream()																			     *
//*                                                                          *
//****************************************************************************

void AudioSample::stop_stream_updates()
{
	if (streaming_mode)
	{
		for (U32 i = 0; i < streaming_count; i++)
		{
			if (streaming_samples[i] == this)
			{
				streaming_count--;
				for (;i < streaming_count; i++)
				{
					streaming_samples[i] = streaming_samples[i+1];
				}
				break;
			}
		}
	}
}

//****************************************************************************
//*                                                                          *
//* update_stream()                                                          *
//*                                                                          *
//****************************************************************************

void AudioSample::update_stream()
{
	U32 buffer_index = AIL_sample_buffer_ready(hsample1);
	if (buffer_index == -1) return;

	if (sample_buffer->at_end()) 
	{
		loops_made++;

		if (loop_count != 0 && loops_made >= loop_count)
		{
			stop_stream_updates();
			return;
		}
	}

	AIL_load_sample_buffer(hsample1, buffer_index,
						  sample_buffer->data(), sample_buffer->size()); 
}

//****************************************************************************
//*                                                                          *
//* timer_callback()                                                         *
//*                                                                          *
//****************************************************************************

void AudioSample::timer_callback(U32)
{
	U32 current_time = (U32)AIL_ms_count();
	U32 i;

	for (i = 0; i < ramping_count; i++)
	{
		ramping_samples[i]->update_ramp(current_time);
	}

	for (i = 0; i < streaming_count; i++)
	{
		streaming_samples[i]->update_stream();
	}
}

//****************************************************************************
//*                                                                          *
//* play_done_callback()                                                     *
//*                                                                          *
//****************************************************************************

void AILCALLBACK AudioSample::play_done_callback(HSAMPLE hsample)
{
	AudioSample* sample = lookup(hsample);

	if (sample) sample->stop_ramp();
}

//****************************************************************************
//*                                                                          *
//* set_timestamp()		                                                       *
//*                                                                          *
//****************************************************************************

void AudioSample::set_timestamp(U32 time)
{
	if (time > 1)
	{
		//
		// Increase the value of infinitely looping samples
		//
		if (AIL_sample_loop_count(hsample1) == 0)
		{
			time |= 0x80000000;
		}
	}
	access_timestamp = time;
}

//****************************************************************************
//*                                                                          *
//* set_timestamp()		                                                       *
//*                                                                          *
//****************************************************************************

U32 AudioSample::apply_base_volume (U32 vol)
{
	// Adjust the given, full deflection value to be relative to the base volume.
	// This is a ratio thing:
	// scale_vol / base_vol = vol / full_vol
	// full_vol = 127;

	U32 result = vol * base_volume / 127;
	return result;
}

//****************************************************************************
//*                                                                          *
//* lookup()																					  *
//*                                                                          *
//****************************************************************************

AudioSample* AudioSample::lookup(HSAMPLE hsample)
{
	return (AudioSample*)AIL_sample_user_data(hsample, AUDIO_SAMPLE);
}

//****************************************************************************
//*                                                                          *
//* lookup()                                                                 *
//*                                                                          *
//****************************************************************************

AudioSample* AudioSample::lookup(SOUND_ID sound_id)
{
	AudioSample* result = NULL;

	if (sound_id != INVALID_SOUND_ID)
	{
		for (U32 i=0; i < max_samples; i++)
		{
			if (samples[i]->sound_id == sound_id)
			{
				result = samples[i];
				break;
			}
		}
	}

	return result;
}
