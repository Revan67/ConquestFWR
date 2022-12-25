// Author: Shaival Varma
//---------------------------------------------------------------------------
#include "PCH.h"
#include "ConstMotionStateAccessor.h"
#include "ACompoundSceneEntity.h"
#include "ACompoundSceneEntityState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN ConstMotionStateAccessor::ConstMotionStateAccessor(const ACompoundSceneEntity& owner, const ACompoundSceneEntityState& state)
: mOwner(owner), mState(state)
{
}
// --------------------------------------------------------------------------
CPP_DEFN bool ConstMotionStateAccessor::IsSkeletonShowing() const
{
	return mOwner.IsSkeletonShowing(); 
}
// --------------------------------------------------------------------------
CPP_DEFN bool ConstMotionStateAccessor::AreHardPointsShowing() const 
{ 
	return mOwner.AreHardPointsShowing(); 
}
// --------------------------------------------------------------------------
CPP_DEFN unsigned int ConstMotionStateAccessor::GetHardPointCount() const 
{ 
	return mOwner.GetHardPointCount(); 
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString ConstMotionStateAccessor::GetHardPointName(unsigned int idx) const 
{ 
	return mOwner.GetHardPointName(idx);
}
// --------------------------------------------------------------------------
CPP_DEFN Location ConstMotionStateAccessor::GetHardPointLocation(unsigned int idx) const 
{
	return mOwner.GetHardPointLocation(idx); 
}
// --------------------------------------------------------------------------
CPP_DEFN Orientation ConstMotionStateAccessor::GetHardPointOrientation(unsigned int idx) const 
{ 
	return mOwner.GetHardPointOrientation(idx); 
}
// --------------------------------------------------------------------------
CPP_DEFN const HardPointHost* ConstMotionStateAccessor::GetHardPointHost(unsigned int idx) const 
{ 
	return mOwner.GetHardPointHost(idx); 
}
// --------------------------------------------------------------------------
CPP_DEFN unsigned int ConstMotionStateAccessor::GetMotionCount() const
{
	return GetOwner().GetMotionCount();
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString ConstMotionStateAccessor::GetMotionName(int motionIdx) const
{
	return GetOwner().GetMotionName(motionIdx);
}
// --------------------------------------------------------------------------
CPP_DEFN Time ConstMotionStateAccessor::GetMotionLength(const ROSString& motionName) const
{
	return GetOwner().GetMotionLength(motionName);
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString ConstMotionStateAccessor::GetCurrentMotionName() const
{
	return GetOwner().GetCurrentMotionName();
}
// --------------------------------------------------------------------------
CPP_DEFN Time ConstMotionStateAccessor::GetCurrentMotionTime() const
{
	return GetOwner().GetCurrentMotionTime();
}
// --------------------------------------------------------------------------
CPP_DEFN IKState ConstMotionStateAccessor::GetIKState(Time startTime) const
{
	return GetOwner().GetIKState(startTime);
}
// --------------------------------------------------------------------------
CPP_DEFN long ConstMotionStateAccessor::GetRootEngineIndex() const
{
	return GetOwner().GetRootEngineIndex();
}
// --------------------------------------------------------------------------
CPP_DEFN const ACompoundSceneEntity& ConstMotionStateAccessor::GetOwner() const
{
	return mOwner;
}
// --------------------------------------------------------------------------
CPP_DEFN const ACompoundSceneEntityState& ConstMotionStateAccessor::GetState() const
{
	return mState;
}
//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------

