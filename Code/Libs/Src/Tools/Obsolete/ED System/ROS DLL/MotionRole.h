// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef MotionRole_h
#define MotionRole_h

#include "Role.h"
#include "MotionState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
typedef Role<MotionState> MotionRole;

// --------------------------------------------------------------------------
static ROSString GetStateName(const MotionState& motionState)
{
	MotionState::MotionEventID	motionEvent = motionState.GetMotionEvent();
    ROSString                   stateName;

    switch(motionEvent)
    {
		case MotionState::kStartMotion:
    		stateName = "Start";
    		break;
    	case MotionState::kLoopMotion:
    		stateName = "Loop";
    		break;
    	case MotionState::kPauseMotion:
    		stateName = "Pause";
        	break;
        case MotionState::kResumeMotion:
    		stateName = "Resume";
        	break;
        case MotionState::kStopMotion:
    		stateName = "Stop";
        	break;
		case MotionState::kStartIK:
			stateName = "Start IK";
			break;
		case MotionState::kStopIK:
			stateName = "Stop IK";
			break;
        default:
        	ASSERT(0);	// Unhandled case
            return "";
    }

    stateName += ROSString(": ") + motionState.GetMotionName();
    return  stateName;
}
// --------------------------------------------------------------------------
inline MotionState Interpolate(MotionRole::TimeStateIterator& previousIterator, MotionRole::TimeStateIterator& nextIterator, Time currentTime, const MotionRole::TimeStateIterator& begin, const MotionRole::TimeStateIterator& end)
{
	const Time			previousTime = (*previousIterator)->GetTime();
	const MotionState	previousState = (*previousIterator)->GetState();

	const Time			nextTime = (*nextIterator)->GetTime();
	const MotionState	nextState = (*nextIterator)->GetState();

	if(previousTime == nextTime)
	{	
		// We are at the first or last know state in the role
		if(currentTime < previousTime)
		{
			// The current time lies before the first state
			// If the time is right on, it will be handled later
			return MotionState(MotionState::kInternalMotion, previousState.GetMotionName(), previousState.GetStartTime(), previousState.GetTransitionTime());
		}
	}

	MotionState::MotionEventID	motionEvent = previousState.GetMotionEvent();

	switch(motionEvent)
    {	
		case MotionState::kStartMotion:
    	case MotionState::kLoopMotion:
    	case MotionState::kPauseMotion:
    	case MotionState::kResumeMotion:
    	case MotionState::kStopMotion:
		case MotionState::kStartIK:
		case MotionState::kStopIK:
    		return	previousState;
        	break;
        default:
        	ASSERT(0);	// Unhandled case
            return previousState;
    }
}
// --------------------------------------------------------------------------
static ROSString GetThornRoleInfo(MotionRole*	role, ROSString &entityName)
{
	char buffer[1024*5];
	int i = 0;
	ROSString stateString;
	ROS::Time time(0);
	const unsigned int timePointCount = role->CountTimePoints();

	if (timePointCount < 1)
	{
		return ROSString("");
	}

	for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
	{
		time = role->GetTime(timePointIdx);
		
		MotionState state = role->GetState(time);

		switch (state.GetMotionEvent())
		{
			case MotionState::kStartMotion:
    			stateString = "START";
    			break;
    		case MotionState::kPauseMotion:
    			stateString = "START";
        		break;
    		case MotionState::kLoopMotion:
    			stateString = "LOOP";
    			break;
			case MotionState::kResumeMotion:
    			stateString = "RESUME";
        		break;
			case MotionState::kStopMotion:
    			stateString = "STOP";
        		break;
			case MotionState::kStartIK:
    			stateString = "START_IK";
				break;
			case MotionState::kStopIK:
    			stateString = "STOP_IK";
				break;
		}

		// add an event line
		i += sprintf(buffer + i, "\nEVENT[{%f, %s, {\"%s\"}, {animation = \"%s\", start_time = %f, trans_time = %f}}]",
						time.GetTime(),
						stateString.c_str(),
						entityName.c_str(),
						state.GetMotionName().c_str(),
						state.GetStartTime().GetTime(),
						state.GetTransitionTime().GetTime()
					);
	}

	return ROSString(buffer);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif