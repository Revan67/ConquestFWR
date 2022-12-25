// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ALightState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
ALightState::~ALightState()
{
}
// --------------------------------------------------------------------------
void ALightState::SetLightState(const ALightState& light)
{
	SetColor(light.GetColor());
}
// --------------------------------------------------------------------------
void ALightState::Interpolate(const ALightState& nextState, float t, ALightState& tState) const
{
	tState.SetColor(GetColor().Interpolate(nextState.GetColor(), t));
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
