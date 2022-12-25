// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <Windows.h>

#include "AmbientLight.h"
#include "LightState.h"
#include "LightRole.h"
#include "Scene.h"
#include "SceneEntityState.h"
// --------------------------------------------------------------------------
/**# implementation AmbientLight:: id(C_0886794282) 
*/
// --------------------------------------------------------------------------
enum FieldID
{
	kSceneEntityState
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
AmbientLight::AmbientLight(const ROSString& name, bool makeNameUnique, Scene& scene, ALightState& lightState)
: mSceneEntityState(*this, name, makeNameUnique)
, mLightState(&lightState)
{
	mSceneEntityState.SetScene(&scene);
	mLightRoleIndex = mSceneEntityState.AddRole(*new LightRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	
	InitializeRole();
	
	Goto(scene.GetCurrentTimePoint());
}
#if 1
/*****FL GAMESTOCK HACK*********/
// --------------------------------------------------------------------------
AmbientLight::AmbientLight(Scene& scene, ALightState* lightState)
: mSceneEntityState(*this, "Ambient Light", false)
, mLightState(lightState)
{
	mLightRoleIndex = mSceneEntityState.AddRole(*new LightRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));

	mSceneEntityState.SetScene(&scene);
}
#else
// --------------------------------------------------------------------------
AmbientLight::AmbientLight(Scene& scene, ALightState& lightState)
: mSceneEntityState(*this, "Ambient Light", false)
, mLightState(&lightState)
{
	mLightRoleIndex = mSceneEntityState.AddRole(*new LightRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));

	mSceneEntityState.SetScene(&scene);
}
#endif
// --------------------------------------------------------------------------
void AmbientLight::Delete()
{
	BaseClass::Delete();

	delete this;
}
// --------------------------------------------------------------------------
void AmbientLight::SetLightState(ALightState& lightState)
{
	ASSERT(IsNull(mLightState));

	mLightState = AggAPointer<ALightState>(&lightState);
}
// --------------------------------------------------------------------------
ROSString AmbientLight::GetArchetypeName() const
{	
	return GetAmbientLightArchetypeName();
}
// --------------------------------------------------------------------------
ROSString AmbientLight::GetAmbientLightArchetypeName()
{    
	return "Ambient Light";
}
// --------------------------------------------------------------------------
void AmbientLight::StateUpdated(Update::ID id, Time time)
{
	const int	lightRoleIndex = GetLightRoleIndex();

	if(lightRoleIndex >= 0)
	{
		ARole* aRole = &GetSceneEntityState().GetRole(lightRoleIndex);

		LightRole*   lRole = dynamic_cast<LightRole*>(aRole);

		if(lRole)
		{   
			lRole->StateUpdated(*mLightState, time);
		}
	}
}
// --------------------------------------------------------------------------
void AmbientLight::InitializeRole()
{
	const int	lightRoleIndex = GetLightRoleIndex();

	if(lightRoleIndex >= 0)
	{
		ARole*	aRole = &GetSceneEntityState().GetRole(lightRoleIndex);

		aRole->SetName("Color");
    
		LightRole*	lRole = dynamic_cast<LightRole*>(aRole);
		ASSERT(lRole);

		if(!GetSceneEntityState().GetScene().IsUsingInitialEntityState())
		{
			lRole->StateUpdated(*mLightState, Time(0));
		}
	}
}
// --------------------------------------------------------------------------
void AmbientLight::Write(std::ostream& oStream) const
{
    BaseClass::Write(oStream);

    WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void AmbientLight::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);
	
	ReadSubObject(iStream);

	mSceneEntityState.SetVisible(true);	// Force this entity visible

	InitializeRole();
}
// --------------------------------------------------------------------------
void AmbientLight::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);
	
	oWiz.Put(kSceneEntityState, mSceneEntityState);

	// Don't wannna write (or read) the light state
	// mLightState->Write(oStream);
}
// --------------------------------------------------------------------------
void AmbientLight::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	iWiz.Get(kSceneEntityState, mSceneEntityState);

	// Don't wannna read (or write) the light state
    // mLightState->Read(iStream);
}
// --------------------------------------------------------------------------
SceneEntityState& AmbientLight::GetSceneEntityState()
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
const SceneEntityState& AmbientLight::GetSceneEntityState() const
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
APhysicalState& AmbientLight::GetPhysicalState()
{
	return *mLightState;
}
// --------------------------------------------------------------------------
const APhysicalState& AmbientLight::GetPhysicalState() const
{
	return *mLightState;
}
// --------------------------------------------------------------------------
int AmbientLight::GetLightRoleIndex() const
{
	return mLightRoleIndex;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
