// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "LightStateAccessor.h"
#include "ALight.h"
#include "ALightState.h"
#include "ROSDLL.h"
#include "Update.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
Color LightStateAccessor::GetColor() const
{
	return mState.GetColor();
}
// --------------------------------------------------------------------------
void LightStateAccessor::SetColor(const Color& color)
{
	mState.SetColor(color);

	OwnerStateUpdated(Update::kLightColor);
}
// --------------------------------------------------------------------------
ALight& LightStateAccessor::GetOwner()
{
	return mOwner;
}
// --------------------------------------------------------------------------
const ALight& LightStateAccessor::GetOwner() const
{
	return mOwner;
}
// --------------------------------------------------------------------------
ALightState& LightStateAccessor::GetState()
{
	return mState;
}
// --------------------------------------------------------------------------
const ALightState& LightStateAccessor::GetState() const
{
	return mState;
}
// --------------------------------------------------------------------------
void LightStateAccessor::OwnerStateUpdated(Update::ID id)
{
	mOwner.StateUpdated(id);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
