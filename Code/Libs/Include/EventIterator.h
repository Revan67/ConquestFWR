#ifndef __EVENTITERATOR_H
#define __EVENTITERATOR_H

const int EVENT_ITERATOR_MAX_EVENTS = 64;

#include "fdump.h"

class EventIterator
{
	public:

		struct Event
		{
			SINGLE time;
			const void* data;
		};

		explicit EventIterator(const Event[]=NULL, unsigned int cnt=0);
		void initialize(const Event[], unsigned int cnt);

		U32	get_event_count() const;

		SINGLE get_event_time(unsigned int event_idx) const;
		U32 get_event_type(unsigned int event_idx) const;
		void* get_event_data(unsigned int event_idx) const;

		const EventIterator& append (const EventIterator& other);

	private:

		Event events[EVENT_ITERATOR_MAX_EVENTS];
		U32 m_data_item_count;
};

inline EventIterator::EventIterator(const Event _events[], unsigned int _cnt)
{
	initialize (_events, _cnt);
}

inline void EventIterator::initialize(const Event _events[], unsigned int _cnt)
{
	ASSERT (_cnt <= EVENT_ITERATOR_MAX_EVENTS);

	m_data_item_count = _cnt;

	for (U32 i = 0; i < _cnt; i++)
		events[i] = _events[i];
}

inline U32 EventIterator::get_event_count() const
{
	return m_data_item_count;
}

inline SINGLE EventIterator::get_event_time(unsigned int event_idx) const
{
	ASSERT (event_idx < m_data_item_count);

	return events[event_idx].time;
}

inline U32 EventIterator::get_event_type(unsigned int event_idx) const
{
	ASSERT (event_idx < m_data_item_count);

	return *((U32*)(events[event_idx].data));
}

inline void* EventIterator::get_event_data(unsigned int event_idx) const
{
	ASSERT (event_idx < m_data_item_count);

	return ((U32*)(events[event_idx].data)) + 1;
}

inline const EventIterator& EventIterator::append (const EventIterator& other)
{
	for (unsigned int i = 0; i < other.m_data_item_count; i++)
	{
		ASSERT (m_data_item_count < EVENT_ITERATOR_MAX_EVENTS);
		events[m_data_item_count] = other.events[i];
		m_data_item_count++;
	}

	return *this;
}

#endif