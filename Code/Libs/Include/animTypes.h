//$Header: /Conquest/Libs/Include/AnimTypes.h 5     4/28/00 11:57p Rmarr $

#ifndef __ANIMTYPES_H
#define __ANIMTYPES_H

#include "eventiterator.h"
#include "engine.h"

class Transform;

namespace Animation
{
	//may be used as the 'time' parameter to IAnimation::script_start ()
	const int BEGIN = 0;
	const int END = -1;
	const int CUR = -2;

	//may be used as the 'flags' parameter to IAnimation::script_start ()
	const unsigned int FORWARD		= 0x00;
	const unsigned int BACKWARDS	= 0x01;
	const unsigned int LOOP			= 0x02;	//may be combined with FORWARD or BACKWARDS
	const unsigned int OSCILLATE	= 0x04;	//may be combined with FORWARD or BACKWARDS

	// the following 3 flags are mutually exclusive, but may be combined with any 
	// valid combination of the flags above.
	const unsigned int XLAT_BLEND_X	= 0x08;
	const unsigned int XLAT_BLEND_Y = 0x10;
	const unsigned int XLAT_BLEND_Z = 0x20;

	// When starting a script at a time other than 0, the default behavior
	// is to offset the translation so the root doesn't move discontinuously.
	// If you want the translation to jump ahead to the start time instead,
	// use the following flag:
	const unsigned int NO_XLAT_OFFSET = 0x40;

	// Prevents destruction of the source animation in a blend.  The dest
	// motion in the blend will transition back to the source upon completion.
	const unsigned int OVERLAY		= 0x80;

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
		//virtual void COMAPI on_event(unsigned int channel_id, void * user_supplied, const EventIterator & event_iterator) = 0;
		virtual void COMAPI on_event(unsigned int channel_id, void * user_supplied, const EventIterator&) = 0;

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

		//in the case of a channel being an event stream.  This user pointer is
		//passed to the callback function as well as the channel id and channel data.
		void* user_data;

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
			user_data = NULL;
		}

		bool operator == (const Target & t)
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

		float	translation_scale;
		const class Transform * xform;

		StartParms (void)
		{
			start_time = 0.0;
			flags = 0;
			time_scale = 1.0;	
			transition_duration = 0.0;
			weight = 1.0;
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