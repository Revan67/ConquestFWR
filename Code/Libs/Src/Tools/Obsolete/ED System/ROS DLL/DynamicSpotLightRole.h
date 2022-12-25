// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DynamicSpotLightRole_h
#define DynamicSpotLightRole_h
// --------------------------------------------------------------------------
#include "Role.h"
#include "DynamicSpotLightState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
typedef Role<DynamicSpotLightState>	DynamicSpotLightRole;
// --------------------------------------------------------------------------
DynamicSpotLightState LinearInterpolate(DynamicSpotLightRole::TimeStateIterator& previousIterator, DynamicSpotLightRole::TimeStateIterator& nextIterator, Time currentTime)
{
	const Time					previousTime = (*previousIterator)->GetTime();
	const DynamicSpotLightState	previousState = (*previousIterator)->GetState();

	const Time					nextTime = (*nextIterator)->GetTime();
	const DynamicSpotLightState	nextState = (*nextIterator)->GetState();

    if(nextTime == previousTime)
    {
		return previousState;
    }
    else
    {
		DynamicSpotLightState	currentState;

		float t = (currentTime - previousTime).GetTime() / (nextTime - previousTime).GetTime();

        previousState.Interpolate(nextState, t, currentState);

		return currentState;
    }
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif