// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef LocationRole_h
#define LocationRole_h

#include "Role.h"
#include "FlaggedLocation.h"
#include "TimeType.h"
#include "Vector.h"
#include "Spline.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
typedef Role<FlaggedLocation> LocationRole;
// --------------------------------------------------------------------------
inline ROSString GetStateName(const FlaggedLocation& location)
{
	switch(location.GetInterpolationType())
	{
		case FlaggedLocation::kLinearFixed:
			return "Linear, Fixed";
			break;
		case FlaggedLocation::kSplineFixed:
			return "Spline, Fixed";
			break;
		case FlaggedLocation::kLinearBlend:
			return "Linear, Blend";
			break;
		case FlaggedLocation::kSplineBlend:
			return "Spline, Blend";
			break;
		default:
			return "";
			ASSERT(0); // Unknown case
	}
}
// --------------------------------------------------------------------------
static ROSString GetThornRoleInfo(LocationRole* role, ROSString &entityName)
{
	char buffer[1024*2];
	char lengthBuffer[1024];

	ROS::Time time(0);
	ROS::Time startTime(0);
	int i = sprintf(buffer, "\n%s_path =\n{\n\ttype = MOTION_PATH,\n\tflags = 0,\n", entityName.c_str());
	i += sprintf(buffer+i, "\tpathprops =\n\t{\n\t\tcontrol_pts =\n\t\t{");

	const unsigned int timePointCount = role->CountTimePoints();

	if (timePointCount < 2)
	{
		// there is no change from the initial position which is stored w/the entity, so just return nothing
		return ROSString("");
	}

	Vector p0(0,0,0);
	Vector p1(0,0,0);
	float totalLength = 0.0f;
	int j = 0;
	FlaggedLocation	location;
	for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
	{
		time = role->GetTime(timePointIdx);
		location = role->GetState(timePointIdx);
		p0 = p1; // store last point in p0
		p1 = location.GetVector();
		if (timePointIdx)
		{	// add trailing comma to last event line
			i += sprintf(buffer + i, ",");
			Spline spline(p0,p0,p1,p1);
			float segLength = spline.approx_length();
			j += sprintf(lengthBuffer + j, "\n--\t\t\t{seg %d, length %f, time %f}", timePointIdx, segLength, time.GetTime());
			totalLength += segLength;
		}
		else
		{
			// store the starting time
			startTime = time;
		}
		// add an indented event line
		i += sprintf(buffer + i, "\n\t\t\t-- type = {%s},\n\t\t\t{%f, %f, %f}",
						GetStateName(location).c_str(), location.GetX(), location.GetY(), location.GetZ()
					);
	}
	j += sprintf(lengthBuffer + j, " \n--\t\t\t{total length %f, total time %f}", totalLength, time.GetTime() - startTime.GetTime());
	i += sprintf(buffer + i, "\n%s", lengthBuffer);

	i += sprintf(buffer + i, "\n\t\t}\n\t}\n},\n");

	// add an event to connect this path
	// add an indented event line
	i += sprintf(buffer + i, "\nEVENT[{%f, ADD_PATH, {\"%s\", \"%s_path\"}, {percent1 = 0.0, percent2 = 1.0, duration = %f, flags = PATH_POSITION}}]",
						startTime.GetTime(),
						entityName.c_str(),
						entityName.c_str(),
						time.GetTime() - startTime.GetTime()
					);



	return ROSString(buffer);
}
// --------------------------------------------------------------------------
FlaggedLocation Interpolate(LocationRole::TimeStateIterator& previousIterator, LocationRole::TimeStateIterator& nextIterator, Time currentTime, const LocationRole::TimeStateIterator& begin, const LocationRole::TimeStateIterator& end);
Vector			GetTangent(const LocationRole& locationRole, Time time);
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif