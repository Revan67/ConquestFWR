#ifndef __CHANNEL_H
#define __CHANNEL_H

#include "Engine.h"
#include "IChannel.h"
#include "IChannel2.h"
#include "TSmartPointer.h"

using namespace Animation;

namespace Channel
{
	int const ANIMATION_COMPLETE = 1;

	const unsigned int INCL_T0 = 1;
	const unsigned int INCL_T1 = 2;

	class Object
	{
		protected:

			IVirtualChannel * ctrl;

			//the object that this instance is controlling
			Target target;

			//current time in the animation.  when playing backwards
			//this value is decreasing
			float playback_head;

		public:
			unsigned int flags;

			COMPTR<IChannel2> archetype;
		protected:

			//used to control playback rate.  when negative, playback is backwards.
			float time_scale;

			//used in blending. Defaults to 1.0.
			float weight;
			float translation_scale;

			U32 suspended:1;
			U32 finished:1;
			U32 updated:1;

			void init(void);

		// interpolates keyframes, calls off to ENGINE or whomever to set the ABSOLUTE target data.
			virtual void interpolate(const void * base, const void * next, double ratio) const;

		// interpolates keyframes, return just the RELATIVE offsets in "dst".
			virtual void interpolate_rel(void * dst, const void * base, const void * next, double ratio) const;

		// interpolates keyframes, returns the ABSOLUTE data in "dst".
			virtual void interpolate(void * dst, const void * base, const void * next, double ratio) const;

			bool legal_start_parms_p (const StartParms& parms);
			virtual void start_without_error_check (const StartParms& parms);

		// collect all events within the time interval specified.
			int deal_with_events(float t0, float t1, unsigned int incl, EventIterator::Event events[], unsigned int buflen, U32& cnt);

		public:

			bool active:1;

			virtual void interpolate_and_store(const void* base, const void* next, double ratio);
			virtual void blend(void * dst, const void * base, const void * next, double ratio);

			Object (IChannel2* arch);
			Object (IVirtualChannel * _ctrl);

			float get_current_time (void) const;
			virtual bool set_current_time (float nu_t);

			virtual U32 get_data_type (void) const;

			bool start (const StartParms&);

			int update (float dt);
			virtual void get_last_result(void * dst) {} 

			void set_weight(float);
			float get_weight(void) const;

			const Target & get_target(void) const
			{
				return target;
			}

			virtual bool set_target (const Target&);

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

			float get_duration (void) const
			{
				SINGLE ret;
				GENRESULT gr = archetype->get_duration (ret);
				ASSERT (gr == GR_OK);

				return ret;
			}

			//used to determine if playback is forward or backwards.
			bool is_backwards (void) const;
	};

	inline bool Object::is_backwards (void) const
	{
		return (time_scale < 0.0);
	}

	inline float Object::get_current_time (void) const
	{
		return playback_head;
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

			FloatStream (IChannel2* a) : Object (a), target_joint_type (JT_NONE)
			{
			}

			FloatStream (IVirtualChannel * c) : Object (c), target_joint_type (JT_NONE)
			{
			}

			virtual void get_last_result(void * dst)
			{
				*((float *) dst) = last_result;
			}

			virtual void change_position(const Vector & p) {}
			virtual void change_orientation(const Matrix & R) {}

			virtual bool set_target (const Target&);

			virtual void * get_result_ptr(void)
			{
				return &last_result;
			}
			virtual U32 get_data_type (void) const;
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

			QuatStream (IChannel2* a) : Object (a)
			{
			}

			QuatStream (IVirtualChannel * c) : Object (c)
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
			virtual U32 get_data_type (void) const;
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

			VectorStream (IChannel2* a) : Object (a)
			{
			}

			VectorStream (IVirtualChannel * c) : Object (c)
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
			virtual U32 get_data_type (void) const;
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

			virtual void start_without_error_check (const StartParms& parms);

			Matrix start_ornt;
			Vector start_pos;

			Vector blend_velocity;

		public:

			Vector last_result_v;
			Quaternion last_result_q;
			FullStream (IChannel2* a) : Object (a)
			{
			}

			FullStream (IVirtualChannel * c) : Object (c)
			{
			}

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
			virtual U32 get_data_type (void) const;
	};
};

#endif