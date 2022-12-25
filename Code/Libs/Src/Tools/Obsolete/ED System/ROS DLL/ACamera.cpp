// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ACamera.h"
#include "Update.h"
#include "SceneEntityState.h"
#include "CameraRole.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
ACamera::ACamera()
{
}
// --------------------------------------------------------------------------
std::auto_ptr<CameraStateAccessor> ACamera::GetCameraStateAccessor()
{
	APhysicalState*	state = &GetPhysicalState();
	ACameraState*	cState = dynamic_cast<ACameraState*>(state);
	ASSERT(cState);

    return std::auto_ptr<CameraStateAccessor>(new CameraStateAccessor(*this, *cState));
}
// --------------------------------------------------------------------------
const std::auto_ptr<ConstCameraStateAccessor> ACamera::GetConstCameraStateAccessor() const
{
	const APhysicalState*	state = &GetPhysicalState();
	const ACameraState*	cState = dynamic_cast<const ACameraState*>(state);
	ASSERT(cState);

    return std::auto_ptr<ConstCameraStateAccessor>(new ConstCameraStateAccessor(*this, *cState));
}
// --------------------------------------------------------------------------
void ACamera::Goto(Time time)
{
	BaseClass::Goto(time);

	GotoForCameraRole(time);
}
// --------------------------------------------------------------------------
void ACamera::GotoForCameraRole(Time time)
{
	const int	cameraRoleIndex = GetCameraRoleIndex();

	if(cameraRoleIndex >= 0)
	{
		ARole& aRole = GetSceneEntityState().GetRole(cameraRoleIndex);

		CameraRole* cRole = dynamic_cast<CameraRole*>(&aRole);
		ASSERT(cRole);

		APhysicalState&	pState = GetPhysicalState();
		ACameraState*	cState = dynamic_cast<ACameraState*>(&pState);
		ASSERT(cState);

		cState->SetCameraState(cRole->GetState(time));
	}
}
// --------------------------------------------------------------------------
void ACamera::StateUpdated(Update::ID id)
{
	BaseClass::StateUpdated(id);
}
// --------------------------------------------------------------------------
void ACamera::StateUpdated(Update::ID id, Time time)
{
	BaseClass::StateUpdated(id, time);

	if(id == Update::kCameraHorizontalFOV || id == Update::kCameraVerticalFOV)
	{
		CameraStateUpdated(time);
	}
}
// --------------------------------------------------------------------------
void ACamera::CameraStateUpdated(Time time)
{
    const int	cameraRoleIndex = GetCameraRoleIndex();
		
	if(cameraRoleIndex >= 0)
	{
		ARole* aRole = &GetSceneEntityState().GetRole(cameraRoleIndex);

		CameraRole*   dRole = dynamic_cast<CameraRole*>(aRole);
		ASSERT(dRole);

		const APhysicalState*	state = &GetPhysicalState();
		const ACameraState*		cState = dynamic_cast<const ACameraState*>(state);

		const CameraState	cameraState(*cState);

		dRole->StateUpdated(cameraState, time);
	}
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
