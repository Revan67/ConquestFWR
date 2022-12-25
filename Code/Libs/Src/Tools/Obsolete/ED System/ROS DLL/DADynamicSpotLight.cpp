// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "DADynamicSpotLight.h"
#include "DADynamicSpotLightState.h"
// --------------------------------------------------------------------------
enum FieldID
{
};
// --------------------------------------------------------------------------
namespace ROS
{
DADynamicSpotLight::DADynamicSpotLight(const ROSString& name, bool makeNameUnique, Scene& scene)
: BaseClass(name, makeNameUnique, scene, *new DADynamicSpotLightState())
{
}
// --------------------------------------------------------------------------
DADynamicSpotLight::DADynamicSpotLight(Scene& scene)
: BaseClass(scene, *new DADynamicSpotLightState())
{
}
// --------------------------------------------------------------------------
ROSString DADynamicSpotLight::GetArchetypeName() const
{
	return GetDADynamicSpotLightArchetypeName();
}
// --------------------------------------------------------------------------
ROSString DADynamicSpotLight::GetDADynamicSpotLightArchetypeName()
{
	return "DADynamicSpotLight";
}
// --------------------------------------------------------------------------
void DADynamicSpotLight::Write(std::ostream& oStream) const
{
	BaseClass::Write(oStream);

	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void DADynamicSpotLight::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void DADynamicSpotLight::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);
}
// --------------------------------------------------------------------------
void DADynamicSpotLight::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
