// Author: Shaival Varma
//---------------------------------------------------------------------------
#include "PCH.h"
#include "MotionStateAccessor.h"
#include "ACompoundSceneEntity.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN MotionStateAccessor::MotionStateAccessor(ACompoundSceneEntity& owner, AStaticsState& state)
: mOwner(owner), mState(state)
{
}
// --------------------------------------------------------------------------
CPP_DEFN void MotionStateAccessor::ShowHardPoints(bool show)
{
	mOwner.ShowHardPoints(show); 
}
// --------------------------------------------------------------------------
CPP_DEFN void MotionStateAccessor::ShowSkeleton(bool show)
{
	mOwner.ShowSkeleton(show); 
}
// --------------------------------------------------------------------------
CPP_DEFN unsigned int MotionStateAccessor::GetHardPointCount() const 
{ 
	return mOwner.GetHardPointCount(); 
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString MotionStateAccessor::GetHardPointName(unsigned int idx) const 
{ 
	return mOwner.GetHardPointName(idx); 
}
// --------------------------------------------------------------------------
CPP_DEFN Location MotionStateAccessor::GetHardPointLocation(unsigned int idx) const 
{ 
	return mOwner.GetHardPointLocation(idx); 
}
// --------------------------------------------------------------------------
CPP_DEFN Orientation MotionStateAccessor::GetHardPointOrientation(unsigned int idx) const 
{ 
	return mOwner.GetHardPointOrientation(idx); 
}
// --------------------------------------------------------------------------
CPP_DEFN void MotionStateAccessor::AttachHardPointToParent(unsigned int hardPointIndex, const HardPoint& parent)
{
	mOwner.AttachHardPointToParent(hardPointIndex, parent);
}
// --------------------------------------------------------------------------
CPP_DEFN void MotionStateAccessor::Start(const ROSString& motionName, Time startTime, Time transition)
{
	GetOwner().Start(motionName, startTime, transition);
}
// --------------------------------------------------------------------------
CPP_DEFN void MotionStateAccessor::Loop(const ROSString& motionName, Time startTime, Time transition)
{
	GetOwner().Loop(motionName, startTime, transition);
}
// --------------------------------------------------------------------------
CPP_DEFN void MotionStateAccessor::Pause(const ROSString& motionName)
{
	GetOwner().Pause(motionName);
}
// --------------------------------------------------------------------------
CPP_DEFN void MotionStateAccessor::Resume(const ROSString& motionName)
{
	GetOwner().Resume(motionName);
}
// --------------------------------------------------------------------------
CPP_DEFN void MotionStateAccessor::Stop(const ROSString& motionName)
{
	GetOwner().Stop(motionName);
}
// --------------------------------------------------------------------------
CPP_DEFN void MotionStateAccessor::StartIK(const ROSString& endEffectorName, unsigned int countToRootEffector, AStaticSceneEntity& targetEntity, Time transition)
{
	GetOwner().StartIK(endEffectorName, countToRootEffector, targetEntity, transition);
}
// --------------------------------------------------------------------------
CPP_DEFN IKState MotionStateAccessor::GetIKState(Time startTime) const
{
	return GetOwner().GetIKState(startTime);
}
// --------------------------------------------------------------------------
CPP_DEFN void MotionStateAccessor::SetIKState(const IKState& iKState, Time startTime)
{
	GetOwner().SetIKState(iKState, startTime);
}
// --------------------------------------------------------------------------
CPP_DEFN unsigned int MotionStateAccessor::GetMotionCount() const
{
	return GetOwner().GetMotionCount();
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString MotionStateAccessor::GetMotionName(unsigned int motionIdx) const
{
	return GetOwner().GetMotionName(motionIdx);
}
// --------------------------------------------------------------------------
CPP_DEFN Time MotionStateAccessor::GetMotionLength(const ROSString& motionName) const
{
	return GetOwner().GetMotionLength(motionName);
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString MotionStateAccessor::GetCurrentMotionName() const
{
	return GetOwner().GetCurrentMotionName();
}
// --------------------------------------------------------------------------
CPP_DEFN void MotionStateAccessor::SetCurrentMotionName(const ROSString& motionName)
{
	GetOwner().SetCurrentMotionName(motionName);
}
// --------------------------------------------------------------------------
CPP_DEFN Time MotionStateAccessor::GetCurrentMotionTime() const
{
	return GetOwner().GetCurrentMotionTime();
}

// --------------------------------------------------------------------------
CPP_DEFN long MotionStateAccessor::GetRootEngineIndex() const
{
	return GetOwner().GetRootEngineIndex();
}

#if 0
// --------------------------------------------------------------------------
CPP_DEFN void MotionStateAccessor::SetTimeTags(const ROSString& motionName, const TimeTagList& timeTagList)
{
	GetOwner().SetTimeTags(motionName, timeTagList);
}
#endif
// --------------------------------------------------------------------------
CPP_DEFN ACompoundSceneEntity& MotionStateAccessor::GetOwner()
{
	return mOwner;
}
// --------------------------------------------------------------------------
CPP_DEFN const ACompoundSceneEntity& MotionStateAccessor::GetOwner() const
{
	return mOwner;
}
// --------------------------------------------------------------------------
CPP_DEFN AStaticsState& MotionStateAccessor::GetState()
{
	return mState;
}
// --------------------------------------------------------------------------
CPP_DEFN const AStaticsState& MotionStateAccessor::GetState() const
{
	return mState;
}
// --------------------------------------------------------------------------
CPP_DEFN void MotionStateAccessor::OwnerStateUpdated(Update::ID id)
{
    mOwner.StateUpdated(id);
}
//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------

