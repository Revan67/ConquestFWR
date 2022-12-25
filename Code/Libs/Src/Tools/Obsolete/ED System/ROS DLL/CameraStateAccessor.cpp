// Author: Shaival Varma
//---------------------------------------------------------------------------
#include "PCH.h"
#include "CameraStateAccessor.h"
#include "ACamera.h"
#include "ACameraState.h"
#include "CodeMsg.h"
#include "Update.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN CameraStateAccessor::CameraStateAccessor(ACamera& owner, ACameraState& state)
: mOwner(owner), mState(state)
{
}
// --------------------------------------------------------------------------
CPP_DEFN float CameraStateAccessor::GetHorizontalFOV() const
{
    return GetState().GetHorizontalFOV();
}
// --------------------------------------------------------------------------
CPP_DEFN float CameraStateAccessor::GetVerticalFOV() const
{
    return GetState().GetVerticalFOV();
}
// --------------------------------------------------------------------------
CPP_DEFN void CameraStateAccessor::SetHorizontalFOV(float hFOV)
{
	GetState().SetHorizontalFOV(hFOV);

	OwnerStateUpdated(Update::kCameraHorizontalFOV);
}
// --------------------------------------------------------------------------
CPP_DEFN void CameraStateAccessor::SetVerticalFOV(float vFOV)
{
    GetState().SetVerticalFOV(vFOV);

	OwnerStateUpdated(Update::kCameraVerticalFOV);
}
// --------------------------------------------------------------------------
CPP_DEFN ACamera& CameraStateAccessor::GetOwner()
{
	return mOwner;
}
// --------------------------------------------------------------------------
CPP_DEFN const ACamera& CameraStateAccessor::GetOwner() const
{
	return mOwner;
}
// --------------------------------------------------------------------------
CPP_DEFN ACameraState& CameraStateAccessor::GetState()
{
	return mState;
}
// --------------------------------------------------------------------------
CPP_DEFN const ACameraState& CameraStateAccessor::GetState() const
{
	return mState;
}
// --------------------------------------------------------------------------
CPP_DEFN void CameraStateAccessor::OwnerStateUpdated(Update::ID id)
{
    mOwner.StateUpdated(id);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
