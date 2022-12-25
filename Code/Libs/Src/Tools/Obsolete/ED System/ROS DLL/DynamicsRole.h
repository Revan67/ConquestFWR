// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DynamicsRole_h
#define DynamicsRole_h

#include "Role.h"
#include "DynamicsState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
typedef Role<DynamicsState> DynamicsRole;
// --------------------------------------------------------------------------
inline ROSString GetStateName(const DynamicsState& dynamicsState)
{
	return "Dynamics Change";
}
// --------------------------------------------------------------------------
inline DynamicsState LinearInterpolate(DynamicsRole::TimeStateIterator& previousIterator, DynamicsRole::TimeStateIterator& nextIterator, Time currentTime, const DynamicsRole::TimeStateIterator& begin, const DynamicsRole::TimeStateIterator& end)
{
	const Time			previousTime = (*previousIterator)->GetTime();
	const DynamicsState	previousState = (*previousIterator)->GetState();

	const Time			nextTime = (*nextIterator)->GetTime();
	const DynamicsState	nextState = (*nextIterator)->GetState();

	DynamicsState	tState;

	float t = (currentTime - previousTime).GetTime() / (nextTime - previousTime).GetTime();

    previousState.Interpolate(nextState, t, tState);

	return tState;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif