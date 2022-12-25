//---------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "MotionState.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kMotionEvent,
	kStartTime,
	kMotionName,
	kTransitionTime,
	kIKRecordIndex
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN void MotionState::Write(std::ostream& oStream) const
{
	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void MotionState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

    int motionEvent = mMotionEventID;

	oWiz.Put(kMotionEvent, motionEvent);
	oWiz.Put(kStartTime, mStartTime);
	oWiz.Put(kMotionName, mMotionName);
    oWiz.Put(kTransitionTime, mTransitionTime);
	oWiz.Put(kIKRecordIndex, mIKRecordIndex);
}
// --------------------------------------------------------------------------
CPP_DEFN void MotionState::Read(std::istream& iStream)
{
	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
inline void MotionState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

    int motionEvent;

	iWiz.Get(kMotionEvent, motionEvent);
    mMotionEventID = (MotionEventID)motionEvent;

	iWiz.Get(kStartTime, mStartTime);
	iWiz.Get(kMotionName, mMotionName);
    iWiz.Get(kTransitionTime, mTransitionTime);

	if(iWiz.Has(kIKRecordIndex))
	{
		iWiz.Get(kIKRecordIndex, mIKRecordIndex);
	}
}
// --------------------------------------------------------------------------
}
