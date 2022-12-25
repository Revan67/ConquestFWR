// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ConstLightStateAccessor.h"
#include "ALightState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
ConstLightStateAccessor::ConstLightStateAccessor(const ALight& owner, const ALightState& state)
: mOwner(owner), mState(state)
{
}
// --------------------------------------------------------------------------
Color ConstLightStateAccessor::GetColor() const
{
	return mState.GetColor();
}
// --------------------------------------------------------------------------
const ALight& ConstLightStateAccessor::GetOwner() const
{
	return mOwner;
}
// --------------------------------------------------------------------------
const ALightState& ConstLightStateAccessor::GetState() const
{
	return mState;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
