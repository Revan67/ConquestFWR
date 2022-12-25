// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "SpotLightStateAccessor.h"
#include "ASpotLight.h"
#include "ASpotLightState.h"
#include "Update.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN SpotLightStateAccessor::SpotLightStateAccessor(ASpotLight& owner, ASpotLightState& state)
: BaseClass(owner, state)
{
}
// --------------------------------------------------------------------------
CPP_DEFN void SpotLightStateAccessor::SetInfinite(bool infinite) 
{
	ASpotLightState&	state = dynamic_cast<ASpotLightState&>(GetState());

	state.SetInfinite(infinite);

	OwnerStateUpdated(Update::kLightInfinite);
}
// --------------------------------------------------------------------------
CPP_DEFN void SpotLightStateAccessor::SetRange(float range)
{
	ASpotLightState&	state = dynamic_cast<ASpotLightState&>(GetState());

	state.SetRange(range);

	OwnerStateUpdated(Update::kLightRange);
}
// --------------------------------------------------------------------------
CPP_DEFN void SpotLightStateAccessor::SetCutOff(float cutOff)
{
	ASpotLightState&	state = dynamic_cast<ASpotLightState&>(GetState());

	state.SetCutOff(cutOff);

	OwnerStateUpdated(Update::kLightCutOff);
}
// --------------------------------------------------------------------------
CPP_DEFN bool SpotLightStateAccessor::IsInfinite() const
{
	const ASpotLightState&	state = dynamic_cast<const ASpotLightState&>(GetState());

	return state.IsInfinite();
}
// --------------------------------------------------------------------------
CPP_DEFN float SpotLightStateAccessor::GetRange() const
{
	const ASpotLightState&	state = dynamic_cast<const ASpotLightState&>(GetState());

	return state.GetRange();
}
// --------------------------------------------------------------------------
CPP_DEFN float SpotLightStateAccessor::GetCutOff() const
{
	const ASpotLightState&	state = dynamic_cast<const ASpotLightState&>(GetState());

	return state.GetCutOff();
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
