#include "Channel.h"
#include "PersistChannel.h"

void DebugPrint (char *fmt, ...);

using namespace Channel;
extern IEngine* Engine;

void Object::init(void)
{
	playback_head = 0.0;
	flags = 0;
	time_scale = 1.0;
	weight = 1.0;
	suspended = false;
	finished = false;
	updated = false;
	translation_scale = 1.0;
}

//

Object::Object (IChannel2* _archetype) : archetype(_archetype), ctrl(NULL)
{
	init();
	ASSERT(archetype);
}

//

Object::Object (IVirtualChannel * _ctrl) : ctrl(_ctrl), archetype(NULL)
{
	init();
	ASSERT(ctrl);
}

//

U32 Object::get_data_type (void) const
{
	return DT_EVENT;
}

bool Object::start (const StartParms& parms)
{
	bool result = legal_start_parms_p (parms);

	if (result)
		start_without_error_check (parms);

	return result;
}

bool Object::set_target (const Target& tgt)
{
	target = tgt;

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

		U32 data_type;
		GENRESULT gr = archetype->get_data_type (data_type);
		ASSERT (gr == GR_OK);

		if (target.type == JOINT && Engine)
		{
			S32 data_size = 0;
			const JointInfo *ji;

			if( (ji = Engine->get_joint_info( target.joint )) != NULL ) {
				data_size = ji->get_state_vector_size();
			}

			result = (data_size * sizeof (SINGLE) == GetFrameSize (data_type));
		}
		else if (target.type == EVENT)
			result = (data_type == DT_EVENT);
		else
			result = (GetFrameSize (data_type) == (sizeof (Quaternion) + sizeof (Vector)));
	}

	return result;
}

void Object::start_without_error_check (const StartParms& parms)
{
	flags = parms.flags;
	time_scale = parms.time_scale;
	weight = parms.weight;
	translation_scale = parms.translation_scale;

	set_current_time (parms.start_time);

	suspended = finished = 0;
}

bool Object::legal_start_parms_p (const StartParms& parms)
{
	//is the specified starting time between the beginning and ending points of the animation?

	if (parms.start_time != Animation::END &&
		parms.start_time != Animation::BEGIN &&
		parms.start_time != Animation::CUR)
	{
		SINGLE dur;
		GENRESULT gr = archetype->get_duration (dur);
		ASSERT (gr == GR_OK);

		return parms.start_time <= dur && parms.start_time >= 0.0f;
	}
	else
		return true;
}

//

bool Object::set_current_time (float t)
{
	bool result = true;

	if (t == Animation::END)
		playback_head = get_duration ();
	else if (t == Animation::BEGIN)
		playback_head = 0.0;
	else if (t == Animation::CUR)
		playback_head = playback_head;
	else if (t < 0.0f)
		result = false;
	else
		playback_head = t;

	return result;
}

//

void Object::suspend(void)
{
	suspended = 1;

	if (archetype)
	{
		U32 data_type;
		GENRESULT gr = archetype->get_data_type (data_type);
		ASSERT (gr == GR_OK);

		if (data_type != DT_EVENT)
		{
		// Be sure current stored result is correct.
			float ratio;

			const void* baseval;
			const void* nextval;

			float time = playback_head;

			if (finished)
			{
				if (is_backwards ())
					time = 0;
				else
				{
					SINGLE dur;
					archetype->get_duration (dur);
					ASSERT (gr == GR_OK);
					
					time = dur;
				}
			}		

			GENRESULT gr = archetype->get_data_pair (time, baseval, nextval, ratio);
			ASSERT (gr == GR_OK);

			interpolate_and_store(baseval, nextval, ratio);
		}
	}
}

//

int Object::deal_with_events(float t0, float t1, unsigned int incl, EventIterator::Event events[], unsigned int buflen, U32& cnt)
{
	//EMAURER does the end of the tape have to be dealt with

	int result = 0;

	ASSERT ((is_backwards ()) ? (t0 >= t1) : (t0 <= t1));

	float dur;
	archetype->get_duration (dur);

	bool xition = (is_backwards ()) ? (t1 <= 0.0) : (t1 >= dur);

	if (xition)
	{
		float local_end;
		if (is_backwards())
		{
			local_end = 0.0f;
		}
		else
		{
			local_end = dur;
		}

		bool at_end = (t0 == local_end);

		U32 addnl = 0;

		//EMAURER should event reporting include the end times?
		if (incl & INCL_T0)
		{
			GENRESULT gr = archetype->get_events_at_time (t0, events, buflen, addnl);
			ASSERT (gr == GR_OK);

			cnt += addnl;
		}

		//EMAURER could be starting at end of tape and playing forwards
		if (!at_end)
		{
			GENRESULT gr = archetype->get_events (t0, local_end, events + cnt, buflen - cnt, addnl);
			ASSERT (gr == GR_OK);

			cnt += addnl;

			if (incl & INCL_T1)
			{
				GENRESULT gr = archetype->get_events_at_time (local_end, events + cnt, buflen - cnt, addnl);
				ASSERT (gr == GR_OK);

				cnt += addnl;
			}			
		}

		if (flags & LOOP)
		{
			addnl = 0;

			if (is_backwards ())
				deal_with_events (dur, dur + t1, INCL_T1|INCL_T0, events + cnt, buflen - cnt, addnl);
			else
				deal_with_events (0.0, t1 - dur, INCL_T0|INCL_T1, events + cnt, buflen - cnt, addnl);

			cnt += addnl;
		}
		else if (flags & OSCILLATE)
		{
			t0 = (is_backwards ()) ? 0.0 : dur;
			t1 = t0 - t1;

			//EMAURER reverse direction.
			time_scale = -time_scale;

			addnl = 0;
			deal_with_events (t0, t1, INCL_T1, events + cnt, buflen - cnt, addnl);
			cnt += addnl;
		}
		else
		{
			//EMAURER set to exact end.  this is important.  if client asks to restart channel
			//from current time (perhaps reverse an animation), current time should be 
			//a legal value
			playback_head = (is_backwards ()) ? 0.0 : dur;

			finished = 1;
			result = ANIMATION_COMPLETE;
		}
	}
	else
	{
		//EMAURER simplest case, no tape boundaries crossed.

		U32 addnl = 0;

		GENRESULT gr;

		if (incl & INCL_T0)
		{
			GENRESULT gr = archetype->get_events_at_time (t0, events, buflen, addnl);
			ASSERT (gr == GR_OK);

			cnt += addnl;
		}

		if (t0 != t1)
		{
			addnl = 0;

			gr = archetype->get_events (t0, t1, events + cnt, buflen - cnt, addnl);
			ASSERT (gr == GR_OK);

			cnt += addnl;

			if (incl & INCL_T1)
			{
				addnl = 0;

				gr = archetype->get_events_at_time (t1, events + cnt, buflen - cnt, addnl);
				ASSERT (gr == GR_OK);

				cnt += addnl;
			}
		}

		playback_head = t1;
	}

	return result;
}

//

int Object::update (float dt)
{
	ASSERT (!((flags & LOOP) && (flags & OSCILLATE)));

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

	if (ctrl)
	{
		result = ctrl->update(get_result_ptr(), (CHANNEL_INSTANCE_INDEX)this, target, playback_head);
	}
	else
	{
		U32 dtype;
		archetype->get_data_type (dtype);
		if (dtype == DT_EVENT)
		{
		// Events are treated a bit differently from normal data channels, which
		// only need to know which 2 keyframes we're currently between.
		// Events need to catch ALL keyframes since the last update.
		// Hence the different codepaths.
		
			EventIterator::Event events[EVENT_ITERATOR_MAX_EVENTS];

			//EMAURER have to run 'deal_with_events' even in case of no handler.
			//'deal...' manages playback head correctly and that should probably
			//be correct even in the absence of a handler.

			U32 cnt = 0;
			result = deal_with_events (t_prev, playback_head, INCL_T0|INCL_T1, events, EVENT_ITERATOR_MAX_EVENTS, cnt);

			if (cnt && target.event_handler)
			{
				EventIterator eit (events, cnt);
				target.event_handler->on_event ((CHANNEL_INSTANCE_INDEX)this, target.user_data, eit);
			}
		}
		else
		{
			SINGLE dur;
			archetype->get_duration (dur);

			float from_end = is_backwards () ? (playback_head - 0.0) : (dur - playback_head);

			//EMAURER has the head travelled beyond the end of the stream?
			//If so, behave differently based on the looping flags.
			if (from_end <= 0.0)
			{
				//EMAURER number of whole 'durations' elapsed
				unsigned int times = 0;
				
				if (dur)
					times = abs(int(from_end / dur));

				float remainder = fabs(from_end) - times * dur;

				if (flags & LOOP)
				{
					Transform T(false);

					if (target.type == OBJECT)
					{
					// Set last frame, call on_loop(), then restart.
						const void* val;

						GENRESULT gr = archetype->get_data_at_extreme (!is_backwards (), val);
						ASSERT (gr == GR_OK);

					// Instead of interpolating and setting position/orientation directly, compute
					// a transform which we pass to on_loop(), which modifies it as necessary.
					// We then pass it as a transform in StartParms as usual.
				
						interpolate_and_store(val, val, 0.0);
						//interpolate(val, val, 0);

						FullStream * fs = (FullStream *) this;
						T.set_position(fs->last_result_v);
						T.set_orientation(fs->last_result_q);

						if (target.event_handler)
						{
							target.event_handler->on_loop((CHANNEL_INSTANCE_INDEX)this, T, target.user_data);
						}
					}

					StartParms parms;
				// start at beginning (or end), then adjust playback head.
				//EMAURER why adjust playback_head later?
					parms.start_time = (is_backwards()) ? dur : 0;
					parms.time_scale = time_scale;
					parms.weight = weight;
					parms.flags = flags;
					parms.translation_scale = translation_scale;
					if (target.type == OBJECT)
					{
						parms.xform = &T;
					}

					start_without_error_check (parms);

					playback_head = (is_backwards ()) ? (dur - remainder) : remainder;
				}
				else if (flags & OSCILLATE)
				{
					StartParms parms;

					//does the elapse of dt cause to run past the end of the tape
					//an even number of times? implies new direction.
					parms.time_scale = (times & 1) ? time_scale : -time_scale;

					parms.start_time = (parms.time_scale < 0) ? (dur - remainder) : remainder;
					parms.weight = weight;
					parms.flags = flags;
					parms.translation_scale = translation_scale;

					start_without_error_check (parms);
				}
				else
				{
					//set to exact end.  this is important.  if client asks to restart channel
					//from current time (perhaps reverse an animation), current time should be 
					//a legal value

					playback_head = (is_backwards ()) ? 0.0 : dur;

					finished = 1;

					if (!suspended)
					{
						result = ANIMATION_COMPLETE;
					}
				}
			}
			//EMAURER no ending transition madness necessary.

#ifndef NDEBUG
			U32 dtype;
			archetype->get_data_type(dtype);
			ASSERT (dtype != DT_EVENT);
#endif

			const void* baseval;
			const void* nextval;

			SINGLE ratio;
			
			GENRESULT gr = archetype->get_data_pair (playback_head, baseval, nextval, ratio);
			ASSERT (gr == GR_OK);

			interpolate_and_store(baseval, nextval, ratio);
		}

	#ifndef NDEBUG
		SINGLE dur;
		archetype->get_duration (dur);
		ASSERT ((playback_head >= 0.0) && (playback_head <= dur));
	#endif
	}

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

bool FloatStream::set_target (const Target& tgt)
{
	bool result = Object::set_target (tgt);

	if (result)
	{
		const JointInfo *ji;

		if( (ji = Engine->get_joint_info( tgt.joint )) != NULL ) {
			target_joint_type = ji->type;
		}
	}

	return result;
}

U32 FloatStream::get_data_type (void) const
{
	return DT_FLOAT;
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

		r = InterpolateArc (*(float*)baseval, *(float*)nextval, ratio);
	}
	else
		r = Interpolate (*(float*)baseval, *(float*)nextval, ratio);

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
		r = InterpolateArc (*(float*)baseval, *(float*)nextval, ratio);
	}
	else
		r = Interpolate (*(float*)baseval, *(float*)nextval, ratio);

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
		r = InterpolateArc (*(float*)baseval, *(float*)nextval, ratio);
	}
	else
		r = Interpolate (*(float*)baseval, *(float*)nextval, ratio);

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

U32 QuatStream::get_data_type (void) const
{
	return DT_QUATERNION;
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

	float dur;
	archetype->get_duration(dur);

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
		overrun = t1 > dur;
	}

	Vector offset0, offset1;

	const void* baseval;
	const void* nextval;
	SINGLE ratio;

	GENRESULT gr = archetype->get_data_pair (t0, baseval, nextval, ratio);
	ASSERT (gr == GR_OK);

	offset0 = *(const Vector*)baseval + (*(const Vector*)nextval - *(const Vector*)baseval) * ratio;
	offset0 *= weight * translation_scale;

	if (overrun && (flags & LOOP))
	{
	// ASSUMES MOTION WON'T LOOP MULTIPLE TIMES DURING TRANSITION.
	// probably wrong.

		const void* data;

		GENRESULT gr = archetype->get_data_at_extreme (!is_backwards (), data);
		ASSERT (gr == GR_OK);

		const Vector * last_val = (const Vector *)data;
		{
			Vector intermediate0 = *last_val * weight * translation_scale;
			total_xlat = intermediate0 - offset0;

			GENRESULT gr = archetype->get_data_at_extreme (is_backwards (), data);
			ASSERT (gr == GR_OK);

			const Vector * first_val = (const Vector *)data;

			Vector intermediate1 = *first_val * weight * translation_scale;

			if (is_backwards())
			{
				t1 = dur - t1;
			}
			else
			{
				t1 -= dur;
			}

			gr = archetype->get_data_pair (t1, baseval, nextval, ratio);
			ASSERT (gr == GR_OK);

			offset1 = *(const Vector*)baseval + (*(const Vector*)nextval - *(const Vector*)baseval) * ratio;
			offset1 *= weight * translation_scale;

			total_xlat += offset1 - intermediate1;
		}
	}
	else
	{
		GENRESULT gr = archetype->get_data_pair (t1, baseval, nextval, ratio);

		if(gr == GR_OK)
		{
			offset1 = *(const Vector*)baseval + ((*(const Vector*)nextval - *(const Vector*)baseval) * ratio);
			offset1 *= weight * translation_scale;

			total_xlat = offset1 - offset0;
		}
		else
		{
			total_xlat.set(0,0,0);	//zero length anim or problems
		}
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

	const void* baseval;
	const void* nextval;

	SINGLE ratio;

	GENRESULT gr = archetype->get_data_pair (t0, baseval, nextval, ratio);
	ASSERT (gr == GR_OK);

	{
		Vector p (*(Vector*)baseval + ((*(Vector*)nextval - *(Vector*)baseval) * ratio));
		p *= weight * translation_scale;
		p0 = start_pos + start_ornt * p;
	}

	gr = archetype->get_data_pair (t1, baseval, nextval, ratio);
	ASSERT (gr == GR_OK);

	{
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

	const void* baseval;
	const void* nextval;

	SINGLE ratio;

	if (finished)
	{
		GENRESULT gr = archetype->get_data_at_extreme (!is_backwards (), baseval);
		ASSERT (gr == GR_OK);

		nextval = baseval;
		ratio = 0;
	}		
	else
	{
		GENRESULT gr = archetype->get_data_pair (playback_head, baseval, nextval, ratio);
		ASSERT (gr == GR_OK);
	}

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

void FullStream::start_without_error_check(const StartParms& parms)
{
	Object::start_without_error_check(parms);

	if (target.type == OBJECT)
	{
		if ((parms.start_time == 0.0f) || (parms.flags & NO_XLAT_OFFSET))
		{ 
		// used for root animation
			if (parms.xform)
			{
				start_ornt = parms.xform->get_orientation();
				start_pos = parms.xform->get_position();
			}
			else
			{
				start_ornt = Engine->get_orientation(target.object);
				start_pos = Engine->get_position(target.object);
			}
		}
		else
		{
		// Need to deal with translation if starting somewhere other than beginning of motion.
			
		// Get relative position and orientation at start time:

			Vector poffset;
			Quaternion qoffset;

			float time = playback_head;	// TIME SCALE?

			const void* baseval;
			const void* nextval;

			SINGLE ratio;

			GENRESULT gr = archetype->get_data_pair (time, baseval, nextval, ratio);
			ASSERT (gr == GR_OK);

			float data[7];
			interpolate_rel(data, baseval, nextval, ratio);
			poffset = *((Vector *) data);
			qoffset = *((Quaternion *) (data+3));

			if (parms.xform)
			{
			// THIS SHOULD PROBABLY BE CONFIGURABLE? ???
			//
			// USING transform that someone has given us, so assume it's a heading matrix.
			// In which case we don't want to adjust by final frame's orientation.

				start_ornt = parms.xform->get_orientation();
				start_pos = parms.xform->get_position() - start_ornt * poffset;
			}
			else
			{
			// AND THIS???
				start_ornt = Engine->get_orientation(target.object) * Matrix(qoffset).get_transpose();
				start_pos = Engine->get_position(target.object) - start_ornt * poffset;
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

void FullStream::change_position(const Vector & new_pos)
{
// Change start position.
	if (target.type == OBJECT)
	{
		Vector offset;

	// instead of using the actual position here, we should be using the 
	// theoretical position based on this motion's parameters only.
		const void* baseval;
		const void* nextval;

		SINGLE ratio;

		GENRESULT gr = archetype->get_data_pair (playback_head, baseval, nextval, ratio);
		ASSERT (gr == GR_OK);

		Vector p (*(Vector*)baseval + ((*(Vector*)nextval - *(Vector*)baseval) * ratio));
		offset = p * weight * translation_scale;
		//pos = start_pos + start_ornt * p;

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
		const void* baseval;
		const void* nextval;

		SINGLE ratio;

		GENRESULT gr = archetype->get_data_pair (playback_head, baseval, nextval, ratio);
		ASSERT (gr == GR_OK);

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

U32 FullStream::get_data_type (void) const
{
	return DT_QUATERNION | DT_VECTOR;
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

U32 VectorStream::get_data_type (void) const
{
	return DT_VECTOR;
}

//

