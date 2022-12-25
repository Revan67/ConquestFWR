// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <windows.h>
#include "DARenderPipeline.h"
#include "GLUtils.h"
#include "DACamera.h"
#include "DACameraDynamicsState.h"
#include "CameraDynamicsState.h"
#include "CodeMsg.h"
#include "Role.h"
#include "LocationRole.h"
#include "OrientationRole.h"
#include "DynamicsRole.h"
#include "CameraRole.h"
#include "AudioRole.h"
#include "Scene.h"
#include "Utils.h"
#include "Char.h"

#if 1
#include "AMarker.h"
#endif
// --------------------------------------------------------------------------
enum FieldID
{
	kSceneEntityState
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
static ROSString GetStateName(const CameraDynamicsState& cameraState)
{
	return "Position and FOV Change";
}
// --------------------------------------------------------------------------
DACamera::DACamera(const ROSString& name, bool makeNameUnique, Scene& scene, bool isInternal)
: mSceneEntityState(*this, name, makeNameUnique), mWidth(640), mHeight(480), mIsInternal(isInternal)
{
	LocationRole*	locationRole = new LocationRole(true, Interpolate, Interpolate, GetStateName, NULL);

	mLocationRoleIndex = mSceneEntityState.AddRole(*locationRole);
	mOrientationRoleIndex = mSceneEntityState.AddRole(*new OrientationRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL, locationRole));
	mCameraRoleIndex = mSceneEntityState.AddRole(*new CameraRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	mDynamicRoleIndex = mSceneEntityState.AddRole(*new DynamicsRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	mAudioRoleIndex = mSceneEntityState.AddRole(*new AudioRole(false, Interpolate, Interpolate, GetStateName, NULL));
	
	mSceneEntityState.SetScene(&scene);

	SceneEventFlag	flag = kSEFNone;
	
	SetupDACamera(flag);

	const bool	useStateInScript = (flag & kSEFUseStateInScriptAsInitialState) != 0;

    InitializeLocationRole(useStateInScript);
    InitializeOrientationRole(useStateInScript);
    InitializeDynamicRole(useStateInScript);
    InitializeCameraRole(useStateInScript);
    InitializeAudioRole(useStateInScript);
	
	Goto(scene.GetCurrentTimePoint());
}
// --------------------------------------------------------------------------
DACamera::DACamera(Scene& scene)
: mSceneEntityState(*this, "DA Camera", false), mWidth(640), mHeight(480), mIsInternal(false)
{
	LocationRole*	locationRole = new LocationRole(true, Interpolate, Interpolate, GetStateName, NULL);

	mLocationRoleIndex = mSceneEntityState.AddRole(*locationRole);
	mOrientationRoleIndex = mSceneEntityState.AddRole(*new OrientationRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL, locationRole));
	mCameraRoleIndex = mSceneEntityState.AddRole(*new CameraRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	mDynamicRoleIndex = mSceneEntityState.AddRole(*new DynamicsRole(true, LinearInterpolate, LinearInterpolate, GetStateName, NULL));
	mAudioRoleIndex = mSceneEntityState.AddRole(*new AudioRole(false, Interpolate, Interpolate, GetStateName, NULL));

	mSceneEntityState.SetScene(&scene);
}
// --------------------------------------------------------------------------
void DACamera::Delete()
{
	BaseClass::Delete();

	delete this;
}
// --------------------------------------------------------------------------
DACamera::~DACamera()
{
	DACameraDynamicsState*	state = dynamic_cast<DACameraDynamicsState*>(mCameraState.get());

	if(state)
	{
		void*	userData = mSceneEntityState.GetUserData();


		if(mIsInternal)
		{
			mSceneEntityState.GetScene().DestroyInternalDABaseCamera(state->GetDABaseCamera(), mSceneEntityState.GetName(), &userData);
		}
		else
		{
			mSceneEntityState.GetScene().DestroyDABaseCamera(state->GetDABaseCamera(), mSceneEntityState.GetName(), &userData);
		}

		mSceneEntityState.SetUserData(userData);

		state->SetDABaseCamera(NULL);
	}
}
// --------------------------------------------------------------------------
bool DACamera::FindIntersect(const IntersectInfo& intersectInfo, float* distance) const
{
	return false;
}
// --------------------------------------------------------------------------
void DACamera::SetupDACamera(SceneEventFlag& flag)
{
	const DABaseCamera* camera = NULL;

    try
    {
		void*	userData = mSceneEntityState.GetUserData();

		if(mIsInternal)
		{
			camera = mSceneEntityState.GetScene().CreateInternalDABaseCamera(mSceneEntityState.GetName(), &userData, &flag);
		}
		else
		{
			camera = mSceneEntityState.GetScene().CreateDABaseCamera(mSceneEntityState.GetName(), &userData, &flag);
		}

		mSceneEntityState.SetUserData(userData);
    }
    catch(...)
    {
    }

    if(IsNotNull(camera))
    {
		try
		{
			DACameraDynamicsState*	state = new DACameraDynamicsState(camera);
			
			state->SetDABaseCamera(camera);

			mCameraState = AggAPointer<ACameraDynamicsState>(state);
		}
		catch(...)
		{
			void*	userData = mSceneEntityState.GetUserData();

			if(mIsInternal)
			{
				mSceneEntityState.GetScene().DestroyInternalDABaseCamera(camera, mSceneEntityState.GetName(), &userData);
			}
			else
			{
				mSceneEntityState.GetScene().DestroyDABaseCamera(camera, mSceneEntityState.GetName(), &userData);
			}
			
			mSceneEntityState.SetUserData(userData);

			throw ExCameraCreationFailed(mSceneEntityState.GetName());
		}
    }
	else
	{
		try
		{
			CameraDynamicsState*	state = new CameraDynamicsState;

			mCameraState = AggAPointer<ACameraDynamicsState>(state);

			flag = kSEFUseStateInScriptAsInitialState;
		}
		catch(...)
		{
			throw ExCameraCreationFailed(mSceneEntityState.GetName());
		}
	}
}
// --------------------------------------------------------------------------
ROSString DACamera::GetArchetypeName()  const
{
	return GetDACameraArchetypeName();
}
// --------------------------------------------------------------------------
ROSString DACamera::GetDACameraArchetypeName()
{
    return "DACamera";
}
// --------------------------------------------------------------------------
void DACamera::Write(std::ostream& oStream) const
{
    BaseClass::Write(oStream);
	
	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void DACamera::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	ReadSubObject(iStream);

	mSceneEntityState.SetVisible(false);	// Force this entity invisible

	SceneEventFlag	flag = kSEFNone;

	SetupDACamera(flag);

	const bool	useStateInScript = (flag & kSEFUseStateInScriptAsInitialState) != 0;

    InitializeLocationRole(useStateInScript);
    InitializeOrientationRole(useStateInScript);
    InitializeDynamicRole(useStateInScript);
    InitializeCameraRole(useStateInScript);
    InitializeAudioRole(useStateInScript);
}
// --------------------------------------------------------------------------
void DACamera::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);
	
	ROS::Time timeZero(0);
	char buffer[1024*4];
	int j;
	// write the lua enity table for this entity
	j = sprintf(buffer,			"\n");
	j+= sprintf(buffer + j,		"%s = \n{\n", mSceneEntityState.GetName().c_str());
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
	j+= sprintf(buffer + j,		"\t}\n");
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
	// write the location animation data
	if (GetLocationRoleIndex() >= 0)
	{
		ARole*			aRole = const_cast<ARole *> (&mSceneEntityState.GetRole(GetLocationRoleIndex()));
		LocationRole*	role = dynamic_cast<LocationRole*>(aRole);
		if (role)
		{
			j+= sprintf ( buffer + j, GetThornRoleInfo( role, GetSceneEntityState().GetName() ).c_str() );
		}
	}
	// write set camera events for this camera
	if (GetCameraRoleIndex() >= 0)
	{
		ARole*			aRole = const_cast<ARole *> (&mSceneEntityState.GetRole(GetCameraRoleIndex()));
		CameraRole*	role = dynamic_cast<CameraRole*>(aRole);
		if (role)
		{
			j+= sprintf ( buffer + j, GetThornRoleInfo( role, GetSceneEntityState().GetName() ).c_str() );
		}
	}

	oWiz.Put(kSceneEntityState, mSceneEntityState, buffer);
}
// --------------------------------------------------------------------------
void DACamera::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
	
	iWiz.Get(kSceneEntityState, mSceneEntityState);
}
// --------------------------------------------------------------------------
void DACamera::SetupPosition() const
{
    // This method is a noop on purpose, since Camera::Render() takes care of transforms
}
// --------------------------------------------------------------------------
void DACamera::Render(const ROS::DABaseCamera* camera) const
{
	DACameraDynamicsState*	state = dynamic_cast<DACameraDynamicsState*>(mCameraState.get());

	if(state)
	{
		if(camera != state->GetDABaseCamera())
		{
			std::auto_ptr<ConstStaticsStateAccessor>	accessor = GetConstStaticsStateAccessor();

			const Location	location = accessor->GetLocation();
			const Vector	vector(location.GetX(), location.GetY(), location.GetZ());

			const Orientation	orientation = accessor->GetOrientation();
			const ::Matrix		matrix(orientation.GetI(), orientation.GetJ(), orientation.GetK());

			const Transform	transform(matrix, vector);

			CharRenderCamera(transform, camera);
		}
	}
}
// --------------------------------------------------------------------------
void DACamera::InitializeLocationRole(bool useStateInScript)
{
	const int	locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
    ARole*		aRole = &mSceneEntityState.GetRole(locationRoleIndex);

    aRole->SetName("Location");

    LocationRole*   lRole = dynamic_cast<LocationRole*>(aRole);
	ASSERT(lRole);

	if(!(mSceneEntityState.GetScene().IsUsingInitialEntityState() || useStateInScript))
    {   
		lRole->StateUpdated(FlaggedLocation(mCameraState->GetPosition().GetLocation(), lRole->GetState(Time(0)).GetInterpolationType()), Time(0));
    }
}
// --------------------------------------------------------------------------
void DACamera::InitializeOrientationRole(bool useStateInScript)
{
	const int	orientationRoleIndex = GetOrientationRoleIndex();
	ASSERT(orientationRoleIndex >= 0);
    ARole*		aRole = &mSceneEntityState.GetRole(orientationRoleIndex);

    aRole->SetName("Orientation");

    OrientationRole*   oRole = dynamic_cast<OrientationRole*>(aRole);
	ASSERT(oRole);

	if(!(mSceneEntityState.GetScene().IsUsingInitialEntityState() || useStateInScript))
    {   
		const FlaggedOrientation	currentOrientation = oRole->GetState(Time(0));

		oRole->StateUpdated(FlaggedOrientation(mCameraState->GetPosition().GetOrientation(), currentOrientation.GetInterpolationType(), currentOrientation.GetTargetEntity()), Time(0));
    }
}
// --------------------------------------------------------------------------
void DACamera::InitializeDynamicRole(bool useStateInScript)
{
	const int	dynamicRoleIndex = GetDynamicRoleIndex();
	ASSERT(dynamicRoleIndex >= 0);
    ARole*  aRole = &mSceneEntityState.GetRole(dynamicRoleIndex);

    aRole->SetName("Dynamics");

    DynamicsRole*   dRole = dynamic_cast<DynamicsRole*>(aRole);
	ASSERT(dRole);

	if(!(mSceneEntityState.GetScene().IsUsingInitialEntityState() || useStateInScript))
    {   
    }
}
// --------------------------------------------------------------------------
void DACamera::InitializeCameraRole(bool useStateInScript)
{
	const int	cameraRoleIndex = GetCameraRoleIndex();
	ASSERT(cameraRoleIndex >= 0);
    ARole*		aRole = &mSceneEntityState.GetRole(cameraRoleIndex);

    aRole->SetName("Camera");

    CameraRole*   cRole = dynamic_cast<CameraRole*>(aRole);
	ASSERT(cRole);

	if(!(mSceneEntityState.GetScene().IsUsingInitialEntityState() || useStateInScript))
    {   
        CameraState		cState(mCameraState->GetHorizontalFOV(), mCameraState->GetVerticalFOV());

		cRole->StateUpdated(cState, Time(0));
    }
}
// --------------------------------------------------------------------------
void DACamera::InitializeAudioRole(bool useStateInScript)
{
	const int	audioRoleIndex = GetAudioRoleIndex();
	ASSERT(audioRoleIndex >= 0);
    ARole&		aRole = mSceneEntityState.GetRole(audioRoleIndex);

    aRole.SetName("Audio");
}

// --------------------------------------------------------------------------
int DACamera::GetAudioRoleIndex() const
{
	return mAudioRoleIndex;
}
// --------------------------------------------------------------------------
void DACamera::Respond(const SceneEntityEvent& event)
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

		const unsigned int	timePointCount = oRole->CountTimePoints();
		unsigned int		idx = 0;
		bool				deletedEntityIsTarget = false;

		while(idx < timePointCount)
		{
			FlaggedOrientation	orientation = oRole->GetState(idx);
			
			if(orientation.GetInterpolationType() == FlaggedOrientation::kLookAt)
			{
				if(orientation.GetTargetEntity() == &entity)
				{
					deletedEntityIsTarget = true;

					orientation.SetInterpolationType(FlaggedOrientation::kLinear);
					orientation.SetTargetEntity(NULL);

					oRole->StateUpdated(orientation, idx);
				}
			}

			++idx;
		}

		if(deletedEntityIsTarget)
		{
			GetSceneEntityState().RemoveSource(entity);
			RoleUpdated();
		}
	}
}
// --------------------------------------------------------------------------
void DACamera::RoleUpdated()
{
	UpdateSourceTargetEntities();

	BaseClass::RoleUpdated();
}
// --------------------------------------------------------------------------
void DACamera::UpdateSourceTargetEntities()
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

		if(timePointIdx == timePointCount &&
			!dynamic_cast<AMarker*>(&entity))	/********** TEMPORARY SOLUTION TO PREVENT Markers FROM BEING REMOVED. THE CORRECT SOLUTION IS TO DETECT REMOVED TARGETS IN THE Respond() METHOD*********/
												/********* ALSO ADDED A TEMPORARY INCLUDE FOR AMarker.h **********/
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
int DACamera::GetLocationRoleIndex() const
{
	return mLocationRoleIndex;
}
// --------------------------------------------------------------------------
int DACamera::GetOrientationRoleIndex() const
{
	return mOrientationRoleIndex;
}
// --------------------------------------------------------------------------
int DACamera::GetDynamicRoleIndex() const
{
	return mDynamicRoleIndex;
}
// --------------------------------------------------------------------------
int DACamera::GetCameraRoleIndex() const
{
	return mCameraRoleIndex;
}
// --------------------------------------------------------------------------
SceneEntityState& DACamera::GetSceneEntityState()
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
const SceneEntityState& DACamera::GetSceneEntityState() const
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
APhysicalState& DACamera::GetPhysicalState()
{
	return *mCameraState;
}
// --------------------------------------------------------------------------
const APhysicalState& DACamera::GetPhysicalState() const
{
	return *mCameraState;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

