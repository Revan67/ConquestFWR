// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <windows.h>
#include "DARenderPipeline.h"
#include "GLUtils.h"
#include "LiveCamera.h"
#include "LiveCameraRole.h"
#include "ConstSceneEntityStateAccessor.h"
#include "StaticsStateAccessor.h"
#include "Char.h"
#include "Utils.h"
#include "Scene.h"
#include "DACamera.h"
#include "SceneEntityEvent.h"
#include "ASceneEntityEventSource.h"
#include "GLUtils.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kSceneEntityState,
	kHasRollingCamera,
	kRollingCameraName
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class DummyState: public APhysicalState
{
	virtual void Write(std::ostream& oStream) const
	{
	}

	virtual void Read(std::istream& iStream)
	{
	}
};
// --------------------------------------------------------------------------
LiveCamera::LiveCamera(const ROSString& name, bool makeNameUnique, Scene& scene)
: mSceneEntityState(*this, name, makeNameUnique)
{
	mSceneEntityState.SetScene(&scene);
	mLiveCameraRoleIndex = mSceneEntityState.AddRole(*new LiveCameraRole(true, Interpolate, Interpolate, GetStateName, NULL));

	mInternalCamera = new DACamera("Camera Internal to: " + mSceneEntityState.GetName(), false, scene, true);
	ASSERT(mInternalCamera);

	mLiveCamera = mInternalCamera;

    std::auto_ptr<StaticsStateAccessor>    stateAccess = GetStaticsStateAccessor();

    stateAccess->SetLocation(Location(0, 0, 40));

	InitializeLiveCameraRole();
	
	Goto(scene.GetCurrentTimePoint());
}
// --------------------------------------------------------------------------
LiveCamera::LiveCamera(Scene& scene)
: mSceneEntityState(*this, "Camera Entity", false)
{
    //OutputDebugString("Constructing Live Camera\n");

	mLiveCameraRoleIndex = mSceneEntityState.AddRole(*new LiveCameraRole(true, Interpolate, Interpolate, GetStateName, NULL));
	
	mSceneEntityState.SetScene(&scene);
}
// --------------------------------------------------------------------------
void LiveCamera::Delete()
{
	BaseClass::Delete();

	delete this;
}
// --------------------------------------------------------------------------
LiveCamera::~LiveCamera()
{
	mInternalCamera->Delete();

	mInternalCamera = NULL;
}
// --------------------------------------------------------------------------
const std::auto_ptr<ConstSceneEntityStateAccessor> LiveCamera::GetConstSceneEntityStateAccessor() const
{
	return std::auto_ptr<ConstSceneEntityStateAccessor>(new ConstSceneEntityStateAccessor(*this, GetSceneEntityState()));
}
// --------------------------------------------------------------------------
std::auto_ptr<SceneEntityStateAccessor> LiveCamera::GetSceneEntityStateAccessor()
{
	return std::auto_ptr<SceneEntityStateAccessor>(new SceneEntityStateAccessor(*this, GetSceneEntityState()));
}
// --------------------------------------------------------------------------
const std::auto_ptr<ConstCameraStateAccessor> LiveCamera::GetConstCameraStateAccessor() const
{
	ASSERT(mLiveCamera);

	return mLiveCamera->GetConstCameraStateAccessor();
}
// --------------------------------------------------------------------------
std::auto_ptr<CameraStateAccessor> LiveCamera::GetCameraStateAccessor()
{
	ASSERT(mLiveCamera);

	return mLiveCamera->GetCameraStateAccessor();
}
// --------------------------------------------------------------------------
const std::auto_ptr<ConstStaticsStateAccessor> LiveCamera::GetConstStaticsStateAccessor() const
{
	ASSERT(mLiveCamera);

	return mLiveCamera->GetConstStaticsStateAccessor();
}
// --------------------------------------------------------------------------
std::auto_ptr<StaticsStateAccessor> LiveCamera::GetStaticsStateAccessor()
{
	ASSERT(mLiveCamera);

	return mLiveCamera->GetStaticsStateAccessor();
}
// --------------------------------------------------------------------------
const std::auto_ptr<ConstDynamicsStateAccessor> LiveCamera::GetConstDynamicsStateAccessor() const
{
	ASSERT(mLiveCamera);

	return mLiveCamera->GetConstDynamicsStateAccessor();
}
// --------------------------------------------------------------------------
std::auto_ptr<DynamicsStateAccessor> LiveCamera::GetDynamicsStateAccessor()
{
	ASSERT(mLiveCamera);

	return mLiveCamera->GetDynamicsStateAccessor();
}
// --------------------------------------------------------------------------
SceneEntityState& LiveCamera::GetSceneEntityState()
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
const SceneEntityState& LiveCamera::GetSceneEntityState() const
{
	return mSceneEntityState;
}
// --------------------------------------------------------------------------
APhysicalState& LiveCamera::GetPhysicalState()
{
	ASSERT(0); // What do we do here?

	return DummyState();	// Just to keep the compiler happy
}
// --------------------------------------------------------------------------
const APhysicalState&	LiveCamera::GetPhysicalState() const
{
	ASSERT(0); // What do we do here?

	return DummyState();	// Just to keep the compiler happy
}
// --------------------------------------------------------------------------
ROSString LiveCamera::GetArchetypeName()  const
{
	return GetLiveCameraArchetypeName();
}
// --------------------------------------------------------------------------
ROSString LiveCamera::GetLiveCameraArchetypeName()
{
    return "LiveCamera";
}
// --------------------------------------------------------------------------
void LiveCamera::Write(std::ostream& oStream) const
{
    BaseClass::Write(oStream);

    WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void LiveCamera::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

    ReadSubObject(iStream);

	mSceneEntityState.SetVisible(true);	// Force this entity visible

	mInternalCamera = new DACamera("Camera Internal to: " + mSceneEntityState.GetName(), false, mSceneEntityState.GetScene(), true);
	ASSERT(mInternalCamera);

	mLiveCamera = mInternalCamera;

	InitializeLiveCameraRole();

    //OutputDebugString("Read Live Camera from istream\n");
}
// --------------------------------------------------------------------------
void LiveCamera::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	oWiz.Put(kSceneEntityState, mSceneEntityState);

	const int				liveCameraRoleIndex = GetLiveCameraRoleIndex();
	ASSERT(liveCameraRoleIndex >= 0);
    const ARole*			aRole = &mSceneEntityState.GetRole(liveCameraRoleIndex);
    const LiveCameraRole*	lRole = dynamic_cast<const LiveCameraRole*>(aRole);
	ASSERT(lRole);

	ADynamicCamera*	camera = lRole->GetState(Time(0)).GetRollingCamera();

	const bool	hasCamera = camera ? true : false;

	oWiz.Put(kHasRollingCamera, hasCamera);

	// write set camera events for this camera
	char buffer[1024*4];
	int j = 0;
	if (GetLiveCameraRoleIndex() >= 0)
	{
		ARole*			aRole = const_cast<ARole *> (&mSceneEntityState.GetRole(GetLiveCameraRoleIndex()));
		LiveCameraRole*	role = dynamic_cast<LiveCameraRole*>(aRole);
		if (role)
		{
			j+= sprintf ( buffer + j, GetThornRoleInfo( role, GetSceneEntityState().GetName() ).c_str() );
		}
	}

	if(hasCamera)
	{
		oWiz.Put(kRollingCameraName, camera->GetConstSceneEntityStateAccessor()->GetName(), buffer);
	}
}
// --------------------------------------------------------------------------
void LiveCamera::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	iWiz.Get(kSceneEntityState, mSceneEntityState);
    
	bool	hasCamera;

	iWiz.Get(kHasRollingCamera, hasCamera);

	if(hasCamera)
	{
		ROSString	name;

		iWiz.Get(kRollingCameraName, name);

		SceneEntityRemapper::Add(name, new SceneEntityRemap<ADynamicCamera*>(&mLiveCamera));
	}
}
// --------------------------------------------------------------------------
void LiveCamera::InitializeLiveCameraRole()
{
	const int	liveCameraRoleIndex = GetLiveCameraRoleIndex();
	ASSERT(liveCameraRoleIndex >= 0);
    ARole*		aRole = &mSceneEntityState.GetRole(liveCameraRoleIndex);

    aRole->SetName("Live Camera");
    
    LiveCameraRole*	lRole = dynamic_cast<LiveCameraRole*>(aRole);
    ASSERT(lRole);

    if(!mSceneEntityState.GetScene().IsUsingInitialEntityState())
    {
		if(mLiveCamera != mInternalCamera)
		{
			lRole->StateUpdated(LiveCameraState(mLiveCamera), Time(0));
			
			RoleUpdated();
		}
    }
}
// --------------------------------------------------------------------------
void LiveCamera::Respond(const SceneEntityEvent& event)
{
	BaseClass::Respond(event);

	if(event.GetID() == SceneEntityEvent::kSourceEntityDeleted)
	{
		ASceneEntity& entity = event.GetSourceEntity();

		const int		liveCameraRoleIndex = GetLiveCameraRoleIndex();
		ASSERT(liveCameraRoleIndex >= 0);
		ARole*			aRole = &mSceneEntityState.GetRole(liveCameraRoleIndex);
		LiveCameraRole*	lRole = dynamic_cast<LiveCameraRole*>(aRole);
		ASSERT(lRole);

		unsigned int timePointCount = lRole->CountTimePoints();
		unsigned int idx = 0;

		while(idx < timePointCount)
		{
			if(lRole->GetState(idx).GetRollingCamera() == &entity)
			{
				lRole->Remove(idx);

				--timePointCount;
			}
			else
			{
				++idx;
			}
		}

		GetSceneEntityState().RemoveSource(entity);

		RoleUpdated();
	}
}
// --------------------------------------------------------------------------
void LiveCamera::RoleUpdated()
{
	UpdateSourceRollingCameras();

	BaseClass::RoleUpdated();
}
// --------------------------------------------------------------------------
void LiveCamera::UpdateSourceRollingCameras()
{
	const int		liveCameraRoleIndex = GetLiveCameraRoleIndex();
	ASSERT(liveCameraRoleIndex >= 0);
    ARole*			aRole = &mSceneEntityState.GetRole(liveCameraRoleIndex);
	LiveCameraRole*	lRole = dynamic_cast<LiveCameraRole*>(aRole);
	ASSERT(lRole);

	// Check every source to make sure it is in at least one marker
	unsigned int sourceCount = mSceneEntityState.GetSourceCount();
	unsigned int sourceIdx = 0;
	
	while(sourceIdx < sourceCount)
	{
		ASceneEntityEventSource&	entity = mSceneEntityState.GetSource(sourceIdx);

		const unsigned int timePointCount = lRole->CountTimePoints();

		for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
		{
			if(lRole->GetState(timePointIdx).GetRollingCamera() == &entity)
			{
				break;
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

	const unsigned int timePointCount = lRole->CountTimePoints();

	for(unsigned int timePointIdx = 0; timePointIdx < timePointCount; ++timePointIdx)
	{
		ADynamicCamera*	camera = lRole->GetState(timePointIdx).GetRollingCamera();

		sourceCount = mSceneEntityState.GetSourceCount();

		for(sourceIdx = 0; sourceIdx < sourceCount; ++sourceIdx)
		{
			if(&mSceneEntityState.GetSource(sourceIdx) == camera)
			{
				break;
			}
		}

		if(sourceIdx == sourceCount)
		{
			// The source is not in the list. Add it and hook up with the
			// source as a listener
			mSceneEntityState.AddSource(*camera);
			
			camera->GetSceneEntityStateAccessor()->AddListener(*this);
		}
	}
}
// --------------------------------------------------------------------------
int LiveCamera::GetLocationRoleIndex() const
{
	return -1;
}
// --------------------------------------------------------------------------
int LiveCamera::GetOrientationRoleIndex() const
{
	return -1;
}
// --------------------------------------------------------------------------
int LiveCamera::GetDynamicRoleIndex() const
{
	return -1;
}
// --------------------------------------------------------------------------
int LiveCamera::GetCameraRoleIndex() const
{
	return -1;
}
// --------------------------------------------------------------------------
int LiveCamera::GetLiveCameraRoleIndex() const
{
	return mLiveCameraRoleIndex;
}
// --------------------------------------------------------------------------
void LiveCamera::Goto(Time time)
{
	// Don't want to invoke the BaseClass::Goto(), since that expects the
	// location, orientation, camera and dynamics roles.

	GotoForLiveCameraRole(time);
	SetInternalCameraStateToLiveCameraState();
}
// --------------------------------------------------------------------------
void LiveCamera::GotoForLiveCameraRole(Time time)
{
	const int				liveCameraRoleIndex = GetLiveCameraRoleIndex();
	ASSERT(liveCameraRoleIndex >= 0);
    const ARole*			aRole = &mSceneEntityState.GetRole(liveCameraRoleIndex);
    const LiveCameraRole* cRole = dynamic_cast<const LiveCameraRole*>(aRole);
	ASSERT(cRole);

	ADynamicCamera*	camera = cRole->GetState(time).GetRollingCamera();

	if(camera)
	{
		mLiveCamera = camera;
	}
	else
	{
		mLiveCamera = mInternalCamera;
	}
}
// --------------------------------------------------------------------------
void LiveCamera::SetInternalCameraStateToLiveCameraState()
{
	if(mLiveCamera != mInternalCamera)
	{
		mInternalCamera->SetState(*mLiveCamera);
	}
}
// --------------------------------------------------------------------------
void LiveCamera::Render(const ROS::DABaseCamera* camera) const
{
	if(mLiveCamera != mInternalCamera)
	{
		float	color[] = {1.0, 0.0, 0.0};

		GL::WireCube(0.8, color);
	}
}
// --------------------------------------------------------------------------
ASceneEntity* LiveCamera::GetDependentEntity()
{
	return mLiveCamera == mInternalCamera ? NULL : mLiveCamera;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
