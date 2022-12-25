#ifndef __CHANNEL_H
#define __CHANNEL_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>	//argh!!!
#include <assert.h>
#include <malloc.h>
#include "FileSys.h"
#include "Engine.h"
#include "ChannelTypes.h"
#include "IChannel.h"


namespace Channel
{
	extern IEngine* Engine;

	typedef float Time;
	typedef U32 Offset;

	int const ANIMATION_COMPLETE = 1;

	class Object;

	class Archetype
	{
		protected:

			IChannel::Header header;

			void* data;

			//including time value for non-periodic data, in bytes
			size_t frame_size;

			//in seconds
			float duration;

		public:

			mutable int ref_count;

			static bool Create (IFileSystem* fs, Archetype& result);

			unsigned int get_data_type (void) const;

			float get_duration (void) const;

			unsigned int get_num_frames (void) const;

			const void* get_frame_data (unsigned int i) const;
			
			float get_frame_time (int i) const;

			const void* get_frame (unsigned int i) const;

			const void* get_event_data (void) const;

			bool legal_frame_index_p (int i) const;

			Object* create_instance (unsigned int _id) const;

			void free (void);
	};

	inline unsigned int Archetype::get_data_type (void) const
	{
		return header.type;
	}

	inline float Archetype::get_duration (void) const
	{
		return duration;
	}

	inline unsigned int Archetype::get_num_frames (void) const
	{
		return header.frames;
	}

	inline const void* Archetype::get_frame_data (unsigned int i) const
	{
		assert (legal_frame_index_p (i));

		return (const char*)data + ((frame_size * i) + ((header.capture_rate < 0.0) ? sizeof (Time) : 0));
	}

	inline const void* Archetype::get_frame (unsigned int i) const
	{
		assert (legal_frame_index_p (i));

		return (const char*)data + (frame_size * i);
	}

	inline bool Archetype::legal_frame_index_p (int i) const
	{
		return (i < (int)get_num_frames () && i >= 0);
	}

	inline float Archetype::get_frame_time (int i) const
	{
		assert (legal_frame_index_p (i));

//		emaurer:  when the above assert is removed and speed
//			optimizations are enabled, the following code doesn't work.
//			use the rewrite below.

//		return (header.capture_rate < 0.0) ?
//			*(Time*)((const char*)data + (frame_size * i)) :
//			(i * header.capture_rate);

		float result;

		if (header.capture_rate < 0.0)
			result = *(Time*)((const char*)data + (frame_size * i));
		else
			result = (i * header.capture_rate);

		return result;
	}

	inline const void* Archetype::get_event_data (void) const
	{
		if(data)
		{
			return (const char*)data + (frame_size * header.frames);
		}
		else
		{
			return NULL;
		}
	}

// TODO: 
// - store current keyframe pair & ratio, which should always be valid from the
//   time the channel is started.
// - always store whether the last result is valid & what time it was computed for
//   easy lookup of current value.
// - extend get_keyframe_pair() to compute a value at any given time, and make use
//   of the current known time and keyframes to reduce search if possible.
// - combine weight & translation scale into one quantity which gets recomputed
//   as necessary.

	 
	class Object
	{
		protected:

			const Archetype * archetype;
			IVirtualChannel * ctrl;

			//the object that this instance is controlling
			Target target;

			//in the case that the target is a callback fn, 
			//'user_data' is passed to the callback
			void * user_data;

			//current time in the animation.  when playing backwards
			//this value is decreasing
			float playback_head;

		public:
			unsigned int flags;

		protected:

			//value added to the current frame index in order to get to
			//the next sequential frame.  may be negative when playing backwards
			int advance;

			//index of the last key frame in the animation sequence
			//'playback_head' is between the time value associated with 'last_frame'
			//index and ('last_frame' + 'advance') index.  used with an event stream
			//to determine when to call the event handler.
			int last_frame;

			//used to control playback rate.  when negative, playback is backwards.
			float time_scale;

			//used in blending. Defaults to 1.0.
			float weight;
			float translation_scale;

			U32 suspended:1;
			U32 finished:1;
			U32 updated:1;

			void init(void);

			//used to compare frame times.  result depends on is_backwards ().
			bool time_less_than (float t0, float t1) const;
			bool time_not_less_than (float t0, float t1) const;

			bool time_greater_than (float t0, float t1) const;
			bool time_not_greater_than (float t0, float t1) const;

			//used to determine if playback is forward or backwards.
			bool is_backwards (void) const;

			//used to retrieve the data to pass to the event handler
			const void* get_event_data (Offset o) const;

		// interpolates keyframes, calls off to ENGINE or whomever to set the ABSOLUTE target data.
			virtual void interpolate(const void * base, const void * next, double ratio) const;

		// interpolates keyframes, return just the RELATIVE offsets in "dst".
			virtual void interpolate_rel(void * dst, const void * base, const void * next, double ratio) const;

		// interpolates keyframes, returns the ABSOLUTE data in "dst".
			virtual void interpolate(void * dst, const void * base, const void * next, double ratio) const;

			bool legal_start_parms_p (StartParms* parms);
			virtual void start_without_error_check (StartParms* parms);

		// Callback all events within the time interval specified.
			void deal_with_events(float t0, float t1);

		public:

			virtual void interpolate_and_store(const void* base, const void* next, double ratio);
			virtual void blend(void * dst, const void * base, const void * next, double ratio);

			const unsigned int id;

			Object (const Archetype* _archetype, unsigned int _id);
			Object (IVirtualChannel * _ctrl, unsigned int _id);

			float get_current_time (void) const;
			virtual bool set_current_time (float nu_t);

			void replace_callback_data (IEventHandler * event_handler_replacement, void* user_data_replacement);

			virtual bool start (StartParms*);

			void get_start_parms (StartParms* parms) const;

			int update (float dt);
			virtual void get_last_result(void * dst) {} 

			void set_weight(float);
			float get_weight(void) const;

			const Target & get_target(void) const
			{
				return target;
			}

			void suspend(void);

			bool is_suspended(void) const
			{
				return (suspended == 1);
			}
			void finish(void)
			{
				finished = 1;
			}
			bool is_finished(void) const
			{
				return (finished == 1);
			}

			virtual void change_position(const Vector & p) {}
			virtual void change_orientation(const Matrix & R) {}
			virtual void adjust_start_position(const Vector & dp) {}

			void clear_update_flag(void)
			{
				updated = 0;
			}
			bool was_updated(void) const
			{
				return updated;
			}

			virtual void * get_result_ptr(void)
			{
				return NULL;
			}

			virtual void get_current_offset(float * abs, float * rel) {}

			virtual void start_blend(float duration) {} 
			virtual Vector get_translation(float dt) const 
			{
				return Vector(0, 0, 0);
			}

			void get_keyframe_pair(int & f0, int & f1, float time) const;

			bool get_data_at_time(float time, void * data) const;
	};

	inline void Object::replace_callback_data (IEventHandler * event_handler_replacement, void* user_data_replacement)
	{
		if (target.type == EVENT)
		{
			target.event_handler = event_handler_replacement;
			user_data = user_data_replacement;
		}
	}

	inline bool Object::is_backwards (void) const
	{
		return (time_scale < 0.0);
	}

	inline bool Object::time_less_than (float t0, float t1) const
	{
		return (is_backwards ()) ? (t0 > t1) : (t0 < t1);
	}

	inline bool Object::time_not_less_than (float t0, float t1) const
	{
		return (is_backwards ()) ? (t0 <= t1) : (t0 >= t1);
	}

	inline bool Object::time_greater_than (float t0, float t1) const
	{
		return (is_backwards ()) ? (t0 < t1) : (t0 > t1);
	}

	inline bool Object::time_not_greater_than (float t0, float t1) const
	{
		return (is_backwards ()) ? (t0 >= t1) : (t0 <= t1);
	}

	inline float Object::get_current_time (void) const
	{
		return playback_head;
	}

	inline const void* Object::get_event_data (Offset o) const
	{
		const void* event_data = archetype->get_event_data ();

		if(event_data)
		{
			return (const char*)(event_data) + o;
		}
		else
		{
			return NULL;
		}
	}

	inline void Object::set_weight(float w)
	{
		weight = w;
	}

	inline float Object::get_weight(void) const
	{
		return weight;
	}

	class FloatStream : public Object
	{
		protected:
			
			float last_result;

			JointType target_joint_type;

			virtual void interpolate (const void* base, 
										const void* next, 
										double ratio) const;

			virtual void interpolate(void * dst, const void * base, const void * next, double ratio) const;

			virtual void interpolate_and_store(const void * base, const void * next, double ratio);
			virtual void blend(void * dst, const void * base, const void * next, double ratio);

		public:

			FloatStream (const Archetype* a, unsigned int _id) : Object (a, _id), target_joint_type (JT_NONE)
			{
			}

			FloatStream (IVirtualChannel * c, unsigned int _id) : Object (c, _id), target_joint_type (JT_NONE)
			{
			}

			virtual bool start (StartParms*);
			virtual void get_last_result(void * dst)
			{
				*((float *) dst) = last_result;
			}

			virtual void change_position(const Vector & p) {}
			virtual void change_orientation(const Matrix & R) {}

			virtual void * get_result_ptr(void)
			{
				return &last_result;
			}
	};

	class QuatStream : public Object
	{
		protected:
			
			Quaternion last_result;

			virtual void interpolate (const void* base, 
										const void* next, 
										double ratio) const;

			virtual void interpolate(void * dst, const void * base, const void * next, double ratio) const;

			virtual void interpolate_and_store(const void * base, const void * next, double ratio);
			virtual void blend(void * dst, const void * base, const void * next, double ratio);

		public:

			QuatStream (const Archetype* a, unsigned int _id) : Object (a, _id)
			{
			}

			QuatStream (IVirtualChannel * c, unsigned int _id) : Object (c, _id)
			{
			}

			virtual void get_last_result(void * dst)
			{
				*((Quaternion *) dst) = last_result;
			}

			virtual void change_position(const Vector & p) {}
			virtual void change_orientation(const Matrix & R) {}

			virtual void * get_result_ptr(void)
			{
				return &last_result;
			}
	};

	class VectorStream : public Object
	{
		protected:
			
			Vector last_result;

			virtual void interpolate (const void* base, 
										const void* next, 
										double ratio) const;

			virtual void interpolate(void * dst, const void * base, const void * next, double ratio) const;

			virtual void interpolate_and_store(const void * base, const void * next, double ratio);
			virtual void blend(void * dst, const void * base, const void * next, double ratio);

		public:

			VectorStream (const Archetype* a, unsigned int _id) : Object (a, _id)
			{
			}

			VectorStream (IVirtualChannel * c, unsigned int _id) : Object (c, _id)
			{
			}

			virtual void get_last_result(void * dst)
			{
				*((Vector *) dst) = last_result;
			}

			virtual void change_position(const Vector & p) {}
			virtual void change_orientation(const Matrix & R) {}

			virtual void * get_result_ptr(void)
			{
				return &last_result;
			}
	};
		
	class FullStream : public Object
	{
		protected:
			
			virtual void interpolate (const void* base, 

										const void* next, 
										double ratio) const;

			virtual void interpolate(void * dst, const void * base, const void * next, double ratio) const;

			virtual void interpolate_rel(void * dst, const void * base, const void * next, double ratio) const;

			virtual void interpolate_and_store(const void * base, const void * next, double ratio);
			virtual void blend(void * dst, const void * base, const void * next, double ratio);

			virtual void start_without_error_check (StartParms* parms);

			Matrix start_ornt;
			Vector start_pos;

			Vector blend_velocity;

		public:

			Vector last_result_v;
			Quaternion last_result_q;
			FullStream (const Archetype* a, unsigned int _id) : Object (a, _id)
			{
			}

			FullStream (IVirtualChannel * c, unsigned int _id) : Object (c, _id)
			{
			}

			virtual bool start (StartParms*);

			virtual void get_last_result(void * dst)
			{
				Vector * v = (Vector *) dst;
				*v = last_result_v;
				Quaternion * q = (Quaternion *) (v+1);
				*q = last_result_q;
			}

			virtual void change_position(const Vector & p);
			virtual void change_orientation(const Matrix & R);

			virtual void * get_result_ptr(void)
			{
				return &last_result_v;
			}

			virtual void get_current_offset(float * abs, float * rel);

			virtual void adjust_start_position(const Vector & dp);

			virtual void start_blend(float duration);

			virtual Vector get_translation(float dt) const;

	};
};

#endif