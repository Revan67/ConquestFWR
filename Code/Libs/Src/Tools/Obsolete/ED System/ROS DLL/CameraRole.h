// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef CameraRole_h
#define CameraRole_h

#include "Role.h"
#include "CameraState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
typedef Role<CameraState> CameraRole;
// --------------------------------------------------------------------------
inline ROSString GetStateName(const CameraState& cameraState)
{
	return "FOV Change";
}
// --------------------------------------------------------------------------
inline CameraState LinearInterpolate(CameraRole::TimeStateIterator& previousIterator, CameraRole::TimeStateIterator& nextIterator, Time currentTime, const CameraRole::TimeStateIterator& begin, const CameraRole::TimeStateIterator& end)
{
	const Time			previousTime = (*previousIterator)->GetTime();
	const CameraState	previousState = (*previousIterator)->GetState();

	const Time			nextTime = (*nextIterator)->GetTime();
	const CameraState	nextState = (*nextIterator)->GetState();


    if(nextTime == previousTime)
    {
		return previousState;
    }
    else
    {
		float t = (currentTime - previousTime).GetTime() / (nextTime - previousTime).GetTime();

		return previousState.Interpolate(nextState, t);
    }
}
// --------------------------------------------------------------------------
static ROSString GetThornRoleInfo(CameraRole* role, ROSString &entityName)
{
	char buffer[1024*2];

	ROS::Time time(0);
	ROS::Time startTime(0);
	int i = sprintf(buffer, "\n%s_camera_anim =\n{\n\ttype = PROPERTY_ANIM,\n\tflags = CAMERA_ANIM,\n", entityName.c_str());
	i += sprintf(buffer+i, "\tanimprops =\n\t{\n\t\tanim_pts =\n\t\t{");

	const unsigned int timePointCount = role->CountTimePoints();


	if (timePointCount < 2)
	{
		// there is no change from the initial orientation which is stored w/the entity, so just return nothing
		return ROSString("");
	}

	for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
	{
		time = role->GetTime(timePointIdx);

		if (timePointIdx)
		{	// add trailing comma to last line
			i += sprintf(buffer + i, ",");
		}
		else
		{
			startTime = time;
		}
		// add an indented line
		i += sprintf(buffer + i, "\n\t\t\t{percent = 0.0, time = %f, fovh = %f}",
						time.GetTime(),
						role->GetState(timePointIdx).GetHorizontalFOV()
					);
	}
	i += sprintf(buffer + i, "\n\t\t}\n\t}\n},\n");

	// add an event line
	i += sprintf(buffer + i, "\nEVENT[{%f, ADD_PATH, {\"%s\", \"%s_camera_anim\"}, {percent1 = 0.0, percent2 = 1.0, duration = %f}}]",
						startTime.GetTime(),
						entityName.c_str(),
						entityName.c_str(),
						time.GetTime() - startTime.GetTime()
					);

	return ROSString(buffer);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif