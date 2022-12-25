// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef EventListenerTracker_h
#define EventListenerTracker_h
// --------------------------------------------------------------------------
#include <list>
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
// EventListenerTracker
// --------------------------------------------------------------------------
template <class TEventListener, class TEvent>
class EventListenerTracker
{
	public:
		void Add(TEventListener& listener)
		{
			mListeners.push_back(&listener);
		}

		void Remove(TEventListener& listener)
		{
#if 0
			// The remove algorithm doesn't work for the last entry
			mListeners.remove(&listener);
#else
			ListenerCollection::iterator	begin = mListeners.begin();
			ListenerCollection::iterator	end = mListeners.end();

			while(begin != end)
			{	
				if(*begin == &listener)
				{
					mListeners.erase(begin);
					
					return;
				}
				
				++begin;
			}

			ASSERT(0);	// If this fires, the entry was not found!
#endif
		}

		void RemoveAll()
		{
			mListeners.clear();
		}

		unsigned int GetCount() const
		{
			return mListeners.size();
		}

		TEventListener& Get(unsigned int listenerIndex) const
		{
			ASSERT(listenerIndex < mListeners.size());

			ListenerCollection::const_iterator	begin = mListeners.begin();
			
			for(unsigned int idx = 0; idx < listenerIndex; ++idx)
			{
				++begin;
			}
			
			return **begin;
		}

		void Fire(const TEvent& event)
		{
			// Inform all listeners. Operate on a copy of the list so
			// that the iterators don't become invalid in case the 
			// listener decides to modify the list of listeners.
			ListenerCollection					listeners = mListeners;
			ListenerCollection::iterator		begin = listeners.begin();
			const ListenerCollection::iterator	end = listeners.end();

			while(begin != end)
			{
				(*begin)->Respond(event);

				++begin;
			}
		}

		void Clear()
		{
			mListeners.clear();
		}

	private:
		typedef std::list<TEventListener*> ListenerCollection;

		ListenerCollection	mListeners;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif