// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ACameraDynamicsState.h"
// --------------------------------------------------------------------------
/**# implementation ACameraDynamicsState:: id(C_0901743906) 
*/
//---------------------------------------------------------------------------
namespace ROS
{
//---------------------------------------------------------------------------
void ACameraDynamicsState::SetCameraDynamicsState(const ACameraDynamicsState& cameraDynamicsState)
{
	SetCameraState(cameraDynamicsState);
	SetStaticsState(cameraDynamicsState);
	SetDynamicsState(cameraDynamicsState);
}
//---------------------------------------------------------------------------
}