// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ConstStaticsStateAccessor.h"
#include "AStaticsState.h"
#include "AStaticSceneEntity.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN ConstStaticsStateAccessor::ConstStaticsStateAccessor(const AStaticSceneEntity& owner, const AStaticsState& state)
: mOwner(owner), mState(state)
{
}
// --------------------------------------------------------------------------
CPP_DEFN Location ConstStaticsStateAccessor::GetLocation() const
{
	return mState.GetPosition().GetLocation();
}
// --------------------------------------------------------------------------
CPP_DEFN Orientation ConstStaticsStateAccessor::GetOrientation() const
{
	return mState.GetPosition().GetOrientation();
}
// --------------------------------------------------------------------------
CPP_DEFN OrientationMemento ConstStaticsStateAccessor::GetOrientationMemento(Time time) const
{
	return mOwner.GetOrientationMemento(time);
}
// --------------------------------------------------------------------------
CPP_DEFN LocationMemento ConstStaticsStateAccessor::GetLocationMemento(Time time) const
{
	return mOwner.GetLocationMemento(time);
}
// --------------------------------------------------------------------------
CPP_DEFN Time ConstStaticsStateAccessor::GetLocationTime(unsigned int keyPointIndex) const
{
	return mOwner.GetLocationTime(keyPointIndex);
}
// --------------------------------------------------------------------------
CPP_DEFN Time ConstStaticsStateAccessor::GetOrientationTime(unsigned int keyPointIndex) const
{
	return mOwner.GetOrientationTime(keyPointIndex);
}
// --------------------------------------------------------------------------
CPP_DEFN Location ConstStaticsStateAccessor::GetLocation(Time time) const
{
	return mOwner.GetLocation(time);
}
// --------------------------------------------------------------------------
CPP_DEFN Orientation ConstStaticsStateAccessor::GetOrientation(Time time) const
{
	return mOwner.GetOrientation(time);
}
// --------------------------------------------------------------------------
CPP_DEFN ConstStaticsStateAccessor::InterpolationType ConstStaticsStateAccessor::GetLocationInterpolationType(Time time) const
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
CPP_DEFN bool ConstStaticsStateAccessor::IsStaticsPathVisible() const
{
	return mOwner.IsStaticsPathVisible();
}
// --------------------------------------------------------------------------
CPP_DEFN Location ConstStaticsStateAccessor::GetLocationInWorld() const
{
	return mOwner.GetLocationInWorld();
}
// --------------------------------------------------------------------------
CPP_DEFN Orientation ConstStaticsStateAccessor::GetOrientationInWorld() const
{
	return mOwner.GetOrientationInWorld();
}
// --------------------------------------------------------------------------
CPP_DEFN const AStaticsState& ConstStaticsStateAccessor::GetState() const
{
	return mState;
}
// --------------------------------------------------------------------------
CPP_DEFN const AStaticSceneEntity& ConstStaticsStateAccessor::GetOwner() const
{
	return mOwner;
}
// --------------------------------------------------------------------------
}
