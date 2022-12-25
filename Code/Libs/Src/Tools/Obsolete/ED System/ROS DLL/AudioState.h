//---------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef AudioState_h
#define AudioState_h
// --------------------------------------------------------------------------
#include <iostream>
#include "TimeType.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
namespace ROS
{
class DAAudioObject;
// --------------------------------------------------------------------------
class AudioState
{
	public:
        enum AudioEventID
        {
			kStartAudio,
            kLoopAudio,
            kPauseAudio,
            kResumeAudio,
            kStopAudio,
			kInternalAudio	// For internal use. 
        };

        AudioState();
		AudioState(AudioEventID audioEvent, Time timeValue, const ROSString& audioName, Time startPoint);

        void SetAudioEvent(AudioEventID audioEvent);
        void SetTimeValue(Time timeValue);
        void SetAudioName(const ROSString& audioName);
        void SetStartTimePoint(Time startPoint);
		void SetDAAudioObject(const DAAudioObject* dAAudioObject);
        void Set(AudioEventID audioEvent, Time timeValue, const ROSString& audioName, Time startPoint);

        AudioEventID GetAudioEvent() const;
        Time GetTimeValue() const;
        ROSString GetAudioName() const;
        Time GetStartTimePoint() const;
		const DAAudioObject* GetDAAudioObject() const;

        void Write(std::ostream& oStream) const;
        void Read(std::istream& iStream);

    private:
		enum FieldID
		{
			kAudioEvent,
			kTimeValue,
			kAudioName,
			kStartTimePoint
		};

    	AudioEventID	        mAudioEventID;
        Time                    mTimeValue;         // Placement of this state on the timeline
        ROSString               mAudioFilename;		// Not to be used starting ROS version 2.5.0.0. Use mAudioName instead.
        Time                    mStartTimePoint;    // How far into the sound does sound start playing
        ROSString               mAudioName;			// Introduced in ROS version 2.5.0.0.
		const DAAudioObject*	mDAAudioObject;		// This member is not to be written out
};
// --------------------------------------------------------------------------
inline AudioState::AudioState()
: mAudioEventID(kStopAudio), mTimeValue(0), mStartTimePoint(0), mDAAudioObject(NULL)
{
}
// --------------------------------------------------------------------------
inline AudioState::AudioState(AudioEventID audioEvent, Time timeValue, const ROSString& audioName, Time startPoint)
: mAudioEventID(audioEvent), mTimeValue(timeValue), mAudioName(audioName), mStartTimePoint(startPoint), mDAAudioObject(NULL)
{
}
// --------------------------------------------------------------------------
inline void AudioState::SetAudioEvent(AudioEventID audioEvent)
{
	mAudioEventID = audioEvent;
}
// --------------------------------------------------------------------------
inline void AudioState::SetTimeValue(Time timeValue)
{
	mTimeValue = timeValue;
}
// --------------------------------------------------------------------------
inline void AudioState::SetAudioName(const ROSString& audioName)
{
	mAudioName = audioName;
}
// --------------------------------------------------------------------------
inline void AudioState::SetStartTimePoint(Time startPoint)
{
	mStartTimePoint = startPoint;
}
// --------------------------------------------------------------------------
inline void AudioState::SetDAAudioObject(const DAAudioObject* dAAudioObject)
{
	mDAAudioObject = dAAudioObject;
}
// --------------------------------------------------------------------------
inline void AudioState::Set(AudioEventID audioEvent, Time timeValue, const ROSString& audioName, Time startPoint)
{
	SetAudioEvent(audioEvent);
    SetTimeValue(timeValue);
    SetAudioName(audioName);
    SetStartTimePoint(startPoint);
}
// --------------------------------------------------------------------------
inline AudioState::AudioEventID AudioState::GetAudioEvent() const
{
	return mAudioEventID;
}
// --------------------------------------------------------------------------
inline Time AudioState::GetTimeValue() const
{
	return mTimeValue;
}
// --------------------------------------------------------------------------
inline ROSString AudioState::GetAudioName() const
{
	return mAudioName;
}
// --------------------------------------------------------------------------
inline Time AudioState::GetStartTimePoint() const
{
	return mStartTimePoint;
}
// --------------------------------------------------------------------------
inline const DAAudioObject* AudioState::GetDAAudioObject() const
{
	return mDAAudioObject;
}
// --------------------------------------------------------------------------
inline void AudioState::Write(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

    int audioEvent = mAudioEventID;

	oWiz.Put(kAudioEvent, audioEvent);
	oWiz.Put(kTimeValue, mTimeValue);
	oWiz.Put(kAudioName, mAudioName);
	oWiz.Put(kStartTimePoint, mStartTimePoint);
}
// --------------------------------------------------------------------------
inline void AudioState::Read(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

    int         audioEvent;
    ROSString   filename;

	iWiz.Get(kAudioEvent, audioEvent);
    mAudioEventID = (AudioEventID)audioEvent;

	iWiz.Get(kTimeValue, mTimeValue);
	iWiz.Get(kAudioName, mAudioName);
	iWiz.Get(kStartTimePoint, mStartTimePoint);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::AudioState& state)
{
	state.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::AudioState& state)
{
	state.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif