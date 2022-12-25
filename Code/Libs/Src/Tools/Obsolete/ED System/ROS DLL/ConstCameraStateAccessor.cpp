// Author: Shaival Varma
//---------------------------------------------------------------------------
#include "PCH.h"
#include "ConstCameraStateAccessor.h"
#include "ACamera.h"
#include "ACameraState.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN ConstCameraStateAccessor::ConstCameraStateAccessor(const ACamera& owner, const ACameraState& state)
: mOwner(owner), mState(state)
{
}
// --------------------------------------------------------------------------
CPP_DEFN float ConstCameraStateAccessor::GetHorizontalFOV() const
{
    return GetState().GetHorizontalFOV();
}
// --------------------------------------------------------------------------
CPP_DEFN float ConstCameraStateAccessor::GetVerticalFOV() const
{
    return GetState().GetVerticalFOV();
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------