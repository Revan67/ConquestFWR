// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <float.h>
#include "ASpotLightState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
ASpotLightState::ASpotLightState()
{
}
// --------------------------------------------------------------------------
ASpotLightState::ASpotLightState(const ASpotLightState& lightState)
: BaseClass(lightState)
{
}
// --------------------------------------------------------------------------
void ASpotLightState::SetSpotLightState(const ASpotLightState& lightState)
{
	SetLightState(lightState);

	SetInfinite(lightState.IsInfinite());
	
	SetRange(lightState.GetRange());
	
	SetCutOff(lightState.GetCutOff());
}
// --------------------------------------------------------------------------
void ASpotLightState::Interpolate(const ASpotLightState& nextState, float t, ASpotLightState& tState) const
{
	BaseClass::Interpolate(nextState, t, tState);

	const diff = 1 - t;

	// Infinite
	tState.SetInfinite(IsInfinite());
		
	// Range
	tState.SetRange(GetRange() * t + nextState.GetRange() * diff);
	
	// Cutoff
	tState.SetCutOff(GetCutOff() * t + nextState.GetCutOff() * diff);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
