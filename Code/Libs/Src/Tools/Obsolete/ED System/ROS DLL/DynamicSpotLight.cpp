// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "DynamicSpotLight.h"
#include "ADynamicSpotLightState.h"
#include "LocationRole.h"
#include "OrientationRole.h"
#include "SpotLightRole.h"
#include "DynamicsRole.h"
#include "AudioRole.h"
#include "Matrix.h"
#include "xform.h"
#include "Char.h"
#include "Scene.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kSceneEntityState
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
DynamicSpotLight::DynamicSpotLight(const ROSString& name, bool makeNameUnique, Scene& scene, ADynamicSpotLightState& lightState)
: mSceneEntityState(*this, name, makeNameUnique), mLightState(lightState)
{
	LocationRole*	locationRole = new LocationRole(true, Interpolate, Interpolate, GetStateName, NULL);
	
	mLocationRoleIndex = mSceneEntityState.AddRole(*locationRole);
	mOrientationRoleIndex = mSceneEntityState.AddRole(*new OrientationRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL, locationRole));
	mSpotLightRoleIndex = mSceneEntityState.AddRole(*new SpotLightRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	mDynamicRoleIndex = mSceneEntityState.AddRole(*new DynamicsRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	mAudioRoleIndex = mSceneEntityState.AddRole(*new AudioRole(false, Interpolate, Interpolate, GetStateName, NULL));

	mSceneEntityState.SetScene(&scene);

	InitializeLocationRole();
	InitializeOrientationRole();
	InitializeSpotLightRole();
	InitializeDynamicRole();
    InitializeAudioRole();

	Goto(scene.GetCurrentTimePoint());
}
// --------------------------------------------------------------------------
DynamicSpotLight::DynamicSpotLight(Scene& scene, ADynamicSpotLightState& lightState)
: mSceneEntityState(*this, "Dynamic Spot Light", false), mLightState(lightState)
{
	LocationRole*	locationRole = new LocationRole(true, Interpolate, Interpolate, GetStateName, NULL);

	mLocationRoleIndex = mSceneEntityState.AddRole(*locationRole);
	mOrientationRoleIndex = mSceneEntityState.AddRole(*new OrientationRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL, locationRole));
	mSpotLightRoleIndex = mSceneEntityState.AddRole(*new SpotLightRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	mDynamicRoleIndex = mSceneEntityState.AddRole(*new DynamicsRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	mAudioRoleIndex = mSceneEntityState.AddRole(*new AudioRole(false, Interpolate, Interpolate, GetStateName, NULL));

	mSceneEntityState.SetScene(&scene);
}
// --------------------------------------------------------------------------
void DynamicSpotLight::Delete()
{
	BaseClass::Delete();

	delete this;
}
// --------------------------------------------------------------------------
DynamicSpotLight::~DynamicSpotLight()
{
	delete &mLightState;
}
// --------------------------------------------------------------------------
ROSString DynamicSpotLight::GetArchetypeName() const
{
	return GetDynamicSpotLightArchetypeName();
}
// --------------------------------------------------------------------------
ROSString DynamicSpotLight::GetDynamicSpotLightArchetypeName()
{
	return "DynamicSpotLight";
}
// --------------------------------------------------------------------------
void DynamicSpotLight::SetupPosition() const
{
    // This method is a noop on purpose, since DynamicSpotLight::Render() takes care of transforms
}
// --------------------------------------------------------------------------
void DynamicSpotLight::Render(const ROS::DABaseCamera* camera) const
{
	std::auto_ptr<ConstStaticsStateAccessor>	accessor = GetConstStaticsStateAccessor();

	const Location	location = accessor->GetLocation();
	const Vector	vector(location.GetX(), location.GetY(), location.GetZ());

	const Orientation	orientation = accessor->GetOrientation();
	const ::Matrix		matrix(orientation.GetI(), orientation.GetJ(), orientation.GetK());

	const Transform	transform(matrix, vector);

	CharRenderLight(transform, camera);
}
// --------------------------------------------------------------------------
bool DynamicSpotLight::FindIntersect(const IntersectInfo& intersectInfo, float* distance) const
{
	return false;
}
// --------------------------------------------------------------------------
SceneEntityState& DynamicSpotLight::GetSceneEntityState()
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
const SceneEntityState& DynamicSpotLight::GetSceneEntityState() const
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
APhysicalState& DynamicSpotLight::GetPhysicalState()
{
	return mLightState;
}
// --------------------------------------------------------------------------
const APhysicalState& DynamicSpotLight::GetPhysicalState() const
{
	return mLightState;
}
// --------------------------------------------------------------------------
void DynamicSpotLight::Write(std::ostream& oStream) const
{
	BaseClass::Write(oStream);

	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void DynamicSpotLight::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	ReadSubObject(iStream);

	mSceneEntityState.SetVisible(true);	// Force this entity visible

	InitializeLocationRole();
	InitializeOrientationRole();
	InitializeSpotLightRole();
	InitializeDynamicRole();
    InitializeAudioRole();
}
// --------------------------------------------------------------------------
void DynamicSpotLight::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	oWiz.Put(kSceneEntityState, mSceneEntityState);
}
// --------------------------------------------------------------------------
void DynamicSpotLight::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	iWiz.Get(kSceneEntityState, mSceneEntityState);
}
// --------------------------------------------------------------------------
void DynamicSpotLight::InitializeLocationRole()
{
	const int	locationRoleIndex = GetLocationRoleIndex();
    ASSERT(locationRoleIndex >= 0);

	ARole* aRole = &mSceneEntityState.GetRole(locationRoleIndex);

    aRole->SetName("Location");
    
    LocationRole*	lRole = dynamic_cast<LocationRole*>(aRole);
    ASSERT(lRole);

    if(!mSceneEntityState.GetScene().IsUsingInitialEntityState())
    {
		lRole->StateUpdated(FlaggedLocation(mLightState.GetPosition().GetLocation(), lRole->GetState(Time(0)).GetInterpolationType()), Time(0));
    }
}
// --------------------------------------------------------------------------
void DynamicSpotLight::InitializeOrientationRole()
{
	const int	orientationRoleIndex = GetOrientationRoleIndex();
	ASSERT(orientationRoleIndex >= 0);

    ARole* aRole = &mSceneEntityState.GetRole(orientationRoleIndex);

    aRole->SetName("Orientation");
    
    OrientationRole*	oRole = dynamic_cast<OrientationRole*>(aRole);
    ASSERT(oRole);

    if(!mSceneEntityState.GetScene().IsUsingInitialEntityState())
    {
		const FlaggedOrientation	currentOrientation = oRole->GetState(Time(0));

		oRole->StateUpdated(FlaggedOrientation(mLightState.GetPosition().GetOrientation(), currentOrientation.GetInterpolationType(), currentOrientation.GetTargetEntity()), Time(0));
    }
}
// --------------------------------------------------------------------------
void DynamicSpotLight::InitializeSpotLightRole()
{
	const int	spotLightRoleIndex = GetSpotLightRoleIndex();
	ASSERT(spotLightRoleIndex >= 0);

    ARole* aRole = &mSceneEntityState.GetRole(spotLightRoleIndex);

    aRole->SetName("Light");
    
    SpotLightRole*	sRole = dynamic_cast<SpotLightRole*>(aRole);
    ASSERT(sRole);

    if(!mSceneEntityState.GetScene().IsUsingInitialEntityState())
    {
		const SpotLightState	state(mLightState);

		sRole->StateUpdated(state, Time(0));
    }
}
// --------------------------------------------------------------------------
void DynamicSpotLight::InitializeDynamicRole()
{
	const int	dynamicRoleIndex = GetDynamicRoleIndex();
	ASSERT(dynamicRoleIndex >= 0);

    ARole* aRole = &mSceneEntityState.GetRole(dynamicRoleIndex);

    aRole->SetName("Dynamics");
    
    DynamicsRole*	dRole = dynamic_cast<DynamicsRole*>(aRole);
    ASSERT(dRole);

    if(!mSceneEntityState.GetScene().IsUsingInitialEntityState())
    {
		dRole->StateUpdated(mLightState, Time(0));
    }
}
// --------------------------------------------------------------------------
void DynamicSpotLight::InitializeAudioRole()
{
	const int	audioRoleIndex = GetAudioRoleIndex();
	ASSERT(audioRoleIndex >= 0);
    ARole&		aRole = mSceneEntityState.GetRole(audioRoleIndex);

    aRole.SetName("Audio");
}
// --------------------------------------------------------------------------
int DynamicSpotLight::GetLocationRoleIndex() const
{
	return mLocationRoleIndex;
}
// --------------------------------------------------------------------------
int DynamicSpotLight::GetOrientationRoleIndex() const
{
	return mOrientationRoleIndex;
}
// --------------------------------------------------------------------------
int DynamicSpotLight::GetDynamicRoleIndex() const
{
	return mDynamicRoleIndex;
}
// --------------------------------------------------------------------------
int DynamicSpotLight::GetSpotLightRoleIndex() const
{
	return mSpotLightRoleIndex;
}
// --------------------------------------------------------------------------
int DynamicSpotLight::GetAudioRoleIndex() const
{
	return mAudioRoleIndex;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
