// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef LocationMemento_h
#define LocationMemento_h
// --------------------------------------------------------------------------
#include "Location.h"
#include "TimeType.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class LocationMemento
{
	public:
		LocationMemento(const ROS::Location& location, bool keyPoint, ROS::Time time)
		:mLocation(location), mKeyPoint(keyPoint), mTime(time)
		{
		}

		Location GetLocation() const
		{
			return mLocation;
		}

		bool IsKeyPoint() const
		{
			return mKeyPoint;
		}

		ROS::Time GetTime() const
		{
			return mTime;
		}

	private:
		ROS::Location	mLocation;
		bool			mKeyPoint;
		ROS::Time		mTime;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif