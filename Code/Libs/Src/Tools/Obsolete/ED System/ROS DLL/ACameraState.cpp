// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ACameraState.h"
// --------------------------------------------------------------------------
namespace ROS
{
//---------------------------------------------------------------------------
void ACameraState::SetCameraState(const ACameraState& cameraState)
{
	SetHorizontalFOV(cameraState.GetHorizontalFOV());
	SetVerticalFOV(cameraState.GetVerticalFOV());
}
//---------------------------------------------------------------------------
}