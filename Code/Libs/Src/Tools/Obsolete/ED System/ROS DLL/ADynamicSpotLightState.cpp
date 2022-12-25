// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <float.h>
#include "ADynamicSpotLightState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
ADynamicSpotLightState::ADynamicSpotLightState()
{
}
// --------------------------------------------------------------------------
ADynamicSpotLightState::ADynamicSpotLightState(const ADynamicSpotLightState& lightState)
: ADynamicsStateBaseClass(lightState), ASpotLightStateBaseClass(lightState)
{
}
// --------------------------------------------------------------------------
void ADynamicSpotLightState::SetDynamicSpotLightState(const ADynamicSpotLightState& lightState)
{
	SetDynamicsState(lightState);
	SetSpotLightState(lightState);
}
// --------------------------------------------------------------------------
void ADynamicSpotLightState::Interpolate(const ADynamicSpotLightState& nextState, float t, ADynamicSpotLightState& tState) const
{
	ADynamicsStateBaseClass::Interpolate(nextState, t, tState);
	ASpotLightStateBaseClass::Interpolate(nextState, t, tState);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
