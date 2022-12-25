struct Joint;

#include <stdlib.h>
#include "Channel.h"
#include "3DMath.h"
#include "PersistChannel.h"
#include "ChannelEventTypes.h"
#include "AnimTypes.h"
#include "fdump.h"

void DebugPrint (char *fmt, ...);

namespace Channel
{
	IEngine* Engine = NULL;
};

using namespace Channel;

static size_t GetFrameSize (U32 t)
{
	size_t result = 0;

	if (DT_FLOAT & t)
	{
		result += sizeof (SINGLE);
	}
	if (DT_VECTOR & t)
	{
		result += sizeof (Vector);
	}
	if (DT_QUATERNION & t)
	{
		result += sizeof (Quaternion);
	}
	if (DT_EVENT & t)
	{
		result = sizeof (Offset);
	}

	return result;
}

//#pragma warning( push, 4 )
//#pragma optimize( "", on )
bool Archetype::Create (IFileSystem* fs, Archetype& result)
{
	bool success = false;

	if (fs)
	{
		DWORD bytes_read;

		PersistChannelHeader hdr;
		
		DAFILEDESC desc ("Header");
		desc.lpImplementation = "DOS";
	
		HANDLE hndl;
		hndl = fs->OpenChild (&desc);

		ASSERT (hndl != INVALID_HANDLE_VALUE);

		BOOL r = fs->ReadFile (hndl, &hdr, sizeof (hdr), &bytes_read);

		fs->CloseHandle (hndl);

		if (r && (bytes_read == sizeof (hdr)))
		{
			result.header.frames = hdr.frames;
			result.header.capture_rate = hdr.capture_rate;
			result.header.type = hdr.type;

			result.frame_size = GetFrameSize (result.header.type);

			//if the capture rate is less than 0.0 then the data is not periodic.
			//in that case each frame consists of a time value of type float and
			//the data.
			if (result.header.capture_rate < 0.0)
			{
				result.frame_size += sizeof (Time);
			}

			desc.lpFileName = "Frames";

			hndl = fs->OpenChild (&desc);
			ASSERT (INVALID_HANDLE_VALUE != hndl);

			unsigned int malloc_size = fs->GetFileSize (hndl);

#ifndef NDEBUG
			//sanity check on the data size
			if (result.header.type != DT_EVENT)
			{
				ASSERT (malloc_size == result.frame_size * result.header.frames);
			}
#endif

			result.data = malloc (malloc_size);
			ASSERT (result.data);

			r = fs->ReadFile (hndl, result.data, malloc_size, &bytes_read);

			fs->CloseHandle (hndl);

			if (r && (bytes_read == malloc_size))
			{
				if ( result.header.capture_rate < 0.0f ) // using 0.0 causes VS 6.0 to emit bad code -ms
				{
					result.duration = result.get_frame_time (result.get_num_frames () - 1);
				}
				else
				{
					result.duration = (result.get_num_frames () - 1) * result.header.capture_rate;
				}

				success = true;
			}
		}
	}
	else
	{	// Special case with blank event channel
		PersistChannelHeader hdr;
		
		hdr.frames = 0;
		hdr.capture_rate = -1;
		hdr.type = PersistDT_EVENT;

		result.header.frames = hdr.frames;
		result.header.capture_rate = hdr.capture_rate;
		result.header.type = hdr.type;

		result.frame_size = GetFrameSize (result.header.type);

		//if the capture rate is less than 0.0 then the data is not periodic.
		//in that case each frame consists of a time value of type float and
		//the data.
		if (result.header.capture_rate < 0.0)
		{
			result.frame_size += sizeof (Time);
		}


		unsigned int malloc_size = 0;

		result.data = NULL;

		result.duration = 0;

		success = true;
	}

	if (success)
		result.ref_count = 1;

	return success;
}
//#pragma optimize( "", on )
//#pragma warning( pop )

Object* Archetype::create_instance (unsigned int _id) const
{
	Object* result = NULL;

	switch (get_data_type ())
	{
		case DT_FLOAT:
			result = new FloatStream (this, _id);
			break;
		case DT_VECTOR:
			result = new VectorStream (this, _id);
			break;
		case DT_QUATERNION:
			result = new QuatStream (this, _id);
			break;
		case DT_QUATERNION | DT_VECTOR:
			result = new FullStream (this, _id);
			break;
		case DT_EVENT:
			result = new Object (this, _id);
			break;
	}

	return result;	
}

void Archetype::free (void)
{
	if (data)
	{
		::free (data);
		data = NULL;
	}
}

void Object::init(void)
{
	playback_head = 0.0;
	flags = 0;
	time_scale = 1.0;
	user_data = NULL;
	weight = 1.0;
	suspended = false;
}

//

Object::Object (const Archetype* _archetype, unsigned int _id) : archetype(_archetype), id(_id), ctrl(NULL)
{
	init();
	ASSERT(archetype);
}

//

Object::Object (IVirtualChannel * _ctrl, unsigned int _id) : ctrl(_ctrl), id(_id), archetype(NULL)
{
	init();
	ASSERT(ctrl);
}

//

bool Object::start (StartParms* parms)
{
	bool result = legal_start_parms_p (parms);

	if (result)
		start_without_error_check (parms);

	return result;
}

void Object::start_without_error_check (StartParms* parms)
{
	ASSERT(parms);

	flags = parms->flags;
	target = parms->target;
	time_scale = parms->time_scale;
	weight = parms->weight;
	user_data = parms->user_data;
	translation_scale = parms->translation_scale;

	playback_head = parms->start_time;

	if (archetype)
	{
		if (is_backwards ())
		{
			advance = -1;
			last_frame = archetype->get_num_frames ();
		}
		else
		{
			advance = 1;
			last_frame = -1;
		}

		float ft = archetype->get_frame_time (last_frame + advance);

	//move last_frame logically forward until the time that it corresponds to 
	//is logically ahead of the playback_head

		while (time_less_than(ft, playback_head))
		{
			last_frame += advance;

			if (archetype->legal_frame_index_p(last_frame + advance))
			{
				ft = archetype->get_frame_time (last_frame + advance);
			}
			else
			{
				break;
			}
		}
	}

	suspended = finished = 0;
}

bool Object::legal_start_parms_p (StartParms* parms)
{
	ASSERT(parms);

	bool result;

	if (ctrl)
	{
		result = true;
	}
	else
	{
		result = false;
		//verify that the outputs of this animation are appropriate for the
		//object that is being controlled.

		unsigned int data_type = archetype->get_data_type ();

		if (parms->target.type == JOINT && Engine)
		{
			S32 data_size = 0;
			const JointInfo *ji;

			if( (ji = Engine->get_joint_info( parms->target.joint )) != NULL ) {
				data_size = ji->get_state_vector_size();
			}

			result = (data_size * sizeof (SINGLE) == GetFrameSize (data_type));
		}
		else if (parms->target.type == EVENT)
			result = (data_type == DT_EVENT);
		else
			result = (GetFrameSize (data_type) == (sizeof (Quaternion) + sizeof (Vector)));

		//is the specified starting time between the beginning and ending points of the animation?

		if (result)
			result = parms->start_time <= archetype->get_duration () && parms->start_time >= 0.0f;
	}

	return result;
}

void Object::get_start_parms (StartParms* parms) const
{
	ASSERT(parms);

	parms->flags = flags;
	parms->target = target;
	parms->time_scale = time_scale;
	parms->weight = weight;
	parms->user_data = user_data;
	parms->start_time = playback_head;
	parms->translation_scale = translation_scale;
}

//

void Object::get_keyframe_pair(int & f0, int & f1, float time) const
{
// Does a full search every time. last_frame causes much confusion.
	int frame = (is_backwards()) ? archetype->get_num_frames() - 1 : 0;
	frame += advance;

	bool past_end = !archetype->legal_frame_index_p(frame);
	if (!past_end)
	{
		float ft = archetype->get_frame_time(frame);

		while (time_less_than(ft, time))
		{
			frame += advance;
			if (archetype->legal_frame_index_p(frame))
			{
				ft = archetype->get_frame_time(frame);
			}
			else
			{
				past_end = true;
				break;
			}
		}
	}

	if (past_end)
	{
		f0 = f1 = frame - advance;
	}
	else
	{
		f0 = frame - advance;
		f1 = frame;
	}
}

//

bool Object::set_current_time (float t)
{
	bool result = true;

	if (t < 0)
	{
		result = false;
	}
	else
	{
		playback_head = t;
	}

	return result;
}

//

void Object::suspend(void)
{
	suspended = 1;

	if (archetype && archetype->get_data_type() != DT_EVENT)
	{
	// Be sure current stored result is correct.
		int next, base;
		double ratio;
		if (finished)
		{
			next = base = (is_backwards ()) ? 0 : (archetype->get_num_frames () - 1);
			ratio = 0;
		}		
		else
		{
			base = last_frame;
			next = last_frame + advance;

			float t0 = archetype->get_frame_time(base);
			float t1 = archetype->get_frame_time(next);
			
			ratio = (playback_head - t0) / (t1 - t0);
		}

		const void* baseval = archetype->get_frame_data(base);
		const void* nextval = archetype->get_frame_data(next);

		interpolate_and_store(baseval, nextval, ratio);
	}
}

//

void Object::deal_with_events(float t0, float t1)
{
// Use last frame, or next valid frame if last_frame is out of bounds.
	int start_frame = __min((int) archetype->get_num_frames() - 1, __max(0, last_frame));
	int end_frame = start_frame;

	float ft = archetype->get_frame_time(start_frame);
	while (time_not_less_than(ft, t0) && time_not_greater_than(ft, t1))
	{
	// We've found another valid frame.
		end_frame += advance;

	// If there is a subsequent frame, check its time. Otherwise, we're done.
		if (is_backwards())
		{
			if (end_frame >= 0)
			{
				ft = archetype->get_frame_time(end_frame);
			}
			else
			{
				ft = -1;
			}
		}
		else
		{
			if ((U32)end_frame < archetype->get_num_frames())
			{
				ft = archetype->get_frame_time(end_frame);
			}
			else
			{
				ft = -1;
			}
		}
	}

	int frame_count = end_frame - start_frame;
	if (is_backwards())
	{
		frame_count = -frame_count;
	}

	if (frame_count)
	{
		const void * frame_data = (is_backwards()) ? archetype->get_frame(end_frame+1) : archetype->get_frame(start_frame);
		const void * event_data = archetype->get_event_data();

		EventIterator event_iterator(frame_data, frame_count, event_data);

		if (is_backwards())
		{
			struct TimeOffset
			{
				float			time;
				unsigned int	offset;
			};
		// translate begin<->end.
			for (unsigned int i = 0; i < event_iterator.get_event_count(); i++)
			{
				unsigned int type = event_iterator.get_event_type(i);
				if (type == CHANNEL_BEGIN)
				{
					char * data = const_cast<char *>((const char *) event_data);
					const TimeOffset * offset = (const TimeOffset *) frame_data;
					*((unsigned int *) (data + offset[i].offset)) = CHANNEL_END;
				}
				else if (type == CHANNEL_END)
				{
					char * data = const_cast<char *>((const char *) event_data);
					const TimeOffset * offset = (const TimeOffset *) frame_data;
					*((unsigned int *) (data + offset[i].offset)) = CHANNEL_BEGIN;
				}
			}
		}

		target.event_handler->on_event(id, user_data, event_iterator);

		if (is_backwards())
		{
			struct TimeOffset
			{
				float			time;
				unsigned int	offset;
			};
		// translate back.
			for (unsigned int i = 0; i < event_iterator.get_event_count(); i++)
			{
				unsigned int type = event_iterator.get_event_type(i);
				if (type == CHANNEL_BEGIN)
				{
					char * data = const_cast<char *>((const char *) event_data);
					const TimeOffset * offset = (const TimeOffset *) frame_data;
					*((unsigned int *) (data + offset[i].offset)) = CHANNEL_END;
				}
				else if (type == CHANNEL_END)
				{
					char * data = const_cast<char *>((const char *) event_data);
					const TimeOffset * offset = (const TimeOffset *) frame_data;
					*((unsigned int *) (data + offset[i].offset)) = CHANNEL_BEGIN;
				}
			}
		}
	}

	last_frame = end_frame;
}

//

int Object::update (float dt)
{
	int result = 0;

	updated = 1;

	if (suspended || finished)
	{
		return result;
	}

// STORE Previous as well as current time.
	float t_prev = playback_head;

	//'advance' the playback head, time_scale < 0 indicates reverse playback.
	playback_head += dt * time_scale;

	float t_curr = playback_head;

	if (ctrl)
	{
		result = ctrl->update(get_result_ptr(), id, target, playback_head);
	}
	else
	{
	//the percentage to interpolate, from value at 'base' to 'next'
	double ratio;

	//frame indices to interpolate between
	int base;
	int next;

	float from_end = is_backwards () ? (playback_head - 0.0) : (archetype->get_duration () - playback_head);

	bool done = false;

	if (from_end <= 0.0)
	{

		if ((archetype->get_duration() != 0.0f) && (from_end < -archetype->get_duration()))
		{
			int times = abs(int(from_end / archetype->get_duration()));

			float remainder = fabs(from_end) - times * archetype->get_duration();

			from_end = -remainder;
		}


		ASSERT (!((flags & LOOP) && (flags & OSCILLATE)));

		if (flags & LOOP)
		{
		// Set last frame, call on_loop(), then restart.
			int frm = (is_backwards()) ? 0 : (archetype->get_num_frames() - 1);

			Transform T(false);

			if (archetype->get_data_type() == DT_EVENT)
			{
			// do any events:
				deal_with_events(t_prev, t_curr);
			}
			else if (target.type == OBJECT)
			{
				const void * val = archetype->get_frame_data(frm);

			// Instead of interpolating and setting position/orientation directly, compute
			// a transform which we pass to on_loop(), which modifies it as necessary.
			// We then pass it as a transform in StartParms as usual.
		
				interpolate_and_store(val, val, 0);
				//interpolate(val, val, 0);

				FullStream * fs = (FullStream *) this;
				T.set_position(fs->last_result_v);
				T.set_orientation(fs->last_result_q);

				if (target.event_handler)
				{
					target.event_handler->on_loop(id, T, user_data);
				}
			}

			StartParms parms;
			parms.target = target;
		// start at beginning (or end), then adjust playback head.
			parms.start_time = (is_backwards()) ? archetype->get_duration() : 0;
			parms.time_scale = time_scale;
			parms.weight = weight;
			parms.flags = flags;
			parms.user_data = user_data;
			parms.translation_scale = translation_scale;
			if (target.type == OBJECT)
			{
				parms.xform = &T;
			}

			start_without_error_check (&parms);

			playback_head = (is_backwards ()) ? (archetype->get_duration () + from_end) : (0.0 - from_end);

		// deal with backward motions correctly:
			if (archetype->get_data_type() == DT_EVENT)
			{
				if (is_backwards())
				{
					t_prev = archetype->get_duration();
				}
				else
				{
					t_prev = 0;
				}
				t_curr = playback_head;
			}
		}
		else if (flags & OSCILLATE)
		{
			StartParms parms;
			parms.target = target;
			parms.start_time = (is_backwards ()) ? (0.0 - from_end) : (archetype->get_duration () + from_end);
			parms.time_scale = -time_scale;
			parms.weight = weight;
			parms.flags = flags;
			parms.user_data = user_data;
			parms.translation_scale = translation_scale;

			start_without_error_check (&parms);
		}
		else
		{
			done = true;

			//set to exact end.  this is important.  if client asks to restart channel
			//from current time (perhaps reverse an animation), current time should be 
			//a legal value

			playback_head = (is_backwards ()) ? 0.0 : archetype->get_duration ();
		}
	}

	bool interesting_time = true;

	if (done)
	{
		finished = 1;

		ratio = 0.0;
		next = base = (is_backwards ()) ? 0 : (archetype->get_num_frames () - 1);

		if (!suspended)
		{
			result = ANIMATION_COMPLETE;
		}
	}
	else
	{
	// Events are treated a bit differently from normal data channels, which
	// only need to know which 2 keyframes we're currently between.
	// Events need to catch ALL keyframes since the last update.
	// Hence the different codepaths.

		if (DT_EVENT != archetype->get_data_type())
		{
			//last_frame is an optimization to prevent searching the data stream
			//every time for the current position.

			next = last_frame + advance;

			if (archetype->legal_frame_index_p(next))
			{
				float ft = archetype->get_frame_time(next);
		
				while (time_less_than(ft, playback_head))
				{
					next += advance;
					if (archetype->legal_frame_index_p(next))
					{
						ft = archetype->get_frame_time(next);
					}
					else
					{
						next = (is_backwards()) ? 0 : archetype->get_num_frames() - 1;
						break;
					}
				}

				base = next - advance;

				if (base < 0)
				{
					base = 0;
				}
				else if (base > (int)archetype->get_num_frames() - 1)
				{
					base = archetype->get_num_frames() - 1;
				}

			//is the time between two legal frames?
				if (archetype->legal_frame_index_p (base) && archetype->legal_frame_index_p (next))
				{
					if (base == next)
					{
						ratio = 1;
					}
					else
					{
						float t0 = archetype->get_frame_time(base);
						float t1 = archetype->get_frame_time(next);
			
						ratio = (playback_head - t0) / (t1 - t0);
					}
				}
				else
				{
					interesting_time = false;
				}
			}
			else
			{
				base = next = last_frame;
				ratio = 0;
			}
		}
	}

	if (interesting_time)
	{
		if (DT_EVENT == archetype->get_data_type() && target.event_handler)
		{
		// Should always be true?
			ASSERT(target.type == EVENT);

			deal_with_events(t_prev, t_curr);
		}
		else
		{
			const void* baseval = archetype->get_frame_data(base);
			const void* nextval = archetype->get_frame_data(next);

			interpolate_and_store(baseval, nextval, ratio);
			last_frame = base;
		}
	}

	}

	return result;
}

//

bool Object::get_data_at_time(float time, void * data) const
{
	bool result = false;
									
	time *= time_scale;

	int f0, f1;
	get_keyframe_pair(f0, f1, time);

	float ft0 = archetype->get_frame_time(f0);
	float ft1 = archetype->get_frame_time(f1);

	double ratio = (time - ft0) / (ft1 - ft0);

	const Vector * baseval = (const Vector *) archetype->get_frame_data(f0);
	const Vector * nextval = (const Vector *) archetype->get_frame_data(f1);

	interpolate(data, baseval, nextval, ratio);

	return result;
}

//

void Object::interpolate (const void* baseval, const void* nextval, double ratio) const
{
}

void Object::interpolate_rel(void * dst, const void* baseval, const void* nextval, double ratio) const
{
}

void Object::interpolate(void * dst, const void * baseval, const void * next, double ratio) const
{
}

void Object::interpolate_and_store(const void * baseval, const void * nextval, double ratio)
{
}

void Object::blend(void * dst, const void * base, const void * next, double ratio)
{
}

//

bool FloatStream::start (StartParms* parms)
{
	ASSERT(parms);

	bool result = legal_start_parms_p (parms);

	if (result)
	{
		const JointInfo *ji;
		if( (ji = Engine->get_joint_info( parms->target.joint )) != NULL ) {
			target_joint_type = ji->type;
			start_without_error_check (parms);
		}
	}

	return result;
}

void FloatStream::interpolate (const void* baseval, const void* nextval, double ratio) const
{
	ASSERT(baseval);
	ASSERT(nextval);
	ASSERT (Engine);
	ASSERT (target.type == JOINT);

	float r;

	if (JT_REVOLUTE == target_joint_type)
	{
		//if the joint is revolute, interpolate along the smaller of
		//the two possible arcs.

		float b = *(float*)baseval;
		float n = *(float*)nextval;

		ASSERT (n <= 2 * PI);

		if (n - b < -PI)
			n += 2 * PI;
		else if (n - b > PI)
			n -= 2 * PI;

		r = b + (n - b) * ratio;
	}
	else
		r = *(float*)baseval + ((*(float*)nextval - *(float*)baseval) * ratio);

	Engine->set_joint_state( target.joint, IE_JST_BASIC, &r );
}

void FloatStream::interpolate (void * dst, const void* baseval, const void* nextval, double ratio) const
{
	ASSERT(baseval);
	ASSERT(nextval);
	ASSERT (target.type == JOINT);

	float r;

	if (JT_REVOLUTE == target_joint_type)
	{
		//if the joint is revolute, interpolate along the smaller of
		//the two possible arcs.

		float b = *(float*)baseval;
		float n = *(float*)nextval;

		ASSERT (n <= 2 * PI);

		if (n - b < -PI)
			n += 2 * PI;
		else if (n - b > PI)
			n -= 2 * PI;

		r = b + (n - b) * ratio;
	}
	else
		r = *(float*)baseval + ((*(float*)nextval - *(float*)baseval) * ratio);

	memcpy(dst, &r, sizeof(float));
}


void FloatStream::interpolate_and_store(const void* baseval, const void* nextval, double ratio)
{
	ASSERT(baseval);
	ASSERT(nextval);
	ASSERT (Engine);
	ASSERT (target.type == JOINT);

	float r;

	if (JT_REVOLUTE == target_joint_type)
	{
		//if the joint is revolute, interpolate along the smaller of
		//the two possible arcs.

		float b = *(float*)baseval;
		float n = *(float*)nextval;

		ASSERT (n <= 2 * PI);

		if (n - b < -PI)
			n += 2 * PI;
		else if (n - b > PI)
			n -= 2 * PI;

		r = b + (n - b) * ratio;
	}
	else
		r = *(float*)baseval + ((*(float*)nextval - *(float*)baseval) * ratio);

	last_result = r * weight;
}

void FloatStream::blend(void * dst, const void * base, const void * next, double ratio)
{
	interpolate_and_store(base, next, ratio);
	get_last_result(dst);
}

void QuatStream::interpolate (const void* baseval, const void* nextval, double ratio) const
{
	Quaternion r (slerp (*(PersistQuaternion*)baseval, *(PersistQuaternion*)nextval, ratio));
	ASSERT (target.type == JOINT);

	Engine->set_joint_state( target.joint, IE_JST_BASIC, (float*)&r );
}

void QuatStream::interpolate (void * dst, const void* baseval, const void* nextval, double ratio) const
{
	Quaternion r (slerp (*(PersistQuaternion*)baseval, *(PersistQuaternion*)nextval, ratio));
	ASSERT (target.type == JOINT);

	memcpy(dst, &r, sizeof(Quaternion));
}


void QuatStream::interpolate_and_store(const void* baseval, const void* nextval, double ratio)
{
	Quaternion r (slerp (*(PersistQuaternion*)baseval, *(PersistQuaternion*)nextval, ratio));
	ASSERT (target.type == JOINT);

	if (weight == 1.0)
	{
		last_result = r;
	}
	else
	{
		Quaternion I; I.set_identity();
		last_result = slerp(I, r, weight);
	}
}

void QuatStream::blend(void * dst, const void * base, const void * next, double ratio)
{
	Quaternion save = last_result;
	interpolate_and_store(base, next, ratio);
	get_last_result(dst);
	last_result = save;
}

//

void FullStream::interpolate (const void* baseval, const void* nextval, double ratio) const
{
	ASSERT(baseval);
	ASSERT(nextval);

	Vector p (*(Vector*)baseval + ((*(Vector*)nextval - *(Vector*)baseval) * ratio));
	p *= weight * translation_scale;

	baseval = (char*)baseval + sizeof (Vector);
	nextval = (char*)nextval + sizeof (Vector);

	Quaternion o (slerp (*(Quaternion*)baseval, *(Quaternion*)nextval, ratio));

	if (weight != 1.0)
	{
		Quaternion I; I.set_identity();
		o = slerp(I, o, weight);
	}

	if (target.type == OBJECT)
	{
		Engine->set_position (target.object, (start_ornt * p) + start_pos);
		Engine->set_orientation (target.object, start_ornt * Matrix(o));
	}
	else if (target.type == JOINT)
	{
		SINGLE buf[7];
		*(Vector*)buf = p;
		*(Quaternion*)(buf + 3) = o;

		Engine->set_joint_state( target.joint, IE_JST_BASIC, buf );
	}
}

void FullStream::interpolate(void * dst, const void* baseval, const void* nextval, double ratio) const
{
	ASSERT(baseval);
	ASSERT(nextval);

	Vector p (*(Vector*)baseval + ((*(Vector*)nextval - *(Vector*)baseval) * ratio));
	p *= weight * translation_scale;

	baseval = (char*)baseval + sizeof (Vector);
	nextval = (char*)nextval + sizeof (Vector);

	Quaternion o (slerp (*(Quaternion*)baseval, *(Quaternion*)nextval, ratio));

	if (weight != 1.0)
	{
		Quaternion I; I.set_identity();
		o = slerp(I, o, weight);
	}

	if (target.type == OBJECT)
	{
		Vector * v = (Vector *) dst;
		*v = start_pos + start_ornt * p;

		Quaternion * q = (Quaternion *) (v+1);
		*q = Quaternion(start_ornt * Matrix(o));
	}
	else if (target.type == JOINT)
	{
		Vector * v = (Vector *) dst;
		*v = p;

		Quaternion * q = (Quaternion *) (v+1);
		*q = o;
	}
}


void FullStream::interpolate_rel(void * dst, const void* baseval, const void* nextval, double ratio) const
{
	ASSERT(baseval);
	ASSERT(nextval);

	Vector p (*(Vector*)baseval + ((*(Vector*)nextval - *(Vector*)baseval) * ratio));

	p *= weight * translation_scale;

	baseval = (char*)baseval + sizeof (Vector);
	nextval = (char*)nextval + sizeof (Vector);

	Quaternion o (slerp (*(Quaternion*)baseval, *(Quaternion*)nextval, ratio));

	if (weight != 1.0)
	{
		Quaternion I; I.set_identity();
		o = slerp(I, o, weight);
	}

	float * result = (float *) dst;
	Vector * vr = (Vector *) &result[0];
	Quaternion * qr = (Quaternion *) &result[3];

	*vr = p;
	*qr = o;
}


void FullStream::interpolate_and_store(const void* baseval, const void* nextval, double ratio)
{
	ASSERT(baseval);
	ASSERT(nextval);

	Vector p (*(Vector*)baseval + ((*(Vector*)nextval - *(Vector*)baseval) * ratio));

	p *= weight * translation_scale;

	baseval = (char*)baseval + sizeof (Vector);
	nextval = (char*)nextval + sizeof (Vector);

	Quaternion o (slerp (*(Quaternion*)baseval, *(Quaternion*)nextval, ratio));

	if (weight != 1.0)
	{
		Quaternion I; I.set_identity();
		o = slerp(I, o, weight);
	}

	if (target.type == OBJECT)
	{
		last_result_v = start_pos + start_ornt * p;
		last_result_q =  Quaternion(start_ornt) * o;
	}
	else if (target.type == JOINT)
	{
		last_result_v = p;
		last_result_q = o;
	}
}

//

Vector FullStream::get_translation(float dt) const
{
// compute amount of translation over duration:

	dt *= time_scale;	// correct?


	Vector total_xlat(0, 0, 0);

	float t0 = playback_head;
	float t1 = (is_backwards()) ? t0 - dt : t0 + dt;

	bool overrun;
	if (is_backwards())
	{
		overrun = t1 < 0;
	}
	else
	{
		overrun = t1 > archetype->get_duration();
	}

	Vector offset0, offset1;

	int f0, f1;
	get_keyframe_pair(f0, f1, t0);

	if (f0 == f1)
	{
		const void* baseval = archetype->get_frame_data(f0);
		offset0 = *((Vector *) baseval) * weight * translation_scale;
	}
	else
	{
		float ft0 = archetype->get_frame_time(f0);
		float ft1 = archetype->get_frame_time(f1);

		double ratio = (t0 - ft0) / (ft1 - ft0);

		const Vector * baseval = (const Vector *) archetype->get_frame_data(f0);
		const Vector * nextval = (const Vector *) archetype->get_frame_data(f1);

		offset0 = *baseval + (*nextval - *baseval) * ratio;
		offset0 *= weight * translation_scale;
	}

	if (overrun && (flags & LOOP))
	{
	// ASSUMES MOTION WON'T LOOP MULTIPLE TIMES DURING TRANSITION.
	// probably wrong.

		int last_frame = (is_backwards()) ? 0 : archetype->get_num_frames() - 1;
		const Vector * last_val = (const Vector *) archetype->get_frame_data(last_frame);
		{
			Vector intermediate0 = *last_val * weight * translation_scale;
			total_xlat = intermediate0 - offset0;

			int first_frame = (is_backwards()) ? archetype->get_num_frames() - 1 : 0;
			const Vector * first_val = (const Vector *) archetype->get_frame_data(first_frame);

			Vector intermediate1 = *first_val * weight * translation_scale;
			if (is_backwards())
			{
				t1 = archetype->get_duration() - t1;
			}
			else
			{
				t1 -= archetype->get_duration();
			}

			get_keyframe_pair(f0, f1, t1);
			if (f0 == f1)
			{
				const void* baseval = archetype->get_frame_data(f0);
				offset1 = *((Vector *) baseval) * weight * translation_scale;
			}
			else
			{
				float ft0 = archetype->get_frame_time(f0);
				float ft1 = archetype->get_frame_time(f1);

				double ratio = (t1 - ft0) / (ft1 - ft0);

				const Vector * baseval = (const Vector *) archetype->get_frame_data(f0);
				const Vector * nextval = (const Vector *) archetype->get_frame_data(f1);

				offset1 = *baseval + (*nextval - *baseval) * ratio;
				offset1 *= weight * translation_scale;
			}

			total_xlat += offset1 - intermediate1;
		}
	}
	else
	{
		get_keyframe_pair(f0, f1, t1);

		if (f0 == f1)
		{
			const void* baseval = archetype->get_frame_data(f0);
			offset1 = *((Vector *) baseval);
			offset1 *= weight * translation_scale;
		}
		else
		{
			float ft0 = archetype->get_frame_time(f0);
			float ft1 = archetype->get_frame_time(f1);

			double ratio = (t1 - ft0) / (ft1 - ft0);

			const void* baseval = archetype->get_frame_data(f0);
			const void* nextval = archetype->get_frame_data(f1);

			offset1 = *(Vector*)baseval + ((*(Vector*)nextval - *(Vector*)baseval) * ratio);
			offset1 *= weight * translation_scale;
		}

		total_xlat = offset1 - offset0;
	}

	Vector result = start_ornt * total_xlat;
	return result;
}

//

void FullStream::start_blend(float duration)
{
// compute velocity based on amount of translation over duration:
	float t0 = playback_head;
	float t1 = (is_backwards()) ? t0 - duration : t0 + duration;

	Vector p0, p1;

	int f0, f1;
	get_keyframe_pair(f0, f1, t0);

	if (f0 == f1)
	{
		const void* baseval = archetype->get_frame_data(f0);
		Vector p(*((Vector *) baseval));
		p *= weight * translation_scale;
		p0 = start_pos + start_ornt * p;
	}
	else
	{
		float ft0 = archetype->get_frame_time(f0);
		float ft1 = archetype->get_frame_time(f1);

		double ratio = (t0 - ft0) / (ft1 - ft0);

		const void* baseval = archetype->get_frame_data(f0);
		const void* nextval = archetype->get_frame_data(f1);

		Vector p (*(Vector*)baseval + ((*(Vector*)nextval - *(Vector*)baseval) * ratio));
		p *= weight * translation_scale;
		p0 = start_pos + start_ornt * p;
	}

	get_keyframe_pair(f0, f1, t1);

	if (f0 == f1)
	{
		const void* baseval = archetype->get_frame_data(f0);
		Vector p(*((Vector *) baseval));
		p *= weight * translation_scale;
		p1 = start_pos + start_ornt * p;
	}
	else
	{
		float ft0 = archetype->get_frame_time(f0);
		float ft1 = archetype->get_frame_time(f1);

		double ratio = (t1 - ft0) / (ft1 - ft0);

		const void* baseval = archetype->get_frame_data(f0);
		const void* nextval = archetype->get_frame_data(f1);

		Vector p (*(Vector*)baseval + ((*(Vector*)nextval - *(Vector*)baseval) * ratio));
		p *= weight * translation_scale;
		p1 = start_pos + start_ornt * p;
	}

	blend_velocity = (p1 - p0) / duration;
}

//

void FullStream::blend(void * dst, const void * base, const void * next, double ratio)
{
	ASSERT(dst);
	ASSERT(base);
	ASSERT(next);

	Vector * vdst = (Vector *) dst;
	Quaternion * qdst = (Quaternion *) (vdst+1);

	Vector * v0 = (Vector *) base;
	Vector * v1 = (Vector *) next;

	*vdst = *v0 + (*v1 - *v0) * ratio;

	Quaternion * q0 = (Quaternion *) (v0+1);
	Quaternion * q1 = (Quaternion *) (v1+1);

	*qdst = slerp(*q0, *q1, ratio);
}

//

void FullStream::get_current_offset(float * abs, float * rel)
{
	ASSERT(abs);
	ASSERT(rel);

// Find data at current time...
	int next, base;
	double ratio;
	if (finished)
	{
		next = base = (is_backwards ()) ? 0 : (archetype->get_num_frames () - 1);
		ratio = 0;
	}		
	else
	{
		get_keyframe_pair(base, next, playback_head);

		if (base == next)
		{
			ratio = 0;
		}
		else
		{
			float t0 = archetype->get_frame_time(base);
			float t1 = archetype->get_frame_time(next);
		
			ratio = (playback_head - t0) / (t1 - t0);
		}
	}

	const void* baseval = archetype->get_frame_data(base);
	const void* nextval = archetype->get_frame_data(next);

	float u[7];
	interpolate_rel(u, baseval, nextval, ratio);

	Vector * offset = (Vector *) u;
	Vector * vrel = (Vector *) rel;
	*vrel = *offset;

	Vector * vabs = (Vector *) abs;
	*vabs = start_pos + start_ornt * *vrel;
}

//

void FullStream::adjust_start_position(const Vector & dp)
{
	start_pos += dp;
	if (suspended)
	{
	// Recompute last data based on new start position:
		suspend();
	}
}

//

void FullStream::start_without_error_check(StartParms * parms)
{
	ASSERT(parms);

	Object::start_without_error_check(parms);

	if (parms->target.type == OBJECT)
	{
		if ((parms->start_time == 0.0f) || (parms->flags & Channel::NO_XLAT_OFFSET))
		{ 
		// used for root animation
			if (parms->xform)
			{
				start_ornt = parms->xform->get_orientation();
				start_pos = parms->xform->get_position();
			}
			else
			{
				start_ornt = Engine->get_orientation(parms->target.object);
				start_pos = Engine->get_position(parms->target.object);
			}
		}
		else
		{
		// Need to deal with translation if starting somewhere other than beginning of motion.
			
		// Get relative position and orientation at start time:

			Vector poffset;
			Quaternion qoffset;

			float time = parms->start_time;	// TIME SCALE?
			int base;
			int next = (is_backwards()) ? archetype->get_num_frames() - 1 : 0;

			if (archetype->legal_frame_index_p(next))
			{
				float ft = archetype->get_frame_time(next);
		
				while (time_less_than(ft, time))
				{
					next += advance;
					if (archetype->legal_frame_index_p(next))
					{
						ft = archetype->get_frame_time(next);
					}
					else
					{
						next = (is_backwards()) ? 0 : archetype->get_num_frames() - 1;
						break;
					}
				}

				base = next - advance;

				double ratio = 0;

			//is the time between two legal frames?
				if (archetype->legal_frame_index_p(base) && archetype->legal_frame_index_p(next))
				{
					float t0 = archetype->get_frame_time(base);
					float t1 = archetype->get_frame_time(next);
			
					ratio = (time - t0) / (t1 - t0);
				}
				else
				{
					next = base = archetype->get_num_frames () - 1;
					ratio = 0;
				}

				const void* baseval = archetype->get_frame_data(base);
				const void* nextval = archetype->get_frame_data(next);

				float data[7];
				interpolate_rel(data, baseval, nextval, ratio);
				poffset = *((Vector *) data);
				qoffset = *((Quaternion *) (data+3));
			}

			if (parms->xform)
			{
			// THIS SHOULD PROBABLY BE CONFIGURABLE? ???
			//
			// USING transform that someone has given us, so assume it's a heading matrix.
			// In which case we don't want to adjust by final frame's orientation.

				start_ornt = parms->xform->get_orientation();
				start_pos = parms->xform->get_position() - start_ornt * poffset;
			}
			else
			{
			// AND THIS???
				start_ornt = Engine->get_orientation(parms->target.object) * Matrix(qoffset).get_transpose();
				start_pos = Engine->get_position(parms->target.object) - start_ornt * poffset;
			}
		}
	}
	else
	{
		start_ornt.set_identity();
		start_pos.zero();
	}

//DebugPrint("start_without_error_check (%X). new start_pos: %f, %f, %f\n", this, start_pos.x, start_pos.y, start_pos.z);
}

//

bool FullStream::start (StartParms* parms)
{
	bool result = legal_start_parms_p (parms);

	if (result)
	{
		start_without_error_check (parms);
	}

	return result;
}

//

void FullStream::change_position(const Vector & new_pos)
{
// Change start position.
	if (target.type == OBJECT)
	{
		Vector offset;

	// instead of using the actual position here, we should be using the 
	// theoretical position based on this motion's parameters only.
		int f0, f1;
		get_keyframe_pair(f0, f1, playback_head);

		if (f0 == f1)
		{
			const void* baseval = archetype->get_frame_data(f0);
			Vector p(*((Vector *) baseval));
			offset = p * weight * translation_scale;
			//pos = start_pos + start_ornt * p;
		}
		else
		{
			float ft0 = archetype->get_frame_time(f0);
			float ft1 = archetype->get_frame_time(f1);

			double ratio = (playback_head - ft0) / (ft1 - ft0);

			const void* baseval = archetype->get_frame_data(f0);
			const void* nextval = archetype->get_frame_data(f1);

			Vector p (*(Vector*)baseval + ((*(Vector*)nextval - *(Vector*)baseval) * ratio));
			offset = p * weight * translation_scale;
			//pos = start_pos + start_ornt * p;
		}

		//Vector pos = Engine->get_position(target.object);
		//Vector offset = start_ornt.get_transpose() * (pos - start_pos);
		start_pos = new_pos - start_ornt * offset;

//DebugPrint("change_position (%X). new start_pos: %f, %f, %f\n", this, start_pos.x, start_pos.y, start_pos.z);
		last_result_v = new_pos;
	}
}

//

void FullStream::change_orientation(const Matrix & R)
{
// Change start orientation.
	if (target.type == OBJECT)
	{
		int f0, f1;
		get_keyframe_pair(f0, f1, playback_head);

		double ratio;
		if (f0 == f1)
		{
			ratio = 0;
		}
		else
		{
			float ft0 = archetype->get_frame_time(f0);
			float ft1 = archetype->get_frame_time(f1);

			ratio = (playback_head - ft0) / (ft1 - ft0);
		}

		const void* baseval = archetype->get_frame_data(f0);
		const void* nextval = archetype->get_frame_data(f1);

		float f[7];
		interpolate_rel(f, baseval, nextval, ratio);

		Vector poffset = *((Vector *) &f[0]);
		Quaternion qoffset = *((Quaternion *) &f[3]);

	//
	// THIS CODE is complicated by the fact that the current root orientation may be the result
	// of a blend between multiple channels, as opposed to the result of this channel alone.
	//
	// Compute relative orientation as difference between current root orientation (maybe blended) and
	// desired root orientation.
	//
		Matrix Rrel = R * Engine->get_orientation(target.object).get_transpose();

	//
	// Apply this relative orientation to the channel's current start orientation.
	//
    	Matrix new_start_ornt = Rrel * start_ornt;

	//
	// Compute new start position to get it to its current position but with the 
	// new orientation. Again, this is based on the current channel only, not the
	// root's global position.
	//
		Vector pos = start_pos + start_ornt * poffset;
		Vector new_start_pos = pos - new_start_ornt * poffset;

		start_pos	= new_start_pos;
		start_ornt	= new_start_ornt;

	//
	// Store results based on new start position & orientation.
	//
		last_result_v = start_pos + start_ornt * poffset;
		last_result_q = Quaternion(new_start_ornt * Matrix(qoffset));
	}
}

//

void VectorStream::interpolate (const void* baseval, const void* nextval, double ratio) const
{
	ASSERT(baseval);
	ASSERT(nextval);

	Vector r (*(Vector*)baseval + ((*(Vector*)nextval - *(Vector*)baseval) * ratio));
	ASSERT (target.type == JOINT);

	r *= translation_scale;

	Engine->set_joint_state( target.joint, IE_JST_BASIC, (SINGLE*)&r );
}

void VectorStream::interpolate(void * dst, const void* baseval, const void* nextval, double ratio) const
{
	ASSERT(baseval);
	ASSERT(nextval);
	ASSERT(dst);

	Vector r (*(Vector*)baseval + ((*(Vector*)nextval - *(Vector*)baseval) * ratio));
	ASSERT (target.type == JOINT);

	r *= translation_scale;

	memcpy(dst, &r, sizeof(Vector));
}

void VectorStream::interpolate_and_store(const void* baseval, const void* nextval, double ratio)
{
	ASSERT(baseval);
	ASSERT(nextval);
	ASSERT (target.type == JOINT);
	last_result = (*(Vector*)baseval + ((*(Vector*)nextval - *(Vector*)baseval) * ratio));
	last_result *= weight * translation_scale;
}

void VectorStream::blend(void * dst, const void * base, const void * next, double ratio)
{
	ASSERT(base);
	ASSERT(next);
	ASSERT(dst);

	interpolate_and_store(base, next, ratio);
	get_last_result(dst);
}

//

