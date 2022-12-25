// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ADynamicSpotLight.h"
#include "CodeMsg.h"
#include "SceneEntityState.h"
#include "SpotLightRole.h"
// --------------------------------------------------------------------------
namespace ROS
{
CPP_DEFN ADynamicSpotLight::ADynamicSpotLight()
{
}
// --------------------------------------------------------------------------
CPP_DEFN ADynamicSpotLight::~ADynamicSpotLight()
{
}
// --------------------------------------------------------------------------
void ADynamicSpotLight::Goto(Time time)
{
	ADynamicSceneEntityBaseClass::Goto(time);
	ASpotLightBaseClass::Goto(time);
}
// --------------------------------------------------------------------------
void ADynamicSpotLight::StateUpdated(Update::ID id, Time time)
{
	ADynamicSceneEntityBaseClass::StateUpdated(id, time);
	ASpotLightBaseClass::StateUpdated(id, time);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
