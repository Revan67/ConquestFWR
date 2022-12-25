// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ParentRole_h
#define ParentRole_h

#include "Role.h"
#include "ParentState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
typedef Role<ParentState> ParentRole;
// --------------------------------------------------------------------------
ROSString GetStateName(const ParentState& parentState);
// --------------------------------------------------------------------------
inline ParentState Interpolate(ParentRole::TimeStateIterator& previousIterator, ParentRole::TimeStateIterator& nextIterator, Time currentTime, const ParentRole::TimeStateIterator& begin, const ParentRole::TimeStateIterator& end)
{
	const Time			previousTime = (*previousIterator)->GetTime();
	const ParentState	previousState = (*previousIterator)->GetState();

	const Time			nextTime = (*nextIterator)->GetTime();
	const ParentState	nextState = (*nextIterator)->GetState();

	if(previousTime == nextTime)
	{	
		// We are at the first or last know state in the role
		if(currentTime < previousTime)
		{
			// The current time lies before the first state
			// If the time is right on, it will be handled later
			ParentState	state = previousState;
			
			state.SetEvent(ParentState::kInternalParentEvent);

			return state;
		}
	}

	ParentState::ParentEventID	parentEvent = previousState.GetEvent();

	switch(parentEvent)
    {	
		case ParentState::kAttachToParent:
    	case ParentState::kDetachFromParent:
    		return	previousState;
    		break;
        default:
        	ASSERT(0);	// Unhandled case
            return previousState;
    }
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif