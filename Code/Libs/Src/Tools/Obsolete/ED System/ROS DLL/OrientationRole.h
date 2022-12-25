// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef OrientationRole_h
#define OrientationRole_h

#include "InfoRole.h"
#include "FlaggedOrientation.h"
#include "LocationRole.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
typedef InfoRole<FlaggedOrientation, LocationRole> OrientationRole;
// --------------------------------------------------------------------------
inline ROSString GetStateName(const FlaggedOrientation& orientation)
{
	switch(orientation.GetInterpolationType())
	{
		case FlaggedOrientation::kLinear:
			return "Linear";
			break;
		case FlaggedOrientation::kTangent:
			return "Tangent";
			break;
		case FlaggedOrientation::kLookAt:
			return "LookAt";
			break;
		case FlaggedOrientation::kSpline:
			return "Spline";
			break;
		default:
			return "";
			ASSERT(0); // Unknown case
	}
}
// --------------------------------------------------------------------------
static ROSString GetThornRoleInfo(OrientationRole* role, ROSString &entityName)
{
	char buffer[1024*2];

	ROS::Time time(0);
	ROS::Time startTime(0);
	ROS::Time totalTime(0);
	int i = sprintf(buffer, "\n%s_orientation_anim =\n{\n\ttype = PROPERTY_ANIM,\n\tflags = ORIENTATION_ANIM,\n", entityName.c_str());
	i += sprintf(buffer+i, "\tanimprops =\n\t{\n\t\tanim_pts =\n\t\t{");

	const unsigned int timePointCount = role->CountTimePoints();


	if (timePointCount < 2)
	{
		// there is no change from the initial orientation which is stored w/the entity, so just return nothing
		return ROSString("");
	}

	totalTime = role->GetTime(timePointCount - 1);

	for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
	{
		time = role->GetTime(timePointIdx);
		const FlaggedOrientation	orientation = role->GetState(timePointIdx);

		if (timePointIdx)
		{	// add trailing comma to last event line
			i += sprintf(buffer + i, ",");
		}
		else
		{
			startTime = time;
		}
		// add an indented event line
		i += sprintf(buffer + i, "\n\t\t\t{percent = %f, time = %f, orient = {{%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}}, type = {%s}}",
						(time.GetTime() - startTime.GetTime())/(totalTime.GetTime() - startTime.GetTime()),
						time.GetTime(),
						orientation.GetI().x, orientation.GetI().y, orientation.GetI().z, 
						orientation.GetJ().x, orientation.GetJ().y, orientation.GetJ().z, 
						orientation.GetK().x, orientation.GetK().y, orientation.GetK().z,
						GetStateName(orientation).c_str()
					);
	}
	i += sprintf(buffer + i, "\n\t\t}\n\t}\n},\n");

	// add an event line
	i += sprintf(buffer + i, "\nEVENT[{%f, ADD_PATH, {\"%s\", \"%s_orientation_anim\"}, {percent1 = 0.0, percent2 = 1.0, duration = %f}}]",
						startTime.GetTime(),
						entityName.c_str(),
						entityName.c_str(),
						time.GetTime() - startTime.GetTime()
					);

	return ROSString(buffer);
}
// --------------------------------------------------------------------------
FlaggedOrientation LinearInterpolate(const OrientationRole& role, OrientationRole::TimeStateIterator& previousIterator, OrientationRole::TimeStateIterator& nextIterator, Time currentTime, const OrientationRole::TimeStateIterator& begin, const OrientationRole::TimeStateIterator& end, LocationRole* locationRole);
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif