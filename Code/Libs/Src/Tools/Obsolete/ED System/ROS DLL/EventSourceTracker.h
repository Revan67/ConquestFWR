// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef EventSourceTracker_h
#define EventSourceTracker_h
// --------------------------------------------------------------------------
#include <list>
#include "CodeMsg.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
// EventSourceTracker
// --------------------------------------------------------------------------
template <class TEventSource, class TEventListener>
class EventSourceTracker
{
	public:
		void Add(TEventSource& source)
		{
			mSources.push_back(&source);
		}

		void Remove(TEventSource& source)
		{
#if 0
			// The remove algorithm doesn't work for the last entry
			mSources.remove(&source);
#else
			SourceCollection::iterator	begin = mSources.begin();
			SourceCollection::iterator	end = mSources.end();

			while(begin != end)
			{	
				if(*begin == &source)
				{
					mSources.erase(begin);
					
					return;
				}
				
				++begin;
			}

			ASSERT(0);	// If this fires, the entry was not found!
#endif
		}

		unsigned int GetCount() const
		{
			return mSources.size();
		}

		TEventSource& Get(unsigned int sourceIndex) const
		{
			ASSERT(sourceIndex < mSources.size());

			SourceCollection::const_iterator	begin = mSources.begin();
			
			for(unsigned int idx = 0; idx < sourceIndex; ++idx)
			{
				++begin;
			}
			
			return **begin;
		}

		void RemoveFromAllSources(TEventListener& listener)
		{
			// Inform all sources. Operate on a copy of the list so
			// that the iterators don't become invalid in case the 
			// source decides to modify the list of sources.
			SourceCollection					sources = mSources;
			SourceCollection::iterator			begin = sources.begin();
			const SourceCollection::iterator	end = sources.end();

			while(begin != end)
			{
				(*begin)->RemoveListener(listener);

				++begin;
			}
		}

		void Clear()
		{
			mSources.clear();
		}

	private:
		typedef std::list<TEventSource*> SourceCollection;

		SourceCollection	mSources;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif