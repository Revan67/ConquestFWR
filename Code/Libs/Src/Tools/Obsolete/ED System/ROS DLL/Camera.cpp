// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <windows.h>
#include "DARenderPipeline.h"
#include "GLUtils.h"
#include "Camera.h"
#include "Role.h"
#include "LocationRole.h"
#include "OrientationRole.h"
#include "DynamicsRole.h"
#include "CameraRole.h"
#include "AudioRole.h"
#include "StaticsStateAccessor.h"
#include "Char.h"
#include "Utils.h"
#include "Scene.h"
// --------------------------------------------------------------------------
/**# implementation Camera:: id(C_0886789897)
*/
// --------------------------------------------------------------------------
enum FieldID
{
	kSceneEntityState,
	kCameraState
};	
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
Camera::Camera(const ROSString& name, bool makeNameUnique, Scene& scene)
: mSceneEntityState(*this, name, makeNameUnique), mCameraState(new CameraDynamicsState)
{
	mSceneEntityState.SetScene(&scene);

	LocationRole*	locationRole = new LocationRole(true, Interpolate, Interpolate, GetStateName, NULL);

	mLocationRoleIndex = mSceneEntityState.AddRole(*locationRole);
	mOrientationRoleIndex = mSceneEntityState.AddRole(*new OrientationRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL, locationRole));
	mCameraRoleIndex = mSceneEntityState.AddRole(*new CameraRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	mDynamicRoleIndex = mSceneEntityState.AddRole(*new DynamicsRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	mAudioRoleIndex = mSceneEntityState.AddRole(*new AudioRole(false, Interpolate, Interpolate, GetStateName, NULL));

    std::auto_ptr<StaticsStateAccessor>    stateAccess = GetStaticsStateAccessor();

    stateAccess->SetLocation(Location(0, 0, 40));

	InitializeLocationRole();
	InitializeOrientationRole();
	InitializeCameraRole();
	InitializeDynamicRole();
    InitializeAudioRole();
	
	Goto(scene.GetCurrentTimePoint());
}
// --------------------------------------------------------------------------
Camera::Camera(Scene& scene)
: mSceneEntityState(*this, "Camera Entity", false), mCameraState(new CameraDynamicsState)
{
	mSceneEntityState.SetScene(&scene);

	LocationRole*	locationRole = new LocationRole(true, Interpolate, Interpolate, GetStateName, NULL);

	mLocationRoleIndex = mSceneEntityState.AddRole(*locationRole);
	mOrientationRoleIndex = mSceneEntityState.AddRole(*new OrientationRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL, locationRole));
	mCameraRoleIndex = mSceneEntityState.AddRole(*new CameraRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	mDynamicRoleIndex = mSceneEntityState.AddRole(*new DynamicsRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	mAudioRoleIndex = mSceneEntityState.AddRole(*new AudioRole(false, Interpolate, Interpolate, GetStateName, NULL));
}
// --------------------------------------------------------------------------
void Camera::Delete()
{
	BaseClass::Delete();

	delete this;
}
// --------------------------------------------------------------------------
Camera::~Camera()
{
}
// --------------------------------------------------------------------------
SceneEntityState& Camera::GetSceneEntityState()
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
const SceneEntityState& Camera::GetSceneEntityState() const
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
APhysicalState& Camera::GetPhysicalState()
{
	ASSERT(IsNotNull(mCameraState));

	return *mCameraState;
}
// --------------------------------------------------------------------------
const APhysicalState&	Camera::GetPhysicalState() const
{
	ASSERT(IsNotNull(mCameraState));

	return *mCameraState;
}
// --------------------------------------------------------------------------
ROSString Camera::GetArchetypeName()  const
{
	return GetCameraArchetypeName();
}
// --------------------------------------------------------------------------
ROSString Camera::GetCameraArchetypeName()
{
    return "Camera";
}
// --------------------------------------------------------------------------
bool Camera::FindIntersect(const IntersectInfo& intersectInfo, float* distance) const
{
	std::auto_ptr<ConstStaticsStateAccessor>	accessor = GetConstStaticsStateAccessor();

	const Location	location = accessor->GetLocation();
	const Vector	vector(location.GetX(), location.GetY(), location.GetZ());

	const Orientation	orientation = accessor->GetOrientation();
	const ::Matrix		matrix(orientation.GetI(), orientation.GetJ(), orientation.GetK());

	const Transform	transform(matrix, vector);

	return CharIntersectCamera(intersectInfo, transform, distance);
}
// --------------------------------------------------------------------------
void Camera::SetupPosition() const
{
    // This method is a noop on purpose, since Camera::Render() takes care of transforms
}
// --------------------------------------------------------------------------
void Camera::Render(const ROS::DABaseCamera* camera) const
{   
	std::auto_ptr<ConstStaticsStateAccessor>	accessor = GetConstStaticsStateAccessor();

	const Location	location = accessor->GetLocation();
	const Vector	vector(location.GetX(), location.GetY(), location.GetZ());

	const Orientation	orientation = accessor->GetOrientation();
	const ::Matrix		matrix(orientation.GetI(), orientation.GetJ(), orientation.GetK());

	const Transform	transform(matrix, vector);

	CharRenderCamera(transform, camera);
}
// --------------------------------------------------------------------------
void Camera::Write(std::ostream& oStream) const
{
    BaseClass::Write(oStream);

    WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void Camera::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

    ReadSubObject(iStream);

	mSceneEntityState.SetVisible(false);	// Force this entity invisible

	InitializeLocationRole();
	InitializeOrientationRole();
	InitializeCameraRole();
	InitializeDynamicRole();
    InitializeAudioRole();

    //OutputDebugString("Read Camera from istream\n");
}
// --------------------------------------------------------------------------
void Camera::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	ROS::Time timeZero(0);
	char buffer[1024];
	int j;
	// write the lua enity table for this entity
	j = sprintf(buffer,			"\n");
	j+= sprintf(buffer + j,		"%s = ,\n{\n", GetSceneEntityState().GetName().c_str());
	j+= sprintf(buffer + j,		"\ttype = CAMERA,\n\tflags = 0,\n");
	j+= sprintf(buffer + j,		"\tspatialprops = \n\t{\n");
	j+= sprintf(buffer + j,		"\t\tpos = {%f, %f, %f},\n", GetLocation(timeZero).GetX(), GetLocation(timeZero).GetY(), GetLocation(timeZero).GetZ());
	j+= sprintf(buffer + j,		"\t\torient = { {%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f} }\n",
			GetOrientation(timeZero).GetI().x, GetOrientation(timeZero).GetI().y, GetOrientation(timeZero).GetI().z, 
			GetOrientation(timeZero).GetJ().x, GetOrientation(timeZero).GetJ().y, GetOrientation(timeZero).GetJ().z, 
			GetOrientation(timeZero).GetK().x, GetOrientation(timeZero).GetK().y, GetOrientation(timeZero).GetK().z
		);
	j+= sprintf(buffer + j,		"\t},\n");
	j+= sprintf(buffer + j,		"\tcameraprops = \n\t{\n");
	j+= sprintf(buffer + j,		"\t\tfovh = %f\n", mCameraState->GetHorizontalFOV());
	j+= sprintf(buffer + j,		"\t},\n");
	j+= sprintf(buffer + j,		"\tuserprops = \n\t{\n");
	j+= sprintf(buffer + j,		"\t},\n");
	j+= sprintf(buffer + j,		"},\n");

	// write the orientation animation data
	if (GetOrientationRoleIndex() >= 0)
	{
		ARole*				aRole = const_cast<ARole *> (&mSceneEntityState.GetRole(GetOrientationRoleIndex()));
		OrientationRole*	role = dynamic_cast<OrientationRole*>(aRole);
		if (role)
		{
			j+= sprintf ( buffer + j, GetThornRoleInfo( role, mSceneEntityState.GetName() ).c_str() );
		}
	}
	// write the orientation animation data
	if (GetCameraRoleIndex() >= 0)
	{
		ARole*				aRole = const_cast<ARole *> (&mSceneEntityState.GetRole(GetCameraRoleIndex()));
		CameraRole*			role = dynamic_cast<CameraRole*>(aRole);
		if (role)
		{
			j+= sprintf ( buffer + j, GetThornRoleInfo( role, mSceneEntityState.GetName() ).c_str() );
		}
	}
	// write the location animation data
	if (GetLocationRoleIndex() >= 0)
	{
		ARole*			aRole = const_cast<ARole *> (&mSceneEntityState.GetRole(GetLocationRoleIndex()));
		LocationRole*	role = dynamic_cast<LocationRole*>(aRole);
		if (role)
		{
			j+= sprintf ( buffer + j, GetThornRoleInfo( role, mSceneEntityState.GetName() ).c_str() );
		}
	}

	oWiz.Put(kSceneEntityState, mSceneEntityState);
    oWiz.Put(kCameraState, *mCameraState, buffer);
}
// --------------------------------------------------------------------------
void Camera::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
	
	iWiz.Get(kSceneEntityState, mSceneEntityState);
    iWiz.Get(kCameraState, *mCameraState);
}
// --------------------------------------------------------------------------
void Camera::InitializeLocationRole()
{
	const int	locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
    ARole*		aRole = &mSceneEntityState.GetRole(locationRoleIndex);

    aRole->SetName("Location");
    
    LocationRole*	lRole = dynamic_cast<LocationRole*>(aRole);
    ASSERT(lRole);

    if(!mSceneEntityState.GetScene().IsUsingInitialEntityState())
    {
		lRole->StateUpdated(FlaggedLocation(mCameraState->GetPosition().GetLocation(), lRole->GetState(Time(0)).GetInterpolationType()), Time(0));
    }
}
// --------------------------------------------------------------------------
void Camera::InitializeOrientationRole()
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

		oRole->StateUpdated(FlaggedOrientation(mCameraState->GetPosition().GetOrientation(), currentOrientation.GetInterpolationType(), currentOrientation.GetTargetEntity()), Time(0));
    }
}
// --------------------------------------------------------------------------
void Camera::InitializeDynamicRole()
{
	const int	dynamicRoleIndex = GetDynamicRoleIndex();
    ASSERT(dynamicRoleIndex >= 0);
	ARole*		aRole = &mSceneEntityState.GetRole(dynamicRoleIndex);

    aRole->SetName("Dynamics");
    
    DynamicsRole*	dRole = dynamic_cast<DynamicsRole*>(aRole);
    ASSERT(dRole);

    if(!mSceneEntityState.GetScene().IsUsingInitialEntityState())
    {
		dRole->StateUpdated(*mCameraState, Time(0));
    }
}
// --------------------------------------------------------------------------
void Camera::InitializeCameraRole()
{
	const int	cameraRoleIndex = GetCameraRoleIndex();
	ASSERT(cameraRoleIndex >= 0);
    ARole*		aRole = &mSceneEntityState.GetRole(cameraRoleIndex);

    aRole->SetName("Camera");
    
    CameraRole*	cRole = dynamic_cast<CameraRole*>(aRole);
    ASSERT(cRole);

    if(!mSceneEntityState.GetScene().IsUsingInitialEntityState())
    {
		cRole->StateUpdated(*mCameraState, Time(0));
    }
}
// --------------------------------------------------------------------------
void Camera::InitializeAudioRole()
{
	const int	audioRoleIndex = GetAudioRoleIndex();
	ASSERT(audioRoleIndex >= 0);
    ARole&		aRole = mSceneEntityState.GetRole(audioRoleIndex);

    aRole.SetName("Audio");
}

// --------------------------------------------------------------------------
void Camera::Respond(const SceneEntityEvent& event)
{
	BaseClass::Respond(event);

	if(event.GetID() == SceneEntityEvent::kSourceEntityDeleted)
	{
		ASceneEntity& entity = event.GetSourceEntity();

		const int			orientationRoleIndex = GetOrientationRoleIndex();
		ASSERT(orientationRoleIndex >= 0);
		ARole*				aRole = &mSceneEntityState.GetRole(orientationRoleIndex);
		OrientationRole*	oRole = dynamic_cast<OrientationRole*>(aRole);
		ASSERT(oRole);

		unsigned int timePointCount = oRole->CountTimePoints();
		unsigned int idx = 0;

		while(idx < timePointCount)
		{
			const FlaggedOrientation	orientation = oRole->GetState(idx);
			
			if(orientation.GetInterpolationType() == FlaggedOrientation::kLookAt)
			{
				if(orientation.GetTargetEntity() == &entity)
				{
					oRole->Remove(idx);

					--timePointCount;

					continue;
				}
			}

			++idx;
		}

		GetSceneEntityState().RemoveSource(entity);

		RoleUpdated();
	}
}
// --------------------------------------------------------------------------
void Camera::RoleUpdated()
{
	UpdateSourceTargetEntities();

	BaseClass::RoleUpdated();
}
// --------------------------------------------------------------------------
void Camera::UpdateSourceTargetEntities()
{
	const int			orientationRoleIndex = GetOrientationRoleIndex();
	ASSERT(orientationRoleIndex >= 0);
	ARole*				aRole = &mSceneEntityState.GetRole(orientationRoleIndex);
	OrientationRole*	oRole = dynamic_cast<OrientationRole*>(aRole);
	ASSERT(oRole);

	// Check every source to make sure it is in at least one marker
	unsigned int sourceCount = mSceneEntityState.GetSourceCount();
	unsigned int sourceIdx = 0;
	
	while(sourceIdx < sourceCount)
	{
		ASceneEntityEventSource&	entity = mSceneEntityState.GetSource(sourceIdx);

		const unsigned int timePointCount = oRole->CountTimePoints();

		for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
		{
			const FlaggedOrientation	orientation = oRole->GetState(timePointIdx);

			if(orientation.GetInterpolationType() == FlaggedOrientation::kLookAt)
			{
				if(orientation.GetTargetEntity() ==&entity)
				{
					break;
				}
			}
		}

		if(timePointIdx == timePointCount)
		{
			// No time point found for this source. Remove it and unhook from
			// the source as a listener
			mSceneEntityState.RemoveSource(entity);

			ASceneEntity*	sEntity = dynamic_cast<ASceneEntity*>(&entity);
			ASSERT(sEntity);

			sEntity->GetSceneEntityStateAccessor()->RemoveListener(*this);
			
			--sourceCount;
		}
		else
		{
			// On to the next source
			++sourceIdx;
		}
	}

	// Now check every time point to ensure that its source is in the list
	// of sources

	const unsigned int timePointCount = oRole->CountTimePoints();

	for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
	{
		const FlaggedOrientation	orientation = oRole->GetState(timePointIdx);

		if(orientation.GetInterpolationType() == FlaggedOrientation::kLookAt)
		{
			AStaticSceneEntity*	target = orientation.GetTargetEntity();

			sourceCount = mSceneEntityState.GetSourceCount();

			for(sourceIdx = 0; sourceIdx < sourceCount; ++sourceIdx)
			{
				if(&mSceneEntityState.GetSource(sourceIdx) == target)
				{
					break;
				}
			}

			if(sourceIdx == sourceCount)
			{
				// The source is not in the list. Add it and hook up with the
				// source as a listener
				mSceneEntityState.AddSource(*target);
				
				target->GetSceneEntityStateAccessor()->AddListener(*this);
			}
		}
	}
}
// --------------------------------------------------------------------------
int Camera::GetLocationRoleIndex() const
{
	return mLocationRoleIndex;
}
// --------------------------------------------------------------------------
int Camera::GetOrientationRoleIndex() const
{
	return mOrientationRoleIndex;
}
// --------------------------------------------------------------------------
int Camera::GetDynamicRoleIndex() const
{
	return mDynamicRoleIndex;
}
// --------------------------------------------------------------------------
int Camera::GetCameraRoleIndex() const
{
	return mCameraRoleIndex;
}
// --------------------------------------------------------------------------
int Camera::GetAudioRoleIndex() const
{
	return mAudioRoleIndex;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
