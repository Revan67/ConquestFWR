// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ConstSpotLightStateAccessor.h"
#include "ASpotLightState.h"
#include "ASpotLight.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN ConstSpotLightStateAccessor::ConstSpotLightStateAccessor(const ASpotLight& owner, const ASpotLightState& state)
: BaseClass(owner, state)
{
}
// --------------------------------------------------------------------------
CPP_DEFN bool ConstSpotLightStateAccessor::IsInfinite() const
{
	const ASpotLightState&	state = dynamic_cast<const ASpotLightState&>(GetState());

	return state.IsInfinite();
}
// --------------------------------------------------------------------------
CPP_DEFN float ConstSpotLightStateAccessor::GetRange() const
{
	const ASpotLightState&	state = dynamic_cast<const ASpotLightState&>(GetState());

	return state.GetRange();
}
// --------------------------------------------------------------------------
CPP_DEFN float ConstSpotLightStateAccessor::GetCutOff() const
{
	const ASpotLightState&	state = dynamic_cast<const ASpotLightState&>(GetState());

	return state.GetCutOff();
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
