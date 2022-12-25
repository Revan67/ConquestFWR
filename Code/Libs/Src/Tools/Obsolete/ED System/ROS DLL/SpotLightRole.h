// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SpotLightRole_h
#define SpotLightRole_h

#include "Role.h"
#include "SpotLightState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
typedef Role<SpotLightState> SpotLightRole;
// --------------------------------------------------------------------------
inline ROSString GetStateName(const SpotLightState& state)
{
	return "Spotlight Change";
}
// --------------------------------------------------------------------------
inline SpotLightState LinearInterpolate(SpotLightRole::TimeStateIterator& previousIterator, SpotLightRole::TimeStateIterator& nextIterator, Time currentTime, const SpotLightRole::TimeStateIterator& begin, const SpotLightRole::TimeStateIterator& end)
{
	const Time				previousTime = (*previousIterator)->GetTime();
	const SpotLightState	previousState = (*previousIterator)->GetState();

	const Time				nextTime = (*nextIterator)->GetTime();
	const SpotLightState	nextState = (*nextIterator)->GetState();


    if(nextTime == previousTime)
    {
		return previousState;
    }
    else
    {
		float t = (currentTime - previousTime).GetTime() / (nextTime - previousTime).GetTime();

		SpotLightState	tState;

        previousState.Interpolate(nextState, t, tState);

		return tState;
    }
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif