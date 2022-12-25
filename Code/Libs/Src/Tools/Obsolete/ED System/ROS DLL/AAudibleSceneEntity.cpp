// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "AAudibleSceneEntity.h"
#include "SceneEntityState.h"
#include "AStaticSceneEntity.h"
#include "ConstAudioStateAccessor.h"
#include "AudioStateAccessor.h"
// --------------------------------------------------------------------------

namespace ROS
{
// --------------------------------------------------------------------------
// AudioRoleCallback methods
// --------------------------------------------------------------------------
AAudibleSceneEntity::AudioRoleCallback::AudioRoleCallback(AAudibleSceneEntity& audibleSE)
: mAudibleSE(audibleSE), mLastDAAudioObjectRemoved(NULL)
{
}
// --------------------------------------------------------------------------
void AAudibleSceneEntity::AudioRoleCallback::RemoveStarted(const AudioRole::UpdateCallback::RoleType& role, Time time) 
{
	mLastDAAudioObjectRemoved = role.GetState(time).GetDAAudioObject();
}
// --------------------------------------------------------------------------
void AAudibleSceneEntity::AudioRoleCallback::RemoveFinished(const AudioRole::UpdateCallback::RoleType& role, Time time) 
{
	RemoveAudio();
}
// --------------------------------------------------------------------------
void AAudibleSceneEntity::AudioRoleCallback::RemoveStarted(const AudioRole::UpdateCallback::RoleType& role, unsigned int timePointIndex) 
{
	mLastDAAudioObjectRemoved = role.GetState(timePointIndex).GetDAAudioObject();
}
// --------------------------------------------------------------------------
void AAudibleSceneEntity::AudioRoleCallback::RemoveFinished(const AudioRole::UpdateCallback::RoleType& role, unsigned int timePointIndex) 
{
	RemoveAudio();
}
// --------------------------------------------------------------------------
void AAudibleSceneEntity::AudioRoleCallback::RemoveAudio()
{
	if(mLastDAAudioObjectRemoved)
	{
		mAudibleSE.DestroyDAAudioObject(mLastDAAudioObjectRemoved);
	}
}
// --------------------------------------------------------------------------
//  AAudibleSceneEntity methods
// ---------------------------------------------------------------------------
AAudibleSceneEntity::AAudibleSceneEntity()
{
}
// ---------------------------------------------------------------------------
AAudibleSceneEntity::~AAudibleSceneEntity()
{
}
// ---------------------------------------------------------------------------
void AAudibleSceneEntity::Delete()
{
	BaseClass::Delete();

	DestroyAllDAAudioObjects();
}
// ---------------------------------------------------------------------------
const std::auto_ptr<ConstAudioStateAccessor> AAudibleSceneEntity::GetConstAudioStateAccessor() const
{
	return std::auto_ptr<ConstAudioStateAccessor>(new ConstAudioStateAccessor(*this));
}
// ---------------------------------------------------------------------------
std::auto_ptr<AudioStateAccessor> AAudibleSceneEntity::GetAudioStateAccessor()
{
	return std::auto_ptr<AudioStateAccessor>(new AudioStateAccessor(*this));
}
// ---------------------------------------------------------------------------
void AAudibleSceneEntity::Goto(Time time)
{	
	BaseClass::Goto(time);

	GotoForAudioRole(time);
}
// --------------------------------------------------------------------------
void AAudibleSceneEntity::GotoForAudioRole(Time time)
{   
	const int	audioRoleIndex = GetAudioRoleIndex();
	ASSERT(audioRoleIndex >= 0);
	ARole*		aRole = &GetSceneEntityState().GetRole(audioRoleIndex);
	AudioRole*	audioRole = dynamic_cast<AudioRole*>(aRole);
    ASSERT(audioRole);
    
	Time		audioStartTime(0);
	const bool	hasEvent = audioRole->GetNearestPreviousOrEqualTime(time, audioStartTime);

	if(!hasEvent || time == kTime0)
	{
		InitAudioPlayingFlags();
		DestroyAllDAAudioObjects();
   	}

	const AudioState                audioState = audioRole->GetState(time);
    const AudioState::AudioEventID  audioEvent = audioState.GetAudioEvent();
	const ROSString					audioName = audioState.GetAudioName();
	const DAAudioObject*			dAAudioObject = audioState.GetDAAudioObject();

    if(Performing())
    {   
        switch(audioEvent)
        {	
			case AudioState::kStartAudio:
                if(IsAudioActuallyPlayingFlagSet(audioStartTime))
                {
                    return;
                }
                else
                {   
					SetAudioActuallyPlayingFlag(audioStartTime, true);
                }

#if 0
//OutputDebugString("Starting audio: ");
//OutputDebugString(audioName);
//OutputDebugString("\n");
#endif
				if(!dAAudioObject)
				{
					dAAudioObject = GetDAAudioObject(audioName, audioStartTime);

					if(dAAudioObject)
					{
						AudioState	newAudioState = audioState;

						newAudioState.SetDAAudioObject(dAAudioObject);
						audioRole->StateUpdated(newAudioState, audioStartTime);
					}
				}

				if(dAAudioObject)
				{	
					PlayDAAudioObject(dAAudioObject, time - audioStartTime);
				}
                break;

            case AudioState::kLoopAudio:
                if(IsAudioActuallyPlayingFlagSet(audioStartTime))
                {
                    return;
                }
                else
                {   
					SetAudioActuallyPlayingFlag(audioStartTime, true);
                }

#if 0
//OutputDebugString("Starting audio loop: ");
//OutputDebugString(audioName);
//OutputDebugString("\n");
#endif
				if(!dAAudioObject)
				{
					dAAudioObject = GetDAAudioObject(audioName, audioStartTime);

					if(dAAudioObject)
					{
						AudioState	newAudioState = audioState;

						newAudioState.SetDAAudioObject(dAAudioObject);
						audioRole->StateUpdated(newAudioState, audioStartTime);
					}
				}

				if(dAAudioObject)
				{	
					PlayDAAudioObject(dAAudioObject, time - audioStartTime);
				}
                break;

#if 0
            case AudioState::kPauseAudio:
                if(IsAudioActuallyPlayingFlagSet(audioStartTime))
                {   
					if(mDADeformableObject)
					{
						AudioObjectPause(audioObj);
					}

                    SetAudioActuallyPlayingFlag(audioStartTime, false);
                }
                break;

            case AudioState::kResumeAudio:
                if(IsAudioActuallyPlayingFlagSet(audioStartTime))
                {   
					if(mDADeformableObject)
					{
						AudioObjectResume(audioObj);
					}

                    SetAudioActuallyPlayingFlag(audioStartTime, true);
                }
                break;
#endif
            case AudioState::kStopAudio:
#if 0
                if(IsAudioActuallyPlayingFlagSet(audioStartTime))
                {   
					if(mDADeformableObject)
					{
		                mSceneEntityState.GetScene().StopDAAudioObject(audioObj);
					}

                    SetAudioActuallyPlayingFlag(audioStartTime, false);
                }
#endif
                break;

			case AudioState::kInternalAudio:
				// Don't do anything!
				break;

            default:
                ASSERT(0);	// Unhandled case
                return;
        }
    }
    else
    {   // Not performing
        InitAudioPlayingFlags();
		DestroyAllDAAudioObjects();

#if 0
        // NOTE:    Leaving this out for now since there is no way to set the current time point in the audio

        switch(audioEvent)
        {	
			case AudioState::kStartAudio:
                Time    time = audioState.GetTimeValue();
                AudioObjectSetCurrentAudioTime(
                break;
            case AudioState::kLoopAudio:
                ASSERT(0);  /*** NOTE: Haven't coded this part yet*****/
                break;
            case AudioState::kPauseAudio:
                ASSERT(0);  /*** NOTE: Haven't coded this part yet*****/
                break;
            case AudioState::kResumeAudio:
                ASSERT(0);  /*** NOTE: Haven't coded this part yet*****/
                break;
            case AudioState::kStopAudio:
                return	AudioState(AudioState::kStopAudio, previousState.GetTimeValue());
                break;
            default:
                ASSERT(0);	// Unhandled case
                return previousState;
        }
#endif
    }
}
// ---------------------------------------------------------------------------
void AAudibleSceneEntity::StateUpdated(Update::ID update, Time time)
{
}
// ---------------------------------------------------------------------------
void AAudibleSceneEntity::StartSound(const ROSString& name, const StringList& descriptionStrings, Time startTimePoint)
{
	if(!Performing())
    {   
		Time		currentTime = GetCurrentTimePoint();
		const int	audioRoleIndex = GetAudioRoleIndex();
		ASSERT(audioRoleIndex >= 0);
		ARole*		aRole = &GetSceneEntityState().GetRole(audioRoleIndex);

        AudioRole* audioRole = dynamic_cast<AudioRole*>(aRole);
		ASSERT(audioRole);

        audioRole->StateUpdated(AudioState(AudioState::kStartAudio, currentTime, name, startTimePoint), currentTime);
    }
}
// ---------------------------------------------------------------------------
void AAudibleSceneEntity::InitAudioPlayingFlags()
{
    ASSERT(GetAudioRoleIndex() >= 0 && GetSceneEntityState().GetRoleCount() > GetAudioRoleIndex());

    GetAudioPlayingFlags().clear();
}
// ---------------------------------------------------------------------------
void AAudibleSceneEntity::SetAudioActuallyPlayingFlag(Time startTime, bool isPlaying)
{
    ASSERT(GetAudioRoleIndex() >= 0 && GetSceneEntityState().GetRoleCount() > GetAudioRoleIndex());
    
	if(isPlaying)
	{
		GetAudioPlayingFlags().insert(startTime);
	}
	else
	{
#if 0
		GetAudioPlayingFlags().erase(startTime);	// erase may be buggy
#else
		AudioPlayingFlags&	flags = GetAudioPlayingFlags();

		AudioPlayingFlags::iterator	iter = flags.find(startTime);

		if(iter != flags.end())
		{
			flags.erase(iter);
		}
#endif
	}
}
// ---------------------------------------------------------------------------
bool AAudibleSceneEntity::IsAudioActuallyPlayingFlagSet(Time startTime) const
{
    ASSERT(GetAudioRoleIndex() >= 0 && GetSceneEntityState().GetRoleCount() > GetAudioRoleIndex());

	const AudioPlayingFlags&	flags = GetAudioPlayingFlags();

    return (flags.find(startTime) != flags.end());
}
// --------------------------------------------------------------------------
const DAAudioObject* AAudibleSceneEntity::GetDAAudioObject(const ROSString& audioName, Time startTime)
{
    return GetSceneEntityState().GetScene().CreateDAAudioObject(audioName, startTime, dynamic_cast<AStaticSceneEntity*>(this));
}
// --------------------------------------------------------------------------
void AAudibleSceneEntity::PlayDAAudioObject(const DAAudioObject* dAAudioObect, Time startTime)
{
	ASSERT(dAAudioObect);
	
	GetSceneEntityState().GetScene().PlayDAAudioObject(dAAudioObect, startTime);
}
// --------------------------------------------------------------------------
void AAudibleSceneEntity::DestroyAllDAAudioObjects()
{
	const int	audioRoleIndex = GetAudioRoleIndex();
	ASSERT(audioRoleIndex >= 0);
	ARole*		aRole = &GetSceneEntityState().GetRole(audioRoleIndex);
    AudioRole*	audioRole = dynamic_cast<AudioRole*>(aRole);
	ASSERT(audioRole);

	const unsigned int count = audioRole->CountTimePoints();

	for(unsigned int idx = 0; idx < count; ++idx)
    {   
		AudioState				audioState = audioRole->GetState(idx);
		const DAAudioObject*	dAAudioObject = audioState.GetDAAudioObject();

		if(dAAudioObject)
		{
			DestroyDAAudioObject(dAAudioObject);

			audioState.SetDAAudioObject(NULL);

			audioRole->StateUpdated(audioState, idx);
		}
    }
}
// --------------------------------------------------------------------------
void AAudibleSceneEntity::DestroyDAAudioObject(const DAAudioObject* dAAudioObject)
{
	ASSERT(dAAudioObject);

	GetSceneEntityState().GetScene().DestroyDAAudioObject(dAAudioObject);
}
// ---------------------------------------------------------------------------
void AAudibleSceneEntity::Write(std::ostream& oStream) const
{
	BaseClass::Write(oStream);
}
// ---------------------------------------------------------------------------
void AAudibleSceneEntity::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	InitAudioPlayingFlags();
}
// --------------------------------------------------------------------------
}