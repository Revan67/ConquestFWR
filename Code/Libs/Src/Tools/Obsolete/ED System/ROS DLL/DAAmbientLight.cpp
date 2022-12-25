// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <Windows.h>

#include "DAAmbientLight.h"
#include "DAAmbientLightState.h"
#include "Scene.h"
// --------------------------------------------------------------------------
enum FieldID
{
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
DAAmbientLight::DAAmbientLight(const ROSString& name, bool makeNameUnique, Scene& scene)
: BaseClass(name, makeNameUnique, scene, *new DAAmbientLightState)
{
}
#if 1
/*******FL GAMESTOCK HACK*********/
// --------------------------------------------------------------------------
DAAmbientLight::DAAmbientLight(Scene& scene)
: BaseClass(scene, NULL)
{
	ALightState*	lightState;
	if(GetSceneEntityState().GetScene().IsUsingInitialEntityState())
	{
		lightState = new DAAmbientLightState;
	    //OutputDebugString("Constructed DA Ambient Light\n");
	}
	else
	{
		lightState = new LightState;
	}

	SetLightState(*lightState);
}
#else
// --------------------------------------------------------------------------
DAAmbientLight::DAAmbientLight(Scene& scene)
: BaseClass(scene, *new DAAmbientLightState)
{
    //OutputDebugString("Constructed DA Ambient Light\n");
}
#endif
// --------------------------------------------------------------------------
ROSString DAAmbientLight::GetArchetypeName() const
{	
	return GetDAAmbientLightArchetypeName();
}
// --------------------------------------------------------------------------
ROSString DAAmbientLight::GetDAAmbientLightArchetypeName()
{    
	return "DA Ambient Light";
}
// --------------------------------------------------------------------------
void DAAmbientLight::Write(std::ostream& oStream) const
{
    BaseClass::Write(oStream);

    WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void DAAmbientLight::Read(std::istream& iStream)
{
    BaseClass::Read(iStream);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void DAAmbientLight::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);
}
// --------------------------------------------------------------------------
void DAAmbientLight::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
