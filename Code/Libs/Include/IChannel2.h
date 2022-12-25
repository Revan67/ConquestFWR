#ifndef __ICHANNEL2_H
#define __ICHANNEL2_H

#include "dacom.h"
#include "3dmath.h"
#include "eventiterator.h"

struct IFileSystem;

typedef U32 Offset;

const unsigned int DT_FLOAT = 1;
const unsigned int DT_VECTOR = 2;
const unsigned int DT_QUATERNION = 4;
const unsigned int DT_EVENT = 8;

struct DACOM_NO_VTABLE IChannel2 : public IDAComponent
{
	virtual GENRESULT COMAPI load (IFileSystem*) = 0;

	virtual GENRESULT COMAPI get_duration (SINGLE&) const = 0;
	virtual GENRESULT COMAPI get_data_type (U32&) const = 0;

	//input range [0..get_duration ()]
	virtual GENRESULT COMAPI get_data_pair (SINGLE, const void*&, const void*&, SINGLE& ratio) const = 0;

	virtual GENRESULT COMAPI get_data_at_extreme (bool end, const void*&) const = 0;

	//input range [0..get_duration ()], t0 <= t1. does not report events defined at time t0 or t1.
	//'eventcnt' is number of events during elapsed time 
	//(may be larger than entries).
	virtual GENRESULT COMAPI get_events (SINGLE t0, SINGLE t1, EventIterator::Event[], U32 entries, U32& eventcnt) const = 0;
	virtual GENRESULT COMAPI get_events_at_time (SINGLE, EventIterator::Event[], U32 entries, U32& eventcnt) const = 0;
};

inline size_t GetFrameSize (U32 t)
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

#define IID_IChannel2 MAKE_IID("ChannelArchetype",1)

inline SINGLE InterpolateArc (SINGLE b, SINGLE n, SINGLE ratio)
{
	//EMAURER Interpolate along the smaller of the two possible arcs.

	ASSERT (n <= 2 * PI);

	if (n - b < -PI)
		n += 2 * PI;
	else if (n - b > PI)
		n -= 2 * PI;

	return b + (n - b) * ratio;
}

template <typename T> T Interpolate (const T& b, const T& n, SINGLE ratio)
{
	return b + (n - b) * ratio;
}

inline Quaternion Interpolate (const Quaternion& b, const Quaternion& n, SINGLE ratio)
{
	return slerp (b, n, ratio);
}


#endif