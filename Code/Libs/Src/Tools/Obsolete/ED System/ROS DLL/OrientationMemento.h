// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef OrientationMemento_h
#define OrientationMemento_h
// --------------------------------------------------------------------------
#include "Orientation.h"
#include "TimeType.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class OrientationMemento
{
	public:
		OrientationMemento(const ROS::Orientation& orientation, bool keyPoint, ROS::Time time)
		:mOrientation(orientation), mKeyPoint(keyPoint), mTime(time)
		{
		}

		Orientation GetOrientation() const
		{
			return mOrientation;
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
		ROS::Orientation	mOrientation;
		bool				mKeyPoint;
		ROS::Time			mTime;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif