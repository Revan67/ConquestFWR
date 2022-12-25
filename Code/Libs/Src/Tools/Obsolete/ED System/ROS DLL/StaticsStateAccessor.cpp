// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "StaticsStateAccessor.h"
#include "AStaticsState.h"
#include "AStaticSceneEntity.h"
#include "Update.h"
// --------------------------------------------------------------------------
/**# implementation StaticsStateAccessor:: id(C_0892741551)
*/
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN Location StaticsStateAccessor::GetLocation() const
{
    return mState.GetPosition().GetLocation();
}
// --------------------------------------------------------------------------
CPP_DEFN Orientation StaticsStateAccessor::GetOrientation() const
{
    return mState.GetPosition().GetOrientation();
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsStateAccessor::SetLocation(const Location& location)
{
    Position pos = mState.GetPosition();
    pos.SetLocation(location);
    mState.SetPosition(pos);

    OwnerStateUpdated(Update::kLocation);
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsStateAccessor::SetOrientation(const Orientation& orientation)
{
    Position pos = mState.GetPosition();
    pos.SetOrientation(orientation);
    mState.SetPosition(pos);

	OwnerStateUpdated(Update::kOrientation);
}
// --------------------------------------------------------------------------
CPP_DEFN OrientationMemento StaticsStateAccessor::GetOrientationMemento(Time time) const 
{ 
	return mOwner.GetOrientationMemento(time); 
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsStateAccessor::SetOrientationMemento(const OrientationMemento& memento) 
{ 
	mOwner.SetOrientationMemento(memento); 
}
// --------------------------------------------------------------------------
CPP_DEFN LocationMemento StaticsStateAccessor::GetLocationMemento(Time time) const 
{ 
	return mOwner.GetLocationMemento(time); 
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsStateAccessor::SetLocationMemento(const LocationMemento& memento) 
{ 
	mOwner.SetLocationMemento(memento); 
}
// --------------------------------------------------------------------------
CPP_DEFN Location StaticsStateAccessor::GetLocation(Time time) const
{
	return mOwner.GetLocation(time);
}
// --------------------------------------------------------------------------
CPP_DEFN Orientation StaticsStateAccessor::GetOrientation(Time time) const
{
	return mOwner.GetOrientation(time);
}
// --------------------------------------------------------------------------
CPP_DEFN Time StaticsStateAccessor::GetLocationTime(unsigned int keyPointIndex) const
{
	return mOwner.GetLocationTime(keyPointIndex);
}
// --------------------------------------------------------------------------
CPP_DEFN Time StaticsStateAccessor::GetOrientationTime(unsigned int keyPointIndex) const
{
	return mOwner.GetOrientationTime(keyPointIndex);
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsStateAccessor::SetLocation(const Location& location, Time time)
{
	mOwner.SetLocation(location, time);
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsStateAccessor::SetOrientation(const Orientation& orientation, Time time)
{
	mOwner.SetOrientation(orientation, time);
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsStateAccessor::RemoveLocation(Time time)
{
	mOwner.RemoveLocation(time);
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsStateAccessor::RemoveOrientation(Time time)
{
	mOwner.RemoveOrientation(time);
}
// --------------------------------------------------------------------------
CPP_DEFN bool StaticsStateAccessor::IsAtKeyLocation() const
{
	return mOwner.IsAtKeyLocation();
}
// --------------------------------------------------------------------------
CPP_DEFN bool StaticsStateAccessor::IsAtKeyOrientation() const
{
	return mOwner.IsAtKeyOrientation();
}
// --------------------------------------------------------------------------
CPP_DEFN StaticsStateAccessor::InterpolationType StaticsStateAccessor::GetLocationInterpolationType(Time time) const
{
	const AStaticSceneEntity::InterpolationType	type = mOwner.GetLocationInterpolationType(time);

	switch(type)
	{
		case AStaticSceneEntity::kLinearFixed:
			return kLinearFixed;
		case AStaticSceneEntity::kSplineFixed:
			return kSplineFixed;
		case AStaticSceneEntity::kLinearBlend:
			return kLinearBlend;
		case AStaticSceneEntity::kSplineBlend:
			return kSplineBlend;
		default:
			ASSERT(0);	// Unknown type!
			return kLinearFixed;	// Just for the compiler's pleasure
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsStateAccessor::SetStaticsPathVisible(bool visible)
{
	mOwner.SetStaticsPathVisible(visible);
}
// --------------------------------------------------------------------------
CPP_DEFN bool StaticsStateAccessor::IsStaticsPathVisible() const
{
	return mOwner.IsStaticsPathVisible();
}
// --------------------------------------------------------------------------
CPP_DEFN AStaticSceneEntity& StaticsStateAccessor::GetOwner()
{
	return mOwner;
}
// --------------------------------------------------------------------------
CPP_DEFN const AStaticSceneEntity& StaticsStateAccessor::GetOwner() const
{
	return mOwner;
}
// --------------------------------------------------------------------------
CPP_DEFN AStaticsState& StaticsStateAccessor::GetState()
{
	return mState;
}
// --------------------------------------------------------------------------
CPP_DEFN const AStaticsState& StaticsStateAccessor::GetState() const
{
	return mState;
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsStateAccessor::OwnerStateUpdated(Update::ID id)
{
    mOwner.StateUpdated(id);
}
// --------------------------------------------------------------------------
CPP_DEFN Location StaticsStateAccessor::GetLocationInWorld() const
{
	return mOwner.GetLocationInWorld();
}
// --------------------------------------------------------------------------
CPP_DEFN Orientation StaticsStateAccessor::GetOrientationInWorld() const
{
	return mOwner.GetOrientationInWorld();
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

