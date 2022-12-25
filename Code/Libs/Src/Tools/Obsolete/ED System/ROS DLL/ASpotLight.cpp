// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ASpotLight.h"
#include "CodeMsg.h"
#include "SceneEntityState.h"
#include "SpotLightRole.h"
#include "ConstSpotLightStateAccessor.h"
#include "SpotLightStateAccessor.h"
// --------------------------------------------------------------------------
namespace ROS
{
CPP_DEFN ASpotLight::ASpotLight()
{
}
// --------------------------------------------------------------------------
CPP_DEFN ASpotLight::~ASpotLight()
{
}
// --------------------------------------------------------------------------
const std::auto_ptr<ConstSpotLightStateAccessor> ASpotLight::GetConstSpotLightStateAccessor() const
{
	const APhysicalState*	state = &GetPhysicalState();
	const ASpotLightState*	sState = dynamic_cast<const ASpotLightState*>(state);
	ASSERT(sState);

	return std::auto_ptr<ConstSpotLightStateAccessor>(new ConstSpotLightStateAccessor(*this, *sState));
}
// --------------------------------------------------------------------------
std::auto_ptr<SpotLightStateAccessor> ASpotLight::GetSpotLightStateAccessor()
{
	APhysicalState*		state = &GetPhysicalState();
	ASpotLightState*	sState = dynamic_cast<ASpotLightState*>(state);
	ASSERT(sState);

	return std::auto_ptr<SpotLightStateAccessor>(new SpotLightStateAccessor(*this, *sState));
}
// --------------------------------------------------------------------------
void ASpotLight::Goto(Time time)
{
	GotoForSpotLightRole(time);
}
// --------------------------------------------------------------------------
void ASpotLight::GotoForSpotLightRole(Time time)
{
    const int spotightRoleIndex = GetSpotLightRoleIndex();

	if(spotightRoleIndex >= 0)
	{
		const ARole& aRole = GetSceneEntityState().GetRole(spotightRoleIndex);

		const SpotLightRole* sRole = dynamic_cast<const SpotLightRole*>(&aRole);

		ASSERT(sRole);

		SpotLightState	state = sRole->GetState(time);

		APhysicalState&	aState = GetPhysicalState();

		ASpotLightState*	sState = dynamic_cast<ASpotLightState*>(&aState);
		ASSERT(sState);
        
		sState->SetSpotLightState(*sState);
	}
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
