// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef AudioRole_h
#define AudioRole_h

#include "Role.h"
#include "AudioState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
typedef Role<AudioState> AudioRole;
// --------------------------------------------------------------------------
static ROSString GetStateName(const AudioState& audioState)
{
	AudioState::AudioEventID	audioEvent = audioState.GetAudioEvent();
    ROSString                   stateName;

    switch(audioEvent)
    {	
		case AudioState::kStartAudio:
    		stateName = "Start";
    		break;
    	case AudioState::kLoopAudio:
    		stateName = "Loop";
    		break;
    	case AudioState::kPauseAudio:
    		stateName = "Pause";
        	break;
        case AudioState::kResumeAudio:
    		stateName = "Resume";
        	break;
        case AudioState::kStopAudio:
    		stateName = "Stop";
        	break;
        default:
        	ASSERT(0);	// Unhandled case
            return "";
    }

    stateName += ROSString(": ") + audioState.GetAudioName();
    
	return  stateName;
}
// --------------------------------------------------------------------------
inline AudioState Interpolate(AudioRole::TimeStateIterator& previousIterator, AudioRole::TimeStateIterator& nextIterator, Time currentTime, const AudioRole::TimeStateIterator& begin, const AudioRole::TimeStateIterator& end)
{
	const Time			previousTime = (*previousIterator)->GetTime();
	const AudioState	previousState = (*previousIterator)->GetState();

	const Time			nextTime = (*nextIterator)->GetTime();
	const AudioState	nextState = (*nextIterator)->GetState();

	if(previousTime == nextTime)
	{	
		// We are at the first or last know state in the role
		if(currentTime < previousTime)
		{
			// The current time lies before the first state
			// if the time is right on, it will be handled later
			return AudioState(AudioState::kInternalAudio, previousState.GetTimeValue(), previousState.GetAudioName(), previousState.GetStartTimePoint());
		}
	}

	AudioState::AudioEventID	audioEvent = previousState.GetAudioEvent();

    switch(audioEvent)
    {	
		case AudioState::kStartAudio:
    		return	AudioState(AudioState::kStartAudio,		previousState.GetTimeValue() + currentTime - previousTime,	previousState.GetAudioName(),	previousState.GetStartTimePoint());
    		break;
    	case AudioState::kLoopAudio:
    		return	AudioState(AudioState::kLoopAudio,		previousState.GetTimeValue() + currentTime - previousTime,	previousState.GetAudioName(),	previousState.GetStartTimePoint());
    		break;
    	case AudioState::kPauseAudio:
    		return	AudioState(AudioState::kPauseAudio,		previousState.GetTimeValue(),								previousState.GetAudioName(),	previousState.GetStartTimePoint());
        	break;
    	case AudioState::kResumeAudio:
    		return	AudioState(AudioState::kResumeAudio,	previousState.GetTimeValue() + currentTime - previousTime,	previousState.GetAudioName(),	previousState.GetStartTimePoint());
        	break;
    	case AudioState::kStopAudio:
    		return	AudioState(AudioState::kStopAudio,		previousState.GetTimeValue(),								previousState.GetAudioName(),	previousState.GetStartTimePoint());
        	break;
        default:
        	ASSERT(0);	// Unhandled case
            return previousState;
    }
}
// --------------------------------------------------------------------------
static ROSString GetThornRoleInfo(AudioRole*	role, ROSString &entityName)
{
	char buffer[1024*5];

	ROSString stateString;
	ROS::Time time(0);
	int i = 0;
	const unsigned int timePointCount = role->CountTimePoints();
	
	if (timePointCount < 1)
	{
		return ROSString("");
	}

	// write the entity info for each start
	for (unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
	{
		time = role->GetTime(timePointIdx);
		
		AudioState& state = role->GetState(time);
		
		switch (state.GetAudioEvent())
		{
			case AudioState::kStartAudio:
				// add an audio entity
				i+= sprintf(buffer + i, "\n%s =\n{\n\ttype = SOUND,\n\tflags = 0,\n",
									state.GetAudioName().c_str());
				i+= sprintf(buffer + i,		"\tspatialprops = \n\t{\n");
				i+= sprintf(buffer + i,		"\t\tpos = {%f, %f, %f},\n", 0,0,0);
				i+= sprintf(buffer + i,		"\t\torient = { {%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f} }\n",
							0,0,0,0,0,0,0,0,0);
				i+= sprintf(buffer + i,		"\t},\n");
				i+= sprintf(buffer + i,		"\tuserprops = \n\t{\n");
				i+= sprintf(buffer + i,		"\t\tassociated_entity = \"%s\"\n", entityName.c_str());
				i+= sprintf(buffer + i,		"\t}\n");
				i+= sprintf(buffer + i,		"},\n");
    			break;
		}
	}

	// write the events
	for (timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
	{
		time = role->GetTime(timePointIdx);
		
		AudioState& state = role->GetState(time);

		switch (state.GetAudioEvent())
		{
			case AudioState::kStartAudio:
    			stateString = "START";
    			break;
    		case AudioState::kPauseAudio:
    			stateString = "START";
        		break;
    		case AudioState::kLoopAudio:
    			stateString = "LOOP";
    			break;
			case AudioState::kResumeAudio:
    			stateString = "RESUME";
        		break;
			case AudioState::kStopAudio:
    			stateString = "STOP";
        		break;
		}
		if (timePointIdx)
		{
			// add trailing comma to last event line
			i += sprintf(buffer + i, ",");
		}
		// add an indented event line
		i += sprintf(buffer + i, "\nEVENT[{%f, %s, {\"%s\"}, {entity = \"%s\"}}]",
						time.GetTime(),
						stateString.c_str(),
						state.GetAudioName().c_str(),
						entityName.c_str()
					);
	}
	
	return ROSString(buffer);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif