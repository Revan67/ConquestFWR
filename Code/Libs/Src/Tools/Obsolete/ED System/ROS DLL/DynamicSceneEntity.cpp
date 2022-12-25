// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <Memory>
#include "DynamicSceneEntity.h"
#include "Role.h"
#include "LocationRole.h"
#include "OrientationRole.h"
#include "DynamicsRole.h"
#include "AudioRole.h"
#include "SingularStateVariable.h"
#include "Utils.h"
#include "Scene.h"
// --------------------------------------------------------------------------
/**# implementation DynamicSceneEntity:: id(C_0890924565)
*/
// --------------------------------------------------------------------------
enum FieldID
{
	kSceneEntityState,
	kDynamicSceneEntityState
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
DynamicSceneEntity::DynamicSceneEntity (const ROSString& name, bool makeNameUnique, Scene& scene)
: mSceneEntityState(*this, name, makeNameUnique)
{
	LocationRole*	locationRole = new LocationRole(true, Interpolate, Interpolate, GetStateName, NULL);

	mLocationRoleIndex = mSceneEntityState.AddRole(*locationRole);
	mOrientationRoleIndex = mSceneEntityState.AddRole(*new OrientationRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL, locationRole));
	mDynamicRoleIndex = mSceneEntityState.AddRole(*new DynamicsRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	mAudioRoleIndex = mSceneEntityState.AddRole(*new AudioRole(false, Interpolate, Interpolate, GetStateName, NULL));

	mSceneEntityState.SetScene(&scene);

	InitializeLocationRole();
	InitializeOrientationRole();
	InitializeDynamicRole();
    InitializeAudioRole();
}
// --------------------------------------------------------------------------
DynamicSceneEntity::DynamicSceneEntity (Scene& scene)
: mSceneEntityState(*this, "Dynamic Scene Entity", false)
{
	LocationRole*	locationRole = new LocationRole(true, Interpolate, Interpolate, GetStateName, NULL);

	mLocationRoleIndex = mSceneEntityState.AddRole(*locationRole);
	mOrientationRoleIndex = mSceneEntityState.AddRole(*new OrientationRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL, locationRole));
	mDynamicRoleIndex = mSceneEntityState.AddRole(*new DynamicsRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	mAudioRoleIndex = mSceneEntityState.AddRole(*new AudioRole(false, Interpolate, Interpolate, GetStateName, NULL));

	mSceneEntityState.SetScene(&scene);
}
// --------------------------------------------------------------------------
void DynamicSceneEntity::Delete()
{
	ADynamicSceneEntityBaseClass::Delete();
	AAudibleSceneEntityBaseClass::Delete();	

	delete this;
}
// --------------------------------------------------------------------------
DynamicSceneEntity::~DynamicSceneEntity ()
{
}
// --------------------------------------------------------------------------
void DynamicSceneEntity::Write(std::ostream& oStream) const
{
	ADynamicSceneEntityBaseClass::Write(oStream);
	AAudibleSceneEntityBaseClass::Write(oStream);

	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void DynamicSceneEntity::Read (std::istream& iStream)
{
	ADynamicSceneEntityBaseClass::Read(iStream);
	AAudibleSceneEntityBaseClass::Read(iStream);

	ReadSubObject(iStream);

	mSceneEntityState.SetVisible(true);	// Force this entity visible

	InitializeLocationRole();
	InitializeOrientationRole();
	InitializeDynamicRole();
    InitializeAudioRole();
}
// --------------------------------------------------------------------------
void DynamicSceneEntity::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);
	
	oWiz.Put(kSceneEntityState, mSceneEntityState);
	oWiz.Put(kDynamicSceneEntityState, mDynamicSceneEntityState);
}
// --------------------------------------------------------------------------
void DynamicSceneEntity::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
	
	iWiz.Get(kSceneEntityState, mSceneEntityState);
	iWiz.Get(kDynamicSceneEntityState, mDynamicSceneEntityState);
}
// --------------------------------------------------------------------------
void DynamicSceneEntity::InitializeLocationRole()
{
	const int	locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
    ARole*		aRole = &mSceneEntityState.GetRole(locationRoleIndex);

    aRole->SetName("Location");

    LocationRole*	lRole = dynamic_cast<LocationRole*>(aRole);
    ASSERT(lRole);

    if(!mSceneEntityState.GetScene().IsUsingInitialEntityState())
    {
		lRole->StateUpdated(FlaggedLocation(mDynamicSceneEntityState.GetPosition().GetLocation(), lRole->GetState(Time(0)).GetInterpolationType()), Time(0));
    }
}
// --------------------------------------------------------------------------
void DynamicSceneEntity::InitializeOrientationRole()
{
	const int	orientationRoleIndex = GetOrientationRoleIndex();
	ASSERT(orientationRoleIndex >= 0);
    ARole*		aRole = &mSceneEntityState.GetRole(orientationRoleIndex);

    aRole->SetName("Orientation");

    OrientationRole*	oRole = dynamic_cast<OrientationRole*>(aRole);
    ASSERT(oRole);

    if(!mSceneEntityState.GetScene().IsUsingInitialEntityState())
    {
		const FlaggedOrientation	currentOrientation = oRole->GetState(Time(0));

		oRole->StateUpdated(FlaggedOrientation(mDynamicSceneEntityState.GetPosition().GetOrientation(), currentOrientation.GetInterpolationType(), currentOrientation.GetTargetEntity()), Time(0));
    }
}
// --------------------------------------------------------------------------
void DynamicSceneEntity::InitializeDynamicRole()
{
	const int	dynamicRoleIndex = GetDynamicRoleIndex();
	ASSERT(dynamicRoleIndex >= 0);
    ARole* aRole = &mSceneEntityState.GetRole(dynamicRoleIndex);

    aRole->SetName("Dynamics");

    DynamicsRole*   dRole = dynamic_cast<DynamicsRole*>(aRole);
    ASSERT(dRole);

    if(!mSceneEntityState.GetScene().IsUsingInitialEntityState())
    {
		const DynamicsState	dState(mDynamicSceneEntityState);

		dRole->StateUpdated(dState, Time(0));
    }
}
// --------------------------------------------------------------------------
void DynamicSceneEntity::InitializeAudioRole()
{
	const int	audioRoleIndex = GetAudioRoleIndex();
	ASSERT(audioRoleIndex >= 0);
    ARole&		aRole = mSceneEntityState.GetRole(audioRoleIndex);

    aRole.SetName("Audio");
}
// --------------------------------------------------------------------------
void DynamicSceneEntity::Goto(Time time)
{
	ADynamicSceneEntityBaseClass::Goto(time);
	AAudibleSceneEntityBaseClass::Goto(time);
}
// --------------------------------------------------------------------------
void DynamicSceneEntity::StateUpdated(Update::ID id, Time time)
{
	ADynamicSceneEntityBaseClass::StateUpdated(id, time);
	AAudibleSceneEntityBaseClass::StateUpdated(id, time);
}
// --------------------------------------------------------------------------
SceneEntityState& DynamicSceneEntity::GetSceneEntityState()
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
const SceneEntityState& DynamicSceneEntity::GetSceneEntityState() const
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
APhysicalState& DynamicSceneEntity::GetPhysicalState()
{
	return mDynamicSceneEntityState;
}
// --------------------------------------------------------------------------
const APhysicalState& DynamicSceneEntity::GetPhysicalState() const
{
	return mDynamicSceneEntityState;
}
// --------------------------------------------------------------------------
int DynamicSceneEntity::GetLocationRoleIndex() const
{
	return mLocationRoleIndex;
}
// --------------------------------------------------------------------------
int DynamicSceneEntity::GetOrientationRoleIndex() const
{
	return mOrientationRoleIndex;
}
// --------------------------------------------------------------------------
int DynamicSceneEntity::GetDynamicRoleIndex() const
{
	return mDynamicRoleIndex;
}
// --------------------------------------------------------------------------
int DynamicSceneEntity::GetAudioRoleIndex() const
{
	return mAudioRoleIndex;
}
// --------------------------------------------------------------------------
DynamicSceneEntity::AudioPlayingFlags& DynamicSceneEntity::GetAudioPlayingFlags()
{
	return mIsAudioActuallyPlaying;
}
// --------------------------------------------------------------------------
const DynamicSceneEntity::AudioPlayingFlags& DynamicSceneEntity::GetAudioPlayingFlags() const
{
	return mIsAudioActuallyPlaying;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
