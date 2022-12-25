// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ALight.h"
#include "LightRole.h"
#include "SceneEntityState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
ROSString ALight::GetArchetypeName() const
{
	return GetALightArchetypeName();
}
// --------------------------------------------------------------------------
ROSString ALight::GetALightArchetypeName()
{
	return "ALight";
}
// --------------------------------------------------------------------------
ALight::ALight()
{
}
// --------------------------------------------------------------------------
void ALight::Goto(Time time)
{
	BaseClass::Goto(time);

	GotoForLightRole(time);
}
// --------------------------------------------------------------------------
void ALight::GotoForLightRole(Time time)
{
    const int	lightRoleIndex = GetLightRoleIndex();
		
	if(lightRoleIndex >= 0)
	{
		ARole* aRole = &GetSceneEntityState().GetRole(lightRoleIndex);

		LightRole* lRole = dynamic_cast<LightRole*>(aRole);
		ASSERT(lRole);

		APhysicalState&	pState = GetPhysicalState();
		ALightState*	lState = dynamic_cast<ALightState*>(&pState);
		ASSERT(lState);

		lState->SetLightState(lRole->GetState(time));
	}
}
// --------------------------------------------------------------------------
std::auto_ptr<LightStateAccessor> ALight::GetLightStateAccessor()
{
	APhysicalState*	state = &GetPhysicalState();
	ALightState*	lState = dynamic_cast<ALightState*>(state);
	ASSERT(lState);

    return std::auto_ptr<LightStateAccessor>(new LightStateAccessor(*this, *lState));
}
// --------------------------------------------------------------------------
const std::auto_ptr<ConstLightStateAccessor> ALight::GetConstLightStateAccessor() const
{
	const APhysicalState*	state = &GetPhysicalState();
	const ALightState*		lState = dynamic_cast<const ALightState*>(state);
	ASSERT(lState);

    return std::auto_ptr<ConstLightStateAccessor>(new ConstLightStateAccessor(*this, *lState));
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
