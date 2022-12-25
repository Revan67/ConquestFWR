// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef LightRole_h
#define LightRole_h

#include "Role.h"
#include "LightState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
typedef Role<LightState> LightRole;
// --------------------------------------------------------------------------
inline ROSString GetStateName(const LightState& lightState)
{
	return "Color Change";
}
// --------------------------------------------------------------------------
static LightState LinearInterpolate(LightRole::TimeStateIterator& previousIterator, LightRole::TimeStateIterator& nextIterator, Time currentTime, const LightRole::TimeStateIterator& begin, const LightRole::TimeStateIterator& end)
{
	const Time			previousTime = (*previousIterator)->GetTime();
	const LightState	previousState = (*previousIterator)->GetState();

	const Time			nextTime = (*nextIterator)->GetTime();
	const LightState	nextState = (*nextIterator)->GetState();


    if(nextTime == previousTime)
    {
		return previousState;
    }
    else
    {
		float t = (currentTime - previousTime).GetTime() / (nextTime - previousTime).GetTime();

		LightState	tState;

		previousState.Interpolate(nextState, t, tState);

		return tState;
    }
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif