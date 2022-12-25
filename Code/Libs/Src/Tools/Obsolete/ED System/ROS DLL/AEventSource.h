// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef AEventSource_h
#define AEventSource_h
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
// AEventSource
// --------------------------------------------------------------------------
template <class TEventListener>
class AEventSource
{
	public:
		virtual ~AEventSource()
		{
		}

		virtual void AddListener(TEventListener& listener) = 0;
		virtual void RemoveListener(TEventListener& listener) = 0;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif