#ifndef __CHANNELTYPES_H
#define __CHANNELTYPES_H

#ifndef US_TYPEDEFS
#include "Typedefs.h"
#endif

#ifndef __EVENTITERATOR_H
#include "EventIterator.h"
#endif

#include "xform.h"

typedef S32 INSTANCE_INDEX;	// defined by Engine.h
typedef S32 JOINT_INDEX;	// defined by Engine.h

#include "fdump.h"


namespace Channel
{
	const unsigned int DT_FLOAT = 1;
	const unsigned int DT_VECTOR = 2;
	const unsigned int DT_QUATERNION = 4;
	const unsigned int DT_EVENT = 8;

	const unsigned int LOOP = 0x01;

	//like looping but once playback reaches an end 
	//it reverses and continues to play
	const unsigned int OSCILLATE = 0x02;

	// The following flags specify complex translation blending (not simple interpolation).
	// One coordinate (usually height) is linearly interpolated. You specify which one.
	const unsigned int XLAT_BLEND_X	= 0x04;
	const unsigned int XLAT_BLEND_Y = 0x08;
	const unsigned int XLAT_BLEND_Z = 0x10;

	// When starting a script at a time other than 0, the default behavior
	// is to offset the translation so the root doesn't move discontinuously.
	// If you want the translation to jump ahead to the start time instead,
	// use the following flag:
	const unsigned int NO_XLAT_OFFSET = 0x20;

	//target can be either an Engine object or a joint
	enum TargetType
	{
		JOINT,
		OBJECT,
		EVENT
	};

	// Use object-oriented callback method.
	struct IEventHandler
	{
		// on_event() is called for channel events.
		virtual void COMAPI on_event(unsigned int channel_id, void * user_supplied, const EventIterator & event_iterator) = 0;

		// THESE ARE NOT GENERALLY USEFUL TO THE APP. Used internally by the animation system.
		// Get called per-channel, so don't give any useful global script status information.
		// Recommended implementation: {}
		virtual void COMAPI on_finished(unsigned int channel_id, void * user_supplied) = 0;
		virtual void COMAPI on_loop(unsigned int channel_id, Transform & xform, void * user_supplied) = 0;
	};

	struct Target
	{
		TargetType type;

		// everybody gets an event handler now.
		IEventHandler *		event_handler;

		union
		{
			INSTANCE_INDEX object;
			JOINT_INDEX joint;
		};

		Target (void)
		{
			type = EVENT;
			object = -1;
			event_handler = 0;	//NULL
		}

		operator == (const Target & t)
		{
			bool result;
			if (type == t.type)
			{
				switch (type)
				{
				case JOINT:
					result = (joint == t.joint);
					break;
				case OBJECT:
					result = (object == t.object);
					break;
				case EVENT:
					result = (event_handler == t.event_handler);

					break;
				default:
					GENERAL_FATAL("Unknown data type!\n");
				}
			}
			else
			{
				result = false;
			}
			return result;
		}
	};

	struct StartParms
	{
		Target target;

		//in the case of a channel being an event stream.  This user pointer is
		//passed to the callback function as well as the channel id and channel data.
		void* user_data;

		//0.0 is the beginning of the animation
		float start_time;

		//bitwise and, 0 for normal playback
		unsigned int flags;

		//used to control playback rate, negative values cause backwards playback.
		//set to 1 for playback at captured rate. set to -1 for backwards playback 
		//at captured rate.
		float time_scale;

		float	transition_duration;

		float	weight;

	// If overlay is true, previous motion won't be deleted at end of
	// transition, just superceded for the duration of the overlay motion.
		bool	overlay;	

		float	translation_scale;
		const class Transform * xform;

		StartParms (void)
		{
			start_time = 0.0;
			flags = 0;
			time_scale = 1.0;	
			user_data = 0;	//NULL
			transition_duration = 0.0;
			weight = 1.0;
			overlay = false;
			translation_scale = 1.0;
			xform = 0;	//NULL
		}
	};

	struct IVirtualChannel
	{
		virtual int update(void * dst, U32 channel_idx, const Target & target, float time) = 0;
	};
};

#endif