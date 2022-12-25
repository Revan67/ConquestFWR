//---------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef MotionState_h
#define MotionState_h
// --------------------------------------------------------------------------
#include <iostream>
#include "TimeType.h"
#include "CodeMsg.h"
#include "StringType.h"
#include "IKState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class CPP_DECL MotionState
{
	public:
        enum MotionEventID
        {	kStartMotion,
            kLoopMotion,
            kPauseMotion,
            kResumeMotion,
            kStopMotion,
			kStartIK,
			kStopIK,
			kInternalMotion	// For internal use. 
        };

        MotionState();
		MotionState(MotionEventID motionEvent, const ROSString& motionName, Time startTime, Time transition);

        void SetMotionEvent(MotionEventID motionEvent);
        void SetStartTime(Time timeValue);
        void SetMotionName(const ROSString& motionName);
        void SetTransitionTime(Time transition);
        void Set(MotionEventID motionEvent, const ROSString& motionName, Time startTime, Time transition);
		void SetIKRecordIndex(unsigned int index);

        MotionEventID GetMotionEvent() const;
        Time GetStartTime() const;
        ROSString GetMotionName() const;
        Time GetTransitionTime() const;
		unsigned int GetIKRecordIndex() const;

        void Write(std::ostream& oStream) const;
        void Read(std::istream& iStream);

    private:
        void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

    	MotionEventID	mMotionEventID;
		Time			mStartTime;		// How far into the motion do we start
		ROSString		mMotionName;
        Time            mTransitionTime;
		unsigned int	mIKRecordIndex;
};
// --------------------------------------------------------------------------
inline MotionState::MotionState()
: mMotionEventID(kStopMotion), mStartTime(0), mTransitionTime(0)
{
}
// --------------------------------------------------------------------------
inline MotionState::MotionState(MotionEventID motionEvent, const ROSString& motionName, Time startTime, Time transition)
: mMotionEventID(motionEvent), mStartTime(startTime), mMotionName(motionName), mTransitionTime(transition)
{
    ASSERT(mStartTime >= kTime0 && transition >= kTime0);
}
// --------------------------------------------------------------------------
inline void MotionState::SetMotionEvent(MotionEventID motionEvent)
{
	mMotionEventID = motionEvent;
}
// --------------------------------------------------------------------------
inline void MotionState::SetStartTime(Time time)
{
	mStartTime = time;
}
// --------------------------------------------------------------------------
inline void MotionState::SetMotionName(const ROSString& motionName)
{
	mMotionName = motionName;
}
// --------------------------------------------------------------------------
inline void MotionState::SetTransitionTime(Time transition)
{
    ASSERT(transition.GetTime() >= 0);

	mTransitionTime = transition;
}
// --------------------------------------------------------------------------
inline void MotionState::Set(MotionEventID motionEvent, const ROSString& motionName, Time startTime, Time transition)
{
	SetMotionEvent(motionEvent);
    SetStartTime(startTime);
	SetMotionName(motionName);
    SetTransitionTime(transition);
}
// --------------------------------------------------------------------------
inline void MotionState::SetIKRecordIndex(unsigned int index)
{
	mIKRecordIndex = index;
}
// --------------------------------------------------------------------------
inline MotionState::MotionEventID MotionState::GetMotionEvent() const
{
	return mMotionEventID;
}
// --------------------------------------------------------------------------
inline Time MotionState::GetStartTime() const
{
	return mStartTime;
}
// --------------------------------------------------------------------------
inline ROSString MotionState::GetMotionName() const
{
	return mMotionName;
}
// --------------------------------------------------------------------------
inline Time MotionState::GetTransitionTime() const
{
	return mTransitionTime;
}
// --------------------------------------------------------------------------
inline unsigned int MotionState::GetIKRecordIndex() const
{
	return mIKRecordIndex;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::MotionState& state)
{
	state.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::MotionState& state)
{
	state.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif