// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "LiveCameraRole.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
LiveCameraState Interpolate(LiveCameraRole::TimeStateIterator& previousIterator, LiveCameraRole::TimeStateIterator& nextIterator, Time currentTime, const LiveCameraRole::TimeStateIterator& begin, const LiveCameraRole::TimeStateIterator& end)
{
	return (*previousIterator)->GetState();
}
// --------------------------------------------------------------------------
}
