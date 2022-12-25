// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef LiveCameraRole_h
#define LiveCameraRole_h
// --------------------------------------------------------------------------
#include "Role.h"
#include "LiveCameraState.h"
#include "ADynamicCamera.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
typedef Role<LiveCameraState> LiveCameraRole;
// --------------------------------------------------------------------------
inline ROSString GetStateName(const LiveCameraState& state)
{
	ASSERT(state.GetRollingCamera() != NULL);

	return ROSString("Camera Change: ") + state.GetRollingCamera()->GetConstSceneEntityStateAccessor()->GetName();
}
// --------------------------------------------------------------------------
LiveCameraState Interpolate(LiveCameraRole::TimeStateIterator& previousIterator, LiveCameraRole::TimeStateIterator& nextIterator, Time currentTime, const LiveCameraRole::TimeStateIterator& begin, const LiveCameraRole::TimeStateIterator& end);
// --------------------------------------------------------------------------
static ROSString GetThornRoleInfo(LiveCameraRole*	role, ROSString &entityName)
{
	char buffer[1024*2];

	ROSString stateString;
	ROS::Time time(0);
	int i = 0;
	const unsigned int timePointCount = role->CountTimePoints();

	if (timePointCount < 1)
	{
		return ROSString("");
	}

	for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
	{
		time = role->GetTime(timePointIdx);
		
		if (timePointIdx)
		{
			// add trailing comma to last event line
			i += sprintf(buffer + i, ",");
		}
		// add an indented event line
		i += sprintf(buffer + i, "\nEVENT[{%f, SET_CAMERA, {\"monitor_1\",\"%s\"}}]",
						time.GetTime(),
						role->GetState(timePointIdx).GetRollingCamera()->GetConstSceneEntityStateAccessor()->GetName().c_str()
					);
	}

	return ROSString(buffer);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif